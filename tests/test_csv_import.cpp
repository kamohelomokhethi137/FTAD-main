
// Integration tests for CsvImportService.
//
// The importer is the main way data enters the system, so these tests
// cover both the happy path and the malformed files that used to be
// accepted silently.

#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QString>
#include <QStringList>

#include "CsvImportService.h"
#include "support/TestSupport.h"

namespace
{
    const char *const kHeader =
        "transaction_id,timestamp,sender_id,receiver_id,"
        "transaction_type,amount,currency,status\n";

    const char *const kValidRow =
        "TXNXXX2,2026-10-30 10:30:00,ACCX101,ACCX202,"
        "TRANSFER,2500.00,LSL,completed\n";
}

class CsvImportTest : public DatabaseTest
{
protected:
    QString writeCsv(const QString &content, const QString &name)
    {
        const QString path = QDir::tempPath() + "/" + name;

        QFile file(path);
        EXPECT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write(content.toUtf8());
        file.close();

        csvFiles.append(path);

        return path;
    }

    void TearDown() override
    {
        for (const QString &path : csvFiles)
            QFile::remove(path);

        csvFiles.clear();

        DatabaseTest::TearDown();
    }

    QStringList csvFiles;
};

TEST_F(CsvImportTest, ImportsTheSampleFile)
{
    CsvImportService importer;

    const CsvImportResult result =
        importer.importFile(QString(FTAD_SAMPLE_CSV));

    EXPECT_EQ(result.imported, 11);
    EXPECT_EQ(result.skipped, 0);
    EXPECT_EQ(result.failed, 0);
    EXPECT_EQ(scalar("SELECT COUNT(*) FROM transactions"), 11);
}

TEST_F(CsvImportTest, ReimportingTheSameFileSkipsEveryRow)
{
    CsvImportService importer;

    ASSERT_EQ(importer.importFile(QString(FTAD_SAMPLE_CSV)).imported, 11);

    const CsvImportResult second =
        importer.importFile(QString(FTAD_SAMPLE_CSV));

    EXPECT_EQ(second.imported, 0);
    EXPECT_EQ(second.skipped, 11);
    EXPECT_EQ(second.failed, 0);
    EXPECT_EQ(scalar("SELECT COUNT(*) FROM transactions"), 11);
}

TEST_F(CsvImportTest, ImportsAValidRow)
{
    const QString path = writeCsv(
        QString(kHeader) + kValidRow, "ftad_valid.csv");

    CsvImportService importer;
    const CsvImportResult result = importer.importFile(path);

    EXPECT_EQ(result.imported, 1);
    EXPECT_EQ(result.failed, 0);
    EXPECT_EQ(text("SELECT transaction_id FROM transactions"), "TXNXXX2");
    EXPECT_EQ(text("SELECT sender_id FROM transactions"), "ACCX101");
}

TEST_F(CsvImportTest, NormalisesPlainIdentifiersFromTheFile)
{
    const QString path = writeCsv(
        QString(kHeader) +
        "TXN2,2026-10-30 10:30:00,ACC101,acc202,"
        "TRANSFER,100.00,LSL,completed\n",
        "ftad_plain_ids.csv");

    CsvImportService importer;
    const CsvImportResult result = importer.importFile(path);

    EXPECT_EQ(result.imported, 1);
    EXPECT_EQ(text("SELECT transaction_id FROM transactions"), "TXNXXX2");
    EXPECT_EQ(text("SELECT sender_id FROM transactions"), "ACCX101");
    EXPECT_EQ(text("SELECT receiver_id FROM transactions"), "ACCX202");
}

TEST_F(CsvImportTest, ReportsAnUnreadableFile)
{
    CsvImportService importer;

    const CsvImportResult result =
        importer.importFile("/nonexistent/ftad.csv");

    EXPECT_EQ(result.imported, 0);
    EXPECT_EQ(result.failed, 1);
    EXPECT_TRUE(result.details.contains("Unable to open"));
}

TEST_F(CsvImportTest, ReportsAnEmptyFile)
{
    const QString path = writeCsv("", "ftad_empty.csv");

    CsvImportService importer;
    const CsvImportResult result = importer.importFile(path);

    EXPECT_EQ(result.failed, 1);
    EXPECT_TRUE(result.details.contains("empty"));
}

TEST_F(CsvImportTest, RejectsAMissingColumn)
{
    const QString path = writeCsv(
        "transaction_id,timestamp,sender_id,receiver_id,"
        "transaction_type,currency,status\n"
        "TXNXXX2,2026-10-30 10:30:00,ACCX101,ACCX202,"
        "TRANSFER,LSL,completed\n",
        "ftad_missing_column.csv");

    CsvImportService importer;
    const CsvImportResult result = importer.importFile(path);

    EXPECT_EQ(result.imported, 0);
    EXPECT_EQ(result.failed, 1);
    EXPECT_TRUE(result.details.contains("Missing CSV column: amount"));
    EXPECT_EQ(scalar("SELECT COUNT(*) FROM transactions"), 0);
}

