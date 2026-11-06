#include "OverviewPage.h"
#include "ProjectPage.h"
#include "ProjectRepository.h"
#include "ComponentRepository.h"
#include "PackageIdentity.h"
#include <QDateTime>
#include <QBoxLayout>
#include <QFont>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QStyle>
#include <QVBoxLayout>

namespace {
QLabel* textLabel(const char* name, const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setObjectName(name);
    label->setTextFormat(Qt::PlainText);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    // Long unbroken project names must wrap rather than widen the scroll content.
    QSizePolicy policy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    label->setSizePolicy(policy);
    return label;
}

void setError(QLabel* label, bool error)
{
    label->setProperty("overviewError", error);
    label->style()->unpolish(label);
    label->style()->polish(label);
}
}

OverviewPage::OverviewPage(ProjectRepository& projects, ComponentRepository& components, QWidget* parent)
    : QWidget(parent), m_projects(projects), m_components(components)
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    auto* scroll = new QScrollArea(this);
    scroll->setObjectName(QStringLiteral("overviewScroll"));
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    outer->addWidget(scroll);
    auto* content = new QWidget;
    content->setObjectName(QStringLiteral("overviewContent"));
    auto* layout = new QVBoxLayout(content);
    layout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);
    auto* title = textLabel("pageTitle", QStringLiteral("项目概览"), content);
    auto titleFont = title->font();
    titleFont.setPointSizeF(titleFont.pointSizeF() + 7);
    titleFont.setWeight(QFont::DemiBold);
    title->setFont(titleFont);
    layout->addWidget(title);
    auto* subtitle = textLabel("overviewSubtitle", QStringLiteral("从当前项目出发，查看供应链信息并进入按需分析。"), content);
    subtitle->setProperty("overviewMuted", true);
    layout->addWidget(subtitle);

    const auto card = [&](const char* name, const QString& heading) {
        auto* frame = new QFrame(content);
        frame->setObjectName(name);
        frame->setProperty("overviewCard", true);
        auto* box = new QVBoxLayout(frame);
        box->setContentsMargins(18, 16, 18, 16);
        box->setSpacing(10);
        auto* caption = textLabel("overviewCardTitle", heading, frame);
        auto captionFont = caption->font();
        captionFont.setWeight(QFont::DemiBold);
        caption->setFont(captionFont);
        box->addWidget(caption);
        return frame;
    };

    m_projectCard = card("overviewProjectCard", QStringLiteral("当前项目"));
    m_projectState = textLabel("overviewProjectState", {}, m_projectCard);
    m_projectState->setProperty("overviewMuted", true);
    m_projectCard->layout()->addWidget(m_projectState);
    m_project = textLabel("overviewProject", {}, m_projectCard);
    m_projectCard->layout()->addWidget(m_project);
    m_supplyCard = card("overviewSupplyCard", QStringLiteral("供应链状态"));
    m_supplyChain = textLabel("overviewSupplyChain", {}, m_supplyCard);
    m_supplyCard->layout()->addWidget(m_supplyChain);
    m_supplyHint = textLabel("overviewSupplyHint", {}, m_supplyCard);
    m_supplyHint->setProperty("overviewMuted", true);
    m_supplyCard->layout()->addWidget(m_supplyHint);
    m_summaryLayout = new QGridLayout;
    m_summaryLayout->setSpacing(16);
    m_summaryLayout->setColumnStretch(0, 1);
    m_summaryLayout->addWidget(m_projectCard, 0, 0);
    m_summaryLayout->addWidget(m_supplyCard, 1, 0);
    layout->addLayout(m_summaryLayout);

    auto* actions = card("overviewActionsCard", QStringLiteral("快捷操作"));
    m_actionsLayout = new QBoxLayout(QBoxLayout::TopToBottom);
    m_actionsLayout->setSpacing(8);
    static_cast<QVBoxLayout*>(actions->layout())->addLayout(m_actionsLayout);
    const auto button = [&](const char* name, const QString& text, const QString& destination) {
        auto* action = new QPushButton(text, actions);
        action->setObjectName(name);
        action->setMinimumHeight(36);
        m_actionsLayout->addWidget(action);
        connect(action, &QPushButton::clicked, this, [this, destination] { emit navigationRequested(destination); });
    };
    button("overviewManage", QStringLiteral("管理项目"), "projects");
    button("overviewAnalyze", QStringLiteral("进入项目分析"), "projects");
    button("overviewValidation", QStringLiteral("查看方法验证"), "validation");
    layout->addWidget(actions);

    auto* capabilities = card("overviewCapabilitiesCard", QStringLiteral("分析能力与范围"));
    auto* capabilityText = textLabel("overviewCapabilities", QStringLiteral(
        "漏洞数据源：OSV  ·  支持生态：%1\n"
        "分析方式：选定组件按需查询，本地判断版本适用性\n"
        "利用信号：FIRST EPSS / CISA KEV  ·  优先级规则：Rules v1\n\n"
        "以上为系统能力说明，不表示当前项目已完成分析。当前系统不提供项目级批量漏洞统计或项目级风险评分。")
        .arg(PackageIdentity::supportedEcosystems().join(" / ")), capabilities);
    capabilities->layout()->addWidget(capabilityText);
    auto* method = textLabel("overviewMethod", QStringLiteral("方法验证：已完成冻结样本实验。适用范围与限制请查看“方法验证”。"), capabilities);
    method->setProperty("overviewMuted", true);
    capabilities->layout()->addWidget(method);
    layout->addWidget(capabilities);
    layout->addStretch();
    scroll->setWidget(content);
    setStyleSheet(QStringLiteral(
        "QWidget#overviewContent, QScrollArea#overviewScroll { background: #f3f6fa; }"
        "QWidget#overviewContent QLabel { color: #20344e; background: transparent; }"
        "QFrame[overviewCard=\"true\"] { background: #ffffff; border: 1px solid #dce4ee; border-radius: 9px; }"
        "QWidget#overviewContent QLabel[overviewMuted=\"true\"] { color: #586a80; }"
        "QWidget#overviewContent QLabel[overviewError=\"true\"] { color: #a52c35; }"
        "QFrame#overviewActionsCard QPushButton { color: #264665; background: #ffffff;"
        " border: 2px solid #d6e1ed; border-radius: 6px; padding: 6px 12px; }"
        "QFrame#overviewActionsCard QPushButton:hover { background: #edf4fc; border-color: #a4bddb; }"
        "QFrame#overviewActionsCard QPushButton:pressed { background: #dbe9f8; }"
        "QFrame#overviewActionsCard QPushButton#overviewAnalyze { background: #255faa; border-color: #255faa; color: #ffffff; }"
        "QFrame#overviewActionsCard QPushButton#overviewAnalyze:hover { background: #1c4f91; border-color: #1c4f91; }"
        "QFrame#overviewActionsCard QPushButton#overviewAnalyze:pressed { background: #163e73; }"
        "QFrame#overviewActionsCard QPushButton:focus { border-color: #173d69; }"
        "QFrame#overviewActionsCard QPushButton#overviewAnalyze:focus { border-color: #9dccff; }"
        "QFrame#overviewActionsCard QPushButton:disabled { background: #edf1f5; border-color: #dce3ec; color: #6c7b8e; }"));
    refresh({});
}

