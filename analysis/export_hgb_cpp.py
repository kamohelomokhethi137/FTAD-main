#!/usr/bin/env python3
import json, math
from pathlib import Path
HERE = Path(__file__).resolve().parent
d = json.loads((HERE/'artifacts/hgb_export.json').read_text())
hdr = HERE.parent/'backend/include/MlFraudModel.h'
src = HERE.parent/'backend/src/MlFraudModel.cpp'

def arr(name, vals, typ='float'):
    def fmtf(v):
        t = f'{float(v):.9g}'
        if 'e' in t or 'E' in t:
            return t + 'f'
        if '.' not in t:
            t += '.0'
        return t + 'f'
    body = ', '.join(fmtf(v) if typ=='float' else str(int(v)) for v in vals)
    return f'static const {typ} {name}[{len(vals)}] = {{ {body} }};\n'

# flatten trees into parallel arrays with per-tree offsets
offsets=[]; feat=[]; thr=[]; left=[]; right=[]; miss=[]; leaf=[]; val=[]
for t in d['trees']:
    offsets.append(len(feat))
    n=len(t['value'])
    feat += [int(x) for x in t['feature']]
    thr += [float(x) for x in t['threshold']]
    left += [int(x) for x in t['left']]
    right += [int(x) for x in t['right']]
    miss += [int(x) for x in t['missing_left']]
    leaf += [int(x) for x in t['is_leaf']]
    val += [float(x) for x in t['value']]
offsets.append(len(feat))

cal = d['calibration_points']
h = r'''#ifndef MLFRAUDMODEL_H
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
'''
s = '#include "MlFraudModel.h"\n#include <cmath>\n#include <algorithm>\n#include <QString>\nnamespace MlFraudModel {\nnamespace {\n'
s += f'static const int N_TREES = {len(d["trees"])};\n'
s += f'static const float BASELINE = {d["baseline"]:.9g}f;\n'
s += f'static const float LR = {d["learning_rate"]:.9g}f;\n'
s += arr('T_OFF', offsets, 'int')
s += arr('T_FEAT', feat, 'int')
s += arr('T_THR', thr, 'float')
s += arr('T_LEFT', left, 'int')
s += arr('T_RIGHT', right, 'int')
s += arr('T_MISS', miss, 'int')
s += arr('T_LEAF', leaf, 'int')
s += arr('T_VAL', val, 'float')
# calibration
px = [p[0] for p in cal]; py=[p[1] for p in cal]
s += arr('CAL_P', px, 'float')
s += arr('CAL_S', py, 'float')
s += f'static const int N_CAL = {len(cal)};\n'
s += r'''
static float sigmoid(float x) {
    if (x >= 0.0f) { float z = std::exp(-x); return 1.0f / (1.0f + z); }
    float z = std::exp(x); return z / (1.0f + z);
}
static double calibrate(float p) {
    if (p <= CAL_P[0]) return CAL_S[0];
    for (int i = 1; i < N_CAL; ++i) {
        if (p <= CAL_P[i]) {
            float t = (p - CAL_P[i-1]) / std::max(CAL_P[i]-CAL_P[i-1], 1e-12f);
            return CAL_S[i-1] + t * (CAL_S[i]-CAL_S[i-1]);
        }
    }
    return CAL_S[N_CAL-1];
}
}
int typeIndex(const QString &transactionType) {
    const QString t = transactionType.trimmed().toUpper();
    if (t == "CASH_IN") return 0;
    if (t == "CASH_OUT") return 1;
    if (t == "DEBIT") return 2;
    if (t == "PAYMENT") return 3;
    if (t == "TRANSFER") return 4;
    return 0; // unknown -> CASH_IN bucket (lowest-risk default)
}
float predictProbability(int typeC, double amount) {
    const float x0 = static_cast<float>(typeC);
    const float x1 = static_cast<float>(std::log1p(std::max(amount, 0.0)));
    float raw = BASELINE;
    for (int ti = 0; ti < N_TREES; ++ti) {
        int off = T_OFF[ti];
        int n = 0;
        for (int step = 0; step < 1024; ++step) {
            int i = off + n;
            if (T_LEAF[i]) { raw += LR * T_VAL[i]; break; }
            float fv = (T_FEAT[i] == 0) ? x0 : x1;
            // sklearn numeric split: NaN goes to missing_left; values here never NaN
            bool goLeft = (fv <= T_THR[i]);
            n = goLeft ? T_LEFT[i] : T_RIGHT[i];
        }
    }
    return sigmoid(raw);
}
double predictRiskScore(const QString &transactionType, double amount) {
    return calibrate(predictProbability(typeIndex(transactionType), amount));
}
QString reasonText(const QString &transactionType, double amount, double score) {
    return QString("ML model (gradient-boosted trees, type+amount) assigned risk %1/100 for %2 amount %3.")
        .arg(score, 0, 'f', 1).arg(transactionType.trimmed().toUpper()).arg(amount, 0, 'f', 2);
}
int treeCount() { return N_TREES; }
}
'''
hdr.write_text(h); src.write_text(s)
print('wrote', hdr, src, 'bytes', hdr.stat().st_size, src.stat().st_size)
