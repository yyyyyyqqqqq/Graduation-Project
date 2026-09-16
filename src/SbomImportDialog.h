#pragma once

#include "CycloneDxParser.h"
#include "SbomQualityAnalyzer.h"
#include "ComponentRepository.h"
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
class QCloseEvent;

// One project-bound, window-modal preview. Closing releases its session data.
// Workers capture values only, never this dialog, the logger or borrowed SQL handles.
class SbomImportDialog final : public QDialog
{
    Q_OBJECT
public:
    SbomImportDialog(const QString& projectId, const QString& projectName, const QString& databaseFile,
                     AppLogger& logger, QWidget* parent = nullptr);
    void chooseFile();
    void importFile(const QString& path);
    void applyToProject();
    void reject() override;

signals:
    void importFinished(bool success);
    void applyFinished(bool success);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    struct ImportResult {
        SbomParseResult parsed;
        SbomQualityReport quality;
    };
    AppLogger& m_logger;
    const QString m_projectId;
    const QString m_databaseFile;
    QFutureWatcher<ImportResult> m_watcher;
    QFutureWatcher<ComponentResult> m_applyWatcher;
    QPointer<QFileDialog> m_picker;
    SbomPreviewModel* m_model;
    SbomQualityModel* m_qualityModel;
    QLabel* m_qualitySummary;
    QTextBrowser* m_summary;
    QLabel* m_status;
    QPushButton* m_choose;
    QPushButton* m_apply;
    QPushButton* m_close;
    enum class ApplyState { Empty, Ready, Working, Applied };
    ApplyState m_applyState = ApplyState::Empty;
    void updateActions();
    QString m_pendingFileName;
    bool m_importing = false; // Covers worker execution through GUI result delivery.
};
