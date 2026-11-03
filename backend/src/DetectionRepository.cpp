
#include "DetectionRepository.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSet>
#include <QVariant>

bool DetectionRepository::upsert(
    QSqlDatabase &database,
    const DetectionResult &result)
{
    QSqlQuery existing(database);

    existing.prepare(
        "SELECT id FROM anomalies "
        "WHERE transaction_id = ? LIMIT 1"
    );

    existing.addBindValue(result.transactionId);

    if (!existing.exec())
    {
        errorMessage = existing.lastError().text();
        return false;
    }

    if (existing.next())
    {
        const int anomalyId = existing.value(0).toInt();

        QSqlQuery update(database);

        update.prepare(
            "UPDATE anomalies SET "
            "detection_method = ?, risk_score = ?, "
            "risk_level = ?, reason = ?, "
            "detected_at = CURRENT_TIMESTAMP "
            "WHERE id = ?"
        );

        update.addBindValue(result.methodText());
        update.addBindValue(result.riskScore);
        update.addBindValue(result.riskLevel);
        update.addBindValue(result.reasonText());
        update.addBindValue(anomalyId);

        if (!update.exec())
        {
            errorMessage = update.lastError().text();
            return false;
        }

        return true;
    }

    QSqlQuery insert(database);

    insert.prepare(
        "INSERT INTO anomalies "
        "(transaction_id, detection_method, risk_score, "
        "risk_level, reason) "
        "VALUES (?, ?, ?, ?, ?)"
    );

    insert.addBindValue(result.transactionId);
    insert.addBindValue(result.methodText());
    insert.addBindValue(result.riskScore);
    insert.addBindValue(result.riskLevel);
    insert.addBindValue(result.reasonText());

    if (!insert.exec())
    {
        errorMessage = insert.lastError().text();
        return false;
    }

    return true;
}

bool DetectionRepository::saveResults(
    const QList<DetectionResult> &results,
    const QList<int> &analyzedTransactionIds)
{
    errorMessage.clear();

    QSqlDatabase database =
        QSqlDatabase::database("FTADConnection");

    if (!database.isOpen())
    {
        errorMessage = "Database is not connected.";
        return false;
    }

    if (!database.transaction())
    {
        errorMessage = database.lastError().text();
        return false;
    }

    QSet<int> flaggedIds;

    for (const DetectionResult &result : results)
    {
        if (result.flagged())
            flaggedIds.insert(result.transactionId);
    }

    // Drop outcomes for transactions that are no longer flagged.
    for (int transactionId : analyzedTransactionIds)
    {
        if (flaggedIds.contains(transactionId))
            continue;

        QSqlQuery remove(database);

        remove.prepare(
            "DELETE FROM anomalies WHERE transaction_id = ?"
        );

        remove.addBindValue(transactionId);

        if (!remove.exec())
        {
            errorMessage = remove.lastError().text();
            database.rollback();
            return false;
        }
    }

    for (const DetectionResult &result : results)
    {
        if (!result.flagged())
            continue;

        if (!upsert(database, result))
        {
            database.rollback();
            return false;
        }
    }

    if (!database.commit())
    {
        errorMessage = database.lastError().text();
        database.rollback();
        return false;
    }

    return true;
}

QList<DetectionResult> DetectionRepository::loadFlaggedResults()
{
    QList<DetectionResult> results;
    errorMessage.clear();

    QSqlDatabase database =
        QSqlDatabase::database("FTADConnection");

    if (!database.isOpen())
    {
        errorMessage = "Database is not connected.";
        return results;
    }

    QSqlQuery query(database);

    if (!query.exec(
        "SELECT a.transaction_id, a.detection_method, "
        "a.risk_score, a.risk_level, a.reason, "
        "t.transaction_id, t.timestamp, t.sender_id, "
        "t.receiver_id, t.amount, t.currency "
        "FROM anomalies a "
        "JOIN transactions t ON t.id = a.transaction_id "
        "ORDER BY a.risk_score DESC, t.timestamp DESC"))
    {
        errorMessage = query.lastError().text();
        return results;
    }

    while (query.next())
    {
        DetectionResult result;

        result.transactionId = query.value(0).toInt();
        result.primaryMethod = query.value(1).toString();
        result.riskScore = query.value(2).toDouble();
        result.riskLevel = query.value(3).toString();

        const QString reason = query.value(4).toString();

        if (!reason.isEmpty())
            result.reasons = reason.split("; ");

        result.methods = query.value(1).toString().split(", ");

        result.transactionReference = query.value(5).toString();
        result.timestamp = query.value(6).toString();
        result.senderId = query.value(7).toString();
        result.receiverId = query.value(8).toString();
        result.amount = query.value(9).toDouble();
        result.currency = query.value(10).toString();

        results.append(result);
    }

    return results;
}

bool DetectionRepository::clearResults()
{
    errorMessage.clear();

    QSqlDatabase database =
        QSqlDatabase::database("FTADConnection");

    if (!database.isOpen())
    {
        errorMessage = "Database is not connected.";
        return false;
    }

    QSqlQuery query(database);

    if (!query.exec("DELETE FROM anomalies"))
    {
        errorMessage = query.lastError().text();
        return false;
    }

    return true;
}

QString DetectionRepository::lastError() const
{
    return errorMessage;
}
