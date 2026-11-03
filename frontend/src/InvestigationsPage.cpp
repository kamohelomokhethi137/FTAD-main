
#include "InvestigationsPage.h"

#include "InvestigationRepository.h"

#include <QAbstractItemView>
#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QDateTime>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QShowEvent>
#include <QSplitter>
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

    QString statusLabel(const QString &status)
    {
        if (status == "pending") return "Pending";
        if (status == "in_progress") return "In progress";
        if (status == "resolved") return "Resolved";
        if (status == "dismissed") return "Dismissed";

        return status;
    }

    QColor statusColor(const QString &status)
    {
        if (status == "pending") return QColor("#FBBF24");
        if (status == "in_progress") return QColor("#38BDF8");
        if (status == "resolved") return QColor("#34D399");
        if (status == "dismissed") return QColor("#94A3B8");

        return QColor("#E2E8F0");
    }

    QColor riskColor(const QString &level)
    {
        if (level == "high") return QColor("#F87171");
        if (level == "medium") return QColor("#FBBF24");
        if (level == "low") return QColor("#38BDF8");

        return QColor("#94A3B8");
    }

    QString eventLabel(const QString &eventType)
    {
        if (eventType == "opened") return "Opened";
        if (eventType == "status_changed") return "Status changed";
        if (eventType == "note_added") return "Note added";

        return "Updated";
    }

    // Compact form of a database timestamp, e.g. "01 Nov 09:02".
    // The full value is still available in the detail panel.
    QString shortTimestamp(const QString &timestamp)
    {
        const QDateTime parsed =
            QDateTime::fromString(timestamp, "yyyy-MM-dd HH:mm:ss");

        if (!parsed.isValid())
            return timestamp;

        return parsed.toString("dd MMM HH:mm");
    }
}

InvestigationsPage::InvestigationsPage(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
    refreshCases();
}

