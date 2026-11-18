#include "SbomImportDialog.h"
#include "AppLogger.h"

#include <QAbstractTableModel>
#include <QCloseEvent>
#include <QColor>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QTableView>
#include <QTabWidget>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QtConcurrentRun>

namespace
{
QString previewText(const QString& text)
{
    // A single hostile metadata/cell string must not force the GUI to lay out megabytes.
    // The document retains the full value; this is a display limit, not parser validation.
    constexpr qsizetype limit = 512;
    return text.size() > limit ? text.left(limit) + QStringLiteral("…") : text;
}
}

// The table model owns the entire successful document. No per-cell copies or
// parallel component cache; QTableView requests only visible cells.
class SbomPreviewModel final : public QAbstractTableModel
{
public:
    explicit SbomPreviewModel(QObject* parent) : QAbstractTableModel(parent) {}
    int rowCount(const QModelIndex& parent = {}) const override
    { return parent.isValid() ? 0 : static_cast<int>(m_document.components.size()); }
    int columnCount(const QModelIndex& parent = {}) const override { return parent.isValid() ? 0 : 5; }
    QVariant data(const QModelIndex& index, int role) const override
    {
        if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()
            || role != Qt::DisplayRole) return {};
        const auto& component = m_document.components.at(index.row());
        switch (index.column()) {
        case 0: return previewText(component.name);
        case 1: return previewText(component.version);
        case 2: return previewText(component.type);
        case 3: return previewText(component.purl);
        case 4: return previewText(component.bomRef);
        }
        return {};
    }
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
            return QAbstractTableModel::headerData(section, orientation, role);
        const QStringList headings{QStringLiteral("名称 / Name"), QStringLiteral("版本 / Version"),
            QStringLiteral("类型 / Type"), QStringLiteral("PURL"), QStringLiteral("bom-ref")};
        return headings.value(section);
    }
    void replace(SbomDocument document)
    {
        beginResetModel();
        m_document = std::move(document);
        endResetModel();
    }
    const SbomDocument& document() const { return m_document; }
private:
    SbomDocument m_document;
};

// Owns the immutable report, with counts computed by the background analyzer.
// Only visible rows are formatted; no QTableWidget allocation per issue.
class SbomQualityModel final : public QAbstractTableModel
{
public:
    explicit SbomQualityModel(QObject* parent) : QAbstractTableModel(parent) {}
    int rowCount(const QModelIndex& parent = {}) const override
    { return parent.isValid() ? 0 : static_cast<int>(m_report.issues().size()); }
    int columnCount(const QModelIndex& parent = {}) const override { return parent.isValid() ? 0 : 4; }
    QVariant data(const QModelIndex& index, int role) const override
    {
        if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) return {};
        const auto& issue = m_report.issues().at(index.row());
        if (role == Qt::ToolTipRole) return issue.message();
        if (role == Qt::ForegroundRole && index.column() == 0) {
            switch (issue.severity()) {
            case QualitySeverity::Error: return QColor(QStringLiteral("#a12622"));
            case QualitySeverity::Warning: return QColor(QStringLiteral("#805700"));
            case QualitySeverity::Info: return QColor(QStringLiteral("#315f88"));
            }
        }
        if (role != Qt::DisplayRole) return {};
        switch (index.column()) {
        case 0:
            switch (issue.severity()) {
            case QualitySeverity::Info: return QStringLiteral("Info / 信息");
            case QualitySeverity::Warning: return QStringLiteral("Warning / 警告");
            case QualitySeverity::Error: return QStringLiteral("Error / 错误");
            }
            break;
        case 1: return issue.codeName();
        case 2:
            switch (issue.scope) {
            case QualityScope::Document: return QStringLiteral("SBOM 文档");
            case QualityScope::MetadataComponent: return QStringLiteral("元数据根组件");
            case QualityScope::Component: return QStringLiteral("Component #%1").arg(issue.componentIndex + 1);
            case QualityScope::Dependency:
                return issue.targetIndex < 0 ? QStringLiteral("Dependency #%1").arg(issue.dependencyIndex + 1)
                    : QStringLiteral("Dependency #%1 / target #%2").arg(issue.dependencyIndex + 1).arg(issue.targetIndex + 1);
            }
            break;
        case 3: return issue.message();
        }
        return {};
    }
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
            return QAbstractTableModel::headerData(section, orientation, role);
        const QStringList headings{QStringLiteral("严重程度 / Severity"), QStringLiteral("代码 / Code"),
            QStringLiteral("位置 / Location"), QStringLiteral("说明 / Description")};
        return headings.value(section);
    }
    void replace(SbomQualityReport report)
    {
        beginResetModel();
        m_report = std::move(report);
        endResetModel();
    }
