
#include "TransactionsPage.h"
#include "TransactionRepository.h"
#include "Transaction.h"
#include "CsvImportService.h"
#include "DetectionService.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QDateTimeEdit>
#include <QMessageBox>
#include <QDateTime>
#include <QTableWidgetItem>
#include <QAbstractItemView>
#include <QFileDialog>
#include <QFrame>
#include <QStringList>

namespace
{
    QString currencySymbol(const QString &currency)
    {
        if (currency == "LSL") return "M";
        if (currency == "ZAR") return "R";
        if (currency == "USD") return "$";
        if (currency == "EUR") return "€";

        return currency;
    }

    QWidget *createIdField(
        const QString &prefix,
        QWidget *numberInput,
        QWidget *parent)
    {
        auto *container = new QWidget(parent);
        auto *layout = new QHBoxLayout(container);

        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(8);

        auto *prefixLabel = new QLabel(prefix);
        prefixLabel->setFixedWidth(65);
        prefixLabel->setAlignment(Qt::AlignCenter);
        prefixLabel->setStyleSheet(
            "background-color: #404040;"
            "color: #ffffff;"
            "border: 1px solid #475569;"
            "border-radius: 6px;"
            "padding: 9px;"
            "font-weight: bold;"
        );

        layout->addWidget(prefixLabel);
        layout->addWidget(numberInput, 1);

        return container;
    }
}

TransactionsPage::TransactionsPage(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(28, 28, 28, 28);
    mainLayout->setSpacing(20);

    auto *title = new QLabel("Transactions");
    title->setObjectName("heading");

    auto *subtitle = new QLabel(
        "View, add, import, and manage financial transactions."
    );
    subtitle->setObjectName("description");

    mainLayout->addWidget(title);
    mainLayout->addWidget(subtitle);

    auto *toolbar = new QHBoxLayout();

    searchInput = new QLineEdit();
    searchInput->setPlaceholderText(
        "Search by transaction ID, sender, receiver, or status..."
    );

    importButton = new QPushButton("Import CSV");
    importButton->setObjectName("secondaryButton");
    clearButton = new QPushButton("Clear All");
    clearButton->setObjectName("dangerButton");
    addButton = new QPushButton("+ Add Transaction");

    toolbar->addWidget(searchInput, 1);
    toolbar->addWidget(importButton);
    toolbar->addWidget(clearButton);
    toolbar->addWidget(addButton);

    mainLayout->addLayout(toolbar);

    transactionsTable = new QTableWidget();

    // Currency is displayed together with the amount.
    transactionsTable->setColumnCount(7);
    transactionsTable->setHorizontalHeaderLabels({
        "Transaction ID",
        "Date",
        "Sender",
        "Receiver",
        "Type",
        "Amount",
        "Status"
    });

    transactionsTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );

    transactionsTable->setSelectionBehavior(
        QAbstractItemView::SelectRows
    );

    transactionsTable->setAlternatingRowColors(true);
    transactionsTable->verticalHeader()->setVisible(false);

    transactionsTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch
    );

    mainLayout->addWidget(transactionsTable);

    connect(
        addButton,
        &QPushButton::clicked,
        this,
        [this]() {
            openAddTransactionDialog();
        }
    );

    connect(
        clearButton,
        &QPushButton::clicked,
        this,
        [this]() {
            clearAllTransactions();
        }
    );

    connect(
        importButton,
        &QPushButton::clicked,
        this,
        [this]() {
            importCsvFile();
        }
    );

    connect(
        searchInput,
        &QLineEdit::textChanged,
        this,
        [this](const QString &text) {
            filterTransactions(text);
        }
    );

    refreshTransactions();
}

void TransactionsPage::refreshTransactions()
{
    TransactionRepository repository;

    const QList<Transaction> transactions =
        repository.getAllTransactions();

    if (!repository.lastError().isEmpty())
    {
        QMessageBox::warning(
            this,
            "Database Error",
            repository.lastError()
        );
        return;
    }

    transactionsTable->setRowCount(0);

    for (const Transaction &transaction : transactions)
    {
        const int row = transactionsTable->rowCount();
        transactionsTable->insertRow(row);

        const QString amountText =
            currencySymbol(transaction.currency) +
            " " +
            QString::number(transaction.amount, 'f', 2);

        const QStringList values = {
            transaction.transactionId,
            transaction.timestamp,
            transaction.senderId,
            transaction.receiverId,
            transaction.transactionType,
            amountText,
            transaction.status
        };

        for (int column = 0; column < values.size(); ++column)
        {
            auto *item = new QTableWidgetItem(values[column]);

            transactionsTable->setItem(
                row,
                column,
                item
            );
        }
    }

    filterTransactions(searchInput->text());
}

