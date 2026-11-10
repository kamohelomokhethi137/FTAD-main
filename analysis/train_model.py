#!/usr/bin/env python3
"""Train the selected FTAD detector model (HGB, restricted features) on PaySim.

Follows documentation/model_decision.md: restricted features (type + amount)
for the honest claim; temporal split step <=372 train, 373-744 test.
Saves a reusable model artifact and writes metrics alongside existing results.
"""
import json, pathlib, time
import numpy as np
import pandas as pd
from sklearn.ensemble import HistGradientBoostingClassifier
from sklearn.metrics import average_precision_score, roc_auc_score
import joblib

HERE = pathlib.Path(__file__).resolve().parent
_CANDIDATES = [
    HERE.parent / "archive" / "PS_20174392719_1491204439457_log.csv",
    pathlib.Path("/home/hashcat/Documents/dataset/FTAD-main/archive/PS_20174392719_1491204439457_log.csv"),
    pathlib.Path("/home/hashcat/Documents/dataset/archive/PS_20174392719_1491204439457_log.csv"),
]
CSV = next((p for p in _CANDIDATES if p.exists()), _CANDIDATES[0])
OUT = HERE / "artifacts"
OUT.mkdir(exist_ok=True)

SEED = 4110
TYPE_ORDER = ["CASH_IN", "CASH_OUT", "DEBIT", "PAYMENT", "TRANSFER"]
FEATURES = ["typeC", "amtLog"]

def load():
    df = pd.read_csv(
        CSV,
        usecols=["step", "type", "amount", "isFraud"],
        dtype={"step": "int16", "type": "category", "amount": "float32", "isFraud": "int8"},
    )
    df["typeC"] = df["type"].map(lambda s: TYPE_ORDER.index(s)).astype(np.int8)
    df["amtLog"] = np.log1p(df["amount"].to_numpy()).astype(np.float32)
    return df

def alert_budget_metrics(y_true, score, budget):
    total = len(y_true); pos = int(y_true.sum()); k = min(budget, total)
    thresh = np.partition(score, total - k)[total - k] if k < total else -np.inf
    flag = score >= thresh
    tp = int(np.sum(flag & (y_true == 1)))
    return dict(budget=int(budget), tp=tp, recall=round(tp / pos, 4),
                precision=round(tp / max(int(flag.sum()), 1), 4), alerts=int(flag.sum()))

t0 = time.time()
df = load()
print(f"loaded {len(df):,} rows from {CSV} ({time.time()-t0:.1f}s)")
train = (df["step"] <= 372).to_numpy()
Xtr, ytr = df.loc[train, FEATURES].to_numpy(), df.loc[train, "isFraud"].to_numpy()
Xte, yte = df.loc[~train, FEATURES].to_numpy(), df.loc[~train, "isFraud"].to_numpy()
pos_w = max((ytr == 0).sum() / max((ytr == 1).sum(), 1), 1.0)
w = np.where(ytr == 1, pos_w, 1.0).astype(np.float32)

model = HistGradientBoostingClassifier(max_iter=300, early_stopping=True, random_state=SEED)
print("training hist-grad-boost [restricted: type + amount]...")
model.fit(Xtr, ytr, sample_weight=w)
score = model.predict_proba(Xte)[:, 1]
metrics = {
    "model": "HistGradientBoostingClassifier",
    "features": FEATURES,
    "feature_semantics": {"typeC": f"index into {TYPE_ORDER}", "amtLog": "log1p(amount)"},
    "dataset": str(CSV),
    "split": "step <= 372 train, step 373-744 test",
    "rows": int(len(df)), "train_rows": int(train.sum()), "test_rows": int((~train).sum()),
    "test_frauds": int(yte.sum()),
    "pr_auc": round(float(average_precision_score(yte, score)), 4),
    "roc_auc": round(float(roc_auc_score(yte, score)), 4),
    "budgets": [alert_budget_metrics(yte, score, b) for b in (20000, 100000, 500000)],
    "seed": SEED, "max_iter": 300, "early_stopping": True,
}
artifact = OUT / "ftad_hgb_restricted.joblib"
joblib.dump({"model": model, "metrics": metrics, "type_order": TYPE_ORDER}, artifact)
(OUT / "ftad_hgb_restricted_metrics.json").write_text(json.dumps(metrics, indent=2))
print(json.dumps(metrics, indent=2))
print(f"saved {artifact}")
