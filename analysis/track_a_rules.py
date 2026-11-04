#!/usr/bin/env python3
"""Track A: score FTAD's detection rules against PaySim ground truth.

FTAD's detector (backend/src/AnomalyDetector.cpp) is a pure function of a
transaction list, so its rules can be reproduced outside the C++ process.
This script mirrors each rule formula-for-formula from the C++ source and
runs it over the PaySim fraud dataset, which carries a ground-truth label
`isFraud` that the application never sees. The output is the honest device
the app has always lacked: precision, recall and false-positive rate for
each rule and for the combined detector.

Fidelity notes, stated so the numbers are not oversold:

  * This is a written mirror of the C++ logic, not the compiled binary.
    The formulas (thresholds, windows, tie-breaks, the population-variance
    z-score, the zero-variance ratio path, weight sums and bands) are
    transcribed from the source; floating-point accumulation order differs,
    so results are equal-to-the-formula, not bit-exact.
  * PaySim timestamps are whole hours. FTAD's duplicate window is ±10 min,
    so at hour-aligned times it contains exactly the same-step siblings;
    the 60-min trailing rapid-withdrawal window is exactly {step-1, step}.
    Both windows therefore reduce to exact integer-step logic.
  * `isWithdrawalType` accepts CASH_OUT and DEBIT, and PaySim has both.
  * The duplicate rule additionally requires the same receiver, and the
    0.01 amount tolerance is applied via cent-rounded amounts (|Δcent|<=1).
  * Behaviour deviation uses strictly-earlier same-sender history with at
    least `minimumHistory` records, ordered by time then import position,
    matching build()/evaluate() in BehaviorAnalyzer.cpp.

Usage:
    python3 track_a_rules.py [path/to/*.csv]

Writes analysis/track_a_results.json and prints a report to stdout.
"""

import json
import math
import pathlib
import sys
import time

import numpy as np
import pandas as pd

HERE = pathlib.Path(__file__).resolve().parent
DEFAULT_CSV = pathlib.Path(
    "/home/hashcat/Documents/dataset/archive/"
    "PS_20174392719_1491204439457_log.csv")

# DetectionSettings.h defaults.
W_DUPLICATE = 40.0
W_LARGE = 35.0
W_RAPID = 35.0
W_BEHAVIOUR = 25.0
LARGE_AMOUNT_ABSOLUTE = 10000.0
DUPLICATE_TOLERANCE = 0.01            # amount tolerance, currency units
RAPID_WINDOW_MINUTES = 60
RAPID_COUNT = 3
Z_THRESHOLD = 3.0
ZERO_VARIANCE_RATIO = 2.0
MINIMUM_HISTORY = 5
MEDIUM_BAND = 35.0
HIGH_BAND = 65.0

# AnomalyDetector::isWithdrawalType.
WITHDRAWAL_TYPES = {"CASH_OUT", "DEBIT"}   # both present in PaySim


def prepare(df):
    """Add sender/receiver int codes and the import-order column.

    Shared by load() and by tests that build synthetic frames.
    """
    n = len(df)
    if "ord" not in df:
        df["ord"] = np.arange(n, dtype=np.int32)
    sender, _ = pd.factorize(df["nameOrig"])
    receiver, _ = pd.factorize(df["nameDest"])
    df["sender"] = sender.astype(np.int32)
    df["receiver"] = receiver.astype(np.int32)
    return n


def load(csv_path):
    t0 = time.time()
    df = pd.read_csv(
        csv_path,
        usecols=["step", "type", "amount", "nameOrig", "nameDest",
                 "isFraud"],
        dtype={"step": "int16", "type": "category", "amount": "float64",
               "nameOrig": "str", "nameDest": "str", "isFraud": "int8"},
    )
    n = len(df)
    print(f"loaded {n:,} rows in {time.time() - t0:.1f} s")
    assert n > 0

    df["isFraud"] = df["isFraud"].astype(np.int8)
    n = prepare(df)
    df.drop(columns=["nameOrig", "nameDest"], inplace=True)

    type_codes, type_uniques = pd.factorize(df["type"])
    df["typeCode"] = type_codes.astype(np.int16)
    withdrawal = np.isin(type_codes, [i for i, u in enumerate(type_uniques)
                                      if u in WITHDRAWAL_TYPES])
    return df, n, withdrawal


