# Chapter 4 — Testing Methodology and Test Plan

> **Evidence note.** Every count, threshold and version number in this chapter was
> read from the repository or produced by executing a command on 4 November 2026.
> Raw output is in `documentation/evidence/`, summarised in
> `documentation/evidence/SUMMARY.md`. The tool inventory, environment
> specification, threshold table, entry/exit criteria, risk register and threat
> table are in Appendices B and D, so that the plan itself stays readable.

---

## 4.1 Introduction

This chapter defines how FTAD is tested, why that approach was selected, and what
evidence each activity must produce. It is written to be executable: every test
level names the suites that implement it, every technique names the artefact it is
applied to, and every requirement is traced to at least one suite.

Four concepts are frequently conflated in test documentation, and the distinction
is load-bearing for the rest of this report:

| Concept | Question it answers | Examples in FTAD |
|---|---|---|
| **Testing level** | *At what granularity?* | Unit, integration, system, acceptance |
| **Test design technique** | *How are inputs chosen?* | Equivalence partitioning, boundary value analysis, decision table, state transition |
| **Verification activity** | *What is inspected without executing?* | Compiler diagnostics, static analysis |
| **Testing type** | *What triggers the activity?* | Regression, performance, security |

"We performed boundary value analysis at unit level as a regression activity" uses
all four correctly. "We performed unit testing and boundary value analysis as two
levels" does not, and the error matters because a strategy can only be justified if
its categories are stable.

---

## 4.2 Objectives and Strategy

Seven objectives drive the plan, ordered so that a schedule trade-off preserves the
higher ones at the expense of the lower. **OBJ-1** each detection rule fires when
and only when its condition holds (verified by the 20 `AnomalyDetector` and 13
`BehaviorAnalyzer` tests). **OBJ-2** invalid input is rejected with a specific
error rather than silently coerced. **OBJ-3** persisted state survives re-analysis
without loss or duplication. **OBJ-4** memory safety and absence of undefined
behaviour. **OBJ-5** a performance baseline with near-linear scaling. **OBJ-6**
quantified structural coverage with the uncovered remainder disclosed. **OBJ-7** a
reproducible, deterministic regression suite. OBJ-7 is not ceremony: a suite that
is green only on the machine that produced it is not a regression gate, so
determinism is treated as a testable property and enforced through the four
controls in §4.7.

FTAD uses a **layered, risk-based, automation-first** strategy. *Layered* means
testing follows the architectural boundaries — pure logic without a database,
persistence against a real database, the assembled pipeline end to end — so each
defect has a natural home: a scoring error surfaces at unit level, a foreign-key
error at integration level, a lost-update error only at system level. *Risk-based*
means effort follows consequence: the detection engine decides whether a fraud
signal is raised or suppressed, so it receives the most tests (33 of 154 across
`AnomalyDetector` and `BehaviorAnalyzer`, plus 10 in `RiskScorer`), while the
presentation layer receives the least, because a cosmetic defect in a Qt page
neither corrupts data nor suppresses a finding. *Automation-first* means every
check that can be automated is wired into CTest, so regression is one command
rather than a checklist; manual testing is reserved for properties that genuinely
require a human observer. The strategy follows the V-model correspondence between
construction and verification — requirements validated at system and acceptance
level, design verified at integration level, implementation at unit level — which
is what makes the traceability matrix in Appendix B.6 meaningful rather than
decorative.

Four grounds justify it. **The architecture makes it cheap:** the backend compiles
once as `ftad_core`, linked by both the application and the test binary, so unit
tests exercise exactly the code the application runs — no mock layer to drift and no
second build configuration. This is the structural decision that made 154 tests
possible without a mocking framework. **The domain has clean oracles:** a rule-based
detector gives a definite answer, so assertion-based testing suffices; a trained
classifier would have made the oracle problem dominate the design. **The risk profile
is asymmetric:** a false negative costs more than a false positive, so rule *firing*
is tested more heavily than *restraint* — though restraint is covered too, since an
engine that flags everything is equally useless, and Appendix B.3 asserts both sides
of each threshold. **Scale makes full regression rational:** at 5.4 s per run,
selection techniques would add complexity without saving anything. One alternative
was rejected — **model-based testing**, because the rules are already executable code
with explicit thresholds, so a model would duplicate the specification into a second
artefact; `DetectionSettings` already is the machine-readable specification.

