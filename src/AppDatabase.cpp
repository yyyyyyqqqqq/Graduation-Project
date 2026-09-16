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
    const QString version = query.value(0).toString();
    if (version != QStringLiteral("1")) {
        error = QStringLiteral("Unsupported schema_version '%1'; expected 1. No migration was performed.").arg(version);
        return false;
    }
    query.finish();
    if (!m_database.commit()) {
        error = QStringLiteral("Cannot commit schema initialization: %1").arg(m_database.lastError().text());
        return false;
    }
    rollback.dismiss();
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
