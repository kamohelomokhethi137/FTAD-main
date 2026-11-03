
// Tests for BehaviorAnalyzer.
//
// The analyser must compare a transaction with the account's *earlier*
// activity only. Using the whole data set would leak future
// transactions into the baseline and make results depend on import
// order, which is the defect these tests were written against.

#include <gtest/gtest.h>

#include <QList>
#include <QString>

#include <cmath>

#include "BehaviorAnalyzer.h"
#include "Transaction.h"
#include "support/TestSupport.h"

namespace
{
    // Six transactions for one account with increasing timestamps.
    QList<Transaction> ascendingHistory()
    {
        QList<Transaction> transactions;

        const double amounts[] = {100.0, 200.0, 300.0, 400.0, 500.0,
                                  1000.0};

        for (int i = 0; i < 6; ++i)
        {
            transactions.append(makeTransaction(
                i + 1,
                QString("TXNXXX%1").arg(i + 1),
                QString("2026-10-%1 09:00:00").arg(10 + i),
                "ACCX101",
                "ACCX900",
                "TRANSFER",
                amounts[i]
            ));
        }

        return transactions;
    }
}

TEST(BehaviorAnalyzer, HasNoBaselineBelowMinimumHistory)
{
    const QList<Transaction> transactions = ascendingHistory();

    BehaviorAnalyzer analyzer(5);
    analyzer.build(transactions);

    // Index 3 has only three earlier transactions.
    const DeviationResult result =
        analyzer.evaluate(3, 3.0, 2.0);

    EXPECT_FALSE(result.hasBaseline);
    EXPECT_FALSE(result.unusual);
    EXPECT_EQ(result.historyCount, 3);
}

TEST(BehaviorAnalyzer, UsesBaselineOnceMinimumHistoryIsReached)
{
    const QList<Transaction> transactions = ascendingHistory();

    BehaviorAnalyzer analyzer(5);
    analyzer.build(transactions);

    const DeviationResult result =
        analyzer.evaluate(5, 3.0, 2.0);

    EXPECT_TRUE(result.hasBaseline);
    EXPECT_EQ(result.historyCount, 5);
}

TEST(BehaviorAnalyzer, BaselineNeverIncludesLaterTransactions)
{
    // Regression test: the original implementation averaged every
    // transaction for the account, including the ones that happen
    // after the transaction under review.
    const QList<Transaction> transactions = ascendingHistory();

    BehaviorAnalyzer analyzer(5);
    analyzer.build(transactions);

    const DeviationResult result =
        analyzer.evaluate(1, 3.0, 2.0);

    EXPECT_EQ(result.historyCount, 1);
    EXPECT_FALSE(result.hasBaseline);
}

TEST(BehaviorAnalyzer, ComputesMeanAndDeviationOfEarlierOnly)
{
    const QList<Transaction> transactions = ascendingHistory();

    BehaviorAnalyzer analyzer(5);
    analyzer.build(transactions);

    const DeviationResult result =
        analyzer.evaluate(5, 3.0, 2.0);

    // Earlier amounts are 100..500, so the mean is 300 and the
    // population deviation is sqrt(20000).
    EXPECT_NEAR(result.meanAmount, 300.0, 1.0e-9);
    EXPECT_NEAR(result.standardDeviation, std::sqrt(20000.0), 1.0e-6);
    EXPECT_NEAR(result.zScore, (1000.0 - 300.0) / std::sqrt(20000.0),
                1.0e-6);
    EXPECT_TRUE(result.unusual);
}

TEST(BehaviorAnalyzer, UniformHistoryFallsBackToRatioRule)
{
    QList<Transaction> transactions;

    for (int i = 0; i < 5; ++i)
    {
        transactions.append(makeTransaction(
            i + 1,
            QString("TXNXXX%1").arg(i + 1),
            QString("2026-10-%1 09:00:00").arg(10 + i),
            "ACCX102",
            "ACCX900",
            "TRANSFER",
            500.0
        ));
    }

    transactions.append(makeTransaction(
        6, "TXNXXX6", "2026-10-20 09:00:00",
        "ACCX102", "ACCX900", "TRANSFER", 2000.0));

    BehaviorAnalyzer analyzer(5);
    analyzer.build(transactions);

    const DeviationResult result =
        analyzer.evaluate(5, 3.0, 2.0);

    EXPECT_TRUE(result.hasBaseline);
    EXPECT_DOUBLE_EQ(result.standardDeviation, 0.0);
    EXPECT_DOUBLE_EQ(result.zScore, 0.0);
    EXPECT_TRUE(result.unusual);
}