def rule_large_amount(amount):
    return amount > LARGE_AMOUNT_ABSOLUTE


def rule_duplicate(df, n):
    """Same sender+receiver within the same hour AND |damount| <= 0.01.

    With whole-hour timestamps the +/-10-min window is exactly the current
    step (adjacent steps are 60 min apart). Equal cent amounts and adjacent
    cents are both inside the 0.01 tolerance.
    """
    flagged = np.zeros(n, dtype=bool)
    cents = np.round(df["amount"].to_numpy() * 100.0).astype(np.int64)
    key = (df["sender"].to_numpy().astype(np.int64) << 42) | (
        df["receiver"].to_numpy().astype(np.int64) << 12) | (
        df["step"].to_numpy().astype(np.int64) & 0xFFF)

    order = np.lexsort((cents, key))
    k, c = key[order], cents[order]

    starts = np.empty(n, dtype=bool)
    starts[0] = True
    starts[1:] = k[1:] != k[:-1]

    # Sibling before me exists iff my group did not start here; sibling
    # after me exists iff the next element belongs to my group.
    gap_prev = np.full(n, np.inf)
    gap_prev[1:] = np.where(~starts[1:], np.abs(c[1:] - c[:-1]), np.inf)

    has_next = ~np.roll(starts, -1)
    gap_next = np.full(n, np.inf)
    gap_next[:-1] = np.where(has_next[:-1], np.abs(c[1:] - c[:-1]), np.inf)

    close = np.minimum(gap_prev, gap_next) <= 1.0
    flagged[order] = close
    return flagged


def rule_rapid_withdrawals(df, n, withdrawal):
    """Withdrawal tx where >= 3 withdrawals of that sender lie in
    [t-60min, t].  With hour times that is exactly steps {h-1, h} and the
    C++ counting includes the current transaction."""
    sender = df["sender"].to_numpy().astype(np.int64)
    step = df["step"].to_numpy().astype(np.int64)

    # Sparse (sender, step) -> count of withdrawals, via a composite key:
    # senders ~2^23, steps < 2^10.
    wkeys = sender[withdrawal] * 1024 + step[withdrawal]
    uk, uc = np.unique(wkeys, return_counts=True)

    def lookup(keys):
        keys = np.asarray(keys, dtype=np.int64)
        pos = np.searchsorted(uk, keys)
        pos_clipped = np.clip(pos, 0, len(uk) - 1)
        hit = (len(uk) > 0) & (uk[pos_clipped] == keys)
        return np.where(hit, uc[pos_clipped], 0)

    idx = np.arange(n)
    count = (np.where(withdrawal, lookup(sender[idx] * 1024 + step[idx]), 0)
             + lookup(sender[idx] * 1024 + step[idx] - 1))
    return withdrawal & (count >= RAPID_COUNT)


def rule_behaviour_deviation(df, n):
    """Strictly-earlier same-sender history, min 5, population variance.

    Matches the C++ per-sender sort: time first, then import order."""
    flagged = np.zeros(n, dtype=bool)
    order = np.lexsort((df["ord"].to_numpy(), df["step"].to_numpy(),
                        df["sender"].to_numpy()))
    amount = df["amount"].to_numpy()[order]
    sender_sorted = df["sender"].to_numpy()[order]

    starts = np.empty(n, dtype=bool)
    starts[0] = True
    starts[1:] = sender_sorted[1:] != sender_sorted[:-1]

    pos = np.arange(n, dtype=np.int64)
    gstart = np.maximum.accumulate(np.where(starts, pos, 0))
    hist_count = pos - gstart

    csum = np.cumsum(amount)
    csumsq = np.cumsum(amount * amount)
    at_start = csum[gstart] - amount  # cumulative *before* group incl self
    # sum of history strictly before i, within group:
    hist_sum = csum - amount - np.where(
        gstart > 0, csum[np.maximum(gstart - 1, 0)], 0.0)
    hist_sumsq = csumsq - amount * amount - np.where(
        gstart > 0, csumsq[np.maximum(gstart - 1, 0)], 0.0)

    eligible = hist_count >= MINIMUM_HISTORY
    mean = np.divide(hist_sum, hist_count, out=np.zeros(n),
                     where=eligible)
    var = np.divide(hist_sumsq, hist_count, out=np.zeros(n),
                    where=eligible) - mean * mean
    var = np.maximum(var, 0.0)
    sigma = np.sqrt(var)

    unusual = np.zeros(n, dtype=bool)
    with np.errstate(divide="ignore", invalid="ignore"):
        z = np.divide(amount - mean, sigma, out=np.zeros(n), where=sigma > 0)
    unusual = eligible & np.where(
        sigma > 0,
        (z >= Z_THRESHOLD) & (amount > mean),
        (mean > 0) & (amount > mean * ZERO_VARIANCE_RATIO))

    flagged[order] = unusual
    return flagged


