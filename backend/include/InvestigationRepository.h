
#ifndef INVESTIGATIONREPOSITORY_H
#define INVESTIGATIONREPOSITORY_H

#include "Investigation.h"

#include <QList>
#include <QString>

// Creates, reads and updates review cases, keeping a history of every
// change so a reviewer's decisions are auditable.
class InvestigationRepository
{
public:
    // Creates a pending case for every anomaly that does not have one.
    // Returns the number of cases created, or -1 on error.
    int ensureCases();

    QList<InvestigationCase> getCases();

    // Records a decision. The case is closed when it is resolved or
    // dismissed, and reopened if the status returns to an open one.
    bool updateCase(
        int investigationId,
        const QString &caseStatus,
        const QString &assignedTo,
        const QString &notes);

    QList<InvestigationEvent> getHistory(int investigationId);

    static bool isValidStatus(const QString &caseStatus);

    QString lastError() const;

private:
    QString errorMessage;
};

#endif // INVESTIGATIONREPOSITORY_H
