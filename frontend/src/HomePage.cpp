
#include "HomePage.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QShowEvent>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>
#include <QPainter>
#include <QColor>
#include <QPen>
#include <QMargins>
#include <QSizePolicy>
#include <QDebug>

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <algorithm>

namespace
{
    int queryCount(
        QSqlDatabase &database,
        const QString &sql)
    {
        QSqlQuery query(database);

        if (!query.exec(sql))
        {
            qWarning()
                << "Home statistics query failed:"
                << query.lastError().text();

            return -1;
        }

        if (!query.next())
            return -1;

        return query.value(0).toInt();
    }

    QString displayCount(int count)
    {
        return count >= 0
            ? QString::number(count)
            : "--";
    }

    QDate transactionDate(const QString &timestamp)
    {
        return QDate::fromString(
            timestamp.left(10),
            "yyyy-MM-dd"
        );
    }
}

HomePage::HomePage(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("content");

    // Main layout contains a scrollable dashboard.
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(
        Qt::ScrollBarAlwaysOff
    );
    scrollArea->setVerticalScrollBarPolicy(
        Qt::ScrollBarAsNeeded
    );

    scrollArea->setStyleSheet(
        "QScrollArea {"
        "background: transparent;"
        "border: none;"
        "}"
        "QScrollBar:vertical {"
        "background: #0B1220;"
        "width: 9px;"
        "margin: 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "background: #334155;"
        "border-radius: 4px;"
        "min-height: 30px;"
        "}"
        "QScrollBar::add-line:vertical,"
        "QScrollBar::sub-line:vertical {"
        "height: 0px;"
        "}"
    );

    auto *dashboard = new QWidget;
    dashboard->setObjectName("content");

    auto *contentLayout = new QVBoxLayout(dashboard);
    contentLayout->setContentsMargins(35, 32, 35, 32);
    contentLayout->setSpacing(22);

    auto *heading = new QLabel("Home");
    heading->setObjectName("heading");
    contentLayout->addWidget(heading);

    auto *description = new QLabel(
        "Monitor transaction activity and identify "
        "unusual financial behavior."
    );
    description->setObjectName("description");
    description->setWordWrap(true);

    contentLayout->addWidget(description);
    contentLayout->addSpacing(12);

    // Responsive statistics cards.
    cardsLayout = new QGridLayout;
    cardsLayout->setHorizontalSpacing(16);
    cardsLayout->setVerticalSpacing(16);
    cardsLayout->setContentsMargins(0, 0, 0, 0);

    cards.append(
        createCard(
            "Total Transactions",
            "--",
            "Loading transaction records...",
            &totalTransactionsValue,
            &totalTransactionsDetail
        )
    );

    cards.append(
        createCard(
            "Flagged Transactions",
            "--",
            "Loading detection results...",
            &flaggedTransactionsValue,
            &flaggedTransactionsDetail
        )
    );

    cards.append(
        createCard(
            "High Risk",
            "--",
            "Loading high-risk alerts...",
            &highRiskValue,
            &highRiskDetail
        )
    );

    cards.append(
        createCard(
            "Pending Investigations",
            "--",
            "Loading investigation cases...",
            &pendingInvestigationsValue,
            &pendingInvestigationsDetail
        )
    );

    contentLayout->addLayout(cardsLayout);

    // Transaction Activity panel.
    auto *activityPanel = new QFrame;
    activityPanel->setObjectName("panel");
    activityPanel->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Preferred
    );

    auto *activityLayout =
        new QVBoxLayout(activityPanel);

    activityLayout->setContentsMargins(
        24, 22, 24, 22
    );
    activityLayout->setSpacing(14);

    auto *activityTitle = new QLabel(
        "Transaction Activity"
    );
    activityTitle->setObjectName("sectionTitle");
    activityLayout->addWidget(activityTitle);

    auto *activityDescription = new QLabel(
        "Daily transaction volume over the last 7 days"
    );
    activityDescription->setStyleSheet(
        "color: #94A3B8; font-size: 12px;"
    );
    activityLayout->addWidget(activityDescription);

    activityChartView = new QChartView;
    activityChartView->setMinimumHeight(320);
    activityChartView->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Fixed
    );
    activityChartView->setRenderHint(
        QPainter::Antialiasing
    );
    activityChartView->setStyleSheet(
        "background: transparent; border: none;"
    );

    activityLayout->addWidget(activityChartView);

    activityMessage = new QLabel;
    activityMessage->setObjectName("emptyMessage");
    activityMessage->setAlignment(Qt::AlignCenter);
    activityMessage->setWordWrap(true);
    activityMessage->hide();

    activityLayout->addWidget(activityMessage);

    contentLayout->addWidget(activityPanel);
    contentLayout->addStretch();

    scrollArea->setWidget(dashboard);
    mainLayout->addWidget(scrollArea);

    updateCardLayout();
    refreshStatistics();
}

