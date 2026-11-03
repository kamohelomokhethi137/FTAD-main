
// Tests for AnomalyDetector.
//
// These cover the rules named in the project proposal: duplicate
// payments, unusually large amounts, rapid withdrawals and deviation
// from the account's own history, plus the negative cases that must
// *not* be flagged.

#include <gtest/gtest.h>

#include <QList>
#include <QString>

#include "AnomalyDetector.h"
#include "DetectionResult.h"
#include "DetectionSettings.h"
#include "Transaction.h"
#include "support/TestSupport.h"

namespace
{
    const DetectionResult *findResult(
        const QList<DetectionResult> &results,
        const QString &reference)
    {
        for (const DetectionResult &result : results)
        {
            if (result.transactionReference == reference)
                return &result;
        }

        return nullptr;
    }

    bool hasMethod(
        const QList<DetectionResult> &results,
        const QString &reference,
        const QString &method)
    {
        const DetectionResult *result =
            findResult(results, reference);

        return result && result->methods.contains(method);
    }

    int flaggedCount(const QList<DetectionResult> &results)
    {
        int flagged = 0;

        for (const DetectionResult &result : results)
        {
            if (result.flagged())
                ++flagged;
        }

        return flagged;
    }
}

TEST(AnomalyDetector, OrdinaryActivityIsNotFlagged)
{
    QList<Transaction> transactions;

    const double amounts[] = {420.0, 510.0, 480.0, 600.0,
                              450.0, 530.0, 470.0, 500.0};

    for (int i = 0; i < 8; ++i)
    {
        transactions.append(makeTransaction(
            i + 1,
            QString("TXNXXX%1").arg(i + 1),
            QString("2026-10-%1 09:00:00").arg(10 + i),
            "ACCX900",
            QString("ACCX%1").arg(300 + i),
            "TRANSFER",
            amounts[i]
        ));
    }

    const AnomalyDetector detector;
    const QList<DetectionResult> results =
        detector.analyze(transactions);

    ASSERT_EQ(results.size(), 8);
    EXPECT_EQ(flaggedCount(results), 0);
}

TEST(AnomalyDetector, EmptyInputProducesNoResults)
{
    const AnomalyDetector detector;

    EXPECT_TRUE(detector.analyze({}).isEmpty());
}

TEST(AnomalyDetector, DuplicatePaymentFlagsBothSides)
{
    const QList<Transaction> transactions = {
        makeTransaction(1, "TXNXXX1", "2026-10-10 09:00:00",
                        "ACCX901", "ACCX501", "TRANSFER", 750.0),
        makeTransaction(2, "TXNXXX2", "2026-10-10 09:02:00",
                        "ACCX901", "ACCX501", "TRANSFER", 750.0)
    };

    const AnomalyDetector detector;
    const QList<DetectionResult> results =
        detector.analyze(transactions);

    EXPECT_TRUE(hasMethod(results, "TXNXXX1", "duplicate_payment"));
    EXPECT_TRUE(hasMethod(results, "TXNXXX2", "duplicate_payment"));

    const DetectionResult *second = findResult(results, "TXNXXX2");

    ASSERT_NE(second, nullptr);
    EXPECT_EQ(second->riskLevel, "medium");
    EXPECT_DOUBLE_EQ(second->riskScore, 40.0);
    EXPECT_TRUE(second->reasonText().contains("TXNXXX1"));
}

TEST(AnomalyDetector, DuplicateOutsideTheWindowIsNotFlagged)
{
    const QList<Transaction> transactions = {
        makeTransaction(1, "TXNXXX1", "2026-10-10 09:00:00",
                        "ACCX902", "ACCX501", "TRANSFER", 750.0),
        makeTransaction(2, "TXNXXX2", "2026-10-10 11:00:00",
                        "ACCX902", "ACCX501", "TRANSFER", 750.0)
    };

    const AnomalyDetector detector;
    const QList<DetectionResult> results =
        detector.analyze(transactions);

    EXPECT_FALSE(hasMethod(results, "TXNXXX2", "duplicate_payment"));
}

