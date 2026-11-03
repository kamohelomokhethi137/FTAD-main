
#ifndef ANOMALYDETECTOR_H
#define ANOMALYDETECTOR_H

#include "BehaviorAnalyzer.h"
#include "DetectionResult.h"
#include "DetectionSettings.h"
#include "Transaction.h"

#include <QDateTime>
#include <QHash>
#include <QList>
#include <QString>
#include <QVector>

// Runs every configured check over a set of transactions.
// The detector is pure: it reads transactions and returns results, so it
// can be unit tested without a database or a user interface.
class AnomalyDetector
{
public:
    explicit AnomalyDetector(
        const DetectionSettings &settings = DetectionSettings());

    // One result per transaction, flagged or not.
    QList<DetectionResult> analyze(
        const QList<Transaction> &transactions) const;

    const DetectionSettings &configuration() const;

    static bool isWithdrawalType(const QString &transactionType);

private:
    using SenderIndex = QHash<QString, QVector<int>>;

    void checkDuplicatePayment(
        int index,
        const QList<Transaction> &transactions,
        const QVector<QDateTime> &times,
        const SenderIndex &bySender,
        QList<RuleHit> &hits) const;

    void checkLargeAmount(
        const Transaction &transaction,
        QList<RuleHit> &hits) const;

    void checkRapidWithdrawals(
        int index,
        const QList<Transaction> &transactions,
        const QVector<QDateTime> &times,
        const SenderIndex &bySender,
        QList<RuleHit> &hits) const;

    void checkBehaviourDeviation(
        int index,
        const Transaction &transaction,
        const BehaviorAnalyzer &analyzer,
        QList<RuleHit> &hits) const;

    DetectionSettings settings;
};

#endif // ANOMALYDETECTOR_H
