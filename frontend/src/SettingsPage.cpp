
#include "SettingsPage.h"

#include "DetectionSettingsStore.h"

#include <QCheckBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

SettingsPage::SettingsPage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(35, 32, 35, 32);
    layout->setSpacing(18);

    auto *heading = new QLabel("Settings");
    heading->setObjectName("heading");
    auto *description = new QLabel(
        "Configure detection engines. ML findings are decision-support signals trained on PaySim (type+amount); "
        "the rule engine remains available for deterministic explainable checks.");
    description->setWordWrap(true);
    description->setObjectName("description");

    auto *panel = new QFrame;
    panel->setObjectName("panel");
    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(22, 20, 22, 20);
    panelLayout->setSpacing(12);

    ruleEngineCheck = new QCheckBox("Enable rule-based detection engine (deterministic, default)");
    mlModelCheck = new QCheckBox("Enable ML model detection (HGB, type+amount, PaySim-trained, experimental)");
    statusLabel = new QLabel();
    statusLabel->setWordWrap(true);
    statusLabel->setObjectName("description");

    auto *buttons = new QHBoxLayout();
    saveButton = new QPushButton("Save detection settings");
    buttons->addWidget(saveButton);
    buttons->addStretch();

    panelLayout->addWidget(ruleEngineCheck);
    panelLayout->addWidget(mlModelCheck);
    panelLayout->addWidget(statusLabel);
    panelLayout->addLayout(buttons);

    layout->addWidget(heading);
    layout->addWidget(description);
    layout->addWidget(panel);
    layout->addStretch();

    QObject::connect(saveButton, &QPushButton::clicked, [this]() { saveSettings(); });
    loadSettings();
}

void SettingsPage::loadSettings()
{
    const DetectionSettings settings = DetectionSettingsStore::load();
    ruleEngineCheck->setChecked(settings.useRuleEngine);
    mlModelCheck->setChecked(settings.useMlModel);
    statusLabel->setText("Current settings loaded. Changes apply to the next analysis run.");
}

void SettingsPage::saveSettings()
{
    DetectionSettings settings = DetectionSettingsStore::load();
    settings.useRuleEngine = ruleEngineCheck->isChecked();
    settings.useMlModel = mlModelCheck->isChecked();

    QString error;
    if (!DetectionSettingsStore::save(settings, &error))
    {
        statusLabel->setStyleSheet("color: #F87171;");
        statusLabel->setText(error.isEmpty() ? "Unable to save settings." : error);
        loadSettings();
        return;
    }

    statusLabel->setStyleSheet("color: #34D399;");
    statusLabel->setText("Detection settings saved. Run analysis again to apply.");
}

