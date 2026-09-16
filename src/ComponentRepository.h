#pragma once

#include "Component.h"
#include <QList>

class AppDatabase;
struct SbomDocument;

enum class ComponentError { None, ProjectNotFound, Database };
struct ComponentResult
{
    ComponentError error = ComponentError::None;
    QString diagnostic; // Internal only; never display/log raw driver text containing input.
    bool ok() const { return error == ComponentError::None; }
    QString userMessage() const;
};

// Borrows AppDatabase on its owning thread; no retained SQL handle/query or cache.
class ComponentRepository final
{
public:
    explicit ComponentRepository(AppDatabase& database) : m_database(database) {}
    ComponentResult listForProject(const QString& projectId, QList<Component>& components) const;
    ComponentResult replaceForProject(const QString& projectId, const SbomDocument& document);
    QString databaseFilePath() const;
    // Called on a worker: owns its AppDatabase/queries entirely within that thread.
    static ComponentResult replaceInFile(const QString& filePath, const QString& projectId, const SbomDocument& document);
private:
    AppDatabase& m_database;
};
