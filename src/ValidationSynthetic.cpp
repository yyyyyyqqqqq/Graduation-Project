#include "ValidationExperiment.h"
#include <algorithm>
#include <cmath>

namespace Validation
{
namespace
{
using P = PriorityClass;
using S = DecisionEvidenceSupport;
using D = PriorityDriverKind;
const QString A = "CVE-2026-1000001", B = "CVE-2026-1000002";
QDateTime reference()
{
    return QDateTime::fromString("2026-09-30T00:00:00.000Z", Qt::ISODateWithMs);
}
RiskEvidenceProfile input(double percentile)
{
    RiskEvidenceProfile p;
    p.generatedAt = reference();
    p.key = {
        "synthetic-validation", "SYNTHETIC-P11", {"npm", "synthetic-phase11", "1.0.0", 1}, reference(), 1};
    EpssEvidence e;
    e.cve = A;
    e.status = EpssStatus::Available;
    e.acquisition = EvidenceAcquisition::Cache;
    e.freshness = EvidenceFreshness::Fresh;
    e.fetchedAt = reference();
    e.probability = .035;
    e.percentile = percentile;
    e.providerVersion = "synthetic-api";
    e.scoreDate = reference().date();
    p.epss = {e};
    KevEvidence k;
    k.cve = A;
    k.status = KevStatus::NotListed;
    k.acquisition = EvidenceAcquisition::Cache;
    k.freshness = EvidenceFreshness::Fresh;
    k.fetchedAt = reference();
    k.catalogVersion = "synthetic-catalog";
    k.dateReleased = "2026-09-30T00:00:00Z";
    p.kev = {k};
    return p;
}
} // namespace
QList<SyntheticCase> syntheticCases()
{
    QList<SyntheticCase> cases;
    for (double t : {.85, .90, .95})
    {
        const QString prefix = QString::number(t, 'f', 2) + "/";
        // Literal expected labels follow the frozen rule boundary, not evaluator outputs.
        for (const auto &point :
             QList<QPair<double, P>>{{std::nextafter(t, 0.), P::BelowResearchPercentileThreshold},
                                     {t, P::AboveResearchPercentileThreshold},
                                     {std::nextafter(t, 1.), P::AboveResearchPercentileThreshold}})
        {
            cases.append({prefix + QString::number(point.first, 'g', 17),
                          input(point.first),
                          reference(),
                          t,
                          point.second,
                          S::Complete,
                          D::Epss,
                          A,
                          {"EpssDriverSelected", "DecisionEvidenceComplete", "relative ranking"}});
        }
        auto add = [&](QString name, RiskEvidenceProfile p, P priority, S support, D driver, QString cve,
                       QStringList facts, QDateTime time = reference()) {
            cases.append({prefix + name, p, time, t, priority, support, driver, cve, facts});
        };
        auto p = input(.99);
        add("before-24h", p, P::AboveResearchPercentileThreshold, S::Complete, D::Epss, A,
            {"FreshEpssAboveThreshold"}, reference().addMSecs(86399999));
        add("at-24h", p, P::InsufficientCurrentExploitEvidence, S::Insufficient, D::None, {},
            {"StaleEpssContextOnly", "NoFreshExploitDriver"}, reference().addMSecs(86400000));
        add("after-24h", p, P::InsufficientCurrentExploitEvidence, S::Insufficient, D::None, {},
            {"StaleKevContextOnly"}, reference().addMSecs(86400001));
        add("future-time", p, P::InsufficientCurrentExploitEvidence, S::Insufficient, D::None, {},
            {"EvidenceTimeInvalidForCurrentDecision"}, reference().addMSecs(-1));
        p = input(.01);
        p.kev[0].status = KevStatus::Listed;
        add("KEV-low-EPSS", p, P::KnownExploited, S::Complete, D::Kev, A,
            {"FreshKevListed", "KEV precedence"});
        p = input(.99);
        p.kev[0].status = KevStatus::Listed;
        p.kev[0].fetchedAt = reference().addDays(-1);
        add("stale-KEV-fresh-EPSS", p, P::AboveResearchPercentileThreshold, S::Partial, D::Epss, A,
            {"StaleKevListedContextOnly", "DecisionEvidencePartial"});
        for (auto status : {EpssStatus::NotScored, EpssStatus::NotQueryable, EpssStatus::Failed,
                            EpssStatus::InvalidResponse})
        {
            p = input(.99);
            p.epss[0].status = status;
            p.epss[0].probability.reset();
            p.epss[0].percentile.reset();
            add("unavailable-" + QString::number(int(status)), p, P::InsufficientCurrentExploitEvidence,
                S::Insufficient, D::None, {}, {"NoFreshExploitDriver", "missing/NotScored is not zero"});
        }
        p = input(.99);
        p.epss[0].acquisition = EvidenceAcquisition::StaleFallback;
        add("fallback-not-fresh", p, P::InsufficientCurrentExploitEvidence, S::Insufficient, D::None, {},
            {"StaleEpssContextOnly"});
        p = input(.02);
        auto e = p.epss[0];
        e.cve = B;
        e.percentile = .99;
        p.epss.append(e);
        auto k = p.kev[0];
        k.cve = B;
        p.kev.append(k);
        add("multi-CVE-maximum", p, P::AboveResearchPercentileThreshold, S::Complete, D::Epss, B,
            {"Maximum Fresh Available EPSS percentile"});
        std::reverse(p.epss.begin(), p.epss.end());
        std::reverse(p.kev.begin(), p.kev.end());
        add("multi-CVE-reordered", p, P::AboveResearchPercentileThreshold, S::Complete, D::Epss, B,
            {"Maximum Fresh Available EPSS percentile"});
        p.epss[1].percentile = .99;
        add("multi-CVE-tie", p, P::AboveResearchPercentileThreshold, S::Complete, D::Epss, A,
            {"equal values use canonical CVE order"});
        p.epss[1].status = EpssStatus::NotScored;
        p.epss[1].percentile.reset();
        p.epss[1].probability.reset();
        add("partial-alias", p, P::AboveResearchPercentileThreshold, S::Partial, D::Epss, B,
            {"EpssAliasCoverageIncomplete", "DecisionEvidencePartial"});
    }
    return cases;
}
QJsonObject syntheticConformance()
{
    const auto frozen = syntheticCases();
    QJsonArray rows;
    int failures = 0;
    for (const auto &c : frozen)
    {
        const auto a = RiskPriorityEvaluator::evaluate(c.profile, c.time, {c.threshold});
        bool ok = a && a->priority == c.priority && a->support == c.support && a->driverKind == c.driver &&
                  a->driverCve == c.driverCve;
        const auto explanation = a ? priorityExplanation(*a) : QString();
        for (const auto &f : c.explanationFacts)
            ok &= explanation.contains(f);
        ok &= explanation.contains(QString("Percentile >= %1").arg(c.threshold, 0, 'f', 2));
        ok &= a == RiskPriorityEvaluator::evaluate(c.profile, c.time, {c.threshold});
        if (!ok)
            ++failures;
        rows.append(
            QJsonObject{{"case", c.name},
                        {"pass", ok},
                        {"expectedPriority", priorityClassText(c.priority)},
                        {"expectedSupport", decisionEvidenceSupportText(c.support)},
                        {"expectedDriver", int(c.driver)},
                        {"expectedDriverCve", c.driverCve},
                        {"expectedExplanationFacts", QJsonArray::fromStringList(c.explanationFacts)}});
    }
    return {{"suiteVersion", "synthetic-v1"},
            {"caseCount", frozen.size()},
            {"failed", failures},
            {"pass", failures == 0},
            {"cases", rows}};
}
} // namespace Validation
