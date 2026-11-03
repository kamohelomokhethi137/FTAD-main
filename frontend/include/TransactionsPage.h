
#ifndef TRANSACTIONSPAGE_H
#define TRANSACTIONSPAGE_H

#include <QWidget>

class QLineEdit;
class QTableWidget;
class QPushButton;

class TransactionsPage : public QWidget
{
public:
    explicit TransactionsPage(QWidget *parent = nullptr);

    void refreshTransactions();

private:
    void openAddTransactionDialog();
    void importCsvFile();
    void filterTransactions(const QString &searchText);

    QLineEdit *searchInput;
    QTableWidget *transactionsTable;
    QPushButton *addButton;
    QPushButton *importButton;
};

#endif // TRANSACTIONSPAGE_H
