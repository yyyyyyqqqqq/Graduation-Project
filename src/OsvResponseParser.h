#pragma once

#include "PackageIdentity.h"
#include <QDateTime>
#include <QJsonObject>
#include <QList>

enum class QueryError {
    None, RequestRejected, AccessDenied, EndpointNotFound, Timeout, TlsFailure,
    ConnectionFailure, RateLimited, ServiceUnavailable, ResponseInvalid,
    ResponseLimitExceeded, UnexpectedRedirect, Cancelled, CacheMiss, CacheInvalid,
    CacheIo, Database
};
QString queryErrorText(QueryError error);
QString queryErrorCode(QueryError error);

// Original record is authoritative; metadata/evidence accessors never evaluate ranges.
struct VulnerabilityCandidate
{
    QJsonObject record;
    QString id() const { return record.value(QStringLiteral("id")).toString(); }
    QString text(const QString& field) const { return record.value(field).toString(); }
    QStringList cveAliases() const;
};
struct OsvSnapshot
{
    QueryIdentity identity;
    QDateTime fetchedAt;
    QList<VulnerabilityCandidate> candidates;
};
struct OsvPageResult
{
    QueryError error = QueryError::None;
    QList<VulnerabilityCandidate> candidates;
    QString nextToken;
    bool ok() const { return error==QueryError::None; }
};
class OsvResponseParser final
{
public:
    static constexpr qint64 MaxResponseBytes = 16 * 1024 * 1024;
    static constexpr qsizetype MaxRecords = 10000;
    static constexpr int MaxPages = 20;
    static OsvPageResult parse(const QByteArray& bytes);
    static bool validTimestamp(const QString& value);
    // UTC timestamps retain submillisecond precision for cross-page deduplication.
    static int compareTimestamp(const QString& a, const QString& b);
    static QueryError merge(QList<VulnerabilityCandidate>& current, const QList<VulnerabilityCandidate>& page);
};
