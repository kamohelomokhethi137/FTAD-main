#!/usr/bin/env python3
"""Track B: train models on PaySim's isFraud label and compare the rules.

Track A established what FTAD's rule set achieves on this dataset: one
rule fires (great_amount, 0.966 recall at alert rate of 0.798) and the
three per-sender rules are unfireable because PaySim gives each sender
about one transaction. Track B asks whether a supervised model does
better, and - deliberately - in two feature regimes:

  * RESTRICTED features: transaction type + amount. The same information
    the amount rule uses, so gap model-vs-rule isolates learning.
  * FULL features: adds the balance-ledger error variables known to
    almost solve PaySim (E. errOrig: oldbalanceOrg - amount does not
    equal newbalanceOrig; errDest analog). They are artifacts of this
    dataset's generation process; that is the point of running both
    regimes, since the gap between the two quantifies how much of the
    headline quality is shortcut rather than signal.

Split is temporal: odd steps train on the first half of the month and
even steps on the second, stated explicitly because random splits on a
generated dataset overstate stability. Results in track_b_results.json.

Run with the venv interpreter:
    /tmp/opencode/mlvenv/bin/python track_b_model.py
"""

import json
import pathlib
import time

import numpy as np
import pandas as pd

from sklearn.linear_model import LogisticRegression
from sklearn.ensemble import HistGradientBoostingClassifier
from sklearn.metrics import average_precision_score, roc_auc_score

HERE = pathlib.Path(__file__).resolve().parent
_CANDIDATES = [
    pathlib.Path(__file__).resolve().parent.parent / "archive" / "PS_20174392719_1491204439457_log.csv",
    pathlib.Path("/home/hashcat/Documents/dataset/FTAD-main/archive/PS_20174392719_1491204439457_log.csv"),
    pathlib.Path("/home/hashcat/Documents/dataset/archive/PS_20174392719_1491204439457_log.csv"),
]
CSV = next((p for p in _CANDIDATES if p.exists()), _CANDIDATES[0])

SEED = 4110
TYPE_ORDER = ["CASH_IN", "CASH_OUT", "DEBIT", "PAYMENT", "TRANSFER"]


def load():
    df = pd.read_csv(
        CSV,
        usecols=["step", "type", "amount", "oldbalanceOrg",
                 "newbalanceOrig", "oldbalanceDest", "newbalanceDest",
                 "isFraud", "isFlaggedFraud"],
        dtype={"step": "int16", "type": "category", "amount": "float32",
               "oldbalanceOrg": "float32", "newbalanceOrig": "float32",
               "oldbalanceDest": "float32", "newbalanceDest": "float32",
               "isFraud": "int8", "isFlaggedFraud": "int8"},
    )
    # Ledger-error features. Amounts have two decimals, so a 0.01
    # tolerance separates a genuine rounding mismatch from a noisy one.
    gap_orig = df["oldbalanceOrg"].to_numpy() - df["amount"].to_numpy() \
        - df["newbalanceOrig"].to_numpy()
    gap_dest = df["oldbalanceDest"].to_numpy() + df["amount"].to_numpy() \
        - df["newbalanceDest"].to_numpy()
    df["errOrig"] = (np.abs(gap_orig) > 0.01).astype(np.int8)
    df["errDest"] = (np.abs(gap_dest) > 0.01).astype(np.int8)
    old = df["oldbalanceOrg"].to_numpy()
    df["amtRatio"] = np.where(
        old > 0, df["amount"].to_numpy() / old, 0.0).astype(np.float32)
    df["typeC"] = df["type"].map(lambda s: TYPE_ORDER.index(s))
    df["amtLog"] = np.log1p(df["amount"].to_numpy()).astype(np.float32)
    return df


def alert_budget_metrics(y_true, score, budget, total, pos):
    """Recall and precision among the `budget` highest-scored rows."""
    k = min(budget, total)
    if k >= total:
        flag = np.ones(total, bool)
    else:
        thresh = np.partition(score, total - k)[total - k]
        flag = score >= thresh
    tp = int(np.sum(flag & (y_true == 1)))
    return dict(budget=int(budget), tp=tp,
                recall=round(tp / pos, 4),
                precision=round(tp / max(int(flag.sum()), 1), 4))


