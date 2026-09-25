#include "ComponentRepository.h"
#include "AppDatabase.h"
#include "SbomDocument.h"

#include <QScopeGuard>
#include <QHash>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

QString ComponentRepository::databaseFilePath() const { return m_database.filePath(); }

ComponentResult ComponentRepository::replaceInFile(const QString& filePath, const QString& projectId, const SbomDocument& document)
{
    AppDatabase database;
    QString error;
    if (!database.open(filePath, error)) return {ComponentError::Database, error};
    ComponentRepository repository(database);
    return repository.replaceForProject(projectId, document);
}

QString ComponentResult::userMessage() const
{
    switch (error) {
    case ComponentError::None: return {};
    case ComponentError::ProjectNotFound: return QStringLiteral("该项目已不存在，请重新选择项目。");
    case ComponentError::Database: return QStringLiteral("无法读写当前 SBOM 数据，请检查磁盘和访问权限后重试；应用失败时原组件及依赖保持不变。");
    }
    return {};
}

ComponentResult ComponentRepository::listForProject(const QString& projectId, QList<Component>& components) const
{
    components.clear();
    if (!m_database.isOpen()) return {ComponentError::Database, QStringLiteral("Database is not open")};
    QSqlQuery query(m_database.connection());
    if (!query.prepare(QStringLiteral("SELECT id,project_id,source_role,source_order,bom_ref,type,name,version,purl "
        "FROM components WHERE project_id=? ORDER BY source_role,source_order")))
        return {ComponentError::Database, query.lastError().text()};
    query.addBindValue(projectId);
    if (!query.exec()) return {ComponentError::Database, query.lastError().text()};
    while (query.next()) {
        components.append({query.value(0).toString(), query.value(1).toString(),
            static_cast<ComponentSourceRole>(query.value(2).toInt()), query.value(3).toLongLong(),
            query.value(4).toString(), query.value(5).toString(), query.value(6).toString(),
            query.value(7).toString(), query.value(8).toString()});
    }
    if (query.lastError().isValid()) {
        components.clear();
        return {ComponentError::Database, query.lastError().text()};
    }
    return {};
}

