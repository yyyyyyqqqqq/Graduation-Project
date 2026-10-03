#include "ValidationExperiment.h"
#include "ValidationBuildInfo.h"
#include <QJsonDocument>
#include <QSaveFile>
#include <algorithm>

namespace Validation
{
namespace
{
QString utc(QDateTime t)
{
    return t.toUTC().toString(Qt::ISODateWithMs);
}
QJsonValue number(std::optional<double> n)
{
    return n ? QJsonValue(*n) : QJsonValue();
}
QString driver(PriorityDriverKind d)
{
    return d == PriorityDriverKind::Kev ? "KEV" : d == PriorityDriverKind::Epss ? "EPSS" : "None";
}
void count(QJsonObject &object, const QString &key)
{
    object[key] = object[key].toInt() + 1;
}
QJsonObject distribution(const QList<RiskPriorityAssessment> &rows)
{
    QJsonObject priority, support, drivers;
    for (const auto p :
         {PriorityClass::KnownExploited, PriorityClass::AboveResearchPercentileThreshold,
          PriorityClass::BelowResearchPercentileThreshold, PriorityClass::InsufficientCurrentExploitEvidence})
        priority[priorityClassText(p)] = 0;
    for (const auto s : {DecisionEvidenceSupport::Complete, DecisionEvidenceSupport::Partial,
                         DecisionEvidenceSupport::Insufficient})
        support[decisionEvidenceSupportText(s)] = 0;
    for (const auto d : {PriorityDriverKind::None, PriorityDriverKind::Kev, PriorityDriverKind::Epss})
        drivers[driver(d)] = 0;
    for (const auto &a : rows)
    {
        count(priority, priorityClassText(a.priority));
        count(support, decisionEvidenceSupportText(a.support));
        count(drivers, driver(a.driverKind));
    }
    return {{"priority", priority}, {"support", support}, {"driver", drivers}};
}
} // namespace
QJsonObject assessmentJson(const RiskPriorityAssessment &a)
{
    QJsonArray reasons, freshness;
    for (const auto &r : a.reasons)
        reasons.append(QJsonObject{
            {"code", priorityReasonCode(r.code)}, {"cve", r.cve}, {"provider", driver(r.provider)}});
    for (const auto &r : a.evidenceFreshness)
        freshness.append(QJsonObject{{"cve", r.cve},
                                     {"epss", effectiveDecisionFreshnessText(r.epss)},
                                     {"kev", effectiveDecisionFreshnessText(r.kev)}});
    RiskEvidenceProfile p;
    p.key = a.key;
    return {{"key", profileJson(p)["key"]},
            {"profileGeneratedAt", utc(a.profileGeneratedAt)},
            {"priority", priorityClassText(a.priority)},
            {"support", decisionEvidenceSupportText(a.support)},
            {"driver", driver(a.driverKind)},
            {"driverCve", a.driverCve},
            {"rulesVersion", a.rulesVersion},
            {"threshold", a.epssPercentileThreshold},
            {"evaluatedAt", utc(a.evaluatedAt)},
            {"nextFreshnessExpiryUtc",
             a.nextFreshnessExpiryUtc ? QJsonValue(utc(*a.nextFreshnessExpiryUtc)) : QJsonValue()},
            {"driverAcquisition", int(a.driverAcquisition)},
            {"driverFreshness", effectiveDecisionFreshnessText(a.driverFreshness)},
            {"epssProbability", number(a.driverEpssProbability)},
            {"epssPercentile", number(a.driverEpssPercentile)},
            {"epssApiVersion", a.driverEpssApiVersion},
            {"epssScoreDate", a.driverEpssScoreDate.toString(Qt::ISODate)},
            {"epssFetchedAt", utc(a.driverEpssFetchedAt)},
            {"kevCatalogVersion", a.kevCatalogVersion},
            {"kevDateReleased", a.kevDateReleased},
            {"kevFetchedAt", utc(a.kevFetchedAt)},
            {"evidenceFreshness", freshness},
            {"reasons", reasons},
            {"explanation", priorityExplanation(a)}};
}
Experiment runExperiment(const Dataset &dataset)
{
    Experiment e;
    // The baseline is deliberately completed before either experimental run.
    for (const auto &s : dataset.samples)
        e.baseline.append(RiskPriorityEvaluator::evaluate(s.profile, dataset.referenceTime));
    for (const auto &s : dataset.samples)
        e.lower.append(*RiskPriorityEvaluator::evaluate(s.profile, dataset.referenceTime, {.85}));
    for (const auto &s : dataset.samples)
        e.upper.append(*RiskPriorityEvaluator::evaluate(s.profile, dataset.referenceTime, {.95}));
    QJsonObject transitions, availability;
    QJsonArray sensitive, assessments;
    bool kevInvariant = true, monotonic = true, driverInvariant = true, maxEpss = true, replay = true,
         ordering = true, explanations = true, defaultEquivalent = true;
    int kevCases = 0, epssCases = 0;
    const QList<QPair<QString, QPair<int, int>>> pairs{
        {"0.85->0.90", {0, 1}}, {"0.90->0.95", {1, 2}}, {"0.85->0.95", {0, 2}}};
    for (int i = 0; i < dataset.samples.size(); ++i)
    {
        const auto &s = dataset.samples[i];
        const auto &b = e.baseline[i];
        const QList<RiskPriorityAssessment> runs{e.lower[i], b, e.upper[i]};
        QJsonArray details;
        for (const auto &a : runs)
        {
            details.append(assessmentJson(a));
            const auto again = *RiskPriorityEvaluator::evaluate(s.profile, dataset.referenceTime,
                                                                {a.epssPercentileThreshold});
            replay &= a == again;
            explanations &= priorityExplanation(a) == priorityExplanation(again);
            auto reversed = s.profile;
            std::reverse(reversed.epss.begin(), reversed.epss.end());
            std::reverse(reversed.kev.begin(), reversed.kev.end());
            ordering &= a == *RiskPriorityEvaluator::evaluate(reversed, dataset.referenceTime,
                                                              {a.epssPercentileThreshold});
            driverInvariant &= a.support == b.support && a.driverKind == b.driverKind &&
                               a.driverCve == b.driverCve &&
                               a.driverEpssPercentile == b.driverEpssPercentile &&
                               a.driverEpssProbability == b.driverEpssProbability;
            if (b.driverKind == PriorityDriverKind::Kev)
                kevInvariant &=
                    a.priority == b.priority && a.support == b.support && a.driverCve == b.driverCve;
        }
        defaultEquivalent &= b == *RiskPriorityEvaluator::evaluate(s.profile, dataset.referenceTime, {.90});
        if (b.driverKind == PriorityDriverKind::Kev)
            ++kevCases;
        if (b.driverKind == PriorityDriverKind::Epss)
        {
            ++epssCases;
            const auto above = [](const auto &a) {
                return a.priority == PriorityClass::AboveResearchPercentileThreshold ? 1 : 0;
            };
            monotonic &= above(runs[0]) >= above(runs[1]) && above(runs[1]) >= above(runs[2]);
            // Independent invariant on already classified Fresh evidence; not another Priority decision tree.
            std::optional<double> maximum;
            for (const auto &f : b.evidenceFreshness)
                if (f.epss == EffectiveDecisionFreshness::Fresh)
                    for (const auto &row : s.profile.epss)
                        if (row.cve == f.cve && row.status == EpssStatus::Available && row.percentile)
                            if (!maximum || *row.percentile > *maximum)
                                maximum = row.percentile;
            maxEpss &= maximum == b.driverEpssPercentile;
        }
        for (const auto &pair : pairs)
        {
            auto matrix = transitions[pair.first].toObject();
            count(matrix, priorityClassText(runs[pair.second.first].priority) + " -> " +
                              priorityClassText(runs[pair.second.second].priority));
            transitions[pair.first] = matrix;
        }
        if (runs[0].priority != runs[2].priority)
            sensitive.append(s.id);
        QJsonObject statuses;
        for (const auto &row : s.profile.epss)
            statuses["EPSS/" + QString::number(int(row.status))] = true;
        for (const auto &row : s.profile.kev)
            statuses["KEV/" + QString::number(int(row.status))] = true;
        for (auto it = statuses.begin(); it != statuses.end(); ++it)
            count(availability, it.key());
        assessments.append(QJsonObject{{"sampleId", s.id}, {"runs", details}});
    }
    const auto synthetic = syntheticConformance();
    QJsonObject invariants{{"kevPrioritySupportInvariant", kevInvariant},
                           {"kevSampleCount", kevCases},
                           {"epssSampleCount", epssCases},
                           {"epssOnlyMonotonicity", monotonic},
                           {"driverAndSupportInvariant", driverInvariant},
                           {"maxFreshEpss", maxEpss},
                           {"deterministicReplay", replay},
                           {"aliasOrdering", ordering},
                           {"explanationConsistency", explanations},
                           {"productionDefaultEquivalent", defaultEquivalent}};
    e.result = {{"applicationVersion", VALIDATION_APP_VERSION},
                {"sourceGitCommit", VALIDATION_SOURCE_COMMIT},
                {"sourceWorkingTreeDirty", bool(VALIDATION_SOURCE_DIRTY)},
                {"sourceInputsSHA256", VALIDATION_SOURCE_SHA256},
                {"schemaVersion", 4},
                {"datasetSchemaVersion", 1},
                {"sampleCount", dataset.samples.size()},
                {"datasetVersion", dataset.metadata["datasetVersion"]},
                {"datasetSHA256", dataset.hash},
                {"candidatePoolVersion", dataset.metadata["candidatePoolVersion"]},
                {"candidatePoolSHA256", dataset.poolHash},
                {"selectionIndexSHA256", dataset.indexHash},
                {"captureWindowStartUtc", dataset.metadata["captureWindowStartUtc"]},
                {"captureWindowEndUtc", dataset.metadata["captureWindowEndUtc"]},
                {"evaluationReferenceTimeUtc", utc(dataset.referenceTime)},
                {"rulesFamilyVersion", 1},
                {"productionThreshold", .90},
                {"sensitivityThresholds", QJsonArray{.85, .90, .95}},
                {"diversity", diversity(dataset.samples)},
                {"distributions", QJsonObject{{"0.85", distribution(e.lower)},
                                              {"0.90", distribution(e.baseline)},
                                              {"0.95", distribution(e.upper)}}},
                {"availabilityBySampleStatus", availability},
                {"transitions", transitions},
                {"sensitiveSamples", sensitive},
                {"assessments", assessments},
                {"syntheticConformance", synthetic},
                {"invariants", invariants},
                {"datasetDisclaimer", datasetDisclaimer()},
                {"automatedPass", synthetic["pass"].toBool() && kevInvariant && monotonic &&
                                      driverInvariant && maxEpss && replay && ordering && explanations &&
                                      defaultEquivalent}};
    return e;
}
bool exportResult(const Experiment &e, const QString &path, QString &error)
{
    auto o = e.result;
    o["runTimestampUtc"] = utc(QDateTime::currentDateTimeUtc());
    const auto bytes = QJsonDocument(o).toJson(QJsonDocument::Indented);
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
    {
        error = file.errorString();
        return false;
    }
    return true;
}
QString datasetDisclaimer()
{
    return QStringLiteral(
        "以下比例仅描述本验证数据集。该数据集为分层目的性验证集，不代表 PyPI/npm 生态中的漏洞总体比例。");
}
QString experimentSummary(const Dataset &d, const Experiment &e)
{
    const auto stats = diversity(d.samples);
    QString text =
        QStringLiteral("方法验证 / Validation & Threshold "
                       "Sensitivity\n\n%1\n正式实验可离线复现。未来在线数据不保证产生相同结果。\n\n")
            .arg(datasetDisclaimer());
    text += QStringLiteral(
        "生产规则 Rules v1 默认值：0.90；实验：0.85 / 0.90 / 0.95。只改变 EPSS percentile 阈值。\nPercentile "
        "是相对排序，不是利用概率；Support 是决策证据支持度，不是预测置信度。\nSeverity / Dependency "
        "是上下文；Quality 当前不可用。\n\n");
    text += QString("Dataset: %1 | N=%2\nUnique packages: %3 | OSV IDs: %4 | CVE aliases: %5 | Alias "
                    "clusters: %6 | Multi-CVE findings: %7\n")
                .arg(d.metadata["datasetVersion"].toString())
                .arg(d.samples.size())
                .arg(stats["uniquePackageCount"].toInt())
                .arg(stats["uniqueOsvIdCount"].toInt())
                .arg(stats["uniqueCveAliasCount"].toInt())
                .arg(stats["uniqueAliasClusterCount"].toInt())
                .arg(stats["multiCveFindingCount"].toInt());
    text += QString("Dataset SHA-256: %1\nPool: candidate-pool-v1\nPool SHA-256: %2\nIndex SHA-256: "
                    "%3\nEvaluation reference (UTC): %4\nCapture window (UTC): %5 — %6\n\n")
                .arg(d.hash, d.poolHash, d.indexHash, utc(d.referenceTime),
                     d.metadata["captureWindowStartUtc"].toString(),
                     d.metadata["captureWindowEndUtc"].toString());
    const auto dist = e.result["distributions"].toObject();
    for (const auto &threshold : QStringList{"0.90", "0.85", "0.95"})
    {
        text += (threshold == "0.90" ? QStringLiteral("基线 / Baseline ")
                                     : QStringLiteral("阈值实验 / Threshold ")) +
                threshold + '\n';
        const auto block = dist[threshold].toObject();
        for (const auto &dimension : QStringList{"priority", "support", "driver"})
        {
            text += dimension + ":\n";
            const auto values = block[dimension].toObject();
            for (auto it = values.begin(); it != values.end(); ++it)
                text += QString("  %1: %2 / %3 (%4%)\n")
                            .arg(it.key())
                            .arg(it.value().toInt())
                            .arg(d.samples.size())
                            .arg(100. * it.value().toInt() / d.samples.size(), 0, 'f', 1);
        }
        text += '\n';
    }
    text += QStringLiteral("迁移矩阵 / Transitions (nonzero cells)\n") +
            QString::fromUtf8(QJsonDocument(e.result["transitions"].toObject()).toJson());
    text += QStringLiteral("证据可用性 / Availability（每样本可含多个状态；不是互斥比例）\nEPSS: 0 "
                           "Available, 1 NotScored, 2 Failed, 3 InvalidResponse, 4 NotQueryable\nKEV: 0 "
                           "Listed, 1 NotListed, 2 Unknown, 3 NotQueryable\n") +
            QString::fromUtf8(QJsonDocument(e.result["availabilityBySampleStatus"].toObject()).toJson());
    text +=
        QString(
            "Sensitive samples: %1\nSynthetic conformance: %2 cases, %3 failed\nAutomated invariants: %4\n")
            .arg(e.result["sensitiveSamples"].toArray().size())
            .arg(e.result["syntheticConformance"].toObject()["caseCount"].toInt())
            .arg(e.result["syntheticConformance"].toObject()["failed"].toInt())
            .arg(e.result["automatedPass"].toBool() ? "PASS" : "FAIL");
    return text;
}
} // namespace Validation
