#pragma once

#include <QString>

// A value describing one application data root; tests supply a temporary root.
struct AppPaths
{
    QString root;

    static AppPaths forApplication();
    QString dataDirectory() const;
    QString logsDirectory() const;
    QString databaseFile() const;
    QString settingsFile() const;
    QString logFile() const;
    bool initialize(QString& error) const;
};
