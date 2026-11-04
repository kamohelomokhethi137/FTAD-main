# FTAD — Testing Report

**Project:** Financial Transaction Anomaly Detector (FTAD)
**Source:** `/home/hashcat/Documents/dataset/FTAD-main`
**Generated:** 2026-11-02

---

## 1. Scope and tool chain

Seven testing tools were applied to the C++/Qt6/SQLite application and
its GoogleTest/CTest suite.

| # | Tool | Version / build | Result |
|---|------|-----------------|--------|
| 1 | GoogleTest / CTest | gtest v1.16.0 (FetchContent), CTest | **154 / 154 pass** |
| 2 | gcov + lcov + genhtml | lcov 2.0-1, gcovr 8.8 | **85.4% lines, 97.4% functions** |
| 3 | ASan + UBSan | LLVM 19 sanitizers | **154 / 154 pass**, no findings |
| 4 | Valgrind memcheck | 3.24.0 | **0 errors, 0 leaks** |
| 5 | cppcheck | 2.17.1 | **0 findings** |
| 6 | clang-tidy | 23.1.1 (VS Code bundle) | **13 design advisories**, 0 errors |
| 7 | Benchmark / performance | ftad_benchmark | 20,000 tx → ~27.6 ms |

---

## 2. GoogleTest / CTest — functional suite

**Command:**
```
QT_QPA_PLATFORM=offscreen /tmp/opencode/toolvenv/bin/ctest --output-on-failure
```

**Result:** `100% tests passed out of 154`

```
[==========] 154 tests from 10 test suites ran. (1.01 s total)
[  PASSED  ] 154 tests.
```

### 2.1 Suites

| Suite | Tests | Notes |
|-------|-------|-------|
| BehaviourAnalyzerTest | 10 | baseline ordering, z-score, uniform history |
| CsvImportTest | 14 | happy path, malformed rows, quoting, blank lines |
| DetectionPipelineTest | 12 | end-to-end analyse → save → investigate |
| DetectionSettingsTest | 14 | threshold / weight validation |
| IdentifierTest | 12 | canonicalisation rules |
| InvestigationRepositoryTest | 14 | cases, history, status transitions |
| RiskScorerTest | 10 | scoring, bands, determinism |
| TransactionRepositoryTest | 18 | CRUD, validation, ID allocation |
| TransactionTimeTest | 13 | timestamp parsing, windows, UTC |
| DatabaseTest | 18 | SQLite fixture (schema per test) |

---

## 3. Coverage — gcov / lcov / genhtml

```
Source files: 12
lines.......: 85.4% (992 of 1162 lines)
functions...: 97.4% (75 of 77 functions)

backend/include/DetectionResult.h           | 100%     6| 100%   3
backend/include/Investigation.h            | 100%     3| 100%   1
backend/include/TransactionTime.h          | 100%     8| 100%   1
backend/src/AnomalyDetector.cpp            | 93.2%   176| 100%   14
backend/src/BehaviorAnalyzer.cpp          | 96.5%    85| 100%    6
backend/src/CsvImportService.cpp          | 86.4%    177| 100%    5
backend/src/DatabaseManager.cpp           | 68.0%    75| 85.7%    7
backend/src/DetectionRepository.cpp      | 70.2%    114| 80.0%    5
backend/src/DetectionService.cpp         | 92.0%    50| 100%    7
backend/src/DetectionSettings.cpp        | 100%     44| 100%    4
backend/src/InvestigationRepository.cpp| 77.6%    174| 100%    8
backend/src/RiskScorer.cpp                | 97.2%    36| 100%    3
backend/src/TransactionRepository.cpp   | 86.4%    214| 100%    13
backend/include/TransactionRepository.h ... (additional headers 100%)
```

HTML report: `/tmp/opencode/coverage-html`
Raw data: `/tmp/opencode/coverage-backend.info`

Lowest coverage is `DatabaseManager` (68%) and
`InvestigationRepository` (77.6%), both dominated by connection lifecycle
and history-query paths that the fixture does not exercise deeply.

---

## 4. Memory — ASan/UBSan and Valgrind

### 4.1 AddressSanitizer + UndefinedBehaviorSanitizer

