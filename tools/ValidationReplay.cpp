#include "ValidationExperiment.h"
#include <QCoreApplication>
#include <QTextStream>
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    const auto args = app.arguments();
    if (args.size() != 3)
    {
        QTextStream(stderr) << "Usage: ValidationReplay FROZEN_DIRECTORY RESULT_JSON\n";
        return 2;
    }
    QString error;
    const auto dataset = Validation::loadDirectory(args[1], error);
    if (!dataset)
    {
        QTextStream(stderr) << error << Qt::endl;
        return 1;
    }
    const auto e = Validation::runExperiment(*dataset);
    if (!Validation::exportResult(e, args[2], error))
    {
        QTextStream(stderr) << error << Qt::endl;
        return 1;
    }
    QTextStream(stdout) << Validation::experimentSummary(*dataset, e) << Qt::endl;
    return e.result["automatedPass"].toBool() ? 0 : 1;
}