---

## 4.3 Testing Levels

| Level | Scope | Suites | Tests |
|---|---|---|---|
| **Unit** | Individual classes and free functions; no database, no widgets | `DetectionSettings` 19, `Identifiers` 14, `TransactionTime` 13, `BehaviorAnalyzer` 13, `RiskScorer` 10 | **69** |
| **Integration** | Components with a real SQLite database: schema, constraints, foreign keys, transactions | `AnomalyDetector` 20, `TransactionRepositoryTest` 18, `InvestigationTest` 18, `CsvImportTest` 16 | **72** |
| **System** | Assembled pipeline: stored transactions → analysis → persisted anomalies, as the application invokes it | `DetectionPipelineTest` 13 | **13** |
| **Acceptance** | End-to-end use by a human observer | Manual | — |
| **Regression** | Trigger, not a granularity: full suite after every change | All | **154** |

`AnomalyDetector` sits at integration level rather than unit level because it
composes `BehaviorAnalyzer` and `RiskScorer`; testing it exercises the contract
between three components, not any one of them. The re-analysis tests in
`DetectionPipelineTest` are the most valuable in the suite: they assert that
running detection twice does not duplicate anomalies, orphan investigations or lose
reviewer decisions — the class of defect unit tests structurally cannot catch.

Acceptance testing is manual and partially executed. Observed on the test machine:
launching the application, importing the 11-row sample CSV, viewing results,
opening a case, changing its status. **Not executed**, and marked as such in
Chapter 5: multi-user concurrency, extended usability evaluation, accessibility
review. These are not automated because the properties under test — legibility of
the risk presentation, clarity of the workflow — are judgements, not assertions.

---

## 4.4 Test Design Techniques

Ten techniques are used, and the full inventory — with the artefact each is applied
to and the reason it was selected — is in **Appendix B.7**. The four carrying most
of the design weight are worth naming here, because they are what makes the case
catalogue in Appendix C derivable rather than invented. **Equivalence partitioning**
governs identifier, settings and CSV field validation, where inputs cluster into
classes with identical handling. **Boundary value analysis** governs every numeric
threshold in `DetectionSettings`, because off-by-one defects concentrate at class
edges. **Decision tables** govern risk banding, where the output depends on
combinations of rule hits rather than any single condition. **State transition
testing** governs the investigation lifecycle, where both legal and illegal
transitions must be covered — reopening a closed case is the case that catches an
implementation that "reopens" by deleting the audit trail. The remaining six are
coverage measurement, static and dynamic analysis, and error guessing, which is
cheap to apply and yields disproportionately on parser code.

Boundary value analysis is only as good as the boundaries it uses. The ten
parameters, their defaults and the ranges `validate()` enforces are tabulated in
**Appendix B.8**, because that table is consulted rather than read. Two properties
of it drive the design and belong in the narrative. Several ranges are **open**
(`> 0`, `> 1`) rather than closed, which changes the boundary set: for
`largeAmountAbsolute` the invalid boundary is 0.0 itself and the valid boundary is
the smallest representable value above it, not 1.0. And two parameters —
`duplicateWindowMinutes` and `rapidWithdrawalWindowMinutes` — have **no upper
bound**, which is not a theoretical gap: it is what makes the latent defect in
§4.6 reachable.

---

## 4.5 Tools, Environment and Scope

Ten tools span build, execution, coverage, static analysis, dynamic analysis and
measurement; the inventory with versions and the gate each supports is in
**Appendix D.1**, and the environment specification in **Appendix D.2**. Three
points belong in the plan itself.