def evaluate(name, model, Xtr, ytr, Xte, yte, budgets):
    t0 = time.time()
    # Class weights proportional to inverse frequency, applied as sample
    # weights so LogisticRegression and HGB share one mechanism.
    pos_w = max((ytr == 0).sum() / max((ytr == 1).sum(), 1), 1.0)
    w = np.where(ytr == 1, pos_w, 1.0).astype(np.float32)
    model.fit(Xtr, ytr, sample_weight=w)
    score = model.predict_proba(Xte)[:, 1]
    ap = average_precision_score(yte, score)
    ra = roc_auc_score(yte, score)
    pos = int(yte.sum())
    budgets_m = [alert_budget_metrics(yte, score, b, len(yte), pos)
                 for b in budgets]
    print(f"  {name:34} PR-AUC {ap:.4f}  ROC {ra:.4f}   "
          f"({time.time() - t0:.0f} s)")
    return dict(pr_auc=round(float(ap), 4), roc_auc=round(float(ra), 4),
                budgets=budgets_m)


def main():
    df = load()
    n = len(df)
    y_all = df["isFraud"].to_numpy()
    train = (df["step"] <= 372).to_numpy()
    ytr = y_all[train]
    yte = y_all[~train]

    FEATURE_SETS = {
        "restricted (type + amount)": ["typeC", "amtLog"],
        "full (ledger errors + ratio)": ["typeC", "amtLog", "errOrig",
                                         "errDest", "amtRatio"],
    }
    MODELS = {
        "logistic": LogisticRegression(max_iter=200, C=1.0),
        "hist-grad-boost": HistGradientBoostingClassifier(
            max_iter=300, early_stopping=True, random_state=SEED),
    }

    pos = int(yte.sum())
    budgets = (20000, 100000, 500000, int(round(n * 0.798)))
    # The last budget reproduces the rule regime's alert rate on the
    # test partition, so recall@rule-alerts is directly comparable.

    print(f"rows {n:,} | train {int(train.sum()):,} | test "
          f"{int((~train).sum()):,} | test frauds {pos:,} "
          f"({pos / yte.size * 100:.3f} %)")
    results = {"split": "step <= 372 train, step 373-744 test",
               "test_rows": int((~train).sum()), "test_frauds": pos,
               "models": {}}

    for fs_name, cols in FEATURE_SETS.items():
        Xtr = df.loc[train, cols].to_numpy()
        Xte = df.loc[~train, cols].to_numpy()
        for m_name, model in MODELS.items():
            key = f"{m_name} [{fs_name}]"
            results["models"][key] = evaluate(
                key, model, Xtr, ytr, Xte, yte, budgets)

    # Reference rows, evaluated on the same test partition.
    for label, flag in (
            ("rule combined (large_amount@10k)",
             df["amount"].to_numpy() > 10000.0),
            ("isFlaggedFraud (dataset rule)",
             df["isFlaggedFraud"].to_numpy() == 1)):
        f_test = flag[~train]
        tp = int(np.sum(f_test & (yte == 1)))
        results.setdefault("references", {})[label] = dict(
            tp=tp, recall=round(tp / pos, 4),
            precision=round(tp / max(int(f_test.sum()), 1), 6),
            alerts=int(f_test.sum()))

    out = HERE / "track_b_results.json"
    out.write_text(json.dumps(results, indent=2))

    print("\nalert-budget table (test partition):")
    hdr = f"  {'model':42}" + "".join(f"B={b:>7,}" for b in budgets)
    print(hdr)
    for key, m in results["models"].items():
        cells = "".join(f"{b['recall']:>8.4f}" for b in m["budgets"])
        print(f"  {key:42}{cells}")
    for label, ref in results["references"].items():
        print(f"  [ref] {label:36} recall {ref['recall']:.4f}  "
              f"precision {ref['precision']:.4f}  "
              f"alerts {ref['alerts']:,}")
    print(f"\nwritten {out}")


if __name__ == "__main__":
    main()
