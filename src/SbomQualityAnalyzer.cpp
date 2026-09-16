#include "SbomQualityAnalyzer.h"

#include <QSet>
#include <QStringView>

namespace
{
struct Rule
{
    const char* name;
    QualitySeverity severity;
    const char* explanation;
};

Rule rule(QualityIssueCode code)
{
    using enum QualityIssueCode;
    using enum QualitySeverity;
    switch (code) {
    case MissingComponentName: return {"MissingComponentName", Error, "组件缺少名称，无法可靠展示或识别该组件。"};
    case MissingComponentVersion: return {"MissingComponentVersion", Warning, "组件缺少版本信息，后续漏洞版本适用性判断可能不可靠。"};
    case MissingComponentPurl: return {"MissingComponentPurl", Warning, "组件缺少 PURL，后续软件包身份识别能力可能降低。"};
    case MissingBomRef: return {"MissingBomRef", Warning, "组件缺少 bom-ref，后续依赖引用无法可靠关联该组件。"};
    case MissingComponentType: return {"MissingComponentType", Warning, "组件缺少类型，后续组件分类依据不足。"};
    case DuplicateBomRef: return {"DuplicateBomRef", Error, "bom-ref 与先前对象重复，依赖引用无法唯一定位对象。"};
    case DuplicatePurl: return {"DuplicatePurl", Warning, "PURL 与先前对象完全相同，可能存在重复记录；不据此认定业务上非法。"};
    case MissingDependencyRef: return {"MissingDependencyRef", Error, "依赖条目缺少 ref，无法确定依赖来源。"};
    case UnknownDependencyRef: return {"UnknownDependencyRef", Error, "依赖来源未在当前 SBOM 的已知 bom-ref 中找到，无法解析该来源。"};
    case EmptyDependsOnRef: return {"EmptyDependsOnRef", Error, "依赖目标为空，后续依赖分析无法解析该边。"};
    case UnknownDependsOnRef: return {"UnknownDependsOnRef", Error, "依赖目标未在当前 SBOM 的已知 bom-ref 中找到，无法解析该边。"};
    case SelfDependency: return {"SelfDependency", Warning, "依赖条目引用自身，请核对原始数据；当前仅报告，不移除该引用。"};
    case DuplicateDependencyRef: return {"DuplicateDependencyRef", Warning, "同一 ref 已在先前依赖条目中出现，后续依赖分析需处理重复条目。"};
    case DuplicateDependsOnTarget: return {"DuplicateDependsOnTarget", Warning, "同一条目重复引用该目标，后续依赖分析需处理重复边。"};
    case MissingMetadataComponent: return {"MissingMetadataComponent", Info, "未提供元数据根组件，缺少对 SBOM 所描述主体的说明。"};
    case NoComponents: return {"NoComponents", Warning, "组件列表为空，没有可用于后续组件风险分析的列表数据（不含元数据根组件）。"};
    }
    Q_UNREACHABLE();
}

bool missing(const QString& value) { return QStringView(value).trimmed().isEmpty(); }
}

QualitySeverity SbomQualityIssue::severity() const { return rule(code).severity; }
QString SbomQualityIssue::codeName() const { return QString::fromLatin1(rule(code).name); }
QString SbomQualityIssue::message() const { return QString::fromUtf8(rule(code).explanation); }

void SbomQualityReport::append(SbomQualityIssue issue)
{
    ++m_counts.at(static_cast<size_t>(issue.severity()));
    m_issues.append(std::move(issue));
}

SbomQualityReport SbomQualityAnalyzer::analyze(const SbomDocument& document)
{
    SbomQualityReport report;
    using enum QualityIssueCode;
    if (!document.metadata.component)
        report.append({MissingMetadataComponent, QualityScope::Document});
    if (document.components.isEmpty())
        report.append({NoComponents, QualityScope::Document});

    QSet<QString> knownRefs;
    QSet<QString> knownPurls;
    knownRefs.reserve(document.components.size() + 1);
    knownPurls.reserve(document.components.size() + 1);
    const auto componentIssues = [&](const SbomComponent& component, QualityScope scope, qsizetype index) {
        const auto add = [&](QualityIssueCode code) { report.append({code, scope, index}); };
        if (missing(component.name)) add(MissingComponentName);
        if (missing(component.version)) add(MissingComponentVersion);
        if (missing(component.purl)) add(MissingComponentPurl);
        if (missing(component.bomRef)) add(MissingBomRef);
        if (missing(component.type)) add(MissingComponentType);
        if (!missing(component.bomRef)) {
            if (knownRefs.contains(component.bomRef)) add(DuplicateBomRef);
            knownRefs.insert(component.bomRef);
        }
        if (!missing(component.purl)) {
            if (knownPurls.contains(component.purl)) add(DuplicatePurl);
            knownPurls.insert(component.purl);
        }
    };
    if (document.metadata.component)
        componentIssues(*document.metadata.component, QualityScope::MetadataComponent, -1);
    for (qsizetype i = 0; i < document.components.size(); ++i)
        componentIssues(document.components.at(i), QualityScope::Component, i);

    QSet<QString> dependencyRefs;
    dependencyRefs.reserve(document.dependencies.size());
    for (qsizetype i = 0; i < document.dependencies.size(); ++i) {
        const auto& dependency = document.dependencies.at(i);
        const auto add = [&](QualityIssueCode code, qsizetype target = -1) {
            report.append({code, QualityScope::Dependency, -1, i, target});
        };
        const bool refMissing = missing(dependency.ref);
        if (refMissing) {
            add(MissingDependencyRef);
        } else {
            if (!knownRefs.contains(dependency.ref)) add(UnknownDependencyRef);
            if (dependencyRefs.contains(dependency.ref)) add(DuplicateDependencyRef);
            dependencyRefs.insert(dependency.ref);
        }
        QSet<QString> targets;
        targets.reserve(dependency.dependsOn.size());
        for (qsizetype j = 0; j < dependency.dependsOn.size(); ++j) {
            const auto& target = dependency.dependsOn.at(j);
            if (missing(target)) {
                add(EmptyDependsOnRef, j);
                continue; // Empty strings do not identify an object or a self-edge.
            }
            if (!knownRefs.contains(target)) add(UnknownDependsOnRef, j);
            if (!refMissing && target == dependency.ref) add(SelfDependency, j);
            if (targets.contains(target)) add(DuplicateDependsOnTarget, j);
            targets.insert(target);
        }
    }
    return report;
}
