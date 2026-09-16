#include "AppSettings.h"

#include <QSettings>

namespace
{
bool checkStatus(const QSettings& settings, QString& error)
{
    switch (settings.status()) {
    case QSettings::NoError:
        return true;
    case QSettings::AccessError:
        error = QStringLiteral("Cannot access settings file: %1").arg(settings.fileName());
        return false;
    case QSettings::FormatError:
        error = QStringLiteral("Invalid INI format: %1").arg(settings.fileName());
        return false;
    }
    return false;
}
}

bool AppSettings::readLastNavigationPage(const QString& filePath, QString& page, QString& error)
{
    error.clear();
    QSettings settings(filePath, QSettings::IniFormat);
    settings.setFallbacksEnabled(false);
    page = settings.value(QStringLiteral("ui/lastNavigationPage"), QStringLiteral("overview")).toString();
    return checkStatus(settings, error);
}

bool AppSettings::writeLastNavigationPage(const QString& filePath, const QString& page, QString& error)
{
    error.clear();
    QSettings settings(filePath, QSettings::IniFormat);
    settings.setFallbacksEnabled(false);
    // Read first so an invalid existing INI is not silently overwritten.
    settings.value(QStringLiteral("ui/lastNavigationPage"));
    if (!checkStatus(settings, error)) {
        return false;
    }
    settings.setValue(QStringLiteral("ui/lastNavigationPage"), page);
    settings.sync();
    return checkStatus(settings, error);
}
