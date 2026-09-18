#include "ProjectPage.h"
#include "ProjectRepository.h"
#include "ComponentRepository.h"
#include "SbomImportDialog.h"
#include "DependencyPage.h"

#include <QDateTime>
#include <QAbstractTableModel>
#include <QHeaderView>
#include <QTableView>
#include <QTabWidget>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTextBrowser>
#include <QTimeZone>
#include <QVBoxLayout>

namespace
{
QLabel* textLabel(const QString& objectName, QWidget* parent)
{
    auto* label = new QLabel(parent);
    label->setObjectName(objectName);
    label->setTextFormat(Qt::PlainText);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return label;
}
}

// A read-only snapshot obtained from the repository on selection/refresh/Apply.
// SQLite remains authoritative; visible cells are formatted lazily.
class ProjectComponentsModel final : public QAbstractTableModel
{
public:
    explicit ProjectComponentsModel(QObject* parent) : QAbstractTableModel(parent) {}
    int rowCount(const QModelIndex& parent = {}) const override { return parent.isValid() ? 0 : int(m_rows.size()); }
    int columnCount(const QModelIndex& parent = {}) const override { return parent.isValid() ? 0 : 6; }
    QVariant data(const QModelIndex& index, int role) const override
    {
        if (!index.isValid() || index.row() < 0 || index.row() >= rowCount() || role != Qt::DisplayRole) return {};
        const auto& c = m_rows.at(index.row());
        if (index.column() == 0) return c.sourceRole == ComponentSourceRole::MetadataRoot
            ? QStringLiteral("元数据根组件") : QStringLiteral("组件 #%1").arg(c.sourceOrder + 1);
        const QString* text = nullptr;
        switch (index.column()) {
        case 1: text = &c.name; break;
        case 2: text = &c.version; break;
        case 3: text = &c.type; break;
        case 4: text = &c.purl; break;
        case 5: text = &c.bomRef; break;
        default: return {};
        }
        return text->size() > 512 ? text->left(512) + QStringLiteral("…") : *text;
    }
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
            return QAbstractTableModel::headerData(section, orientation, role);
        return QStringList{QStringLiteral("来源"), QStringLiteral("名称 / Name"), QStringLiteral("版本 / Version"),
            QStringLiteral("类型 / Type"), QStringLiteral("PURL"), QStringLiteral("bom-ref")}.value(section);
    }
    void replace(QList<Component> rows) { beginResetModel(); m_rows = std::move(rows); endResetModel(); }
private:
    QList<Component> m_rows;
};

