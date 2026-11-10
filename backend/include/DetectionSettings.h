
#ifndef DETECTIONSETTINGS_H
#define DETECTIONSETTINGS_H

#include <QString>

// Thresholds, rule weights and risk bands used by the detection engine.
// Every value is configurable so the behaviour can be tested and tuned
// without recompiling (see Phase 7 of the project plan).
struct DetectionSettings
{
    // Rule thresholds.
    double largeAmountAbsolute = 10000.0;
    int duplicateWindowMinutes = 10;
    double duplicateAmountTolerance = 0.01;
    int rapidWithdrawalWindowMinutes = 60;
    int rapidWithdrawalCount = 3;
    double unusualAmountZScore = 3.0;
    double zeroVarianceRatio = 2.0;

    // History required before a personal baseline may be used.
    // Five earlier transactions keep the z-score stable enough to
    // avoid flagging ordinary variation on tiny samples.
    int minimumHistory = 5;

    // Rule weights.
    double weightDuplicatePayment = 40.0;
    double weightLargeAmount = 35.0;
    double weightRapidWithdrawals = 35.0;
    double weightBehaviourDeviation = 25.0;

    // Risk bands. Scores at or above a band are classified into it.
    double mediumThreshold = 35.0;
    double highThreshold = 65.0;

    // Detection engines. The rule engine preserves the deterministic,
    // explainable checks; the ML model (PaySim-trained HGB, type+amount)
    // adds a learned score when enabled. Defaults keep existing behaviour.
    bool useRuleEngine = true;
    bool useMlModel = false;

    // Returns false and fills error when a value is out of range.
    bool validate(QString *error = nullptr) const;
};

#endif // DETECTIONSETTINGS_H
