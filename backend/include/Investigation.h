
#ifndef INVESTIGATION_H
#define INVESTIGATION_H

#include <QString>
#include <QStringList>

// One review case: an anomaly plus the reviewer's decision on it.
struct InvestigationCase
{
    int investigationId = 0;
    int anomalyId = 0;
    int transactionId = 0;

    QString transactionReference;
    QString timestamp;
    QString senderId;
    QString receiverId;
    double amount = 0.0;
    QString currency = "LSL";

    double riskScore = 0.0;
    QString riskLevel;
    QStringList methods;

    QString caseStatus = "pending";
    QString assignedTo;
    QString notes;
    QString openedAt;
    QString closedAt;

    bool isOpen() const
    {
        return caseStatus == "pending" ||
               caseStatus == "in_progress";
    }
};

// One entry in a case's review history.
struct InvestigationEvent
{
    int id = 0;
    QString eventType;
    QString previousStatus;
    QString newStatus;
    QString notes;
    QString createdAt;
};

#endif // INVESTIGATION_H
