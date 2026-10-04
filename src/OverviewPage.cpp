#include "OverviewPage.h"
#include "ProjectPage.h"
#include "ProjectRepository.h"
#include "ComponentRepository.h"
#include "PackageIdentity.h"
#include <QDateTime>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

OverviewPage::OverviewPage(ProjectRepository& projects, ComponentRepository& components, QWidget* parent)
    : QWidget(parent), m_projects(projects), m_components(components)
{
    auto* outer = new QVBoxLayout(this);
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    outer->addWidget(scroll);
    auto* content = new QWidget;
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);
    const auto label = [&](const char* name, const QString& text) {
        auto* item = new QLabel(text, content);
        item->setObjectName(name);
        item->setTextFormat(Qt::PlainText);
        item->setWordWrap(true);
        item->setTextInteractionFlags(Qt::TextSelectableByMouse);
        layout->addWidget(item);
        return item;
    };
    label("pageTitle", QStringLiteral("项目概览"));
    m_project = label("overviewProject", {});
    m_supplyChain = label("overviewSupplyChain", {});
    label("overviewCapabilities", QStringLiteral(
        "当前分析能力\n漏洞数据源：OSV\n支持生态：%1\n分析模式：选定组件按需分析\n"
        "版本适用性：本地判断\n利用信号：FIRST EPSS / CISA KEV\n\n当前系统不提供项目级批量漏洞统计。")
        .arg(PackageIdentity::supportedEcosystems().join(" / ")));
    label("overviewMethod", QStringLiteral("方法状态\nPhase 11 验证实验：已完成\n详细范围与限制见“方法验证”。"));
    const auto button = [&](const char* name, const QString& text, const QString& destination) {
        auto* action = new QPushButton(text, content);
        action->setObjectName(name);
        layout->addWidget(action);
        connect(action, &QPushButton::clicked, this, [this, destination] { emit navigationRequested(destination); });
    };
    button("overviewManage", QStringLiteral("管理项目"), "projects");
    button("overviewAnalyze", QStringLiteral("进入项目分析"), "projects");
    button("overviewValidation", QStringLiteral("查看方法验证"), "validation");
    layout->addStretch();
    scroll->setWidget(content);
    refresh({});
}

void OverviewPage::follow(ProjectPage& page)
{
    connect(&page, &ProjectPage::currentProjectChanged, this, &OverviewPage::refresh, Qt::UniqueConnection);
    connect(&page, &ProjectPage::currentProjectStateChanged, this, &OverviewPage::refresh, Qt::UniqueConnection);
    // Initial reload may have occurred before the connections existed.
    refresh(page.currentProjectId());
}

void OverviewPage::refresh(const QString& id)
{
    m_supplyChain->clear();
    if (id.isEmpty()) {
        m_project->setText(QStringLiteral("当前未选择项目\n请进入“项目”页面选择或创建项目。"));
        return;
    }
    Project project;
    if (!m_projects.findById(id, project).ok()) {
        m_project->setText(QStringLiteral("当前项目读取失败，请进入“项目”页面刷新或重新选择。"));
        m_supplyChain->setText(QStringLiteral("供应链状态：读取失败"));
        return;
    }
    m_project->setText(QStringLiteral("当前项目：%1\n项目描述：%2\n创建时间（本地）：%3")
        .arg(project.name, project.description.isEmpty() ? QStringLiteral("未填写描述") : project.description,
             QDateTime::fromMSecsSinceEpoch(project.createdAt).toLocalTime().toString("yyyy-MM-dd HH:mm:ss")));
    ProjectOverviewSummary summary;
    if (!m_components.readOverviewSummary(id, summary).ok()) {
        m_supplyChain->setText(QStringLiteral("供应链状态：读取失败"));
        return;
    }
    m_supplyChain->setText(QStringLiteral("当前供应链状态\n普通组件数：%1\n依赖信息：%2")
        .arg(summary.ordinaryComponentCount)
        .arg(summary.dependencyCaptured ? QStringLiteral("已捕获") : QStringLiteral("未捕获")));
}
