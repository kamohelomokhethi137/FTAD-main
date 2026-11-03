
#include "AnomalyDetector.h"
#include "RiskScorer.h"
#include "TransactionTime.h"

#include <algorithm>
#include <cmath>

namespace
{
    QString currencySymbol(const QString &currency)
    {
        if (currency == "LSL") return "M";
        if (currency == "ZAR") return "R";
        if (currency == "USD") return "$";
        if (currency == "EUR") return "\u20AC";

        return currency;
    }

    QString formatAmount(double amount, const QString &currency)
    {
        return currencySymbol(currency) + " " +
               QString::number(amount, 'f', 2);
    }

    // Index range of siblings whose timestamp falls inside [from, to].
    // The sibling list is sorted by time, so two binary searches suffice.
    void timeWindow(
        const QVector<int> &siblings,
        const QVector<QDateTime> &times,
        const QDateTime &from,
        const QDateTime &to,
        int *begin,
        int *end)
    {
        // lower_bound compares (element, value); upper_bound compares
        // (value, element).
        auto elementBefore = [&times](int index, const QDateTime &value)
        {
            return times.at(index) < value;
        };

        auto valueBeforeElement =
            [&times](const QDateTime &value, int index)
        {
            return value < times.at(index);
        };

        const auto first = std::lower_bound(
            siblings.cbegin(),
            siblings.cend(),
            from,
            elementBefore
        );

        const auto last = std::upper_bound(
            first,
            siblings.cend(),
            to,
            valueBeforeElement
        );

        *begin = static_cast<int>(first - siblings.cbegin());
        *end = static_cast<int>(last - siblings.cbegin());
    }
}

AnomalyDetector::AnomalyDetector(const DetectionSettings &settings)
    : settings(settings)
{
}

const DetectionSettings &AnomalyDetector::configuration() const
{
    return settings;
}

bool AnomalyDetector::isWithdrawalType(const QString &transactionType)
{
    static const QStringList withdrawalTypes = {
        "CASH_OUT",
        "DEBIT",
        "WITHDRAWAL",
        "ATM_WITHDRAWAL"
    };

    return withdrawalTypes.contains(
        transactionType.trimmed().toUpper()
    );
}

QList<DetectionResult> AnomalyDetector::analyze(
    const QList<Transaction> &transactions) const
{
    QList<DetectionResult> results;
    results.reserve(transactions.size());

    RiskScorer scorer(settings);

    QVector<QDateTime> times;
    times.reserve(transactions.size());

    for (const Transaction &transaction : transactions)
    {
        times.append(parseTransactionTime(transaction.timestamp));
    }

    // Group by sender so each check only inspects that account's own
    // transactions instead of the whole data set.
    SenderIndex bySender;

    for (int index = 0; index < transactions.size(); ++index)
    {
        bySender[transactions.at(index).senderId].append(index);
    }

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
    }

    BehaviorAnalyzer analyzer(settings.minimumHistory);
    analyzer.build(transactions);

    for (int index = 0; index < transactions.size(); ++index)
    {
        QList<RuleHit> hits;

        checkDuplicatePayment(
            index, transactions, times, bySender, hits);

        checkLargeAmount(transactions.at(index), hits);

        checkRapidWithdrawals(
            index, transactions, times, bySender, hits);

        checkBehaviourDeviation(
            index, transactions.at(index), analyzer, hits);

        results.append(
            scorer.score(transactions.at(index), hits));
    }

    return results;
}

void AnomalyDetector::checkDuplicatePayment(
    int index,
    const QList<Transaction> &transactions,
    const QVector<QDateTime> &times,
    const SenderIndex &bySender,
    QList<RuleHit> &hits) const
{
    const Transaction &current = transactions.at(index);

    if (!times.at(index).isValid())
        return;

    const QVector<int> &siblings =
        bySender.value(current.senderId);

    if (siblings.size() < 2)
        return;

    const QDateTime from = times.at(index).addSecs(
        -60 * settings.duplicateWindowMinutes);

    const QDateTime to = times.at(index).addSecs(
        60 * settings.duplicateWindowMinutes);

    int begin = 0;
    int end = 0;

    timeWindow(siblings, times, from, to, &begin, &end);

    for (int position = begin; position < end; ++position)
    {
        const int siblingIndex = siblings.at(position);

        if (siblingIndex == index)
            continue;

        const Transaction &other = transactions.at(siblingIndex);

        if (other.receiverId != current.receiverId)
            continue;

        if (std::fabs(other.amount - current.amount) >
            settings.duplicateAmountTolerance)
        {
            continue;
        }

        RuleHit hit;
        hit.method = "duplicate_payment";
        hit.weight = settings.weightDuplicatePayment;
        hit.reason =
            QString("Possible duplicate payment of %1 to %2 "
                    "within %3 minutes of transaction %4.")
                .arg(formatAmount(current.amount, current.currency))
                .arg(current.receiverId)
                .arg(settings.duplicateWindowMinutes)
                .arg(other.transactionId);

        hits.append(hit);
        return;
    }
}

