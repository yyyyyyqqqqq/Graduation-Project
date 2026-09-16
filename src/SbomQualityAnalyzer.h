#pragma once

#include "SbomDocument.h"
#include <array>

enum class QualityIssueCode {
    MissingComponentName, MissingComponentVersion, MissingComponentPurl, MissingBomRef,
    MissingComponentType, DuplicateBomRef, DuplicatePurl, MissingDependencyRef,
    UnknownDependencyRef, EmptyDependsOnRef, UnknownDependsOnRef, SelfDependency,
    DuplicateDependencyRef, DuplicateDependsOnTarget, MissingMetadataComponent, NoComponents
};

enum class QualitySeverity { Info, Warning, Error };
enum class QualityScope { Document, MetadataComponent, Component, Dependency };

struct SbomQualityIssue
{
    QualityIssueCode code;
    QualityScope scope;
    // Zero-based source positions; -1 means not applicable. Components use the
    // parser's flattened order. Metadata root is a separate scope, never row 0.
    qsizetype componentIndex = -1;
    qsizetype dependencyIndex = -1;
    qsizetype targetIndex = -1;

    // Derived from code so severity and explanation cannot disagree with the rule.
    QualitySeverity severity() const;
    QString codeName() const;
    QString message() const;
    bool operator==(const SbomQualityIssue&) const = default;
};

class SbomQualityReport
{
public:
    const QList<SbomQualityIssue>& issues() const { return m_issues; }
    qsizetype count(QualitySeverity severity) const { return m_counts.at(static_cast<size_t>(severity)); }

private:
    friend class SbomQualityAnalyzer;
    void append(SbomQualityIssue issue);
    QList<SbomQualityIssue> m_issues;
    std::array<qsizetype, 3> m_counts{};
};

// Read-only, Qt Core only. Report order: document notices, metadata root,
// flattened components, dependencies and their targets, all in source order.
// Blank/whitespace-only fields are missing; other identifiers compare exactly,
// without trimming, case folding or normalization. Each occurrence after the
// first emits a duplicate issue; blank identifiers emit only missing issues.
class SbomQualityAnalyzer final
{
public:
    static SbomQualityReport analyze(const SbomDocument& document);
};
