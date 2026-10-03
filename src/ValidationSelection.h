#pragma once
#include "ValidationDataset.h"

namespace Validation
{
struct CandidateUnit
{
    QueryIdentity identity;
    VulnerabilityCandidate representative;
    QStringList clusterIds;
    QString clusterHash, selectionHash;
};
struct Discovery
{
    QList<CandidateUnit> units;
    QJsonArray exclusions;
};
// Research preparation only: no use by production matching/controllers.
Discovery discover(const QString &ecosystem, const QJsonArray &records);
QJsonObject unitJson(const CandidateUnit &);
std::optional<CandidateUnit> readUnit(const QJsonObject &);
RiskEvidenceProfile prepareProfile(const CandidateUnit &, QDateTime snapshotTime,
                                   const QHash<QString, EpssSnapshot> &, const std::optional<KevSnapshot> &,
                                   QDateTime generatedAt);
} // namespace Validation
