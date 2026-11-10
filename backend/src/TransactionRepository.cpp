
#include "TransactionRepository.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QVariant>
#include <QSet>
#include <QStringList>

#include <cmath>

namespace
{
    constexpr int MAX_ACCOUNT_NUMBER = 9999;
    constexpr int MAX_TRANSACTION_NUMBER = 9999;
    constexpr int ID_BODY_WIDTH = 4;

    // Builds the canonical identifier body, for example 101 -> "X101".
    QString paddedBody(int number)
    {
        const QString digits = QString::number(number);

        return QString(ID_BODY_WIDTH - digits.size(), 'X') + digits;
    }

    // Parses "PREFIX" followed by exactly four characters where the
    // leading characters may be 'X' padding, for example "ACCX101".
    // Returns false when the body does not match that shape.
    bool parsePaddedId(
        const QString &identifier,
        const QString &prefix,
        int maximumNumber,
        int *number)
    {
        if (!identifier.startsWith(prefix))
            return false;

        const QString body = identifier.mid(prefix.size());

        if (body.size() != ID_BODY_WIDTH)
            return false;

        int padding = 0;

        while (padding < body.size() &&
               body.at(padding) == QChar('X'))
        {
            ++padding;
        }

        const QString digits = body.mid(padding);

        if (digits.isEmpty())
            return false;

        // Leading zeroes are not produced by the formatter, so they
        // are rejected instead of being guessed at.
        if (digits.size() > 1 && digits.front() == QChar('0'))
            return false;

        bool valid = false;
        const int value = digits.toInt(&valid);

        if (!valid || value < 0 || value > maximumNumber)
            return false;

        *number = value;

        return true;
    }

    // Parses an unpadded identifier body, for example "ACC101".
    bool parsePlainId(
        const QString &identifier,
        const QString &prefix,
        int minimumNumber,
        int maximumNumber,
        int *number)
    {
        if (!identifier.startsWith(prefix))
            return false;

        const QString digits = identifier.mid(prefix.size());

        if (digits.isEmpty() || digits.size() > ID_BODY_WIDTH)
            return false;

        bool valid = false;
        const int value = digits.toInt(&valid);

        if (!valid || value < minimumNumber || value > maximumNumber)
            return false;

        // Reject "ACC0101" style input rather than reinterpreting it.
        if (QString::number(value) != digits)
            return false;

        *number = value;

        return true;
    }
}

QString TransactionRepository::formatAccountId(int number)
{
    if (number < 1 || number > MAX_ACCOUNT_NUMBER)
        return QString();

    return "ACC" + paddedBody(number);
}

QString TransactionRepository::formatTransactionId(int number)
{
    if (number < 0 || number > MAX_TRANSACTION_NUMBER)
        return QString();

    return "TXN" + paddedBody(number);
}

QString TransactionRepository::normalizeAccountId(
    const QString &accountId)
{
    const QString identifier = accountId.trimmed().toUpper();

    int number = 0;

    if (parsePaddedId(
            identifier,
            "ACC",
            MAX_ACCOUNT_NUMBER,
            &number) &&
        number >= 1)
    {
        return formatAccountId(number);
    }

    if (parsePlainId(
            identifier,
            "ACC",
            1,
            MAX_ACCOUNT_NUMBER,
            &number))
    {
        return formatAccountId(number);
    }

    return QString();
}

bool TransactionRepository::isValidAccountId(
    const QString &accountId)
{
    return !normalizeAccountId(accountId).isEmpty();
}

QString TransactionRepository::normalizeTransactionId(
    const QString &transactionId)
{
    const QString identifier = transactionId.trimmed().toUpper();

    int number = 0;

    if (parsePaddedId(
            identifier,
            "TXN",
            MAX_TRANSACTION_NUMBER,
            &number))
    {
        return formatTransactionId(number);
    }

    if (parsePlainId(
            identifier,
            "TXN",
            0,
            MAX_TRANSACTION_NUMBER,
            &number))
    {
        return formatTransactionId(number);
    }

    return QString();
}

bool TransactionRepository::isValidTransactionId(
    const QString &transactionId)
{
    return !normalizeTransactionId(transactionId).isEmpty();
}

QString TransactionRepository::generateNextTransactionId()
{
    errorMessage.clear();

    QSqlDatabase db =
        QSqlDatabase::database("FTADConnection");

    if (!db.isOpen())
    {
        errorMessage = "Database is not connected.";
        return QString();
    }

    QSqlQuery query(db);

    if (!query.exec(
        "SELECT transaction_id FROM transactions"))
    {
        errorMessage = query.lastError().text();
        return QString();
    }

    QSet<int> usedNumbers;

    while (query.next())
    {
        const QString id = normalizeTransactionId(
            query.value(0).toString()
        );

        if (id.isEmpty())
            continue;

        QString digits = id.mid(3);
        digits.remove('X');

        bool valid = false;
        const int number = digits.toInt(&valid);

        if (valid && number >= 0 && number <= MAX_TRANSACTION_NUMBER)
            usedNumbers.insert(number);
    }

    for (int number = 0; number <= MAX_TRANSACTION_NUMBER; ++number)
    {
        if (!usedNumbers.contains(number))
            return formatTransactionId(number);
    }

    errorMessage =
        "All transaction IDs from TXNXXX0 to TXN9999 are used.";

    return QString();
}

