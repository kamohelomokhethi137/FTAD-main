
#include "InvestigationRepository.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QVariant>

namespace
{
    QStringList allowedStatuses()
    {
        return {
            "pending",
            "in_progress",
            "resolved",
            "dismissed"
        };
    }

    bool isClosedStatus(const QString &status)
    {
        return status == "resolved" || status == "dismissed";
    }
}

bool InvestigationRepository::isValidStatus(const QString &caseStatus)
{
    return allowedStatuses().contains(caseStatus);
}

int InvestigationRepository::ensureCases()
{
    errorMessage.clear();

    QSqlDatabase database =
        QSqlDatabase::database("FTADConnection");

    if (!database.isOpen())
    {
        errorMessage = "Database is not connected.";
        return -1;
    }

    if (!database.transaction())
    {
        errorMessage = database.lastError().text();
        return -1;
    }

    QSqlQuery missing(database);

    if (!missing.exec(
        "SELECT a.id FROM anomalies a "
        "LEFT JOIN investigations i ON i.anomaly_id = a.id "
        "WHERE i.id IS NULL"))
    {
        errorMessage = missing.lastError().text();
        database.rollback();
        return -1;
    }

    QList<int> anomalyIds;

    while (missing.next())
        anomalyIds.append(missing.value(0).toInt());

    int created = 0;

    for (int anomalyId : anomalyIds)
    {
        QSqlQuery insert(database);

        insert.prepare(
            "INSERT INTO investigations (anomaly_id, case_status) "
            "VALUES (?, 'pending')"
        );

        insert.addBindValue(anomalyId);

        if (!insert.exec())
        {
            errorMessage = insert.lastError().text();
            database.rollback();
            return -1;
        }

        const int investigationId =
            insert.lastInsertId().toInt();

        QSqlQuery event(database);

        event.prepare(
            "INSERT INTO investigation_events "
            "(investigation_id, event_type, new_status, notes) "
            "VALUES (?, 'opened', 'pending', ?)"
        );

        event.addBindValue(investigationId);
        event.addBindValue("Case opened automatically "
                           "from a detection finding.");

        if (!event.exec())
        {
            errorMessage = event.lastError().text();
            database.rollback();
            return -1;
        }

        ++created;
    }

    if (!database.commit())
    {
        errorMessage = database.lastError().text();
        database.rollback();
        return -1;
    }

    return created;
}

QList<InvestigationCase> InvestigationRepository::getCases()
{
    QList<InvestigationCase> cases;
    errorMessage.clear();

    QSqlDatabase database =
        QSqlDatabase::database("FTADConnection");

    if (!database.isOpen())
    {
        errorMessage = "Database is not connected.";
        return cases;
    }

    QSqlQuery query(database);

    if (!query.exec(
        "SELECT i.id, i.anomaly_id, i.case_status, i.assigned_to, "
        "i.notes, i.opened_at, i.closed_at, "
        "a.transaction_id, a.risk_score, a.risk_level, "
        "a.detection_method, "
        "t.id, t.transaction_id, t.timestamp, t.sender_id, "
        "t.receiver_id, t.amount, t.currency "
        "FROM investigations i "
        "JOIN anomalies a ON a.id = i.anomaly_id "
        "JOIN transactions t ON t.id = a.transaction_id "
        "ORDER BY "
        "CASE i.case_status "
        "  WHEN 'pending' THEN 0 "
        "  WHEN 'in_progress' THEN 1 "
        "  WHEN 'resolved' THEN 2 "
        "  ELSE 3 END, "
        "a.risk_score DESC, t.timestamp DESC"))
    {
        errorMessage = query.lastError().text();
        return cases;
    }

    while (query.next())
    {
        InvestigationCase item;

        item.investigationId = query.value(0).toInt();
        item.anomalyId = query.value(1).toInt();
        item.caseStatus = query.value(2).toString();
        item.assignedTo = query.value(3).toString();
        item.notes = query.value(4).toString();
        item.openedAt = query.value(5).toString();
        item.closedAt = query.value(6).toString();

        item.riskScore = query.value(8).toDouble();
        item.riskLevel = query.value(9).toString();
        item.methods = query.value(10).toString().split(", ");

        item.transactionId = query.value(11).toInt();
        item.transactionReference = query.value(12).toString();
        item.timestamp = query.value(13).toString();
        item.senderId = query.value(14).toString();
        item.receiverId = query.value(15).toString();
        item.amount = query.value(16).toDouble();
        item.currency = query.value(17).toString();

        cases.append(item);
    }

    return cases;
}

