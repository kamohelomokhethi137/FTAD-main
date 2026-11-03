
#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QString>
#include <QSqlDatabase>

class DatabaseManager
{
public:
    DatabaseManager();
    ~DatabaseManager();

    bool connect();
    void disconnect();
    bool isConnected() const;
    QString lastError() const;

    bool initializeSchema();

private:
    QSqlDatabase database;
    QString errorMessage;
};

#endif // DATABASEMANAGER_H