ComponentResult ComponentRepository::replaceForProject(const QString& projectId, const SbomDocument& document)
{
    if (!m_database.isOpen()) return {ComponentError::Database, QStringLiteral("Database is not open")};
    auto db = m_database.connection();
    if (!db.transaction()) return {ComponentError::Database, db.lastError().text()};
    auto rollback = qScopeGuard([&] { db.rollback(); });
    QSqlQuery query(db);
    if (!query.prepare(QStringLiteral("SELECT id FROM projects WHERE id=?")))
        return {ComponentError::Database, query.lastError().text()};
    query.addBindValue(projectId);
    if (!query.exec()) return {ComponentError::Database, query.lastError().text()};
    if (!query.next()) return query.lastError().isValid()
        ? ComponentResult{ComponentError::Database, query.lastError().text()}
        : ComponentResult{ComponentError::ProjectNotFound, {}};
    query.finish();
    // Targets cascade from entries. The capture marker is replaced in this same transaction.
    for (const auto& sql : {QStringLiteral("DELETE FROM dependency_entries WHERE project_id=?"),
                           QStringLiteral("DELETE FROM dependency_capture WHERE project_id=?")}) {
        if (!query.prepare(sql)) return {ComponentError::Database, query.lastError().text()};
        query.addBindValue(projectId);
        if (!query.exec()) return {ComponentError::Database, query.lastError().text()};
    }
    if (!query.prepare(QStringLiteral("DELETE FROM components WHERE project_id=?")))
        return {ComponentError::Database, query.lastError().text()};
    query.addBindValue(projectId);
    if (!query.exec()) return {ComponentError::Database, query.lastError().text()};
    if (!query.prepare(QStringLiteral("INSERT INTO components "
        "(id,project_id,source_role,source_order,bom_ref,type,name,version,purl) VALUES (?,?,?,?,?,?,?,?,?)")))
        return {ComponentError::Database, query.lastError().text()};
    const auto insert = [&](const SbomComponent& c, ComponentSourceRole role, qint64 order) {
        query.bindValue(0, QUuid::createUuid().toString(QUuid::WithoutBraces));
        query.bindValue(1, projectId);
        query.bindValue(2, static_cast<int>(role));
        query.bindValue(3, order);
        // QString null denotes a missing optional parser field; persist it as empty TEXT, not SQL NULL.
        int index = 4;
        for (const auto* value : {&c.bomRef, &c.type, &c.name, &c.version, &c.purl})
            query.bindValue(index++, value->isNull() ? QStringLiteral("") : *value);
        return query.exec();
    };
    if (document.metadata.component && !insert(*document.metadata.component, ComponentSourceRole::MetadataRoot, 0))
        return {ComponentError::Database, query.lastError().text()};
    for (qsizetype i = 0; i < document.components.size(); ++i)
        if (!insert(document.components.at(i), ComponentSourceRole::Component, i))
            return {ComponentError::Database, query.lastError().text()};
    query.finish();
    QSqlQuery entry(db), target(db);
    if (!entry.prepare(QStringLiteral("INSERT INTO dependency_entries(id,project_id,source_order,source_ref) VALUES(?,?,?,?)")))
        return {ComponentError::Database, entry.lastError().text()};
    if (!target.prepare(QStringLiteral("INSERT INTO dependency_targets(id,dependency_entry_id,target_order,target_ref) VALUES(?,?,?,?)")))
        return {ComponentError::Database, target.lastError().text()};
    const auto text = [](const QString& value) { return value.isNull() ? QStringLiteral("") : value; };
    for (qsizetype i = 0; i < document.dependencies.size(); ++i) {
        const auto& declaration = document.dependencies.at(i);
        const auto entryId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        entry.bindValue(0, entryId);
        entry.bindValue(1, projectId);
        entry.bindValue(2, qint64(i));
        entry.bindValue(3, text(declaration.ref));
        if (!entry.exec()) return {ComponentError::Database, entry.lastError().text()};
        for (qsizetype j = 0; j < declaration.dependsOn.size(); ++j) {
            target.bindValue(0, QUuid::createUuid().toString(QUuid::WithoutBraces));
            target.bindValue(1, entryId);
            target.bindValue(2, qint64(j));
            target.bindValue(3, text(declaration.dependsOn.at(j)));
            if (!target.exec()) return {ComponentError::Database, target.lastError().text()};
        }
    }
    entry.finish();
    target.finish();
    if (!query.prepare(QStringLiteral("INSERT INTO dependency_capture(project_id) VALUES(?)")))
        return {ComponentError::Database, query.lastError().text()};
    query.addBindValue(projectId);
    if (!query.exec()) return {ComponentError::Database, query.lastError().text()};
    query.finish();
    if (!db.commit()) return {ComponentError::Database, db.lastError().text()};
    rollback.dismiss();
    return {};
}

ComponentResult ComponentRepository::readSnapshotInFile(const QString& filePath, const QString& projectId, DependencySnapshot& snapshot)
{
    snapshot = {};
    AppDatabase database;
    QString error;
    if (!database.open(filePath, error)) return {ComponentError::Database, error};
    return ComponentRepository(database).readSnapshot(projectId, snapshot);
}

ComponentResult ComponentRepository::listForProjectInFile(const QString& filePath, const QString& projectId, QList<Component>& components)
{
    components.clear();
    AppDatabase database;
    QString error;
    if (!database.open(filePath,error)) return {ComponentError::Database,error};
    auto db = database.connection();
    if (!db.transaction()) return {ComponentError::Database,db.lastError().text()};
    auto rollback = qScopeGuard([&]{ db.rollback(); });
    QSqlQuery query(db);
    if (!query.prepare(QStringLiteral("SELECT id FROM projects WHERE id=?"))) return {ComponentError::Database,query.lastError().text()};
    query.addBindValue(projectId);
    if (!query.exec()) return {ComponentError::Database,query.lastError().text()};
    if (!query.next()) return query.lastError().isValid() ? ComponentResult{ComponentError::Database,query.lastError().text()}
        : ComponentResult{ComponentError::ProjectNotFound,{}};
    query.finish();
    QList<Component> candidate;
    const auto result = ComponentRepository(database).listForProject(projectId,candidate);
    if (!result.ok()) return result;
    if (!db.commit()) return {ComponentError::Database,db.lastError().text()};
    rollback.dismiss(); components = std::move(candidate); return {};
}

