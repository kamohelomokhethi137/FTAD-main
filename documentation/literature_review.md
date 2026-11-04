# Chapter 2 — Literature Review and Background Research

**Project:** Financial Transaction Anomaly Detector (FTAD)
**Document type:** Literature review and background research (Checkpoint 3)
**Prepared:** 4 November 2026

> **Evidence note.** Every citation in this chapter resolves to an entry in
> `documentation/references.bib`. Journal and book metadata was retrieved from
> the Crossref DOI registration record on 4 November 2026; see
> `documentation/references_verification.md` for the per-reference verification
> table and for the entries that could not be machine-verified. No reference was
> written from memory alone.

---

## 2.1 Introduction

Software testing is the principal empirical mechanism by which engineers gain
confidence that a program behaves as intended. Unlike mathematical proof, which
establishes correctness over all inputs, testing samples a program's input space
and observes whether the realised behaviour matches an expected outcome
[1], [2]. This distinction matters for a system such as FTAD, which ingests
externally supplied transaction records, applies statistical rules, and persists
risk judgements that a human reviewer then acts upon. A defect in such a system
does not merely produce an incorrect screen; it can suppress a genuine fraud
signal or manufacture a false accusation against an account holder.

This chapter establishes the conceptual and empirical background against which
the FTAD testing programme was designed. It proceeds from first principles —
what testing is, what quality assurance adds to it, and which techniques and
levels the discipline recognises — before narrowing to the specific tooling,
performance, security and reliability concerns that shape the test plan in
Chapter 4. The final section reviews prior work on automated financial anomaly
detection, because the domain literature determines what "correct behaviour"
means for FTAD's detection engine and therefore what the test oracles must
assert.

---

## 2.2 Software Testing Fundamentals

Myers' definition — testing is "the process of executing a program with the intent
of finding errors" [1] — is deliberately adversarial: a test that cannot fail is not
a test. Myers also separates testing from debugging, since testing discovers the
presence of defects while debugging locates them, and that separation is visible in
FTAD, where automated checks detect and a separate investigation module records human
diagnosis and resolution.

Ammann and Offutt generalise this into a structural account [2]: a test case is a
four-tuple of pre-state, inputs, expected post-state and an oracle deciding whether
the observed post-state matches, and coverage criteria supply the adequacy argument —
the engineer names a criterion (statement, branch, condition, path, input-space) and
measures how much of the artefact it requires has been exercised. Jorgensen derives
tests from a model of required behaviour [3] and Beizer supplies the
functional/structural dichotomy behind structural adequacy [4]. Two consequences bind
FTAD: every test carries an explicit oracle, and the adequacy argument names its
criterion. The project therefore reports gcov/lcov line and function coverage and
treats the uncovered remainder as a documented limitation.

## 2.3 Software Quality Assurance

Testing is one activity inside software quality assurance. Laporte and April define
SQA as the planned, systematic activities that provide adequate confidence of
conformance to established requirements and standards [5], and separate quality
assurance — process-oriented and preventive — from quality control, which is
product-oriented and detective. Automated tests are quality control; compiling with
warnings-as-errors, running static analysis on every build and gating merges on a
passing suite are quality assurance.

Kan supplies the measurement substrate [6]: defect density and reliability growth are
ratios over a measurement base, useful only if the counting rules are fixed in
advance — which is why §1.4 records its SLOC methodology rather than a bare number.
ISO/IEC 25010 supplies the vocabulary for quality requirements [7], decomposing
quality into measurable sub-characteristics, and Stefani, Vamvatsikou and Vassiliadis
show those characteristics can be instantiated as scored metrics for a specific
application class [8]. IEEE Std 1012 separates verification, whether the system was
built right, from validation, whether the right system was built, and scales V&V
rigour by integrity level [9]. FTAD's integrity level is low in safety terms, but its
decisions are consequential for individuals, which justifies treating detection logic
as the highest-rigour component and the presentation layer as the lowest.

## 2.4 Testing Methodologies and Techniques

Pressman and Maxim distinguish process models, which organise when testing happens,
from techniques, which determine how test data are selected [10]. The distinction is
frequently blurred in student projects and is worth stating plainly: unit testing is a
*level*, boundary value analysis is a *design technique*, static analysis is a
*verification activity*, and regression testing is a *type* defined by its trigger
rather than its granularity. FTAD keeps these categories separate so that every case
traces to both a level and a technique.

