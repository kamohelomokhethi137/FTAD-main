
#include "MainWindow.h"

#include "HomePage.h"
#include "TransactionsPage.h"
#include "DetectionPage.h"
#include "InvestigationsPage.h"
#include "ReportsPage.h"
#include "SettingsPage.h"

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QStatusBar>
#include <QStackedWidget>
#include <QStringList>
#include <QStyle>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(
        "FTAD | Financial Transaction Anomaly Detector"
    );

    resize(1200, 750);
    setMinimumSize(950, 600);

    auto *central = new QWidget(this);

    auto *mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Sidebar
    auto *sidebar = new QFrame;
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(235);

    auto *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(18, 28, 18, 24);
    sidebarLayout->setSpacing(10);

    auto *brand = new QLabel("FTAD");
    brand->setObjectName("brand");
    sidebarLayout->addWidget(brand);

    auto *subtitle = new QLabel("Transaction Intelligence");
    subtitle->setObjectName("sidebarSubtitle");
    sidebarLayout->addWidget(subtitle);

    sidebarLayout->addSpacing(35);

    const QStringList menuItems = {
        "Home",
        "Transactions",
        "Anomaly Detection",
        "Investigations",
        "Reports",
        "Settings"
    };

    for (int i = 0; i < menuItems.size(); ++i)
    {
        auto *button = new QPushButton(menuItems[i]);

        button->setObjectName(
            i == 0 ? "activeNav" : "navButton"
        );

        button->setCursor(Qt::PointingHandCursor);
        button->setMinimumHeight(44);

        navigationButtons.append(button);
        sidebarLayout->addWidget(button);

        connect(
            button,
            &QPushButton::clicked,
            this,
            [this, i]()
            {
                navigateTo(i);
            }
        );
    }

    sidebarLayout->addStretch();

    auto *footer = new QLabel("FTAD v1.0\nC++17 / Qt 6");
    footer->setObjectName("sidebarSubtitle");
    sidebarLayout->addWidget(footer);

    mainLayout->addWidget(sidebar);

    // Independent application pages
    pageStack = new QStackedWidget;
    pageStack->setObjectName("pageStack");

    pageStack->addWidget(new HomePage);
    pageStack->addWidget(new TransactionsPage);
    pageStack->addWidget(new DetectionPage);
    pageStack->addWidget(new InvestigationsPage);
    pageStack->addWidget(new ReportsPage);
    pageStack->addWidget(new SettingsPage);

    mainLayout->addWidget(pageStack, 1);

    setCentralWidget(central);

    statusBar()->showMessage(
        "System ready | No transaction data loaded"
    );

    applyStyles();
    navigateTo(0);
}

void MainWindow::navigateTo(int pageIndex)
{
    if (!pageStack ||
        pageIndex < 0 ||
        pageIndex >= pageStack->count())
    {
        return;
    }

    pageStack->setCurrentIndex(pageIndex);

    for (int i = 0; i < navigationButtons.size(); ++i)
    {
        QPushButton *button = navigationButtons[i];

        button->setObjectName(
            i == pageIndex ? "activeNav" : "navButton"
        );

        button->style()->unpolish(button);
        button->style()->polish(button);
        button->update();
    }

    const QString pageName =
        navigationButtons[pageIndex]->text();

    if (pageIndex == 0)
    {
        statusBar()->showMessage(
            "Dashboard | System ready"
        );
    }
    else
    {
        statusBar()->showMessage(
            pageName + " | Page loaded"
        );
    }
}

void MainWindow::applyStyles()
{
    setStyleSheet(R"(
        QMainWindow {
            background-color: #0B1220;
        }

        #sidebar {
            background-color: #111C30;
            border-right: 1px solid #263449;
        }

        #brand {
            color: #38BDF8;
            font-size: 30px;
            font-weight: bold;
        }

        #sidebarSubtitle {
            color: #94A3B8;
            font-size: 12px;
        }

        #navButton, #activeNav {
            text-align: left;
            padding-left: 16px;
            border: none;
            border-radius: 8px;
            font-size: 14px;
        }

        #navButton {
            background-color: transparent;
            color: #CBD5E1;
        }

        #navButton:hover {
            background-color: #243550;
        }

        #activeNav {
            background-color: #075985;
            color: white;
            font-weight: bold;
        }

        #content, #pageStack {
            background-color: #0B1220;
        }

        #heading {
            color: #F8FAFC;
            font-size: 27px;
            font-weight: bold;
        }

        #description {
            color: #94A3B8;
            font-size: 13px;
        }

        #card, #panel {
            background-color: #172338;
            border: 1px solid #2A3A52;
            border-radius: 12px;
        }

        #cardTitle {
            color: #94A3B8;
            font-size: 13px;
        }

        #cardValue {
            color: #F8FAFC;
            font-size: 32px;
            font-weight: bold;
        }

        #cardDetail {
            color: #38BDF8;
            font-size: 12px;
        }

        #sectionTitle {
            color: #F8FAFC;
            font-size: 17px;
            font-weight: bold;
        }

        #emptyMessage {
            color: #94A3B8;
            font-size: 13px;
        }

        QStatusBar {
            background-color: #111C30;
            color: #94A3B8;
        }
    )");
}
