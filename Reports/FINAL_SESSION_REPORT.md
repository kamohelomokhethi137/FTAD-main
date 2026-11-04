# FTAD — Final Report (Session Record)

**Project:** Financial Transaction Anomaly Detector (FTAD)
**Date:** 2026-11-02
**Source root:** `/home/hashcat/Documents/dataset/FTAD-main`
**Build output:** `/home/hashcat/Documents/dataset/FTAD-main/build`
**Working session:** testing campaign, defect fixes, performance work, launcher, CSV expansion, permanent toolchain fix

---

## 1. Executive summary

The FTAD application is **built, tested, running, and fully reported**.
The permanent system toolchain fix was executed, the build moved into
the project root, and the application now launches from the permanent
build.

## 2. Final testing results (all seven tools)

| # | Tool | Version / Build | Final result |
|---|------|-----------------|--------------|
| 1 | GoogleTest + CTest | gtest v1.16.0, CTest | **154 / 154 pass** (10 suites, ~3.63 s) |
| 2 | gcov + lcov + genhtml | lcov 2.0-1, genhtml | **85.4% lines** (992/1162), **97.4% functions** (75/77) |
| 3 | ASan + UBSan | LLVM 19 | **154 / 154 pass**, 0 findings, no leaks |
| 4 | Valgrind memcheck | 3.24.0 | **0 errors**, 0 leaks |
| 5 | cppcheck | 2.17.1 | **0 findings** |
| 6 | clang-tidy | 23.1.1 | 13 design advisories, 0 errors |
| 7 | Performance / benchmark | ftad_benchmark | 20,000 tx → **27.6 ms** median |

## 3. Defects resolved during the campaign

- **BUG-01** Behaviour baseline included future transactions → fixed.
- **BUG-02** Investigation cases lost on re-analysis → fixed (upsert).
- **BUG-03** Investigation no-op save left transaction open → fixed.
- **BUG-04** Duplicate / malformed identifiers accepted → fixed.
- **BUG-05** Sample dataset did not exercise the detection rules → fixed
  (expanded 3 → 11 rows).
- **BUG-06** Local-time `QDateTime` → ~2 ms/tx → fixed with UTC parse.

## 4. Performance

- Before: 20,000 tx analysis ≈ **40.5 s** (2.0 ms/tx).
- After: 20,000 tx analysis ≈ **27.6 ms** (1.38 µs/tx).
- Root cause: local-time construction forced timezone lookup per record.
- Fix: `backend/include/TransactionTime.h` builds UTC instants.

## 5. Deliverables

- `Reports/bug_report.md`
- `Reports/performance_report.md`
- `Reports/testing_report.md`
- `Reports/FINAL_SESSION_REPORT.md` (this file)
- `docs/latex/FTAD_Report.tex`
- `data/samples/test_transactions.csv` (expanded: 3 → 11 rows)
- Launcher: `/home/hashcat/Documents/dataset/FTAD-main/ftad.desktop`
- Run script: `/home/hashcat/Documents/dataset/FTAD-main/run.sh`

## 6. Application launch state

- App binary: `/home/hashcat/Documents/dataset/FTAD-main/build/FTAD`
- Currently running: same binary (pid 122068) under Wayland display.
- Desktop launcher: `/home/hashcat/Documents/dataset/FTAD-main/ftad.desktop`
- Run script: `/home/hashcat/Documents/dataset/FTAD-main/run.sh`

## 7. Permanent toolchain fix

- `sudo apt-get install -y qt6-base-dev qt6-charts-dev cmake ninja-build`
  was run by the operator and completed successfully.
- Packages now installed system-wide: `qt6-base-dev`, `qt6-charts-dev`,
  `cmake` 3.31.6, `ninja-build` 1.12.1.
- Permanent build configured and built in-project at
  `/home/hashcat/Documents/dataset/FTAD-main/build` using the system Qt6.

## 8. Current open questions / next steps

1. ML model is **not trained** — engine is rule-based statistical.
2. `ReportsPage` / `SettingsPage` remain UI placeholders.

---

*This final report consolidates the FTAD testing campaign, defect fixes and
performance work for the BIST4110 group project submission.*
