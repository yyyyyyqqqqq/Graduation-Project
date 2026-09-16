#pragma once

#include <QString>

// Explicit INI files avoid registry/global settings and keep tests isolated.
namespace AppSettings
{
bool readLastNavigationPage(const QString& filePath, QString& page, QString& error);
bool writeLastNavigationPage(const QString& filePath, const QString& page, QString& error);
}
