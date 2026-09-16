#pragma once

#include "Project.h"
#include <QList>

class AppDatabase;
class AppLogger;

enum class ProjectError { None, InvalidName, InvalidDescription, NotFound, Database };

struct ProjectResult
{
    ProjectError error = ProjectError::None;
    QString diagnostic; // Internal only; userMessage() is safe for UI display.
    bool ok() const { return error == ProjectError::None; }
    QString userMessage() const;
};

// Borrows existing services on their owning thread. They must outlive this repository.
// No connection handle, query, or project cache is retained between calls.
class ProjectRepository final
{
public:
    ProjectRepository(AppDatabase& database, AppLogger& logger);
    ProjectResult create(const QString& name, const QString& description, Project& project);
    ProjectResult list(QList<Project>& projects) const;
    ProjectResult findById(const QString& id, Project& project) const;
    ProjectResult remove(const QString& id);

private:
    ProjectResult databaseError(const QString& operation, const QString& diagnostic) const;
    AppDatabase& m_database;
    AppLogger& m_logger;
};
