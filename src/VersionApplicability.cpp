#include "VersionApplicability.h"
#include "Semver.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <algorithm>

namespace {
using State = ApplicabilityState;
using Reason = ApplicabilityReason;
struct RangeResult { State state; Reason reason; };
struct Event {
    QString type;
    std::optional<Semver> version; // Absent only for introduced="0".
};
RangeResult semverRange(const QString& version, const QJsonObject& range)
{
    const auto unknown = [](Reason reason) { return RangeResult{State::Unknown, reason}; };
    if (!range.value("events").isArray()) return unknown(Reason::MissingIntroducedEvent);
    const auto events = range.value("events").toArray();
    bool introduced = false, fixed = false, last = false;
    // Validate the entire sequence before allowing a range hit. JSON parsing
    // stays in OsvResponseParser; these are interpretation-specific constraints.
    for (const auto& item : events) {
        if (!item.isObject()) return unknown(Reason::InvalidRangeEvents);
        const auto event = item.toObject();
        int kinds = 0;
        for (const auto* key : {"introduced", "fixed", "last_affected", "limit"}) {
            if (!event.contains(QLatin1String(key))) continue;
            ++kinds;
            if (!event.value(QLatin1String(key)).isString()) return unknown(Reason::InvalidRangeEvents);
        }
        if (kinds != 1) return unknown(Reason::InvalidRangeEvents);
        introduced |= event.contains("introduced");
        fixed |= event.contains("fixed");
        last |= event.contains("last_affected");
    }
    if (!introduced) return unknown(Reason::MissingIntroducedEvent);
    if (fixed && last) return unknown(Reason::ConflictingRangeEvents);
    const auto current = Semver::parse(version);
    if (!current) return unknown(Reason::InvalidSemver);
    QList<Event> ordered;
    bool hasLimit = false, beforeLimits = false;
    for (const auto& item : events) {
        const auto event = item.toObject();
        for (const auto* key : {"introduced", "fixed", "last_affected", "limit"}) {
            const auto type = QLatin1String(key);
            if (!event.contains(type)) continue;
            const auto text = event.value(type).toString();
            if (type == QLatin1String("limit") && text.contains(u'*')) {
                hasLimit = beforeLimits = true;
            } else if (type == QLatin1String("introduced") && text == "0") {
                ordered.append({QString(type), {}});
            } else {
                const auto parsed = Semver::parse(text);
                if (!parsed) return unknown(Reason::InvalidSemver);
                if (type == QLatin1String("limit")) {
                    hasLimit = true;
                    // OSV BeforeLimits is an existential gate, not intersection.
                    beforeLimits |= current->compare(*parsed) < 0;
                } else ordered.append({QString(type), parsed});
            }
        }
    }
    const auto order = [](const Event& a, const Event& b) {
        if (!a.version && !b.version) return 0;
        if (!a.version) return -1;
        if (!b.version) return 1;
        return a.version->compare(*b.version);
    };
    std::sort(ordered.begin(), ordered.end(), [&](const Event& a, const Event& b) { return order(a,b) < 0; });
    // Different transitions at identical precedence have no safe tie ordering.
    // Equal transitions are harmless duplicates, including distinct build metadata.
    for (qsizetype i = 1; i < ordered.size(); ++i)
        if (order(ordered[i-1], ordered[i]) == 0 && ordered[i-1].type != ordered[i].type)
            return unknown(Reason::InvalidRangeEvents);
    bool affected = false;
    for (const auto& event : ordered) {
        const int compared = event.version ? current->compare(*event.version) : 1;
        if (event.type == "introduced" && compared >= 0) affected = true;
        else if (event.type == "fixed" && compared >= 0) affected = false;
        else if (event.type == "last_affected" && compared > 0) affected = false;
    }
    const bool hit = affected && (!hasLimit || beforeLimits);
    return {hit ? State::Affected : State::NotAffected, hit ? Reason::SemverRangeMatch : Reason::EvaluatedNoMatch};
}

bool matches(const QueryIdentity& identity, const QJsonObject& package)
{
    if (package.value("ecosystem").toString() != identity.ecosystem) return false;
    const auto name = package.value("name").toString();
    if (name == "*") return true;
    if (identity.ecosystem == "PyPI") {
        const auto a = PackageIdentity::canonicalPypiName(name);
        const auto b = PackageIdentity::canonicalPypiName(identity.name);
        return a && b && *a == *b;
    }
    return identity.ecosystem == "npm" && name == identity.name;
}
}

