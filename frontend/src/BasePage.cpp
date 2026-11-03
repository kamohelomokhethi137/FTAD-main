
#include "BasePage.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>

BasePage::BasePage(
    const QString &title,
    const QString &description,
    const QString &message,
    QWidget *parent
)
    : QWidget(parent)
{
    setObjectName("content");

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(35, 32, 35, 32);
    layout->setSpacing(20);

    auto *heading = new QLabel(title);
    heading->setObjectName("heading");
    layout->addWidget(heading);

    auto *descriptionLabel = new QLabel(description);
    descriptionLabel->setObjectName("description");
    descriptionLabel->setWordWrap(true);
    layout->addWidget(descriptionLabel);

    layout->addSpacing(15);

    auto *panel = new QFrame;
    panel->setObjectName("panel");

    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(24, 22, 24, 22);

    auto *messageLabel = new QLabel(message);
    messageLabel->setObjectName("emptyMessage");
    messageLabel->setAlignment(Qt::AlignCenter);
    messageLabel->setWordWrap(true);
    messageLabel->setMinimumHeight(180);

    panelLayout->addWidget(messageLabel);
    layout->addWidget(panel);
    layout->addStretch();
}
