#include "ComponentRepository.h"
#include "AppDatabase.h"
#include "SbomDocument.h"

#include <QScopeGuard>
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
    case ComponentError::Database: return QStringLiteral("无法读写组件数据，请检查磁盘和访问权限后重试；应用失败时原组件保持不变。");
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
    if (!db.commit()) return {ComponentError::Database, db.lastError().text()};
    rollback.dismiss();
    return {};
}
