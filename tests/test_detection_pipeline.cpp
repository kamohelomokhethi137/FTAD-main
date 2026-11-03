
// End-to-end tests for the detection pipeline: transactions in the
// database, results out of it, and the guarantee that re-analysing
// never destroys review work.

#include <gtest/gtest.h>

#include <QList>
#include <QString>

#include "DetectionResult.h"
#include "DetectionService.h"
#include "DetectionSettings.h"
#include "Investigation.h"
#include "InvestigationRepository.h"
#include "Transaction.h"
#include "TransactionRepository.h"
#include "support/TestSupport.h"

class DetectionPipelineTest : public DatabaseTest
{
protected:
    void SetUp() override
    {
        DatabaseTest::SetUp();

        TransactionRepository repository;

        // A duplicate pair.
        ASSERT_TRUE(repository.addTransaction(makeTransaction(
            "TXNXXX1", "2026-10-30 10:00:00",
            "ACCX800", "ACCX801", "TRANSFER", 500.0)));
        ASSERT_TRUE(repository.addTransaction(makeTransaction(
            "TXNXXX2", "2026-10-30 10:02:00",
            "ACCX800", "ACCX801", "TRANSFER", 500.0)));

        // A large transfer.
        ASSERT_TRUE(repository.addTransaction(makeTransaction(
            "TXNXXX3", "2026-10-30 11:00:00",
            "ACCX802", "ACCX803", "TRANSFER", 15000.0)));
    }

    static const DetectionResult *find(
        const QList<DetectionResult> &results,
        const QString &reference)
    {
        for (const DetectionResult &result : results)
        {
            if (result.transactionReference == reference)
                return &result;
        }

        return nullptr;
    }
};

TEST_F(DetectionPipelineTest, AnalysesEveryStoredTransaction)
{
    DetectionService service;

    int analyzed = 0;
    int flagged = 0;

    ASSERT_TRUE(service.analyzeAll(&analyzed, &flagged))
        << service.lastError().toStdString();

    EXPECT_EQ(analyzed, 3);
    EXPECT_EQ(flagged, 3);
}

TEST_F(DetectionPipelineTest, StoresOneRowPerFlaggedTransaction)
{
    DetectionService service;
    service.analyzeAll();

    EXPECT_EQ(scalar("SELECT COUNT(*) FROM anomalies"), 3);

    const QList<DetectionResult> results = service.flaggedResults();

    EXPECT_EQ(results.size(), 3);
    EXPECT_TRUE(service.lastError().isEmpty());
}

TEST_F(DetectionPipelineTest, LoadedResultsCarryTransactionDetails)
{
    DetectionService service;
    service.analyzeAll();

    const QList<DetectionResult> results = service.flaggedResults();
    const DetectionResult *large = find(results, "TXNXXX3");

    ASSERT_NE(large, nullptr);
    EXPECT_EQ(large->senderId, "ACCX802");
    EXPECT_EQ(large->receiverId, "ACCX803");
    EXPECT_DOUBLE_EQ(large->amount, 15000.0);
    EXPECT_EQ(large->currency, "LSL");
    EXPECT_EQ(large->riskLevel, "medium");
    EXPECT_TRUE(large->methods.contains("large_amount"));
    EXPECT_FALSE(large->reasonText().isEmpty());
}

TEST_F(DetectionPipelineTest, PrimaryMethodIsTheHeaviestCheck)
{
    DetectionService service;
    service.analyzeAll();

    const QList<DetectionResult> results = service.flaggedResults();
    const DetectionResult *duplicate = find(results, "TXNXXX2");

    ASSERT_NE(duplicate, nullptr);
    EXPECT_EQ(duplicate->methods.size(), 1);
    EXPECT_EQ(duplicate->primaryMethod, "duplicate_payment");
}

TEST_F(DetectionPipelineTest, ReanalysisDoesNotDuplicateRows)
{
    DetectionService service;

    ASSERT_TRUE(service.analyzeAll());
    ASSERT_TRUE(service.analyzeAll());
    ASSERT_TRUE(service.analyzeAll());

    EXPECT_EQ(scalar("SELECT COUNT(*) FROM anomalies"), 3);
}

