
// Timing benchmark for the detection pipeline.
//
// The detector is the only component with super-linear work, so the
// benchmark measures how analysis time grows with the number of
// transactions. Each size is run several times and the median is
// reported, which removes most of the noise from a shared machine.
//
// Build:  cmake --build <dir> --target ftad_benchmark
// Run:    QT_QPA_PLATFORM=offscreen ./ftad_benchmark

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QList>
#include <QString>
#include <QTextStream>

#include <algorithm>
#include <clocale>
#include <cmath>
#include <cstdio>
#include <functional>

#include "AnomalyDetector.h"
#include "BehaviorAnalyzer.h"
#include "DatabaseManager.h"
#include "DetectionResult.h"
#include "DetectionSettings.h"
#include "RiskScorer.h"
#include "Transaction.h"
#include "TransactionRepository.h"

namespace
{
    // Builds a data set with realistic shape: several accounts, amounts
    // clustered around a mean, and a fixed number of injected anomalies
    // so that the detector's work does not depend on the data alone.
    QList<Transaction> buildDataSet(int count)
    {
        QList<Transaction> transactions;
        transactions.reserve(count);

        const int accountCount = 50;

        for (int i = 0; i < count; ++i)
        {
            const int account = 100 + (i % accountCount);

            // A deterministic pseudo-random amount between 100 and 1000.
            const int wobble = (i * 37) % 900;
            double amount = 100.0 + wobble;

            // One transaction in a thousand is an outlier.
            if (i % 1000 == 0)
                amount = 25000.0;

            Transaction transaction;

            transaction.transactionId =
                QString("TXN%1").arg(i + 1, 4, 10, QChar('0'));
            transaction.timestamp =
                QString("2026-10-%1 %2:%3:00")
                    .arg(1 + (i / 1440) % 28, 2, 10, QChar('0'))
                    .arg((i / 60) % 24, 2, 10, QChar('0'))
                    .arg(i % 60, 2, 10, QChar('0'));
            transaction.senderId =
                QString("ACCX%1").arg(account);
            transaction.receiverId =
                QString("ACCX%1").arg(100 + ((account + 7) % accountCount));
            transaction.transactionType =
                (i % 5 == 0) ? "CASH_OUT" : "TRANSFER";
            transaction.amount = amount;
            transaction.currency = "LSL";
            transaction.status = "completed";

            transactions.append(transaction);
        }

        return transactions;
    }

    double medianOf(QList<double> samples)
    {
        std::sort(samples.begin(), samples.end());

        const int middle = samples.size() / 2;

        if (samples.size() % 2 == 0)
        {
            return (samples.at(middle - 1) + samples.at(middle)) / 2.0;
        }

        return samples.at(middle);
    }

    // Runs one workload repeatedly and returns the median wall time in
    // milliseconds, plus the number of flagged transactions so the
    // optimiser cannot remove the work.
    double measure(int repetitions, const std::function<int()> &workload,
                   int *flagged)
    {
        QList<double> samples;

        for (int run = 0; run < repetitions; ++run)
        {
            QElapsedTimer timer;
            timer.start();

            *flagged = workload();

            samples.append(timer.nsecsElapsed() / 1.0e6);
        }

        return medianOf(samples);
    }

    void reportHeader(const QString &title)
    {
        std::printf("\n%s\n", qPrintable(title));
        std::printf("%-10s %12s %12s %14s\n",
                    "size", "median ms", "us/txn", "flagged");
        std::printf("%s\n",
                    "--------------------------------------------------"
                    "-------");
    }

    void reportRow(int size, double milliseconds, int flagged)
    {
        const double perTransaction =
            (milliseconds * 1000.0) / std::max(size, 1);

        std::printf("%-10d %12.3f %12.3f %14d\n",
                    size, milliseconds, perTransaction, flagged);
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);

    // QCoreApplication installs the user's locale, so this has to run
    // afterwards. printf would otherwise print a decimal comma.
    std::setlocale(LC_ALL, "C");

    const int repetitions = 5;

    std::printf("FTAD detection benchmark\n");
    std::printf("repetitions per size: %d (median reported)\n",
                repetitions);

    // --- Full analysis -------------------------------------------------
    reportHeader("AnomalyDetector::analyze()");

    const int sizes[] = {1000, 2000, 5000, 10000, 20000};

    QList<double> analysisTimes;

    for (const int size : sizes)
    {
        const QList<Transaction> data = buildDataSet(size);
        const AnomalyDetector detector;

        int flagged = 0;

        const double milliseconds = measure(
            repetitions,
            [&detector, &data]() {
                const QList<DetectionResult> results =
                    detector.analyze(data);

                int count = 0;

                for (const DetectionResult &result : results)
                {
                    if (result.flagged())
                        ++count;
                }

                return count;
            },
            &flagged);

        analysisTimes.append(milliseconds);
        reportRow(size, milliseconds, flagged);
    }

    // --- Behavioural baseline only -------------------------------------
    reportHeader("BehaviorAnalyzer::build()");

    for (const int size : sizes)
    {
        const QList<Transaction> data = buildDataSet(size);

        int dummy = 0;

        const double milliseconds = measure(
            repetitions,
            [&data]() {
                BehaviorAnalyzer analyzer(5);
                analyzer.build(data);

                return analyzer.hasBaseline("ACCX100") ? 1 : 0;
            },
            &dummy);

        reportRow(size, milliseconds, dummy);
    }

    // --- Scoring only ---------------------------------------------------
    reportHeader("RiskScorer::score() x10000");

    {
        const RiskScorer scorer{DetectionSettings()};
        const Transaction transaction = buildDataSet(1).first();

        QList<RuleHit> hits;

        RuleHit hit;
        hit.method = "duplicate_payment";
        hit.reason = "benchmark";
        hit.weight = 40.0;
        hits.append(hit);

        int dummy = 0;

        const double milliseconds = measure(
            repetitions,
            [&scorer, &transaction, &hits]() {
                int flagged = 0;

                for (int i = 0; i < 10000; ++i)
                {
                    if (scorer.score(transaction, hits).flagged())
                        ++flagged;
                }

                return flagged;
            },
            &dummy);

        std::printf("10000 scores: %.3f ms total, %.3f us/score\n",
                    milliseconds, milliseconds * 1000.0 / 10000.0);
    }

    // --- Persistence ----------------------------------------------------
    reportHeader("TransactionRepository::addTransaction()");

    {
        DatabaseManager manager;

        if (!manager.connect() || !manager.initializeSchema())
        {
            std::printf("database unavailable: %s\n",
                        qPrintable(manager.lastError()));
        }
        else
        {
            const int batchSizes[] = {100, 500, 1000};

            for (const int batch : batchSizes)
            {
                const QList<Transaction> data = buildDataSet(batch);

                QElapsedTimer timer;
                timer.start();

                TransactionRepository repository;
                int inserted = 0;

                for (const Transaction &transaction : data)
                {
                    if (repository.addTransaction(transaction))
                        ++inserted;
                }

                const double milliseconds =
                    timer.nsecsElapsed() / 1.0e6;

                reportRow(batch, milliseconds, inserted);
            }

            manager.disconnect();
        }
    }

    // --- Scaling summary -------------------------------------------------
    if (analysisTimes.size() == static_cast<int>(sizeof(sizes) / sizeof(sizes[0])))
    {
        std::printf("\nScaling factor (analysis time relative to 1000)\n");

        for (int i = 0; i < analysisTimes.size(); ++i)
        {
            std::printf("%-10d %12.2fx\n",
                        sizes[i],
                        analysisTimes.at(i) / analysisTimes.first());
        }
    }

    return 0;
}
