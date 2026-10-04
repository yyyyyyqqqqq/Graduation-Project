#pragma once
#include "RiskPriority.h"
#include <QMetaType>
class QSaveFile;

// Immutable capture. Domain snapshots may contain internal members: only the
// renderer's explicit field allowlist is reportable, never whole-object dumps.
// No request, candidate index, repository, storage path or UI object is retained.
struct FindingReportContext final {
    const QString projectName, projectDescription;
    const QueryIdentity identity;
    const QString osvId;
    const QStringList cveAliases;
    const QString applicabilitySummary, osvAcquisition;
    const QDateTime osvFetchedAt;
    const RiskEvidenceProfile profile;
    const RiskPriorityAssessment assessment;
    const QDateTime capturedAt;
    const QString applicationVersion;
    const int databaseSchema;
};

enum class ReportError { None, NoAnalysis, StateChanged, ProjectRead, Render, FileOpen, FileWrite, FileCommit };
struct ReportCaptureResult {
    std::optional<FindingReportContext> context;
    ReportError error = ReportError::None;
};
struct ReportWriteResult {
    ReportError error = ReportError::None;
    qint64 outputBytes = 0;
    qint64 renderNanoseconds = 0, writeNanoseconds = 0;
};
Q_DECLARE_METATYPE(ReportWriteResult)

namespace FindingReport {
QByteArray render(const FindingReportContext&); // Pure value → self-contained UTF-8 HTML.
// Narrow writer operation; caller owns the local QSaveFile. Also permits deterministic
// QFileDevice write-error tests without disk exhaustion or a production fault switch.
ReportError saveBytes(QSaveFile& file, const QByteArray& bytes);
ReportWriteResult write(const FindingReportContext&, const QString& destination); // Worker only.
QString errorCode(ReportError);
QString userMessage(ReportError);
}
