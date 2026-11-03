
// Integration tests for TransactionRepository against a real SQLite
// database. Each test starts from an empty file (see DatabaseTest).

#include <gtest/gtest.h>

#include <QList>
#include <QString>

#include <cmath>
#include <limits>

#include "Transaction.h"
#include "TransactionRepository.h"
#include "support/TestSupport.h"

// Named fixture so the CTest output reads
// "TransactionRepositoryTest.<behaviour>" rather than the generic base.
class TransactionRepositoryTest : public DatabaseTest
{
};

namespace
{
    Transaction validTransaction()
    {
        return makeTransaction(
            "", "2026-10-30 10:30:00",
            "ACCX101", "ACCX202", "TRANSFER", 2500.0);
    }
}

TEST_F(TransactionRepositoryTest, AssignsTheLowestFreeIdWhenNoneIsSupplied)
{
    TransactionRepository repository;

    ASSERT_TRUE(repository.addTransaction(validTransaction()))
        << repository.lastError().toStdString();

    EXPECT_EQ(text("SELECT transaction_id FROM transactions"),
              "TXNXXX0");
}

TEST_F(TransactionRepositoryTest, HonoursASuppliedTransactionId)
{
    TransactionRepository repository;

    Transaction transaction = validTransaction();
    transaction.transactionId = "TXNXXX7";

    ASSERT_TRUE(repository.addTransaction(transaction))
        << repository.lastError().toStdString();

    EXPECT_EQ(text("SELECT transaction_id FROM transactions"),
              "TXNXXX7");
}

TEST_F(TransactionRepositoryTest, RejectsADuplicateSuppliedId)
{
    TransactionRepository repository;

    Transaction first = validTransaction();
    first.transactionId = "TXNXXX7";
    ASSERT_TRUE(repository.addTransaction(first));

    Transaction second = validTransaction();
    second.transactionId = "TXNXXX7";

    EXPECT_FALSE(repository.addTransaction(second));
    EXPECT_TRUE(repository.lastError().contains("Duplicate"));
    EXPECT_EQ(scalar("SELECT COUNT(*) FROM transactions"), 1);
}

TEST_F(TransactionRepositoryTest, FillsTheLowestFreeNumber)
{
    TransactionRepository repository;

    Transaction first = validTransaction();
    first.transactionId = "TXNXXX0";
    ASSERT_TRUE(repository.addTransaction(first));

    Transaction third = validTransaction();
    third.transactionId = "TXNXXX2";
    ASSERT_TRUE(repository.addTransaction(third));

    // TXNXXX1 is free, so it is allocated before TXNXXX3.
    EXPECT_EQ(repository.generateNextTransactionId(), "TXNXXX1");

    ASSERT_TRUE(repository.addTransaction(validTransaction()));
    EXPECT_EQ(scalar(
        "SELECT COUNT(*) FROM transactions "
        "WHERE transaction_id = 'TXNXXX1'"), 1);
}

TEST_F(TransactionRepositoryTest, RejectsAMalformedSuppliedId)
{
    TransactionRepository repository;

    Transaction transaction = validTransaction();
    transaction.transactionId = "TXN002";

    EXPECT_FALSE(repository.addTransaction(transaction));
    EXPECT_TRUE(
        repository.lastError().contains("Invalid transaction ID"));
    EXPECT_EQ(scalar("SELECT COUNT(*) FROM transactions"), 0);
}

TEST_F(TransactionRepositoryTest, NormalisesAccountIdentifiers)
{
    TransactionRepository repository;

    Transaction transaction = validTransaction();
    transaction.senderId = "ACC101";
    transaction.receiverId = "acc202";

    ASSERT_TRUE(repository.addTransaction(transaction))
        << repository.lastError().toStdString();

    EXPECT_EQ(text("SELECT sender_id FROM transactions"), "ACCX101");
    EXPECT_EQ(text("SELECT receiver_id FROM transactions"), "ACCX202");
}

TEST_F(TransactionRepositoryTest, RejectsAnInvalidSender)
{
    TransactionRepository repository;

    Transaction transaction = validTransaction();
    transaction.senderId = "ACC0";

    EXPECT_FALSE(repository.addTransaction(transaction));
    EXPECT_TRUE(repository.lastError().contains("Invalid sender"));
}

TEST_F(TransactionRepositoryTest, RejectsAnInvalidReceiver)
{
    TransactionRepository repository;

    Transaction transaction = validTransaction();
    transaction.receiverId = "not-an-account";

    EXPECT_FALSE(repository.addTransaction(transaction));
    EXPECT_TRUE(repository.lastError().contains("Invalid receiver"));
}

TEST_F(TransactionRepositoryTest, RejectsZeroAndNegativeAmounts)
{
    TransactionRepository repository;

    Transaction zero = validTransaction();
    zero.amount = 0.0;
    EXPECT_FALSE(repository.addTransaction(zero));
    EXPECT_TRUE(repository.lastError().contains("greater than zero"));

    Transaction negative = validTransaction();
    negative.amount = -100.0;
    EXPECT_FALSE(repository.addTransaction(negative));

    EXPECT_EQ(scalar("SELECT COUNT(*) FROM transactions"), 0);
}