void TransactionsPage::filterTransactions(
    const QString &searchText)
{
    for (int row = 0; row < transactionsTable->rowCount(); ++row)
    {
        bool matches = searchText.trimmed().isEmpty();

        for (int column = 0;
             column < transactionsTable->columnCount();
             ++column)
        {
            QTableWidgetItem *item =
                transactionsTable->item(row, column);

            if (item && item->text().contains(
                    searchText,
                    Qt::CaseInsensitive))
            {
                matches = true;
                break;
            }
        }

        transactionsTable->setRowHidden(row, !matches);
    }
}

void TransactionsPage::importCsvFile()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        "Import Financial Transactions",
        QString(),
        "CSV Files (*.csv);;All Files (*)"
    );

    if (filePath.isEmpty())
        return;

    CsvImportService importer;

    const CsvImportResult result =
        importer.importFile(filePath);

    refreshTransactions();

    QString summary =
        QString("Imported: %1\nSkipped duplicates: %2\nFailed: %3")
            .arg(result.imported)
            .arg(result.skipped)
            .arg(result.failed);

    if (result.imported > 0)
    {
        DetectionService detection;

        int analyzed = 0;
        int flagged = 0;

        if (detection.analyzeAll(&analyzed, &flagged))
        {
            summary += QString(
                "\n\nAnalysed: %1\nFlagged for review: %2")
                .arg(analyzed)
                .arg(flagged);
        }
        else
        {
            summary += "\n\nDetection failed: " +
                       detection.lastError();
        }
    }

    if (!result.details.isEmpty())
    {
        summary += "\n\nDetails:\n" + result.details;
    }

    if (result.failed > 0)
    {
        QMessageBox::warning(
            this,
            "CSV Import Results",
            summary
        );
    }
    else
    {
        QMessageBox::information(
            this,
            "CSV Import Results",
            summary
        );
    }
}