void OverviewPage::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    const bool wide = event->size().width() >= 700;
    if (wide == m_wideLayout) return;
    m_wideLayout = wide;
    // Move only layout items; widgets, connections and project ownership stay intact.
    m_summaryLayout->removeWidget(m_projectCard);
    m_summaryLayout->removeWidget(m_supplyCard);
    m_summaryLayout->addWidget(m_projectCard, 0, 0);
    m_summaryLayout->addWidget(m_supplyCard, wide ? 0 : 1, wide ? 1 : 0);
    m_summaryLayout->setColumnStretch(1, wide ? 1 : 0);
    m_actionsLayout->setDirection(wide ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom);
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
    setError(m_project, false);
    setError(m_supplyChain, false);
    setError(m_projectState, false);
    m_supplyHint->setText(QStringLiteral("选择项目后显示已应用的组件数量与依赖捕获情况。"));
    if (id.isEmpty()) {
        m_projectState->setText(QStringLiteral("尚未选择"));
        m_project->setText(QStringLiteral("当前未选择项目\n请进入“项目”页面选择或创建项目。"));
        return;
    }
    Project project;
    if (!m_projects.findById(id, project).ok()) {
        m_projectState->setText(QStringLiteral("读取失败"));
        setError(m_projectState, true);
        setError(m_project, true);
        setError(m_supplyChain, true);
        m_supplyHint->setText(QStringLiteral("当前数据不可用，请重新选择项目或进入项目页刷新。"));
        m_project->setText(QStringLiteral("当前项目读取失败，请进入“项目”页面刷新或重新选择。"));
        m_supplyChain->setText(QStringLiteral("供应链状态：读取失败"));
        return;
    }
    m_projectState->setText(QStringLiteral("已选择 · 当前分析上下文"));
    m_project->setText(QStringLiteral("当前项目：%1\n项目描述：%2\n创建时间（本地）：%3")
        .arg(project.name, project.description.isEmpty() ? QStringLiteral("未填写描述") : project.description,
             QDateTime::fromMSecsSinceEpoch(project.createdAt).toLocalTime().toString("yyyy-MM-dd HH:mm:ss")));
    ProjectOverviewSummary summary;
    if (!m_components.readOverviewSummary(id, summary).ok()) {
        setError(m_supplyChain, true);
        m_supplyHint->setText(QStringLiteral("无法读取摘要；此处不以零值代替读取失败。"));
        m_supplyChain->setText(QStringLiteral("供应链状态：读取失败"));
        return;
    }
    m_supplyHint->setText(QStringLiteral("数量不含元数据根组件；依赖已捕获不表示依赖信息完整。"));
    m_supplyChain->setText(QStringLiteral("当前供应链状态\n普通组件数：%1\n依赖信息：%2")
        .arg(summary.ordinaryComponentCount)
        .arg(summary.dependencyCaptured ? QStringLiteral("已捕获") : QStringLiteral("未捕获")));
}