Among black-box techniques, equivalence partitioning divides the input domain into
classes the specification treats identically, and boundary value analysis concentrates
effort at class edges where off-by-one defects concentrate [1], [3]. Both apply
directly to FTAD's numeric thresholds: an amount exactly at the large transaction
threshold, one cent below and one cent above is a minimal, high-yield test set for that
rule. Combinatorial and random techniques offer a complementary adequacy argument for
parameter interactions, though Wu *et al.* find that their relative effectiveness
depends on the structure of the fault population rather than being uniformly ordered
[11], which cautions against treating any single generation strategy as sufficient.
Liu and Nakajima address the harder half of the problem — the oracle — by deriving both
cases and oracles from functional scenarios in formal specifications [12], precisely
FTAD's difficulty: the expected risk score is not obvious unless the scoring model is
itself treated as the specification.

White-box techniques supply structural adequacy. Aghamohammadi, Mirian-Hosseinabadi
and Jalali propose statement frequency coverage, arguing that how often a statement
executes carries information that binary coverage discards [13]; Lee, Kang and Jung
show that the choice of coverage criterion interacts strongly with the artefact under
test [14]. Hora examines the practice of excluding code from coverage measurement and
documents both the motivations — generated code, defensive branches, platform-specific
paths — and the risk that exclusion silently inflates reported coverage [15]. That
bears directly on FTAD's 85.4 % line figure, which is meaningful only alongside an
explicit statement of what was excluded. Derakhshanfar, Devroey and Zaidman show that
basic-block coverage can outperform coarser criteria for search-based test generation
[16], reinforcing that granularity is a design decision with measurable consequences,
and Forgács and Kovács favour risk-based selection of techniques over uniform
application of all of them [17].

## 2.5 Testing Levels

The conventional ladder — unit, integration, system, acceptance — decomposes the
testing problem along the same boundaries as the architecture. Kessel and Atkinson show
that diversity among generated unit tests predicts fault-finding better than raw test
count [18], and Menéndez *et al.* diversify tests around specific targets rather than
uniformly across the code base [19]; both argue for deliberate construction around
distinct behaviours rather than exhaustive enumeration of inputs, which is how FTAD's
ten suites are organised. Integration testing addresses faults at component
boundaries. Arcuri and Galeotti confront a directly relevant instance — handling SQL
databases in automated test generation — and show that database state is a first-class
source of complexity that must be modelled explicitly rather than mocked away [20];
FTAD's repositories are therefore tested against a real, freshly created SQLite file
per case.

Regression testing, triggered by change rather than by level, is what makes iterative
development safe. Greca, Miranda and Bertolino document a persistent gap between
regression techniques published and techniques adopted in practice [21]: the literature
offers prioritisation, selection and minimisation, while practice frequently runs the
whole suite. That gap has not closed — Mondal and Nasre propose prioritisation based on
delta displacement in coverage [22], Rothermel and Untch reassess their foundational
prioritisation assumptions two decades on [23], Maurina, Cazzola and Ghosh extend
selection to polyglot systems [24], Bertolino *et al.* combine coverage spectrum with
operational profile [25], and Bluemke and Malanowska show that effort estimation
remains less mature than defect prediction [26]. For a project of FTAD's scale — 154
tests executing in seconds — full-suite regression is both feasible and preferable to
selective techniques, and the literature supports that as a rational choice rather than
a shortcut.

## 2.6 Testing Tools and Technologies

Tooling determines which techniques are economically available. GoogleTest provides the
xUnit-style fixture, assertion and discovery framework used for FTAD's unit and
integration levels [27], and its death-test and parameterised facilities make negative
and boundary testing of validation logic practical at scale. Cppcheck performs static
analysis without a full compilation database and reports defect classes compilers do
not diagnose [28]. Valgrind's Memcheck detects invalid access and leaks at run time by
instrumenting the binary [29], while AddressSanitizer and UndefinedBehaviorSanitizer
instrument at compile time with far lower overhead and detect undefined behaviour that
may never manifest as a visible failure [30]. Running both is justified by their
complementary blind spots — sanitizers see into inlined code, Valgrind catches
library-level misuse — a trade-off between instrumentation granularity and observation
fidelity that Chen *et al.* make explicit [31].