ApplicabilityResult VersionApplicability::evaluateLocal(const QueryIdentity& identity, const VulnerabilityCandidate& candidate)
{
    ApplicabilityResult result;
    result.queryVersion = identity.version;
    result.reason = Reason::NoMatchingAffectedPackage;
    const auto affected = candidate.record.value("affected").toArray();
    for (qsizetype ai = 0; ai < affected.size(); ++ai) {
        const auto entry = affected[ai].toObject();
        const auto package = entry.value("package").toObject();
        if (!matches(identity, package)) continue; // Nonmatching malformed ranges are irrelevant.
        const bool wildcard = package.value("name").toString() == "*";
        const auto firstEvidence = result.evidence.size();
        const auto add = [&](EvidenceKind kind, RangeResult evaluated, qsizetype ri, QJsonObject detail) {
            result.evidence.append({kind, evaluated.state, evaluated.reason, ai, ri, wildcard, std::move(detail)});
        };
        const auto versions = entry.value("versions").toArray();
        if (!versions.isEmpty()) {
            bool hit = false;
            for (const auto& value : versions) if (value.isString() && value.toString() == identity.version) hit = true;
            add(EvidenceKind::ExplicitVersions, {hit ? State::Affected : State::NotAffected,
                hit ? Reason::ExplicitVersionMatch : Reason::EvaluatedNoMatch}, -1, {{"versions", versions}});
        }
        const auto ranges = entry.value("ranges").toArray();
        for (qsizetype ri = 0; ri < ranges.size(); ++ri) {
            const auto range = ranges[ri].toObject();
            const auto type = range.value("type").toString();
            if (type == "SEMVER") add(EvidenceKind::SemverRange, semverRange(identity.version, range), ri, range);
            else add(EvidenceKind::UnsupportedRange, {State::Unknown,
                type == "ECOSYSTEM" ? Reason::UnsupportedEcosystemRange
                : type == "GIT" ? Reason::GitRangeRequiresCommitGraph : Reason::UnsupportedRangeType}, ri, range);
        }
        if (result.evidence.size() == firstEvidence)
            add(EvidenceKind::Package, {State::Unknown, Reason::MissingUsableEvidence}, -1, {{"package", package}});
    }
    if (result.evidence.isEmpty()) return result;
    // Complete evidence remains inspectable even when reliable positive evidence wins.
    for (const auto state : {State::Affected, State::Unknown}) {
        for (const auto& evidence : result.evidence) {
            if (evidence.state != state) continue;
            result.state = state; result.reason = evidence.reason;
            return result;
        }
    }
    result.state = State::NotAffected;
    result.reason = Reason::EvaluatedNoMatch;
    return result;
}

ApplicabilityResult VersionApplicability::publish(ApplicabilityResult local, CandidateSource source)
{
    // OSV may use fuzzy upstream-version matching. A local miss cannot reverse
    // a provider package+version candidate into a published NotAffected claim.
    if (source == CandidateSource::OsvPackageVersionQuery && local.state == State::NotAffected) {
        local.state = State::Unknown;
        local.reason = Reason::ProviderEvidenceConflict;
    }
    return local;
}

CandidateAssessment VersionApplicability::assess(const QueryIdentity& identity, const VulnerabilityCandidate& candidate,
                                                CandidateSource source)
{
    CandidateAssessment result;
    result.source = source;
    if (OsvResponseParser::validTimestamp(candidate.text("withdrawn"))) {
        result.eligibility = CandidateEligibility::Excluded;
        return result;
    }
    result.local = evaluateLocal(identity, candidate);
    result.published = publish(*result.local, source);
    return result;
}

ApplicabilitySnapshot VersionApplicability::evaluateSnapshot(const QString& componentId, const OsvSnapshot& snapshot,
                                                            quint64 generation, CandidateSource source)
{
    ApplicabilitySnapshot result;
    result.componentId = componentId; result.identity = snapshot.identity;
    result.fetchedAt = snapshot.fetchedAt; result.generation = generation;
    result.candidates.reserve(snapshot.candidates.size());
    for (const auto& candidate : snapshot.candidates) {
        auto assessment = assess(snapshot.identity, candidate, source);
        if (assessment.hasFinding()) {
            ++result.affectedCount;
            result.findings.append({componentId, snapshot.identity, candidate.id(), candidate.cveAliases(),
                                    *assessment.published, snapshot.identity, snapshot.fetchedAt, source});
        } else if (assessment.eligibility == CandidateEligibility::Excluded) ++result.excludedCount;
        else ++result.unknownCount;
        result.candidates.append(std::move(assessment));
    }
    return result;
}

