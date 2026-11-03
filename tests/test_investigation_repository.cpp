
// Integration tests for InvestigationRepository.
//
// Cases are the reviewer-facing half of the system: every anomaly gets
// a case, every decision is recorded, and the history must stay
// complete and ordered.

#include <gtest/gtest.h>

#include <QList>
#include <QString>

#include "Investigation.h"
#include "InvestigationRepository.h"
#include "support/TestSupport.h"

class InvestigationTest : public DatabaseTest
{
protected:
    void SetUp() override
    {
        DatabaseTest::SetUp();

        // Two anomalies, each with a transaction behind it.
        firstAnomalyId = insertAnomaly("TXNXXX1", 40.0, "medium");
        secondAnomalyId = insertAnomaly("TXNXXX2", 75.0, "high");
    }

    int insertAnomaly(
        const QString &reference,
        double score,
        const QString &level)
    {
        exec(QString(
            "INSERT INTO transactions "
            "(transaction_id, timestamp, sender_id, receiver_id, "
            "transaction_type, amount, currency, status) "
            "VALUES ('%1', '2026-10-30 10:00:00', 'ACCX101', "
            "'ACCX202', 'TRANSFER', 500.0, 'LSL', 'completed')")
            .arg(reference));

        const int transactionId = scalar(
            "SELECT id FROM transactions WHERE transaction_id = '" +
            reference + "'");

        exec(QString(
            "INSERT INTO anomalies "
            "(transaction_id, detection_method, risk_score, "
            "risk_level, reason) "
            "VALUES (%1, 'duplicate_payment', %2, '%3', 'test')")
            .arg(transactionId)
            .arg(score)
            .arg(level));

        return scalar("SELECT MAX(id) FROM anomalies");
    }

    int firstAnomalyId = 0;
    int secondAnomalyId = 0;
};

TEST_F(InvestigationTest, CreatesOneCasePerAnomaly)
{
    InvestigationRepository repository;

    EXPECT_EQ(repository.ensureCases(), 2);

    const QList<InvestigationCase> cases = repository.getCases();

    ASSERT_EQ(cases.size(), 2);
    EXPECT_EQ(cases.at(0).caseStatus, "pending");
    EXPECT_EQ(cases.at(1).caseStatus, "pending");
}

TEST_F(InvestigationTest, CreatingCasesIsIdempotent)
{
    InvestigationRepository repository;

    ASSERT_EQ(repository.ensureCases(), 2);
    EXPECT_EQ(repository.ensureCases(), 0);
    EXPECT_EQ(repository.ensureCases(), 0);

    EXPECT_EQ(repository.getCases().size(), 2);
}

TEST_F(InvestigationTest, NewCasesStartWithAnOpenedEvent)
{
    InvestigationRepository repository;
    repository.ensureCases();

    const InvestigationCase item = repository.getCases().first();
    const QList<InvestigationEvent> history =
        repository.getHistory(item.investigationId);

    ASSERT_EQ(history.size(), 1);
    EXPECT_EQ(history.first().eventType, "opened");
    EXPECT_EQ(history.first().newStatus, "pending");
    EXPECT_TRUE(history.first().previousStatus.isEmpty());
    EXPECT_FALSE(item.openedAt.isEmpty());
    EXPECT_TRUE(item.closedAt.isEmpty());
}

TEST_F(InvestigationTest, RecordsAStatusChange)
{
    InvestigationRepository repository;
    repository.ensureCases();

    const int caseId = repository.getCases().first().investigationId;

    ASSERT_TRUE(repository.updateCase(
        caseId, "in_progress", "T. Mokoena", "Checking the receipt."))
        << repository.lastError().toStdString();

    const QList<InvestigationEvent> history =
        repository.getHistory(caseId);

    ASSERT_EQ(history.size(), 2);
    EXPECT_EQ(history.last().eventType, "status_changed");
    EXPECT_EQ(history.last().previousStatus, "pending");
    EXPECT_EQ(history.last().newStatus, "in_progress");

    bool found = false;

    for (const InvestigationCase &item : repository.getCases())
    {
        if (item.investigationId == caseId)
        {
            found = true;
            EXPECT_EQ(item.caseStatus, "in_progress");
            EXPECT_EQ(item.assignedTo, "T. Mokoena");
            EXPECT_EQ(item.notes, "Checking the receipt.");
        }
    }

    EXPECT_TRUE(found);
}

