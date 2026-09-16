#include "ProjectPage.h"
#include "ProjectRepository.h"
#include "SbomImportDialog.h"

#include <QDateTime>
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

ProjectPage::ProjectPage(ProjectRepository& repository, AppLogger& logger, QWidget* parent)
    : QWidget(parent), m_repository(repository), m_logger(logger)
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
    layout->addWidget(m_details, 2);

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
    m_details->hide();
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
    m_details->show();
    m_remove->setEnabled(true);
    m_import->setEnabled(true);
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
    auto* dialog = new SbomImportDialog(project.name, m_logger, this);
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
