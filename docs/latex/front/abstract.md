# Abstract

This report documents the verification of FTAD, a single-user desktop
application written in C++17 with Qt 6 and SQLite that flags suspicious
financial transactions using four rule-based statistical checks. The
application comprises 4,612 executable source lines across 41 files. Testing
followed a risk-based plan derived from ISO/IEC/IEEE 29119-3 test documentation
and applied equivalence partitioning, boundary value analysis, decision tables
and state-transition testing to produce 45 designed cases, implemented as 154
automated tests in ten suites. All 154 tests pass in 5.42 s, with 85.4 % line
coverage (992 of 1,162 lines), 97.4 % function coverage and 51.4 % branch
coverage. Cppcheck reports no findings in application source, Clang-Tidy reports
six diagnostics that were individually triaged, and AddressSanitizer,
UndefinedBehaviorSanitizer and Valgrind report no errors across the full suite.
Six defects were found and fixed, four of them by tests written for this
campaign rather than during construction; two latent defects and two security
findings remain open and are documented with their reachability and remedies.
Performance measurement exposed a defect invisible to code review: timestamps
were constructed as local time, resolving the system time zone once per record.
Rebuilding them explicitly in UTC reduced analysis of 20,000 transactions from
40.50 s to 27.6 ms, a factor of approximately 1,466. Because no labelled fraud
corpus exists, this report makes no claim about detection accuracy: it
establishes that each rule implements its specification, which is a different
and narrower claim than that the specification detects real fraud.
