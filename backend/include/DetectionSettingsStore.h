
#ifndef DETECTIONSETTINGSSTORE_H
#define DETECTIONSETTINGSSTORE_H

#include "DetectionSettings.h"

class QSettings;

// Loads and saves detection engine preferences. Defaults match DetectionSettings:
// rule engine on, ML model off.
class DetectionSettingsStore
{
public:
    static DetectionSettings load();
    static bool save(const DetectionSettings &settings, QString *error = nullptr);
    static void apply(QSettings &store, const DetectionSettings &settings);
    static DetectionSettings read(const QSettings &store);
};

#endif
