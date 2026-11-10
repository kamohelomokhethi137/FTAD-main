# ML integration evidence (2026-11-09)

Evidence for integrating the PaySim-trained restricted HGB model into FTAD as an optional detector (`useMlModel`, default off; rule engine remains default on).

## Files
- `environment.txt` — tool versions, host, timestamp.
- `ctest-full.log` — full regression: **158/158 passed**.
- `ctest-ml-verbose.log` — verbose run of ML-related tests (4 tests).
- `cppcheck-app.log` — Cppcheck 2.17.1, application scope (`backend/src`, `frontend/src`, includes `MlFraudModel.cpp`); no application errors reported (informational branch-limit notes only).
- `valgrind-ml.log` — Valgrind memcheck on ML-related tests; **0 errors**, definitely/indirectly/possibly lost 0; still-reachable blocks attributed to Qt/glib/global registries.
- `benchmark.log` — `ftad_benchmark` post-integration run (not a controlled benchmark; host noise applies).
- `asan-note.txt` — ML tests executed in default build; dedicated ASan/UBSan rebuild not re-run for this incremental change.
- `README.md` — this index.

## Reproduce
```bash
cmake -B build -G Ninja -DFTAD_BUILD_TESTS=ON
ninja -C build
ctest --test-dir build --output-on-failure
ctest --test-dir build -R 'MlFraudModel|MlModel|RejectsNoEnabledEngines' -V
cppcheck --enable=warning,style,performance,portability --inline-suppr --quiet -I backend/include backend/src frontend/src
valgrind --leak-check=full --show-leak-kinds=all ./build/tests/ftad_tests --gtest_filter='MlFraudModel.*:AnomalyDetector.MlModelCanFlagWhenEnabled:DetectionSettings.RejectsNoEnabledEngines'
./build/tests/ftad_benchmark
```
