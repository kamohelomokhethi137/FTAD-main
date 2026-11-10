
#include <gtest/gtest.h>
#include "MlFraudModel.h"
#include "AnomalyDetector.h"
#include "DetectionResult.h"
#include "DetectionSettings.h"
#include "support/TestSupport.h"

TEST(MlFraudModel, ProducesDeterministicScores)
{
    const double a = MlFraudModel::predictRiskScore("TRANSFER", 200000.0);
    const double b = MlFraudModel::predictRiskScore("TRANSFER", 200000.0);
    EXPECT_DOUBLE_EQ(a, b);
    EXPECT_GE(a, 0.0);
    EXPECT_LE(a, 100.0);
}

TEST(MlFraudModel, HigherRiskForLargeTransferThanSmallCashIn)
{
    const double large = MlFraudModel::predictRiskScore("TRANSFER", 250000.0);
    const double small = MlFraudModel::predictRiskScore("CASH_IN", 100.0);
    EXPECT_GT(large, small);
}

TEST(AnomalyDetector, MlModelCanFlagWhenEnabled)
{
    DetectionSettings settings;
    settings.useRuleEngine = false;
    settings.useMlModel = true;
    settings.largeAmountAbsolute = 1e18; // irrelevant
    AnomalyDetector detector(settings);
    QList<Transaction> tx = {
        makeTransaction(1, "TXNXXX1", "2026-10-10 09:00:00",
                        "ACCX901", "ACCX501", "TRANSFER", 250000.0)
    };
    const auto results = detector.analyze(tx);
    ASSERT_EQ(results.size(), 1);
    EXPECT_TRUE(results[0].methods.contains("ml_model"));
    EXPECT_GT(results[0].riskScore, 0.0);
}

TEST(DetectionSettings, RejectsNoEnabledEngines)
{
    DetectionSettings settings;
    settings.useRuleEngine = false;
    settings.useMlModel = false;
    EXPECT_FALSE(settings.validate());
}
