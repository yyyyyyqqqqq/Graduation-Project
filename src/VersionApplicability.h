#pragma once

#include "OsvResponseParser.h"

enum class CandidateSource { OsvPackageVersionQuery };
enum class ApplicabilityState { Affected, NotAffected, Unknown };
enum class ApplicabilityReason {
    ExplicitVersionMatch, SemverRangeMatch, EvaluatedNoMatch,
    UnsupportedEcosystemRange, GitRangeRequiresCommitGraph, UnsupportedRangeType,
    InvalidSemver, InvalidRangeEvents, MissingIntroducedEvent, ConflictingRangeEvents,
    NoMatchingAffectedPackage, MissingUsableEvidence, ProviderEvidenceConflict, RecordWithdrawn
};
enum class EvidenceKind { Package, ExplicitVersions, SemverRange, UnsupportedRange };
struct ApplicabilityEvidence {
    EvidenceKind kind = EvidenceKind::Package;
    ApplicabilityState state = ApplicabilityState::Unknown;
    ApplicabilityReason reason = ApplicabilityReason::MissingUsableEvidence;
    qsizetype affectedIndex = -1, rangeIndex = -1;
    bool wildcard = false;
    QJsonObject detail; // Original versions/range evidence, never normalized provider JSON.
};
struct ApplicabilityResult {
    ApplicabilityState state = ApplicabilityState::Unknown;
    ApplicabilityReason reason = ApplicabilityReason::MissingUsableEvidence;
    QString queryVersion;
    int rulesVersion = 1;
    QList<ApplicabilityEvidence> evidence;
};
enum class CandidateEligibility { Eligible, Excluded };
struct CandidateAssessment {
    CandidateSource source = CandidateSource::OsvPackageVersionQuery;
    CandidateEligibility eligibility = CandidateEligibility::Eligible;
    // Excluded records have no version judgment: withdrawal is a lifecycle decision.
    std::optional<ApplicabilityResult> local, published;
    bool hasFinding() const { return eligibility == CandidateEligibility::Eligible && published
        && published->state == ApplicabilityState::Affected; }
};
struct VulnerabilityFinding {
    QString componentId;
    QueryIdentity queryIdentity;
    QString osvId;
    QStringList cveAliases;
    ApplicabilityResult applicability;
    QueryIdentity snapshotIdentity;
    QDateTime fetchedAt;
    CandidateSource source = CandidateSource::OsvPackageVersionQuery;
};
struct ApplicabilitySnapshot {
    QString componentId;
    QueryIdentity identity;
    QDateTime fetchedAt;
    quint64 generation = 0;
    QList<CandidateAssessment> candidates; // Same order as the owning raw snapshot.
    QList<VulnerabilityFinding> findings;
    qsizetype affectedCount = 0, unknownCount = 0, excludedCount = 0;
};

// Pure Core, value-only interpretation of the supported affected evidence.
// Neither storage nor provider transport participates in evaluation.
// Input records come from OsvResponseParser (including validated cache reads).
// assess() adds record eligibility and the provider gate around evaluateLocal().
class VersionApplicability final {
public:
    static ApplicabilityResult evaluateLocal(const QueryIdentity&, const VulnerabilityCandidate&);
    static ApplicabilityResult publish(ApplicabilityResult local, CandidateSource source);
    static CandidateAssessment assess(const QueryIdentity&, const VulnerabilityCandidate&, CandidateSource);
    static ApplicabilitySnapshot evaluateSnapshot(const QString& componentId, const OsvSnapshot&,
                                                  quint64 generation, CandidateSource);
};
QString applicabilityStateText(ApplicabilityState);
QString applicabilityReasonCode(ApplicabilityReason);
QString applicabilityReasonText(ApplicabilityReason);
QString applicabilityExplanation(const CandidateAssessment&);
