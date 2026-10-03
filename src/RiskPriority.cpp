#include "RiskPriority.h"
#include "CveIdentity.h"

#include <cmath>

namespace {
using Freshness = EffectiveDecisionFreshness;
using Reason = PriorityReasonCode;
constexpr qint64 FreshnessMilliseconds = 24LL * 60 * 60 * 1000;

Freshness effectiveFreshness(bool usable, QDateTime fetched, EvidenceAcquisition acquisition, QDateTime evaluated)
{
    // Provider failure is not temporal invalidity. Only usable provider facts have a decision age.
    if (!usable || acquisition == EvidenceAcquisition::None) return Freshness::NotApplicable;
    if (!evaluated.isValid() || !fetched.isValid() || evaluated < fetched
        || !fetched.addMSecs(FreshnessMilliseconds).isValid()) return Freshness::Ineligible;
    if (acquisition == EvidenceAcquisition::StaleFallback || fetched.msecsTo(evaluated) >= FreshnessMilliseconds)
        return Freshness::Stale;
    return Freshness::Fresh;
}
bool unitValue(const std::optional<double>& value)
{
    return value && std::isfinite(*value) && *value >= 0.0 && *value <= 1.0;
}
QString acquisitionText(EvidenceAcquisition acquisition)
{
    switch (acquisition) {
    case EvidenceAcquisition::Live: return "Live";
    case EvidenceAcquisition::Cache: return "Cache";
    case EvidenceAcquisition::StaleFallback: return "StaleFallback";
    case EvidenceAcquisition::None: return "None";
    }
    Q_UNREACHABLE();
}
QString providerText(PriorityDriverKind provider)
{
    switch (provider) {
    case PriorityDriverKind::Kev: return "CISA KEV";
    case PriorityDriverKind::Epss: return "FIRST EPSS";
    case PriorityDriverKind::None: return "None";
    }
    Q_UNREACHABLE();
}
QString reasonText(Reason reason, double threshold)
{
    switch (reason) {
    case Reason::FreshKevListed: return "Fresh KEV Listed independently determines Known Exploited.";
    case Reason::EpssDriverSelected: return "Maximum Fresh Available EPSS percentile; equal values use canonical CVE order.";
    case Reason::FreshEpssAboveThreshold: return QString("Fresh EPSS percentile meets this run threshold (>= %1); Production Rules v1 default remains 0.90.").arg(threshold, 0, 'f', 2);
    case Reason::FreshEpssBelowThreshold: return QStringLiteral("当前 Fresh EPSS evidence 未达到 Rules v1 Research Percentile Threshold。这不是 Safe、Low Risk 或 Not Exploitable 结论。");
    case Reason::DecisionEvidenceComplete: return "The evidence required for this Rules v1 decision is current, complete and usable.";
    case Reason::DecisionEvidencePartial: return "A Fresh EPSS driver exists; corroborating alias/provider coverage is incomplete.";
    case Reason::DecisionEvidenceInsufficient: return "No Fresh exploit signal can support a current decision.";
    case Reason::NoFreshExploitDriver: return "No Fresh KEV Listed and no Fresh EPSS Available.";
    case Reason::KevCoverageIncompleteOrStale: return "Fresh NotListed coverage from a complete KEV catalog is unavailable for this alias. NotListed does not prove absence of exploitation.";
    case Reason::EpssAliasCoverageIncomplete: return "Fresh Available EPSS evidence is unavailable for this alias; missing/NotScored is not zero.";
    case Reason::StaleKevListedContextOnly: return "Historical / Stale Evidence Available: KEV Listed is context-only in Rules v1 and does not drive the current Priority decision.";
    case Reason::StaleKevContextOnly: return "Historical / Stale Evidence Available: KEV catalog evidence is context-only in Rules v1.";
    case Reason::StaleEpssContextOnly: return "Historical / Stale Evidence Available: EPSS evidence is context-only in Rules v1 and does not drive the current Priority decision.";
    case Reason::EvidenceTimeInvalidForCurrentDecision: return "Evidence time invalid for current decision (Ineligible); this is not ordinary Stale evidence.";
    case Reason::SeverityContextOnly: return "Technical severity context only. Rules v1 does not convert this into a numeric Risk Score. Provider CVSS vectors remain in Provider Snapshot.";
    case Reason::DependencyContextOnly: return "Dependency context only. Does not prove runtime reachability and does not change Priority or Support.";
    case Reason::QualityUnavailable: return "Quality: Unavailable for persisted current state; not a Rules v1 requirement, so it does not reduce Support.";
    }
    Q_UNREACHABLE();
}
}

