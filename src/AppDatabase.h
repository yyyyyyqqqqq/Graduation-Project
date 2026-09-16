#pragma once

#include <QSqlDatabase>

// Owns one connection on the calling thread. Queries/borrowed handles must be
// destroyed before close(); never share this object across threads.
class AppDatabase final
{
public:
    static constexpr int SchemaVersion = 2;
    AppDatabase();
    ~AppDatabase();
    AppDatabase(const AppDatabase&) = delete;
    AppDatabase& operator=(const AppDatabase&) = delete;

    bool open(const QString& filePath, QString& error);
    void close();
    bool isOpen() const;
    QString connectionName() const;
    QSqlDatabase connection() const;

private:
    bool initializeSchema(QString& error);
    bool migrateV1ToV2(QString& error);
    const QString m_connectionName;
    QSqlDatabase m_database;
};