QFrame *HomePage::createCard(
    const QString &title,
    const QString &value,
    const QString &detail,
    QLabel **valueLabel,
    QLabel **detailLabel)
{
    auto *card = new QFrame;
    card->setObjectName("card");

    card->setMinimumHeight(145);
    card->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Preferred
    );

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->setSpacing(10);

    auto *titleWidget = new QLabel(title);
    titleWidget->setObjectName("cardTitle");
    titleWidget->setWordWrap(true);

    *valueLabel = new QLabel(value);
    (*valueLabel)->setObjectName("cardValue");

    *detailLabel = new QLabel(detail);
    (*detailLabel)->setObjectName("cardDetail");
    (*detailLabel)->setWordWrap(true);

    layout->addWidget(titleWidget);
    layout->addWidget(*valueLabel);
    layout->addWidget(*detailLabel);
    layout->addStretch();

    return card;
}

void HomePage::updateCardLayout()
{
    if (!cardsLayout || !scrollArea)
        return;

    // Four columns on wide screens,
    // two columns on smaller screens.
    const int availableWidth =
        scrollArea->viewport()->width();

    const int columns =
        availableWidth >= 950 ? 4 : 2;

    if (columns == currentColumns)
        return;

    currentColumns = columns;

    // Remove widgets from their previous grid positions
    // without deleting them.
    while (cardsLayout->count() > 0)
    {
        cardsLayout->takeAt(0);
    }

    for (int i = 0; i < cards.size(); ++i)
    {
        const int row = i / columns;
        const int column = i % columns;

        cardsLayout->addWidget(
            cards[i],
            row,
            column
        );
    }

    for (int column = 0; column < 4; ++column)
    {
        cardsLayout->setColumnStretch(
            column,
            column < columns ? 1 : 0
        );
    }

    cardsLayout->invalidate();
}

void HomePage::refreshStatistics()
{
    QSqlDatabase database =
        QSqlDatabase::database(
            "FTADConnection",
            false
        );

    if (!database.isValid() || !database.isOpen())
    {
        totalTransactionsValue->setText("--");
        flaggedTransactionsValue->setText("--");
        highRiskValue->setText("--");
        pendingInvestigationsValue->setText("--");

        const QString message =
            "Database connection unavailable.";

        totalTransactionsDetail->setText(message);
        flaggedTransactionsDetail->setText(message);
        highRiskDetail->setText(message);
        pendingInvestigationsDetail->setText(message);

        activityChartView->hide();
        activityMessage->setText(
            "Unable to load transaction activity."
        );
        activityMessage->show();

        return;
    }

    const int totalTransactions = queryCount(
        database,
        "SELECT COUNT(*) FROM transactions"
    );

    const int flaggedTransactions = queryCount(
        database,
        "SELECT COUNT(DISTINCT transaction_id) "
        "FROM anomalies"
    );

    const int highRisk = queryCount(
        database,
        "SELECT COUNT(DISTINCT transaction_id) "
        "FROM anomalies "
        "WHERE risk_level = 'high'"
    );

    const int pendingInvestigations = queryCount(
        database,
        "SELECT COUNT(*) "
        "FROM investigations "
        "WHERE case_status = 'pending'"
    );

    totalTransactionsValue->setText(
        displayCount(totalTransactions)
    );

    flaggedTransactionsValue->setText(
        displayCount(flaggedTransactions)
    );

    highRiskValue->setText(
        displayCount(highRisk)
    );

    pendingInvestigationsValue->setText(
        displayCount(pendingInvestigations)
    );

    totalTransactionsDetail->setText(
        totalTransactions < 0
            ? "Unable to retrieve transaction records"
            : totalTransactions == 0
                ? "No transactions available"
                : "Transactions stored in the database"
    );

    flaggedTransactionsDetail->setText(
        flaggedTransactions < 0
            ? "Unable to retrieve detection results"
            : flaggedTransactions == 0
                ? "No anomalies recorded"
                : "Transactions requiring review"
    );

    highRiskDetail->setText(
        highRisk < 0
            ? "Unable to retrieve high-risk alerts"
            : highRisk == 0
                ? "No high-risk activity recorded"
                : "Transactions classified as high risk"
    );

    pendingInvestigationsDetail->setText(
        pendingInvestigations < 0
            ? "Unable to retrieve investigation cases"
            : pendingInvestigations == 0
                ? "No pending cases"
                : "Cases awaiting investigation"
    );

    refreshActivityChart();
}

