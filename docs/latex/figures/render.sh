#!/bin/bash
# Render the six architectural views to PDF for inclusion in Appendix G.
# Graphviz is used because mermaid-cli is not installed on this host and
# installing it would pull in a Chromium download.
set -euo pipefail
cd "$(dirname "$0")"

FONT="Tahoma"
COMMON='graph [fontname="'"$FONT"'", fontsize=10, nodesep=0.25, ranksep=0.35, pad=0.15];
        node  [fontname="'"$FONT"'", fontsize=9, shape=box, style="rounded,filled",
               fillcolor="#f7f7f7", color="#666666", penwidth=0.8];
        edge  [fontname="'"$FONT"'", fontsize=8, color="#555555", arrowsize=0.6];'

# ---------------------------------------------------------------- G.1 system
cat > system_architecture.dot <<EOF
digraph G {
  rankdir=TB; $COMMON
  subgraph cluster_client {
    label="Qt 6 desktop client (frontend/)"; labelloc="t"; style="rounded,dashed"; color="#999999";
    MW [label="MainWindow\nQStackedWidget", fillcolor="#e8eef7"];
    HP [label="HomePage"]; TP [label="TransactionsPage"]; DP [label="DetectionPage"];
    IP [label="InvestigationsPage"]; RP [label="ReportsPage*"]; SP [label="SettingsPage*"];
    BP [label="BasePage"];
  }
  subgraph cluster_backend {
    label="ftad_core (static library, C++17)"; labelloc="b"; style="rounded,dashed"; color="#999999";
    DS [label="DetectionService\nanalyzeAll / flaggedResults", fillcolor="#eef3e8"];
    TR [label="TransactionRepository\nCRUD / normalise / validate", fillcolor="#eef3e8"];
    CI [label="CsvImportService\nparse / validate", fillcolor="#eef3e8"];
    AN [label="AnomalyDetector\n4 rules", fillcolor="#eef3e8"];
    BA [label="BehaviorAnalyzer\nprefix stats / z-score", fillcolor="#eef3e8"];
    RS [label="RiskScorer\nweights / bands", fillcolor="#eef3e8"];
    DR [label="DetectionRepository\npersist / load", fillcolor="#eef3e8"];
    INS [label="InvestigationRepository\ncases / history", fillcolor="#eef3e8"];
    DM [label="DatabaseManager\nconnect / schema", fillcolor="#eef3e8"];
  }
  DB [label="SQLite\ndatabase/schema.sql", shape=cylinder, fillcolor="#fdf3e0"];

  MW -> {HP TP DP IP RP SP BP} [style=dotted, arrowhead=none]
  HP -> DS; TP -> TR; TP -> CI; DP -> DS; DP -> DR; IP -> INS; SP -> DS
  DS -> TR; DS -> AN; DS -> DR
  AN -> BA; AN -> RS
  CI -> TR
  {TR DR INS DM} -> DM [style=dotted]
  DM -> DB [penwidth=1.2]
}
EOF

# ---------------------------------------------------------------- G.2 classes
cat > class_diagram.dot <<EOF
digraph G {
  rankdir=LR; $COMMON
  subgraph cluster_svc {
    label="Services"; style="rounded,dashed"; color="#999999";
    DS [label="DetectionService", fillcolor="#eef3e8"];
    AN [label="AnomalyDetector", fillcolor="#eef3e8"];
    BA [label="BehaviorAnalyzer", fillcolor="#eef3e8"];
    RS [label="RiskScorer", fillcolor="#eef3e8"];
    TR [label="TransactionRepository", fillcolor="#eef3e8"];
    CI [label="CsvImportService", fillcolor="#eef3e8"];
    DR [label="DetectionRepository", fillcolor="#eef3e8"];
    INS [label="InvestigationRepository", fillcolor="#eef3e8"];
    DM [label="DatabaseManager", fillcolor="#eef3e8"];
  }
  subgraph cluster_val {
    label="Value types"; style="rounded,dashed"; color="#999999";
    TX [label="Transaction", fillcolor="#e8eef7"];
    RES [label="DetectionResult", fillcolor="#e8eef7"];
    HIT [label="RuleHit", fillcolor="#e8eef7"];
    DEV [label="DeviationResult", fillcolor="#e8eef7"];
    PRF [label="AccountProfile", fillcolor="#e8eef7"];
    CASE [label="InvestigationCase", fillcolor="#e8eef7"];
    EV [label="InvestigationEvent", fillcolor="#e8eef7"];
    SET [label="DetectionSettings", fillcolor="#fdf3e0"];
  }
  DS -> AN; DS -> DR; DS -> TR; AN -> BA; AN -> RS; AN -> SET; RS -> SET; BA -> SET
  CI -> TR; TR -> DM; DR -> DM; INS -> DM
  DS -> RES [style=dashed]; AN -> HIT [style=dashed]; BA -> DEV [style=dashed]
  BA -> PRF [style=dashed]; TR -> TX [style=dashed]
  INS -> CASE [style=dashed]; INS -> EV [style=dashed]
}
EOF