Adoption is not automatic. Eisty and Carver find testing in research software widely
acknowledged yet under-resourced [32]; Murazvu *et al.* isolate organisational rather
than technical barriers [33]; Stradowski and Madeyski document testing at a scale where
heterogeneity dominates [34]. These explain why a small project can hold a mature
toolchain yet still under-test: the binding constraint is the cost of constructing and
maintaining oracles, not the availability of runners. That cost includes flakiness —
Fatima, Hemmati and Briand show flaky-test repairs can be predicted and applied
automatically [35] — which is why FTAD's suite creates its own database per test, uses
fixed timestamps and forces the offscreen Qt platform.

## 2.7 Performance Testing

Performance testing establishes whether a system meets response-time and
resource-consumption requirements under representative load. Imran *et al.* find that
the coverage achieved by performance tests is systematically different from that of
functional tests in open-source Java systems [36], so a fast functional suite says
nothing about the detection pipeline, which must be measured separately at realistic
volumes. Traini and Cortellessa use multi-objective optimisation to detect
latency-degradation patterns in service-based systems [37], illustrating that a single
aggregate latency figure conceals the structure of degradation: effective reporting
identifies *which* input class triggers the slow path. FTAD's performance report
therefore gives both the aggregate figure and the before/after comparison isolating the
responsible defect — a timestamp routine resolving local time instead of UTC.

## 2.8 Security Testing

Security testing seeks to demonstrate that a system resists inputs intended to violate
confidentiality, integrity or availability. Lachkov, Tawalbeh and Bhatt describe
vulnerability assessment and penetration testing as structured discovery, simulation
and verification [38], and Leithner, Garn and Simos present a feedback-driven black-box
approach to exploiting injection vulnerabilities [39] — the category most relevant to
any application assembling SQL from user-supplied values. For a C++ application the
surface is dominated by memory safety and input handling, and the dynamic-analysis
techniques of §2.6 are the standard detection mechanisms for those defect classes
[29], [30], [31]. A second surface is the SQL layer, where FTAD uses bound parameters
throughout, and a third is file handling, since CSV import reads an
attacker-controllable file, making header validation and per-row error reporting
security controls as much as usability features. Equally important is stating what FTAD
does *not* implement — no authentication, no authorisation model, no encryption at rest
and no network interface — so the assessment is scoped to the threats that apply to a
single-user local desktop tool, consistent with the standard's guidance that V&V be
scoped to the integrity level and operating context [9].

## 2.9 Software Reliability

Software reliability is the probability of failure-free operation over a specified
period in a specified environment. Kan gives the classical treatment, including
defect-density models and reliability growth curves relating observed failure intensity
to testing effort [6]. Practice has shifted toward just-in-time defect prediction, which
estimates the defect-proneness of an individual change rather than a module; Zhao,
Damevski and Chen survey the features, models and evaluation protocols in use [40], and
Cabral and Minku address keeping such models reliable when they operate online and must
adapt to distribution shift [41]. Two cautionary results temper enthusiasm for
prediction-based quality claims: Herbold shows the economic case for defect prediction
is far weaker than accuracy figures suggest once inspection cost and base rate are
accounted for [42], and Yao and Shepperd demonstrate that metrics such as AUC can be
biased in ways that systematically mislead the research [43] — a conclusion Stradowski
and Madeyski reach from the industry side, noting limited translation of published
models into operational use [44].

These findings informed a deliberate architectural decision. FTAD's detection engine is
rule-based and statistical — duplicate detection, absolute and relative amount
thresholds, velocity checks, and per-account z-score deviation against a strictly
earlier baseline — rather than a trained classifier. A rule-based engine has an
inspectable specification, so every decision traces to a named rule and can be asserted
by a test with a definite oracle. The trade-off is acknowledged in Chapter 8: a learned
model could capture patterns the rules miss, but it would need labelled data and would
weaken the traceability the testing programme depends on. Defect management closes the
reliability loop: Jahanshahi and Cevik show schedule and dependency information
improves bug triage [45], and FTAD's reports record severity, priority, affected
module, reproduction steps and resolution, which is the minimum structure triage
requires.

---

## 2.10 Related Studies and Existing Solutions

FTAD sits within the literature on automated financial fraud and anomaly
detection. Hilal, Gadsden and Yawney review anomaly-detection techniques applied
to financial fraud and trace the field's progression from statistical
outlier detection through classical machine learning to deep and hybrid
approaches [46]. Their review establishes the taxonomy in which FTAD's four rule
families are positioned: duplicate and velocity checks are deterministic
business rules, the large-amount rule is a threshold detector, and the
behavioural z-score is a statistical outlier detector.

