# Appendix B — Test Case Design and Requirements Traceability

**Companion to:** Chapter 4, §4.11
**Status:** Reference material — excluded from the body page count

This appendix holds the test-case design and the requirements traceability
matrix. They are placed here rather than in Chapter 4 because they are consulted
rather than read: a reader checks one case identifier or one requirement row, and
does not need them in the narrative flow.

The design decisions they embody are stated in Chapter 4 §4.6 and §4.11. In
summary: cases are derived from the threshold values in §4.6.1, not from observed
program behaviour, so that a test asserts the specification rather than restating
the implementation. Every functional requirement is traced to at least one suite
in B.6, and the requirements that cannot be verified are listed explicitly in
B.6.3 rather than left to inference.

---


### B.1 Test case identifier scheme

```
TC-<AREA>-<NNN>

AREA in { CSV, TXN, ID, DUP, LARGE, RAPID, BEH, RISK,
          SET, INV, TIME, SYS }
```

Examples: `TC-DUP-004`, `TC-SET-011`, `TC-INV-007`. The area token makes the
traceability matrix readable without a lookup, and the numeric token is assigned
in creation order and never reused, so a deleted case leaves a visible gap
rather than silently renumbering.

### B.2 Test case template

Every case records the ten fields the assignment specifies, plus the technique
that generated it — without which the case cannot be defended as designed rather
than discovered.

| Field | Content |
|---|---|
| Test Case ID | `TC-<AREA>-<NNN>` |
| Objective | The single property being verified |
| Module | Backend class or suite under test |
| Technique | Design technique that produced the inputs |
| Preconditions | Fixture state required before step 1 |
| Test data | Concrete values, including units |
| Steps | Ordered actions |
| Expected result | Assertion, stated before execution |
| Actual result | Observed outcome, recorded after execution |
| Status | Pass / Fail / Blocked / Not Executed |
| Evidence | Log file or CTest test name |

### B.3 Worked example: boundary value analysis on the large-amount rule

`largeAmountAbsolute` defaults to 10000.0 and `validate()` requires it to be
strictly greater than zero and finite. The rule fires when a transaction amount
**exceeds** the threshold — the comparison in `checkLargeAmount()` is
`amount <= threshold → return`, so the operator is strict:

```cpp
if (transaction.amount <= settings.largeAmountAbsolute)
    return;
```

**Equivalence classes for amount, given a 10000.0 threshold:**

| Class | Range | Expected |
|---|---|---|
| Invalid | amount < 0 | Rejected by repository validation |
| Below threshold | 0 ≤ amount ≤ 10000.0 | Rule does not fire |
| Above threshold | amount > 10000.0 | Rule fires |

**Boundary set derived from those classes:**

| Case | Amount (LSL) | Rationale | Expected | Implemented by |
|---|---|---|---|---|
| `TC-LARGE-001` | 9999.99 | Largest value below threshold | Not flagged | `AmountExactlyAtTheLimitIsNotLarge` (lower case) |
| `TC-LARGE-002` | 10000.00 | Exactly at threshold | **Not flagged** — `>` is strict | `AmountExactlyAtTheLimitIsNotLarge` |
| `TC-LARGE-003` | 10000.01 | Smallest value above threshold | Flagged, weight 35.0 | `LargeAmountIsFlaggedWithoutHistory` |
| `TC-LARGE-004` | 0.00 | Lower bound of the valid amount range | Not flagged; accepted by schema | `RejectsZeroAndNegativeAmounts` |
| `TC-LARGE-005` | −0.01 | Just below the valid range | Rejected by validation | `RejectsZeroAndNegativeAmounts` |

`TC-LARGE-002` is the case that matters, and it is worth being precise about why.
A boundary test exists to pin the *operator*, not to assert a preferred
operator. This project's specification is "exceeds the limit", the reason text
says "exceeds", and the code uses `>`; the test asserts that 10000.00 is not
flagged. Had the test instead asserted that it *is* flagged, it would have been
testing a different specification from the one implemented — and the mismatch
would have been a defect in the test, not in the code.

**Boundary analysis of the other thresholds.** These were checked against the
source rather than assumed, and one ambiguity was found:

| Parameter | Boundary cases | Operator in code | Result |
|---|---|---|---|
| `rapidWithdrawalCount` = 3 | 2, 3, 4 withdrawals in 60 min | `withdrawals < count → return` | Inclusive: 3 fires, 2 does not |
| `unusualAmountZScore` = 3.0 | 2.99, 3.00, 3.01 | `zScore >= threshold && amount > mean` | Inclusive at 3.00 |
| `minimumHistory` = 5 | 4, 5, 6 prior transactions | **Two different operators** | **Ambiguous — see below** |
| `zeroVarianceRatio` = 2.0 | setting 1.00 (invalid), 1.01 (valid) | `validate()` requires `> 1.0` | 2.0 is valid; the open bound is at 1.0 |
| `highThreshold` = 65.0 | 64.99, 65.00 | band assignment is `>=` | 65.00 bands as high |

**The ambiguity found.** `minimumHistory` is read with two different operators in
the same class. `evaluate()` accepts a baseline at exactly the threshold:

```cpp
if (historyCount < minimumHistory)   // >= 5 permits a baseline
    return;
```

while `hasBaseline()` and `profile()` require strictly more:

```cpp
return it->count > minimumHistory;   // > 5, so exactly 5 reports no baseline
```

An account with exactly five prior transactions is therefore evaluated against a
behavioural baseline while the profile simultaneously reports that no baseline
exists. **Impact: none at run time.** Neither `hasBaseline()` nor `profile()` is
called anywhere outside `BehaviorAnalyzer`; they are unused public API, and the
detection path goes through `evaluate()` only. The finding is recorded as a code
quality issue — the same setting should not mean two things — rather than as a
defect, because nothing observable depends on the second meaning.

This is the most useful outcome the boundary technique produced: not a failing
test, but a demonstration that the technique finds *specification* ambiguity as
well as off-by-one errors. An equivalence-partition-only approach would have
tested 4 and 6 prior transactions and missed the case at exactly 5.

### B.4 Worked example: state transition on investigation status

The investigation lifecycle has four statuses and an asymmetric transition
rule: resolving or dismissing closes the case, and moving back to an open status
reopens it.

| From | To | Expected | Case |
|---|---|---|---|
| pending | in_progress | Allowed; event recorded | `TC-INV-002` |
| in_progress | resolved | Allowed; `closed_at` set | `TC-INV-003` |
| resolved | in_progress | Allowed; `closed_at` cleared (reopen) | `TC-INV-005` |
| pending | resolved | Allowed | `TC-INV-004` |
| any | `invalid_status` | Rejected by `isValidStatus` | `TC-INV-009` |

The reopen case is the interesting one: it asserts that a closed case can be
reopened *and* that the audit trail retains the earlier closure rather than
overwriting it. A state-transition approach finds this because it enumerates
transitions rather than exercising a happy path.

### B.5 Negative and robustness cases

Negative testing is not confined to validation suites. The following classes are
covered explicitly:

| Class | Example | Suite |
|---|---|---|
| Empty input | CSV with header only | `CsvImportTest` |
| Missing field | Row with 6 of 8 columns | `CsvImportTest` |
| Wrong type | Non-numeric amount | `CsvImportTest` |
| Duplicate record | Same `transaction_id` twice | `TransactionRepositoryTest` |
| Malformed quoting | Unbalanced quote in a field | `CsvImportTest` |
| Blank lines | Trailing newlines and empty rows | `CsvImportTest` |
| Duplicate header | Two `amount` columns | `CsvImportTest` |
| Out-of-range setting | `rapidWithdrawalCount = 1` | `DetectionSettings` |
| Illegal state transition | Unknown case status | `InvestigationTest` |
| Invalid timestamp | Unparseable date string | `TransactionTime` |

---

### B.6 Requirements traceability matrix

Every functional requirement (FR) and non-functional requirement (NFR) is traced
to the suite or tool that verifies it. A requirement with no test is a
requirement with no evidence, and is flagged as such.

#### B.6.1 Functional requirements

| ID | Requirement | Verified by | Tests |
|---|---|---|---|
| FR-01 | Import transactions from a CSV file, reporting per-row errors | `CsvImportTest` | 16 |
| FR-02 | Add a transaction manually through the repository | `TransactionRepositoryTest` | 18 |
| FR-03 | Validate and normalise account and transaction identifiers | `Identifiers`, `TransactionRepositoryTest` | 14 + 18 |
| FR-04 | Detect duplicate payments within a time window and tolerance | `AnomalyDetector` | 20 |
| FR-05 | Detect amounts at or above the large-amount threshold | `AnomalyDetector` | 20 |
| FR-06 | Detect rapid successive withdrawals within a window | `AnomalyDetector` | 20 |
| FR-07 | Detect behavioural deviation against a strictly-earlier account baseline | `BehaviorAnalyzer`, `AnomalyDetector` | 13 + 20 |
| FR-08 | Score risk by weight and band into low / medium / high | `RiskScorer`, `AnomalyDetector` | 10 + 20 |
| FR-09 | Persist detection results idempotently across re-analysis | `DetectionPipelineTest` | 13 |
| FR-10 | Create and update investigation cases | `InvestigationTest` | 18 |
| FR-11 | Record an append-only audit history of case changes | `InvestigationTest` | 18 |
| FR-12 | Validate detection settings against documented ranges | `DetectionSettings` | 19 |