void AnomalyDetector::checkLargeAmount(
    const Transaction &transaction,
    QList<RuleHit> &hits) const
{
    if (transaction.amount <= settings.largeAmountAbsolute)
        return;

    RuleHit hit;
    hit.method = "large_amount";
    hit.weight = settings.weightLargeAmount;
    hit.reason =
        QString("Amount %1 exceeds the single-transaction "
                "limit of %2.")
            .arg(formatAmount(transaction.amount, transaction.currency))
            .arg(formatAmount(
                settings.largeAmountAbsolute,
                transaction.currency));

    hits.append(hit);
}

void AnomalyDetector::checkRapidWithdrawals(
    int index,
    const QList<Transaction> &transactions,
    const QVector<QDateTime> &times,
    const SenderIndex &bySender,
    QList<RuleHit> &hits) const
{
    const Transaction &current = transactions.at(index);

    if (!isWithdrawalType(current.transactionType))
        return;

    if (!times.at(index).isValid())
        return;

    const QVector<int> &siblings =
        bySender.value(current.senderId);

    const QDateTime from = times.at(index).addSecs(
        -60 * settings.rapidWithdrawalWindowMinutes);

    int begin = 0;
    int end = 0;

    timeWindow(
        siblings, times, from, times.at(index), &begin, &end);

    int withdrawals = 0;

    for (int position = begin; position < end; ++position)
    {
        const int siblingIndex = siblings.at(position);

        if (!times.at(siblingIndex).isValid())
            continue;

        if (!isWithdrawalType(
                transactions.at(siblingIndex).transactionType))
        {
            continue;
        }

        ++withdrawals;
    }

    if (withdrawals < settings.rapidWithdrawalCount)
        return;

    RuleHit hit;
    hit.method = "rapid_withdrawals";
    hit.weight = settings.weightRapidWithdrawals;
    hit.reason =
        QString("%1 withdrawals by this account within "
                "%2 minutes (threshold %3).")
            .arg(withdrawals)
            .arg(settings.rapidWithdrawalWindowMinutes)
            .arg(settings.rapidWithdrawalCount);

    hits.append(hit);
}

void AnomalyDetector::checkBehaviourDeviation(
    int index,
    const Transaction &transaction,
    const BehaviorAnalyzer &analyzer,
    QList<RuleHit> &hits) const
{
    const DeviationResult deviation = analyzer.evaluate(
        index,
        settings.unusualAmountZScore,
        settings.zeroVarianceRatio
    );

    if (!deviation.unusual)
        return;

    RuleHit hit;
    hit.method = "behaviour_deviation";
    hit.weight = settings.weightBehaviourDeviation;

    if (deviation.standardDeviation > 0.0)
    {
        hit.reason =
            QString("Amount %1 is %2 standard deviations above "
                    "this account's average of %3 over %4 earlier "
                    "transactions.")
                .arg(formatAmount(
                    transaction.amount, transaction.currency))
                .arg(deviation.zScore, 0, 'f', 1)
                .arg(formatAmount(
                    deviation.meanAmount, transaction.currency))
                .arg(deviation.historyCount);
    }
    else
    {
        hit.reason =
            QString("Amount %1 is more than %2 times this "
                    "account's uniform average of %3 over %4 "
                    "earlier transactions.")
                .arg(formatAmount(
                    transaction.amount, transaction.currency))
                .arg(settings.zeroVarianceRatio, 0, 'f', 1)
                .arg(formatAmount(
                    deviation.meanAmount, transaction.currency))
                .arg(deviation.historyCount);
    }

    hits.append(hit);
}