Pourhabibi *et al.* review graph-based fraud detection, arguing that relational
structure between accounts, merchants and devices carries signal that
transaction-level features cannot express [47], and Motie and Raahemi extend this
to graph neural networks [48]. Both are strictly more expressive than FTAD's
per-account statistics; the gap is recorded as a limitation, not concealed.

Baesens, Höppner and Verdonck examine data engineering for fraud detection and
emphasise that feature construction and class-imbalance handling dominate
outcomes at least as much as model choice [49]. This is consistent with FTAD's
design, in which the behavioural baseline — computed per account from strictly
earlier transactions — is the component that most affects detection quality, and
which produced one of the project's more consequential defects when a
future-leaking baseline was discovered and corrected.

Interpretability is a recurring concern. Lin and Gao apply group SHAP to explain
financial fraud model outputs [50], reflecting the operational reality that a
flagged transaction must be justifiable to a reviewer and, ultimately, to the
account holder. FTAD addresses the same requirement structurally: every anomaly
record stores a `detection_method`, a numeric `risk_score`, a `risk_level` and a
human-readable `reason`, and the investigation module preserves an append-only
event history so that a reviewer's decision is auditable. The interpretability
requirement is thus satisfied by construction rather than by post-hoc
explanation.

---

## 2.11 Literature Review Summary and Identified Gaps

The reviewed literature converges on five propositions that shape this project's
strategy. First, testing is adversarial and oracle-driven, and suite adequacy is
meaningful only relative to a named criterion [1], [2], [13], [14], so coverage is
reported explicitly with exclusions documented [15]. Second, techniques are
complementary rather than ranked — combinatorial, random, structural and
specification-based approaches detect different fault classes [11], [12], [16] — so
several are applied in combination. Third, the research-to-practice gap in
regression testing is real [21]; for a suite that runs in seconds, full execution
beats the selective techniques designed for suites where it does not. Fourth,
defect-prediction accuracy is not defect-prediction value, and common metrics can
mislead [42], [43], [44], which justifies preferring an inspectable rule engine with
testable oracles over an unvalidated learned model. Fifth, relational and learned
approaches outperform per-account statistical rules on benchmark fraud data [46],
[47], [48], but impose interpretability and data-availability costs [49], [50];
FTAD occupies a defensible point on that trade-off and states its position.

Three gaps remain unaddressed by the current implementation and are carried
forward as limitations. The first is the absence of a graph representation of
transactions, which forecloses the relational detection techniques reviewed in
[47] and [48]. The second is the absence of labelled ground-truth data, which
prevents any quantitative estimate of detection precision and recall; the
project reports rule-level test outcomes instead, and does not claim accuracy
figures it cannot support. The third is the absence of authentication and
encryption, which means the security report evaluates a threat model appropriate
to a single-user local tool rather than to a multi-tenant service.

The test processes and documentation structures used in Chapters 4 and 5 follow
ISO/IEC/IEEE 29119-2 [51] and 29119-3 [52], which provide the organising
framework — organisational test policy, test planning, test monitoring, and the
standard artefact set — within which the techniques reviewed above are applied.

---

## References
[1] G. J. Myers, C. Sandler, and T. Badgett, *The Art of Software Testing*, 3rd ed. Hoboken, NJ, USA: John Wiley & Sons, 2012. doi: 10.1002/9781119202486.

[2] P. Ammann and J. Offutt, *Introduction to Software Testing*, 2nd ed. Cambridge, UK: Cambridge University Press, 2016. doi: 10.1017/9781316771273.

[3] P. C. Jorgensen, *Software Testing: A Craftsman's Approach*, 4th ed. Boca Raton, FL, USA: Auerbach Publications, 2013. doi: 10.1201/9781439889503.

[4] B. Beizer, *Software Testing Techniques*, 2nd ed. New York, NY, USA: Van Nostrand Reinhold, 1990.

[5] C. Y. Laporte and A. April, *Software Quality Assurance* Hoboken, NJ, USA: John Wiley & Sons, 2017. doi: 10.1002/9781119312451.

[6] S. H. Kan, *Metrics and Models in Software Quality Engineering*, 2nd ed. Boston, MA, USA: Addison-Wesley, 2003.