RiskPriorityAssessment RiskPriorityEvaluator::evaluate(const RiskEvidenceProfile& profile, QDateTime time)
{
    return evaluateAtThreshold(profile, time, 0.90);
}
std::optional<RiskPriorityAssessment> RiskPriorityEvaluator::evaluate(const RiskEvidenceProfile& profile,
        QDateTime time, ExperimentalPriorityOptions options)
{
    const double threshold = options.epssPercentileThreshold;
    if (threshold != 0.85 && threshold != 0.90 && threshold != 0.95) return {};
    return evaluateAtThreshold(profile, time, threshold);
}
RiskPriorityAssessment RiskPriorityEvaluator::evaluateAtThreshold(const RiskEvidenceProfile& profile,
        QDateTime evaluationTimeUtc, double threshold)
{
    RiskPriorityAssessment result;
    result.epssPercentileThreshold = threshold;
    result.key = profile.key;
    result.profileGeneratedAt = profile.generatedAt;
    result.evaluatedAt = evaluationTimeUtc.toUTC();

    // Phase 09 emits a row for every queryable alias in BOTH provider lists, including failures.
    // Their union also detects a missing counterpart defensively without a second Finding owner.
    // Hashes are lookup-only; duplicate provider rows are unusable, never chosen by insertion order.
    QHash<QString, const EpssEvidence*> epss;
    QHash<QString, const KevEvidence*> kev;
    QStringList aliases;
    for (const auto& evidence : profile.epss) {
        aliases.append(evidence.cve);
        if (epss.contains(evidence.cve)) epss[evidence.cve] = nullptr;
        else epss.insert(evidence.cve, &evidence);
    }
    for (const auto& evidence : profile.kev) {
        aliases.append(evidence.cve);
        if (kev.contains(evidence.cve)) kev[evidence.cve] = nullptr;
        else kev.insert(evidence.cve, &evidence);
    }
    // Canonical sorting is solely a deterministic provenance tie-break, never risk/importance/confidence order.
    const auto cves = sortedCveIds(aliases);
    QList<PriorityReason> coverage, historical;
    const KevEvidence* kevDriver = nullptr;
    const EpssEvidence* epssDriver = nullptr;
    bool completeCoverage = !cves.isEmpty();
    const auto considerExpiry = [&](Freshness freshness, QDateTime fetched) {
        if (freshness != Freshness::Fresh) return;
        const auto expiry = fetched.toUTC().addMSecs(FreshnessMilliseconds);
        if (!result.nextFreshnessExpiryUtc || expiry < *result.nextFreshnessExpiryUtc)
            result.nextFreshnessExpiryUtc = expiry;
    };
    for (const auto& cve : cves) {
        const auto* k = kev.value(cve);
        const auto* e = epss.value(cve);
        const bool usableKev = k && (k->status == KevStatus::Listed || k->status == KevStatus::NotListed);
        const bool usableEpss = e && (e->status == EpssStatus::NotScored
            || (e->status == EpssStatus::Available && unitValue(e->probability) && unitValue(e->percentile)));
        const auto kf = k ? effectiveFreshness(usableKev, k->fetchedAt, k->acquisition, result.evaluatedAt) : Freshness::NotApplicable;
        const auto ef = e ? effectiveFreshness(usableEpss, e->fetchedAt, e->acquisition, result.evaluatedAt) : Freshness::NotApplicable;
        result.evidenceFreshness.append({cve, kf, ef});
        if (k) considerExpiry(kf, k->fetchedAt);
        if (e) considerExpiry(ef, e->fetchedAt);
        if (!kevDriver && kf == Freshness::Fresh && k->status == KevStatus::Listed) kevDriver = k;
        if (ef == Freshness::Fresh && e->status == EpssStatus::Available
            && (!epssDriver || *e->percentile > *epssDriver->percentile)) epssDriver = e;

        const bool kevCovered = kf == Freshness::Fresh && k->status == KevStatus::NotListed;
        const bool epssCovered = ef == Freshness::Fresh && e->status == EpssStatus::Available;
        completeCoverage = completeCoverage && kevCovered && epssCovered;
        if (!kevCovered) coverage.append({Reason::KevCoverageIncompleteOrStale, cve, PriorityDriverKind::Kev});
        if (!epssCovered) coverage.append({Reason::EpssAliasCoverageIncomplete, cve, PriorityDriverKind::Epss});
        if (kf == Freshness::Ineligible) coverage.append({Reason::EvidenceTimeInvalidForCurrentDecision, cve, PriorityDriverKind::Kev});
        if (ef == Freshness::Ineligible) coverage.append({Reason::EvidenceTimeInvalidForCurrentDecision, cve, PriorityDriverKind::Epss});
        if (kf == Freshness::Stale) historical.append({k->status == KevStatus::Listed
            ? Reason::StaleKevListedContextOnly : Reason::StaleKevContextOnly, cve, PriorityDriverKind::Kev});
        if (ef == Freshness::Stale) historical.append({Reason::StaleEpssContextOnly, cve, PriorityDriverKind::Epss});
    }
    if (kevDriver) {
        result.priority = PriorityClass::KnownExploited;
        result.support = DecisionEvidenceSupport::Complete;
        result.driverKind = PriorityDriverKind::Kev;
        result.driverCve = kevDriver->cve;
        result.driverAcquisition = kevDriver->acquisition;
        result.driverFreshness = Freshness::Fresh;
        result.kevCatalogVersion = kevDriver->catalogVersion;
        result.kevDateReleased = kevDriver->dateReleased;
        result.kevFetchedAt = kevDriver->fetchedAt.toUTC();
        result.reasons.append({Reason::FreshKevListed, result.driverCve, result.driverKind});
    } else if (epssDriver) {
        result.priority = *epssDriver->percentile >= result.epssPercentileThreshold
            ? PriorityClass::AboveResearchPercentileThreshold : PriorityClass::BelowResearchPercentileThreshold;
        result.support = completeCoverage ? DecisionEvidenceSupport::Complete : DecisionEvidenceSupport::Partial;
        result.driverKind = PriorityDriverKind::Epss;
        result.driverCve = epssDriver->cve;
        result.driverAcquisition = epssDriver->acquisition;
        result.driverFreshness = Freshness::Fresh;
        result.driverEpssProbability = epssDriver->probability;
        result.driverEpssPercentile = epssDriver->percentile;
        result.driverEpssApiVersion = epssDriver->providerVersion; // FIRST API Version, not EPSS Model Version.
        result.driverEpssScoreDate = epssDriver->scoreDate;
        result.driverEpssFetchedAt = epssDriver->fetchedAt.toUTC();
        result.reasons.append({Reason::EpssDriverSelected, result.driverCve, result.driverKind});
        result.reasons.append({result.priority == PriorityClass::AboveResearchPercentileThreshold
            ? Reason::FreshEpssAboveThreshold : Reason::FreshEpssBelowThreshold, result.driverCve, result.driverKind});
    } else {
        result.reasons.append({Reason::NoFreshExploitDriver, {}});
    }
    result.reasons.append({result.support == DecisionEvidenceSupport::Complete ? Reason::DecisionEvidenceComplete
        : result.support == DecisionEvidenceSupport::Partial ? Reason::DecisionEvidencePartial : Reason::DecisionEvidenceInsufficient, {}});
    // KEV Listed is independently sufficient: missing EPSS does not diminish its support.
    // Retain temporal warnings, but do not misleadingly describe Listed as missing KEV coverage.
    for (const auto& reason : coverage)
        if (!kevDriver || reason.code == Reason::EvidenceTimeInvalidForCurrentDecision) result.reasons.append(reason);
    result.reasons.append(historical);
    result.reasons.append({Reason::SeverityContextOnly, {}});
    result.reasons.append({Reason::DependencyContextOnly, {}});
    result.reasons.append({Reason::QualityUnavailable, {}});
    return result;
}