private:
    SbomQualityReport m_report;
};

SbomImportDialog::SbomImportDialog(const QString& projectId, const QString& projectName, const QString& databaseFile,
                                 AppLogger& logger, QWidget* parent)
    : QDialog(parent), m_logger(logger), m_projectId(projectId), m_databaseFile(databaseFile), m_model(new SbomPreviewModel(this)),
      m_qualityModel(new SbomQualityModel(this))
{
    setObjectName(QStringLiteral("sbomImportDialog"));
    setWindowTitle(QStringLiteral("导入 SBOM · 预览、质量诊断与应用"));
    setWindowModality(Qt::WindowModal);
    resize(900, 600);
    setMinimumSize(560, 400);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(4);
    auto* project = new QLabel(QStringLiteral("当前项目：%1").arg(projectName), this);
    project->setObjectName(QStringLiteral("sbomProject"));
    project->setTextFormat(Qt::PlainText);
    project->setWordWrap(true);
    project->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    QSizePolicy projectPolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    projectPolicy.setHeightForWidth(true);
    project->setSizePolicy(projectPolicy);
    auto* projectScroll = new QScrollArea(this);
    projectScroll->setObjectName(QStringLiteral("sbomProjectScroll"));
    projectScroll->setWidgetResizable(true);
    projectScroll->setFrameShape(QFrame::NoFrame);
    projectScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    projectScroll->setFixedHeight(project->fontMetrics().lineSpacing() * 2 + 4);
    projectScroll->setWidget(project);
    layout->addWidget(projectScroll);
    auto* fileActions = new QHBoxLayout;
    auto* hint = new QLabel(QStringLiteral("CycloneDX JSON 1.4 / 1.5 / 1.6 · 最大 %1 MiB")
        .arg(CycloneDxParser::MaxFileBytes / (1024 * 1024)), this);
    hint->setObjectName(QStringLiteral("sbomFormatHint"));
    hint->setWordWrap(true);
    m_choose = new QPushButton(QStringLiteral("选择 SBOM 文件…"), this);
    m_choose->setObjectName(QStringLiteral("chooseSbomFile"));
    fileActions->addWidget(m_choose);
    fileActions->addWidget(hint, 1);
    layout->addLayout(fileActions);
    m_status = new QLabel(QStringLiteral("请选择文件。"), this);
    m_status->setObjectName(QStringLiteral("sbomStatus"));
    m_status->setTextFormat(Qt::PlainText);
    m_status->setWordWrap(true);
    layout->addWidget(m_status);
    auto* tabs = new QTabWidget(this);
    tabs->setObjectName(QStringLiteral("sbomTabs"));
    auto* componentPage = new QWidget(tabs);
    auto* componentLayout = new QVBoxLayout(componentPage);
    componentLayout->setContentsMargins(6, 6, 6, 6);
    componentLayout->setSpacing(4);
    m_summary = new QTextBrowser(componentPage);
    m_summary->setObjectName(QStringLiteral("sbomSummary"));
    m_summary->setOpenLinks(false);
    m_summary->setOpenExternalLinks(false);
    m_summary->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    m_summary->setMinimumHeight(54);
    m_summary->setMaximumHeight(100);
    m_summary->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
    m_summary->setPlainText(QStringLiteral("尚无成功解析的 SBOM。"));
    componentLayout->addWidget(m_summary, 1);
    auto* table = new QTableView(componentPage);
    table->setObjectName(QStringLiteral("sbomComponents"));
    table->setModel(m_model);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setAlternatingRowColors(true);
    table->setWordWrap(false);
    table->setShowGrid(false);
    table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    table->horizontalHeader()->setStretchLastSection(true);
    const QList<int> previewWidths{180, 120, 110, 260, 200};
    for (int column = 0; column < previewWidths.size(); ++column)
        table->setColumnWidth(column, previewWidths[column]);
    componentLayout->addWidget(table, 4);
    tabs->addTab(componentPage, QStringLiteral("组件预览"));
    auto* qualityPage = new QWidget(tabs);
    auto* qualityLayout = new QVBoxLayout(qualityPage);
    qualityLayout->setContentsMargins(6, 6, 6, 6);
    qualityLayout->setSpacing(4);
    m_qualitySummary = new QLabel(QStringLiteral("尚无质量诊断结果。"), qualityPage);
    m_qualitySummary->setObjectName(QStringLiteral("sbomQualitySummary"));
    m_qualitySummary->setTextFormat(Qt::PlainText);
    m_qualitySummary->setWordWrap(true);
    qualityLayout->addWidget(m_qualitySummary);
    auto* issues = new QTableView(qualityPage);
    issues->setObjectName(QStringLiteral("sbomQualityIssues"));
    issues->setModel(m_qualityModel);
    issues->setEditTriggers(QAbstractItemView::NoEditTriggers);
    issues->setSelectionBehavior(QAbstractItemView::SelectRows);
    issues->setAlternatingRowColors(true);
    issues->setWordWrap(false);
    issues->setShowGrid(false);
    issues->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    issues->horizontalHeader()->setStretchLastSection(true);
    // The wrapping summary drives tab height-for-width. The scrollable table
    // must use available height instead of imposing its preferred row area.
    issues->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
    issues->setColumnWidth(0, 150);
    issues->setColumnWidth(1, 230);
    issues->setColumnWidth(2, 230);
    issues->setColumnWidth(3, 720);
    qualityLayout->addWidget(issues, 1);
    tabs->addTab(qualityPage, QStringLiteral("质量诊断"));
    layout->addWidget(tabs, 1);
    auto* applyHint = new QLabel(QStringLiteral("预览不写库；应用将完整替换组件（含根组件）与原始依赖。"), this);
    applyHint->setObjectName(QStringLiteral("sbomApplyHint"));
    applyHint->setWordWrap(true);
    layout->addWidget(applyHint);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    m_close = buttons->button(QDialogButtonBox::Close);
    m_close->setObjectName(QStringLiteral("closeSbom"));
    m_close->setText(QStringLiteral("关闭"));
    m_apply = buttons->addButton(QStringLiteral("应用到项目"), QDialogButtonBox::ActionRole);
    m_apply->setObjectName(QStringLiteral("applySbom"));
    m_apply->setAutoDefault(false);
    layout->addWidget(buttons);
    setStyleSheet(QStringLiteral(
        "QDialog#sbomImportDialog { background: #f3f6fa; }"
        "QLabel#sbomProject { color: #243b53; font-weight: 600; }"
        "QLabel#sbomFormatHint, QLabel#sbomApplyHint { color: #52647b; }"
        "QLabel#sbomStatus { color: #244765; }"
        "QTextBrowser#sbomSummary { background: #edf2f8; color: #334e68; border: 1px solid #d6e1ed; border-radius: 4px; }"
        "QLabel#sbomQualitySummary { color: #334e68; }"
        "QTabWidget#sbomTabs::pane { background: white; border: 1px solid #d6e1ed; }"
        "QTabWidget#sbomTabs QTabBar::tab { padding: 6px 16px; color: #52647b; background: #e8eef6; }"
        "QTabWidget#sbomTabs QTabBar::tab:selected { background: white; color: #174f87; border-bottom: 2px solid #3976b6; }"
        "QTabWidget#sbomTabs QTabBar::tab:focus { outline: 1px solid #3976b6; }"
        "QTableView#sbomComponents, QTableView#sbomQualityIssues { background: white; alternate-background-color: #f5f8fc;"
        " color: #243b53; border: 1px solid #d6e1ed; selection-background-color: #d6e8fb; selection-color: #173d69; }"
        "QTableView#sbomComponents:focus, QTableView#sbomQualityIssues:focus { border-color: #3976b6; }"
        "QTableView#sbomComponents QHeaderView::section, QTableView#sbomQualityIssues QHeaderView::section"
        " { background: #edf2f8; color: #334e68; padding: 4px; border: none; border-right: 1px solid #d6e1ed; border-bottom: 1px solid #d6e1ed; }"
        "QPushButton#chooseSbomFile, QPushButton#applySbom, QPushButton#closeSbom"
        " { color: #244765; background: white; padding: 5px 12px; border: 1px solid #b6c8dc; border-radius: 5px; }"
        "QPushButton#applySbom { background: #255faa; border-color: #255faa; color: white; }"
        "QPushButton#chooseSbomFile:hover, QPushButton#closeSbom:hover { background: #e8eff7; }"
        "QPushButton#applySbom:hover { background: #1c4f91; }"
        "QPushButton#chooseSbomFile:focus, QPushButton#applySbom:focus, QPushButton#closeSbom:focus { border: 2px solid #619bd4; padding: 4px 11px; }"
        "QPushButton#chooseSbomFile:disabled, QPushButton#applySbom:disabled, QPushButton#closeSbom:disabled"
        " { color: #697b8e; background: #e8edf3; border-color: #d6e1ed; }"));
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_choose, &QPushButton::clicked, this, &SbomImportDialog::chooseFile);
    connect(m_apply, &QPushButton::clicked, this, &SbomImportDialog::applyToProject);
    connect(&m_applyWatcher, &QFutureWatcher<ComponentResult>::finished, this, [this] {
        const auto result = m_applyWatcher.result();
        m_applyState = result.ok() ? ApplyState::Applied : ApplyState::Ready;
        m_status->setStyleSheet(result.ok() ? QString() : QStringLiteral("color: #a12622;"));
        m_status->setText(result.ok()
            ? QStringLiteral("已应用：项目当前组件及依赖已完整替换。质量诊断仍有效，保存不代表数据没有问题。")
            : result.userMessage());
        // Driver diagnostics may contain input; log only a fixed error category.
        m_logger.write(result.ok() ? AppLogger::Level::Info : AppLogger::Level::Warning,
            QStringLiteral("Component apply finished, result=%1").arg(static_cast<int>(result.error)));
        updateActions();
        emit applyFinished(result.ok());
    });
    updateActions();
    connect(&m_watcher, &QFutureWatcher<ImportResult>::finished, this, [this] {
        auto result = m_watcher.result();
        auto& parsed = result.parsed;
        m_importing = false;
        if (!parsed.ok()) {
            m_logger.write(AppLogger::Level::Warning, QStringLiteral("SBOM parse failed: %1").arg(parsed.errorCode()));
            m_status->setText(parsed.userMessage() + QStringLiteral(" 当前预览与质量诊断未更改。"));
            m_status->setStyleSheet(QStringLiteral("color: #a12622;"));
        } else {
            const auto& document = parsed.document;
            m_logger.write(AppLogger::Level::Info,
                QStringLiteral("CycloneDX SBOM parsed successfully, components=%1, dependencies=%2")
                    .arg(document.components.size()).arg(document.dependencies.size()));
            const QString absent = QStringLiteral("未提供");
            const QString root = document.metadata.component ? previewText(document.metadata.component->name) : absent;
            const QString serial = document.serialNumber.isEmpty() ? absent : previewText(document.serialNumber);
            m_summary->setPlainText(QStringLiteral("文件：%1\nCycloneDX：%2　BOM version：%3\n"
                "serialNumber：%4\n组件数量：%5　依赖条目数量：%6\n元数据根组件：%7\n时间戳：%8")
                .arg(m_pendingFileName, document.specVersion,
                     document.bomVersion ? QString::number(*document.bomVersion) : absent, serial,
                     QString::number(document.components.size()), QString::number(document.dependencies.size()),
                     root, document.metadata.timestamp.isEmpty() ? absent : previewText(document.metadata.timestamp)));
            const auto errors = result.quality.count(QualitySeverity::Error);
            const auto warnings = result.quality.count(QualitySeverity::Warning);
            const auto infos = result.quality.count(QualitySeverity::Info);
            const auto total = result.quality.issues().size();
            m_qualitySummary->setText(QStringLiteral("总问题：%1　Error：%2　Warning：%3　Info：%4\n%5")
                .arg(total).arg(errors).arg(warnings).arg(infos)
                .arg(total == 0 ? QStringLiteral("当前诊断规则未发现问题。")
                               : QStringLiteral("质量问题不等于解析失败或漏洞风险；位置按输入顺序编号，可横向滚动或悬停查看说明。")));
            m_logger.write(AppLogger::Level::Info,
                QStringLiteral("SBOM quality analysis completed, errorCount=%1, warningCount=%2, infoCount=%3")
                    .arg(errors).arg(warnings).arg(infos));
            m_model->replace(std::move(parsed.document));
            m_qualityModel->replace(std::move(result.quality));
            m_applyState = ApplyState::Ready;
            m_status->setStyleSheet(QString());
            m_status->setText(QStringLiteral("解析成功 · 尚未应用（%1 项问题，Error：%2）。%3")
                .arg(total).arg(errors).arg(errors > 0
                    ? QStringLiteral("存在质量 Error；仍可应用，保存不代表数据没有问题。") : QString()));
        }
        m_pendingFileName.clear();
        updateActions();
        emit importFinished(parsed.ok());
    });
}

