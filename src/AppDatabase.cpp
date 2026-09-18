#include "AppDatabase.h"

#include <QScopeGuard>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

AppDatabase::AppDatabase()
    : m_connectionName(QStringLiteral("application-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)))
{
}

AppDatabase::~AppDatabase() { close(); }

bool AppDatabase::open(const QString& filePath, QString& error)
{
    close();
    error.clear();
    m_database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_database.setDatabaseName(filePath);
    if (!m_database.open()) {
        error = QStringLiteral("Cannot open database: %1").arg(m_database.lastError().text());
        close();
        return false;
    }
    bool foreignKeysEnabled = false;
    {
        QSqlQuery query(m_database);
        foreignKeysEnabled = query.exec(QStringLiteral("PRAGMA foreign_keys=ON"))
            && query.exec(QStringLiteral("PRAGMA foreign_keys")) && query.next() && query.value(0).toInt() == 1;
    }
    if (!foreignKeysEnabled) {
        error = QStringLiteral("Cannot enable SQLite foreign keys");
        close();
        return false;
    }
    if (!initializeSchema(error)) {
        close();
        return false;
    }
    return true;
}

bool AppDatabase::initializeSchema(QString& error)
{
    if (!m_database.transaction()) {
        error = m_database.lastError().text();
        return false;
    }
    auto rollback = qScopeGuard([this] { m_database.rollback(); });
    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral("SELECT COUNT(*) FROM sqlite_master "
                                   "WHERE type IN ('table', 'view') AND name NOT LIKE 'sqlite_%'")) || !query.next()) {
        error = QStringLiteral("Cannot inspect database schema: %1").arg(query.lastError().text());
        return false;
    }
    const bool empty = query.value(0).toInt() == 0;
    query.finish();
    if (empty) {
        if (!query.exec(QStringLiteral("CREATE TABLE app_meta (key TEXT PRIMARY KEY NOT NULL, value TEXT NOT NULL)")) ||
            !query.exec(QStringLiteral("INSERT INTO app_meta (key, value) VALUES ('schema_version', '1')"))) {
            error = QStringLiteral("Cannot initialize schema: %1").arg(query.lastError().text());
            return false;
        }
    }
    if (!query.exec(QStringLiteral("SELECT value FROM app_meta WHERE key = 'schema_version'")) || !query.next()) {
        error = QStringLiteral("Missing or unreadable schema_version: %1").arg(query.lastError().text());
        return false;
    }
    QString version = query.value(0).toString();
    query.finish();
    if (version == QStringLiteral("1")) {
        if (!migrateV1ToV2(error)) {
            return false;
        }
        version = QStringLiteral("2");
    } else if (version != QStringLiteral("2") && version != QStringLiteral("3") && version != QString::number(SchemaVersion)) {
        error = QStringLiteral("Unsupported schema_version '%1'; supported versions are 1 through 4. No migration was performed.").arg(version);
        return false;
    }
    // Reject incomplete v2 databases instead of silently recreating missing user tables.
    if (!query.exec(QStringLiteral("SELECT id, name, description, created_at FROM projects LIMIT 0"))) {
        error = QStringLiteral("Invalid projects schema: %1").arg(query.lastError().text());
        return false;
    }
    query.finish();
    if (version == QStringLiteral("2")) {
        if (!migrateV2ToV3(error)) return false;
        version = QStringLiteral("3");
    }
    if (!query.exec(QStringLiteral("SELECT id,project_id,source_role,source_order,bom_ref,type,name,version,purl FROM components LIMIT 0"))) {
        error = QStringLiteral("Invalid components schema: %1").arg(query.lastError().text());
        return false;
    }
    query.finish();
    if (version == QStringLiteral("3") && !migrateV3ToV4(error)) return false;
    for (const auto& sql : {QStringLiteral("SELECT project_id FROM dependency_capture LIMIT 0"),
                           QStringLiteral("SELECT id,project_id,source_order,source_ref FROM dependency_entries LIMIT 0"),
                           QStringLiteral("SELECT id,dependency_entry_id,target_order,target_ref FROM dependency_targets LIMIT 0")}) {
        if (!query.exec(sql)) {
            error = QStringLiteral("Invalid dependency schema: %1").arg(query.lastError().text());
            return false;
        }
        query.finish();
    }
    if (!m_database.commit()) {
        error = QStringLiteral("Cannot commit schema initialization: %1").arg(m_database.lastError().text());
        return false;
    }
    rollback.dismiss();
    return true;
}