TEST(AnomalyDetector, DuplicateToADifferentReceiverIsNotFlagged)
{
    const QList<Transaction> transactions = {
        makeTransaction(1, "TXNXXX1", "2026-10-10 09:00:00",
                        "ACCX903", "ACCX501", "TRANSFER", 750.0),
        makeTransaction(2, "TXNXXX2", "2026-10-10 09:01:00",
                        "ACCX903", "ACCX502", "TRANSFER", 750.0)
    };

    const AnomalyDetector detector;
    const QList<DetectionResult> results =
        detector.analyze(transactions);

    EXPECT_FALSE(hasMethod(results, "TXNXXX2", "duplicate_payment"));
}

TEST(AnomalyDetector, DuplicateOutsideAmountToleranceIsNotFlagged)
{
    const QList<Transaction> transactions = {
        makeTransaction(1, "TXNXXX1", "2026-10-10 09:00:00",
                        "ACCX904", "ACCX501", "TRANSFER", 750.0),
        makeTransaction(2, "TXNXXX2", "2026-10-10 09:01:00",
                        "ACCX904", "ACCX501", "TRANSFER", 750.02)
    };

    const AnomalyDetector detector;
    const QList<DetectionResult> results =
        detector.analyze(transactions);

    EXPECT_FALSE(hasMethod(results, "TXNXXX2", "duplicate_payment"));
}

TEST(AnomalyDetector, LargeAmountIsFlaggedWithoutHistory)
{
    const QList<Transaction> transactions = {
        makeTransaction(1, "TXNXXX1", "2026-10-10 09:00:00",
                        "ACCX905", "ACCX502", "TRANSFER", 20000.0)
    };

    const AnomalyDetector detector;
    const QList<DetectionResult> results =
        detector.analyze(transactions);

    const DetectionResult *result = findResult(results, "TXNXXX1");

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->methods.contains("large_amount"));
    EXPECT_FALSE(result->methods.contains("behaviour_deviation"));
    EXPECT_EQ(result->riskLevel, "medium");
    EXPECT_TRUE(result->reasonText().contains("limit"));
}

TEST(AnomalyDetector, AmountExactlyAtTheLimitIsNotLarge)
{
    const DetectionSettings settings;
    const AnomalyDetector detector(settings);

    const QList<Transaction> transactions = {
        makeTransaction(1, "TXNXXX1", "2026-10-10 09:00:00",
                        "ACCX906", "ACCX502", "TRANSFER",
                        settings.largeAmountAbsolute)
    };

    EXPECT_FALSE(hasMethod(
        detector.analyze(transactions), "TXNXXX1", "large_amount"));
}

TEST(AnomalyDetector, RapidWithdrawalsAreFlaggedAtTheThreshold)
{
    const QList<Transaction> transactions = {
        makeTransaction(1, "TXNXXX1", "2026-10-10 09:00:00",
                        "ACCX907", "ACCX503", "CASH_OUT", 500.0),
        makeTransaction(2, "TXNXXX2", "2026-10-10 09:20:00",
                        "ACCX907", "ACCX503", "CASH_OUT", 550.0),
        makeTransaction(3, "TXNXXX3", "2026-10-10 09:45:00",
                        "ACCX907", "ACCX503", "CASH_OUT", 520.0)
    };

    const AnomalyDetector detector;
    const QList<DetectionResult> results =
        detector.analyze(transactions);

    EXPECT_FALSE(hasMethod(results, "TXNXXX2", "rapid_withdrawals"));
    EXPECT_TRUE(hasMethod(results, "TXNXXX3", "rapid_withdrawals"));
    EXPECT_FALSE(hasMethod(results, "TXNXXX3", "duplicate_payment"));
}

TEST(AnomalyDetector, NonWithdrawalTypesAreNotRapidWithdrawals)
{
    const QList<Transaction> transactions = {
        makeTransaction(1, "TXNXXX1", "2026-10-10 09:00:00",
                        "ACCX908", "ACCX503", "TRANSFER", 500.0),
        makeTransaction(2, "TXNXXX2", "2026-10-10 09:20:00",
                        "ACCX908", "ACCX503", "TRANSFER", 550.0),
        makeTransaction(3, "TXNXXX3", "2026-10-10 09:45:00",
                        "ACCX908", "ACCX503", "TRANSFER", 520.0)
    };

    const AnomalyDetector detector;

    EXPECT_FALSE(hasMethod(
        detector.analyze(transactions), "TXNXXX3",
        "rapid_withdrawals"));
}

