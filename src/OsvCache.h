#pragma once
#include "OsvResponseParser.h"

inline constexpr auto OsvEndpoint = "https://api.osv.dev/v1/query";
struct CacheResult {
    QueryError error = QueryError::CacheMiss;
    std::optional<OsvSnapshot> snapshot;
    bool fresh = false;
};
// Value object. Each call runs on its caller's worker; no shared handles or index.
class OsvCache final {
public:
    explicit OsvCache(QString directory) : m_directory(std::move(directory)) {}
    static constexpr qsizetype MaxFileBytes = 20 * 1024 * 1024;
    QString filePath(const QueryIdentity&) const;
    CacheResult read(const QueryIdentity&, QDateTime now = QDateTime::currentDateTimeUtc()) const;
    QueryError write(const OsvSnapshot&) const;
    QueryError clear() const;
private:
    bool safeDirectory() const;
    QString m_directory;
};