[7] ISO/IEC, *ISO/IEC 25010:2023 Systems and software Quality Requirements and Evaluation (SQuaRE) — Product quality model*, International Organization for Standardization, Geneva, Switzerland, 2023.

[8] A. Stefani, E. Vamvatsikou, and B. Vassiliadis, “Insights into B2C E-Commerce Quality Using ISO 25010,” *Journal of Software Engineering and Applications*, vol. 16, no. 11, pp. 622–639, 2023. doi: 10.4236/jsea.2023.1611032.

[9] IEEE, *IEEE Std 1012-2016 IEEE Standard for System, Software, and Hardware Verification and Validation*, IEEE, New York, NY, USA, 2017. doi: 10.1109/IEEESTD.2017.8055462.

[10] R. S. Pressman and B. R. Maxim, *Software Engineering: A Practitioner's Approach*, 9th ed. New York, NY, USA: McGraw-Hill Education, 2020.

[11] H. Wu, C. Nie, J. Petke, Y. Jia, and M. Harman, “An Empirical Comparison of Combinatorial Testing, Random Testing and Adaptive Random Testing,” *IEEE Transactions on Software Engineering*, vol. 46, no. 3, pp. 302–320, 2020. doi: 10.1109/TSE.2018.2852744.

[12] S. Liu and S. Nakajima, “Automatic Test Case and Test Oracle Generation Based on Functional Scenarios in Formal Specifications for Conformance Testing,” *IEEE Transactions on Software Engineering*, vol. 48, no. 2, pp. 691–712, 2022. doi: 10.1109/TSE.2020.2999884.

[13] A. Aghamohammadi, S. Mirian-Hosseinabadi, and S. Jalali, “Statement frequency coverage: A code coverage criterion for assessing test suite effectiveness,” *Information and Software Technology*, vol. 129, pp. 106426, 2021. doi: 10.1016/j.infsof.2020.106426.

[14] J. Lee, S. Kang, and P. Jung, “Test coverage criteria for software product line testing: Systematic literature review,” *Information and Software Technology*, vol. 122, pp. 106272, 2020. doi: 10.1016/j.infsof.2020.106272.

[15] A. Hora, “Excluding code from test coverage: practices, motivations, and impact,” *Empirical Software Engineering*, vol. 28, no. 1, 2022. doi: 10.1007/s10664-022-10259-7.

[16] P. Derakhshanfar, X. Devroey, and A. Zaidman, “Basic block coverage for search-based unit testing and crash reproduction,” *Empirical Software Engineering*, vol. 27, no. 7, 2022. doi: 10.1007/s10664-022-10155-0.

[17] I. Forgács and A. Kovács, *Modern Software Testing Techniques: A Practical Guide for Developers and Testers* Berkeley, CA, USA: Apress, 2024. doi: 10.1007/978-1-4842-9893-0.

[18] M. Kessel and C. Atkinson, “Diversity-driven unit test generation,” *Journal of Systems and Software*, vol. 193, pp. 111442, 2022. doi: 10.1016/j.jss.2022.111442.

[19] H. D. Menéndez, G. Jahangirova, F. Sarro, P. Tonella, and D. Clark, “Diversifying Focused Testing for Unit Testing,” *ACM Transactions on Software Engineering and Methodology*, vol. 30, no. 4, pp. 1–24, 2021. doi: 10.1145/3447265.

[20] A. Arcuri and J. P. Galeotti, “Handling SQL Databases in Automated System Test Generation,” *ACM Transactions on Software Engineering and Methodology*, vol. 29, no. 4, pp. 1–31, 2020. doi: 10.1145/3391533.

[21] R. Greca, B. Miranda, and A. Bertolino, “State of Practical Applicability of Regression Testing Research: A Live Systematic Literature Review,” *ACM Computing Surveys*, vol. 55, no. 13s, pp. 1–36, 2023. doi: 10.1145/3579851.

[22] S. Mondal and R. Nasre, “Colosseum: Regression Test Prioritization by Delta Displacement in Test Coverage,” *IEEE Transactions on Software Engineering*, vol. 48, no. 10, pp. 4060–4073, 2022. doi: 10.1109/TSE.2021.3111169.

[23] G. Rothermel and R. H. Untch, On “Prioritizing Test Cases for Regression Testing” *IEEE Transactions on Software Engineering*, vol. 51, no. 3, pp. 802–807, 2025. doi: 10.1109/TSE.2025.3538490.

