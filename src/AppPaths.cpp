#include "AppPaths.h"

#include <QDir>
#include <QStandardPaths>

AppPaths AppPaths::forApplication()
{
    return {QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)};
}

QString AppPaths::dataDirectory() const { return QDir(root).filePath(QStringLiteral("data")); }
QString AppPaths::logsDirectory() const { return QDir(root).filePath(QStringLiteral("logs")); }
QString AppPaths::databaseFile() const { return QDir(dataDirectory()).filePath(QStringLiteral("supply_chain_risk.db")); }
QString AppPaths::settingsFile() const { return QDir(root).filePath(QStringLiteral("settings.ini")); }
QString AppPaths::logFile() const { return QDir(logsDirectory()).filePath(QStringLiteral("application.log")); }

bool AppPaths::initialize(QString& error) const
{
    error.clear();
    if (root.isEmpty() || !QDir::isAbsolutePath(root)) {
        error = QStringLiteral("Application data root must be an absolute, non-empty path.");
        return false;
    }
    for (const auto& directory : {root, dataDirectory(), logsDirectory()}) {
        if (!QDir().mkpath(directory)) {
            error = QStringLiteral("Cannot create runtime directory: %1").arg(directory);
            return false;
        }
    }
    return true;
}
