
#ifndef BEHAVIORANALYZER_H
#define BEHAVIORANALYZER_H

#include "Transaction.h"

#include <QHash>
#include <QList>
#include <QString>
#include <QVector>

// Aggregate spending profile for one account, used for display.
struct AccountProfile
{
    QString accountId;
    int transactionCount = 0;
    double meanAmount = 0.0;
    double standardDeviation = 0.0;
    bool hasBaseline = false;
};

// How far one transaction deviates from its sender's earlier history.
struct DeviationResult
{
    bool hasBaseline = false;
    bool unusual = false;
    int historyCount = 0;
    double meanAmount = 0.0;
    double standardDeviation = 0.0;
    double zScore = 0.0;
};

// Builds per-account baselines and decides whether an amount is unusual
// for that account.
//
// Only transactions that happened *before* the one under review are used
// as its baseline. Including later transactions would leak future
// information into the assessment and make the result depend on the
// order in which records happen to be imported.
class BehaviorAnalyzer
{
public:
    explicit BehaviorAnalyzer(int minimumHistory = 5);

    // Pre-computes running statistics. Must be called before evaluate().
    void build(const QList<Transaction> &transactions);

    AccountProfile profile(const QString &accountId) const;
    bool hasBaseline(const QString &accountId) const;

    // Compares transactions[index] with that sender's earlier records.
    DeviationResult evaluate(
        int index,
        double zScoreThreshold,
        double zeroVarianceRatio) const;

private:
    struct Accumulator
    {
        int count = 0;
        double sum = 0.0;
        double sumOfSquares = 0.0;
    };

    // Per transaction: statistics of the sender's earlier transactions.
    QVector<int> historyCounts;
    QVector<double> historySums;
    QVector<double> historySumSquares;

    // Amounts, indexed the same way as the input list.
    QVector<double> amounts;

    // Per account totals, used by profile().
    QHash<QString, Accumulator> totals;

    int minimumHistory;
};

#endif // BEHAVIORANALYZER_H
