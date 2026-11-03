
#include "DatabaseManager.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QDir>
#include <QFile>
#include <QCoreApplication>
#include <QDebug>

DatabaseManager::DatabaseManager()
{
}

DatabaseManager::~DatabaseManager()
{
    disconnect();
}

bool DatabaseManager::connect()
{
    if (database.isValid() && database.isOpen())
    {
        return true;
    }

    const QString databaseFolder =
        QCoreApplication::applicationDirPath() + "/database";

    QDir directory;

    if (!directory.mkpath(databaseFolder))
    {
        errorMessage = "Unable to create database directory.";
        return false;
    }

    database = QSqlDatabase::addDatabase(
        "QSQLITE",
        "FTADConnection"
    );

    database.setDatabaseName(
        databaseFolder + "/ftad.db"
    );

    if (!database.open())
    {
        errorMessage = database.lastError().text();
        return false;
    }

    QSqlQuery pragma(database);

    if (!pragma.exec("PRAGMA foreign_keys = ON"))
    {
        errorMessage = pragma.lastError().text();
        disconnect();
        return false;
    }

    errorMessage.clear();
    return true;
}

bool DatabaseManager::initializeSchema()
{
    if (!isConnected())
    {
        errorMessage = "Database is not connected.";
        return false;
    }

    const QString schemaPath =
        QCoreApplication::applicationDirPath() +
        "/database/schema.sql";

    QFile schemaFile(schemaPath);

    if (!schemaFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        errorMessage =
            "Unable to open database schema: " + schemaPath;
        return false;
    }

    const QString schema =
        QString::fromUtf8(schemaFile.readAll());

    schemaFile.close();

    // Execute individual SQL statements.
    // This schema contains no semicolons inside SQL strings.
    const QStringList statements =
        schema.split(';', Qt::SkipEmptyParts);

    if (!database.transaction())
    {
        errorMessage = database.lastError().text();
        return false;
    }

    for (const QString &statement : statements)
    {
        const QString sql = statement.trimmed();

        if (sql.isEmpty())
        {
            continue;
        }

        QSqlQuery query(database);

        if (!query.exec(sql))
        {
            errorMessage =
                "Schema initialization failed: " +
                query.lastError().text();

            database.rollback();
            return false;
        }
    }

    if (!database.commit())
    {
        errorMessage =
            "Unable to commit database schema: " +
            database.lastError().text();

        database.rollback();
        return false;
    }

    errorMessage.clear();
    return true;
}

void DatabaseManager::disconnect()
{
    if (!database.isValid())
    {
        return;
    }

    const QString connectionName =
        database.connectionName();

    database.close();
    database = QSqlDatabase();

    QSqlDatabase::removeDatabase(connectionName);
}

bool DatabaseManager::isConnected() const
{
    return database.isValid() && database.isOpen();
}

QString DatabaseManager::lastError() const
{
    return errorMessage;
}
