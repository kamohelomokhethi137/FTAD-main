# Appendix E — Performance Measurement Record

**Companion to:** Chapter 6, §6.6
**Status:** Reference material — excluded from the body page count

---

### E.1 Timestamp construction cost

The defect behind BUG-06. Each variant constructs the same date and time; only
the time-zone handling differs. Measured over 20,000 records.

| Construction | Time for 20,000 records |
|---|---:|
| `QDateTime::fromString(ts, "yyyy-MM-dd HH:mm:ss")` (local) | 2,917 ms |
| `QDateTime(date, time, Qt::LocalTime)` | 2,901 ms |
| `QDateTime(date, time, Qt::UTC)` | **6 ms** |

End-to-end analysis at the same size: **40.50 s before, 27.6 ms after** — roughly
**1,466×**. The "before" case is not re-runnable once the defect is fixed, so it is
attributed to its own run and is never blended with current figures.

### E.2 Current baseline, 4 November 2026, Release `-O2`

| Size | `analyze()` | Per transaction | `build()` |
|---:|---:|---:|---:|
| 1,000 | 1.70 ms | 1.70 µs | 0.44 ms |
| 5,000 | 9.94 ms | 1.99 µs | 3.13 ms |
| 10,000 | 18.76 ms | 1.88 µs | 5.87 ms |
| 20,000 | 39.88 ms | 1.99 µs | 12.87 ms |

Scoring throughput is 0.238 µs per score. Each figure is the median of five
repetitions.

### E.3 Measurement noise

Three runs of the same binary on the same data gave 31.7 ms, 34.8 ms and 39.9 ms
at 20,000 transactions — a spread of roughly 25 %. No CPU frequency scaling was
pinned and other processes were running, so the host is not a controlled benchmark
platform. The report quotes the range and the median (34.8 ms) and draws no
conclusion from differences smaller than the spread.

### E.4 Scaling

The measured scaling factor at 20,000 against 1,000 is 23.5×, against a linear
ideal of 20×. The excess is consistent with cache effects and with the noise in
E.3. The decisive observation is that per-transaction cost stays between 1.7 and
2.0 µs across the whole range: under quadratic behaviour that column would rise
steadily with size, and it does not. The algorithm is therefore effectively
linear, which is what the detection rules imply — each transaction is compared
against a bounded history window rather than against all prior transactions.
