
#ifndef DETECTIONRESULT_H
#define DETECTIONRESULT_H

#include <QString>
#include <QStringList>

// One triggered check before risk scoring.
struct RuleHit
{
    QString method;
    QString reason;
    double weight = 0.0;
};

// The outcome of analysing a single transaction.
struct DetectionResult
{
    // Transaction data, copied so the result can be shown or stored
    // without a second database lookup.
    int transactionId = 0;
    QString transactionReference;
    QString timestamp;
    QString senderId;
    QString receiverId;
    double amount = 0.0;
    QString currency = "LSL";

    // Scoring outcome.
    double riskScore = 0.0;
    QString riskLevel;
    QString primaryMethod;
    QStringList methods;
    QStringList reasons;

    bool flagged() const
    {
        return riskScore > 0.0;
    }

    QString reasonText() const
    {
        return reasons.join("; ");
    }

    QString methodText() const
    {
        return methods.join(", ");
    }
};

#endif // DETECTIONRESULT_H