void InvestigationsPage::buildUi()
{
    setStyleSheet(
        "QWidget { background-color: #0B1220; color: #E2E8F0; }"
        "QComboBox, QLineEdit {"
        " background-color: #1E293B;"
        " color: #F8FAFC;"
        " border: 1px solid #334155;"
        " border-radius: 6px;"
        " padding: 8px;"
        "}"
        "QPlainTextEdit {"
        " background-color: #1E293B;"
        " color: #F8FAFC;"
        " border: 1px solid #334155;"
        " border-radius: 6px;"
        " padding: 6px;"
        "}"
        "QPushButton {"
        " background-color: #2563EB;"
        " color: white;"
        " border: none;"
        " border-radius: 6px;"
        " padding: 10px 16px;"
        " font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #1D4ED8; }"
        "QPushButton:disabled { background-color: #334155; }"
        "QTableWidget {"
        " background-color: #1E293B;"
        " alternate-background-color: #172033;"
        " color: #E2E8F0;"
        " gridline-color: #334155;"
        " border: 1px solid #334155;"
        " border-radius: 8px;"
        "}"
        "QHeaderView::section {"
        " background-color: #334155;"
        " color: #F8FAFC;"
        " padding: 10px;"
        " border: none;"
        "}"
        "#detailPanel {"
        " background-color: #172338;"
        " border: 1px solid #2A3A52;"
        " border-radius: 10px;"
        "}"
        "#detailCaption { color: #94A3B8; font-size: 12px; }"
    );

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(28, 28, 28, 28);
    mainLayout->setSpacing(16);

    auto *title = new QLabel("Investigations");
    title->setStyleSheet("font-size: 26px; font-weight: bold;");

    auto *subtitle = new QLabel(
        "Review flagged transactions, record a decision and keep "
        "a history of every change."
    );
    subtitle->setStyleSheet("color: #94A3B8;");
    subtitle->setWordWrap(true);

    mainLayout->addWidget(title);
    mainLayout->addWidget(subtitle);

    // Summary cards.
    auto *summaryLayout = new QHBoxLayout();
    summaryLayout->setSpacing(14);

    auto createSummary = [&summaryLayout](
        const QString &caption,
        QLabel **valueLabel)
    {
        auto *card = new QFrame;
        card->setObjectName("summaryCard");
        card->setStyleSheet(
            "#summaryCard {"
            " background-color: #172338;"
            " border: 1px solid #2A3A52;"
            " border-radius: 10px;"
            "}"
        );

        auto *layout = new QVBoxLayout(card);
        layout->setContentsMargins(18, 12, 18, 12);
        layout->setSpacing(4);

        auto *captionLabel = new QLabel(caption);
        captionLabel->setStyleSheet(
            "color: #94A3B8; font-size: 12px;");

        *valueLabel = new QLabel("0");
        (*valueLabel)->setStyleSheet(
            "color: #F8FAFC; font-size: 24px; font-weight: bold;");

        layout->addWidget(captionLabel);
        layout->addWidget(*valueLabel);
        summaryLayout->addWidget(card, 1);
    };

    createSummary("Total cases", &totalValue);
    createSummary("Open", &openValue);
    createSummary("Resolved", &resolvedValue);
    createSummary("Dismissed", &dismissedValue);

    mainLayout->addLayout(summaryLayout);

    // Toolbar.
    auto *toolbar = new QHBoxLayout();

    statusFilter = new QComboBox();
    statusFilter->addItems({
        "All cases",
        "Pending",
        "In progress",
        "Resolved",
        "Dismissed"
    });

    refreshButton = new QPushButton("Refresh cases");

    toolbar->addWidget(statusFilter);
    toolbar->addStretch();
    toolbar->addWidget(refreshButton);

    mainLayout->addLayout(toolbar);

    // Case list and detail panel.
    auto *splitter = new QSplitter(Qt::Horizontal);

    casesTable = new QTableWidget();
    casesTable->setObjectName("casesTable");
    casesTable->setColumnCount(6);
    casesTable->setHorizontalHeaderLabels({
        "Transaction",
        "Date",
        "Amount",
        "Risk",
        "Status",
        "Assignee"
    });

    casesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    casesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    casesTable->setSelectionMode(
        QAbstractItemView::SingleSelection);
    casesTable->setAlternatingRowColors(true);
    casesTable->verticalHeader()->setVisible(false);

    // Fixed widths keep every column visible without a horizontal
    // scroll bar; the assignee column takes the remaining space.
    QHeaderView *caseHeader = casesTable->horizontalHeader();
    caseHeader->setSectionResizeMode(QHeaderView::Interactive);
    caseHeader->setStretchLastSection(true);
    caseHeader->resizeSection(0, 94);
    caseHeader->resizeSection(1, 96);
    caseHeader->resizeSection(2, 92);
    caseHeader->resizeSection(3, 56);
    caseHeader->resizeSection(4, 90);

    splitter->addWidget(casesTable);

    auto *detailPanel = new QFrame;
    detailPanel->setObjectName("detailPanel");

    auto *panelLayout = new QVBoxLayout(detailPanel);
    panelLayout->setContentsMargins(0, 0, 0, 0);

    // The detail form is scrollable so that it never overlaps itself
    // when the window is short.
    auto *detailScroll = new QScrollArea();
    detailScroll->setWidgetResizable(true);
    detailScroll->setFrameShape(QFrame::NoFrame);
    detailScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    detailScroll->setStyleSheet(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollArea > QWidget > QWidget { background: transparent; }"
    );

    auto *detailContent = new QWidget();
    detailContent->setObjectName("detailContent");

    auto *detailLayout = new QVBoxLayout(detailContent);
    detailLayout->setContentsMargins(20, 16, 20, 14);
    detailLayout->setSpacing(8);

    detailTitle = new QLabel("Case details");
    detailTitle->setStyleSheet(
        "font-size: 16px; font-weight: bold;");
    detailLayout->addWidget(detailTitle);

    auto makeDetailLabel = [&detailLayout](QLabel **target)
    {
        *target = new QLabel("-");
        (*target)->setWordWrap(true);
        (*target)->setStyleSheet("color: #CBD5E1; font-size: 12px;");
        detailLayout->addWidget(*target);
    };

    makeDetailLabel(&detailTransaction);
    makeDetailLabel(&detailAccounts);
    makeDetailLabel(&detailRisk);

    auto *form = new QFormLayout();
    form->setContentsMargins(0, 8, 0, 0);
    form->setSpacing(8);

    statusInput = new QComboBox();
    statusInput->addItems({
        "Pending",
        "In progress",
        "Resolved",
        "Dismissed"
    });

    assigneeInput = new QLineEdit();
    assigneeInput->setPlaceholderText("Reviewer name");

    notesInput = new QPlainTextEdit();
    notesInput->setPlaceholderText(
        "Reviewer notes for this case...");
    notesInput->setFixedHeight(66);

    form->addRow("Status:", statusInput);
    form->addRow("Assigned to:", assigneeInput);
    form->addRow("Notes:", notesInput);

    detailLayout->addLayout(form);

    saveButton = new QPushButton("Save decision");
    detailLayout->addWidget(saveButton);
    detailLayout->addStretch();

    detailScroll->setWidget(detailContent);
    panelLayout->addWidget(detailScroll);

    splitter->addWidget(detailPanel);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    splitter->setSizes({540, 450});

    mainLayout->addWidget(splitter, 1);

    // Review history spans the full width below the case list so that
    // notes have room to wrap instead of being clipped.
    auto *historyCaption = new QLabel("Review history");
    historyCaption->setStyleSheet(
        "color: #F8FAFC; font-size: 13px; font-weight: bold;");

    mainLayout->addWidget(historyCaption);

    historyTable = new QTableWidget();
    historyTable->setObjectName("historyTable");
    historyTable->setColumnCount(4);
    historyTable->setHorizontalHeaderLabels({
        "When",
        "Event",
        "Status",
        "Note"
    });

    historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    historyTable->setSelectionMode(
        QAbstractItemView::NoSelection);
    historyTable->setAlternatingRowColors(true);
    historyTable->verticalHeader()->setVisible(false);

    QHeaderView *historyHeader = historyTable->horizontalHeader();
    historyHeader->setSectionResizeMode(QHeaderView::Interactive);
    historyHeader->setStretchLastSection(true);
    historyHeader->resizeSection(0, 120);
    historyHeader->resizeSection(1, 110);
    historyHeader->resizeSection(2, 170);

    historyTable->setFixedHeight(132);

    mainLayout->addWidget(historyTable);

    emptyMessage = new QLabel();
    emptyMessage->setAlignment(Qt::AlignCenter);
    emptyMessage->setWordWrap(true);
    emptyMessage->setStyleSheet(
        "color: #94A3B8; font-size: 13px; padding: 12px;");
    emptyMessage->hide();

    mainLayout->addWidget(emptyMessage);

    connect(
        refreshButton,
        &QPushButton::clicked,
        this,
        [this]() { refreshCases(); }
    );

    connect(
        statusFilter,
        &QComboBox::currentTextChanged,
        this,
        [this](const QString &) { applyFilter(); }
    );

    connect(
        casesTable,
        &QTableWidget::itemSelectionChanged,
        this,
        [this]()
        {
            const QModelIndexList selected =
                casesTable->selectionModel()->selectedRows();

            if (!selected.isEmpty())
                showSelectedCase(selected.first().row());
        }
    );

    connect(
        saveButton,
        &QPushButton::clicked,
        this,
        [this]() { saveSelectedCase(); }
    );

    clearDetail();
}

void InvestigationsPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    refreshCases();
}

void InvestigationsPage::refreshCases()
{
    InvestigationRepository repository;

    const int created = repository.ensureCases();

    if (created < 0)
    {
        cachedCases.clear();
        populateTable();
        updateSummary();

        emptyMessage->setText(repository.lastError());
        emptyMessage->show();

        return;
    }

    cachedCases = repository.getCases();

    if (!repository.lastError().isEmpty())
    {
        cachedCases.clear();
        populateTable();
        updateSummary();

        emptyMessage->setText(repository.lastError());
        emptyMessage->show();

        return;
    }

    populateTable();
    updateSummary();
    applyFilter();
}

void InvestigationsPage::populateTable()
{
    const int keepId = selectedInvestigationId;

    casesTable->blockSignals(true);
    casesTable->setRowCount(0);

    for (const InvestigationCase &item : cachedCases)
    {
        const int row = casesTable->rowCount();
        casesTable->insertRow(row);

        const QStringList values = {
            item.transactionReference,
            shortTimestamp(item.timestamp),
            currencySymbol(item.currency) + " " +
                QString::number(item.amount, 'f', 2),
            QString::number(item.riskScore, 'f', 0),
            statusLabel(item.caseStatus),
            item.assignedTo
        };

        for (int column = 0; column < values.size(); ++column)
        {
            auto *cell = new QTableWidgetItem(values.at(column));

            if (column == 0)
                cell->setData(Qt::UserRole, item.investigationId);

            if (column == 3)
            {
                cell->setForeground(QBrush(riskColor(item.riskLevel)));
                cell->setToolTip(
                    "Risk score " +
                    QString::number(item.riskScore, 'f', 0) +
                    " (" + item.riskLevel + ")");
            }

            if (column == 4)
                cell->setForeground(QBrush(statusColor(item.caseStatus)));

            casesTable->setItem(row, column, cell);
        }
    }

    casesTable->blockSignals(false);

    if (cachedCases.isEmpty())
    {
        clearDetail();

        emptyMessage->setText(
            "No investigation cases yet.\n"
            "Run the anomaly detection to create cases from "
            "flagged transactions."
        );
        emptyMessage->show();
        casesTable->hide();

        return;
    }

    emptyMessage->hide();
    casesTable->show();

    int targetRow = -1;

    for (int row = 0; row < cachedCases.size(); ++row)
    {
        if (cachedCases.at(row).investigationId == keepId)
        {
            targetRow = row;
            break;
        }
    }

    if (targetRow < 0)
        targetRow = 0;

    casesTable->selectRow(targetRow);
    showSelectedCase(targetRow);
}

