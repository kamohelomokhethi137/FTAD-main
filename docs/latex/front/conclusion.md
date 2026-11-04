# Chapter 8 — Conclusion

The testing campaign set out to establish whether FTAD implements its
specification, and on the evidence gathered it does. All 154 automated tests
pass, coverage reaches 85.4 % of lines and 97.4 % of functions, Cppcheck finds
nothing in application source, and three independent dynamic tools —
AddressSanitizer, UndefinedBehaviorSanitizer and Valgrind — report no memory or
undefined-behaviour errors across the full suite. Six defects were found and
fixed. Four of them were found by tests written for this campaign rather than
during construction, which is the result worth carrying forward: the suite
detected failures that building the software had not. The shipped sample dataset
was shown to exercise all four detection rules end to end, with one transaction
triggering two rules simultaneously and so exercising the additive scoring path.
Performance work produced the largest single improvement in the project, from
40.50 s to 27.6 ms at 20,000 transactions, and it came from a defect that no
amount of code review would have found, because the offending call was idiomatic
and its result was correct.

The limits of that evidence matter as much as the evidence. Branch coverage is
51.4 %, so while most lines are executed, roughly half of all decision outcomes
are never exercised; the line figure should not be read as a statement about
decision coverage. The lowest coverage sits in persistence error paths that
cannot be reached without injecting a fault. No labelled fraud corpus exists, so
no precision, recall or accuracy figure is claimed anywhere in this report — the
rules are shown to implement their definitions, not to identify real fraud. The
graphical interface was assessed by inspection only, because no widget-level
harness was built; `ReportsPage` and `SettingsPage` remain placeholders with no
behaviour to test. Two latent defects remain open by choice, both unreachable in
the shipped configuration, and two security findings remain open: the database
is created world-readable, and nothing is encrypted at rest. Everything was
measured on one Linux host, so portability is declared rather than demonstrated.

Three actions follow. First, the open items should be closed and the gates
re-run: setting the database to mode 0600, bounding the two window settings and
making `hasBaseline()` and `profile()` agree with `evaluate()` on
`minimumHistory` together amount to under ten lines of change, and they were
deferred only because applying them would have invalidated a measured baseline.
Second, branch coverage should be raised deliberately by adding negative-path
tests to the repository methods, which is where the gap is concentrated. Third,
if the detector is ever to be claimed as accurate, a labelled dataset must be
obtained and precision and recall measured against it; until then the honest
description of FTAD is a rule-based screening aid whose rules work as specified,
not a fraud detector with demonstrated detection performance.
