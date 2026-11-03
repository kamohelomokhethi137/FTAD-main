
#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <QString>

struct Transaction
{
    int id = 0;

    QString transactionId;
    QString timestamp;
    QString senderId;
    QString receiverId;
    QString transactionType;

    double amount = 0.0;

    QString currency = "LSL";
    QString status = "completed";
};

#endif // TRANSACTION_H
