#include "FindingReport.h"
#include "PresentationText.h"
#include <QElapsedTimer>
#include <QSaveFile>

namespace {
QString escaped(const QString& text)
{
    return text.toHtmlEscaped().replace('\'', "&#39;");
}
QString utc(const QDateTime& time)
{
    return time.isValid() ? time.toUTC().toString(Qt::ISODateWithMs) : QStringLiteral("不可用 (Unavailable)");
}
}

QByteArray FindingReport::render(const FindingReportContext& c)
{
    using namespace PresentationText;
    QString html = QStringLiteral(
        "<!doctype html><html lang=\"zh-CN\"><head><meta charset=\"utf-8\">"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        "<title>Current Finding Analysis Report</title><style>"
        "body{font:16px/1.65 system-ui,sans-serif;color:#243b53;max-width:1000px;margin:32px auto;padding:0 24px}"
        "h1{font-size:28px}h2{font-size:21px;border-bottom:1px solid #cbd5e1;padding-bottom:6px;margin-top:32px}"
        "h3{font-size:17px}p{white-space:pre-wrap;overflow-wrap:anywhere}"
        ".evidence{border-left:3px solid #cbd5e1;padding-left:16px;margin:20px 0}"
        "@media print{body{max-width:none;margin:0;padding:0;font-size:11pt}h2,h3{break-after:avoid}}"
        "</style></head><body><h1>当前漏洞项分析报告<br>Current Finding Analysis Report</h1>");
    const auto section = [&](const QString& heading, const QString& text) {
        html += "<h2>" + escaped(heading) + "</h2><p>" + escaped(text) + "</p>";
    };
    const auto evidence = [&](const QString& heading, const QString& text) {
        html += "<div class=\"evidence\"><h3>" + escaped(heading) + "</h3><p>" + escaped(text) + "</p></div>";
    };
    section(QStringLiteral("1 · 范围与限制 / Scope & Limitations"), QStringLiteral(
        "本报告针对生成时当前选择的一个 confirmed Finding，不代表对整个项目进行了全量漏洞扫描或整体安全评估。\n"
        "本报告消费已有分析结果，不刷新证据，不提供项目级排名、综合风险评分或安全认证。"));
    section(QStringLiteral("2 · 时间范围 / Time Scope"), QStringLiteral(
        "本报告为静态分析快照。报告中的“当前 / Current”等决策状态均表示 Assessment Evaluated At 时刻的 RiskPriorityAssessment 状态，"
        "不表示之后阅读该 HTML 文件时的实时状态。报告生成后不会自动刷新 Provider Evidence，也不会重新计算 Freshness。\n"
        "Profile Generated At (UTC)：%1\nAssessment Evaluated At (UTC)：%2\nReport Snapshot Captured At (UTC)：%3\n"
        "Profile Generated At 是证据快照形成时间；Assessment Evaluated At 是 Priority / Support / Driver / 当前决策新鲜度的权威时间锚点；"
        "Report Snapshot Captured At 是报告上下文固定时间，不是风险评估时间。")
        .arg(utc(c.profile.generatedAt), utc(c.assessment.evaluatedAt), utc(c.capturedAt)));
    section(QStringLiteral("3 · 项目上下文 / Project Context"),
        QStringLiteral("项目名称：%1\n项目描述：%2").arg(c.projectName, c.projectDescription));
    section(QStringLiteral("4 · 组件上下文 / Component Context"),
        QStringLiteral("QueryIdentity\nEcosystem：%1\nPackage：%2\nVersion：%3")
            .arg(c.identity.ecosystem, c.identity.name, c.identity.version));
    section(QStringLiteral("5 · 当前漏洞项 / Finding"),
        QStringLiteral("OSV ID：%1\nCVE aliases：%2\n已确认受影响 (Affected)\n"
                       "Analysis Source Type：OSV Package-Version Query\nOSV Snapshot Acquisition：%3\nSnapshot Fetched At (UTC)：%4")
            .arg(c.osvId, c.cveAliases.isEmpty() ? QStringLiteral("无合法 CVE (No-CVE)") : c.cveAliases.join('\n'),
                 c.osvAcquisition, utc(c.osvFetchedAt)));
    section(QStringLiteral("6 · 当前结论 / Current Conclusion"), conclusionText(c.assessment));
    section(QStringLiteral("7 · 为什么 / Why"), whyText(c.assessment));
    section(QStringLiteral("8 · 驱动证据 / Driver"), driverText(c.assessment));
    section(QStringLiteral("9 · 分析证据 / Evidence"), QStringLiteral("保留驱动与非驱动 CVE 的证据、未评分和缺口；获取来源与新鲜度分别解释。"));
    const DecisionFreshnessLookup lookup(c.assessment);
    for (const auto& row : c.profile.epss) {
        evidence("FIRST EPSS", epssText(row, lookup) + '\n' + profileSnapshotFreshnessLabel(row.freshness)
            + QStringLiteral("\nFIRST API Version：%1\nScore Date：%2\nProvider Error：%3\nCache Error：%4")
                .arg(row.providerVersion, row.scoreDate.toString(Qt::ISODate), queryErrorCode(row.error), queryErrorCode(row.cacheError)));
    }
    for (const auto& row : c.profile.kev) {
        QString text = kevText(row, lookup) + '\n' + profileSnapshotFreshnessLabel(row.freshness)
            + QStringLiteral("\nCatalog Version：%1\nDate Released：%2\nProvider Error：%3\nCache Error：%4")
                .arg(row.catalogVersion, row.dateReleased, queryErrorCode(row.error), queryErrorCode(row.cacheError));
        // Explicit provider field allowlist. No raw entry serialization, extra keys or paths.
        for (const auto* field : {"vulnerabilityName", "vendorProject", "product", "dateAdded", "shortDescription",
                                  "requiredAction", "dueDate", "knownRansomwareCampaignUse", "notes"})
            if (row.entry.value(field).isString()) text += '\n' + QString::fromLatin1(field) + "：" + row.entry.value(field).toString();
        evidence("CISA KEV", text);
    }
    evidence(QStringLiteral("严重性上下文 / Severity Context"), severityText(c.profile.severity));
    for (const auto& item : c.profile.severity.items)
        evidence(QStringLiteral("严重性来源 / Severity Provenance"), QStringLiteral("Source：%1\nRecord：%2").arg(item.source, item.recordId));
    QString dependency = dependencyText(c.profile.dependency);
    if (c.profile.dependency.directDependents) dependency += QStringLiteral("\n直接依赖方数量：%1").arg(*c.profile.dependency.directDependents);
    if (c.profile.dependency.transitiveDependents) dependency += QStringLiteral("\n传递依赖方数量：%1").arg(*c.profile.dependency.transitiveDependents);
    evidence(QStringLiteral("依赖上下文 / Dependency Context"), dependency);
    evidence(QStringLiteral("质量证据 / Quality"), qualityLabel(c.profile.quality));
    section(QStringLiteral("10 · 解读边界 / Interpretation Boundaries"), boundaryText(c.assessment));
    section(QStringLiteral("11 · 方法信息 / Method Metadata"),
        QStringLiteral("Application Version：%1\nDatabase Schema：%2\nRules Version：v%3\nProduction EPSS Percentile Research Threshold：%4")
            .arg(c.applicationVersion).arg(c.databaseSchema).arg(ProductionRulesVersion).arg(ProductionEpssPercentileThreshold, 0, 'f', 2));
    section(QStringLiteral("12 · 技术附录 / Technical Appendix"),
        QStringLiteral("Applicability：Affected\n%1\nPriority：%2\nDecision Evidence Support：%3\nDriver Provider：%4\nDriver CVE：%5\n"
                       "Assessment Rules：v%6\nAssessment EPSS Percentile Threshold：%7")
            .arg(c.applicabilitySummary, priorityClassText(c.assessment.priority), decisionEvidenceSupportText(c.assessment.support),
                 priorityDriverLabel(c.assessment.driverKind), c.assessment.driverCve)
            .arg(c.assessment.rulesVersion).arg(c.assessment.epssPercentileThreshold, 0, 'f', 2));
    for (const auto& reason : c.assessment.reasons)
        evidence(priorityReasonCode(reason.code), reasonText(reason, c.assessment.epssPercentileThreshold));
    html += "</body></html>";
    return html.toUtf8();
}

