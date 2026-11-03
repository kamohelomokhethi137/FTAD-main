
// Tests for RiskScorer.
//
// Scoring must be deterministic and bounded, because the score is
// stored in the anomalies table and drives the risk level shown to the
// reviewer.

#include <gtest/gtest.h>

#include <QList>
#include <QString>

#include "DetectionResult.h"
#include "DetectionSettings.h"
#include "RiskScorer.h"
#include "Transaction.h"
#include "support/TestSupport.h"

namespace
{
    RuleHit makeHit(
        const QString &method,
        const QString &reason,
        double weight)
    {
        RuleHit hit;

        hit.method = method;
        hit.reason = reason;
        hit.weight = weight;

        return hit;
    }

    Transaction sampleTransaction()
    {
        return makeTransaction(
            42, "TXNXX42", "2026-10-30 09:15:00",
            "ACCX101", "ACCX202", "TRANSFER", 1500.0);
    }
}

TEST(RiskScorer, CopiesTransactionDetailsIntoTheResult)
{
    const DetectionSettings settings;
    const RiskScorer scorer(settings);

    const DetectionResult result =
        scorer.score(sampleTransaction(), {});

    EXPECT_EQ(result.transactionId, 42);
    EXPECT_EQ(result.transactionReference, "TXNXX42");
    EXPECT_EQ(result.timestamp, "2026-10-30 09:15:00");
    EXPECT_EQ(result.senderId, "ACCX101");
    EXPECT_EQ(result.receiverId, "ACCX202");
    EXPECT_DOUBLE_EQ(result.amount, 1500.0);
    EXPECT_EQ(result.currency, "LSL");
}

TEST(RiskScorer, NoHitsMeansZeroScoreAndNoLevel)
{
    const RiskScorer scorer{DetectionSettings()};

    const DetectionResult result =
        scorer.score(sampleTransaction(), {});

    EXPECT_DOUBLE_EQ(result.riskScore, 0.0);
    EXPECT_TRUE(result.riskLevel.isEmpty());
    EXPECT_FALSE(result.flagged());
    EXPECT_TRUE(result.methods.isEmpty());
}

TEST(RiskScorer, SingleHitUsesItsWeight)
{
    const DetectionSettings settings;
    const RiskScorer scorer(settings);

    const DetectionResult result = scorer.score(
        sampleTransaction(),
        {makeHit("duplicate_payment", "duplicate", 40.0)}
    );

    EXPECT_DOUBLE_EQ(result.riskScore, 40.0);
    EXPECT_EQ(result.riskLevel, "medium");
    EXPECT_TRUE(result.flagged());
    EXPECT_EQ(result.primaryMethod, "duplicate_payment");
}

TEST(RiskScorer, MultipleHitsAddUp)
{
    const RiskScorer scorer{DetectionSettings()};

    const DetectionResult result = scorer.score(
        sampleTransaction(),
        {
            makeHit("duplicate_payment", "duplicate", 40.0),
            makeHit("rapid_withdrawals", "rapid", 35.0)
        }
    );

    EXPECT_DOUBLE_EQ(result.riskScore, 75.0);
    EXPECT_EQ(result.riskLevel, "high");
    EXPECT_EQ(result.methods.size(), 2);
    EXPECT_EQ(result.reasons.size(), 2);
}

TEST(RiskScorer, ScoreNeverExceedsOneHundred)
{
    const RiskScorer scorer{DetectionSettings()};

    // All four rules together weigh 135.
    const DetectionResult result = scorer.score(
        sampleTransaction(),
        {
            makeHit("duplicate_payment", "a", 40.0),
            makeHit("large_amount", "b", 35.0),
            makeHit("rapid_withdrawals", "c", 35.0),
            makeHit("behaviour_deviation", "d", 25.0)
        }
    );

    EXPECT_DOUBLE_EQ(result.riskScore, 100.0);
    EXPECT_EQ(result.riskLevel, "high");
    EXPECT_EQ(result.methods.size(), 4);
}

TEST(RiskScorer, PrimaryMethodIsTheStrongestHit)
{
    const RiskScorer scorer{DetectionSettings()};

    const DetectionResult result = scorer.score(
        sampleTransaction(),
        {
            makeHit("behaviour_deviation", "behaviour", 25.0),
            makeHit("large_amount", "large", 35.0),
            makeHit("duplicate_payment", "duplicate", 40.0)
        }
    );

    EXPECT_EQ(result.primaryMethod, "duplicate_payment");
}

TEST(RiskScorer, LevelBoundariesAreInclusiveAtTheLowerEdge)
{
    const DetectionSettings settings;
    const RiskScorer scorer(settings);

    EXPECT_TRUE(scorer.levelForScore(0.0).isEmpty());
    EXPECT_EQ(scorer.levelForScore(0.5), "low");
    EXPECT_EQ(scorer.levelForScore(settings.mediumThreshold - 1.0),
              "low");
    EXPECT_EQ(scorer.levelForScore(settings.mediumThreshold),
              "medium");
    EXPECT_EQ(scorer.levelForScore(settings.highThreshold - 1.0),
              "medium");
    EXPECT_EQ(scorer.levelForScore(settings.highThreshold), "high");
    EXPECT_EQ(scorer.levelForScore(100.0), "high");
}

TEST(RiskScorer, HonoursCustomBands)
{
    DetectionSettings settings;
    settings.mediumThreshold = 20.0;
    settings.highThreshold = 50.0;

    const RiskScorer scorer(settings);

    const DetectionResult result = scorer.score(
        sampleTransaction(),
        {makeHit("behaviour_deviation", "behaviour", 25.0)}
    );

    EXPECT_EQ(result.riskLevel, "medium");
}

TEST(RiskScorer, ReasonTextJoinsAllHits)
{
    const RiskScorer scorer{DetectionSettings()};

    const DetectionResult result = scorer.score(
        sampleTransaction(),
        {
            makeHit("duplicate_payment", "first reason", 40.0),
            makeHit("large_amount", "second reason", 35.0)
        }
    );

    EXPECT_EQ(result.reasonText(),
              "first reason; second reason");
    EXPECT_EQ(result.methodText(),
              "duplicate_payment, large_amount");
}

TEST(RiskScorer, IsDeterministicForTheSameInput)
{
    const RiskScorer scorer{DetectionSettings()};

    const QList<RuleHit> hits = {
        makeHit("duplicate_payment", "duplicate", 40.0),
        makeHit("large_amount", "large", 35.0)
    };

    const DetectionResult first = scorer.score(sampleTransaction(), hits);
    const DetectionResult second = scorer.score(sampleTransaction(), hits);

    EXPECT_DOUBLE_EQ(first.riskScore, second.riskScore);
    EXPECT_EQ(first.riskLevel, second.riskLevel);
    EXPECT_EQ(first.primaryMethod, second.primaryMethod);
    EXPECT_EQ(first.reasons, second.reasons);
}
