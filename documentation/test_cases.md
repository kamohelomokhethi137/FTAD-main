# Appendix C — Test Case Catalogue

**Companion to:** Chapter 5, §5.3
**Status:** Reference material — excluded from the body page count

---

## C.1 Relationship to the Automated Suite

The 154 automated tests are not 154 hand-written cases. They are the executable
form of the 45 designed cases catalogued below, expanded where a case has
several distinguishable variants. This appendix records the designed cases, the
technique that produced each, the expected result stated *before* execution, and
the test that implements it.

The distinction matters for review. A test that exists but was never designed is
a test whose coverage cannot be argued; a case that was designed but never
implemented is a gap that can be seen and counted. Every row below has both.

Case identifiers follow `TC-<AREA>-<NNN>`. The numeric token is assigned in
creation order and never reused, so a deleted case leaves a visible gap rather
than silently renumbering its successors.

Expected results are stated as the property to be demonstrated, not as the
observed output. Where a case pins a comparison operator, the operator is named,
because that is the part most likely to change silently.

---

## C.2 Case Catalogue

| ID | Objective | Technique | Expected result | Test | Status |
|----------|---------------|-------------|-------------------|-----------------------|--------|
| `TC-DUP-001` | Duplicate payment flags both transactions | Decision table | Both TXNXXX1 and TXNXXX2 carry `duplicate_payment` | `DuplicatePaymentFlagsBothSides` | Pass |
| `TC-DUP-002` | Duplicate outside the time window is ignored | BVA | TXNXXX2 has no `duplicate_payment` | `DuplicateOutsideTheWindowIsNotFlagged` | Pass |
| `TC-DUP-003` | Duplicate outside the amount tolerance is ignored | BVA | TXNXXX2 has no `duplicate_payment` | `DuplicateOutsideAmountToleranceIsNotFlagged` | Pass |
| `TC-DUP-004` | Same amount to a different receiver is not a duplicate | Equivalence partitioning | No `duplicate_payment` | `DuplicateToADifferentReceiverIsNotFlagged` | Pass |
| `TC-LARGE-001` | Amount below the limit is not large | BVA | No `large_amount` | `AmountExactlyAtTheLimitIsNotLarge` | Pass |
| `TC-LARGE-002` | Amount exactly at the limit is not large | BVA | No `large_amount` — operator is strict `>` | `AmountExactlyAtTheLimitIsNotLarge` | Pass |
| `TC-LARGE-003` | Amount above the limit is large, without history | BVA | `large_amount` present, `behaviour_deviation` absent | `LargeAmountIsFlaggedWithoutHistory` | Pass |
| `TC-RAPID-001` | Third withdrawal in the window fires the rule | BVA | TXNXXX2 not flagged, TXNXXX3 flagged | `RapidWithdrawalsAreFlaggedAtTheThreshold` | Pass |
| `TC-RAPID-002` | Withdrawals outside the window do not fire | BVA | No `rapid_withdrawals` | `WithdrawalsOutsideTheWindowAreNotRapid` | Pass |
| `TC-RAPID-003` | Non-withdrawal types are never rapid withdrawals | Equivalence partitioning | No `rapid_withdrawals` | `NonWithdrawalTypesAreNotRapidWithdrawals` | Pass |
| `TC-BEH-001` | Insufficient history produces no behavioural finding | BVA | No `behaviour_deviation` | `InsufficientHistoryProducesNoBehaviouralFinding` | Pass |
| `TC-BEH-002` | Z-score deviation is reported with its magnitude | Decision table | `behaviour_deviation` present; reason contains "standard deviations" | `BehaviouralDeviationReportsTheZScore` | Pass |
| `TC-BEH-003` | Zero-variance history falls back to a ratio rule | Error guessing | `behaviour_deviation` present; reason contains "uniform" | `UniformHistoryUsesTheRatioRule` | Pass |
| `TC-RISK-001` | No rule hits means no score and no level | Equivalence partitioning | `riskScore == 0.0`, `riskLevel` empty | `NoHitsMeansZeroScoreAndNoLevel` | Pass |
| `TC-RISK-002` | A single hit scores its own weight | Decision table | `riskScore == 40.0`, level `medium` | `SingleHitUsesItsWeight` | Pass |
| `TC-RISK-003` | Multiple hits add up and band as high | Decision table | `riskScore == 75.0`, level `high` | `MultipleHitsAddUp` | Pass |
| `TC-RISK-004` | Score is capped at 100 | BVA | `riskScore == 100.0`, level `high` | `ScoreNeverExceedsOneHundred` | Pass |
| `TC-RISK-005` | Band lower edges are inclusive | BVA | `levelForScore(0.0)` empty; `levelForScore(0.5)` = `low` | `LevelBoundariesAreInclusiveAtTheLowerEdge` | Pass |
| `TC-RISK-006` | Custom bands are honoured | Decision table | Configured bands produce `medium` | `HonoursCustomBands` | Pass |
| `TC-SET-001` | Shipped defaults are valid | Equivalence partitioning | `validate()` true, no error text | `DefaultsAreValid` | Pass |
| `TC-SET-002` | Zero large-amount threshold is rejected | BVA | Invalid; message names "Large amount" | `RejectsZeroLargeAmountThreshold` | Pass |
| `TC-SET-003` | Non-finite thresholds are rejected | Error guessing | NaN and infinity both invalid | `RejectsNonFiniteThresholds` | Pass |
| `TC-SET-004` | Zero duplicate tolerance is accepted | BVA | Valid — lower bound is closed at 0 | `AcceptsZeroDuplicateTolerance` | Pass |
| `TC-SET-005` | Rapid withdrawal count below two is rejected | BVA | Invalid; message names "Rapid withdrawal count" | `RejectsRapidWithdrawalCountBelowTwo` | Pass |
| `TC-SET-006` | Zero-variance ratio at or below 1 is rejected | BVA | Invalid; message names "Zero variance ratio" | `RejectsZeroVarianceRatioAtOrBelowOne` | Pass |
| `TC-SET-007` | High threshold not above medium is rejected | Decision table | Invalid; message names "above the medium" | `RejectsHighThresholdNotAboveMedium` | Pass |
| `TC-SET-008` | High threshold above 100 is rejected | BVA | Invalid; message names "cannot exceed 100" | `RejectsHighThresholdAboveHundred` | Pass |
| `TC-TXN-001` | Zero and negative amounts are rejected | BVA | Rejected by repository validation | `RejectsZeroAndNegativeAmounts` | Pass |
| `TC-TXN-002` | Non-finite amounts are rejected | Error guessing | Rejected | `RejectsNonFiniteAmounts` | Pass |
| `TC-TXN-003` | Duplicate supplied transaction ID is rejected | Error guessing | Rejected; row not inserted | `RejectsADuplicateSuppliedId` | Pass |
| `TC-TXN-004` | Account identifiers are normalised to canonical form | Equivalence partitioning | Stored in `ACC` + 4-character form | `NormalisesAccountIdentifiers` | Pass |
| `TC-CSV-001` | Sample file imports completely | Equivalence partitioning | 11 imported, 0 skipped, 0 failed, 11 rows in table | `ImportsTheSampleFile` | Pass |
| `TC-CSV-002` | Re-importing the same file skips every row | State transition | 0 imported, 11 skipped, table still 11 | `ReimportingTheSameFileSkipsEveryRow` | Pass |
| `TC-CSV-003` | Duplicate column is rejected | Error guessing | File rejected with an error | `RejectsADuplicateColumn` | Pass |
| `TC-CSV-004` | Blank lines are skipped, not failed | Error guessing | Valid rows imported, blank rows ignored | `SkipsBlankLines` | Pass |
| `TC-INV-001` | Resolving a case records its closing time | State transition | `closed_at` populated | `ResolvingRecordsTheClosingTime` | Pass |
| `TC-INV-002` | Reopening a case clears the closing time | State transition | `closed_at` cleared; history retained | `ReopeningClearsTheClosingTime` | Pass |
| `TC-INV-003` | Invalid status is rejected | Equivalence partitioning | Rejected by `isValidStatus` | `RejectsAnInvalidStatus` | Pass |
| `TC-INV-004` | Saving an unchanged case adds no event | State transition | Event count unchanged | `SavingAnUnchangedCaseAddsNoEvent` | Pass |
| `TC-SYS-001` | Re-analysis does not duplicate anomaly rows | State transition | Row count stable across two runs | `ReanalysisDoesNotDuplicateRows` | Pass |
| `TC-SYS-002` | Re-analysis preserves investigation work | State transition | Case assignments and notes survive | `ReanalysisPreservesInvestigationWork` | Pass |
| `TC-SYS-003` | Correcting a transaction removes its stale finding | State transition | Anomaly no longer present after correction | `CorrectingATransactionRemovesTheStaleFinding` | Pass |
| `TC-TIME-001` | Leap day parses correctly | BVA | 29 Feb 2024 accepted | `HandlesALeapDay` | Pass |
| `TC-TIME-002` | Non-leap 29 February is rejected | BVA | Rejected as impossible | `RejectsNonLeapYearFebruary29` | Pass |
| `TC-TIME-003` | Timestamps are unaffected by DST transitions | Error guessing | Same instant across a DST boundary | `IsUnaffectedByDaylightSavingTransitions` | Pass |

