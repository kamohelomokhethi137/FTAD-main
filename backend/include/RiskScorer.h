
#ifndef RISKSCORER_H
#define RISKSCORER_H

#include "DetectionResult.h"
#include "DetectionSettings.h"
#include "Transaction.h"

#include <QList>

// Combines triggered checks into a single 0-100 score and risk band.
// The result is deterministic: the same hits always produce the same
// score, which keeps assessments reproducible.
class RiskScorer
{
public:
    explicit RiskScorer(const DetectionSettings &settings);

    DetectionResult score(
        const Transaction &transaction,
        const QList<RuleHit> &hits) const;

    QString levelForScore(double score) const;

private:
    DetectionSettings settings;
};

#endif // RISKSCORER_H