# ---------------------------------------------------------------- G.3 use case
cat > use_case.dot <<EOF
digraph G {
  rankdir=LR; $COMMON
  A [label="Fraud analyst\n(investigator)", shape=box, style="rounded,filled", fillcolor="#e8eef7"]
  subgraph cluster_sys {
    label="FTAD"; style="rounded,dashed"; color="#999999";
    U1 [label="Import / enter\ntransactions", fillcolor="#eef3e8"]
    U2 [label="Run analysis", fillcolor="#eef3e8"]
    U3 [label="Review cases", fillcolor="#eef3e8"]
    U4 [label="View reports", fillcolor="#eef3e8"]
  }
  DB [label="SQLite", shape=cylinder, fillcolor="#fdf3e0"]
  A -> U1; A -> U2; A -> U3; A -> U4
  U1 -> DB; U2 -> DB; U3 -> DB
}
EOF

# ---------------------------------------------------------------- G.4 activity
cat > activity_detection.dot <<EOF
digraph G {
  rankdir=TB; $COMMON
  S  [label="Application start", shape=box, style="rounded,filled", fillcolor="#e8eef7"]
  C  [label="DatabaseManager.connect"]
  SC [label="initializeSchema"]
  L  [label="Load transactions"]
  F  [label="For each transaction", shape=diamond, fillcolor="#fdf3e0"]
  D1 [label="Duplicate check"]
  D2 [label="Large amount check"]
  D3 [label="Rapid withdrawal check"]
  D4 [label="Behavioural deviation check"]
  R  [label="RiskScorer.score"]
  P  [label="DetectionRepository\n.saveResults"]
  FL [label="Flagged?", shape=diamond, fillcolor="#fdf3e0"]
  O  [label="DetectionPage /\nInvestigations"]
  N  [label="More\ntransactions?", shape=diamond, fillcolor="#fdf3e0"]
  E  [label="End", shape=box, style="rounded,filled", fillcolor="#e8eef7"]
  S->C->SC->L->F
  F->D1; F->D2; F->D3; F->D4
  {D1 D2 D3 D4}->R
  R->P->FL
  FL->O [label=" yes"]
  FL->N [label=" no"]
  N->F [label=" yes"]
  N->E [label=" no"]
}
EOF

# ---------------------------------------------------------------- G.5 sequence
cat > sequence_detection.dot <<EOF
digraph G {
  rankdir=LR; $COMMON
  node [shape=box, style="rounded,filled", fillcolor="#eef3e8"]
  UI [label="Qt UI\nDetectionPage", fillcolor="#e8eef7"]
  M1 [label="1  analyzeAll()", shape=plaintext, style="", fillcolor="#ffffff"]
  DS [label="DetectionService"]
  M2 [label="2  getAllTransactions()", shape=plaintext, style="", fillcolor="#ffffff"]
  TR [label="TransactionRepository"]
  M3 [label="3  analyze()", shape=plaintext, style="", fillcolor="#ffffff"]
  AN [label="AnomalyDetector"]
  M4 [label="4  saveResults()", shape=plaintext, style="", fillcolor="#ffffff"]
  DR [label="DetectionRepository"]
  M5 [label="5  counts + results", shape=plaintext, style="", fillcolor="#ffffff"]
  UI -> M1 -> DS -> M2 -> TR -> M3 -> AN -> M4 -> DR -> M5
  M5 -> UI [style=dashed, constraint=false, label="returns", fontcolor="#777777"]
}
EOF