ComponentResult ComponentRepository::readSnapshot(const QString& projectId, DependencySnapshot& snapshot) const
{
    snapshot = {};
    if (!m_database.isOpen()) return {ComponentError::Database, QStringLiteral("Database is not open")};
    auto db = m_database.connection();
    if (!db.transaction()) return {ComponentError::Database, db.lastError().text()};
    auto rollback = qScopeGuard([&] { db.rollback(); });
    // The first SELECT establishes SQLite's snapshot; all subsequent SELECTs share it.
    QSqlQuery query(db);
    if (!query.prepare(QStringLiteral("SELECT id FROM projects WHERE id=?")))
        return {ComponentError::Database, query.lastError().text()};
    query.addBindValue(projectId);
    if (!query.exec()) return {ComponentError::Database, query.lastError().text()};
    if (!query.next()) return query.lastError().isValid()
        ? ComponentResult{ComponentError::Database, query.lastError().text()}
        : ComponentResult{ComponentError::ProjectNotFound, {}};
    query.finish();
    DependencySnapshot candidate;
    const auto componentsResult = listForProject(projectId, candidate.components);
    if (!componentsResult.ok()) return componentsResult;
    if (!query.prepare(QStringLiteral("SELECT project_id FROM dependency_capture WHERE project_id=?")))
        return {ComponentError::Database, query.lastError().text()};
    query.addBindValue(projectId);
    if (!query.exec()) return {ComponentError::Database, query.lastError().text()};
    candidate.captured = query.next();
    if (query.lastError().isValid()) return {ComponentError::Database, query.lastError().text()};
    query.finish();
    if (!query.prepare(QStringLiteral("SELECT id,project_id,source_order,source_ref FROM dependency_entries WHERE project_id=? ORDER BY source_order")))
        return {ComponentError::Database, query.lastError().text()};
    query.addBindValue(projectId);
    if (!query.exec()) return {ComponentError::Database, query.lastError().text()};
    QHash<QString, qsizetype> positions;
    while (query.next()) {
        positions.insert(query.value(0).toString(), candidate.entries.size());
        candidate.entries.append({query.value(0).toString(),query.value(1).toString(),
            query.value(2).toLongLong(),query.value(3).toString(),{}});
    }
    if (query.lastError().isValid()) return {ComponentError::Database, query.lastError().text()};
    query.finish();
    if (!query.prepare(QStringLiteral("SELECT t.id,t.dependency_entry_id,t.target_order,t.target_ref "
        "FROM dependency_targets t JOIN dependency_entries e ON e.id=t.dependency_entry_id "
        "WHERE e.project_id=? ORDER BY e.source_order,t.target_order")))
        return {ComponentError::Database, query.lastError().text()};
    query.addBindValue(projectId);
    if (!query.exec()) return {ComponentError::Database, query.lastError().text()};
    while (query.next()) {
        const auto entryId = query.value(1).toString();
        const auto position = positions.constFind(entryId);
        if (position == positions.cend()) return {ComponentError::Database, QStringLiteral("Inconsistent dependency entry")};
        candidate.entries[*position].targets.append({query.value(0).toString(),entryId,
            query.value(2).toLongLong(),query.value(3).toString()});
    }
    if (query.lastError().isValid()) return {ComponentError::Database, query.lastError().text()};
    query.finish();
    if (!db.commit()) return {ComponentError::Database, db.lastError().text()};
    rollback.dismiss();
    snapshot = std::move(candidate);
    return {};
}
