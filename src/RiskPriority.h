#pragma once

#include "RiskEvidence.h"

enum class PriorityClass {
    KnownExploited,
    AboveResearchPercentileThreshold,
    BelowResearchPercentileThreshold,
    InsufficientCurrentExploitEvidence
};
enum class DecisionEvidenceSupport { Complete, Partial, Insufficient };
enum class PriorityDriverKind { None, Kev, Epss };
enum class EffectiveDecisionFreshness { Fresh, Stale, NotApplicable, Ineligible };
enum class PriorityReasonCode {
    FreshKevListed, StaleKevListedContextOnly, StaleKevContextOnly,
    FreshEpssAboveThreshold, FreshEpssBelowThreshold, EpssDriverSelected,
    DecisionEvidenceComplete, DecisionEvidencePartial, DecisionEvidenceInsufficient,
    NoFreshExploitDriver, KevCoverageIncompleteOrStale, EpssAliasCoverageIncomplete,
    StaleEpssContextOnly, EvidenceTimeInvalidForCurrentDecision,
    SeverityContextOnly, DependencyContextOnly, QualityUnavailable
};
struct PriorityReason {
    PriorityReasonCode code;
    QString cve;
    PriorityDriverKind provider = PriorityDriverKind::None;
    bool operator==(const PriorityReason&) const = default;
};
struct DecisionEvidenceFreshness {
    QString cve;
    EffectiveDecisionFreshness kev = EffectiveDecisionFreshness::NotApplicable;
    EffectiveDecisionFreshness epss = EffectiveDecisionFreshness::NotApplicable;
    bool operator==(const DecisionEvidenceFreshness&) const = default;
};
struct RiskPriorityAssessment {
    RiskEvidenceKey key;
    QDateTime profileGeneratedAt;
    PriorityClass priority = PriorityClass::InsufficientCurrentExploitEvidence;
    DecisionEvidenceSupport support = DecisionEvidenceSupport::Insufficient;
    PriorityDriverKind driverKind = PriorityDriverKind::None;
    QString driverCve;
    int rulesVersion = 1;
    // Research Default for relative percentile; neither probability nor an official FIRST threshold.
    double epssPercentileThreshold = 0.90;
    QDateTime evaluatedAt;
    std::optional<QDateTime> nextFreshnessExpiryUtc;
    EvidenceAcquisition driverAcquisition = EvidenceAcquisition::None;
    EffectiveDecisionFreshness driverFreshness = EffectiveDecisionFreshness::NotApplicable;
    std::optional<double> driverEpssProbability, driverEpssPercentile;
    QString driverEpssApiVersion;
    QDate driverEpssScoreDate;
    QDateTime driverEpssFetchedAt;
    QString kevCatalogVersion, kevDateReleased;
    QDateTime kevFetchedAt;
    QList<DecisionEvidenceFreshness> evidenceFreshness;
    QList<PriorityReason> reasons;
    bool operator==(const RiskPriorityAssessment&) const = default;
};

struct ExperimentalPriorityOptions { double epssPercentileThreshold = 0.90; };

// Pure Core/value interpretation. The caller owns the clock and the profile lifecycle.
class RiskPriorityEvaluator final {
public:
    static RiskPriorityAssessment evaluate(const RiskEvidenceProfile&, QDateTime evaluationTimeUtc);
    // Only the three frozen sensitivity thresholds are accepted; no other rule is configurable.
    static std::optional<RiskPriorityAssessment> evaluate(const RiskEvidenceProfile&, QDateTime, ExperimentalPriorityOptions);
private:
    static RiskPriorityAssessment evaluateAtThreshold(const RiskEvidenceProfile&, QDateTime, double);
};
QString priorityClassText(PriorityClass);
QString decisionEvidenceSupportText(DecisionEvidenceSupport);
QString effectiveDecisionFreshnessText(EffectiveDecisionFreshness);
QString priorityReasonCode(PriorityReasonCode);
QString priorityExplanation(const RiskPriorityAssessment&);
