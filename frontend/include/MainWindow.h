
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>

class QStackedWidget;
class QPushButton;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    QStackedWidget *pageStack = nullptr;
    QVector<QPushButton *> navigationButtons;

    void navigateTo(int pageIndex);
    void applyStyles();
};

#endif // MAINWINDOW_H
    