**One version string is inconsistent and is recorded rather than corrected.** The
Debian package `lcov` is 2.3.1-1 according to `dpkg -s`, but the binary prints
`LCOV version 2.0-1`. The package version is authoritative; anyone reproducing the
run will see the same output, so the discrepancy is stated. **Static analysis has
two scopes.** Cppcheck runs twice: the headline figure covers application source
only (`backend/src`, `frontend/src`) and excludes `tests/` and the vendored
GoogleTest that `FetchContent` places in `build/_deps`; the second pass covers the
whole tree and is informational. The distinction is not cosmetic — the unscoped
run reports 74 findings, of which 69 are in third-party code FTAD does not own and
should not patch, so quoting 74 as the project's defect count would misrepresent
the application. **Tools were installed during the project:** lcov, Cppcheck,
Clang-Tidy and Valgrind were absent when testing began and were installed by the
operator with `install_test_tools.sh`, which wraps `apt-get` and supports a
`--check` mode that changes nothing. The installation is part of the record rather
than an unstated precondition, because results that depend on tools the reader
cannot reproduce are not reproducible.

The environment is a rolling security-oriented Linux distribution: representative
of the target platform but **not** a controlled benchmark platform, because no CPU
frequency scaling was pinned and no other processes were excluded during
measurement. Chapter 6 must be read with that caveat.

**Determinism rests on four controls.** The `DatabaseTest` fixture deletes the
database, reconnects through the production `DatabaseManager` and re-applies the
schema in `SetUp()`, then removes the file in `TearDown()`, so tests cannot share
rows or depend on order. `gtest_discover_tests` sets `QT_QPA_PLATFORM=offscreen`,
so no test needs a display server. Every test supplies explicit timestamps rather
than reading the clock. And the schema is copied beside the test binary by a
`POST_BUILD` step, so the tests validate the same `schema.sql` the application
ships rather than a divergent copy. Coverage reconfigures the same tree with
`-DFTAD_ENABLE_COVERAGE=ON`, adding `--coverage -O0 -g` to `ftad_core`, because
optimisation inlines and merges statements and distorts line attribution.

**In scope** is everything in `ftad_core` and the logic of the presentation layer:
CSV parsing, validation and error reporting; transaction persistence, constraints
and identifier normalisation; the four detection rules; risk scoring, banding and
reason generation; settings validation; detection persistence and idempotent
re-analysis; investigation lifecycle and audit history; timestamp parsing and UTC
normalisation; memory safety and undefined behaviour; static defect classes; and
analysis throughput. **Out of scope**, with reasons in Appendix B.9, is Qt widget
rendering and layout, the two placeholder pages, authentication and encryption,
network and multi-client concurrency, datasets above 20,000 transactions, and
localisation. Exclusions are stated so that the absence of tests is not mistaken
for evidence of correctness; excluding the two placeholder pages in particular is
a correctness point rather than convenience, because testing a stub produces
passing tests that assert nothing about the shipped product.

---

## 4.6 Plan, Entry and Exit

Dates are when the activity occurred, not a projection. Environment setup and a
source build were completed on 1–2 November, followed by the audit and SLOC
baseline, extension of the unit and integration suites to 154 tests, a full
regression and coverage baseline, then static and dynamic analysis which raised and
closed BUG-01 to BUG-06. Benchmarking on 2 November diagnosed the timestamp
regression and produced the ~1,466× improvement. On 4 November the missing tools
were installed, all nine gates were re-run and the evidence set was captured.
Documentation and this report occupied 4–6 November. The performance figure is a
historical measurement: the "before" case is not re-runnable once the defect is
fixed, so it is attributed to its own report rather than blended with current
numbers.

Five roles are defined — test manager, test designer, developer, executor and
reviewer — owning respectively the plan and entry/exit criteria, technique
selection and case design, fixes and the build, suite execution and evidence
capture, and sign-off. In this project **all five were performed by one person**,
and that is the principal threat to validity (§4.9): the suite shares any blind
spot present in the design. It is mitigated, not eliminated, by deriving expected
values from the threshold specification rather than from the implementation.

All eight exit criteria are met by measurement and each is backed by a log; the
criteria and their measured outcomes are in **Appendix B.10**. No criterion is
satisfied by assertion, and none was waived. X3, X4 and X5 were open while this
chapter was first drafted because the tools that produce them were not installed;
they were closed on 4 November 2026 after installation and a full re-run. The
policy during the interim was: mark open, never waive; mark dependent figures as
unverified until a log exists; list open items as limitations rather than
dropping them.

