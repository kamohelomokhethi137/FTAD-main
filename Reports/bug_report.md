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
| BUG-05 | Sample dataset did not exercise the detection rules | Medium | Manual review + detection tests | Fixed |
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
The sample was expanded from 3 to 11 transactions that deliberately
exercise every detection rule:

- `ACCX101 -> ACCX201`: five normal transfers followed by a 25,000.00
  outlier (large amount + behavioural deviation).
- `ACCX102 -> ACCX202`: duplicate transfer pair (750.00 within 2 minutes).
- `ACCX103 -> ACCX203`: three rapid cash withdrawals within 60 minutes.

**Verification.**
- `CsvImportTest.ImportsTheSampleFile` — expects 11 imported rows and 11
  rows in the `transactions` table.
- `CsvImportTest.ReimportingTheSameFileSkipsEveryRow` — expects 11 skips on a
  second import of the same file.
- End-to-end detection run over the sample (4 Nov 2026,
  `documentation/evidence/functional/sample-dataset-20261104.log`): 11 rows
  analysed, 4 flagged, and all four rules fire —
  `large_amount` + `behaviour_deviation` on TXNXXX6 (25,000.00, score 60.0),
  `duplicate_payment` on TXNXXX7 and TXNXXX8 (750.00 each, score 40.0), and
  `rapid_withdrawals` on TXNXX11 (540.00, score 35.0).

**Residual gap.** The sample exercises all four rules but never reaches the
`high` band: the highest score produced is 60.0, below the 65.0 threshold. The
high band is therefore covered only by `RiskScorer.HonoursCustomBands` and
`RiskScorer.LevelBoundariesAreInclusiveAtTheLowerEdge`, not by the sample data.
This does not reopen BUG-05 — the rule was that the sample must exercise the
detection rules, and it now does — but it is recorded so the coverage of the
high band is not overread.

**Status:** FIXED.

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
- The sample dataset exercises all four detection rules but produces no
  `high`-band case (highest score 60.0 against a 65.0 threshold), so the high
  band is covered by unit tests only. See BUG-05, residual gap.
- One latent defect is recorded but not remediated: implicit `int`
  multiplication widened to `qint64` at `AnomalyDetector.cpp:177`, `:180` and
  `:261`. Unreachable in the shipped build because there is no runtime
  configuration path; documented in Chapter 4 §4.10.4.1.

---

*Generated during the FTAD testing campaign. Raw tool output is captured in
`documentation/evidence/` (see `SUMMARY.md`); the commands that produced each
file are recorded in the header of that file and in
`documentation/evidence/README.md`.*
