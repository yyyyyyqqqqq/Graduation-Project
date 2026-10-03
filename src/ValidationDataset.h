#pragma once
#include "RiskPriority.h"
#include <QJsonArray>

namespace Validation
{
QString sha256(const QByteArray &);
QString arrayHash(const QJsonArray &);
QByteArray readFile(const QString &path);
bool writeFrozen(const QString &path, const QJsonObject &, QString &error);
QJsonObject profileJson(const RiskEvidenceProfile &);
std::optional<RiskEvidenceProfile> readProfile(const QJsonObject &, QString &error);

struct Sample
{
    QString id, stratum, clusterHash;
    QStringList clusterIds;
    VulnerabilityCandidate candidate;
    RiskEvidenceProfile profile;
};
struct Dataset
{
    QJsonObject metadata;
    QString hash, poolHash, indexHash;
    QDateTime referenceTime;
    QList<Sample> samples;
};
// Dataset-local validation for the product: no pool/index IO or selection-history claim.
std::optional<Dataset> loadRuntimeDataset(const QByteArray &, const QByteArray &, QString &error);
std::optional<Dataset> loadRuntimeDirectory(const QString &, QString &error);
// Full research audit. A dataset is accepted atomically; no bad sample is skipped.
std::optional<Dataset> loadDataset(const QByteArray &dataset, const QByteArray &datasetSidecar,
                                   const QByteArray &pool, const QByteArray &poolSidecar,
                                   const QByteArray &index, const QByteArray &indexSidecar, QString &error);
std::optional<Dataset> loadDirectory(const QString &, QString &error);
QJsonObject diversity(const QList<Sample> &);
QString primaryStratum(const RiskEvidenceProfile &, QDateTime);
} // namespace Validation