#### B.6.2 Non-functional requirements

| ID | Requirement | Verified by | Status |
|---|---|---|---|
| NFR-01 | Analyse 20,000 transactions within a usable interactive time | `ftad_benchmark` | Measured: 31.7–39.9 ms across three runs (median 34.8 ms) |
| NFR-02 | No memory errors or undefined behaviour under test | ASan + UBSan; Valgrind | **Verified** — 0 findings in both |
| NFR-03 | Referential integrity enforced across all four tables | `TransactionRepositoryTest`, `InvestigationTest` | Verified |
| NFR-04 | Test execution is deterministic and order-independent | Fixture design; full-suite runs | Verified (154/154; 3.8–5.4 s across runs) |
| NFR-05 | Builds and runs on Linux | Build and launch on Parrot 7.4 / Debian 13 | Verified on one distribution |
| NFR-06 | Static analysis free of high-severity findings | Cppcheck, Clang-Tidy | **Verified** — 0 application-scope Cppcheck findings; 6 Clang-Tidy diagnostics triaged (§4.10.4.1) |

#### B.6.3 Requirements without direct coverage

| Requirement | Gap | Disposition |
|---|---|---|
| GUI rendering correctness | No widget test harness | Manual acceptance only; declared in §4.9.2 |
| `ReportsPage` behaviour | Page is a placeholder | No behaviour exists to test |
| `SettingsPage` persistence | Page is a placeholder | No behaviour exists to test |
| Multi-user concurrency | Not implemented | Out of scope |
| Detection accuracy (precision/recall) | No labelled ground-truth dataset | Not claimed anywhere in this report |

The last row is the most important. The suite verifies that each rule
*implements its specification*. It does not and cannot verify that the
specification detects real fraud, because that would require labelled data the
project does not possess. No accuracy figure is reported, and any claim that the
detector is "accurate" would be unsupported.

---

### B.7 Test design technique inventory

Ten techniques are used. The category column follows the usual black-box /
white-box / experience-based / verification split; the "applied to" column names
the artefact, not the suite, because one technique is often applied to several
suites.

| Technique | Category | Applied to | Rationale |
|---|---|---|---|
| Equivalence partitioning | Black-box | Identifier, settings and CSV field validation | Inputs cluster into classes with identical handling |
| Boundary value analysis | Black-box | Every numeric threshold in `DetectionSettings` | Off-by-one defects concentrate at class edges |
| Decision table | Black-box | Risk banding (medium ≥ 35, high ≥ 65) and weight combinations | Output depends on combinations of conditions |
| State transition | Black-box | Investigation status, including reopening | Legal *and* illegal transitions must both be covered |
| Error guessing | Experience-based | Malformed CSV, quoted fields, blank lines, duplicate headers/IDs | Cheap to apply, high yield on parser code |
| Statement coverage | White-box | `ftad_core` via gcov | Establishes which code the suite never reaches |
| Branch coverage | White-box | Conditionals in detector and validator | Statement coverage hides untaken branches |
| Static analysis | Verification | `ftad_core` and `frontend` | Detects classes testing may not trigger |
| Dynamic analysis | Verification | Test binary under sanitizers and Valgrind | Detects memory and UB faults at run time |
| Measurement | Performance | `AnomalyDetector`, `BehaviorAnalyzer`, `RiskScorer` | Establishes cost and scaling, not pass/fail |

### B.8 Threshold inventory and enforced ranges

Boundary value analysis is only as good as the boundaries it uses. These values
are read from `DetectionSettings` and are the defaults the application runs with.

| Parameter | Default | Range enforced by `validate()` |
|---|---|---|
| `largeAmountAbsolute` | 10000.0 | > 0, finite |
| `duplicateWindowMinutes` | 10 | ≥ 1 (**no upper bound**) |
| `duplicateAmountTolerance` | 0.01 | ≥ 0, finite |
| `rapidWithdrawalWindowMinutes` | 60 | ≥ 1 (**no upper bound**) |
| `rapidWithdrawalCount` | 3 | ≥ 2 |
| `unusualAmountZScore` | 3.0 | > 0, finite |
| `zeroVarianceRatio` | 2.0 | > 1, finite |
| `minimumHistory` | 5 | ≥ 1 |
| `mediumThreshold` | 35.0 | > 0, finite |
| `highThreshold` | 65.0 | > mediumThreshold, ≤ 100 |