[24] G. Maurina, W. Cazzola, and S. Ghosh, “BabelRTS: Polyglot Regression Test Selection,” *IEEE Transactions on Software Engineering*, vol. 51, no. 5, pp. 1487–1499, 2025. doi: 10.1109/TSE.2025.3554403.

[25] A. Bertolino, B. Miranda, R. Pietrantuono, and S. Russo, “Adaptive Test Case Allocation, Selection and Generation Using Coverage Spectrum and Operational Profile,” *IEEE Transactions on Software Engineering*, vol. 47, no. 5, pp. 881–898, 2021. doi: 10.1109/TSE.2019.2906187.

[26] I. Bluemke and A. Malanowska, “Software Testing Effort Estimation and Related Problems,” *ACM Computing Surveys*, vol. 54, no. 3, pp. 1–38, 2021. doi: 10.1145/3442694.

[27] Google, “GoogleTest User's Guide,” 2024. [Online]. Available: https://google.github.io/googletest/

[28] Cppcheck Project, “Cppcheck Manual,” 2024. [Online]. Available: https://cppcheck.sourceforge.io/manual.pdf

[29] Valgrind Developers, “Valgrind User Manual,” 2024. [Online]. Available: https://valgrind.org/docs/manual/manual.html

[30] LLVM Project, “AddressSanitizer and UndefinedBehaviorSanitizer Reference,” 2024. [Online]. Available: https://clang.llvm.org/docs/AddressSanitizer.html

[31] Z. Chen, Q. Zhang, J. Wu, J. Yan, and J. Xue, “A Source-Level Instrumentation Framework for the Dynamic Analysis of Memory Safety,” *IEEE Transactions on Software Engineering*, vol. 49, no. 4, pp. 2107–2127, 2023. doi: 10.1109/TSE.2022.3210580.

[32] N. U. Eisty and J. C. Carver, “Testing research software: a survey,” *Empirical Software Engineering*, vol. 27, no. 6, 2022. doi: 10.1007/s10664-022-10184-9.

[33] G. Murazvu, S. Parkinson, S. Khan, N. Liu, and G. Allen, “A Survey on Factors Preventing the Adoption of Automated Software Testing: A Principal Component Analysis Approach,” *Software*, vol. 3, no. 1, pp. 1–27, 2024. doi: 10.3390/software3010001.

[34] S. Stradowski and L. Madeyski, “Exploring the challenges in software testing of the 5G system at Nokia: A survey,” *Information and Software Technology*, vol. 153, pp. 107067, 2023. doi: 10.1016/j.infsof.2022.107067.

[35] S. Fatima, H. Hemmati, and L. C. Briand, “FlakyFix: Using Large Language Models for Predicting Flaky Test Fix Categories and Test Code Repair,” *IEEE Transactions on Software Engineering*, vol. 50, no. 12, pp. 3146–3171, 2024. doi: 10.1109/TSE.2024.3472476.

[36] M. Imran, V. Cortellessa, D. Di Ruscio, R. Rubei, and L. Traini, “Is code coverage of performance tests related to source code features? An empirical study on open-source Java systems,” *Empirical Software Engineering*, vol. 30, no. 6, 2025. doi: 10.1007/s10664-025-10712-3.

[37] L. Traini and V. Cortellessa, “DeLag: Using Multi-Objective Optimization to Enhance the Detection of Latency Degradation Patterns in Service-Based Systems,” *IEEE Transactions on Software Engineering*, 2023. doi: 10.1109/TSE.2023.3266041.

[38] P. Lachkov, L. Tawalbeh, and S. Bhatt, “Vulnerability Assessment for Applications Security Through Penetration Simulation and Testing,” *Journal of Web Engineering*, 2022. doi: 10.13052/jwe1540-9589.2178.

[39] M. Leithner, B. Garn, and D. E. Simos, “HYDRA: Feedback-driven black-box exploitation of injection vulnerabilities,” *Information and Software Technology*, vol. 140, pp. 106703, 2021. doi: 10.1016/j.infsof.2021.106703.

[40] Y. Zhao, K. Damevski, and H. Chen, “A Systematic Survey of Just-in-Time Software Defect Prediction,” *ACM Computing Surveys*, vol. 55, no. 10, pp. 1–35, 2023. doi: 10.1145/3567550.

