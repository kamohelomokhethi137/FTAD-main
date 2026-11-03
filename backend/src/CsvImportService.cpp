
#include "CsvImportService.h"
#include "TransactionRepository.h"
#include "Transaction.h"

#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStringList>
#include <QtGlobal>

#include <cmath>

#include <algorithm>

namespace
{
    // Parses quoted CSV fields, including escaped double quotes.
    // Each transaction must occupy one physical line.
    bool parseCsvLine(const QString &line, QStringList &fields)
    {
        fields.clear();

        QString field;
        bool insideQuotes = false;
        bool closedQuote = false;

        for (int i = 0; i < line.size(); ++i)
        {
            const QChar ch = line.at(i);

            if (insideQuotes)
            {
                if (ch == '"')
                {
                    if (i + 1 < line.size() &&
                        line.at(i + 1) == '"')
                    {
                        field += '"';
                        ++i;
                    }
                    else
                    {
                        insideQuotes = false;
                        closedQuote = true;
                    }
                }
                else
                {
                    field += ch;
                }
            }
            else if (ch == ',')
            {
                fields.append(field.trimmed());
                field.clear();
                closedQuote = false;
            }
            else if (ch == '"' && field.trimmed().isEmpty() &&
                     !closedQuote)
            {
                insideQuotes = true;
            }
            else if (closedQuote)
            {
                if (!ch.isSpace())
                    return false;
            }
            else if (ch == '"')
            {
                return false;
            }
            else
            {
                field += ch;
            }
        }

        if (insideQuotes)
            return false;

        fields.append(field.trimmed());
        return true;
    }

    QString normalizedHeader(const QString &value)
    {
        QString header = value.trimmed().toLower();

        if (!header.isEmpty() && header.front() == QChar(0xFEFF))
            header.remove(0, 1);

        return header;
    }
}

CsvImportResult CsvImportService::importFile(const QString &filePath)
{
    CsvImportResult result;

    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        result.failed++;
        result.details = "Unable to open CSV file: " +
                         file.errorString();
        return result;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    if (stream.atEnd())
    {
        result.failed++;
        result.details = "The CSV file is empty.";
        return result;
    }

    QStringList headers;

    if (!parseCsvLine(stream.readLine(), headers))
    {
        result.failed++;
        result.details = "Invalid CSV header.";
        return result;
    }

    const QStringList requiredHeaders = {
        "transaction_id",
        "timestamp",
        "sender_id",
        "receiver_id",
        "transaction_type",
        "amount",
        "currency",
        "status"
    };

    QHash<QString, int> columnIndexes;

    for (int i = 0; i < headers.size(); ++i)
    {
        const QString header = normalizedHeader(headers.at(i));

        if (columnIndexes.contains(header))
        {
            result.failed++;
            result.details = "Duplicate CSV column: " + header;
            return result;
        }

        columnIndexes.insert(header, i);
    }

    auto missingColumn = std::find_if(
        requiredHeaders.begin(),
        requiredHeaders.end(),
        [&columnIndexes](const QString &required)
        {
            return !columnIndexes.contains(required);
        });

    if (missingColumn != requiredHeaders.end())
    {
        result.failed++;
        result.details =
            "Missing CSV column: " + *missingColumn;
        return result;
    }

    QSqlDatabase db = QSqlDatabase::database("FTADConnection");

    if (!db.isOpen())
    {
        result.failed++;
        result.details = "SQLite database is not connected.";
        return result;
    }

    QSet<QString> existingIds;

    QSqlQuery existingQuery(db);

    if (!existingQuery.exec("SELECT transaction_id FROM transactions"))
    {
        result.failed++;
        result.details = existingQuery.lastError().text();
        return result;
    }

    while (existingQuery.next())
    {
        const QString storedId =
            existingQuery.value(0).toString();

        const QString normalizedId =
            TransactionRepository::normalizeTransactionId(storedId);

        existingIds.insert(
            normalizedId.isEmpty() ? storedId : normalizedId
        );
    }

    TransactionRepository repository;
    QStringList errors;

    int lineNumber = 1;

    while (!stream.atEnd())
    {
        const QString line = stream.readLine();
        ++lineNumber;

        if (line.trimmed().isEmpty())
            continue;

        QStringList fields;

        if (!parseCsvLine(line, fields) ||
            fields.size() != headers.size())
        {
            result.failed++;
            errors.append(
                QString("Line %1: Invalid CSV format.")
                    .arg(lineNumber)
            );
            continue;
        }

        auto value = [&](const QString &column) -> QString
        {
            return fields.at(columnIndexes.value(column)).trimmed();
        };

        Transaction transaction;

        const QString rawTransactionId = value("transaction_id");

        transaction.transactionId =
            TransactionRepository::normalizeTransactionId(
                rawTransactionId
            );

        if (transaction.transactionId.isEmpty())
        {
            result.failed++;
            errors.append(
                QString("Line %1: Invalid transaction ID \"%2\". "
                        "Expected TXN followed by four characters, "
                        "for example TXNXXX1.")
                    .arg(lineNumber)
                    .arg(rawTransactionId)
            );
            continue;
        }

        transaction.timestamp = value("timestamp");
        transaction.senderId = value("sender_id");
        transaction.receiverId = value("receiver_id");
        transaction.transactionType = value("transaction_type");
        transaction.currency = value("currency").toUpper();
        transaction.status = value("status").toLower();

        bool amountValid = false;

        transaction.amount =
            value("amount").toDouble(&amountValid);

        if (transaction.senderId.isEmpty() ||
            transaction.receiverId.isEmpty() ||
            transaction.transactionType.isEmpty() ||
            !amountValid ||
            !std::isfinite(transaction.amount) ||
            transaction.amount <= 0.0)
        {
            result.failed++;
            errors.append(
                QString("Line %1: Invalid transaction fields.")
                    .arg(lineNumber)
            );
            continue;
        }

        const QDateTime date = QDateTime::fromString(
            transaction.timestamp,
            "yyyy-MM-dd HH:mm:ss"
        );

        if (!date.isValid())
        {
            result.failed++;
            errors.append(
                QString("Line %1: Invalid timestamp.")
                    .arg(lineNumber)
            );
            continue;
        }

        if (!QStringList{"LSL", "ZAR", "USD", "EUR"}
                 .contains(transaction.currency) ||
            !QStringList{"completed", "pending", "failed"}
                 .contains(transaction.status))
        {
            result.failed++;
            errors.append(
                QString("Line %1: Invalid currency or status.")
                    .arg(lineNumber)
            );
            continue;
        }

        if (existingIds.contains(transaction.transactionId))
        {
            result.skipped++;
            continue;
        }

        if (!repository.addTransaction(transaction))
        {
            result.failed++;
            errors.append(
                QString("Line %1: %2")
                    .arg(lineNumber)
                    .arg(repository.lastError())
            );
            continue;
        }

        existingIds.insert(transaction.transactionId);
        result.imported++;
    }

    result.details = errors.join("\n");

    return result;
}
