#pragma once
#include "RiskEvidence.h"

struct EpssCacheRead {
    QueryError error = QueryError::CacheMiss;
    std::optional<EpssSnapshot> snapshot;
};
struct KevCacheRead {
    QueryError error = QueryError::CacheMiss;
    std::optional<KevSnapshot> snapshot;
};
// Separate per-CVE FIRST snapshots and complete CISA catalog. No derived profile.
class EpssCache final {
public:
    explicit EpssCache(QString root):m_root(std::move(root)){}
    QString filePath(const QString& cve) const;
    EpssCacheRead read(const QString&, QDateTime now=QDateTime::currentDateTimeUtc()) const;
    QueryError write(const EpssSnapshot&) const;
private:
    QString m_root;
};
class KevCache final {
public:
    explicit KevCache(QString root):m_root(std::move(root)){}
    QString filePath() const;
    KevCacheRead read(QDateTime now=QDateTime::currentDateTimeUtc()) const;
    QueryError write(const KevSnapshot&) const;
private:
    QString m_root;
};