QString applicabilityStateText(ApplicabilityState state)
{
    switch (state) {
    case State::Affected: return QStringLiteral("Affected / 本地确认受影响");
    case State::NotAffected: return QStringLiteral("NotAffected / 本地未命中");
    case State::Unknown: return QStringLiteral("Unknown / 未确认");
    }
    Q_UNREACHABLE();
}
QString applicabilityReasonCode(ApplicabilityReason reason)
{
    switch (reason) {
#define REASON(x) case Reason::x: return QStringLiteral(#x);
    REASON(ExplicitVersionMatch) REASON(SemverRangeMatch) REASON(EvaluatedNoMatch)
    REASON(UnsupportedEcosystemRange) REASON(GitRangeRequiresCommitGraph) REASON(UnsupportedRangeType)
    REASON(InvalidSemver) REASON(InvalidRangeEvents) REASON(MissingIntroducedEvent) REASON(ConflictingRangeEvents)
    REASON(NoMatchingAffectedPackage) REASON(MissingUsableEvidence) REASON(ProviderEvidenceConflict) REASON(RecordWithdrawn)
#undef REASON
    }
    Q_UNREACHABLE();
}
QString applicabilityReasonText(ApplicabilityReason reason)
{
    switch (reason) {
    case Reason::ExplicitVersionMatch: return QStringLiteral("versions 中存在与查询版本完全相同的字符串；未进行版本规范化。");
    case Reason::SemverRangeMatch: return QStringLiteral("当前版本命中有效 SEMVER 区间；introduced 包含边界，fixed / limit 不包含，last_affected 包含。");
    case Reason::EvaluatedNoMatch: return QStringLiteral("本地支持的相关证据均已解释，但未命中当前版本；这只是本地解释结果。");
    case Reason::UnsupportedEcosystemRange: return QStringLiteral("本阶段未解释 ECOSYSTEM 版本顺序（包括 PEP 440），保留未确认。");
    case Reason::GitRangeRequiresCommitGraph: return QStringLiteral("GIT 区间需要提交图，本阶段不克隆仓库或推断提交可达性。");
    case Reason::UnsupportedRangeType: return QStringLiteral("区间类型缺失、为空或不受支持，不能默认为 SEMVER。");
    case Reason::InvalidSemver: return QStringLiteral("查询版本或区间边界不是严格 SemVer 2.0，未进行模糊转换。");
    case Reason::InvalidRangeEvents: return QStringLiteral("事件必须恰含一种边界类型；不同转换处于相同优先级时无法确定安全顺序。");
    case Reason::MissingIntroducedEvent: return QStringLiteral("SEMVER 事件序列缺少 introduced。");
    case Reason::ConflictingRangeEvents: return QStringLiteral("同一事件序列混用了 fixed 与 last_affected。");
    case Reason::NoMatchingAffectedPackage: return QStringLiteral("没有与 QueryIdentity 匹配的 affected package；未使用组件显示名称作为后备身份。");
    case Reason::MissingUsableEvidence: return QStringLiteral("匹配的 affected entry 没有可用版本或区间证据。");
    case Reason::ProviderEvidenceConflict: return QStringLiteral("OSV 针对当前 package/version 返回了该记录，但本地支持的 affected 解释规则未能独立复现命中；保留 Unknown，不反向认定当前版本不受影响。Provider 可能执行 fuzzy upstream-version matching。");
    case Reason::RecordWithdrawn: return QStringLiteral("OSV 记录已撤回；作为记录生命周期排除，不发布版本判断，不形成 Finding。");
    }
    Q_UNREACHABLE();
}

QString applicabilityExplanation(const CandidateAssessment& assessment)
{
    QString text = QStringLiteral("Provider context: OsvPackageVersionQuery / OSV package + version query\n");
    if (assessment.eligibility == CandidateEligibility::Excluded)
        return text + QStringLiteral("Excluded / RecordWithdrawn\n") + applicabilityReasonText(Reason::RecordWithdrawn);
    if (!assessment.local || !assessment.published) return text;
    const auto describe = [](const ApplicabilityResult& result) {
        return applicabilityStateText(result.state) + " / " + applicabilityReasonCode(result.reason);
    };
    text += QStringLiteral("Local: %1\nPublished: %2\n%3\nQuery version: %4\nRules version: %5\nFinding: %6\n")
        .arg(describe(*assessment.local), describe(*assessment.published), applicabilityReasonText(assessment.published->reason),
             assessment.published->queryVersion).arg(assessment.published->rulesVersion).arg(assessment.hasFinding() ? "Yes" : "No");
    for (const auto& e : assessment.local->evidence) {
        text += QStringLiteral("\naffected[%1], range[%2]: %3\n%4\n")
            .arg(e.affectedIndex).arg(e.rangeIndex).arg(applicabilityReasonCode(e.reason), applicabilityReasonText(e.reason));
        if (e.wildcard) text += QStringLiteral("OSV wildcard affected-package evidence：原始 package.name 为 *；当前 ecosystem 匹配该 ecosystem-wide package wildcard。\n");
        text += QString::fromUtf8(QJsonDocument(e.detail).toJson(QJsonDocument::Indented));
        if (text.size() > 65536) { text = text.left(65536) + QStringLiteral("\n[解释显示截断；完整证据保留在结果中]"); break; }
    }
    return text;
}
