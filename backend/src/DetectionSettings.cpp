
#include "DetectionSettings.h"

#include <algorithm>

namespace
{
    bool isFinite(double value)
    {
        return value == value && value > -1.0e308 && value < 1.0e308;
    }
}

bool DetectionSettings::validate(QString *error) const
{
    auto fail = [error](const QString &message)
    {
        if (error)
            *error = message;

        return false;
    };

    if (!isFinite(largeAmountAbsolute) || largeAmountAbsolute <= 0.0)
        return fail("Large amount threshold must be greater than zero.");

    if (duplicateWindowMinutes <= 0)
        return fail("Duplicate window must be at least one minute.");

    if (!isFinite(duplicateAmountTolerance) ||
        duplicateAmountTolerance < 0.0)
        return fail("Duplicate amount tolerance cannot be negative.");

    if (rapidWithdrawalWindowMinutes <= 0)
        return fail("Rapid withdrawal window must be at least one minute.");

    if (rapidWithdrawalCount < 2)
        return fail("Rapid withdrawal count must be at least two.");

    if (!isFinite(unusualAmountZScore) || unusualAmountZScore <= 0.0)
        return fail("Behavioural z-score threshold must be positive.");

    if (!isFinite(zeroVarianceRatio) || zeroVarianceRatio <= 1.0)
        return fail("Zero variance ratio must be greater than one.");

    if (minimumHistory < 1)
        return fail("Minimum history must be at least one transaction.");

    const double weights[] = {
        weightDuplicatePayment,
        weightLargeAmount,
        weightRapidWithdrawals,
        weightBehaviourDeviation
    };

    const std::size_t weightCount =
        sizeof(weights) / sizeof(weights[0]);

    if (std::any_of(
            weights,
            weights + weightCount,
            [](double weight)
            {
                return !isFinite(weight) || weight < 0.0;
            }))
    {
        return fail("Rule weights cannot be negative.");
    }

    if (!isFinite(mediumThreshold) || mediumThreshold <= 0.0)
        return fail("Medium risk threshold must be greater than zero.");

    if (!isFinite(highThreshold) || highThreshold <= mediumThreshold)
        return fail("High risk threshold must be above the medium threshold.");

    if (highThreshold > 100.0)
        return fail("High risk threshold cannot exceed 100.");

    if (!useRuleEngine && !useMlModel)
        return fail("At least one detection engine must be enabled.");

    if (error)
        error->clear();

    return true;
}