void HomePage::refreshActivityChart()
{
    QSqlDatabase database =
        QSqlDatabase::database(
            "FTADConnection",
            false
        );

    if (!database.isValid() || !database.isOpen())
    {
        activityChartView->hide();
        activityMessage->setText(
            "Database connection unavailable."
        );
        activityMessage->show();
        return;
    }

    const QDate today = QDate::currentDate();
    const QDate firstDay = today.addDays(-6);

    int dailyCounts[7] = {0, 0, 0, 0, 0, 0, 0};

    QSqlQuery query(database);

    query.prepare(
        "SELECT timestamp "
        "FROM transactions "
        "WHERE substr(timestamp, 1, 10) >= :startDate "
        "AND substr(timestamp, 1, 10) <= :endDate"
    );

    query.bindValue(
        ":startDate",
        firstDay.toString("yyyy-MM-dd")
    );

    query.bindValue(
        ":endDate",
        today.toString("yyyy-MM-dd")
    );

    if (!query.exec())
    {
        qWarning()
            << "Activity query failed:"
            << query.lastError().text();

        activityChartView->hide();
        activityMessage->setText(
            "Unable to load transaction activity."
        );
        activityMessage->show();
        return;
    }

    int totalInPeriod = 0;

    while (query.next())
    {
        const QDate date = transactionDate(
            query.value(0).toString()
        );

        if (!date.isValid())
            continue;

        // daysTo() returns qint64 in Qt 6, so the value is narrowed only
        // after the range check; a distant date cannot wrap into a
        // valid index.
        const qint64 dayIndex = firstDay.daysTo(date);

        if (dayIndex >= 0 && dayIndex < 7)
        {
            dailyCounts[static_cast<int>(dayIndex)]++;
            totalInPeriod++;
        }
    }

    auto *series = new QLineSeries;
    series->setName("Transactions");

    QPen linePen(QColor("#38BDF8"));
    linePen.setWidth(3);
    series->setPen(linePen);
    series->setPointsVisible(true);

    int maximumCount = 0;

    for (int day = 0; day < 7; ++day)
    {
        series->append(day, dailyCounts[day]);

        maximumCount = std::max(
            maximumCount,
            dailyCounts[day]
        );
    }

    auto *chart = new QChart;
    chart->addSeries(series);
    chart->legend()->hide();

    chart->setBackgroundVisible(false);
    chart->setPlotAreaBackgroundVisible(false);
    chart->setMargins(QMargins(8, 12, 18, 8));

    // X axis: day labels.
    auto *axisX = new QValueAxis;
    axisX->setRange(0, 6);
    axisX->setTickCount(7);
    axisX->setLabelFormat("%d");
    axisX->setLabelsColor(QColor("#94A3B8"));
    axisX->setGridLineVisible(false);
    axisX->setLinePenColor(QColor("#334155"));
    axisX->setTitleText(
        "Days (0 = six days ago, 6 = today)"
    );
    axisX->setTitleBrush(QBrush(QColor("#94A3B8")));

    // Y axis: transaction count.
    auto *axisY = new QValueAxis;
    axisY->setRange(
        0,
        std::max(4, maximumCount + 1)
    );
    axisY->setLabelFormat("%d");
    axisY->setTickCount(5);
    axisY->setLabelsColor(QColor("#94A3B8"));
    axisY->setGridLineColor(QColor("#334155"));
    axisY->setLinePenColor(QColor("#334155"));

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);

    series->attachAxis(axisX);
    series->attachAxis(axisY);

    activityChartView->setChart(chart);
    activityChartView->show();

    if (totalInPeriod == 0)
    {
        activityMessage->setText(
            "No transactions recorded in the last 7 days."
        );
        activityMessage->show();
    }
    else
    {
        activityMessage->hide();
    }
}

void HomePage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    refreshStatistics();
}

void HomePage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateCardLayout();
}