[41] G. E. Cabral and L. L. Minku, “Towards Reliable Online Just-in-Time Software Defect Prediction,” *IEEE Transactions on Software Engineering*, vol. 49, no. 3, pp. 1342–1358, 2023. doi: 10.1109/TSE.2022.3175789.

[42] S. Herbold, “On the Costs and Profit of Software Defect Prediction,” *IEEE Transactions on Software Engineering*, vol. 47, no. 11, pp. 2617–2631, 2021. doi: 10.1109/TSE.2019.2957794.

[43] J. Yao and M. Shepperd, “The impact of using biased performance metrics on software defect prediction research,” *Information and Software Technology*, vol. 139, pp. 106664, 2021. doi: 10.1016/j.infsof.2021.106664.

[44] S. Stradowski and L. Madeyski, “Machine learning in software defect prediction: A business-driven systematic mapping study,” *Information and Software Technology*, vol. 155, pp. 107128, 2023. doi: 10.1016/j.infsof.2022.107128.

[45] H. Jahanshahi and M. Cevik, “S-DABT: Schedule and Dependency-aware Bug Triage in open-source bug tracking systems,” *Information and Software Technology*, vol. 151, pp. 107025, 2022. doi: 10.1016/j.infsof.2022.107025.

[46] W. Hilal, S. A. Gadsden, and J. Yawney, “Financial Fraud: A Review of Anomaly Detection Techniques and Recent Advances,” *Expert Systems with Applications*, vol. 193, pp. 116429, 2022. doi: 10.1016/j.eswa.2021.116429.

[47] T. Pourhabibi, K. Ong, B. H. Kam, and Y. L. Boo, “Fraud detection: A systematic literature review of graph-based anomaly detection approaches,” *Decision Support Systems*, vol. 133, pp. 113303, 2020. doi: 10.1016/j.dss.2020.113303.

[48] S. Motie and B. Raahemi, “Financial fraud detection using graph neural networks: A systematic review,” *Expert Systems with Applications*, vol. 240, pp. 122156, 2024. doi: 10.1016/j.eswa.2023.122156.

[49] B. Baesens, S. Höppner, and T. Verdonck, “Data engineering for fraud detection,” *Decision Support Systems*, vol. 150, pp. 113492, 2021. doi: 10.1016/j.dss.2021.113492.

[50] K. Lin and Y. Gao, “Model interpretability of financial fraud detection by group SHAP,” *Expert Systems with Applications*, vol. 210, pp. 118354, 2022. doi: 10.1016/j.eswa.2022.118354.

[51] ISO/IEC/IEEE, *ISO/IEC/IEEE 29119-2:2021 Software and systems engineering — Software testing — Part 2: Test processes*, International Organization for Standardization, Geneva, Switzerland, 2021.

[52] ISO/IEC/IEEE, *ISO/IEC/IEEE 29119-3:2021 Software and systems engineering — Software testing — Part 3: Test documentation*, International Organization for Standardization, Geneva, Switzerland, 2021.

---

---

## Appendix — Reference verification summary

| Category | Count | Machine-verified |
|---|---|---|
| Books | 8 | 8 (5 via Crossref DOI, 3 via Open Library ISBN) |
| Journal articles | 36 | 36 via Crossref DOI |
| Standards | 4 | 0 (iso.org blocks automated clients; cited by number) |
| Tool documentation | 4 | 4 (URL fetched, HTTP 200) |
| **Total** | **52** | **48** |

**Academic ratio (journals + books):** 44 of 52 = **84.6%**, satisfying the
requirement that at least 80% of references be academic journals and books.

**Recency:** of the 36 journal articles, 24 (66.7%) were published in 2022 or
later, 14 (38.9%) in 2023 or later, and 32 (88.9%) in 2021 or later. The
lecturer's "three-year" requirement is
ambiguous as stated — it could mean publication within three years of submission,
or a requirement that a proportion of references be recent. This chapter
interprets it as the former, prefers recent work wherever a recent source
supports the same claim, and retains older foundational sources where they remain
the authoritative statement of a concept (for example [1], [3], [4]). The
ambiguity is flagged rather than resolved silently.

**Not verified:** the four ISO/IEC and IEEE standards are cited by standard
number. Automated verification was attempted and blocked (HTTP 403). They should
be checked manually against the university library catalogue before submission.