TEST_F(DetectionPipelineTest, ReanalysisKeepsAnomalyIdsStable)
{
    DetectionService service;
    service.analyzeAll();

    const int before = scalar(
        "SELECT a.id FROM anomalies a "
        "JOIN transactions t ON t.id = a.transaction_id "
        "WHERE t.transaction_id = 'TXNXXX3'");

    service.analyzeAll();

    const int after = scalar(
        "SELECT a.id FROM anomalies a "
        "JOIN transactions t ON t.id = a.transaction_id "
        "WHERE t.transaction_id = 'TXNXXX3'");

    EXPECT_GT(before, 0);
    EXPECT_EQ(before, after);
}

TEST_F(DetectionPipelineTest, ReanalysisPreservesInvestigationWork)
{
    DetectionService service;
    service.analyzeAll();

    InvestigationRepository investigations;
    ASSERT_EQ(investigations.ensureCases(), 3);

    const int caseId = investigations.getCases().first().investigationId;

    ASSERT_TRUE(investigations.updateCase(
        caseId, "in_progress", "T. Mokoena", "Reviewing."));

    service.analyzeAll();

    const QList<InvestigationCase> cases = investigations.getCases();

    ASSERT_EQ(cases.size(), 3);

    bool found = false;

    for (const InvestigationCase &item : cases)
    {
        if (item.investigationId == caseId)
        {
            found = true;
            EXPECT_EQ(item.caseStatus, "in_progress");
            EXPECT_EQ(item.notes, "Reviewing.");
        }
    }

    EXPECT_TRUE(found);
}

TEST_F(DetectionPipelineTest, CorrectingATransactionRemovesTheStaleFinding)
{
    DetectionService service;
    service.analyzeAll();

    ASSERT_EQ(scalar("SELECT COUNT(*) FROM anomalies"), 3);

    exec("UPDATE transactions SET amount = 100.0 "
         "WHERE transaction_id = 'TXNXXX3'");

    int flagged = 0;
    ASSERT_TRUE(service.analyzeAll(nullptr, &flagged));

    EXPECT_EQ(flagged, 2);
    EXPECT_EQ(scalar("SELECT COUNT(*) FROM anomalies"), 2);
    EXPECT_EQ(scalar(
        "SELECT COUNT(*) FROM anomalies a "
        "JOIN transactions t ON t.id = a.transaction_id "
        "WHERE t.transaction_id = 'TXNXXX3'"), 0);
}

TEST_F(DetectionPipelineTest, OrdersResultsByRiskDescending)
{
    DetectionService service;
    service.analyzeAll();

    const QList<DetectionResult> results = service.flaggedResults();

    ASSERT_GE(results.size(), 2);

    for (int i = 1; i < results.size(); ++i)
    {
        EXPECT_GE(results.at(i - 1).riskScore, results.at(i).riskScore);
    }
}

TEST_F(DetectionPipelineTest, SucceedsWithNoTransactions)
{
    exec("DELETE FROM transactions");

    DetectionService service;

    int analyzed = 0;
    int flagged = 0;

    EXPECT_TRUE(service.analyzeAll(&analyzed, &flagged));
    EXPECT_EQ(analyzed, 0);
    EXPECT_EQ(flagged, 0);
    EXPECT_TRUE(service.flaggedResults().isEmpty());
}

TEST_F(DetectionPipelineTest, RejectsInvalidSettings)
{
    DetectionSettings settings;
    settings.largeAmountAbsolute = -1.0;

    DetectionService service(settings);

    int analyzed = 0;

    EXPECT_FALSE(service.analyzeAll(&analyzed));
    EXPECT_TRUE(service.lastError().contains("Large amount"));
    EXPECT_EQ(analyzed, 0);
    EXPECT_EQ(scalar("SELECT COUNT(*) FROM anomalies"), 0);
}

TEST_F(DetectionPipelineTest, AcceptsTunedSettings)
{
    DetectionSettings settings;
    settings.largeAmountAbsolute = 100.0;

    DetectionService service(settings);

    int flagged = 0;
    ASSERT_TRUE(service.analyzeAll(nullptr, &flagged))
        << service.lastError().toStdString();

    // Every transaction now exceeds the M 100 limit.
    EXPECT_EQ(flagged, 3);
    EXPECT_EQ(service.configuration().largeAmountAbsolute, 100.0);
}

TEST_F(DetectionPipelineTest, ClearingResultsEmptiesTheTable)
{
    DetectionService service;
    service.analyzeAll();

    ASSERT_EQ(scalar("SELECT COUNT(*) FROM anomalies"), 3);

    // Re-analysing after the transactions are gone removes every row.
    exec("DELETE FROM transactions");
    service.analyzeAll();

    EXPECT_EQ(scalar("SELECT COUNT(*) FROM anomalies"), 0);
}
