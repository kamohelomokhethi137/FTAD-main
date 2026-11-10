# ML Integration — Testing Report (BIST4110 alignment)

**Project:** FTAD — Financial Transaction Anomaly Detector (C++17/Qt 6/SQLite)  
**Scope:** Integration of a PaySim-trained supervised model into the backend detector, tested as an incremental change.  
**Date:** 2026-11-09 (UTC evidence timestamps in `documentation/evidence/ml-integration-20261109/`)  
**Evidence root:** `documentation/evidence/ml-integration-20261109/`

## 1. Objective and alignment to assignment

The assignment requires testing a Linux C/C++ application, documenting test strategy/plan/cases/execution, bug/performance/security reporting, and using appropriate testing techniques/tools. This report documents testing performed for the ML-model integration:

- verify the trained model artifact can be used deterministically from C++ (no Python runtime);
- verify integration does not regress existing behavior (defaults preserved);
- verify configuration/engine selection is validated;
- run regression, static analysis, dynamic/memory checks (targeted), and a performance benchmark;
- record evidence and limitations honestly.

## 2. Change under test

| Component | Change | Default behavior impact |
|---|---|---|
| `DetectionSettings` | Added `useRuleEngine` (default `true`), `useMlModel` (default `false`); validation rejects “no engines enabled”. | **None** when defaults used. |
| `AnomalyDetector` | Runs rule checks when `useRuleEngine`; emits `ml_model` hit with calibrated 0–100 score when `useMlModel`. | **None** when defaults used. |
| `MlFraudModel` | Generated deterministic HGB inference (`typeC` + `amtLog`) from `analysis/artifacts/hgb_export.json`. | Not invoked unless enabled. |
| Build/tests | `ftad_core` includes `MlFraudModel`; added `tests/test_ml_model.cpp`. | Test count 154 → 158. |

Training artifact: `HistGradientBoostingClassifier` (restricted features: transaction type index + `log1p(amount)`), PaySim temporal split `step<=372`/`373-744`, `seed=4110`, 71 iterations, PR-AUC 0.2590 / ROC-AUC 0.9201 on test partition (analysis-grade metric; see `analysis/MODEL_STATUS.md`).

## 3. Test strategy (levels, techniques, types)

| Level | What | Technique(s) | Evidence |
|---|---|---|---|
| Unit | ML inference determinism/bounds; score ordering sanity; settings validation | Functional assertions, boundary/value checks, equivalence (riskier type/amount vs low-risk) | `test_ml_model.cpp`, `ctest-ml-verbose.log` |
| Integration | `AnomalyDetector` with ML enabled/rules disabled produces `ml_model` finding | Component integration, configuration-based behavior | `AnomalyDetector.MlModelCanFlagWhenEnabled` |
| Regression/System | Full existing suite still passes with new sources linked | Regression, automation-first | `ctest-full.log` (158/158) |
| Static verification | Source inspection by analyzer | Static analysis | `cppcheck-app.log` |
| Dynamic verification | Memory/leak behavior on ML-related paths | Valgrind memcheck (targeted) | `valgrind-ml.log` |
| Performance | Post-change benchmark run | Benchmark/measurement (non-controlled host) | `benchmark.log` |

## 4. Test cases (ML integration)

| ID | Case | Technique | Expected | Result | Evidence |
|---|---|---|---|---|---|
| ML-01 | `MlFraudModel.ProducesDeterministicScores` | Determinism, bounds | Same input → same score; `0 <= score <= 100` | Pass | `ctest-ml-verbose.log` |
| ML-02 | `MlFraudModel.HigherRiskForLargeTransferThanSmallCashIn` | Equivalence/risk ordering | Large `TRANSFER` scores higher than small `CASH_IN` | Pass | `ctest-ml-verbose.log` |
| ML-03 | `AnomalyDetector.MlModelCanFlagWhenEnabled` | Integration/config | With `useRuleEngine=false,useMlModel=true`, result contains `ml_model`, score > 0 | Pass | `ctest-ml-verbose.log` |
| ML-04 | `DetectionSettings.RejectsNoEnabledEngines` | Validation/negative | `useRuleEngine=false,useMlModel=false` → `validate()` false | Pass | `ctest-ml-verbose.log` |
| REG-01 | Full CTest regression | Regression | All tests pass | **158/158 pass** | `ctest-full.log` |

