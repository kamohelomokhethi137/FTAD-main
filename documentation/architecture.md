# FTAD — System Architecture

> **Source root:** `/home/hashcat/Documents/dataset/FTAD-main`. The architecture
> below was read from the source tree on 4 November 2026; the module table and the
> layering figure are statements about the code, not about intent.

---

## 3.1 Overview

The application is a Qt 6 desktop client in C++17 that connects to a local SQLite
database, ingests financial transactions, runs a statistical anomaly-detection
engine, persists risk findings and supports a fraud-investigation workflow. The
backend is compiled once as the static library `ftad_core`; both the Qt application
and the GoogleTest suite link against it, so the logic is tested with the same code
the application uses. That single-link decision is what makes the test strategy in
Chapter 4 affordable, and it is the most consequential structural choice in the
project.

## 3.2 Layering

```
┌────────────────────────────────────────────┐
│ Qt Widgets layer (frontend/)               │
│ MainWindow + page stack                    │
│ Home / Transactions / Detection /          │
│ Investigations / Reports / Settings        │
└──────────────────┬─────────────────────────┘
                   │  in-process calls
┌──────────────────▼─────────────────────────┐
│ Backend library (ftad_core)                │
│ DatabaseManager, TransactionRepository,    │
│ CsvImportService, DetectionService,        │
│ AnomalyDetector, BehaviorAnalyzer,         │
│ RiskScorer, DetectionRepository,           │
│ InvestigationRepository                    │
└──────────────────┬─────────────────────────┘
                   │  Qt SQL (QSQLITE)
┌──────────────────▼─────────────────────────┐
│ SQLite database (database/schema.sql)      │
└────────────────────────────────────────────┘
```

Three layers, two boundaries. The upper boundary is an in-process call: there is no
IPC, no serialisation and no network hop, so a frontend defect and a backend defect
are separated by a function signature rather than by a protocol. The lower boundary
is Qt's `QSQLITE` driver, which is the only place the application touches persistent
state.

## 3.3 Module Responsibilities

| Module | Responsibility |
|--------|----------------|
| `DatabaseManager` | Connection lifecycle, schema initialisation, error state. |
| `TransactionRepository` | Transaction persistence, ID normalisation, validation, duplicate-ID rejection, next-ID allocation. |
| `CsvImportService` | CSV parsing (quotes, blank lines), header/column validation, per-row error reporting, import execution. |
| `DetectionSettings` | Thresholds, weights, risk bands, validation rules. |
| `BehaviorAnalyzer` | Per-account prefix statistics over strictly earlier transactions; z-score; uniform-history ratio. |
| `RiskScorer` | Weighted scoring, level assignment, primary method, reason text. |
| `AnomalyDetector` | Rule orchestration: duplicate, large-amount, rapid-withdrawal, behaviour. |
| `DetectionRepository` | Persistence and retrieval of `DetectionResult` rows. |
| `DetectionService` | Ties detection to storage: load, analyse, persist, report counts. |
| `InvestigationRepository` | Case lifecycle, status transitions, audit events. |

## 3.4 Architectural Views

Six views were prepared as Mermaid sources under `documentation/diagrams/` and are
reproduced in the accompanying documentation set. Their content is as follows.
`system_architecture.mmd` shows the client → backend → database composition and the
call relationships between the Qt pages and the backend modules — the same structure
as the layering figure above, with the page-to-service edges made explicit.
`class_diagram.mmd` shows the backend value types (`Transaction`, `DetectionResult`,
`RuleHit`, `DeviationResult`, `AccountProfile`, `InvestigationCase`,
`InvestigationEvent`) together with the services that collaborate with them.
`use_case.mmd` gives the actors and use cases: an investigator enters or imports
transactions, runs analysis, reviews cases and views reports. `activity_detection.mmd`
traces the detection workflow from connect and schema application through per-transaction
checks, scoring and persistence to the flagged decision. `sequence_detection.mmd`
gives the runtime interaction for one analysis pass —
`UI → DetectionService → TransactionRepository → AnomalyDetector → DetectionRepository`.
`erd.mmd` gives the entity relationships: `transactions` (1) to `anomalies` (1..1)
via `transaction_id`, `anomalies` (1) to `investigations` (1) via `anomaly_id`, and
`investigations` (1) to `investigation_events` (N) via `investigation_id`.

## 3.5 Data Flow and Storage

The runtime sequence is short because everything is in-process. A UI action calls a
backend service; the service loads transactions from SQLite; the detector computes
results; results are persisted and the flagged set is returned; and the investigation
workflow then reads and writes cases and events. Persistence is SQLite through the
Qt SQL driver, with the schema at `database/schema.sql` defining four tables —
`transactions`, `anomalies`, `investigations` and `investigation_events`. Foreign
keys are enforced, with cascade deletes running anomaly → investigation → events, so
removing a transaction cannot leave an orphaned case behind.

## 3.6 Error Handling

Errors propagate rather than abort. `DatabaseManager::lastError()` is surfaced
through the services, `DetectionService::analyzeAll` returns false on invalid
settings or a database error, and the repositories return error strings for invalid
identifiers, amounts, timestamps, currencies and statuses. This matters for testing:
because failure is a return value rather than an exception, every failure path is
reachable by a normal call with bad input, which is why the negative cases in
Appendix B.5 can be asserted directly instead of by fault injection.
