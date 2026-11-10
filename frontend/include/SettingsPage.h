
#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include <QWidget>

class QCheckBox;
class QLabel;
class QPushButton;

class SettingsPage : public QWidget
{
public:
    explicit SettingsPage(QWidget *parent = nullptr);

private:
    void loadSettings();
    void saveSettings();

    QCheckBox *ruleEngineCheck = nullptr;
    QCheckBox *mlModelCheck = nullptr;
    QLabel *statusLabel = nullptr;
    QPushButton *saveButton = nullptr;
};

#endif // SETTINGSPAGE_H
