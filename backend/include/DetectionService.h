
#ifndef DETECTIONSERVICE_H
#define DETECTIONSERVICE_H

#include "DetectionResult.h"
#include "DetectionSettings.h"

#include <QList>
#include <QString>

// Ties the pure detection engine to the database: loads transactions,
// runs the detector, stores the outcomes and returns the summary counts.
class DetectionService
{
public:
    DetectionService();
    explicit DetectionService(const DetectionSettings &settings);

    // Re-analyses every stored transaction. Returns false only when the
    // analysis could not be completed (invalid settings or database
    // error); a successful run with no findings still returns true.
    bool analyzeAll(int *analyzedCount = nullptr,
                    int *flaggedCount = nullptr);

    QList<DetectionResult> flaggedResults();

    const DetectionSettings &configuration() const;

    QString lastError() const;

private:
    DetectionSettings settings;
    QString errorMessage;
};

#endif // DETECTIONSERVICE_H