void InvestigationsPage::showSelectedCase(int row)
{
    if (row < 0 || row >= cachedCases.size())
    {
        clearDetail();
        return;
    }

    const InvestigationCase &item = cachedCases.at(row);

    selectedInvestigationId = item.investigationId;

    detailTitle->setText(
        "Case " + item.transactionReference);

    detailTransaction->setText(
        item.transactionReference + "  \u00B7  " + item.timestamp);

    detailAccounts->setText(
        item.senderId + "  \u2192  " + item.receiverId +
        "  \u00B7  " + currencySymbol(item.currency) + " " +
        QString::number(item.amount, 'f', 2));

    detailRisk->setText(
        "Risk " + QString::number(item.riskScore, 'f', 0) +
        " (" + item.riskLevel + ")  \u00B7  " +
        item.methods.join(", "));

    const QStringList statuses = {
        "Pending",
        "In progress",
        "Resolved",
        "Dismissed"
    };

    // indexOf() returns qsizetype in Qt 6; the value is narrowed only
    // after the not-found case has been handled.
    const qsizetype statusIndex = statuses.indexOf(
        statusLabel(item.caseStatus));

    statusInput->setCurrentIndex(
        statusIndex < 0 ? 0 : static_cast<int>(statusIndex));
    assigneeInput->setText(item.assignedTo);
    notesInput->setPlainText(item.notes);

    statusInput->setEnabled(true);
    assigneeInput->setEnabled(true);
    notesInput->setEnabled(true);
    saveButton->setEnabled(true);

    loadHistory(item.investigationId);
}

void InvestigationsPage::loadHistory(int investigationId)
{
    InvestigationRepository repository;

    const QList<InvestigationEvent> events =
        repository.getHistory(investigationId);

    historyTable->setRowCount(0);

    for (const InvestigationEvent &event : events)
    {
        const int row = historyTable->rowCount();
        historyTable->insertRow(row);

        const QString transition =
            event.previousStatus.isEmpty()
                ? statusLabel(event.newStatus)
                : statusLabel(event.previousStatus) + " \u2192 " +
                  statusLabel(event.newStatus);

        const QStringList values = {
            shortTimestamp(event.createdAt),
            eventLabel(event.eventType),
            transition,
            event.notes
        };

        for (int column = 0; column < values.size(); ++column)
        {
            auto *cell = new QTableWidgetItem(values.at(column));
            cell->setToolTip(values.at(column));

            historyTable->setItem(row, column, cell);
        }
    }

    // Notes wrap, so each row is sized to its content.
    historyTable->resizeRowsToContents();
}

void InvestigationsPage::saveSelectedCase()
{
    if (selectedInvestigationId == 0)
        return;

    const QStringList statuses = {
        "pending",
        "in_progress",
        "resolved",
        "dismissed"
    };

    const QString caseStatus =
        statuses.value(statusInput->currentIndex());

    InvestigationRepository repository;

    if (!repository.updateCase(
            selectedInvestigationId,
            caseStatus,
            assigneeInput->text().trimmed(),
            notesInput->toPlainText().trimmed()))
    {
        QMessageBox::warning(
            this,
            "Unable to Save Case",
            repository.lastError()
        );

        return;
    }

    refreshCases();
}

void InvestigationsPage::clearDetail()
{
    selectedInvestigationId = 0;

    detailTitle->setText("Case details");
    detailTransaction->setText("No case selected.");
    detailAccounts->clear();
    detailRisk->clear();

    statusInput->setCurrentIndex(0);
    assigneeInput->clear();
    notesInput->clear();

    statusInput->setEnabled(false);
    assigneeInput->setEnabled(false);
    notesInput->setEnabled(false);
    saveButton->setEnabled(false);

    historyTable->setRowCount(0);
}

void InvestigationsPage::updateSummary()
{
    int open = 0;
    int resolved = 0;
    int dismissed = 0;

    for (const InvestigationCase &item : cachedCases)
    {
        if (item.isOpen())
            ++open;
        else if (item.caseStatus == "resolved")
            ++resolved;
        else if (item.caseStatus == "dismissed")
            ++dismissed;
    }

    totalValue->setText(QString::number(cachedCases.size()));
    openValue->setText(QString::number(open));
    resolvedValue->setText(QString::number(resolved));
    dismissedValue->setText(QString::number(dismissed));
}

void InvestigationsPage::applyFilter()
{
    const QString selection = statusFilter->currentText();

    QString requiredStatus;

    if (selection == "Pending")
        requiredStatus = "Pending";
    else if (selection == "In progress")
        requiredStatus = "In progress";
    else if (selection == "Resolved")
        requiredStatus = "Resolved";
    else if (selection == "Dismissed")
        requiredStatus = "Dismissed";

    for (int row = 0; row < casesTable->rowCount(); ++row)
    {
        bool visible = requiredStatus.isEmpty();

        if (!visible)
        {
            QTableWidgetItem *statusItem =
                casesTable->item(row, 4);

            visible = statusItem &&
                      statusItem->text() == requiredStatus;
        }

        casesTable->setRowHidden(row, !visible);
    }
}
