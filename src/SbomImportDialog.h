#pragma once

#include "CycloneDxParser.h"
#include "SbomQualityAnalyzer.h"
#include <QDialog>
#include <QFutureWatcher>
#include <QPointer>

class AppLogger;
class QFileDialog;
class QLabel;
class QPushButton;
class QTextBrowser;
class SbomPreviewModel;
class SbomQualityModel;

// One project-bound, window-modal preview. Closing releases its session data.
// The worker owns only a path; it never touches this dialog, the logger or SQLite.
class SbomImportDialog final : public QDialog
{
    Q_OBJECT
public:
    SbomImportDialog(const QString& projectName, AppLogger& logger, QWidget* parent = nullptr);
    void chooseFile();
    void importFile(const QString& path);

signals:
    void importFinished(bool success);

private:
    struct ImportResult {
        SbomParseResult parsed;
        SbomQualityReport quality;
    };
    AppLogger& m_logger;
    QFutureWatcher<ImportResult> m_watcher;
    QPointer<QFileDialog> m_picker;
    SbomPreviewModel* m_model;
    SbomQualityModel* m_qualityModel;
    QLabel* m_qualitySummary;
    QTextBrowser* m_summary;
    QLabel* m_status;
    QPushButton* m_choose;
    QString m_pendingFileName;
    bool m_importing = false; // Covers worker execution through GUI result delivery.
};
