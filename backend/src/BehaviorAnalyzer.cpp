
#include "BehaviorAnalyzer.h"
#include "TransactionTime.h"

#include <QDateTime>
#include <QHash>

#include <algorithm>
#include <cmath>

BehaviorAnalyzer::BehaviorAnalyzer(int minimumHistory)
    : minimumHistory(minimumHistory < 1 ? 1 : minimumHistory)
{
}

void BehaviorAnalyzer::build(const QList<Transaction> &transactions)
{
    totals.clear();
    historyCounts.fill(0, transactions.size());
    historySums.fill(0.0, transactions.size());
    historySumSquares.fill(0.0, transactions.size());
    amounts.resize(transactions.size());

    QVector<QDateTime> times;
    times.reserve(transactions.size());

    QHash<QString, QVector<int>> bySender;

    for (int index = 0; index < transactions.size(); ++index)
    {
        const Transaction &transaction = transactions.at(index);

        times.append(parseTransactionTime(transaction.timestamp));

        bySender[transaction.senderId].append(index);

        amounts[index] = transaction.amount;

        Accumulator &accumulator = totals[transaction.senderId];

        ++accumulator.count;
        accumulator.sum += transaction.amount;
        accumulator.sumOfSquares +=
            transaction.amount * transaction.amount;
    }

    // Within each account, order by time so that "earlier" is well
    // defined even when records are imported out of order.
    for (auto it = bySender.begin(); it != bySender.end(); ++it)
    {
        std::sort(
            it->begin(),
            it->end(),
            [&times](int left, int right)
            {
                if (times.at(left) == times.at(right))
                    return left < right;

                return times.at(left) < times.at(right);
            }
        );

        int count = 0;
        double sum = 0.0;
        double sumOfSquares = 0.0;

        for (int index : *it)
        {
            historyCounts[index] = count;
            historySums[index] = sum;
            historySumSquares[index] = sumOfSquares;

            const double amount = transactions.at(index).amount;

            sum += amount;
            sumOfSquares += amount * amount;
            ++count;
        }
    }
}

bool BehaviorAnalyzer::hasBaseline(const QString &accountId) const
{
    const auto it = totals.constFind(accountId);

    if (it == totals.constEnd())
        return false;

    return it->count > minimumHistory;
}

AccountProfile BehaviorAnalyzer::profile(
    const QString &accountId) const
{
    AccountProfile profile;
    profile.accountId = accountId;

    const auto it = totals.constFind(accountId);

    if (it == totals.constEnd() || it->count == 0)
        return profile;

    profile.transactionCount = it->count;
    profile.meanAmount = it->sum / it->count;

    double variance =
        it->sumOfSquares / it->count -
        profile.meanAmount * profile.meanAmount;

    if (variance < 0.0)
        variance = 0.0;

    profile.standardDeviation = std::sqrt(variance);
    profile.hasBaseline = it->count > minimumHistory;

    return profile;
}

DeviationResult BehaviorAnalyzer::evaluate(
    int index,
    double zScoreThreshold,
    double zeroVarianceRatio) const
{
    DeviationResult result;

    if (index < 0 || index >= historyCounts.size())
        return result;

    const int historyCount = historyCounts.at(index);

    result.historyCount = historyCount;

    if (historyCount < minimumHistory)
        return result;

    const double mean = historySums.at(index) / historyCount;

    double variance =
        historySumSquares.at(index) / historyCount - mean * mean;

    if (variance < 0.0)
        variance = 0.0;

    const double deviation = std::sqrt(variance);

    result.hasBaseline = true;
    result.meanAmount = mean;
    result.standardDeviation = deviation;

    const double amount = amounts.at(index);

    if (deviation > 0.0)
    {
        result.zScore = (amount - mean) / deviation;
        result.unusual =
            result.zScore >= zScoreThreshold && amount > mean;
    }
    else
    {
        // A perfectly uniform history has no standard deviation, so a
        // ratio is used instead of a z-score.
        result.zScore = 0.0;
        result.unusual =
            mean > 0.0 && amount > mean * zeroVarianceRatio;
    }

    return result;
}
