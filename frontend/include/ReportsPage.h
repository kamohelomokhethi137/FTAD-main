#ifndef REPORTSPAGE_H
#define REPORTSPAGE_H

#include <QWidget>

class QLabel;
class QPushButton;
class QTableWidget;
class QPlainTextEdit;
class QShowEvent;

class ReportsPage : public QWidget
{
public:
    explicit ReportsPage(QWidget *parent = nullptr);

    void generateComparisonReport();

protected:
    void showEvent(QShowEvent *event) override;

private:
    QPushButton *generateButton = nullptr;
    QLabel *summaryLabel = nullptr;
    QPlainTextEdit *notes = nullptr;
    QTableWidget *table = nullptr;
};

#endif // REPORTSPAGE_H