TEST_F(InvestigationTest, SavingAnUnchangedCaseAddsNoEvent)
{
    InvestigationRepository repository;
    repository.ensureCases();

    const int caseId = repository.getCases().first().investigationId;

    ASSERT_TRUE(repository.updateCase(
        caseId, "in_progress", "T. Mokoena", "Checking."));

    const int before = repository.getHistory(caseId).size();

    EXPECT_TRUE(repository.updateCase(
        caseId, "in_progress", "T. Mokoena", "Checking."));

    EXPECT_EQ(repository.getHistory(caseId).size(), before);
}

TEST_F(InvestigationTest, AnUnchangedSaveDoesNotBlockTheNextSave)
{
    // Regression test: the no-op path used to return without closing
    // its transaction, so the following save failed with
    // "cannot start a transaction within a transaction".
    InvestigationRepository repository;
    repository.ensureCases();

    const int caseId = repository.getCases().first().investigationId;

    ASSERT_TRUE(repository.updateCase(
        caseId, "in_progress", "T. Mokoena", "Checking."));

    EXPECT_TRUE(repository.updateCase(
        caseId, "in_progress", "T. Mokoena", "Checking."));

    EXPECT_TRUE(repository.updateCase(
        caseId, "resolved", "T. Mokoena", "Confirmed."))
        << repository.lastError().toStdString();
}

TEST_F(InvestigationTest, ResolvingRecordsTheClosingTime)
{
    InvestigationRepository repository;
    repository.ensureCases();

    const int caseId = repository.getCases().first().investigationId;

    ASSERT_TRUE(repository.updateCase(
        caseId, "resolved", "N. Dlamini", "Refund authorised."));

    for (const InvestigationCase &item : repository.getCases())
    {
        if (item.investigationId == caseId)
        {
            EXPECT_EQ(item.caseStatus, "resolved");
            EXPECT_FALSE(item.closedAt.isEmpty());
        }
    }
}

TEST_F(InvestigationTest, DismissingAlsoClosesTheCase)
{
    InvestigationRepository repository;
    repository.ensureCases();

    const int caseId = repository.getCases().first().investigationId;

    ASSERT_TRUE(repository.updateCase(
        caseId, "dismissed", "N. Dlamini", "Normal business payment."));

    for (const InvestigationCase &item : repository.getCases())
    {
        if (item.investigationId == caseId)
        {
            EXPECT_EQ(item.caseStatus, "dismissed");
            EXPECT_FALSE(item.closedAt.isEmpty());
        }
    }
}

TEST_F(InvestigationTest, ReopeningClearsTheClosingTime)
{
    InvestigationRepository repository;
    repository.ensureCases();

    const int caseId = repository.getCases().first().investigationId;

    ASSERT_TRUE(repository.updateCase(
        caseId, "resolved", "N. Dlamini", "Refund authorised."));

    ASSERT_TRUE(repository.updateCase(
        caseId, "in_progress", "N. Dlamini", "Customer disputed."));

    for (const InvestigationCase &item : repository.getCases())
    {
        if (item.investigationId == caseId)
        {
            EXPECT_EQ(item.caseStatus, "in_progress");
            EXPECT_TRUE(item.closedAt.isEmpty());
        }
    }
}

TEST_F(InvestigationTest, RejectsAnInvalidStatus)
{
    InvestigationRepository repository;
    repository.ensureCases();

    const int caseId = repository.getCases().first().investigationId;

    EXPECT_FALSE(repository.updateCase(
        caseId, "archived", "", ""));

    EXPECT_TRUE(repository.lastError().contains("Invalid case status"));
}

TEST_F(InvestigationTest, ReportsAnUnknownCase)
{
    InvestigationRepository repository;

    EXPECT_FALSE(repository.updateCase(
        9999, "resolved", "", ""));

    EXPECT_TRUE(repository.lastError().contains("not found"));
}

