
#include "DetectionService.h"

#include "AnomalyDetector.h"
#include "DetectionRepository.h"
#include "Transaction.h"
#include "TransactionRepository.h"

#include <utility>

#include <algorithm>

DetectionService::DetectionService()
{
}

DetectionService::DetectionService(const DetectionSettings &settings)
    : settings(settings)
{
}

const DetectionSettings &DetectionService::configuration() const
{
    return settings;
}

bool DetectionService::analyzeAll(
    int *analyzedCount,
    int *flaggedCount)
{
    errorMessage.clear();

    if (analyzedCount)
        *analyzedCount = 0;

    if (flaggedCount)
        *flaggedCount = 0;

    QString settingsError;

    if (!settings.validate(&settingsError))
    {
        errorMessage = std::move(settingsError);
        return false;
    }

    TransactionRepository transactionRepository;

    const QList<Transaction> transactions =
        transactionRepository.getAllTransactions();

    if (!transactionRepository.lastError().isEmpty())
    {
        errorMessage = transactionRepository.lastError();
        return false;
    }

    QList<int> analyzedIds;
    analyzedIds.reserve(transactions.size());

    for (const Transaction &transaction : transactions)
        analyzedIds.append(transaction.id);

    AnomalyDetector detector(settings);

    const QList<DetectionResult> results =
        detector.analyze(transactions);

    DetectionRepository repository;

    if (!repository.saveResults(results, analyzedIds))
    {
        errorMessage = repository.lastError();
        return false;
    }

    int flagged = static_cast<int>(std::count_if(
        results.begin(),
        results.end(),
        [](const DetectionResult &result)
        {
            return result.flagged();
        }));

    if (analyzedCount)
        *analyzedCount = static_cast<int>(results.size());

    if (flaggedCount)
        *flaggedCount = flagged;

    return true;
}

QList<DetectionResult> DetectionService::flaggedResults()
{
    DetectionRepository repository;

    const QList<DetectionResult> results =
        repository.loadFlaggedResults();

    errorMessage = repository.lastError();

    return results;
}

QString DetectionService::lastError() const
{
    return errorMessage;
}