bool InvestigationRepository::updateCase(
    int investigationId,
    const QString &caseStatus,
    const QString &assignedTo,
    const QString &notes)
{
    errorMessage.clear();

    if (!isValidStatus(caseStatus))
    {
        errorMessage = "Invalid case status.";
        return false;
    }

    QSqlDatabase database =
        QSqlDatabase::database("FTADConnection");

    if (!database.isOpen())
    {
        errorMessage = "Database is not connected.";
        return false;
    }

    // The current values are read before the write transaction is opened
    // so that an unchanged submission can be accepted without starting
    // (and then having to unwind) a transaction.
    QSqlQuery current(database);

    current.prepare(
        "SELECT case_status, assigned_to, notes "
        "FROM investigations WHERE id = ?"
    );

    current.addBindValue(investigationId);

    if (!current.exec())
    {
        errorMessage = current.lastError().text();
        return false;
    }

    if (!current.next())
    {
        errorMessage = "Investigation case not found.";
        return false;
    }

    const QString previousStatus = current.value(0).toString();
    const QString previousAssignee = current.value(1).toString();
    const QString previousNotes = current.value(2).toString();

    current.finish();

    const bool statusChanged = previousStatus != caseStatus;
    const bool notesChanged = previousNotes != notes;
    const bool assigneeChanged = previousAssignee != assignedTo;

    if (!statusChanged && !notesChanged && !assigneeChanged)
        return true;

    if (!database.transaction())
    {
        errorMessage = database.lastError().text();
        return false;
    }

    QSqlQuery update(database);

    update.prepare(
        "UPDATE investigations SET "
        "case_status = ?, assigned_to = ?, notes = ?, "
        "closed_at = CASE WHEN ? THEN CURRENT_TIMESTAMP ELSE NULL END "
        "WHERE id = ?"
    );

    update.addBindValue(caseStatus);
    update.addBindValue(assignedTo);
    update.addBindValue(notes);
    update.addBindValue(isClosedStatus(caseStatus) ? 1 : 0);
    update.addBindValue(investigationId);

    if (!update.exec())
    {
        errorMessage = update.lastError().text();
        database.rollback();
        return false;
    }

    QString eventType = "updated";

    if (statusChanged)
        eventType = "status_changed";
    else if (notesChanged)
        eventType = "note_added";

    QSqlQuery event(database);

    event.prepare(
        "INSERT INTO investigation_events "
        "(investigation_id, event_type, previous_status, "
        "new_status, notes) "
        "VALUES (?, ?, ?, ?, ?)"
    );

    event.addBindValue(investigationId);
    event.addBindValue(eventType);
    event.addBindValue(previousStatus);
    event.addBindValue(caseStatus);
    event.addBindValue(notes);

    if (!event.exec())
    {
        errorMessage = event.lastError().text();
        database.rollback();
        return false;
    }

    if (!database.commit())
    {
        errorMessage = database.lastError().text();
        database.rollback();
        return false;
    }

    return true;
}

QList<InvestigationEvent> InvestigationRepository::getHistory(
    int investigationId)
{
    QList<InvestigationEvent> events;
    errorMessage.clear();

    QSqlDatabase database =
        QSqlDatabase::database("FTADConnection");

    if (!database.isOpen())
    {
        errorMessage = "Database is not connected.";
        return events;
    }

    QSqlQuery query(database);

    query.prepare(
        "SELECT id, event_type, previous_status, new_status, "
        "notes, created_at "
        "FROM investigation_events "
        "WHERE investigation_id = ? "
        "ORDER BY id ASC"
    );

    query.addBindValue(investigationId);

    if (!query.exec())
    {
        errorMessage = query.lastError().text();
        return events;
    }

    while (query.next())
    {
        InvestigationEvent event;

        event.id = query.value(0).toInt();
        event.eventType = query.value(1).toString();
        event.previousStatus = query.value(2).toString();
        event.newStatus = query.value(3).toString();
        event.notes = query.value(4).toString();
        event.createdAt = query.value(5).toString();

        events.append(event);
    }

    return events;
}

QString InvestigationRepository::lastError() const
{
    return errorMessage;
}