**Summary:** 45 designed cases, all implemented, all passing, distributed across
eight suites. Every case has a test and every test has a case; neither list has
an orphan.

---

## C.3 Cases That Were Not Executed

| Area | Case | Reason | Disposition |
|---|---|---|---|
| GUI rendering | Widget layout, table sorting, chart rendering | No widget harness; needs a display and a screenshot baseline | Manual acceptance only |
| `ReportsPage` | Any behaviour | Thirteen-line placeholder; no behaviour exists | Out of scope |
| `SettingsPage` | Any behaviour | Placeholder; no behaviour exists | Out of scope |
| Concurrency | Two clients writing simultaneously | Single-user local application; no network layer | Out of scope |
| Detection accuracy | Precision and recall against labelled fraud | No labelled ground-truth dataset exists | Not claimed anywhere |
| Scale beyond 20,000 | Behaviour at 100,000+ transactions | Benchmark ceiling chosen for runtime | Extrapolated, not measured |
| Portability | Behaviour on Windows or macOS | Tested on one Linux distribution | Declared limitation |

The accuracy row is the one a reader is most likely to look for, so it is stated
plainly: this catalogue verifies that each rule implements its specification. It
does not and cannot verify that the specification detects real fraud.

---

## C.4 What the Catalogue Demonstrates About the Design

Three patterns recur across the 46 cases, and they are the reason the catalogue
is arranged by area rather than by suite.

**Every threshold is pinned from both sides.** `TC-LARGE-001/002/003` sit below,
at, and above the large-amount limit; `TC-SET-005` pins the count boundary at
two; `TC-RISK-005` pins the band edge at its inclusive lower end. A suite that
only tested values above each threshold would pass an implementation that
rejected everything.

**State transitions are tested for reversal, not just progression.**
`TC-INV-001` closes a case and `TC-INV-002` reopens it, asserting that the
closing time is cleared *and* that the history survives. Progression-only testing
would miss an implementation that reopens a case by deleting its audit trail.

**Idempotency is tested explicitly.** `TC-SYS-001` and `TC-SYS-002` run
detection twice and assert that nothing is duplicated and nothing is lost. This
is the class of defect that unit tests structurally cannot reach, and it is where
BUG-02 and BUG-03 were found.