QString priorityClassText(PriorityClass priority)
{
    switch (priority) {
    case PriorityClass::KnownExploited: return "Known Exploited";
    case PriorityClass::AboveResearchPercentileThreshold: return "Above Rules v1 Research Percentile Threshold";
    case PriorityClass::BelowResearchPercentileThreshold: return "Below Rules v1 Research Percentile Threshold";
    case PriorityClass::InsufficientCurrentExploitEvidence: return "Insufficient Current Exploit Evidence";
    }
    Q_UNREACHABLE();
}
QString decisionEvidenceSupportText(DecisionEvidenceSupport support)
{
    switch (support) {
    case DecisionEvidenceSupport::Complete: return "Complete";
    case DecisionEvidenceSupport::Partial: return "Partial";
    case DecisionEvidenceSupport::Insufficient: return "Insufficient";
    }
    Q_UNREACHABLE();
}
QString effectiveDecisionFreshnessText(Freshness freshness)
{
    switch (freshness) {
    case Freshness::Fresh: return "Fresh";
    case Freshness::Stale: return "Stale";
    case Freshness::NotApplicable: return "NotApplicable";
    case Freshness::Ineligible: return "Ineligible";
    }
    Q_UNREACHABLE();
}
QString priorityReasonCode(Reason reason)
{
    switch (reason) {
#define REASON_NAME(name) case Reason::name: return QStringLiteral(#name)
    REASON_NAME(FreshKevListed); REASON_NAME(StaleKevListedContextOnly); REASON_NAME(StaleKevContextOnly);
    REASON_NAME(FreshEpssAboveThreshold); REASON_NAME(FreshEpssBelowThreshold); REASON_NAME(EpssDriverSelected);
    REASON_NAME(DecisionEvidenceComplete); REASON_NAME(DecisionEvidencePartial); REASON_NAME(DecisionEvidenceInsufficient);
    REASON_NAME(NoFreshExploitDriver); REASON_NAME(KevCoverageIncompleteOrStale); REASON_NAME(EpssAliasCoverageIncomplete);
    REASON_NAME(StaleEpssContextOnly); REASON_NAME(EvidenceTimeInvalidForCurrentDecision);
    REASON_NAME(SeverityContextOnly); REASON_NAME(DependencyContextOnly); REASON_NAME(QualityUnavailable);
#undef REASON_NAME
    }
    Q_UNREACHABLE();
}

