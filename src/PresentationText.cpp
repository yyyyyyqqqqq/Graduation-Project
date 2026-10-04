#include "PresentationText.h"
#include "CveIdentity.h"

namespace PresentationText {
QString priorityLabel(PriorityClass value)
{
    switch (value) {
    case PriorityClass::KnownExploited: return QStringLiteral("存在已知利用证据 (Known Exploited)");
    case PriorityClass::AboveResearchPercentileThreshold: return QStringLiteral("达到或高于研究百分位阈值 (Above Research Percentile Threshold)");
    case PriorityClass::BelowResearchPercentileThreshold: return QStringLiteral("低于研究百分位阈值 (Below Research Percentile Threshold)");
    case PriorityClass::InsufficientCurrentExploitEvidence: return QStringLiteral("当前利用证据不足 (Insufficient Current Exploit Evidence)");
    }
    Q_UNREACHABLE();
}
QString supportLabel(DecisionEvidenceSupport value)
{
    switch (value) {
    case DecisionEvidenceSupport::Complete: return QStringLiteral("决策证据支持：完整 (Complete)");
    case DecisionEvidenceSupport::Partial: return QStringLiteral("决策证据支持：部分 (Partial)");
    case DecisionEvidenceSupport::Insufficient: return QStringLiteral("决策证据支持：不足 (Insufficient)");
    }
    Q_UNREACHABLE();
}
QString priorityDriverLabel(PriorityDriverKind value)
{
    switch (value) {
    case PriorityDriverKind::Kev: return QStringLiteral("CISA KEV");
    case PriorityDriverKind::Epss: return QStringLiteral("FIRST EPSS");
    case PriorityDriverKind::None: return QStringLiteral("无当前利用信号驱动 (None)");
    }
    Q_UNREACHABLE();
}
QString acquisitionLabel(EvidenceAcquisition value)
{
    switch (value) {
    case EvidenceAcquisition::Live: return QStringLiteral("实时获取 (Live)");
    case EvidenceAcquisition::Cache: return QStringLiteral("缓存获取 (Cache)");
    case EvidenceAcquisition::StaleFallback: return QStringLiteral("在线刷新失败后使用缓存 (Stale Fallback)");
    case EvidenceAcquisition::None: return QStringLiteral("无获取来源 (None)");
    }
    Q_UNREACHABLE();
}
QString kevStatusLabel(KevStatus value)
{
    switch (value) {
    case KevStatus::Listed: return QStringLiteral("该 KEV 快照已收录 (Listed)");
    case KevStatus::NotListed: return QStringLiteral("该 KEV 快照未收录 (NotListed)");
    case KevStatus::Unknown: return QStringLiteral("KEV 状态未知 (Unknown)");
    case KevStatus::NotQueryable: return QStringLiteral("无可用于 KEV 目录匹配的合法 CVE (NotQueryable)");
    }
    Q_UNREACHABLE();
}
QString epssStatusLabel(EpssStatus value)
{
    switch (value) {
    case EpssStatus::Available: return QStringLiteral("存在评分记录 (Available)");
    case EpssStatus::NotScored: return QStringLiteral("该 EPSS 查询快照未返回评分记录 (NotScored)");
    case EpssStatus::Failed: return QStringLiteral("EPSS 查询失败 (Failed)");
    case EpssStatus::InvalidResponse: return QStringLiteral("EPSS 响应无效 (Invalid Response)");
    case EpssStatus::NotQueryable: return QStringLiteral("无可用于 EPSS 查询的合法 CVE (NotQueryable)");
    }
    Q_UNREACHABLE();
}
QString effectiveFreshnessLabel(EffectiveDecisionFreshness value)
{
    switch (value) {
    case EffectiveDecisionFreshness::Fresh: return QStringLiteral("当前决策证据有效 (Fresh)");
    case EffectiveDecisionFreshness::Stale: return QStringLiteral("当前决策证据为 Stale 上下文 (Stale)");
    case EffectiveDecisionFreshness::NotApplicable: return QStringLiteral("当前决策新鲜度不适用 (Not Applicable)");
    case EffectiveDecisionFreshness::Ineligible: return QStringLiteral("当前决策不可用 (Ineligible)");
    }
    Q_UNREACHABLE();
}
QString profileSnapshotFreshnessLabel(EvidenceFreshness value)
{
    switch (value) {
    case EvidenceFreshness::Fresh: return QStringLiteral("Profile 生成时新鲜度：Fresh（当时有效）");
    case EvidenceFreshness::Stale: return QStringLiteral("Profile 生成时新鲜度：Stale（当时未满足 Fresh 条件）");
    case EvidenceFreshness::NotApplicable: return QStringLiteral("Profile 生成时新鲜度：Not Applicable（不适用）");
    }
    Q_UNREACHABLE();
}
QString severityStatusLabel(SeverityStatus value)
{
    switch (value) {
    case SeverityStatus::Present: return QStringLiteral("存在严重性证据 (Present)");
    case SeverityStatus::UnsupportedType: return QStringLiteral("严重性类型暂不支持解释 (Unsupported Type)");
    case SeverityStatus::InvalidStructure: return QStringLiteral("严重性证据结构无效 (Invalid Structure)");
    case SeverityStatus::SchemaConflict: return QStringLiteral("严重性证据存在结构冲突 (Schema Conflict)");
    case SeverityStatus::Missing: return QStringLiteral("未提供严重性证据 (Missing)");
    }
    Q_UNREACHABLE();
}
QString qualityLabel(QualityEvidenceStatus value)
{
    switch (value) {
    case QualityEvidenceStatus::UnavailableForPersistedCurrentState:
        return QStringLiteral("当前持久化状态无完整质量证据 (Unavailable for Persisted Current State)\n"
            "当前持久化 Current State 不包含完整 SBOM Quality Report，因此当前呈现无法提供完整质量证据。");
    }
    Q_UNREACHABLE();
}
QString dependencyPathLabel(DependencyPathState value)
{
    switch (value) {
    case DependencyPathState::RootComponentSelf: return QStringLiteral("当前组件即依赖根组件 (Root Component Self)");
    case DependencyPathState::ResolvedPathFound: return QStringLiteral("已观察到解析依赖路径 (Resolved Path Found)");
    case DependencyPathState::NoResolvedPath: return QStringLiteral("未观察到解析依赖路径 (No Resolved Path)");
    case DependencyPathState::RootMissing: return QStringLiteral("依赖根组件缺失 (Root Missing)");
    case DependencyPathState::RootAmbiguous: return QStringLiteral("依赖根组件存在歧义 (Root Ambiguous)");
    case DependencyPathState::NotCaptured: return QStringLiteral("未捕获依赖信息 (Not Captured)");
    }
    Q_UNREACHABLE();
}
QString dependencyRootLabel(DependencyRootStatus value)
{
    switch (value) {
    case DependencyRootStatus::RootAvailable: return QStringLiteral("依赖根组件可用 (Root Available)");
    case DependencyRootStatus::RootMissing: return QStringLiteral("依赖根组件缺失 (Root Missing)");
    case DependencyRootStatus::RootAmbiguous: return QStringLiteral("依赖根组件存在歧义 (Root Ambiguous)");
    case DependencyRootStatus::NotCaptured: return QStringLiteral("未捕获依赖信息 (Not Captured)");
    }
    Q_UNREACHABLE();
}
QString referenceResolutionLabel(ReferenceResolutionCompleteness value)
{
    switch (value) {
    case ReferenceResolutionCompleteness::Complete: return QStringLiteral("引用解析完整 (Complete)");
    case ReferenceResolutionCompleteness::Partial: return QStringLiteral("引用解析不完整 (Partial)");
    }
    Q_UNREACHABLE();
}
QString reasonText(const PriorityReason& reason, double threshold)
{
    QString text;
    switch (reason.code) {
    case PriorityReasonCode::FreshKevListed: text = QStringLiteral("有效的 KEV 收录证据独立支持“存在已知利用证据”，优先于 EPSS。"); break;
    case PriorityReasonCode::StaleKevListedContextOnly: text = QStringLiteral("历史 KEV 收录证据仅作上下文，不驱动当前优先级。"); break;
    case PriorityReasonCode::StaleKevContextOnly: text = QStringLiteral("该 KEV 目录证据当前为 Stale，仅作历史上下文，不作为 Fresh 决策驱动。"); break;
    case PriorityReasonCode::FreshEpssAboveThreshold: text = QStringLiteral("当前有效 EPSS 百分位达到或高于本次研究阈值 %1（>=）。").arg(threshold, 0, 'f', 2); break;
    case PriorityReasonCode::FreshEpssBelowThreshold: text = QStringLiteral("当前有效 EPSS 百分位低于本次研究阈值 %1；不代表安全或低风险。").arg(threshold, 0, 'f', 2); break;
    case PriorityReasonCode::EpssDriverSelected: text = QStringLiteral("规则选择了当前有效评分中的最大 EPSS 百分位；相等时按规范 CVE 顺序确定来源。"); break;
    case PriorityReasonCode::DecisionEvidenceComplete: text = QStringLiteral("本规则做出当前判断所需的证据当前、完整、可用；不表示所有漏洞数据全部完整。"); break;
    case PriorityReasonCode::DecisionEvidencePartial: text = QStringLiteral("存在当前有效的 EPSS 驱动，但其他 CVE 或提供方的佐证不完整；逐项缺口见下方证据。"); break;
    case PriorityReasonCode::DecisionEvidenceInsufficient: text = QStringLiteral("缺少支持当前判断的有效利用信号，不能据此得出低风险结论。"); break;
    case PriorityReasonCode::NoFreshExploitDriver: text = QStringLiteral("没有当前有效的 KEV 收录证据，也没有当前有效的 EPSS 评分驱动。"); break;
    case PriorityReasonCode::KevCoverageIncompleteOrStale: text = QStringLiteral("此 CVE 缺少来自完整目录且当前有效的 KEV 未收录佐证；未收录也不证明从未被利用。"); break;
    case PriorityReasonCode::EpssAliasCoverageIncomplete: text = QStringLiteral("此 CVE 缺少当前有效的 EPSS 评分记录；缺失或未评分不等于零。"); break;
    case PriorityReasonCode::StaleEpssContextOnly: text = QStringLiteral("该 EPSS 证据仅作历史上下文，不驱动当前优先级。"); break;
    case PriorityReasonCode::EvidenceTimeInvalidForCurrentDecision: text = QStringLiteral("证据时间不适用于当前决策；这是不可用（Ineligible），不是 Stale 上下文。"); break;
    case PriorityReasonCode::SeverityContextOnly: text = QStringLiteral("技术严重性仅作上下文，不直接决定利用信号优先级。"); break;
    case PriorityReasonCode::DependencyContextOnly: text = QStringLiteral("依赖仅作上下文，不证明运行时可达，也不改变优先级或证据支持度。"); break;
    case PriorityReasonCode::QualityUnavailable: text = QStringLiteral("持久化状态没有完整质量报告；这不是本规则的必需证据，不降低决策证据支持度。"); break;
    }
    return reason.cve.isEmpty() ? text : reason.cve + " / " + priorityDriverLabel(reason.provider) + "：" + text;
}
QString conclusionText(const RiskPriorityAssessment& a)
{
    return priorityLabel(a.priority) + '\n' + supportLabel(a.support)
        + QStringLiteral("\n本次 Rules v%1 · EPSS 百分位阈值 %2\n决策证据支持描述判断所需证据的当前有效性、完整性和可用性，不是预测置信度。")
            .arg(a.rulesVersion).arg(a.epssPercentileThreshold, 0, 'f', 2);
}
QString whyText(const RiskPriorityAssessment& a)
{
    QStringList text;
    for (const auto& reason : a.reasons) text.append(reasonText(reason, a.epssPercentileThreshold));
    return text.isEmpty() ? QStringLiteral("当前评估未提供原因记录。") : text.join('\n');
}
namespace {
QString number(std::optional<double> value)
{
    return value ? QString::number(*value, 'g', 12) : QStringLiteral("未提供 (Unavailable)");
}
QString fetchedAtText(const QDateTime& value)
{
    return value.isValid() ? value.toUTC().toString(Qt::ISODateWithMs) : QStringLiteral("不可用 (Unavailable)");
}
}
QString driverText(const RiskPriorityAssessment& a)
{
    QString text = priorityDriverLabel(a.driverKind);
    if (a.driverKind == PriorityDriverKind::None) return text + QStringLiteral("\n无驱动不代表无漏洞、无风险或安全。");
    text += "\nCVE：" + a.driverCve + '\n' + effectiveFreshnessLabel(a.driverFreshness)
        + '\n' + acquisitionLabel(a.driverAcquisition);
    if (a.driverKind == PriorityDriverKind::Epss)
        text += QStringLiteral("\nEPSS 概率 (Probability)：%1\nEPSS 百分位 (Percentile)：%2\nFIRST API Version：%3\n评分日期：%4")
            .arg(number(a.driverEpssProbability), number(a.driverEpssPercentile), a.driverEpssApiVersion, a.driverEpssScoreDate.toString(Qt::ISODate));
    if (a.driverKind == PriorityDriverKind::Kev)
        text += QStringLiteral("\n驱动事实：该 KEV 快照已收录 (Listed)\n目录版本：%1\n目录发布日期：%2")
            .arg(a.kevCatalogVersion, a.kevDateReleased);
    return text;
}
QString boundaryText(const RiskPriorityAssessment& a)
{
    QStringList text;
    text << QStringLiteral("“低于研究百分位阈值（Below Rules v1 Research Percentile Threshold）”仅表示未达到本次研究百分位阈值；低于阈值不代表安全、低风险或不可利用（Below ≠ Safe / Low Risk / Not Exploitable）。");
    if (a.priority == PriorityClass::InsufficientCurrentExploitEvidence)
        text << QStringLiteral("当前利用证据不足不是低风险结论。");
    if (a.support == DecisionEvidenceSupport::Partial)
        text << QStringLiteral("部分证据支持不代表“判断只有一半可信”。");
    if (a.support == DecisionEvidenceSupport::Complete)
        text << QStringLiteral("完整仅针对本规则当前判断所需证据，不表示所有漏洞数据全部完整。");
    text << QStringLiteral("决策证据支持不是预测置信度、漏洞概率或 Finding 正确性（Support ≠ predictive confidence）。")
         << QStringLiteral("EPSS 百分位是相对排名，不是利用概率；百分位 0.93 不等于 93% 利用概率。")
         << QStringLiteral("KEV 快照未收录不证明从未被利用；未知也不等于未收录。")
         << QStringLiteral("CISA KEV 使用公开完整目录：应用获取该目录后，在本地按 CVE 匹配，不会将每个 CVE 逐条发送给 CISA 查询。")
         << QStringLiteral("缺失、未评分和失败均不等于零。")
         << QStringLiteral("依赖路径不证明运行时可达；未观察到路径也不证明运行时不可达。")
         << QStringLiteral("引用解析完整只针对已捕获的引用，不保证 SBOM 依赖覆盖完整。")
         << QStringLiteral("质量证据不可用不代表质量良好或较差，也不降低本规则的证据支持度。")
         << QStringLiteral("严重性仅作上下文；缺失严重性证据不等于低风险。")
         << QStringLiteral("获取来源不等于新鲜度：缓存不必然过期，实时获取不保证当前有效。")
         << QStringLiteral("Stale 证据不作为 Fresh 决策驱动；可能因为超过刷新窗口，也可能因为在线刷新失败后使用缓存。")
         << QStringLiteral("当前新鲜度以评估时刻为准。StaleFallback 即使缓存未满 24 小时，也按当前规则作为 Stale 上下文。")
         << QStringLiteral("CISA KEV 字段是提供方证据，不是本系统的整改期限或强制指令。");
    return text.join('\n');
}
DecisionFreshnessLookup::DecisionFreshnessLookup(const RiskPriorityAssessment& a)
{
    for (const auto& row : a.evidenceFreshness) {
        if (m_rows.contains(row.cve)) m_duplicates.insert(row.cve);
        else m_rows.insert(row.cve, row);
    }
}
QString DecisionFreshnessLookup::label(const QString& cve, PriorityDriverKind provider, bool notQueryable) const
{
    if (!validCveId(cve) && notQueryable)
        return QStringLiteral("无可关联项：当前 evidence 没有合法 CVE，Assessment 不产生逐 CVE 当前决策新鲜度。");
    const auto row = m_rows.constFind(cve);
    if (!validCveId(cve) || row == m_rows.cend() || m_duplicates.contains(cve))
        return QStringLiteral("关联信息不可用（呈现一致性异常）");
    switch (provider) {
    case PriorityDriverKind::Epss: return effectiveFreshnessLabel(row->epss);
    case PriorityDriverKind::Kev: return effectiveFreshnessLabel(row->kev);
    case PriorityDriverKind::None: return QStringLiteral("关联信息不可用（未指定提供方）");
    }
    Q_UNREACHABLE();
}
QString epssText(const EpssEvidence& e, const DecisionFreshnessLookup& lookup)
{
    return QStringLiteral("CVE：%1\n%2\n当前决策新鲜度：%3\n获取来源：%4\n获取时间 / Fetched At (UTC)：%5\n概率 (Probability)：%6 · 百分位 (Percentile)：%7")
        .arg(e.cve.isEmpty() ? QStringLiteral("无合法 CVE") : e.cve, epssStatusLabel(e.status),
             lookup.label(e.cve, PriorityDriverKind::Epss, e.status == EpssStatus::NotQueryable),
             acquisitionLabel(e.acquisition), fetchedAtText(e.fetchedAt), number(e.probability), number(e.percentile));
}
QString kevText(const KevEvidence& e, const DecisionFreshnessLookup& lookup)
{
    QString text = QStringLiteral("CVE：%1\n%2\n当前决策新鲜度：%3\n获取来源：%4\n获取时间 / Fetched At (UTC)：%5")
        .arg(e.cve.isEmpty() ? QStringLiteral("无合法 CVE") : e.cve, kevStatusLabel(e.status),
             lookup.label(e.cve, PriorityDriverKind::Kev, e.status == KevStatus::NotQueryable), acquisitionLabel(e.acquisition), fetchedAtText(e.fetchedAt));
    if (e.status == KevStatus::NotQueryable)
        text += QStringLiteral("\n当前 Finding 没有合法 CVE，因此无法在已获取的 CISA KEV 目录中执行本地匹配。");
    return text;
}
QString severityText(const SeverityEvidence& severity)
{
    QString text = severityStatusLabel(severity.status);
    for (const auto& item : severity.items)
        text += '\n' + severityStatusLabel(item.status) + "\n" + item.type + "：" + item.vector;
    return text;
}
QString dependencyText(const EvidenceDependencyContext& d)
{
    QString text = dependencyRootLabel(d.root) + '\n' + dependencyPathLabel(d.path);
    if (d.referenceResolution) text += '\n' + referenceResolutionLabel(*d.referenceResolution);
    if (d.depthFromRoot) text += QStringLiteral("\n已解析路径深度：%1").arg(*d.depthFromRoot);
    return text;
}
}
