// Read-only verification harness for the FTAD sample dataset.
//
// Answers one question with evidence rather than inference: does
// data/samples/test_transactions.csv actually trigger all four detection
// rules? BUG-05 claims it does not.
//
// It links the production ftad_core library, uses the production
// DatabaseManager, CsvImportService and DetectionService, and touches no
// repository file. It runs in its own directory with its own database.

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSqlDatabase>
#include <QTextStream>

#include "CsvImportService.h"
#include "DatabaseManager.h"
#include "DetectionResult.h"
#include "DetectionService.h"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QTextStream out(stdout);
    const QString base = QCoreApplication::applicationDirPath();

    // Fresh database every run, same discipline as the test fixture.
    QDir().mkpath(base + "/database");
    QFile::remove(base + "/database/ftad.db");

    DatabaseManager manager;

    if (!manager.connect())
    {
        out << "connect failed: " << manager.lastError() << "\n";
        return 1;
    }

    if (!manager.initializeSchema())
    {
        out << "schema failed: " << manager.lastError() << "\n";
        return 1;
    }

    CsvImportService importer;
    const CsvImportResult imported =
        importer.importFile(base + "/test_transactions.csv");

    out << "imported=" << imported.imported
        << " skipped=" << imported.skipped
        << " failed=" << imported.failed << "\n";

    DetectionService service;
    int analyzed = 0;
    int flagged = 0;

    if (!service.analyzeAll(&analyzed, &flagged))
    {
        out << "analyze failed: " << service.lastError() << "\n";
        return 1;
    }

    out << "analyzed=" << analyzed << " flagged=" << flagged << "\n\n";

    out << "reference  amount      score  level   methods\n";
    out << "--------------------------------------------------------------\n";

    const QList<DetectionResult> results = service.flaggedResults();

    for (const DetectionResult &result : results)
    {
        out << result.transactionReference << "  "
            << QString::number(result.amount, 'f', 2).leftJustified(10) << "  "
            << QString::number(result.riskScore, 'f', 1).leftJustified(5) << "  "
            << result.riskLevel.leftJustified(7) << "  "
            << result.methodText() << "\n";
    }

    out << "--------------------------------------------------------------\n";
    out << "flagged transactions: " << results.size() << "\n";

    return 0;
}