TEST_F(InvestigationTest, NotesOnlyChangeIsRecordedAsANote)
{
    InvestigationRepository repository;
    repository.ensureCases();

    const int caseId = repository.getCases().first().investigationId;

    ASSERT_TRUE(repository.updateCase(
        caseId, "pending", "", "First note."));
    ASSERT_TRUE(repository.updateCase(
        caseId, "pending", "", "Second note."));

    const QList<InvestigationEvent> history =
        repository.getHistory(caseId);

    ASSERT_EQ(history.size(), 3);
    EXPECT_EQ(history.last().eventType, "note_added");
    EXPECT_EQ(history.last().notes, "Second note.");
}

TEST_F(InvestigationTest, AssigneeOnlyChangeIsRecordedAsAnUpdate)
{
    InvestigationRepository repository;
    repository.ensureCases();

    const int caseId = repository.getCases().first().investigationId;

    ASSERT_TRUE(repository.updateCase(
        caseId, "pending", "T. Mokoena", ""));

    const QList<InvestigationEvent> history =
        repository.getHistory(caseId);

    ASSERT_EQ(history.size(), 2);
    EXPECT_EQ(history.last().eventType, "updated");
    EXPECT_EQ(history.last().previousStatus, "pending");
    EXPECT_EQ(history.last().newStatus, "pending");
}

TEST_F(InvestigationTest, ListsOpenCasesBeforeClosedOnes)
{
    InvestigationRepository repository;
    repository.ensureCases();

    QList<InvestigationCase> cases = repository.getCases();

    ASSERT_EQ(cases.size(), 2);

    // Close the first case, then confirm it moves behind the open one.
    ASSERT_TRUE(repository.updateCase(
        cases.at(0).investigationId, "resolved", "", "Done."));

    cases = repository.getCases();

    ASSERT_EQ(cases.size(), 2);
    EXPECT_EQ(cases.at(0).caseStatus, "pending");
    EXPECT_EQ(cases.at(1).caseStatus, "resolved");
}

TEST_F(InvestigationTest, OrdersCasesByRiskWithinTheSameStatus)
{
    InvestigationRepository repository;
    repository.ensureCases();

    const QList<InvestigationCase> cases = repository.getCases();

    ASSERT_EQ(cases.size(), 2);

    // The high-risk anomaly (75) must come before the medium one (40).
    EXPECT_GE(cases.at(0).riskScore, cases.at(1).riskScore);
    EXPECT_EQ(cases.at(0).riskLevel, "high");
}

TEST_F(InvestigationTest, JoinsTransactionDetailsIntoTheCase)
{
    InvestigationRepository repository;
    repository.ensureCases();

    const InvestigationCase item = repository.getCases().first();

    EXPECT_EQ(item.transactionReference, "TXNXXX2");
    EXPECT_EQ(item.senderId, "ACCX101");
    EXPECT_EQ(item.receiverId, "ACCX202");
    EXPECT_DOUBLE_EQ(item.amount, 500.0);
    EXPECT_EQ(item.currency, "LSL");
    EXPECT_EQ(item.anomalyId, secondAnomalyId);
    EXPECT_TRUE(item.isOpen());
}

TEST_F(InvestigationTest, KeepsHistoryInChronologicalOrder)
{
    InvestigationRepository repository;
    repository.ensureCases();

    const int caseId = repository.getCases().first().investigationId;

    ASSERT_TRUE(repository.updateCase(caseId, "in_progress", "", "a"));
    ASSERT_TRUE(repository.updateCase(caseId, "resolved", "", "b"));
    ASSERT_TRUE(repository.updateCase(caseId, "dismissed", "", "c"));

    const QList<InvestigationEvent> history =
        repository.getHistory(caseId);

    ASSERT_EQ(history.size(), 4);

    for (int i = 1; i < history.size(); ++i)
        EXPECT_LT(history.at(i - 1).id, history.at(i).id);
}

TEST_F(InvestigationTest, ValidatesStatusNames)
{
    EXPECT_TRUE(InvestigationRepository::isValidStatus("pending"));
    EXPECT_TRUE(InvestigationRepository::isValidStatus("in_progress"));
    EXPECT_TRUE(InvestigationRepository::isValidStatus("resolved"));
    EXPECT_TRUE(InvestigationRepository::isValidStatus("dismissed"));

    EXPECT_FALSE(InvestigationRepository::isValidStatus(""));
    EXPECT_FALSE(InvestigationRepository::isValidStatus("Pending"));
    EXPECT_FALSE(InvestigationRepository::isValidStatus("closed"));
}
