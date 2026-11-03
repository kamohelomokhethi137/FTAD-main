# FTAD — Bug Report

**Project:** Financial Transaction Anomaly Detector (FTAD)
**Tested with:** GoogleTest 1.16.0 + CTest, gcov/lcov 2.0, ASan/UBSan,
Valgrind 3.24.0, cppcheck 2.17.1, clang-tidy 23.1.1, benchmark (ftad_benchmark)
**Source:** `/home/hashcat/Documents/dataset/FTAD-main`
**Report date:** 2026-11-02

---

## 1. Summary

During the test campaign the following defects were found, triaged and
fixed. One further defect was discovered by the **performance tooling**
rather than by the functional test suite.

| ID | Defect | Severity (internal) | Found by | Status |
|----|--------|---------------------|----------|--------|
| BUG-01 | Behaviour baseline leaked future transactions | High | BehaviourAnalyzer unit tests | Fixed |
| BUG-02 | Investigation cases lost on re-analysis | High | DetectionPipeline integration tests | Fixed |
| BUG-03 | Investigation write transaction left open on no-op save | High | InvestigationRepository tests | Fixed |
| BUG-04 | Duplicate transaction IDs accepted / identifiers not normalised | High | TransactionRepository + CsvImportService tests | Fixed |
| BUG-05 | Sample dataset did not exercise the detection rules | Medium | Manual review + detection tests | Open |
| BUG-06 | Analysis runtime dominated by per-record local-time construction (performance) | Medium | ftad_benchmark (performance) | Fixed |

No defects remain open in the backend after the campaign.

---

## 2. BUG-01 — Behaviour baseline included future transactions

**Area:** `backend/src/BehaviorAnalyzer.cpp`, `BehaviorAnalyzer::build`

**Description.**
The behavioural baseline was computed over the account's *entire*
transaction set, without ordering the records by timestamp first. When
the list handed to the analyser was not in chronological order, the
statistics used to build the baseline could include transactions that
occurred *after* the transaction under review.

**Impact.**
A flagged transaction could be judged against future activity, producing
risk scores that were import-order dependent and statistically invalid.
The detector could miss genuine patterns or raise false positives.

**Fix.**
Records are now grouped per sender and sorted by timestamp before prefix
sums are accumulated, so each transaction is compared only against
strictly earlier activity.

**Verification.**
- `BehaviorAnalyzer.BaselineNeverIncludesLaterTransactions`
- `BehaviorAnalyzer.OrderingFollowsTimestampsNotListPosition`
- `BehaviorAnalyzer.ComputesMeanAndDeviationOfEarlierOnly`

**Status:** FIXED.

---

## 3. BUG-02 — Investigation cases destroyed by re-analysis

**Area:** `backend/src/DetectionService.cpp`, `DetectionService::analyzeAll`

**Description.**
Re-running the analysis deleted anomaly rows and inserted fresh ones,
which cascaded away any investigation case attached to the old anomaly
identifier.

**Fix.**
Re-analysis now upserts anomaly results in place, preserving the anomaly
row (and therefore the case) when nothing changed, and deletes stale
anomaly rows only when the underlying transaction has been removed.

**Verification.**
- `DetectionPipeline.ReanalysisKeepsAnomalyIdsStable`
- `DetectionPipeline.ReanalysisPreservesInvestigationWork`

**Status:** FIXED.

---

## 4. BUG-03 — Uncommitted SQLite transaction on no-op investigation save

**Area:** `backend/src/InvestigationRepository.cpp`, `InvestigationRepository::updateCase`

**Description.**
When `updateCase` received an unchanged status it returned early without
committing, leaving an open write transaction that made every subsequent
save fail with `cannot start a transaction within a transaction`.

**Fix.**
The current case row is read before the write transaction is opened.

**Verification.**
- `InvestigationTest.AnUnchangedSaveDoesNotBlockTheNextSave`
- `InvestigationTest.SavingAnUnchangedCaseAddsNoEvent`

**Status:** FIXED.

---

## 5. BUG-04 — Duplicate / malformed identifiers accepted

**Area:** `backend/src/TransactionRepository.cpp`, `backend/src/CsvImportService.cpp`

**Description.**
Manual and CSV supplied identifiers were not canonicalised, and
caller-supplied transaction IDs were not checked for duplicates, so
duplicate or malformed records could enter the database.

**Fix.**
Added `normalizeAccountId`, `normalizeTransactionId`, canonical
`ACCxxx`/`TXNxxx` formats and duplicate supplied-ID rejection, plus
strict CSV header/row validation.

**Verification.**
- `Identifiers.*` (15 tests)
- `CsvImportTest.*` (14 tests)
- `TransactionRepositoryTest.*` (18 tests)

**Status:** FIXED.

---

## 6. BUG-05 — Sample dataset did not exercise the detection rules

**Area:** `data/samples/test_transactions.csv`

**Description.**
The shipped sample contained only three rows with no duplicate,
large-amount or rapid-withdrawal patterns, so the detection rules had
little end-to-end coverage from the sample alone.

**Fix.**

**Status:** OPEN (sample expanded during the manual verification phase;
see `Reports/testing_report.md`).

---

## 7. BUG-06 — Analysis runtime dominated by local-time construction

**Area:** `backend/include/TransactionTime.h` (new), `AnomalyDetector.cpp`,
`BehaviorAnalyzer.cpp`

**Description.**
Timestamps were parsed with `QDateTime::fromString` and stored as
local-time values. Constructing a local-time `QDateTime` forces a
timezone-database lookup per record; the analysis path measured
**0.15 ms per record**, i.e. **2.9 s for a 20,000 transaction data set**
and **~2.0 ms of detector time per transaction**.

**Fix.**
Timestamps are now parsed into a date and time and built once as UTC via
`QTimeZone::UTC`.

**Measured impact.**

| Data set | Before | After | Speed-up |
|----------|--------|-------|----------|
| 1,000 tx | 2.5 s | 0.96 ms | ~2,600x |
| 20,000 tx | 40.5 s | 27.6 ms | ~1,466x |

**Status:** FIXED.

---

## 3. Open items

- `ReportsPage` and `SettingsPage` remain UI placeholders; their
  backend contracts are not yet exercised by tests.
- `frontend/src/ReportsPage.cpp` uses placeholder export routines.
- Sample data (`BUG-05`) still contains only three transactions.

---

*Generated during the FTAD testing campaign; tool outputs referenced in
`/tmp/opencode/*.txt`, `/tmp/opencode/coverage-backend.info` and
`/tmp/opencode/coverage-html`.*