void SbomImportDialog::chooseFile()
{
    if (m_importing || m_picker || m_applyState == ApplyState::Working) return;
    auto* picker = new QFileDialog(this, QStringLiteral("选择 CycloneDX JSON"));
    m_picker = picker;
    picker->setAttribute(Qt::WA_DeleteOnClose);
    picker->setFileMode(QFileDialog::ExistingFile);
    picker->setNameFilter(QStringLiteral("CycloneDX / JSON (*.json)"));
    connect(picker, &QFileDialog::fileSelected, this, &SbomImportDialog::importFile);
    picker->open();
}

void SbomImportDialog::importFile(const QString& path)
{
    if (m_importing || path.isEmpty() || m_applyState == ApplyState::Working) return;
    m_importing = true;
    m_pendingFileName = QFileInfo(path).fileName();
    updateActions();
    m_status->setStyleSheet(QString());
    m_status->setText(QStringLiteral("正在读取、解析并诊断质量…"));
    // Value capture only: closing the window during parsing cannot access destroyed UI/services.
    m_watcher.setFuture(QtConcurrent::run([path] {
        ImportResult result;
        result.parsed = CycloneDxParser::parseFile(path);
        if (result.parsed.ok()) result.quality = SbomQualityAnalyzer::analyze(result.parsed.document);
        return result;
    }));
}