ProjectPage::ProjectPage(ProjectRepository& repository, ComponentRepository& components, AppLogger& logger, QWidget* parent)
    : QWidget(parent), m_repository(repository), m_components(components), m_logger(logger)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(32, 24, 32, 24);
    layout->setSpacing(12);
    auto* title = new QLabel(QStringLiteral("项目"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    layout->addWidget(title);
    auto* actions = new QHBoxLayout;
    auto* create = new QPushButton(QStringLiteral("新建项目"), this);
    create->setObjectName(QStringLiteral("createProject"));
    m_remove = new QPushButton(QStringLiteral("删除项目"), this);
    m_remove->setObjectName(QStringLiteral("removeProject"));
    auto* refresh = new QPushButton(QStringLiteral("刷新"), this);
    refresh->setObjectName(QStringLiteral("refreshProjects"));
    actions->addWidget(create);
    actions->addWidget(m_remove);
    m_import = new QPushButton(QStringLiteral("导入 SBOM"), this);
    m_import->setObjectName(QStringLiteral("importSbom"));
    actions->addWidget(m_import);
    actions->addStretch();
    actions->addWidget(refresh);
    layout->addLayout(actions);

    m_error = textLabel(QStringLiteral("projectError"), this);
    m_error->setStyleSheet(QStringLiteral("color: #a12622;"));
    layout->addWidget(m_error);
    m_empty = textLabel(QStringLiteral("projectsEmpty"), this);
    m_empty->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    m_empty->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_empty->setText(QStringLiteral("当前还没有项目\n点击“新建项目”，创建第一个分析项目。"));
    layout->addWidget(m_empty);
    m_list = new QListWidget(this);
    m_list->setObjectName(QStringLiteral("projectList"));
    m_list->setMinimumHeight(80);
    m_list->setMaximumHeight(180);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_list, 1);
    m_selectionHint = textLabel(QStringLiteral("projectSelectionHint"), this);
    m_selectionHint->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    m_selectionHint->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_selectionHint->setText(QStringLiteral("请选择一个项目查看详情。"));
    layout->addWidget(m_selectionHint);

    m_details = new QTextBrowser(this);
    m_details->setObjectName(QStringLiteral("projectDetails"));
    m_details->setFrameShape(QFrame::NoFrame);
    m_details->setOpenLinks(false);
    m_details->setOpenExternalLinks(false);
    m_details->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    m_details->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    m_tabs = new QTabWidget(this);
    m_tabs->setObjectName(QStringLiteral("projectTabs"));
    m_tabs->addTab(m_details, QStringLiteral("项目详情"));
    auto* componentPage = new QWidget(m_tabs);
    auto* componentLayout = new QVBoxLayout(componentPage);
    m_componentSummary = textLabel(QStringLiteral("currentComponentsSummary"), componentPage);
    componentLayout->addWidget(m_componentSummary);
    auto* table = new QTableView(componentPage);
    table->setObjectName(QStringLiteral("currentComponents"));
    m_componentModel = new ProjectComponentsModel(table);
    table->setModel(m_componentModel);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setAlternatingRowColors(true);
    table->setWordWrap(false);
    table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
    table->horizontalHeader()->setDefaultSectionSize(150);
    table->setColumnWidth(4, 280);
    componentLayout->addWidget(table, 1);
    m_tabs->addTab(componentPage, QStringLiteral("当前组件"));
    m_dependencies = new DependencyPage(m_components.databaseFilePath(),m_tabs);
    m_tabs->addTab(m_dependencies,QStringLiteral("依赖关系"));
    layout->addWidget(m_tabs, 2);

    connect(create, &QPushButton::clicked, this, &ProjectPage::showCreateDialog);
    connect(m_remove, &QPushButton::clicked, this, &ProjectPage::confirmRemoval);
    connect(m_import, &QPushButton::clicked, this, &ProjectPage::showSbomImport);
    connect(refresh, &QPushButton::clicked, this, [this] { reload(selectedId()); });
    connect(m_list, &QListWidget::currentItemChanged, this, [this] { showSelection(); });
    reload();
}

QString ProjectPage::selectedId() const
{
    const auto* item = m_list->currentItem();
    return item ? item->data(Qt::UserRole).toString() : QString();
}

void ProjectPage::reload(const QString& preferredId)
{
    QList<Project> projects;
    const ProjectResult result = m_repository.list(projects);
    {
        const QSignalBlocker blocker(m_list);
        m_list->clear();
        for (const auto& project : projects) {
            auto* item = new QListWidgetItem(project.name, m_list);
            item->setData(Qt::UserRole, project.id);
            item->setToolTip(project.name);
            if (project.id == preferredId) {
                m_list->setCurrentItem(item);
            }
        }
    }
    m_empty->setVisible(result.ok() && projects.isEmpty());
    m_list->setVisible(!projects.isEmpty());
    showSelection();
    if (!result.ok()) {
        m_error->setText(result.userMessage());
        m_error->show();
    }
}

void ProjectPage::showSelection()
{
    m_details->clear();
    m_tabs->hide();
    m_dependencies->setProject({});
    m_componentModel->replace({});
    m_remove->setEnabled(false);
    m_import->setEnabled(false);
    m_selectionHint->setVisible(m_list->count() > 0);
    m_error->hide();
    const QString id = selectedId();
    if (id.isEmpty()) {
        return;
    }
    Project project;
    const auto result = m_repository.findById(id, project);
    if (!result.ok()) {
        m_error->setText(result.userMessage());
        m_error->show();
        return;
    }
    const QString createdAt = QDateTime::fromMSecsSinceEpoch(project.createdAt, QTimeZone::UTC)
                                  .toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    const QString description = project.description.isEmpty() ? QStringLiteral("未填写描述") : project.description;
    // Only the fixed headings are markup; project input must always display as literal text.
    m_details->setHtml(QStringLiteral("<p><b>项目名称</b><br>%1</p>"
                                     "<p><b>创建时间（本地）</b><br>%2</p>"
                                     "<p><b>项目描述</b></p><div style='white-space: pre-wrap;'>%3</div>")
                           .arg(project.name.toHtmlEscaped(), createdAt, description.toHtmlEscaped()));
    m_selectionHint->hide();
    m_tabs->show();
    m_dependencies->setProject(id);
    m_remove->setEnabled(true);
    m_import->setEnabled(true);
    QList<Component> components;
    const auto componentResult = m_components.listForProject(id, components);
    if (!componentResult.ok()) {
        m_error->setText(componentResult.userMessage());
        m_error->show();
    }
    m_componentSummary->setText(!componentResult.ok() ? QStringLiteral("当前组件读取失败。")
        : components.isEmpty() ? QStringLiteral("当前项目没有已保存组件。导入预览后点击“应用到项目”保存。")
        : QStringLiteral("当前已保存 %1 条组件（包含元数据根组件）。来源序号为展开顺序；超长文本省略显示。")
            .arg(components.size()));
    m_componentModel->replace(std::move(components));
}

