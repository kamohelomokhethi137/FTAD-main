#ifndef MLFRAUDMODEL_H
#define MLFRAUDMODEL_H
#include <QString>
// Generated from analysis/artifacts/hgb_export.json (HistGradientBoostingClassifier,
// restricted features: type + amount). Deterministic inference, no Python runtime.
namespace MlFraudModel {
int typeIndex(const QString &transactionType);
float predictProbability(int typeC, double amount); // 0..1
double predictRiskScore(const QString &transactionType, double amount); // 0..100
QString reasonText(const QString &transactionType, double amount, double score);
int treeCount();
}
#endif
