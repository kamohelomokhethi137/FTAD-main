
// Tests for the identifier helpers in TransactionRepository.
//
// These are the rules that decide whether a CSV row or a manual entry
// is accepted, so they are tested independently of the database.

#include <gtest/gtest.h>

#include <QString>
#include <QStringList>

#include "TransactionRepository.h"

TEST(Identifiers, FormatsAccountsWithPadding)
{
    EXPECT_EQ(TransactionRepository::formatAccountId(1), "ACCXXX1");
    EXPECT_EQ(TransactionRepository::formatAccountId(101), "ACCX101");
    EXPECT_EQ(TransactionRepository::formatAccountId(9999), "ACC9999");
}

TEST(Identifiers, FormatsTransactionsWithPadding)
{
    EXPECT_EQ(TransactionRepository::formatTransactionId(0), "TXNXXX0");
    EXPECT_EQ(TransactionRepository::formatTransactionId(2), "TXNXXX2");
    EXPECT_EQ(TransactionRepository::formatTransactionId(9999), "TXN9999");
}

TEST(Identifiers, NormalisesCanonicalAccountIds)
{
    EXPECT_EQ(
        TransactionRepository::normalizeAccountId("ACCX101"),
        "ACCX101");

    EXPECT_EQ(
        TransactionRepository::normalizeAccountId("ACCXXX1"),
        "ACCXXX1");
}

TEST(Identifiers, NormalisesPlainAccountIds)
{
    EXPECT_EQ(
        TransactionRepository::normalizeAccountId("ACC101"),
        "ACCX101");

    EXPECT_EQ(
        TransactionRepository::normalizeAccountId("ACC1"),
        "ACCXXX1");
}

TEST(Identifiers, NormalisationIsCaseInsensitive)
{
    EXPECT_EQ(
        TransactionRepository::normalizeAccountId("acc101"),
        "ACCX101");

    EXPECT_EQ(
        TransactionRepository::normalizeTransactionId("txnxxx2"),
        "TXNXXX2");
}

TEST(Identifiers, NormalisationTrimsSurroundingWhitespace)
{
    EXPECT_EQ(
        TransactionRepository::normalizeAccountId("  ACC101  "),
        "ACCX101");

    EXPECT_EQ(
        TransactionRepository::normalizeTransactionId(" TXN2 "),
        "TXNXXX2");
}

TEST(Identifiers, RejectsAccountZero)
{
    // Account numbers start at one, so zero is never a real account.
    EXPECT_TRUE(
        TransactionRepository::normalizeAccountId("ACC0").isEmpty());
    EXPECT_TRUE(
        TransactionRepository::normalizeAccountId("ACCX0").isEmpty());
}

TEST(Identifiers, RejectsLeadingZeroes)
{
    // The formatter never produces a padded zero, so such an ID is a
    // typo rather than a canonical value.
    EXPECT_TRUE(
        TransactionRepository::normalizeAccountId("ACC0101").isEmpty());
    EXPECT_TRUE(
        TransactionRepository::normalizeAccountId("ACCX0101").isEmpty());
}

TEST(Identifiers, RejectsOutOfRangeNumbers)
{
    EXPECT_TRUE(
        TransactionRepository::normalizeAccountId("ACC10000").isEmpty());
    EXPECT_TRUE(
        TransactionRepository::normalizeTransactionId("TXN10000")
            .isEmpty());
}

TEST(Identifiers, RejectsMissingOrWrongPrefix)
{
    EXPECT_TRUE(
        TransactionRepository::normalizeAccountId("101").isEmpty());
    EXPECT_TRUE(
        TransactionRepository::normalizeAccountId("TXN101").isEmpty());
    EXPECT_TRUE(
        TransactionRepository::normalizeTransactionId("ACC101")
            .isEmpty());
}

TEST(Identifiers, RejectsEmptyAndMalformedValues)
{
    EXPECT_TRUE(TransactionRepository::normalizeAccountId("").isEmpty());
    EXPECT_TRUE(TransactionRepository::normalizeAccountId("   ").isEmpty());
    EXPECT_TRUE(TransactionRepository::normalizeAccountId("ACC").isEmpty());
    EXPECT_TRUE(TransactionRepository::normalizeAccountId("ACCX").isEmpty());
    EXPECT_TRUE(
        TransactionRepository::normalizeAccountId("ACCX10A").isEmpty());
    EXPECT_TRUE(
        TransactionRepository::normalizeAccountId("ACCX1011").isEmpty());
}

TEST(Identifiers, TransactionIdsAllowZeroButAccountsDoNot)
{
    EXPECT_EQ(
        TransactionRepository::normalizeTransactionId("TXNXXX0"),
        "TXNXXX0");

    EXPECT_TRUE(
        TransactionRepository::normalizeTransactionId("TXN").isEmpty());
}

TEST(Identifiers, ValidityPredicatesAgreeWithNormalisation)
{
    const QStringList accepted = {
        "ACCX101", "ACC101", "acc9999", "ACC1"
    };

    for (const QString &value : accepted)
    {
        EXPECT_TRUE(TransactionRepository::isValidAccountId(value))
            << value.toStdString();
    }

    const QStringList rejected = {
        "", "ACC", "ACC0", "ACC0101", "ACC10000", "TXN101"
    };

    for (const QString &value : rejected)
    {
        EXPECT_FALSE(TransactionRepository::isValidAccountId(value))
            << value.toStdString();
    }

    EXPECT_TRUE(
        TransactionRepository::isValidTransactionId("TXNXXX0"));
    EXPECT_TRUE(
        TransactionRepository::isValidTransactionId("TXN2"));
    EXPECT_FALSE(
        TransactionRepository::isValidTransactionId("TXN002"));
}

TEST(Identifiers, NormalisationIsIdempotent)
{
    const QStringList values = {
        "ACC1", "ACC101", "ACC9999", "ACCX101"
    };

    for (const QString &value : values)
    {
        const QString once =
            TransactionRepository::normalizeAccountId(value);
        const QString twice =
            TransactionRepository::normalizeAccountId(once);

        EXPECT_EQ(once, twice) << value.toStdString();
    }
}
