
#ifndef CSVIMPORTSERVICE_H
#define CSVIMPORTSERVICE_H

#include <QString>

struct CsvImportResult
{
    int imported = 0;
    int skipped = 0;
    int failed = 0;
    QString details;
};

class CsvImportService
{
public:
    CsvImportResult importFile(const QString &filePath);
};

#endif // CSVIMPORTSERVICE_H