void ProjectPage::showSbomImport()
{
    const QString id = selectedId();
    if (id.isEmpty()) return;
    Project project;
    const auto result = m_repository.findById(id, project);
    if (!result.ok()) {
        reload();
        m_error->setText(result.userMessage());
        m_error->show();
        return;
    }
    // Window modality keeps this project's context fixed until its preview is closed.
    auto* dialog = new SbomImportDialog(project.id, project.name, m_components.databaseFilePath(), m_logger, this);
    connect(dialog, &SbomImportDialog::applyFinished, this, [this](bool success) {
        if (success) showSelection();
    });
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->open();
    dialog->chooseFile();
}

void ProjectPage::showCreateDialog()
{
    auto* dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setObjectName(QStringLiteral("createProjectDialog"));
    dialog->setWindowTitle(QStringLiteral("新建项目"));
    dialog->resize(460, 360);
    auto* layout = new QVBoxLayout(dialog);
    auto* form = new QFormLayout;
    form->setRowWrapPolicy(QFormLayout::WrapAllRows);
    auto* name = new QLineEdit(dialog);
    name->setObjectName(QStringLiteral("newProjectName"));
    name->setPlaceholderText(QStringLiteral("必填，1—100 个字符"));
    auto* description = new QPlainTextEdit(dialog);
    description->setObjectName(QStringLiteral("newProjectDescription"));
    description->setPlaceholderText(QStringLiteral("可选，最多 500 个字符"));
    form->addRow(QStringLiteral("项目名称"), name);
    form->addRow(QStringLiteral("项目描述"), description);
    layout->addLayout(form);
    auto* error = textLabel(QStringLiteral("createProjectError"), dialog);
    error->setStyleSheet(QStringLiteral("color: #a12622;"));
    layout->addWidget(error);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("创建"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, dialog, [this, dialog, name, description, error] {
        Project project;
        const auto result = m_repository.create(name->text(), description->toPlainText(), project);
        if (!result.ok()) {
            error->setText(result.userMessage());
            return;
        }
        dialog->accept();
        reload(project.id);
    });
    dialog->open();
    name->setFocus();
}

void ProjectPage::confirmRemoval()
{
    const QString id = selectedId();
    if (id.isEmpty()) {
        return;
    }
    Project project;
    const auto result = m_repository.findById(id, project);
    if (!result.ok()) {
        reload();
        m_error->setText(result.userMessage());
        m_error->show();
        return;
    }
    auto* confirmation = new QMessageBox(QMessageBox::Question, QStringLiteral("删除项目"),
        QStringLiteral("确定删除项目“%1”吗？删除后无法恢复。").arg(project.name),
        QMessageBox::Yes | QMessageBox::Cancel, this);
    confirmation->setObjectName(QStringLiteral("removeProjectDialog"));
    confirmation->setAttribute(Qt::WA_DeleteOnClose);
    confirmation->setTextFormat(Qt::PlainText);
    confirmation->button(QMessageBox::Yes)->setText(QStringLiteral("删除"));
    confirmation->button(QMessageBox::Cancel)->setText(QStringLiteral("取消"));
    confirmation->setDefaultButton(QMessageBox::Cancel);
    confirmation->setEscapeButton(QMessageBox::Cancel);
    connect(confirmation, &QDialog::finished, this, [this, id](int answer) {
        if (answer != QMessageBox::Yes) {
            return;
        }
        const auto result = m_repository.remove(id);
        if (result.ok() || result.error == ProjectError::NotFound) {
            reload();
        }
        if (!result.ok()) {
            m_error->setText(result.userMessage());
            m_error->show();
        }
    });
    confirmation->open();
}