bool AppDatabase::migrateV1ToV2(QString& error)
{
    // The caller owns the transaction: schema and metadata commit or roll back together.
    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral("CREATE TABLE projects ("
                                   "id TEXT PRIMARY KEY NOT NULL, name TEXT NOT NULL, "
                                   "description TEXT NOT NULL DEFAULT '', created_at INTEGER NOT NULL)"))) {
        error = QStringLiteral("Migration 1 -> 2: cannot create projects: %1").arg(query.lastError().text());
        return false;
    }
    if (!query.exec(QStringLiteral("UPDATE app_meta SET value='2' WHERE key='schema_version'")) ||
        query.numRowsAffected() != 1) {
        error = QStringLiteral("Migration 1 -> 2: cannot update schema_version: %1").arg(query.lastError().text());
        return false;
    }
    return true;
}

bool AppDatabase::migrateV2ToV3(QString& error)
{
    // No IF NOT EXISTS: a conflicting user table must abort the entire migration.
    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral("CREATE TABLE components ("
        "id TEXT PRIMARY KEY NOT NULL, project_id TEXT NOT NULL, "
        "source_role INTEGER NOT NULL CHECK(source_role IN (0,1)), "
        "source_order INTEGER NOT NULL CHECK(source_order>=0 AND (source_role=1 OR source_order=0)), "
        "bom_ref TEXT NOT NULL DEFAULT '', type TEXT NOT NULL DEFAULT '', name TEXT NOT NULL DEFAULT '', "
        "version TEXT NOT NULL DEFAULT '', purl TEXT NOT NULL DEFAULT '', "
        "FOREIGN KEY(project_id) REFERENCES projects(id) ON DELETE CASCADE)")) ||
        // Uniqueness describes a storage position, never software package identity.
        !query.exec(QStringLiteral("CREATE UNIQUE INDEX components_project_source ON components(project_id,source_role,source_order)")) ||
        !query.exec(QStringLiteral("UPDATE app_meta SET value='3' WHERE key='schema_version'")) ||
        query.numRowsAffected() != 1) {
        error = QStringLiteral("Migration 2 -> 3 failed: %1").arg(query.lastError().text());
        return false;
    }
    return true;
}

bool AppDatabase::migrateV3ToV4(QString& error)
{
    // Absence of a capture row means Not Captured, including all migrated projects.
    // The initialization transaction owns every DDL statement and version update.
    QSqlQuery query(m_database);
    const QStringList statements{
        QStringLiteral("CREATE TABLE dependency_capture (project_id TEXT PRIMARY KEY NOT NULL, "
                       "FOREIGN KEY(project_id) REFERENCES projects(id) ON DELETE CASCADE)"),
        QStringLiteral("CREATE TABLE dependency_entries (id TEXT PRIMARY KEY NOT NULL, project_id TEXT NOT NULL, "
                       "source_order INTEGER NOT NULL CHECK(source_order>=0), source_ref TEXT NOT NULL DEFAULT '', "
                       "FOREIGN KEY(project_id) REFERENCES projects(id) ON DELETE CASCADE)"),
        QStringLiteral("CREATE UNIQUE INDEX dependency_entries_project_order ON dependency_entries(project_id,source_order)"),
        QStringLiteral("CREATE TABLE dependency_targets (id TEXT PRIMARY KEY NOT NULL, dependency_entry_id TEXT NOT NULL, "
                       "target_order INTEGER NOT NULL CHECK(target_order>=0), target_ref TEXT NOT NULL DEFAULT '', "
                       "FOREIGN KEY(dependency_entry_id) REFERENCES dependency_entries(id) ON DELETE CASCADE)"),
        QStringLiteral("CREATE UNIQUE INDEX dependency_targets_entry_order ON dependency_targets(dependency_entry_id,target_order)"),
        QStringLiteral("UPDATE app_meta SET value='4' WHERE key='schema_version'")};
    for (const auto& sql : statements) {
        if (!query.exec(sql)) {
            error = QStringLiteral("Migration 3 -> 4 failed: %1").arg(query.lastError().text());
            return false;
        }
    }
    if (query.numRowsAffected() != 1) {
        error = QStringLiteral("Migration 3 -> 4: missing schema_version");
        return false;
    }
    return true;
}

void AppDatabase::close()
{
    if (m_database.isValid()) {
        m_database.close();
        m_database = QSqlDatabase();
    }
    if (QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

bool AppDatabase::isOpen() const { return m_database.isOpen(); }
QString AppDatabase::connectionName() const { return m_connectionName; }
QSqlDatabase AppDatabase::connection() const { return m_database; }
QString AppDatabase::filePath() const { return m_database.databaseName(); }