def metrics(flag, truth):
    tp = int(np.sum(flag & (truth == 1)))
    fp = int(np.sum(flag & (truth == 0)))
    fn = int(np.sum((~flag) & (truth == 1)))
    tn = int(np.sum((~flag) & (truth == 0)))
    prec = tp / (tp + fp) if tp + fp else 0.0
    rec = tp / (tp + fn) if tp + fn else 0.0
    fpr = fp / (fp + tn) if fp + tn else 0.0
    return dict(alerts=tp + fp, tp=tp, fp=fp, fn=fn, tn=tn,
                precision=round(prec, 6), recall=round(rec, 6),
                false_positive_rate=round(fpr, 10))


def main():
    csv_path = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_CSV
    df, n, withdrawal = load(csv_path)
    truth = df["isFraud"].to_numpy()
    amount = df["amount"].to_numpy()

    print("running rules...")
    t0 = time.time()
    f_large = rule_large_amount(amount)
    f_dup = rule_duplicate(df, n)
    f_rapid = rule_rapid_withdrawals(df, n, withdrawal)
    f_beh = rule_behaviour_deviation(df, n)
    print(f"rules done in {time.time() - t0:.1f} s")

    combined = f_large | f_dup | f_rapid | f_beh
    score = (W_DUPLICATE * f_dup + W_LARGE * f_large +
             W_RAPID * f_rapid + W_BEHAVIOUR * f_beh)
    score = np.minimum(score, 100.0)
    level = np.where(score >= HIGH_BAND, "high",
                     np.where(score >= MEDIUM_BAND, "medium",
                              np.where(score > 0, "low", "")))

    results = {
        "dataset": str(csv_path),
        "rows": n,
        "frauds": int(truth.sum()),
        "rules": {
            "large_amount": metrics(f_large, truth),
            "duplicate_payment": metrics(f_dup, truth),
            "rapid_withdrawals": metrics(f_rapid, truth),
            "behaviour_deviation": metrics(f_beh, truth),
        },
        "combined_flagged": metrics(combined, truth),
        "band_medium_or_high": metrics(level != "", truth),
        "band_high": metrics(level == "high", truth),
        "notes": [
            "Python mirror of AnomalyDetector.cpp, not the compiled binary",
            "PaySim hour timestamps collapse the 10-min duplicate window to "
            "the same step and the 60-min rapid window to steps {h-1, h}",
            "duplicate-payment rule requires same sender AND receiver; "
            "PaySim has almost no such repeats, so expect near-zero alerts",
        ],
    }

    out = HERE / "track_a_results.json"
    out.write_text(json.dumps(results, indent=2))

    def line(name, m):
        print(f"  {name:24} alerts {m['alerts']:>9,}  "
              f"P {m['precision']:.4f}  R {m['recall']:.4f}  "
              f"FPR {m['false_positive_rate']:.2e}")

    print(f"\nrows {n:,} | frauds {int(truth.sum()):,} "
          f"({truth.mean() * 100:.3f} %)")
    print("per rule (against isFraud):")
    for k in ("large_amount", "duplicate_payment", "rapid_withdrawals",
              "behaviour_deviation"):
        line(k, results["rules"][k])
    print("combined:")
    for k in ("combined_flagged", "band_medium_or_high", "band_high"):
        line(k, results[k])
    print(f"\nwritten {out}")


if __name__ == "__main__":
    main()
