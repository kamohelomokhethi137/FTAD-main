# Detector model decision — handoff note (2026-11-04)

Status: decision made, not yet implemented. Pick up from "Next steps".

## What was measured (committed as `5851a90`)

Evaluation of FTAD's rule engine and supervised models on the PaySim
fraud dataset (`/home/hashcat/Documents/dataset/archive/`, 6,362,620
rows, label `isFraud`, 8,213 frauds, 0.129 % base rate). Scripts print
and write their numbers; results also live in the JSON files alongside.

* `analysis/track_a_rules.py` — rule engine mirrored formula-for-formula
  from `backend/src/AnomalyDetector.cpp`; results in
  `analysis/track_a_results.json`.
* `analysis/track_b_model.py` — supervised models on a temporal split
  (train steps 1–372, test 373–744; 4,060 test frauds); results in
  `analysis/track_b_results.json`.
* Run environment for B: `/tmp/opencode/mlvenv/bin/python` (sklearn
  1.9.1, installed in the venv; system Python is untouched). A is plain
  pandas/numpy. The venv is NOT in the repo — recreate with
  `python3 -m venv --system-site-packages` + `pip install scikit-learn`
  if `/tmp` was cleaned.

## Track A result (rules vs ground truth, full dataset)

| Detector | Alerts | Precision | Recall | Why |
|---|---|---|---|---|
| large_amount @ 10k | 5,076,529 (79.8 %) | 0.0016 | 0.966 | fires, but the threshold is calibrated to the app's sample scale, not PaySim's |
| duplicate_payment | 0 | — | 0 | unfireable: zero (sender, receiver, same-step) pairs |
| rapid_withdrawals | 0 | — | 0 | unfireable: no sender makes 3 withdrawals in two adjacent steps |
| behaviour_deviation | 0 | — | 0 | unfireable: no sender has ≥6 transactions (mean 1.0015 tx/sender) |
| isFlaggedFraud (dataset's own rule) | 16 | 1.000 | 0.19 % | narrow-rule extreme |

The three zeros were cross-checked with independent groupby queries.
Finding: PaySim generates fraud from nearly-fresh sender accounts
(1.0015 transactions per sender), while FTAD's per-sender rules assume
recurring accounts. The rules are not buggy; the dataset starves them.

## Track B result (what learning adds)

Recall at fixed alert budgets on the test partition:

| Detector | PR-AUC | @ 20k alerts | @ 500k alerts |
|---|---|---|---|
| Logistic, restricted features (type + amount) | 0.044 | 0.162 | 0.889 |
| Gradient boost, restricted features | 0.259 | 0.407 | **1.000** |
| Logistic, full features | 0.601 | 0.810 | 0.997 |
| Gradient boost, full features | **0.946** | **0.999** | 1.000 |
| [ref] rules combined | — | 0.967 @ 865,448 alerts (P 0.0045) | — |
| [ref] isFlaggedFraud | — | P 1.000, R 0.003 @ 13 alerts | — |

Caveat that must survive any write-up: the full-feature PR-AUC is
inflated by the balance-ledger inconsistency features, which are an
artifact of how PaySim generates its ledger, not transferable signal.
The gap 0.259 → 0.946 measures that artifact.

## Decision

**Gradient-boosted decision trees (HistGradientBoostingClassifier),**
exported to plain generated C++, as the primary detector. Logistic
regression stays as the interpretable calibrator/baseline. Rules are
kept as a fallback layer for what PaySim cannot teach (duplicate
payment has no positive examples in the dataset).

Reasons, in order:

1. Evidence: restricted-features GBDT reaches recall 1.000 at a 0.23 %
   alert budget where the rule engine needs 80 % of traffic; on the
   same information (type + amount), logistic manages 0.044 PR-AUC
   vs 0.259.
2. Fit: FTAD's detector is already threshold logic; a tree ensemble is
   that logic, learned. RuleHit "reason" strings can carry the firing
   threshold instead of a summed weight.
3. Determinism: pure float inference, unit-testable, no seed at
   inference — preserves the RiskScorer determinism promise.
4. Serving: no Python runtime in the delivered app. Trees export as
   JSON → generated `.cpp` arrays; ~100 lines of evaluation code, zero
   dependencies.
5. Rejected: neural nets (no tabular edge, heavy integration),
   unsupervised methods (labels exist for every row), XGBoost/LightGBM
   as runtime (same family, heavier serving).

Training regime: **restricted features (type + amount)** for the honest
claim; full-features result kept as the published ablation with the
caveat above attached.

## Next steps

1. Train final HGB on restricted features; calibrate scores onto the
   existing 0–100 band scale (35 medium / 65 high preserved).
2. Export trees to generated C++ (JSON → arrays) plus an evaluator;
   keep the pure-deterministic property.
3. Unit test: run the Appendix C catalogue vectors through the
   exported model; document expected-score changes.
4. Decide on PaySim-calibrated `large_amount` default, or retire the
   rule into the fallback layer.
5. Scope decision (owner: operator): folding this into the committed
   report makes Chapter 5/6 no longer fully accurate — an edit pass there
   is deferred until this item is asked for.

Nothing here modifies application source (`backend/`, `frontend/`,
`tests/`); all analysis lives in `analysis/`.
