
#ifndef HOMEPAGE_H
#define HOMEPAGE_H

#include <QWidget>
#include <QFrame>
#include <QString>
#include <QList>

class QLabel;
class QShowEvent;
class QResizeEvent;
class QChartView;
class QGridLayout;
class QScrollArea;

class HomePage : public QWidget
{
public:
    explicit HomePage(QWidget *parent = nullptr);

    void refreshStatistics();

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QFrame *createCard(
        const QString &title,
        const QString &value,
        const QString &detail,
        QLabel **valueLabel,
        QLabel **detailLabel
    );

    void refreshActivityChart();
    void updateCardLayout();

    QLabel *totalTransactionsValue = nullptr;
    QLabel *totalTransactionsDetail = nullptr;

    QLabel *flaggedTransactionsValue = nullptr;
    QLabel *flaggedTransactionsDetail = nullptr;

    QLabel *highRiskValue = nullptr;
    QLabel *highRiskDetail = nullptr;

    QLabel *pendingInvestigationsValue = nullptr;
    QLabel *pendingInvestigationsDetail = nullptr;

    QGridLayout *cardsLayout = nullptr;
    QList<QFrame *> cards;

    QScrollArea *scrollArea = nullptr;
    QChartView *activityChartView = nullptr;
    QLabel *activityMessage = nullptr;

    int currentColumns = 0;
};

#endif
