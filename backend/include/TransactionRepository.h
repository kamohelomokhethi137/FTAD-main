
#ifndef TRANSACTIONREPOSITORY_H
#define TRANSACTIONREPOSITORY_H

#include "Transaction.h"

#include <QList>
#include <QString>

class TransactionRepository
{
public:
    bool addTransaction(const Transaction &transaction);
    QList<Transaction> getAllTransactions();
    bool clearAllTransactions();

    QString generateNextTransactionId();
    QString lastError() const;

    static QString formatAccountId(int number);
    static bool isValidAccountId(const QString &accountId);

    // Converts a user or CSV supplied account ID into the canonical
    // "ACC" + 4 character form. Returns an empty string when invalid.
    static QString normalizeAccountId(const QString &accountId);

    static QString formatTransactionId(int number);
    static bool isValidTransactionId(const QString &transactionId);

    // Converts a user or CSV supplied transaction ID into the canonical
    // "TXN" + 4 character form. Returns an empty string when invalid.
    static QString normalizeTransactionId(const QString &transactionId);

private:
    QString errorMessage;
};

#endif // TRANSACTIONREPOSITORY_H