TEST_F(TransactionRepositoryTest, RejectsNonFiniteAmounts)
{
    TransactionRepository repository;

    Transaction notANumber = validTransaction();
    notANumber.amount = std::numeric_limits<double>::quiet_NaN();
    EXPECT_FALSE(repository.addTransaction(notANumber));

    Transaction infinite = validTransaction();
    infinite.amount = std::numeric_limits<double>::infinity();
    EXPECT_FALSE(repository.addTransaction(infinite));

    EXPECT_EQ(scalar("SELECT COUNT(*) FROM transactions"), 0);
}

TEST_F(TransactionRepositoryTest, RejectsAnInvalidTimestamp)
{
    TransactionRepository repository;

    Transaction transaction = validTransaction();
    transaction.timestamp = "30/10/2026 10:30";

    EXPECT_FALSE(repository.addTransaction(transaction));
    EXPECT_TRUE(repository.lastError().contains("date and time"));
}

TEST_F(TransactionRepositoryTest, RejectsAnUnsupportedCurrency)
{
    TransactionRepository repository;

    Transaction transaction = validTransaction();
    transaction.currency = "GBP";

    EXPECT_FALSE(repository.addTransaction(transaction));
    EXPECT_TRUE(repository.lastError().contains("currency"));
}

TEST_F(TransactionRepositoryTest, RejectsAnInvalidStatus)
{
    TransactionRepository repository;

    Transaction transaction = validTransaction();
    transaction.status = "archived";

    EXPECT_FALSE(repository.addTransaction(transaction));
    EXPECT_TRUE(repository.lastError().contains("status"));
}

TEST_F(TransactionRepositoryTest, RejectsAnEmptyTransactionType)
{
    TransactionRepository repository;

    Transaction transaction = validTransaction();
    transaction.transactionType = "   ";

    EXPECT_FALSE(repository.addTransaction(transaction));
    EXPECT_TRUE(repository.lastError().contains("type"));
}

TEST_F(TransactionRepositoryTest, ReturnsNewestFirst)
{
    TransactionRepository repository;

    Transaction first = validTransaction();
    first.transactionId = "TXNXXX1";
    first.amount = 100.0;
    ASSERT_TRUE(repository.addTransaction(first));

    Transaction second = validTransaction();
    second.transactionId = "TXNXXX2";
    second.amount = 200.0;
    ASSERT_TRUE(repository.addTransaction(second));

    const QList<Transaction> transactions =
        repository.getAllTransactions();

    ASSERT_EQ(transactions.size(), 2);
    EXPECT_EQ(transactions.first().transactionId, "TXNXXX2");
    EXPECT_DOUBLE_EQ(transactions.first().amount, 200.0);
    EXPECT_EQ(transactions.last().transactionId, "TXNXXX1");
}

TEST_F(TransactionRepositoryTest, RoundTripsEveryStoredField)
{
    TransactionRepository repository;

    Transaction transaction = makeTransaction(
        "TXNXXX5", "2026-11-01 08:45:12",
        "ACCX123", "ACCX456", "PAYMENT", 1234.56);
    transaction.currency = "ZAR";
    transaction.status = "pending";

    ASSERT_TRUE(repository.addTransaction(transaction))
        << repository.lastError().toStdString();

    const QList<Transaction> stored = repository.getAllTransactions();

    ASSERT_EQ(stored.size(), 1);
    EXPECT_EQ(stored.first().transactionId, "TXNXXX5");
    EXPECT_EQ(stored.first().timestamp, "2026-11-01 08:45:12");
    EXPECT_EQ(stored.first().senderId, "ACCX123");
    EXPECT_EQ(stored.first().receiverId, "ACCX456");
    EXPECT_EQ(stored.first().transactionType, "PAYMENT");
    EXPECT_NEAR(stored.first().amount, 1234.56, 1.0e-9);
    EXPECT_EQ(stored.first().currency, "ZAR");
    EXPECT_EQ(stored.first().status, "pending");
}

TEST_F(TransactionRepositoryTest, AllowsRepeatedContentBecauseDetectionHandlesIt)
{
    // Two identical payments are legitimate data; deciding that they
    // look like a duplicate is the detector's job, not the store's.
    TransactionRepository repository;

    ASSERT_TRUE(repository.addTransaction(validTransaction()));
    ASSERT_TRUE(repository.addTransaction(validTransaction()));

    EXPECT_EQ(scalar("SELECT COUNT(*) FROM transactions"), 2);
}

TEST_F(TransactionRepositoryTest, ReportsAnErrorForAnEmptyDatabase)
{
    TransactionRepository repository;

    EXPECT_TRUE(repository.getAllTransactions().isEmpty());
    EXPECT_TRUE(repository.lastError().isEmpty());
}