bool TransactionRepository::addTransaction(
    const Transaction &transaction)
{
    errorMessage.clear();

    const QString senderId =
        normalizeAccountId(transaction.senderId);

    if (senderId.isEmpty())
    {
        errorMessage =
            "Invalid sender account. Enter a number from 1 to 9999.";
        return false;
    }

    const QString receiverId =
        normalizeAccountId(transaction.receiverId);

    if (receiverId.isEmpty())
    {
        errorMessage =
            "Invalid receiver account. Enter a number from 1 to 9999.";
        return false;
    }

    // A caller supplied ID is honoured so imported records keep the
    // identifier that appears in the CSV file. An empty ID means the
    // repository must allocate the next available one.
    QString transactionId = transaction.transactionId.trimmed();

    if (!transactionId.isEmpty())
    {
        const QString normalizedId =
            normalizeTransactionId(transactionId);

        if (normalizedId.isEmpty())
        {
            errorMessage =
                "Invalid transaction ID \"" + transactionId +
                "\". Expected TXN followed by four characters, "
                "for example TXNXXX1.";
            return false;
        }

        transactionId = normalizedId;
    }

    if (transaction.transactionType.trimmed().isEmpty())
    {
        errorMessage = "Transaction type is required.";
        return false;
    }

    if (!std::isfinite(transaction.amount) ||
        transaction.amount <= 0.0)
    {
        errorMessage = "Amount must be greater than zero.";
        return false;
    }

    if (!QDateTime::fromString(
        transaction.timestamp,
        "yyyy-MM-dd HH:mm:ss").isValid())
    {
        errorMessage = "Invalid transaction date and time.";
        return false;
    }

    if (!QStringList{"LSL", "ZAR", "USD", "EUR"}
             .contains(transaction.currency))
    {
        errorMessage = "Unsupported currency.";
        return false;
    }

    if (!QStringList{"completed", "pending", "failed"}
             .contains(transaction.status))
    {
        errorMessage = "Invalid transaction status.";
        return false;
    }

    QSqlDatabase db =
        QSqlDatabase::database("FTADConnection");

    if (!db.isOpen())
    {
        errorMessage = "Database is not connected.";
        return false;
    }

    // Allocate the next available ID, or verify the supplied one,
    // inside the same transaction as the insert.
    if (!db.transaction())
    {
        errorMessage = db.lastError().text();
        return false;
    }

    if (transactionId.isEmpty())
    {
        transactionId = generateNextTransactionId();

        if (transactionId.isEmpty())
        {
            db.rollback();
            return false;
        }
    }
    else
    {
        QSqlQuery duplicate(db);

        duplicate.prepare(
            "SELECT 1 FROM transactions "
            "WHERE transaction_id = ? LIMIT 1"
        );

        duplicate.addBindValue(transactionId);

        if (!duplicate.exec())
        {
            errorMessage = duplicate.lastError().text();
            db.rollback();
            return false;
        }

        if (duplicate.next())
        {
            errorMessage =
                "Duplicate transaction ID: " + transactionId +
                " already exists.";
            db.rollback();
            return false;
        }
    }

    QSqlQuery query(db);

    query.prepare(
        "INSERT INTO transactions "
        "(transaction_id, timestamp, sender_id, receiver_id, "
        "transaction_type, amount, currency, status) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?)"
    );

    query.addBindValue(transactionId);
    query.addBindValue(transaction.timestamp);
    query.addBindValue(senderId);
    query.addBindValue(receiverId);
    query.addBindValue(transaction.transactionType);
    query.addBindValue(transaction.amount);
    query.addBindValue(transaction.currency);
    query.addBindValue(transaction.status);

    if (!query.exec())
    {
        errorMessage = query.lastError().text();
        db.rollback();
        return false;
    }

    if (!db.commit())
    {
        errorMessage = db.lastError().text();
        db.rollback();
        return false;
    }

    return true;
}

QList<Transaction> TransactionRepository::getAllTransactions()
{
    QList<Transaction> transactions;
    errorMessage.clear();

    QSqlDatabase db =
        QSqlDatabase::database("FTADConnection");

    if (!db.isOpen())
    {
        errorMessage = "Database is not connected.";
        return transactions;
    }

    QSqlQuery query(db);

    if (!query.exec(
        "SELECT id, transaction_id, timestamp, sender_id, "
        "receiver_id, transaction_type, amount, currency, status "
        "FROM transactions ORDER BY id DESC"))
    {
        errorMessage = query.lastError().text();
        return transactions;
    }

    while (query.next())
    {
        Transaction transaction;

        transaction.id = query.value(0).toInt();
        transaction.transactionId = query.value(1).toString();
        transaction.timestamp = query.value(2).toString();
        transaction.senderId = query.value(3).toString();
        transaction.receiverId = query.value(4).toString();
        transaction.transactionType = query.value(5).toString();
        transaction.amount = query.value(6).toDouble();
        transaction.currency = query.value(7).toString();
        transaction.status = query.value(8).toString();

        transactions.append(transaction);
    }

    return transactions;
}

QString TransactionRepository::lastError() const
{
    return errorMessage;
}

bool TransactionRepository::clearAllTransactions()
{
    errorMessage.clear();

    QSqlDatabase db = QSqlDatabase::database("FTADConnection");

    if (!db.isOpen())
    {
        errorMessage = "Database is not connected.";
        return false;
    }

    QSqlQuery query(db);

    // Cascades remove anomalies, investigations and events.
    if (!query.exec("DELETE FROM transactions"))
    {
        errorMessage = query.lastError().text();
        return false;
    }

    return true;
}