TEST(AnomalyDetector, WithdrawalsOutsideTheWindowAreNotRapid)
{
    const QList<Transaction> transactions = {
        makeTransaction(1, "TXNXXX1", "2026-10-10 09:00:00",
                        "ACCX909", "ACCX503", "CASH_OUT", 500.0),
        makeTransaction(2, "TXNXXX2", "2026-10-10 10:30:00",
                        "ACCX909", "ACCX503", "CASH_OUT", 550.0),
        makeTransaction(3, "TXNXXX3", "2026-10-10 12:00:00",
                        "ACCX909", "ACCX503", "CASH_OUT", 520.0)
    };

    const AnomalyDetector detector;

    EXPECT_FALSE(hasMethod(
        detector.analyze(transactions), "TXNXXX3",
        "rapid_withdrawals"));
}

TEST(AnomalyDetector, UniformHistoryUsesTheRatioRule)
{
    QList<Transaction> transactions;

    for (int i = 0; i < 5; ++i)
    {
        transactions.append(makeTransaction(
            i + 1,
            QString("TXNXXX%1").arg(i + 1),
            QString("2026-10-%1 09:00:00").arg(10 + i),
            "ACCX910",
            QString("ACCX%1").arg(600 + i),
            "TRANSFER",
            500.0
        ));
    }

    transactions.append(makeTransaction(
        6, "TXNXXX6", "2026-10-16 09:00:00",
        "ACCX910", "ACCX700", "TRANSFER", 2000.0));

    const AnomalyDetector detector;
    const QList<DetectionResult> results =
        detector.analyze(transactions);

    const DetectionResult *result = findResult(results, "TXNXXX6");

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->methods.contains("behaviour_deviation"));
    EXPECT_TRUE(result->reasonText().contains("uniform"));
}

TEST(AnomalyDetector, InsufficientHistoryProducesNoBehaviouralFinding)
{
    const QList<Transaction> transactions = {
        makeTransaction(1, "TXNXXX1", "2026-10-10 09:00:00",
                        "ACCX911", "ACCX504", "TRANSFER", 900.0),
        makeTransaction(2, "TXNXXX2", "2026-10-10 09:30:00",
                        "ACCX911", "ACCX505", "TRANSFER", 9000.0)
    };

    const AnomalyDetector detector;

    EXPECT_FALSE(hasMethod(
        detector.analyze(transactions), "TXNXXX2",
        "behaviour_deviation"));
}

TEST(AnomalyDetector, BehaviouralDeviationReportsTheZScore)
{
    QList<Transaction> transactions;

    for (int i = 0; i < 5; ++i)
    {
        transactions.append(makeTransaction(
            i + 1,
            QString("TXNXXX%1").arg(i + 1),
            QString("2026-10-%1 09:00:00").arg(10 + i),
            "ACCX912",
            QString("ACCX%1").arg(800 + i),
            "TRANSFER",
            500.0 + i * 10.0
        ));
    }

    transactions.append(makeTransaction(
        6, "TXNXXX6", "2026-10-16 09:00:00",
        "ACCX912", "ACCX900", "TRANSFER", 3000.0));

    const AnomalyDetector detector;
    const QList<DetectionResult> results =
        detector.analyze(transactions);

    const DetectionResult *result = findResult(results, "TXNXXX6");

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->methods.contains("behaviour_deviation"));
    EXPECT_TRUE(result->reasonText().contains("standard deviations"));
    EXPECT_TRUE(result->reasonText().contains("5 earlier"));
}

