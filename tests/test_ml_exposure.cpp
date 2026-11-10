
#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QTemporaryDir>
#include "DetectionService.h"
#include "DetectionSettingsStore.h"
#include "TransactionRepository.h"
#include "support/TestSupport.h"

class MlExposureTest : public DatabaseTest
{
protected:
    void SetUp() override
    {
        DatabaseTest::SetUp();
        QCoreApplication::setOrganizationName("FTAD-Test");
        QCoreApplication::setApplicationName("FTAD-Test-ML-Exposure");
        QSettings store; store.clear();
    }
    void TearDown() override
    {
        QSettings store; store.clear();
        QCoreApplication::setOrganizationName(QString());
        QCoreApplication::setApplicationName(QString());
        DatabaseTest::TearDown();
    }
};

TEST_F(MlExposureTest, DetectionServiceUsesStoredMlSetting)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QCoreApplication::setOrganizationName("FTAD-Test");
    QCoreApplication::setApplicationName("FTAD-Test-" + QDir(dir.path()).dirName());
    QSettings store;
    store.clear();

    DetectionSettings s;
    s.useRuleEngine = false;
    s.useMlModel = true;
    ASSERT_TRUE(DetectionSettingsStore::save(s));

    TransactionRepository repo;
    ASSERT_TRUE(repo.addTransaction(makeTransaction(
        "TXNXXX1","2026-10-30 10:00:00","ACCX800","ACCX801","TRANSFER",250000.0)));

    DetectionService service;
    int analyzed=0, flagged=0;
    ASSERT_TRUE(service.analyzeAll(&analyzed, &flagged)) << service.lastError().toStdString();
    EXPECT_EQ(analyzed, 1);
    EXPECT_GE(flagged, 1);
    auto results = service.flaggedResults();
    ASSERT_FALSE(results.isEmpty());
    EXPECT_TRUE(results.first().methods.contains("ml_model"));
}

TEST_F(MlExposureTest, DefaultDetectionServiceRemainsRuleBased)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QCoreApplication::setOrganizationName("FTAD-Test");
    QCoreApplication::setApplicationName("FTAD-Test-default-" + QDir(dir.path()).dirName());
    QSettings store; store.clear();

    TransactionRepository repo;
    ASSERT_TRUE(repo.addTransaction(makeTransaction(
        "TXNXXX1","2026-10-30 10:00:00","ACCX800","ACCX801","TRANSFER",250000.0)));

    DetectionService service;
    ASSERT_TRUE(service.analyzeAll());
    auto results = service.flaggedResults();
    ASSERT_FALSE(results.isEmpty());
    EXPECT_FALSE(results.first().methods.contains("ml_model"));
    EXPECT_TRUE(results.first().methods.contains("large_amount"));
}