ReportError FindingReport::saveBytes(QSaveFile& file, const QByteArray& bytes)
{
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly)) return ReportError::FileOpen;
    if (file.write(bytes) != bytes.size()) { file.cancelWriting(); return ReportError::FileWrite; }
    return file.commit() ? ReportError::None : ReportError::FileCommit;
}

ReportWriteResult FindingReport::write(const FindingReportContext& context, const QString& destination)
{
    ReportWriteResult result;
    QElapsedTimer timer;
    timer.start();
    QByteArray bytes;
    try { bytes = render(context); }
    catch (...) { result.error = ReportError::Render; return result; }
    result.renderNanoseconds = timer.nsecsElapsed();
    if (bytes.isEmpty()) { result.error = ReportError::Render; return result; }
    result.outputBytes = bytes.size();
    timer.restart();
    QSaveFile file(destination);
    result.error = saveBytes(file, bytes); // Never risk an existing target on a failed export.
    result.writeNanoseconds = timer.nsecsElapsed();
    return result;
}

QString FindingReport::errorCode(ReportError error)
{
    switch (error) {
    case ReportError::None: return "None";
    case ReportError::NoAnalysis: return "NoAnalysis";
    case ReportError::StateChanged: return "StateChanged";
    case ReportError::ProjectRead: return "ProjectRead";
    case ReportError::Render: return "Render";
    case ReportError::FileOpen: return "FileOpen";
    case ReportError::FileWrite: return "FileWrite";
    case ReportError::FileCommit: return "FileCommit";
    }
    Q_UNREACHABLE();
}
QString FindingReport::userMessage(ReportError error)
{
    switch (error) {
    case ReportError::None: return QStringLiteral("当前漏洞项报告已保存。");
    case ReportError::NoAnalysis: return QStringLiteral("没有可导出的完整当前分析，请先选择已确认的漏洞项并完成证据加载。");
    case ReportError::StateChanged: return QStringLiteral("当前分析状态或关联已变化，请重新选择漏洞项并完成分析后再导出。");
    case ReportError::ProjectRead: return QStringLiteral("无法读取当前项目，请刷新项目列表后重试。");
    case ReportError::Render: return QStringLiteral("报告生成失败，未保存报告。");
    case ReportError::FileOpen: return QStringLiteral("无法创建报告文件，请选择可写的保存位置。");
    case ReportError::FileWrite: return QStringLiteral("报告写入失败，未替换目标文件。");
    case ReportError::FileCommit: return QStringLiteral("报告保存未完成，未替换目标文件。");
    }
    Q_UNREACHABLE();
}
