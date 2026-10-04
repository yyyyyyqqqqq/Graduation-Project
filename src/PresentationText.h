#pragma once
#include "RiskPriority.h"
#include <QSet>

// Value-to-language only. No clock, evaluator, IO, identity normalization or policy.
namespace PresentationText {
QString priorityLabel(PriorityClass);
QString supportLabel(DecisionEvidenceSupport);
QString priorityDriverLabel(PriorityDriverKind);
QString acquisitionLabel(EvidenceAcquisition);
QString kevStatusLabel(KevStatus);
QString epssStatusLabel(EpssStatus);
QString effectiveFreshnessLabel(EffectiveDecisionFreshness);
QString profileSnapshotFreshnessLabel(EvidenceFreshness);
QString severityStatusLabel(SeverityStatus);
QString qualityLabel(QualityEvidenceStatus);
QString dependencyPathLabel(DependencyPathState);
QString dependencyRootLabel(DependencyRootStatus);
QString referenceResolutionLabel(ReferenceResolutionCompleteness);
QString reasonText(const PriorityReason&, double threshold);
QString conclusionText(const RiskPriorityAssessment&);
QString whyText(const RiskPriorityAssessment&);
QString driverText(const RiskPriorityAssessment&);
QString boundaryText(const RiskPriorityAssessment&);
QString severityText(const SeverityEvidence&);
QString dependencyText(const EvidenceDependencyContext&);

// Exact canonical CVE lookup, with provider-specific fields. Not a domain state.
class DecisionFreshnessLookup final {
public:
    explicit DecisionFreshnessLookup(const RiskPriorityAssessment&);
    QString label(const QString& cve, PriorityDriverKind provider, bool notQueryable) const;
private:
    QHash<QString, DecisionEvidenceFreshness> m_rows;
    QSet<QString> m_duplicates;
};
QString epssText(const EpssEvidence&, const DecisionFreshnessLookup&);
QString kevText(const KevEvidence&, const DecisionFreshnessLookup&);
}
