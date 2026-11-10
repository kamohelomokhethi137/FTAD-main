
#include "DetectionPage.h"

#include "DetectionService.h"
#include "DetectionSettingsStore.h"

#include <QAbstractItemView>
#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QShowEvent>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace
{
    QString currencySymbol(const QString &currency)
    {
        if (currency == "LSL") return "M";
        if (currency == "ZAR") return "R";
        if (currency == "USD") return "$";
        if (currency == "EUR") return "\u20AC";

        return currency;
    }

    QString amountText(const DetectionResult &result)
    {
        return currencySymbol(result.currency) + " " +
               QString::number(result.amount, 'f', 2);
    }

    QColor levelColor(const QString &level)
    {
        if (level == "high") return QColor("#ffffff");
        if (level == "medium") return QColor("#d0d0d0");
        if (level == "low") return QColor("#a0a0a0");

        return QColor("#808080");
    }
}

DetectionPage::DetectionPage(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(28, 28, 28, 28);
    mainLayout->setSpacing(18);

    auto *title = new QLabel("Anomaly Detection");
    title->setObjectName("heading");

    auto *subtitle = new QLabel(
        "Transactions flagged by the detection engine. "
        "Each finding lists the checks that were triggered."
    );
    subtitle->setObjectName("description");
    subtitle->setWordWrap(true);

    engineStatus = new QLabel();
    engineStatus->setWordWrap(true);
    engineStatus->setObjectName("panel");

    mainLayout->addWidget(title);
    mainLayout->addWidget(subtitle);
    mainLayout->addWidget(engineStatus);

    // Summary cards.
    auto *summaryLayout = new QHBoxLayout();
    summaryLayout->setSpacing(14);

    auto createSummary = [&summaryLayout](
        const QString &caption,
        QLabel **valueLabel)
    {
        auto *card = new QFrame;
        card->setObjectName("summaryCard");

        auto *layout = new QVBoxLayout(card);
        layout->setContentsMargins(18, 14, 18, 14);
        layout->setSpacing(6);

        auto *captionLabel = new QLabel(caption);
        captionLabel->setObjectName("cardTitle");

        *valueLabel = new QLabel("0");
        (*valueLabel)->setObjectName("cardValue");

        layout->addWidget(captionLabel);
        layout->addWidget(*valueLabel);
        summaryLayout->addWidget(card, 1);
    };

    createSummary("Flagged", &flaggedValue);
    createSummary("High risk", &highValue);
    createSummary("Medium risk", &mediumValue);
    createSummary("Low risk", &lowValue);

    mainLayout->addLayout(summaryLayout);

    // Toolbar.
    auto *toolbar = new QHBoxLayout();

    levelFilter = new QComboBox();
    levelFilter->addItems({
        "All levels",
        "High only",
        "Medium only",
        "Low only"
    });

    analyzeButton = new QPushButton("Re-analyze transactions");

    toolbar->addWidget(levelFilter);
    toolbar->addStretch();
    toolbar->addWidget(analyzeButton);

    mainLayout->addLayout(toolbar);

    resultsTable = new QTableWidget();
    resultsTable->setColumnCount(8);
    resultsTable->setHorizontalHeaderLabels({
        "Transaction",
        "Date",
        "Sender",
        "Receiver",
        "Amount",
        "Score",
        "Level",
        "Findings"
    });

    resultsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    resultsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    resultsTable->setAlternatingRowColors(true);
    resultsTable->verticalHeader()->setVisible(false);
    resultsTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents);
    resultsTable->horizontalHeader()->setStretchLastSection(true);

    mainLayout->addWidget(resultsTable, 1);

    emptyMessage = new QLabel();
    emptyMessage->setAlignment(Qt::AlignCenter);
    emptyMessage->setWordWrap(true);
    emptyMessage->setStyleSheet(
        "color: #a0a0a0; font-size: 13px; padding: 12px;");
    emptyMessage->hide();

    mainLayout->addWidget(emptyMessage);

    connect(
        analyzeButton,
        &QPushButton::clicked,
        this,
        [this]() { runAnalysis(); }
    );

    connect(
        levelFilter,
        &QComboBox::currentTextChanged,
        this,
        [this](const QString &) { applyFilter(); }
    );

    updateEngineStatus();
    refreshResults();
}

void DetectionPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    updateEngineStatus();
    refreshResults();
}

void DetectionPage::runAnalysis()
{
    updateEngineStatus();
    DetectionService service;

    int analyzed = 0;
    int flagged = 0;

    if (!service.analyzeAll(&analyzed, &flagged))
    {
        QMessageBox::warning(
            this,
            "Detection Failed",
            service.lastError()
        );

        return;
    }

    refreshResults();

    QMessageBox::information(
        this,
        "Detection Complete",
        QString("Analysed %1 transactions.\n"
                "Flagged for review: %2")
            .arg(analyzed)
            .arg(flagged)
    );
}

void DetectionPage::refreshResults()
{
    updateEngineStatus();
    DetectionService service;

    cachedResults = service.flaggedResults();

    if (!service.lastError().isEmpty())
    {
        cachedResults.clear();
        emptyMessage->setText(service.lastError());
        emptyMessage->show();
        resultsTable->hide();
        updateSummary();
        return;
    }

    resultsTable->setRowCount(0);

    for (const DetectionResult &result : cachedResults)
    {
        const int row = resultsTable->rowCount();
        resultsTable->insertRow(row);

        const QStringList values = {
            result.transactionReference,
            result.timestamp,
            result.senderId,
            result.receiverId,
            amountText(result),
            QString::number(result.riskScore, 'f', 0),
            result.riskLevel,
            result.methodText()
        };

        for (int column = 0; column < values.size(); ++column)
        {
            auto *item = new QTableWidgetItem(values.at(column));

            if (column == 6)
            {
                item->setForeground(
                    QBrush(levelColor(result.riskLevel)));
            }

            if (column == 7)
            {
                item->setToolTip(result.reasonText());
            }

            resultsTable->setItem(row, column, item);
        }
    }

    resultsTable->show();

    if (cachedResults.isEmpty())
    {
        emptyMessage->setText(
            "No transactions have been flagged.\n"
            "Import or add transactions, then run the analysis."
        );
        emptyMessage->show();
    }
    else
    {
        emptyMessage->hide();
    }

    updateSummary();
    applyFilter();
}

void DetectionPage::updateSummary()
{
    int high = 0;
    int medium = 0;
    int low = 0;

    for (const DetectionResult &result : cachedResults)
    {
        if (result.riskLevel == "high")
            ++high;
        else if (result.riskLevel == "medium")
            ++medium;
        else if (result.riskLevel == "low")
            ++low;
    }

    flaggedValue->setText(QString::number(cachedResults.size()));
    highValue->setText(QString::number(high));
    mediumValue->setText(QString::number(medium));
    lowValue->setText(QString::number(low));
}

void DetectionPage::applyFilter()
{
    const QString selection = levelFilter->currentText();

    QString requiredLevel;

    if (selection == "High only")
        requiredLevel = "high";
    else if (selection == "Medium only")
        requiredLevel = "medium";
    else if (selection == "Low only")
        requiredLevel = "low";

    for (int row = 0; row < resultsTable->rowCount(); ++row)
    {
        bool visible = requiredLevel.isEmpty();

        if (!visible)
        {
            QTableWidgetItem *levelItem =
                resultsTable->item(row, 6);

            visible = levelItem &&
                      levelItem->text() == requiredLevel;
        }

        resultsTable->setRowHidden(row, !visible);
    }
}

void DetectionPage::updateEngineStatus()
{
    if (!engineStatus)
        return;

    const DetectionSettings settings = DetectionSettingsStore::load();
    QStringList engines;
    if (settings.useRuleEngine)
        engines << "rule engine";
    if (settings.useMlModel)
        engines << "ML model (HGB, type+amount, PaySim-trained)";

    const QString text = engines.isEmpty()
        ? "Detection engines: none enabled"
        : QString("Detection engines: %1. Configure in Settings.")
              .arg(engines.join(" + "));

    engineStatus->setText(text);
}
