#pragma once
#include "VersionApplicability.h"
#include "DependencyAnalyzer.h"
#include <QHash>
#include <QDate>

enum class EvidenceFreshness { Fresh, Stale, NotApplicable };
enum class EvidenceAcquisition { Live, Cache, StaleFallback, None };
enum class SeverityStatus { Present, UnsupportedType, InvalidStructure, SchemaConflict, Missing };
enum class SeverityScope { TopLevel, MatchingAffected };
enum class SeverityProvenance { ExplicitSource, ImplicitHomeDatabase };
struct SeverityEvidenceItem {
    QString type, vector, source, recordId;
    SeverityScope scope = SeverityScope::TopLevel;
    SeverityProvenance provenance = SeverityProvenance::ImplicitHomeDatabase;
    std::optional<qsizetype> affectedIndex;
    SeverityStatus status = SeverityStatus::Missing;
};
struct SeverityEvidence {
    SeverityStatus status = SeverityStatus::Missing;
    QList<SeverityEvidenceItem> items;
};
enum class EpssStatus { Available, NotScored, Failed, InvalidResponse, NotQueryable };
struct EpssSnapshot {
    QString cve, providerVersion;
    QDateTime fetchedAt;
    // Empty only for a complete successful per-CVE NotScored result.
    QJsonObject record;
};
struct EpssEvidence {
    QString cve;
    EpssStatus status = EpssStatus::Failed;
    EvidenceFreshness freshness = EvidenceFreshness::NotApplicable;
    EvidenceAcquisition acquisition = EvidenceAcquisition::None;
    std::optional<double> probability, percentile;
    QDate scoreDate;
    QDateTime fetchedAt;
    QString providerVersion;
    QueryError error = QueryError::None, cacheError = QueryError::None;
};
struct KevSnapshot {
    QDateTime fetchedAt;
    QJsonObject catalog;
    QHash<QString,QJsonObject> entries;
};
enum class KevStatus { Listed, NotListed, Unknown, NotQueryable };
struct KevEvidence {
    QString cve;
    KevStatus status = KevStatus::Unknown;
    EvidenceFreshness freshness = EvidenceFreshness::NotApplicable;
    EvidenceAcquisition acquisition = EvidenceAcquisition::None;
    QString catalogVersion, dateReleased;
    QDateTime fetchedAt;
    QJsonObject entry;
    QueryError error = QueryError::None, cacheError = QueryError::None;
};
enum class DependencyPathState { RootComponentSelf, ResolvedPathFound, NoResolvedPath, RootMissing, RootAmbiguous, NotCaptured };
enum class DependencyRootStatus { RootAvailable, RootMissing, RootAmbiguous, NotCaptured };
enum class ReferenceResolutionCompleteness { Complete, Partial };
struct EvidenceDependencyContext {
    bool captured = false;
    DependencyRootStatus root = DependencyRootStatus::NotCaptured;
    DependencyPathState path = DependencyPathState::NotCaptured;
    std::optional<int> depthFromRoot;
    std::optional<qsizetype> directDependents, transitiveDependents;
    std::optional<ReferenceResolutionCompleteness> referenceResolution;
};
enum class QualityEvidenceStatus { UnavailableForPersistedCurrentState };
struct RiskEvidenceKey {
    QString componentId, osvId;
    QueryIdentity snapshotIdentity;
    QDateTime snapshotFetchedAt;
    quint64 applicabilityGeneration = 0;
    bool operator==(const RiskEvidenceKey&) const = default;
};

// Only the authoritative Phase 08 controller can issue this immutable input.
// No JSON is added to VulnerabilityFinding; the copy lasts for this request.
class VulnerabilityController;
class RiskEvidenceRequest final {
public:
    const RiskEvidenceKey& key() const { return m_key; }
    const VulnerabilityFinding& finding() const { return m_finding; }
    const VulnerabilityCandidate& candidate() const { return m_candidate; }
    const QString& databasePath() const { return m_databasePath; }
    const QString& projectId() const { return m_projectId; }
    static bool validOwnership(const Component&, const OsvSnapshot&, const ApplicabilitySnapshot&,
                               quint64 generation, qsizetype candidateIndex, const VulnerabilityFinding&);
private:
    friend class VulnerabilityController;
    RiskEvidenceRequest(RiskEvidenceKey key, VulnerabilityFinding finding, VulnerabilityCandidate candidate,
                        QString databasePath, QString projectId)
        : m_key(std::move(key)), m_finding(std::move(finding)), m_candidate(std::move(candidate)),
          m_databasePath(std::move(databasePath)), m_projectId(std::move(projectId)) {}
    RiskEvidenceKey m_key;
    VulnerabilityFinding m_finding;
    VulnerabilityCandidate m_candidate;
    QString m_databasePath, m_projectId;
};
enum class EvidenceOperationState { NotStarted, Loading, AwaitingConsent, Refreshing, Complete, Cancelled, Failed, Stale };
enum class EvidenceLoadMode { PreferCache, Refresh, CacheOnly };
struct RiskEvidenceProfile {
    RiskEvidenceKey key;
    SeverityEvidence severity;
    QList<EpssEvidence> epss;
    QList<KevEvidence> kev;
    EvidenceDependencyContext dependency;
    QualityEvidenceStatus quality = QualityEvidenceStatus::UnavailableForPersistedCurrentState;
    QDateTime generatedAt;
    EvidenceOperationState operation = EvidenceOperationState::Complete;
};
namespace RiskEvidence {
constexpr qsizetype MaxEpssBytes = 2 * 1024 * 1024;
constexpr qsizetype MaxKevBytes = 20 * 1024 * 1024;
constexpr qsizetype MaxKevRecords = 50000;
constexpr qsizetype MaxAliases = 10000;
inline constexpr auto EpssEndpoint = "https://api.first.org/data/v1/epss";
inline constexpr auto KevEndpoint = "https://raw.githubusercontent.com/cisagov/kev-data/develop/known_exploited_vulnerabilities.json";
SeverityEvidence severity(const RiskEvidenceRequest&);
SeverityEvidence severity(const VulnerabilityCandidate&, const ApplicabilityResult&);
std::optional<EvidenceDependencyContext> dependency(DependencySnapshot, const QString& componentId);
QList<QStringList> epssChunks(const QStringList&);
std::optional<QList<EpssSnapshot>> parseEpss(const QByteArray&, const QStringList& requested, QDateTime fetchedAt);
std::optional<KevSnapshot> parseKev(const QByteArray&, QDateTime fetchedAt);
EvidenceFreshness freshness(QDateTime fetchedAt, QDateTime now);
EpssEvidence epssEvidence(const EpssSnapshot&, EvidenceAcquisition, QDateTime now);
KevEvidence kevEvidence(const KevSnapshot&, const QString& cve, EvidenceAcquisition, QDateTime now);
QString profileText(const RiskEvidenceProfile&);
QString operationText(EvidenceOperationState);
}
