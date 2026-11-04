# Appendix G — Architectural Views

**Companion to:** Chapter 3, §3.4
**Status:** Reference material — excluded from the body page count

---

The six views below were prepared as Mermaid sources under
`documentation/diagrams/` and rendered for this report with Graphviz. They are
reference material: Chapter 3 states the structure in prose, and these figures show
it. Each was checked against the module list in §3.3 and the schema in
`database/schema.sql`; where a page is a placeholder it is marked with an asterisk.

![Figure G.1 — Layering and call relationships. The Qt client sits above the
`ftad_core` static library, which is the only component that touches SQLite.
`ReportsPage` and `SettingsPage` (asterisked) are placeholders.](figures/system_architecture.pdf){width=100%}

![Figure G.2 — Backend classes. Services on the left collaborate through the
value types on the right; `DetectionSettings` (shaded) is the shared
configuration object that every rule reads.](figures/class_diagram.pdf){width=95%}

![Figure G.3 — Actors and use cases. A single analyst role drives import,
analysis, case review and reporting, all against the local
database.](figures/use_case.pdf){width=62%}

![Figure G.4 — Detection activity. Every transaction passes all four rule checks
before scoring, so a single transaction can accumulate several
hits.](figures/activity_detection.pdf){width=88%}

![Figure G.5 — Call sequence for one analysis pass, from the UI through
`DetectionService` to persistence and back.](figures/sequence_detection.pdf){width=100%}

![Figure G.6 — Entity relationships. Foreign keys run transactions → anomalies →
investigations → investigation_events, with cascade deletes in the same
direction.](figures/erd.pdf){width=100%}
