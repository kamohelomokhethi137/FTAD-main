# Introduction

Financial institutions detect fraudulent transactions by looking for patterns
that are unusual relative to an account's own history: an amount far above what
that account normally moves, the same payment submitted twice within minutes, a
burst of withdrawals in an hour, or a transfer to a receiver never seen before.
At small institutions and in coursework environments this review is often
manual, and manual review does not scale. A reviewer can inspect a few dozen
transactions a day; a ledger produces thousands. The gap between what can be
reviewed and what is recorded is where losses accumulate, and it is also where
naive automation does damage, because a detector that flags everything is
indistinguishable in practice from one that flags nothing.

FTAD (Financial Transaction Anomaly Detector) was built to occupy that gap
deliberately narrowly. It is a single-user desktop application in C++17 using
Qt 6 for the interface and SQLite for storage. It imports transactions from CSV,
applies four detection rules — duplicate payment, large amount, rapid
withdrawals, and deviation from an account's behavioural baseline — scores each
finding by summing rule weights into a 0–100 risk score, bands that score into
low, medium and high, and lets a reviewer open an investigation case, assign it,
annotate it and close it, with every change recorded as an event. It is
rule-based and statistical rather than machine-learned, and that choice is
stated plainly because it determines what the system can and cannot be shown to
do.

This report is a verification record, not a construction log. It asks whether
the application does what it claims, and it answers with measurements rather
than assertions: every figure that follows was produced by a command whose
output is retained, and where a number varies between runs the range is given
rather than the most favourable value. Chapter 1 audits the codebase. Chapter 2
reviews the testing and reliability literature that shaped the plan. Chapter 3
describes the architecture. Chapter 4 sets out the test plan and its exit
criteria. Chapter 5 reports what happened when that plan was executed. Chapter 6
assesses security, performance and the defects found. Chapter 7 is the
installation manual, and Chapter 8 concludes. The contribution is not a novel
detection algorithm; it is a demonstration that a small rule-based system can be
verified to a defined standard, and an honest account of the places where this
one was not.