TEST(BehaviorAnalyzer, UniformHistoryBelowRatioIsNotUnusual)
{
    QList<Transaction> transactions;

    for (int i = 0; i < 5; ++i)
    {
        transactions.append(makeTransaction(
            i + 1,
            QString("TXNXXX%1").arg(i + 1),
            QString("2026-10-%1 09:00:00").arg(10 + i),
            "ACCX103",
            "ACCX900",
            "TRANSFER",
            500.0
        ));
    }

    // 900 is only 1.8x the uniform average, below the 2.0 ratio.
    transactions.append(makeTransaction(
        6, "TXNXXX6", "2026-10-20 09:00:00",
        "ACCX103", "ACCX900", "TRANSFER", 900.0));

    BehaviorAnalyzer analyzer(5);
    analyzer.build(transactions);

    EXPECT_FALSE(analyzer.evaluate(5, 3.0, 2.0).unusual);
}

TEST(BehaviorAnalyzer, ZScoreThresholdIsRespected)
{
    const QList<Transaction> transactions = ascendingHistory();

    BehaviorAnalyzer analyzer(5);
    analyzer.build(transactions);

    // The final amount is about 4.95 deviations above the mean, so a
    // threshold of 6 must not flag it.
    EXPECT_FALSE(analyzer.evaluate(5, 6.0, 2.0).unusual);
    EXPECT_TRUE(analyzer.evaluate(5, 4.0, 2.0).unusual);
}

TEST(BehaviorAnalyzer, AmountsBelowTheMeanAreNotFlagged)
{
    QList<Transaction> transactions;

    const double amounts[] = {100.0, 200.0, 300.0, 400.0, 500.0, 50.0};

    for (int i = 0; i < 6; ++i)
    {
        transactions.append(makeTransaction(
            i + 1,
            QString("TXNXXX%1").arg(i + 1),
            QString("2026-10-%1 09:00:00").arg(10 + i),
            "ACCX104",
            "ACCX900",
            "TRANSFER",
            amounts[i]
        ));
    }

    BehaviorAnalyzer analyzer(5);
    analyzer.build(transactions);

    // The final amount is far below the mean, which is a large
    // deviation but not the pattern the rule is looking for.
    const DeviationResult result =
        analyzer.evaluate(5, 3.0, 2.0);

    EXPECT_TRUE(result.hasBaseline);
    EXPECT_LT(result.zScore, 0.0);
    EXPECT_FALSE(result.unusual);
}

TEST(BehaviorAnalyzer, OrderingFollowsTimestampsNotListPosition)
{
    QList<Transaction> transactions = ascendingHistory();

    // Move the latest transaction to the front of the list. Its
    // baseline must still be the five transactions that precede it in
    // time.
    transactions.move(5, 0);

    BehaviorAnalyzer analyzer(5);
    analyzer.build(transactions);

    const DeviationResult result =
        analyzer.evaluate(0, 3.0, 2.0);

    EXPECT_TRUE(result.hasBaseline);
    EXPECT_EQ(result.historyCount, 5);
    EXPECT_NEAR(result.meanAmount, 300.0, 1.0e-9);
}

TEST(BehaviorAnalyzer, ProfileReportsAccountTotals)
{
    const QList<Transaction> transactions = ascendingHistory();

    BehaviorAnalyzer analyzer(5);
    analyzer.build(transactions);

    const AccountProfile profile = analyzer.profile("ACCX101");

    EXPECT_EQ(profile.accountId, "ACCX101");
    EXPECT_EQ(profile.transactionCount, 6);
    EXPECT_NEAR(profile.meanAmount, 2500.0 / 6.0, 1.0e-9);
    EXPECT_TRUE(profile.hasBaseline);
    EXPECT_TRUE(analyzer.hasBaseline("ACCX101"));
}

TEST(BehaviorAnalyzer, UnknownAccountHasNoProfile)
{
    BehaviorAnalyzer analyzer(5);
    analyzer.build(ascendingHistory());

    const AccountProfile profile = analyzer.profile("ACCX999");

    EXPECT_EQ(profile.transactionCount, 0);
    EXPECT_FALSE(profile.hasBaseline);
    EXPECT_FALSE(analyzer.hasBaseline("ACCX999"));
}

TEST(BehaviorAnalyzer, HandlesOutOfRangeIndexes)
{
    BehaviorAnalyzer analyzer(5);
    analyzer.build(ascendingHistory());

    EXPECT_FALSE(analyzer.evaluate(-1, 3.0, 2.0).hasBaseline);
    EXPECT_FALSE(analyzer.evaluate(99, 3.0, 2.0).hasBaseline);
}

TEST(BehaviorAnalyzer, EmptyDataSetIsSafe)
{
    BehaviorAnalyzer analyzer(5);
    analyzer.build({});

    const DeviationResult result =
        analyzer.evaluate(0, 3.0, 2.0);

    EXPECT_FALSE(result.hasBaseline);
    EXPECT_FALSE(analyzer.hasBaseline("ACCX101"));
}
