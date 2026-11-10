
#include "DetectionSettingsStore.h"

#include <QCoreApplication>
#include <QSettings>

namespace {
const char *kGroup = "detection";
const char *kUseRules = "useRuleEngine";
const char *kUseMl = "useMlModel";

QSettings makeStore()
{
    if (!QCoreApplication::organizationName().isEmpty() ||
        !QCoreApplication::applicationName().isEmpty())
    {
        return QSettings();
    }
    return QSettings("FTAD", "FTAD");
}
}

DetectionSettings DetectionSettingsStore::read(const QSettings &store)
{
    DetectionSettings settings;
    settings.useRuleEngine = store.value(QString("%1/%2").arg(kGroup, kUseRules), true).toBool();
    settings.useMlModel = store.value(QString("%1/%2").arg(kGroup, kUseMl), false).toBool();
    return settings;
}

void DetectionSettingsStore::apply(QSettings &store, const DetectionSettings &settings)
{
    store.beginGroup(kGroup);
    store.setValue(kUseRules, settings.useRuleEngine);
    store.setValue(kUseMl, settings.useMlModel);
    store.endGroup();
}

DetectionSettings DetectionSettingsStore::load()
{
    QSettings store = makeStore();
    return read(store);
}

bool DetectionSettingsStore::save(const DetectionSettings &settings, QString *error)
{
    QString validationError;
    if (!settings.validate(&validationError))
    {
        if (error) *error = validationError;
        return false;
    }
    QSettings store = makeStore();
    apply(store, settings);
    store.sync();
    if (store.status() != QSettings::NoError)
    {
        if (error) *error = "Unable to save detection settings.";
        return false;
    }
    return true;
}