void TransactionsPage::openAddTransactionDialog()
{
    QDialog dialog(this);

    dialog.setWindowTitle("Add Transaction");
    dialog.setMinimumWidth(480);

    dialog.setStyleSheet(
        "QDialog {"
        " background-color: #000000;"
        " color: #ffffff;"
        "}"
        "QLabel { color: #ffffff; }"
        "QLineEdit, QSpinBox, QDoubleSpinBox,"
        "QComboBox, QDateTimeEdit {"
        " background-color: #0f0f0f;"
        " color: #ffffff;"
        " border: 1px solid #404040;"
        " border-radius: 6px;"
        " padding: 8px;"
        " min-height: 22px;"
        "}"
        "QPushButton {"
        " background-color: #ffffff;"
        " color: white;"
        " border: none;"
        " border-radius: 6px;"
        " padding: 10px 16px;"
        "}"
        "QPushButton:hover { background-color: #e0e0e0; }"
    );

    auto *form = new QFormLayout(&dialog);

    form->setContentsMargins(24, 24, 24, 24);
    form->setSpacing(14);

    // Transaction ID: generated by the backend.
    TransactionRepository repository;

    const QString nextId =
        repository.generateNextTransactionId();

    if (nextId.isEmpty())
    {
        QMessageBox::warning(
            this,
            "Transaction ID Error",
            repository.lastError()
        );
        return;
    }

    auto *transactionNumber = new QLineEdit(
        nextId.mid(3)
    );

    transactionNumber->setReadOnly(true);
    transactionNumber->setToolTip(
        "Transaction ID is generated automatically."
    );

    auto *transactionIdField = createIdField(
        "TXN",
        transactionNumber,
        &dialog
    );

    // Sender: users enter numbers only.
    auto *senderNumber = new QSpinBox();
    senderNumber->setRange(1, 9999);
    senderNumber->setValue(1);
    senderNumber->setButtonSymbols(
        QAbstractSpinBox::NoButtons
    );

    auto *senderField = createIdField(
        "ACC",
        senderNumber,
        &dialog
    );

    // Receiver: users enter numbers only.
    auto *receiverNumber = new QSpinBox();
    receiverNumber->setRange(1, 9999);
    receiverNumber->setValue(1);
    receiverNumber->setButtonSymbols(
        QAbstractSpinBox::NoButtons
    );

    auto *receiverField = createIdField(
        "ACC",
        receiverNumber,
        &dialog
    );

    auto *senderPreview = new QLabel(
        "Saved as: ACCXXX1"
    );

    auto *receiverPreview = new QLabel(
        "Saved as: ACCXXX1"
    );

    senderPreview->setStyleSheet(
        "color: #a0a0a0; font-size: 11px;"
    );

    receiverPreview->setStyleSheet(
        "color: #a0a0a0; font-size: 11px;"
    );

    connect(
        senderNumber,
        QOverload<int>::of(&QSpinBox::valueChanged),
        &dialog,
        [senderPreview](int value) {
            senderPreview->setText(
                "Saved as: " +
                TransactionRepository::formatAccountId(value)
            );
        }
    );

    connect(
        receiverNumber,
        QOverload<int>::of(&QSpinBox::valueChanged),
        &dialog,
        [receiverPreview](int value) {
            receiverPreview->setText(
                "Saved as: " +
                TransactionRepository::formatAccountId(value)
            );
        }
    );

    auto *timestamp = new QDateTimeEdit(
        QDateTime::currentDateTime()
    );

    timestamp->setCalendarPopup(true);
    timestamp->setDisplayFormat(
        "yyyy-MM-dd HH:mm:ss"
    );

    auto *transactionType = new QComboBox();

    transactionType->addItems({
        "TRANSFER",
        "PAYMENT",
        "CASH_IN",
        "CASH_OUT",
        "DEBIT"
    });

    auto *amount = new QDoubleSpinBox();

    amount->setRange(0.01, 1000000000.00);
    amount->setDecimals(2);
    amount->setValue(0.01);

    auto *currency = new QComboBox();

    currency->addItems({
        "LSL",
        "ZAR",
        "USD",
        "EUR"
    });

    auto updateAmountPrefix = [amount, currency]() {
        amount->setPrefix(
            currencySymbol(currency->currentText()) + " "
        );
    };

    connect(
        currency,
        &QComboBox::currentTextChanged,
        &dialog,
        [updateAmountPrefix](const QString &) {
            updateAmountPrefix();
        }
    );

    updateAmountPrefix();

    auto *status = new QComboBox();

    status->addItems({
        "completed",
        "pending",
        "failed"
    });

    form->addRow("Transaction ID:", transactionIdField);
    form->addRow("Date & Time:", timestamp);

    form->addRow("Sender Account:", senderField);
    form->addRow("", senderPreview);

    form->addRow("Receiver Account:", receiverField);
    form->addRow("", receiverPreview);

    form->addRow("Transaction Type:", transactionType);
    form->addRow("Amount:", amount);
    form->addRow("Currency:", currency);
    form->addRow("Status:", status);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save |
        QDialogButtonBox::Cancel
    );

    form->addRow(buttons);

    connect(
        buttons,
        &QDialogButtonBox::rejected,
        &dialog,
        &QDialog::reject
    );

    connect(
        buttons,
        &QDialogButtonBox::accepted,
        &dialog,
        [&]() {
            Transaction transaction;

            // ID is generated inside TransactionRepository.
            transaction.transactionId = nextId;

            transaction.timestamp =
                timestamp->dateTime().toString(
                    "yyyy-MM-dd HH:mm:ss"
                );

            transaction.senderId =
                TransactionRepository::formatAccountId(
                    senderNumber->value()
                );

            transaction.receiverId =
                TransactionRepository::formatAccountId(
                    receiverNumber->value()
                );

            transaction.transactionType =
                transactionType->currentText();

            transaction.amount = amount->value();

            transaction.currency =
                currency->currentText();

            transaction.status =
                status->currentText();

            TransactionRepository saveRepository;

            if (!saveRepository.addTransaction(transaction))
            {
                QMessageBox::warning(
                    &dialog,
                    "Unable to Save Transaction",
                    saveRepository.lastError()
                );

                return;
            }

            dialog.accept();
        }
    );

    if (dialog.exec() == QDialog::Accepted)
    {
        refreshTransactions();

        DetectionService detection;
        detection.analyzeAll();
    }
}

void TransactionsPage::clearAllTransactions()
{
    TransactionRepository repository;
    const int count = repository.getAllTransactions().size();
    if (count <= 0 && repository.lastError().isEmpty())
    {
        QMessageBox::information(this, "Transactions", "There are no transactions to clear.");
        return;
    }

    const auto answer = QMessageBox::warning(
        this,
        "Clear All Transactions",
        "This will permanently delete all transactions and any related detection/investigation records.\n\n"
        "Continue?",
        QMessageBox::Cancel | QMessageBox::Yes,
        QMessageBox::Cancel
    );

    if (answer != QMessageBox::Yes)
        return;

    if (!repository.clearAllTransactions())
    {
        QMessageBox::critical(this, "Clear Failed", repository.lastError().isEmpty()
            ? "Unable to clear transactions." : repository.lastError());
        return;
    }

    DetectionService detection;
    detection.analyzeAll();
    refreshTransactions();
    QMessageBox::information(this, "Transactions Cleared", "All transactions were removed. You can import or add new transactions and run analysis.");
}
