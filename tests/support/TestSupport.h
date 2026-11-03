
#ifndef FTAD_TESTSUPPORT_H
#define FTAD_TESTSUPPORT_H

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>

#include "DatabaseManager.h"
#include "Transaction.h"

// Base fixture for every test that touches the database.
//
// Each test gets a freshly created SQLite file, so tests never share
// rows and can be run in any order. The schema is applied through the
// production DatabaseManager, which also exercises schema parsing.
class DatabaseTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        QDir().mkpath(databaseFolder());
        QFile::remove(databaseFile());

        ASSERT_TRUE(manager.connect())
            << manager.lastError().toStdString();

        ASSERT_TRUE(manager.initializeSchema())
            << manager.lastError().toStdString();
    }

    void TearDown() override
    {
        manager.disconnect();
        QFile::remove(databaseFile());
    }

    static QString databaseFolder()
    {
        return QCoreApplication::applicationDirPath() + "/database";
    }

    static QString databaseFile()
    {
        return databaseFolder() + "/ftad.db";
    }

    // Executes a statement, failing the test when it does not run.
    static void exec(const QString &sql)
    {
        QSqlQuery query(QSqlDatabase::database("FTADConnection"));

        ASSERT_TRUE(query.exec(sql))
            << sql.toStdString() << ": "
            << query.lastError().text().toStdString();
    }

    // First column of the first row, or -1 when the query fails.
    static int scalar(const QString &sql)
    {
        QSqlQuery query(QSqlDatabase::database("FTADConnection"));

        if (!query.exec(sql) || !query.next())
            return -1;

        return query.value(0).toInt();
    }

    static QString text(const QString &sql)
    {
        QSqlQuery query(QSqlDatabase::database("FTADConnection"));

        if (!query.exec(sql) || !query.next())
            return QString();

        return query.value(0).toString();
    }

    DatabaseManager manager;
};

// Builds a transaction with sensible defaults so each test only has to
// state the fields it actually cares about.
inline Transaction makeTransaction(
    const QString &reference,
    const QString &timestamp,
    const QString &sender,
    const QString &receiver,
    const QString &type,
    double amount)
{
    Transaction transaction;

    transaction.transactionId = reference;
    transaction.timestamp = timestamp;
    transaction.senderId = sender;
    transaction.receiverId = receiver;
    transaction.transactionType = type;
    transaction.amount = amount;
    transaction.currency = "LSL";
    transaction.status = "completed";

    return transaction;
}

// Convenience overload for the detection tests, which index by number.
inline Transaction makeTransaction(
    int id,
    const QString &reference,
    const QString &timestamp,
    const QString &sender,
    const QString &receiver,
    const QString &type,
    double amount)
{
    Transaction transaction = makeTransaction(
        reference, timestamp, sender, receiver, type, amount);

    transaction.id = id;

    return transaction;
}

#endif // FTAD_TESTSUPPORT_H
