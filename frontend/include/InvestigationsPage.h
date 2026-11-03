
#ifndef INVESTIGATIONSPAGE_H
#define INVESTIGATIONSPAGE_H

#include "Investigation.h"

#include <QList>
#include <QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QShowEvent;
class QTableWidget;

class InvestigationsPage : public QWidget
{
public:
    explicit InvestigationsPage(QWidget *parent = nullptr);

    void refreshCases();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void buildUi();
    void populateTable();
    void applyFilter();
    void showSelectedCase(int row);
    void loadHistory(int investigationId);
    void saveSelectedCase();
    void clearDetail();
    void updateSummary();

    QLabel *totalValue = nullptr;
    QLabel *openValue = nullptr;
    QLabel *resolvedValue = nullptr;
    QLabel *dismissedValue = nullptr;

    QComboBox *statusFilter = nullptr;
    QPushButton *refreshButton = nullptr;
    QTableWidget *casesTable = nullptr;
    QLabel *emptyMessage = nullptr;

    QLabel *detailTitle = nullptr;
    QLabel *detailTransaction = nullptr;
    QLabel *detailAccounts = nullptr;
    QLabel *detailRisk = nullptr;

    QComboBox *statusInput = nullptr;
    QLineEdit *assigneeInput = nullptr;
    QPlainTextEdit *notesInput = nullptr;
    QPushButton *saveButton = nullptr;
    QTableWidget *historyTable = nullptr;

    QList<InvestigationCase> cachedCases;
    int selectedInvestigationId = 0;
};

#endif // INVESTIGATIONSPAGE_H