QString priorityExplanation(const RiskPriorityAssessment& assessment)
{
    const auto utcText = [](QDateTime time) { return time.isValid() ? time.toUTC().toString(Qt::ISODateWithMs) : QString("Unavailable"); };
    const auto& a = assessment;
    QString text = QStringLiteral("利用信号优先级评估 / Exploit-Signal Priority Assessment\n");
    text += QString("Priority: %1\nDecision Evidence Support: %2\nRules: v%3\n")
        .arg(priorityClassText(a.priority), decisionEvidenceSupportText(a.support)).arg(a.rulesVersion);
    text += QString("Rules v%1 Research Threshold: Percentile >= %2\n")
        .arg(a.rulesVersion).arg(a.epssPercentileThreshold, 0, 'f', 2);
    text += a.epssPercentileThreshold == 0.90 ? "Production default run: 0.90.\n"
        : "Experimental threshold run; Production Rules v1 default remains 0.90.\n";
    text += "0.90 is an EPSS percentile Research Default: relative ranking, not 90% exploitation probability or a FIRST official High threshold.\n";
    text += "Support describes current, complete, usable evidence for this decision; it is not predictive confidence, Finding correctness, vulnerability probability or overall risk certainty.\n";
    text += QString("Driver: %1\nDriver CVE: %2\nDriver Effective Freshness: %3\nAcquisition: %4\n")
        .arg(providerText(a.driverKind), a.driverCve.isEmpty() ? "None" : a.driverCve,
             effectiveDecisionFreshnessText(a.driverFreshness), acquisitionText(a.driverAcquisition));
    if (a.driverKind == PriorityDriverKind::Kev)
        text += QString("Catalog Version: %1\nDate Released: %2\nFetched At: %3\n")
            .arg(a.kevCatalogVersion, a.kevDateReleased, utcText(a.kevFetchedAt));
    if (a.driverKind == PriorityDriverKind::Epss)
        text += QString("EPSS Probability: %1\nEPSS Percentile: %2\nFIRST API Version: %3\nScore Date: %4\nFetched At: %5\n")
            .arg(a.driverEpssProbability ? QString::number(*a.driverEpssProbability, 'g', 12) : "Unavailable",
                 a.driverEpssPercentile ? QString::number(*a.driverEpssPercentile, 'g', 12) : "Unavailable",
                 a.driverEpssApiVersion, a.driverEpssScoreDate.toString(Qt::ISODate), utcText(a.driverEpssFetchedAt));
    text += QString("Profile Generated At: %1\nEvaluated At: %2\nNext Freshness Expiry (UTC): %3\n")
        .arg(utcText(a.profileGeneratedAt), utcText(a.evaluatedAt), a.nextFreshnessExpiryUtc ? utcText(*a.nextFreshnessExpiryUtc) : "None");
    text += "\nDecision Path\n";
    // Rendering consumes the assessment only; no second provider scan or Priority decision exists in UI.
    text += a.driverKind == PriorityDriverKind::Kev ? "1. Fresh KEV Listed? Yes; KEV precedence.\n"
        : a.driverKind == PriorityDriverKind::Epss ? "1. Fresh KEV Listed? No.\n2. Fresh EPSS Available? Yes.\n"
        : "1. Fresh KEV Listed? No.\n2. Fresh EPSS Available? No.\n";
    for (const auto& reason : a.reasons) {
        text += priorityReasonCode(reason.code);
        if (!reason.cve.isEmpty()) text += " [" + reason.cve + " / " + providerText(reason.provider) + "]";
        text += ": " + reasonText(reason.code, a.epssPercentileThreshold) + '\n';
    }
    text += "\nEffective Freshness at Evaluated At (provider snapshot freshness remains unchanged)\n";
    for (const auto& row : a.evidenceFreshness)
        text += row.cve + " | KEV: " + effectiveDecisionFreshnessText(row.kev) + " | EPSS: " + effectiveDecisionFreshnessText(row.epss) + '\n';
    // Bound presentation only; the full assessment and reason list remain available in memory.
    return text.left(262144) + (text.size() > 262144 ? QStringLiteral("\n[显示已截断；完整评估保留在内存中]") : QString());
}
