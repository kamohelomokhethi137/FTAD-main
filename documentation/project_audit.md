# Chapter 1 — Project Audit

> **Audit date:** 2 November 2026, against the commit state of the repository
> root as inspected that day. Every count below was produced by a command over
> the repository, not estimated.

---

## 1.1 Project Overview

FTAD — Financial Transaction Anomaly Detector — is a Linux desktop application
written in C++17 (`CMAKE_CXX_STANDARD 17`) with Qt 6 Widgets, Sql and Charts, that
surfaces suspicious financial transactions to a reviewer before they become fraud
losses. The problem it addresses is that duplicates, unusually large amounts, rapid
withdrawal sequences and behaviourally deviant transactions must be found by a human
who cannot read a whole ledger; the application ranks them so that attention goes
where the risk is. Its users are financial monitoring reviewers, and in its current
form it is a student group project for BIST4110.

The repository is organised as `backend/` (13 headers, 13 sources), `frontend/`
(8 headers, 8 sources), `database/` (`schema.sql`), `data/samples/`
(`test_transactions.csv`), `tests/` (nine suites plus a benchmark and support
harness), `docs/`, and `Reports/`, with `CMakeLists.txt` and `main.cpp` at the root.

## 1.2 Feature Inventory

The backend implements ten modules. `DatabaseManager` owns the SQLite connection,
schema initialisation and error state. `TransactionRepository` provides CRUD,
canonical identifier normalisation (`normalizeAccountId`, `normalizeTransactionId`),
rejection of duplicate supplied identifiers, validation and next-ID allocation.
`CsvImportService` performs header and row validation, quoting, blank-line handling,
duplicate-column detection and per-line error reporting. `DetectionSettings` holds
the thresholds, weights, bands and validation rules. `BehaviorAnalyzer` computes
per-account prefix statistics over strictly earlier transactions, producing a
z-score and a uniform-history ratio. `RiskScorer` performs weighted scoring, risk
level assignment, primary-method selection and reason generation. `AnomalyDetector`
implements the duplicate, large-amount, rapid-withdrawal and behaviour checks.
`DetectionRepository` persists and loads anomaly results, `DetectionService`
orchestrates analysis and flagged retrieval, and `InvestigationRepository` manages
cases, status transitions and history events.

The frontend implements `MainWindow`, `BasePage` and six pages — `HomePage`,
`TransactionsPage`, `DetectionPage`, `InvestigationsPage`, `ReportsPage` and
`SettingsPage` — under a `QStackedWidget` shell. Reporting export routines and
settings-persistence UI are **not implemented**; both pages are placeholders and
are documented as such rather than counted as features.

## 1.3 Architecture Observations

The backend compiles as a static library, `ftad_core`, which is linked by both the
Qt application and the test binary, so there is a single build and a single copy of
the code under test. Persistence is SQLite through the Qt `QSQLITE` driver with
foreign keys enforced, and the schema lives in `database/schema.sql`. The detection
pipeline runs transactions → analysis → results → anomalies → investigations, and
that boundary structure is what the layered test strategy in Chapter 4 follows.

## 1.4 SLOC Results

Lines were counted by a scripted pass over the source files (`.h`, `.cpp`, `.c`)
that excludes blank lines and comments; physical lines are reported alongside
because the two are routinely confused. Application sources — `backend/`,
`frontend/` and `main.cpp` — comprise **41 files**, **5,948** physical lines and
**4,612** executable source-code lines. The whole repository including tests
comprises **54 files**, **9,128** physical and **6,980** executable lines. The
assignment threshold of 4,000 SLOC is therefore satisfied by the application sources
alone, at 4,612 executable lines.

The counting rule is stated because a SLOC figure without one is not comparable
with anything — the same point Kan makes about defect density in §2.3.

## 1.5 Existing Testing Infrastructure

GoogleTest 1.16.0 is fetched by CMake's `FetchContent` and driven by a custom
`QCoreApplication`-based main with `gtest_discover_tests`; a `DatabaseTest` fixture
supplies a fresh SQLite file per test. Ten suites exist —
`TransactionRepositoryTest`, `CsvImportTest`, `DetectionPipelineTest`,
`InvestigationTest`, `AnomalyDetector`, `BehaviorAnalyzer`, `RiskScorer`,
`DetectionSettings`, `Identifiers` and `TransactionTime` — totalling 154 tests, all
passing under CTest. Coverage is measured with gcov/lcov at 85.4 % of lines and
97.4 % of functions, and the toolchain extends to ASan/UBSan, Valgrind memcheck,
Cppcheck, Clang-Tidy and a project-local benchmark.

The audit's finding here is that the infrastructure pre-dated this campaign but had
not been executed as a whole: the tools were present in configuration while lcov,
Cppcheck, Clang-Tidy and Valgrind were absent from the machine. That gap became risk
R4 in Chapter 4 and was closed on 4 November.

## 1.6 Gaps and Compliance

Four gaps were open at audit time and their disposition is recorded rather than
silently resolved. Lecturer-specific formatting details could not be fully verified
from the brief and were interpreted conservatively. A Turnitin similarity percentage
has not been measured, so no originality figure is claimed. The ML training pipeline
does not exist — the detection engine is rule-based by design (§2.9), so this is a
scope decision rather than an omission. The final report was still a scaffold at
audit time; it has since been produced, and this chapter is part of it.

Against the assignment requirements the position is: C/C++ on Linux **satisfied**
(built, run and tested on Linux); more than 4,000 SLOC **satisfied** (4,612
executable application lines); testing performed **yes** (154 automated tests plus
seven analysis tools); bug report **yes** (`Reports/bug_report.md`); testing
strategy and tool documentation **yes** (Chapter 4 and Appendices B and D); report
deliverables **complete** in this document.
