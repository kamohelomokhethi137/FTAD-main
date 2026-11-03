
// Tests for DetectionSettings::validate().
//
// Every threshold is user-tunable, so the validation rules are the
// first line of defence against a configuration that would make the
// detector behave nonsensically (zero windows, inverted bands, ...).

#include <gtest/gtest.h>

#include <QString>

#include <cmath>
#include <limits>

#include "DetectionSettings.h"

namespace
{
    QString failureOf(const DetectionSettings &settings)
    {
        QString error;
        EXPECT_FALSE(settings.validate(&error));
        EXPECT_FALSE(error.isEmpty());

        return error;
    }
}

TEST(DetectionSettings, DefaultsAreValid)
{
    const DetectionSettings settings;

    QString error;
    EXPECT_TRUE(settings.validate(&error));
    EXPECT_TRUE(error.isEmpty());
}

TEST(DetectionSettings, DefaultWeightsAndBandsAreConsistent)
{
    const DetectionSettings settings;

    EXPECT_GT(settings.weightDuplicatePayment, 0.0);
    EXPECT_GT(settings.weightLargeAmount, 0.0);
    EXPECT_GT(settings.weightRapidWithdrawals, 0.0);
    EXPECT_GT(settings.weightBehaviourDeviation, 0.0);

    // A single rule must at least reach the medium band, otherwise a
    // flagged transaction would be reported with no risk level.
    const double strongest = std::max(
        std::max(settings.weightDuplicatePayment,
                 settings.weightLargeAmount),
        std::max(settings.weightRapidWithdrawals,
                 settings.weightBehaviourDeviation));

    EXPECT_GE(strongest, settings.mediumThreshold);
    EXPECT_LT(settings.mediumThreshold, settings.highThreshold);
    EXPECT_LE(settings.highThreshold, 100.0);
}

TEST(DetectionSettings, RejectsZeroLargeAmountThreshold)
{
    DetectionSettings settings;
    settings.largeAmountAbsolute = 0.0;

    EXPECT_TRUE(failureOf(settings).contains("Large amount"));
}

TEST(DetectionSettings, RejectsNegativeLargeAmountThreshold)
{
    DetectionSettings settings;
    settings.largeAmountAbsolute = -1.0;

    EXPECT_FALSE(settings.validate());
}

TEST(DetectionSettings, RejectsNonFiniteThresholds)
{
    DetectionSettings nan;
    nan.largeAmountAbsolute =
        std::numeric_limits<double>::quiet_NaN();
    EXPECT_FALSE(nan.validate());

    DetectionSettings infinite;
    infinite.unusualAmountZScore =
        std::numeric_limits<double>::infinity();
    EXPECT_FALSE(infinite.validate());
}

TEST(DetectionSettings, RejectsZeroDuplicateWindow)
{
    DetectionSettings settings;
    settings.duplicateWindowMinutes = 0;

    EXPECT_TRUE(failureOf(settings).contains("Duplicate window"));
}

TEST(DetectionSettings, RejectsNegativeDuplicateTolerance)
{
    DetectionSettings settings;
    settings.duplicateAmountTolerance = -0.01;

    EXPECT_FALSE(settings.validate());
}

TEST(DetectionSettings, AcceptsZeroDuplicateTolerance)
{
    // An exact-match duplicate rule is legitimate.
    DetectionSettings settings;
    settings.duplicateAmountTolerance = 0.0;

    EXPECT_TRUE(settings.validate());
}

TEST(DetectionSettings, RejectsZeroRapidWithdrawalWindow)
{
    DetectionSettings settings;
    settings.rapidWithdrawalWindowMinutes = 0;

    EXPECT_TRUE(failureOf(settings).contains("Rapid withdrawal window"));
}

TEST(DetectionSettings, RejectsRapidWithdrawalCountBelowTwo)
{
    DetectionSettings settings;
    settings.rapidWithdrawalCount = 1;

    EXPECT_TRUE(failureOf(settings).contains("Rapid withdrawal count"));
}

TEST(DetectionSettings, RejectsNonPositiveZScore)
{
    DetectionSettings settings;
    settings.unusualAmountZScore = 0.0;

    EXPECT_FALSE(settings.validate());
}

TEST(DetectionSettings, RejectsZeroVarianceRatioAtOrBelowOne)
{
    DetectionSettings settings;
    settings.zeroVarianceRatio = 1.0;

    EXPECT_TRUE(failureOf(settings).contains("Zero variance ratio"));
}

TEST(DetectionSettings, RejectsMinimumHistoryBelowOne)
{
    DetectionSettings settings;
    settings.minimumHistory = 0;

    EXPECT_TRUE(failureOf(settings).contains("Minimum history"));
}

TEST(DetectionSettings, RejectsNegativeRuleWeight)
{
    DetectionSettings settings;
    settings.weightBehaviourDeviation = -5.0;

    EXPECT_TRUE(failureOf(settings).contains("weights"));
}

TEST(DetectionSettings, RejectsZeroMediumThreshold)
{
    DetectionSettings settings;
    settings.mediumThreshold = 0.0;

    EXPECT_FALSE(settings.validate());
}

TEST(DetectionSettings, RejectsHighThresholdNotAboveMedium)
{
    DetectionSettings settings;
    settings.mediumThreshold = 60.0;
    settings.highThreshold = 60.0;

    EXPECT_TRUE(failureOf(settings).contains("above the medium"));
}

TEST(DetectionSettings, RejectsHighThresholdAboveHundred)
{
    DetectionSettings settings;
    settings.highThreshold = 120.0;

    EXPECT_TRUE(failureOf(settings).contains("cannot exceed 100"));
}

TEST(DetectionSettings, AcceptsCustomValidConfiguration)
{
    DetectionSettings settings;
    settings.largeAmountAbsolute = 25000.0;
    settings.duplicateWindowMinutes = 30;
    settings.rapidWithdrawalCount = 5;
    settings.minimumHistory = 10;
    settings.mediumThreshold = 30.0;
    settings.highThreshold = 80.0;

    QString error;
    EXPECT_TRUE(settings.validate(&error));
    EXPECT_TRUE(error.isEmpty());
}

TEST(DetectionSettings, ValidationWithoutErrorPointerDoesNotCrash)
{
    DetectionSettings settings;
    settings.largeAmountAbsolute = -1.0;

    EXPECT_FALSE(settings.validate());
    EXPECT_FALSE(settings.validate(nullptr));
}
