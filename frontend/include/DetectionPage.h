
#ifndef DETECTIONPAGE_H
#define DETECTIONPAGE_H

#include "DetectionResult.h"

#include <QList>
#include <QWidget>

class QLabel;
class QPushButton;
class QComboBox;
class QTableWidget;
class QShowEvent;

class DetectionPage : public QWidget
{
public:
    explicit DetectionPage(QWidget *parent = nullptr);

    void refreshResults();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void runAnalysis();
    void applyFilter();
    void updateSummary();

    QLabel *flaggedValue = nullptr;
    QLabel *highValue = nullptr;
    QLabel *mediumValue = nullptr;
    QLabel *lowValue = nullptr;

    QComboBox *levelFilter = nullptr;
    QPushButton *analyzeButton = nullptr;
    QTableWidget *resultsTable = nullptr;
    QLabel *emptyMessage = nullptr;

    QList<DetectionResult> cachedResults;
};

#endif // DETECTIONPAGE_H
