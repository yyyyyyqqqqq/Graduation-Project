#include "ProjectRepository.h"
#include "AppDatabase.h"
#include "AppLogger.h"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

namespace
{
Project readProject(const QSqlQuery& query)
{
    return {query.value(0).toString(), query.value(1).toString(),
            query.value(2).toString(), query.value(3).toLongLong()};
}
}

QString ProjectResult::userMessage() const
{
    switch (error) {
    case ProjectError::None: return {};
    case ProjectError::InvalidName: return QStringLiteral("项目名称去除首尾空白后须为 1—100 个字符。");
    case ProjectError::InvalidDescription: return QStringLiteral("项目描述去除首尾空白后最多为 500 个字符。");
    case ProjectError::NotFound: return QStringLiteral("该项目已不存在，请重新选择项目。");
    case ProjectError::Database: return QStringLiteral("无法读写项目数据，请检查磁盘和访问权限后重试。详情见本地日志。");
    }
    return {};
}

ProjectRepository::ProjectRepository(AppDatabase& database, AppLogger& logger)
    : m_database(database), m_logger(logger)
{
}

ProjectResult ProjectRepository::databaseError(const QString& operation, const QString& diagnostic) const
{
    m_logger.write(AppLogger::Level::Error, QStringLiteral("Project %1 failed: %2").arg(operation, diagnostic));
    return {ProjectError::Database, diagnostic};
}

ProjectResult ProjectRepository::create(const QString& name, const QString& description, Project& project)
{
    project = {};
    const QString normalizedName = name.trimmed();
    const QString normalizedDescription = description.trimmed();
    // Count Unicode code points, so non-BMP characters are not counted twice.
    if (normalizedName.isEmpty() || normalizedName.toUcs4().size() > 100) {
        return {ProjectError::InvalidName, {}};
    }
    if (normalizedDescription.toUcs4().size() > 500) {
        return {ProjectError::InvalidDescription, {}};
    }
    if (!m_database.isOpen()) {
        return databaseError(QStringLiteral("create"), QStringLiteral("Database is not open"));
    }
    const Project candidate{QUuid::createUuid().toString(QUuid::WithoutBraces), normalizedName,
                            normalizedDescription.isEmpty() ? QStringLiteral("") : normalizedDescription,
                            QDateTime::currentMSecsSinceEpoch()};
    QSqlQuery query(m_database.connection());
    if (!query.prepare(QStringLiteral("INSERT INTO projects (id, name, description, created_at) VALUES (?, ?, ?, ?)"))) {
        return databaseError(QStringLiteral("create"), query.lastError().text());
    }
    query.addBindValue(candidate.id);
    query.addBindValue(candidate.name);
    query.addBindValue(candidate.description);
    query.addBindValue(candidate.createdAt);
    if (!query.exec()) {
        return databaseError(QStringLiteral("create"), query.lastError().text());
    }
    project = candidate;
    m_logger.write(AppLogger::Level::Info, QStringLiteral("Project created, id=%1").arg(project.id));
    return {};
}

ProjectResult ProjectRepository::list(QList<Project>& projects) const
{
    projects.clear();
    if (!m_database.isOpen()) {
        return databaseError(QStringLiteral("list"), QStringLiteral("Database is not open"));
    }
    QSqlQuery query(m_database.connection());
    if (!query.exec(QStringLiteral("SELECT id, name, description, created_at FROM projects ORDER BY created_at DESC, id DESC"))) {
        return databaseError(QStringLiteral("list"), query.lastError().text());
    }
    while (query.next()) {
        projects.append(readProject(query));
    }
    if (query.lastError().isValid()) {
        projects.clear();
        return databaseError(QStringLiteral("list"), query.lastError().text());
    }
    return {};
}

ProjectResult ProjectRepository::findById(const QString& id, Project& project) const
{
    project = {};
    if (!m_database.isOpen()) {
        return databaseError(QStringLiteral("find"), QStringLiteral("Database is not open"));
    }
    QSqlQuery query(m_database.connection());
    if (!query.prepare(QStringLiteral("SELECT id, name, description, created_at FROM projects WHERE id=?"))) {
        return databaseError(QStringLiteral("find"), query.lastError().text());
    }
    query.addBindValue(id);
    if (!query.exec()) {
        return databaseError(QStringLiteral("find"), query.lastError().text());
    }
    if (!query.next()) {
        return query.lastError().isValid() ? databaseError(QStringLiteral("find"), query.lastError().text())
                                          : ProjectResult{ProjectError::NotFound, {}};
    }
    project = readProject(query);
    return {};
}

ProjectResult ProjectRepository::remove(const QString& id)
{
    if (!m_database.isOpen()) {
        return databaseError(QStringLiteral("remove"), QStringLiteral("Database is not open"));
    }
    QSqlQuery query(m_database.connection());
    if (!query.prepare(QStringLiteral("DELETE FROM projects WHERE id=?"))) {
        return databaseError(QStringLiteral("remove"), query.lastError().text());
    }
    query.addBindValue(id);
    if (!query.exec()) {
        return databaseError(QStringLiteral("remove"), query.lastError().text());
    }
    if (query.numRowsAffected() == 0) {
        return {ProjectError::NotFound, {}};
    }
    m_logger.write(AppLogger::Level::Info, QStringLiteral("Project deleted, id=%1").arg(id));
    return {};
}