Several ranges are **open** (`> 0`, `> 1`) rather than closed, which changes the
boundary set: for `largeAmountAbsolute` the invalid boundary is 0.0 itself and
the valid boundary is the smallest representable value above it, not 1.0. The
cases in B.3 respect this. The two missing upper bounds are not theoretical: they
are what makes the latent defect described in §4.10.4.1 reachable.

---

### B.9 Out-of-scope areas and reasons

Exclusions are stated so that the absence of tests is not mistaken for evidence of
correctness.

| Excluded | Reason |
|---|---|
| Qt widget rendering and layout | No GUI harness; needs a display and screenshot baseline |
| `ReportsPage`, `SettingsPage` | Placeholder implementations — no behaviour exists to test |
| Authentication, authorisation, encryption at rest | Not implemented |
| Network and multi-client concurrency | Single-user local application |
| Datasets above 20,000 transactions | Benchmark ceiling; beyond this is extrapolated, not measured |
| Localisation, platform matrix | Single-language UI; one Linux distribution tested |

### B.10 Entry and exit criteria with measured outcomes

| Type | # | Criterion | Threshold | Result |
|---|---|---|---|---|
| Entry | E1 | Source compiles with `-Wall -Wextra`, zero warnings | 0 | Met |
| Entry | E2 | `ftad_core` links into both `FTAD` and `ftad_tests` | — | Met |
| Entry | E3 | `schema.sql` present beside the test binary | — | Met |
| Entry | E4 | Qt platform plugin resolves headless | — | Met |
| Entry | E5 | Fixtures create and destroy a clean database | — | Met |
| Entry | E6 | Requirements and acceptance conditions written down | — | Met (B.6) |
| Exit | X1 | All automated tests pass | 100 % | **154/154** in 5.4 s |
| Exit | X2 | No compiler warnings | 0 | **0** |
| Exit | X3 | No sanitizer or Valgrind errors | 0 | **0** — ASan+UBSan 154/154 clean; Valgrind 0 errors from 0 contexts |
| Exit | X4 | Static analysis findings triaged | 100 % | **0** application-scope Cppcheck findings; 6 Clang-Tidy diagnostics, all triaged |
| Exit | X5 | Line coverage recorded | ≥ 80 % | **85.4 %** lines (992/1162), 97.4 % functions, 51.4 % branches |
| Exit | X6 | Every requirement traced to a test | 100 % | B.6 |
| Exit | X7 | Every raised defect has a status | 100 % | BUG-01 … BUG-06 closed; LAT-01/02 open and unreachable |
| Exit | X8 | Performance baseline recorded | 5 sizes | 1,000–20,000, 5 repetitions each |

E6 is listed last but is logically first: without a written expectation there is
nothing to test against, only behaviour to describe.

### B.11 Risk register and threats to validity

| ID | Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|---|
| R1 | Tests encode the implementation, not the specification | Medium | High | Derive expected values from `DetectionSettings` ranges, not observed output |
| R2 | Coverage inflated by silently excluding code | Medium | Medium | State exclusions; report the raw figure |
| R3 | Suite is non-deterministic and flaky | Low | High | Fresh DB per test, fixed timestamps, offscreen platform |
| R4 | Tools unavailable, so gates cannot be evidenced | Realised, then closed | High | Installed 4 Nov 2026; all gates re-run with captured logs |
| R5 | Performance measured on an unpinned, noisy machine | **Realised** | Medium | Report median of 5 repetitions and the observed range |
| R6 | GUI untested, so presentation defects reach submission | High | Medium | Manual acceptance pass; declare the gap |
| R7 | Single-distribution testing masks portability defects | Medium | Low | Declare the limitation; avoid portability claims |

Threats to validity, distinct from project risks:

| Threat | Effect | Mitigation applied | Residual |
|---|---|---|---|
| Author–tester identity | Tests share the designer's blind spots | Expected values derived from specification | **Not eliminated** |
| Single implementation | No differential testing against a reference | None | Accepted |
| No labelled data | Detection accuracy unmeasurable | Accuracy not claimed | Accepted |
| One platform | Portability untested | Limitation declared | Accepted |
| Unpinned hardware | Timing varies between runs | Medians plus reported range | Reduced |
| Placeholder pages | Two modules untested | Declared out of scope | Accepted |