## 5. Test execution summary

- **Environment:** recorded in `environment.txt` (Linux, GCC 14.2.0, CMake 3.31.6, CTest 3.31.6, Cppcheck 2.17.1, Valgrind 3.24.0).
- **Build:** `cmake -B build -G Ninja -DFTAD_BUILD_TESTS=ON && ninja -C build` (successful).
- **Regression:** `ctest --test-dir build --output-on-failure` → `100% tests passed, 0 tests failed out of 158`, total ~3.59 s.
- **ML-focused (verbose):** 4/4 passed, ~0.02 s.
- **Cppcheck (app scope):** `backend/src frontend/src -I backend/include`; no application errors/warnings emitted as defects in the captured output; informational `normalCheckLevelMaxBranches` notes present.
- **Valgrind (ML-focused tests):** `ERROR SUMMARY: 0 errors from 0 contexts`; `definitely/indirectly/possibly lost: 0`; `still reachable: 17,456 bytes in 20 blocks` (Qt/glib/global registries — not treated as application leaks).
- **Benchmark:** `ftad_benchmark` completed; post-integration scaling remains near-linear in reported run (host noise applies; do not compare small deltas to historical baselines without controlled reruns).

## 6. Defect / risk notes

| ID | Type | Description | Status | Notes |
|---|---|---|---|---|
| ML-DEF-01 | Resolved during integration | Generated C++ had malformed float literals (e.g. scientific values suffixed incorrectly / integer literals like `0f` in strict contexts), causing compile errors. | Fixed | Exporter formatting corrected; `ftad_core` rebuilt successfully. |
| ML-RISK-01 | Calibration/methodology | ML score is mapped to 0–100 by piecewise calibration for band compatibility; not a clinically/forensically validated fraud probability. | Known limitation | Treat as decision-support signal; defaults keep ML off. |
| ML-RISK-02 | Data realism | Trained on PaySim (synthetic). Full-feature ledger artifacts are excluded; restricted features used, but domain shift from real bank data remains. | Known limitation | Documented in `documentation/model_decision.md`. |
| ML-RISK-03 | Coverage of new behavior | ML-enabled path covered by targeted tests; UI has no toggle and default config does not exercise ML in normal runs. | Known limitation | Add UI/settings persistence tests if exposed. |
| ML-RISK-04 | Dedicated sanitizers | ASan/UBSan dedicated rebuild was not rerun for this incremental change; Valgrind targeted run was clean. | Partially mitigated | Re-run `-fsanitize=address,undefined` build for full confidence. |

No open functional regression was observed.

## 7. Security considerations (incremental)

- Model inference is pure/local C++ computation over in-memory transaction fields; no network access, file parsing, SQL, shell execution, or secrets added.
- Input to model: transaction type (normalized to known buckets; unknown maps to a default bucket) and amount (`log1p(max(amount,0))`) — avoids NaN/domain issues for non-negative amounts.
- Existing SEC-01/SEC-03 project-level risks (DB permissions, no encryption at rest) are unchanged by ML integration.

## 8. Compliance / traceability notes

- **Determinism preserved:** model inference and tests are deterministic for identical inputs; default engine remains rule-based.
- **Evidence-based:** raw command outputs stored under `documentation/evidence/ml-integration-20261109/`.
- **Honesty about scope:** this is integration testing for an ML component added to an existing, previously tested system; it does not replace full campaign gates (coverage, full ASan/UBSan, acceptance GUI evidence) except where explicitly captured.

## 9. Conclusion

The ML model (restricted HGB, type+amount, PaySim-trained) was integrated as an optional backend detector with generated C++ inference. Default behavior remains unchanged. All 158 automated tests passed, including 4 new ML/config/integration tests; targeted Valgrind checks reported no errors/leaks attributable to the change; Cppcheck showed no application defects in the captured app-scope run. The integration is suitable for controlled use/experimentation when `useMlModel` is enabled, with documented calibration/domain limitations.