TEST(AnomalyDetector, CombinedRulesReachHighRisk)
{
    // A duplicated large payment trips two rules at once.
    const QList<Transaction> transactions = {
        makeTransaction(1, "TXNXXX1", "2026-10-16 09:00:00",
                        "ACCX913", "ACCX700", "TRANSFER", 25000.0),
        makeTransaction(2, "TXNXXX2", "2026-10-16 09:02:00",
                        "ACCX913", "ACCX700", "TRANSFER", 25000.0)
    };

    const AnomalyDetector detector;
    const QList<DetectionResult> results =
        detector.analyze(transactions);

    const DetectionResult *result = findResult(results, "TXNXXX2");

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->methods.contains("large_amount"));
    EXPECT_TRUE(result->methods.contains("duplicate_payment"));
    EXPECT_EQ(result->riskLevel, "high");
    EXPECT_DOUBLE_EQ(result->riskScore, 75.0);
}

TEST(AnomalyDetector, SingleRuleNeverExceedsMedium)
{
    const QList<Transaction> transactions = {
        makeTransaction(1, "TXNXXX1", "2026-10-16 09:00:00",
                        "ACCX916", "ACCX701", "TRANSFER", 25000.0)
    };

    const AnomalyDetector detector;
    const QList<DetectionResult> results =
        detector.analyze(transactions);

    const DetectionResult *result = findResult(results, "TXNXXX1");

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->riskLevel, "medium");
    EXPECT_DOUBLE_EQ(result->riskScore, 35.0);
}

TEST(AnomalyDetector, RepeatedAnalysisIsIdentical)
{
    const QList<Transaction> transactions = {
        makeTransaction(1, "TXNXXX1", "2026-10-10 09:00:00",
                        "ACCX914", "ACCX505", "TRANSFER", 750.0),
        makeTransaction(2, "TXNXXX2", "2026-10-10 09:01:00",
                        "ACCX914", "ACCX505", "TRANSFER", 750.0),
        makeTransaction(3, "TXNXXX3", "2026-10-10 09:02:00",
                        "ACCX914", "ACCX506", "CASH_OUT", 15000.0)
    };

    const AnomalyDetector detector;

    const QList<DetectionResult> first = detector.analyze(transactions);
    const QList<DetectionResult> second = detector.analyze(transactions);

    ASSERT_EQ(first.size(), second.size());

    for (int i = 0; i < first.size(); ++i)
    {
        EXPECT_DOUBLE_EQ(first.at(i).riskScore,
                         second.at(i).riskScore);
        EXPECT_EQ(first.at(i).methods, second.at(i).methods);
        EXPECT_EQ(first.at(i).reasons, second.at(i).reasons);
    }
}

TEST(AnomalyDetector, MalformedTimestampsDoNotCrash)
{
    QList<Transaction> transactions = {
        makeTransaction(1, "TXNXXX1", "not-a-date",
                        "ACCX915", "ACCX505", "TRANSFER", 750.0),
        makeTransaction(2, "TXNXXX2", "2026-10-10 09:01:00",
                        "ACCX915", "ACCX505", "TRANSFER", 750.0)
    };

    const AnomalyDetector detector;
    const QList<DetectionResult> results =
        detector.analyze(transactions);

    EXPECT_EQ(results.size(), 2);
}

TEST(AnomalyDetector, ClassifiesWithdrawalTypes)
{
    EXPECT_TRUE(AnomalyDetector::isWithdrawalType("CASH_OUT"));
    EXPECT_TRUE(AnomalyDetector::isWithdrawalType("cash_out"));
    EXPECT_TRUE(AnomalyDetector::isWithdrawalType(" ATM_WITHDRAWAL "));
    EXPECT_TRUE(AnomalyDetector::isWithdrawalType("DEBIT"));
    EXPECT_TRUE(AnomalyDetector::isWithdrawalType("WITHDRAWAL"));

    EXPECT_FALSE(AnomalyDetector::isWithdrawalType("TRANSFER"));
    EXPECT_FALSE(AnomalyDetector::isWithdrawalType("PAYMENT"));
    EXPECT_FALSE(AnomalyDetector::isWithdrawalType(""));
}

TEST(AnomalyDetector, ExposesItsConfiguration)
{
    DetectionSettings settings;
    settings.duplicateWindowMinutes = 15;

    const AnomalyDetector detector(settings);

    EXPECT_EQ(detector.configuration().duplicateWindowMinutes, 15);
}