### 4.6.1 Triage of the static-analysis findings

The six Clang-Tidy diagnostics split into two classes, and only one is a defect.
Three are `bugprone-easily-swappable-parameters` in `AnomalyDetector::timeWindow`,
`BehaviorAnalyzer::evaluate` and `DetectionService::analyzeAll`; all were
**accepted** after inspecting every call site, since a strong-type wrapper would
remove the risk but change the public interface and is not justified at this scale.
Three describe one latent defect: `bugprone-implicit-widening-of-multiplication-result`
at `AnomalyDetector.cpp:177`, `:180` and `:261`.

The widening finding is real. The code computes

```cpp
const QDateTime from = times.at(index).addSecs(
    -60 * settings.duplicateWindowMinutes);
```

`60` and `duplicateWindowMinutes` are both `int`, so the multiplication is
performed in `int` and only then widened to the `qint64` that `addSecs()` expects.
Above 35,791,394 the product overflows, which is undefined behaviour, and
`validate()` enforces only a lower bound and no upper bound (Appendix B.8).

**Reachability.** The defect is latent, not live. The shipped application
constructs `DetectionSettings` with compiled-in defaults, there is no `QSettings`
or other runtime configuration path, and `SettingsPage` is a thirteen-line
placeholder that writes nothing. Reaching the overflow requires editing the source.
It is therefore recorded with severity Low, and the one-line remediation
(`qint64(60) * settings.duplicateWindowMinutes`, plus an upper bound in
`validate()`) is documented for the next revision. This is both the honest reading
and the more useful one: recording three "bugs" would overstate the risk, and
recording zero findings would conceal a genuine latent defect that a future
configuration feature would expose.

The risk register, with likelihood, impact and mitigation for each of seven risks,
is in **Appendix B.11**. Two of its rows matter enough to repeat. R4 — tools
unavailable so gates cannot be evidenced — materialised and was closed. R5 —
performance measured on an unpinned, noisy machine — materialised and cannot be
closed on this host: three runs of the same binary on the same data gave 31.7,
34.8 and 39.9 ms for 20,000 transactions, a spread of roughly 25 %, so the report
quotes the range and the median and draws no conclusion from differences smaller
than that.

---

## 4.7 Test Case Design and Traceability

The test-case design is in **Appendix B**: a case template, two worked derivations,
a negative-case inventory and a twelve-row traceability matrix. It is reference
material that a reader consults rather than reads through, so it is kept out of the
chapter body.

What belongs in the narrative is the decision that produced it. Cases are derived
from the thresholds in Appendix B.8 and from the comparison operators in the
source, not from observed behaviour, because a test written against observed output
asserts only that the program still does what it already did. Appendix B.3
demonstrates the derivation for the large-amount rule, and records what the
technique actually turned up: not an off-by-one error, but an ambiguity in how
`minimumHistory` is interpreted in two places in `BehaviorAnalyzer`.

---

## 4.8 Deliverables

The chapter itself is delivered as `documentation/test_plan.md`; test-case design
and traceability as Appendix B; the execution record as Chapter 5; defect,
performance and security findings in `Reports/` and Chapter 6; and the raw evidence
as nine gate logs under `documentation/evidence/`, driven by `run_test_gates.sh`
and `install_test_tools.sh`.

---

## 4.9 Limitations and Threats to Validity

The threat table is in **Appendix B.11**; its first row is the most serious and is
stated here plainly. A project in which the author of the code also writes the
tests cannot demonstrate independent verification: the mitigations reduce the risk
of tests that merely restate the implementation, but they do not substitute for a
second engineer, and the reader should weight the results accordingly.

The functional and tooling limitations are these. The detection engine is
rule-based and does not learn from data. Behavioural baselines require five prior
transactions, so a new account is never behaviourally assessed. `ReportsPage` and
`SettingsPage` are placeholders. The suite runs on one Linux distribution and one
Qt version, so portability is declared rather than demonstrated. And no labelled
fraud corpus exists, so no accuracy figure is available — a limitation that
constrains what the whole report may claim.
