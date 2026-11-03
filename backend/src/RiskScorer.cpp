
#include "RiskScorer.h"

RiskScorer::RiskScorer(const DetectionSettings &settings)
    : settings(settings)
{
}

QString RiskScorer::levelForScore(double score) const
{
    if (score >= settings.highThreshold)
        return "high";

    if (score >= settings.mediumThreshold)
        return "medium";

    if (score > 0.0)
        return "low";

    return QString();
}

DetectionResult RiskScorer::score(
    const Transaction &transaction,
    const QList<RuleHit> &hits) const
{
    DetectionResult result;

    result.transactionId = transaction.id;
    result.transactionReference = transaction.transactionId;
    result.timestamp = transaction.timestamp;
    result.senderId = transaction.senderId;
    result.receiverId = transaction.receiverId;
    result.amount = transaction.amount;
    result.currency = transaction.currency;

    if (hits.isEmpty())
    {
        result.riskScore = 0.0;
        return result;
    }

    double total = 0.0;
    double strongestWeight = -1.0;

    for (const RuleHit &hit : hits)
    {
        total += hit.weight;

        result.methods.append(hit.method);
        result.reasons.append(hit.reason);

        if (hit.weight > strongestWeight)
        {
            strongestWeight = hit.weight;
            result.primaryMethod = hit.method;
        }
    }

    // The score is capped so no combination can exceed the schema limit.
    result.riskScore = total > 100.0 ? 100.0 : total;
    result.riskLevel = levelForScore(result.riskScore);

    return result;
}