TEST_F(CsvImportTest, RejectsADuplicateColumn)
{
    const QString path = writeCsv(
        "transaction_id,transaction_id,timestamp,sender_id,"
        "receiver_id,transaction_type,amount,currency,status\n"
        "TXNXXX2,TXNXXX2,2026-10-30 10:30:00,ACCX101,ACCX202,"
        "TRANSFER,2500.00,LSL,completed\n",
        "ftad_duplicate_column.csv");

    CsvImportService importer;
    const CsvImportResult result = importer.importFile(path);

    EXPECT_EQ(result.failed, 1);
    EXPECT_TRUE(result.details.contains("Duplicate CSV column"));
}

TEST_F(CsvImportTest, ReportsInvalidAmounts)
{
    const QString path = writeCsv(
        QString(kHeader) +
        "TXNXXX2,2026-10-30 10:30:00,ACCX101,ACCX202,"
        "TRANSFER,not-a-number,LSL,completed\n",
        "ftad_bad_amount.csv");

    CsvImportService importer;
    const CsvImportResult result = importer.importFile(path);

    EXPECT_EQ(result.imported, 0);
    EXPECT_EQ(result.failed, 1);
    EXPECT_TRUE(result.details.contains("Line 2"));
    EXPECT_EQ(scalar("SELECT COUNT(*) FROM transactions"), 0);
}

TEST_F(CsvImportTest, ReportsInvalidTimestamps)
{
    const QString path = writeCsv(
        QString(kHeader) +
        "TXNXXX2,30-10-2026 10:30,ACCX101,ACCX202,"
        "TRANSFER,2500.00,LSL,completed\n",
        "ftad_bad_timestamp.csv");

    CsvImportService importer;
    const CsvImportResult result = importer.importFile(path);

    EXPECT_EQ(result.failed, 1);
    EXPECT_TRUE(result.details.contains("Invalid timestamp"));
}

TEST_F(CsvImportTest, ReportsInvalidCurrencyOrStatus)
{
    const QString path = writeCsv(
        QString(kHeader) +
        "TXNXXX2,2026-10-30 10:30:00,ACCX101,ACCX202,"
        "TRANSFER,2500.00,GBP,completed\n",
        "ftad_bad_currency.csv");

    CsvImportService importer;
    const CsvImportResult result = importer.importFile(path);

    EXPECT_EQ(result.failed, 1);
    EXPECT_TRUE(result.details.contains("currency or status"));
}

TEST_F(CsvImportTest, ReportsInvalidTransactionIds)
{
    const QString path = writeCsv(
        QString(kHeader) +
        "TXN002,2026-10-30 10:30:00,ACCX101,ACCX202,"
        "TRANSFER,2500.00,LSL,completed\n",
        "ftad_bad_id.csv");

    CsvImportService importer;
    const CsvImportResult result = importer.importFile(path);

    EXPECT_EQ(result.failed, 1);
    EXPECT_TRUE(result.details.contains("Invalid transaction ID"));
}

TEST_F(CsvImportTest, ImportsValidRowsAndSkipsInvalidOnes)
{
    const QString path = writeCsv(
        QString(kHeader) + kValidRow +
        "TXNXXX3,2026-10-30 10:35:00,ACCX103,ACCX204,"
        "TRANSFER,abc,LSL,completed\n",
        "ftad_mixed.csv");

    CsvImportService importer;
    const CsvImportResult result = importer.importFile(path);

    EXPECT_EQ(result.imported, 1);
    EXPECT_EQ(result.failed, 1);
    EXPECT_EQ(scalar("SELECT COUNT(*) FROM transactions"), 1);
}

TEST_F(CsvImportTest, HandlesQuotedFields)
{
    const QString path = writeCsv(
        QString(kHeader) +
        "\"TXNXXX2\",\"2026-10-30 10:30:00\",\"ACCX101\","
        "\"ACCX202\",\"TRANSFER\",\"2500.00\",\"LSL\","
        "\"completed\"\n",
        "ftad_quoted.csv");

    CsvImportService importer;
    const CsvImportResult result = importer.importFile(path);

    EXPECT_EQ(result.imported, 1);
    EXPECT_EQ(result.failed, 0);
}

TEST_F(CsvImportTest, SkipsBlankLines)
{
    const QString path = writeCsv(
        QString(kHeader) + "\n" + kValidRow + "\n\n",
        "ftad_blank_lines.csv");

    CsvImportService importer;
    const CsvImportResult result = importer.importFile(path);

    EXPECT_EQ(result.imported, 1);
    EXPECT_EQ(result.failed, 0);
}

TEST_F(CsvImportTest, RejectsARowWithTooFewFields)
{
    const QString path = writeCsv(
        QString(kHeader) +
        "TXNXXX2,2026-10-30 10:30:00,ACCX101\n",
        "ftad_short_row.csv");

    CsvImportService importer;
    const CsvImportResult result = importer.importFile(path);

    EXPECT_EQ(result.failed, 1);
    EXPECT_TRUE(result.details.contains("Invalid CSV format"));
}
