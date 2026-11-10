
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
        QMainWindow, QWidget#content, QWidget#pageStack, QScrollArea, QScrollArea > QWidget > QWidget {
            background-color: #000000;
            color: #ffffff;
            font-size: 13px;
        }
        QLabel { color: #ffffff; }
        #sidebar {
            background-color: #0a0a0a;
            border-right: 1px solid #2a2a2a;
        }
        #brand { color: #ffffff; font-size: 28px; font-weight: 700; letter-spacing: 0.5px; }
        #sidebarSubtitle { color: #a0a0a0; font-size: 12px; }
        #navButton, #activeNav {
            text-align: left;
            padding: 10px 14px;
            border: none;
            border-radius: 10px;
            font-size: 14px;
            font-weight: 500;
        }
        #navButton { background-color: transparent; color: #d0d0d0; }
        #navButton:hover { background-color: #1a1a1a; color: #ffffff; }
        #activeNav { background-color: #ffffff; color: #000000; font-weight: 700; }
        #heading { color: #ffffff; font-size: 26px; font-weight: 700; }
        #description { color: #b0b0b0; font-size: 13px; }
        #card, #panel, QFrame#summaryCard, QFrame#detailPanel {
            background-color: #0f0f0f;
            border: 1px solid #2a2a2a;
            border-radius: 14px;
        }
        #cardTitle, #detailCaption { color: #a0a0a0; font-size: 12px; font-weight: 500; }
        #cardValue { color: #ffffff; font-size: 30px; font-weight: 700; }
        #cardDetail { color: #d0d0d0; font-size: 12px; }
        #sectionTitle { color: #ffffff; font-size: 17px; font-weight: 600; }
        #emptyMessage { color: #a0a0a0; font-size: 13px; }
        QLineEdit, QComboBox, QDoubleSpinBox, QSpinBox, QDateTimeEdit, QPlainTextEdit {
            background-color: #000000;
            color: #ffffff;
            border: 1px solid #404040;
            border-radius: 10px;
            padding: 9px 10px;
            selection-background-color: #ffffff;
            selection-color: #000000;
        }
        QLineEdit:focus, QComboBox:focus, QDoubleSpinBox:focus, QSpinBox:focus,
        QDateTimeEdit:focus, QPlainTextEdit:focus {
            border: 1px solid #ffffff;
        }
        QComboBox QAbstractItemView {
            background-color: #0f0f0f; color: #ffffff; border: 1px solid #404040;
            selection-background-color: #ffffff; selection-color: #000000;
        }
        QPushButton {
            background-color: #ffffff;
            color: #000000;
            border: 1px solid #ffffff;
            border-radius: 10px;
            padding: 9px 14px;
            font-weight: 700;
        }
        QPushButton:hover { background-color: #e0e0e0; }
        QPushButton:pressed { background-color: #c0c0c0; }
        QPushButton:disabled { background-color: #2a2a2a; color: #707070; border-color: #2a2a2a; }
        QPushButton#secondaryButton {
            background-color: #000000; color: #ffffff; border: 1px solid #505050;
        }
        QPushButton#secondaryButton:hover { background-color: #1a1a1a; }
        QPushButton#dangerButton { background-color: #ffffff; color: #000000; border: 2px solid #ffffff; }
        QPushButton#dangerButton:hover { background-color: #d0d0d0; }
        QTableWidget {
            background-color: #0f0f0f;
            alternate-background-color: #000000;
            color: #ffffff;
            gridline-color: #2a2a2a;
            border: 1px solid #2a2a2a;
            border-radius: 12px;
            selection-background-color: #ffffff;
            selection-color: #000000;
        }
        QTableWidget::item { padding: 6px; }
        QHeaderView::section {
            background-color: #1a1a1a;
            color: #ffffff;
            padding: 10px 8px;
            border: none;
            border-right: 1px solid #0f0f0f;
            border-bottom: 1px solid #2a2a2a;
            font-weight: 700;
        }
        QTableCornerButton::section { background-color: #1a1a1a; border: none; }
        QScrollBar:vertical {
            background: transparent; width: 10px; margin: 2px;
        }
        QScrollBar::handle:vertical {
            background: #404040; border-radius: 5px; min-height: 28px;
        }
        QScrollBar::handle:vertical:hover { background: #606060; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
        QScrollBar:horizontal { background: transparent; height: 10px; margin: 2px; }
        QScrollBar::handle:horizontal { background: #404040; border-radius: 5px; min-width: 28px; }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }
        QStatusBar {
            background-color: #0a0a0a;
            color: #a0a0a0;
            border-top: 1px solid #2a2a2a;
        }
        QToolTip {
            background-color: #0f0f0f; color: #ffffff; border: 1px solid #404040; padding: 6px;
        }
        QDialog, QMessageBox { background-color: #000000; color: #ffffff; }
        QCheckBox { color: #ffffff; spacing: 8px; }
        QCheckBox::indicator { width: 16px; height: 16px; border: 1px solid #606060; background: #000000; border-radius: 3px; }
        QCheckBox::indicator:checked { background: #ffffff; border-color: #ffffff; }
    )");
}