# ---------------------------------------------------------------- G.6 ERD
cat > erd.dot <<EOF
digraph G {
  rankdir=LR; $COMMON
  node [shape=plaintext, style="", fontname="Courier New", fontsize=8]
  T [label=<<TABLE BORDER="1" CELLBORDER="0" CELLSPACING="0" CELLPADDING="3">
     <TR><TD BGCOLOR="#e8eef7"><B>transactions</B></TD></TR>
     <TR><TD ALIGN="LEFT">id  INTEGER PK</TD></TR>
     <TR><TD ALIGN="LEFT">transaction_id  TEXT UK</TD></TR>
     <TR><TD ALIGN="LEFT">timestamp  TEXT</TD></TR>
     <TR><TD ALIGN="LEFT">sender_id  TEXT</TD></TR>
     <TR><TD ALIGN="LEFT">receiver_id  TEXT</TD></TR>
     <TR><TD ALIGN="LEFT">transaction_type  TEXT</TD></TR>
     <TR><TD ALIGN="LEFT">amount  REAL</TD></TR>
     <TR><TD ALIGN="LEFT">currency  TEXT</TD></TR>
     <TR><TD ALIGN="LEFT">status  TEXT</TD></TR></TABLE>>]
  A [label=<<TABLE BORDER="1" CELLBORDER="0" CELLSPACING="0" CELLPADDING="3">
     <TR><TD BGCOLOR="#eef3e8"><B>anomalies</B></TD></TR>
     <TR><TD ALIGN="LEFT">id  INTEGER PK</TD></TR>
     <TR><TD ALIGN="LEFT">transaction_id  FK</TD></TR>
     <TR><TD ALIGN="LEFT">detection_method  TEXT</TD></TR>
     <TR><TD ALIGN="LEFT">risk_score  REAL</TD></TR>
     <TR><TD ALIGN="LEFT">risk_level  TEXT</TD></TR>
     <TR><TD ALIGN="LEFT">reason  TEXT</TD></TR>
     <TR><TD ALIGN="LEFT">detected_at  TEXT</TD></TR></TABLE>>]
  I [label=<<TABLE BORDER="1" CELLBORDER="0" CELLSPACING="0" CELLPADDING="3">
     <TR><TD BGCOLOR="#fdf3e0"><B>investigations</B></TD></TR>
     <TR><TD ALIGN="LEFT">id  INTEGER PK</TD></TR>
     <TR><TD ALIGN="LEFT">anomaly_id  FK</TD></TR>
     <TR><TD ALIGN="LEFT">case_status  TEXT</TD></TR>
     <TR><TD ALIGN="LEFT">assigned_to  TEXT</TD></TR>
     <TR><TD ALIGN="LEFT">notes  TEXT</TD></TR>
     <TR><TD ALIGN="LEFT">opened_at  TEXT</TD></TR>
     <TR><TD ALIGN="LEFT">closed_at  TEXT</TD></TR></TABLE>>]
  E [label=<<TABLE BORDER="1" CELLBORDER="0" CELLSPACING="0" CELLPADDING="3">
     <TR><TD BGCOLOR="#f7f7f7"><B>investigation_events</B></TD></TR>
     <TR><TD ALIGN="LEFT">id  INTEGER PK</TD></TR>
     <TR><TD ALIGN="LEFT">investigation_id  FK</TD></TR>
     <TR><TD ALIGN="LEFT">event_type  TEXT</TD></TR>
     <TR><TD ALIGN="LEFT">previous_status  TEXT</TD></TR>
     <TR><TD ALIGN="LEFT">new_status  TEXT</TD></TR>
     <TR><TD ALIGN="LEFT">notes  TEXT</TD></TR>
     <TR><TD ALIGN="LEFT">created_at  TEXT</TD></TR></TABLE>>]
  T -> A [label="1 : 1..1", arrowhead=crow, arrowtail=none, dir=both]
  A -> I [label="1 : 0..1", arrowhead=crow, arrowtail=none, dir=both]
  I -> E [label="1 : N", arrowhead=crow, arrowtail=none, dir=both]
}
EOF

for f in system_architecture class_diagram use_case activity_detection sequence_detection erd; do
    dot -Tpdf "$f.dot" -o "$f.pdf"
    printf "  %-22s %s bytes\n" "$f.pdf" "$(stat -c%s "$f.pdf")"
done