void SbomImportDialog::updateActions()
{
    const bool working = m_applyState == ApplyState::Working;
    m_choose->setEnabled(!m_importing && !working);
    m_apply->setEnabled(!m_importing && m_applyState == ApplyState::Ready);
    m_apply->setText(working ? QStringLiteral("正在应用…")
        : m_applyState == ApplyState::Applied ? QStringLiteral("已应用") : QStringLiteral("应用到项目"));
    m_close->setEnabled(!working);
}

void SbomImportDialog::applyToProject()
{
    if (m_importing || m_picker || m_applyState != ApplyState::Ready) return;
    m_applyState = ApplyState::Working;
    updateActions();
    m_status->setStyleSheet(QString());
    m_status->setText(QStringLiteral("正在应用当前 SBOM，请等待事务完成。组件及依赖仅在完整保存成功后一起替换。"));
    m_applyWatcher.setFuture(QtConcurrent::run([path = m_databaseFile, id = m_projectId,
                                               document = m_model->document()] {
        return ComponentRepository::replaceInFile(path, id, document);
    }));
}

void SbomImportDialog::reject()
{
    if (m_applyState != ApplyState::Working) QDialog::reject();
}

void SbomImportDialog::closeEvent(QCloseEvent* event)
{
    if (m_applyState == ApplyState::Working) event->ignore();
    else QDialog::closeEvent(event);
}