```
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 \
  QT_QPA_PLATFORM=offscreen ./tests/ftad_tests
```

**Result:** 154 / 154 pass, **0 sanitizer findings**, no leak reports.

### 4.2 Valgrind — memcheck

```
valgrind --leak-check=full --show-leak-kinds=definite,indirect \
        --errors-for-leak-kinds=definite,indirect --track-origins=yes \
        --error-exitcode=99 --log-file=... ./tests/ftad_tests
```

```
HEAP SUMMARY:   in use at exit: 20,913 bytes in 28 blocks
                total heap usage: 134,884 allocs, 134,856 frees

LEAK SUMMARY:
    definitely lost: 0 bytes in 0 blocks
    indirectly lost:  0 bytes in 0 blocks
    possibly lost:     0 bytes in 0 blocks
    still reachable: 20,913 bytes in 28 blocks

ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

**Result:** clean — no invalid reads/writes, no leaks, no UB reports.

---

## 5. Static analysis — cppcheck and clang-tidy

### 5.1 cppcheck 2.17.1

After applying the `useStlAlgorithm` and `duplicateCondition` hints,
a full source run produced:

```
cppcheck: 0 findings
```

(no errors, no warnings, no style/performance/portability issues).

### 5.2 clang-tidy 23.1.1

Checks: `-*,bugprone-*,performance-*,portability-*,modernize-*,readability-*,clang-diagnostic-*`

Final run: **13 warnings, 0 errors**.

All 13 remaining warnings are of a single advisory class:

```
bugprone-easily-swappable-parameters
```

(e.g. `timeWindow(from, to)`, `evaluate(index, zScore, ratio)`,
`BasePage(title, content, ...)`). These are design-level advisories and
were reviewed manually; the parameter semantics are documented by name at
each call site.

No `bugprone-*`, `performance-*`, `portability-*`, `modernize-*` or
`readability-*` findings of blocking severity remain.

---

## 6. Performance summary

Full numbers in `Reports/performance_report.md`.

Final state:

- 20,000 transactions analysed in **27.6 ms** (median).
- Behaviour baseline: **0.48 µs / transaction**.
- Scoring: **0.19 µs / transaction**.

Post-fix defect (see `Reports/bug_report.md`, BUG-06) resolved by
building timestamps in UTC.

---

## 7. Evidence index

| Evidence | Path |
|----------|------|
| Coverage data | `/tmp/opencode/coverage-backend.info` |
| Coverage HTML | `/tmp/opencode/coverage-html` |
| Valgrind log | `/tmp/opencode/valgrind-report2.txt` |
| ASan log | `/tmp/opencode/asan-report2.txt` |
| CTest log | `/tmp/opencode/ctest-final.txt` |
| cppcheck report | `/tmp/opencode/cppcheck-report2.txt` |
| clang-tidy report | `/tmp/opencode/clang-tidy-final2.txt` |
| Benchmark output | `/tmp/opencode/benchmark-report.txt` |
| Test binary | `/tmp/opencode/build-tests/tests/ftad_tests` |

---

## 8. Final status update (2026-11-02)

The session ended with all seven tools green and the final state recorded
in `Reports/FINAL_SESSION_REPORT.md`:

- GoogleTest/CTest: **154 / 154 pass** (10 suites).
- Coverage: **85.4% lines, 97.4% functions**.
- ASan/UBSan: clean. Valgrind: clean. cppcheck: **0 findings**.
- clang-tidy: 13 design advisories, 0 errors.
- Performance: **20,000 tx → 27.6 ms** (post UTC fix).

The FTAD application binary is built at
`/home/hashcat/Documents/dataset/FTAD-main/build/FTAD` and was running
under a virtual X display at the close of the session.

### Permanent toolchain fix (final action of the session)

`sudo apt-get install -y qt6-base-dev qt6-charts-dev cmake ninja-build`
was executed by the operator; the packages are now installed system-wide.
A permanent in-project build was then produced at
`/home/hashcat/Documents/dataset/FTAD-main/build` with the system Qt6,
with **0 errors / 0 warnings** and **154 / 154 tests passing**.

The desktop launcher and run script now point at that permanent build.

---

*End of testing report.*
