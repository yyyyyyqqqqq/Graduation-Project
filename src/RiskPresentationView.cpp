#include "RiskPresentationView.h"
#include "PresentationText.h"

RiskPresentationView::RiskPresentationView(QWidget* parent) : QTextBrowser(parent)
{
    setObjectName("riskHumanPresentation");
    setOpenLinks(false);
    setOpenExternalLinks(false);
    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
}
void RiskPresentationView::showResult(const RiskEvidenceProfile& p, const RiskPriorityAssessment& a)
{
    if (p.key != a.key || p.generatedAt != a.profileGeneratedAt) {
        setPlainText(QStringLiteral("结果关联信息不可用：证据快照与评估不属于同一结果。"));
        return;
    }
    using namespace PresentationText;
    // Escape every value, including fixed text, before constructing the visual sections.
    const auto escaped = [](const QString& text) { return text.toHtmlEscaped().replace('\n', "<br>"); };
    QString html = "<html><body style='color:#243b53;'>";
    const auto section = [&](const QString& title, const QString& body) {
        html += "<h3 style='color:#234c74;'>" + escaped(title) + "</h3><p>" + escaped(body) + "</p>";
    };
    section(QStringLiteral("1 · 结论 / Conclusion"), conclusionText(a));
    section(QStringLiteral("2 · 为什么 / Why"), whyText(a));
    section(QStringLiteral("3 · 驱动证据 / Driver"), driverText(a));
    html += "<h3>4 · 关键证据 / Key Evidence</h3>";
    const DecisionFreshnessLookup lookup(a);
    QStringList kev, epss;
    // Keep every provider row, including failures and gaps. Never truncate coverage.
    for (const auto& row : p.kev) kev.append(kevText(row, lookup));
    for (const auto& row : p.epss) epss.append(epssText(row, lookup));
    section("CISA KEV", kev.isEmpty() ? QStringLiteral("未提供 KEV 证据行。") : kev.join("\n\n"));
    section("FIRST EPSS", epss.isEmpty() ? QStringLiteral("未提供 EPSS 证据行。") : epss.join("\n\n"));
    section(QStringLiteral("严重性上下文 / Severity Context"), severityText(p.severity));
    section(QStringLiteral("依赖上下文 / Dependency Context"), dependencyText(p.dependency));
    section(QStringLiteral("质量证据 / Quality Availability"), qualityLabel(p.quality));
    section(QStringLiteral("5 · 这个结论不能说明什么"), boundaryText(a));
    html += "</body></html>";
    setHtml(html);
}
