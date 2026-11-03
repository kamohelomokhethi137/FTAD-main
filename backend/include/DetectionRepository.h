
#ifndef DETECTIONREPOSITORY_H
#define DETECTIONREPOSITORY_H

#include "DetectionResult.h"

#include <QList>
#include <QSqlDatabase>
#include <QString>

// Stores and loads detection outcomes in the anomalies table.
class DetectionRepository
{
public:
    // Replaces the stored outcome for every analysed transaction.
    // Rows for transactions that are no longer flagged are removed,
    // while still-flagged rows are updated in place so that an existing
    // investigation keeps pointing at the same anomaly record.
    bool saveResults(
        const QList<DetectionResult> &results,
        const QList<int> &analyzedTransactionIds);

    QList<DetectionResult> loadFlaggedResults();

    bool clearResults();

    QString lastError() const;

private:
    bool upsert(
        QSqlDatabase &database,
        const DetectionResult &result);

    QString errorMessage;
};

#endif // DETECTIONREPOSITORY_H
