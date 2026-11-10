#include "ReportsPage.h"

#include "AnomalyDetector.h"
#include "DetectionService.h"
#include "DetectionSettings.h"
#include "DetectionSettingsStore.h"
#include "TransactionRepository.h"
#include "Transaction.h"
#include "DetectionResult.h"

#include <QAbstractItemView>
#include <QBrush>
#include <QColor>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QShowEvent>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QMap>
#include <QSet>

namespace
{
QString levelOrDash(const DetectionResult &r)
{
    if (!r.flagged()) return "-";
    return r.riskLevel.isEmpty() ? "low" : r.riskLevel;
}
QString methodsFor(const DetectionResult &r)
{
    if (!r.flagged()) return "-";
    const QString m = r.methodText();
    return m.isEmpty() ? "-" : m;
}
QString scoreFor(const DetectionResult &r)
{
    if (!r.flagged()) return "-";
    return QString::number(r.riskScore, 'f', 1);
}
}

ReportsPage::ReportsPage(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(28, 28, 28, 28);
    mainLayout->setSpacing(14);

    auto *title = new QLabel("Reports");
    title->setObjectName("heading");
    auto *subtitle = new QLabel(
        "Generate a detection comparison report showing rule-based, ML-model, and combined outcomes side by side.");
    subtitle->setObjectName("description");
    subtitle->setWordWrap(true);

    auto *toolbar = new QHBoxLayout();
    generateButton = new QPushButton("Generate rule vs ML comparison");
    auto *hint = new QLabel("Read-only report (does not overwrite saved detection results).");
    hint->setObjectName("description");
    toolbar->addWidget(generateButton);
    toolbar->addWidget(hint, 1);

    summaryLabel = new QLabel("No report generated yet.");
    summaryLabel->setObjectName("panel");
    summaryLabel->setWordWrap(true);

    notes = new QPlainTextEdit();
    notes->setReadOnly(true);
    notes->setMaximumHeight(130);
    notes->setPlainText(
        "Why differences occur:\n"
        "- Rule engine is deterministic and explains pattern/context checks (e.g. high_amount, duplicate_payment, rapid_withdrawals, behaviour_deviation) using configured weights/thresholds.\n"
        "- ML model (HistGradientBoosting, PaySim-trained) scores ONLY transaction type + amount (0-100). It does not see duplicates, burst frequency, sender/receiver relationships, or personal history.\n"
        "- Combined level = sum of all hit weights/scores mapped to bands (low > 0, medium >= 35, high >= 65, capped 100). A transaction can be rule-flagged but ML-low (or vice versa).\n"
        "- ML metrics shown in project artifacts are offline evaluation (e.g. PR-AUC/ROC-AUC on PaySim split), not live UI accuracy.");

    table = new QTableWidget();
    table->setColumnCount(10);
    table->setHorizontalHeaderLabels({
        "Transaction", "Amount", "Type",
        "Rule Score", "Rule Level", "Rule Findings",
        "ML Score", "ML Level", "ML Findings",
        "Combined"
    });
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setAlternatingRowColors(true);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setStretchLastSection(true);

    mainLayout->addWidget(title);
    mainLayout->addWidget(subtitle);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(summaryLabel);
    mainLayout->addWidget(notes);
    mainLayout->addWidget(table, 1);

    QObject::connect(generateButton, &QPushButton::clicked, [this]() { generateComparisonReport(); });
}

void ReportsPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
}

void ReportsPage::generateComparisonReport()
{
    TransactionRepository repo;
    const QList<Transaction> transactions = repo.getAllTransactions();
    if (!repo.lastError().isEmpty())
    {
        summaryLabel->setText("Failed to load transactions: " + repo.lastError());
        table->setRowCount(0);
        return;
    }

    DetectionSettings base = DetectionSettingsStore::load();
    DetectionSettings ruleS = base; ruleS.useRuleEngine = true;  ruleS.useMlModel = false;
    DetectionSettings mlS   = base; mlS.useRuleEngine   = false; mlS.useMlModel   = true;
    DetectionSettings combS = base; // as configured/saved (may be rule-only, ml-only, or both)

    auto run = [&](const DetectionSettings &s) {
        QMap<int, DetectionResult> map;
        AnomalyDetector d(s);
        for (const DetectionResult &r : d.analyze(transactions)) map.insert(r.transactionId, r);
        return map;
    };

    const QMap<int, DetectionResult> ruleMap = run(ruleS);
    const QMap<int, DetectionResult> mlMap = run(mlS);
    const QMap<int, DetectionResult> combMap = run(combS);

    int ruleFlag=0, mlFlag=0, combFlag=0, agree=0, diff=0;
    for (const Transaction &t : transactions)
    {
        const bool r = ruleMap.value(t.id).flagged();
        const bool m = mlMap.value(t.id).flagged();
        const bool c = combMap.value(t.id).flagged();
        if (r) ++ruleFlag;
        if (m) ++mlFlag;
        if (c) ++combFlag;
        if (r == m) ++agree; else ++diff;
    }

    QStringList engines;
    if (base.useRuleEngine) engines << "rule engine";
    if (base.useMlModel) engines << "ML model";
    summaryLabel->setText(QString(
        "Transactions: %1 | Flagged — Rule: %2, ML: %3, Combined(saved config: %4): %5 | "
        "Rule vs ML agreement: %6, differences: %7. "
        "This report shows per-engine results (rule-only, ML-only) and combined outcome under saved settings.")
        .arg(transactions.size()).arg(ruleFlag).arg(mlFlag)
        .arg(engines.isEmpty() ? "none" : engines.join(" + ")).arg(combFlag)
        .arg(agree).arg(diff));

    table->setRowCount(transactions.size());
    for (int row = 0; row < transactions.size(); ++row)
    {
        const Transaction &t = transactions.at(row);
        const DetectionResult rr = ruleMap.value(t.id);
        const DetectionResult mr = mlMap.value(t.id);
        const DetectionResult cr = combMap.value(t.id);

        auto set = [&](int col, const QString &text) {
            auto *item = new QTableWidgetItem(text);
            table->setItem(row, col, item);
        };

        set(0, t.transactionId);
        set(1, QString("%1 %2").arg(t.currency).arg(t.amount, 0, 'f', 2));
        set(2, t.transactionType);
        set(3, scoreFor(rr)); set(4, levelOrDash(rr)); set(5, methodsFor(rr));
        set(6, scoreFor(mr)); set(7, levelOrDash(mr)); set(8, methodsFor(mr));
        set(9, cr.flagged() ? QString("%1 (%2)").arg(levelOrDash(cr), scoreFor(cr)) : "-");

        // Visual cue for disagreement (subtle via text already; keep B&W-safe)
        if (rr.flagged() != mr.flagged())
        {
            for (int c = 0; c < table->columnCount(); ++c)
                if (table->item(row,c)) table->item(row,c)->setToolTip("Rule and ML disagree on flagging for this transaction.");
        }
    }
}
