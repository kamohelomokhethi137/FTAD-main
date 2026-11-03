
#include <QApplication>
#include <QMessageBox>
#include <QDebug>

#include "MainWindow.h"
#include "DatabaseManager.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    DatabaseManager databaseManager;

    if (!databaseManager.connect())
    {
        QMessageBox::critical(
            nullptr,
            "FTAD - Database Error",
            "Unable to connect to SQLite database:\n" +
            databaseManager.lastError()
        );

        return 1;
    }

    qDebug() << "FTAD SQLite database connected successfully!";

    if (!databaseManager.initializeSchema())
    {
        QMessageBox::critical(
            nullptr,
            "FTAD - Schema Error",
            "Unable to initialize database tables:\n" +
            databaseManager.lastError()
        );

        return 1;
    }

    qDebug() << "FTAD database tables initialized successfully!";

    MainWindow window;
    window.show();

    return app.exec();
}
