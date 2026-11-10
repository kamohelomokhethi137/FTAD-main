# FTAD Model Status

**Date:** 2026-11-09  
**Scope:** Documents the trained ML model artifact and the model currently shipped in the FTAD application.

## Trained / Selected Model (Analysis Artifact)

- **Model:** `HistGradientBoostingClassifier` (`sklearn.ensemble.HistGradientBoostingClassifier`, scikit-learn)
- **Feature regime:** **Restricted** (honest claim)
  - `typeC` — index into `['CASH_IN', 'CASH_OUT', 'DEBIT', 'PAYMENT', 'TRANSFER']`
  - `amtLog` — `log1p(amount)`
- **Dataset:** PaySim (`archive/PS_20174392719_1491204439457_log.csv`, 6,362,620 rows)
- **Split:** Temporal — `step <= 372` train / `step 373-744` test
  - Train rows: 5,285,888
  - Test rows: 1,076,732
  - Test frauds: 4,060
- **Training config:** `seed=4110`, `max_iter=300`, `early_stopping=true`, class-weighted samples
- **Test metrics (restricted HGB):**
  - PR-AUC: **0.2590**
  - ROC-AUC: **0.9201**
  - Alert-budget recall/precision: 20k `R=0.4067, P=0.0817`; 100k `R=0.7323, P=0.0295`; 500k `R=1.0000, P=0.0081`

### Artifacts

| Artifact | Path |
|---|---|
| Trained model | `analysis/artifacts/ftad_hgb_restricted.joblib` |
| Metrics | `analysis/artifacts/ftad_hgb_restricted_metrics.json` |
| Trainer script | `analysis/train_model.py` |
| Comparison script | `analysis/track_b_model.py` |
| Prior comparison results | `analysis/track_b_results.json` |
| Model decision note | `documentation/model_decision.md` |

### Re-run

```bash
# from repo root
.venv-ml/bin/python analysis/train_model.py
.venv-ml/bin/python analysis/track_b_model.py
```

## Shipped in FTAD Application (C++/Qt)

- **Current shipped detector:** **Rule-based by default, ML-capable when enabled**.
  - `DetectionSettings` now has `useRuleEngine` (default `true`) and `useMlModel` (default `false`) to preserve existing deterministic behavior.
  - `AnomalyDetector` can emit an `ml_model` hit when `useMlModel=true`, using generated C++ inference from the trained HGB (`backend/include/MlFraudModel.h`, `backend/src/MlFraudModel.cpp`, generated via `analysis/export_hgb_cpp.py` from `analysis/artifacts/hgb_export.json`).
  - `DetectionService`/UI still use default settings unless configured; no UI toggle added yet.
- **Rules (default):** duplicate payment, large amount, rapid withdrawals, behaviour deviation (weighted scoring, capped at 100, banded low/medium/high). See `backend/include/DetectionSettings.h`, `backend/src/AnomalyDetector.cpp`, `README.md`.

## Caveats

- The HGB is the selected/implemented analysis model per `documentation/model_decision.md`, but **integration/export to C++ is not done yet**.
- Restricted features are used for the honest claim. Full-feature models can score higher on PaySim but may be inflated by ledger-artifact features (see `documentation/model_decision.md`).
- ML analysis lives in `analysis/`; application source remains unchanged.
