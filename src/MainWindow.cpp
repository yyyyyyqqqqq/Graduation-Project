#include "MainWindow.h"
#include "AppDatabase.h"
#include "RiskPriority.h"

#include <QButtonGroup>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedWidget>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>

namespace {
QString pageForRoute(const QString& route)
{
    if (route == "components" || route == "dependencies" || route == "vulnerabilities" || route == "risk")
        return QStringLiteral("projects");
    return route;
}
}

MainWindow::MainWindow(QWidget* projectPage, QWidget* parent, const QString& initialPage, QWidget* validationPage, QWidget* overviewPage)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("软件供应链漏洞风险评估系统"));
    resize(1000, 640);
    setMinimumSize(680, 420);

    auto* content = new QWidget(this);
    content->setObjectName(QStringLiteral("applicationShell"));
    auto* layout = new QHBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    auto* sidebar = new QWidget(content);
    sidebar->setObjectName(QStringLiteral("sidebar"));
    sidebar->setFixedWidth(190);
    auto* sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);
    auto* navigationScroll = new QScrollArea(sidebar);
    m_navigationScroll = navigationScroll;
    navigationScroll->setObjectName(QStringLiteral("navigationScroll"));
    navigationScroll->setWidgetResizable(true);
    navigationScroll->setFrameShape(QFrame::NoFrame);
    auto* navigationContent = new QWidget;
    navigationContent->setObjectName(QStringLiteral("navigationContent"));
    navigationScroll->setWidget(navigationContent);
    sidebarLayout->addWidget(navigationScroll);
    auto* navigationLayout = new QVBoxLayout(navigationContent);
    navigationLayout->setContentsMargins(14, 22, 14, 18);
    navigationLayout->setSpacing(8);
    auto* brand = new QLabel(QStringLiteral("供应链风险评估\n应用工作台"), navigationContent);
    brand->setObjectName(QStringLiteral("brand"));
    brand->setWordWrap(true);
    auto brandFont = brand->font();
    brandFont.setPointSizeF(brandFont.pointSizeF() + 2);
    brandFont.setWeight(QFont::DemiBold);
    brand->setFont(brandFont);
    navigationLayout->addWidget(brand);
    navigationLayout->addSpacing(14);
    const auto addGroup = [&](const QString& text) {
        auto* label = new QLabel(text, navigationContent);
        label->setObjectName(QStringLiteral("navigationGroup"));
        navigationLayout->addWidget(label);
    };
    addGroup(QStringLiteral("工作台"));

    m_navigation = new QButtonGroup(this);
    m_navigation->setExclusive(true);
    m_pages = new QStackedWidget(content);
    m_pages->setObjectName(QStringLiteral("pages"));
    const auto addRoute = [&](const QString& id, const QString& heading) {
        auto* button = new QPushButton(heading, navigationContent);
        button->setObjectName(QStringLiteral("nav_%1").arg(id));
        button->setCheckable(true);
        button->setMinimumHeight(42);
        m_navigation->addButton(button);
        navigationLayout->addWidget(button);
        connect(button, &QPushButton::clicked, this, [this, id] { selectRoute(id); });
    };
    const auto addPage = [&](QWidget* page, const QString& id, const QString& heading) {
        page->setObjectName(id);
        m_pages->addWidget(page);
        addRoute(id, heading);
    };
    const auto addTextPage = [&](const QString& id, const QString& heading, const QString& description) {
        auto* page = new QScrollArea(m_pages);
        page->setWidgetResizable(true);
        page->setFrameShape(QFrame::NoFrame);
        auto* textContent = new QWidget;
        textContent->setObjectName(QStringLiteral("shellTextContent"));
        page->setWidget(textContent);
        auto* pageLayout = new QVBoxLayout(textContent);
        pageLayout->setContentsMargins(32, 32, 32, 32);
        pageLayout->setSpacing(20);
        auto* title = new QLabel(heading, textContent);
        title->setObjectName(QStringLiteral("pageTitle"));
        title->setWordWrap(true);
        auto* body = new QLabel(description, textContent);
        body->setWordWrap(true);
        body->setTextFormat(Qt::PlainText);
        body->setTextInteractionFlags(Qt::TextSelectableByMouse);
        pageLayout->addWidget(title);
        pageLayout->addWidget(body);
        pageLayout->addStretch();
        addPage(page, id, heading);
    };
    if (overviewPage) addPage(overviewPage, QStringLiteral("overview"), QStringLiteral("项目概览"));
    else addTextPage(QStringLiteral("overview"), QStringLiteral("项目概览"),
                    QStringLiteral("当前未选择项目\n请进入“项目”页面选择或创建项目。"));
    addPage(projectPage, QStringLiteral("projects"), QStringLiteral("项目管理"));
    navigationLayout->addSpacing(12);
    addGroup(QStringLiteral("供应链分析"));
    addRoute(QStringLiteral("components"), QStringLiteral("SBOM 与组件"));
    addRoute(QStringLiteral("dependencies"), QStringLiteral("依赖关系"));
    addRoute(QStringLiteral("vulnerabilities"), QStringLiteral("漏洞匹配"));
    addRoute(QStringLiteral("risk"), QStringLiteral("风险证据"));
    navigationLayout->addSpacing(12);
    addGroup(QStringLiteral("研究与配置"));
    if (validationPage) addPage(validationPage, QStringLiteral("validation"), QStringLiteral("方法验证"));
    addTextPage(QStringLiteral("settings"), QStringLiteral("设置"),
            QStringLiteral("页面偏好\n\n应用自动保存最后访问的页面，并在下次启动时恢复。\n\n"
                           "应用信息（只读）\n\nApplication Version：%1\nDatabase Schema Version：%2\n"
                           "Rules Version：v%3\nProduction EPSS Percentile Threshold：%4\n\n"
                           "上述方法信息为只读，不是可编辑设置。")
                .arg(QStringLiteral(APPLICATION_VERSION)).arg(AppDatabase::SchemaVersion)
                .arg(ProductionRulesVersion).arg(ProductionEpssPercentileThreshold, 0, 'f', 2));
    navigationLayout->addStretch();
    layout->addWidget(sidebar);
    layout->addWidget(m_pages, 1);
    setCentralWidget(content);
    statusBar()->setObjectName(QStringLiteral("shellStatus"));

    setStyleSheet(QStringLiteral(
        "QWidget#applicationShell, QStackedWidget#pages { background: #f3f6fa; }"
        "QWidget#sidebar, QWidget#navigationContent, QScrollArea#navigationScroll { background: #162d47; }"
        "QLabel#brand { color: #ffffff; }"
        "QLabel#navigationGroup { color: #a8bdd4; padding: 4px 8px; }"
        "QWidget#navigationContent QPushButton { color: #dce6f2; background: transparent;"
        " text-align: left; padding: 8px 14px; border: 2px solid transparent; border-radius: 7px; }"
        "QWidget#navigationContent QPushButton:hover:!checked { background: #213f5d; color: #ffffff; }"
        "QWidget#navigationContent QPushButton:checked { background: #285780; color: #ffffff; font-weight: 600; }"
        "QWidget#navigationContent QPushButton:pressed { background: #326b99; }"
        "QWidget#navigationContent QPushButton:focus { border-color: #9dccff; }"
        "QWidget#navigationContent QPushButton:disabled { color: #8fa4bb; background: #1d354e; }"
        "QWidget#shellTextContent { background: #f3f6fa; }"
        "QWidget#shellTextContent QLabel#pageTitle { color: #172e49; font-size: 26px; font-weight: 600; }"
        // Preserve existing headings without styling other pages' controls or dialogs.
        "QWidget#projects QLabel#pageTitle, QWidget#validation QLabel#pageTitle"
        " { color: #243b53; font-size: 26px; font-weight: 600; }"
        "QStatusBar#shellStatus { background: #ffffff; color: #52647b; border-top: 1px solid #dce4ed; }"));

    auto* bar = navigationScroll->verticalScrollBar();
    connect(bar, &QScrollBar::rangeChanged, this,
            [this, previousMaximum = bar->maximum()](int, int maximum) mutable {
        const int oldMaximum = previousMaximum;
        previousMaximum = maximum;
        if (maximum <= oldMaximum) return;
        auto* current = m_navigation->checkedButton();
        if (!current) return;
        // Keep a previously visible selection reachable when the viewport shrinks.
        // If the user scrolled it out of view, preserve their browsing position.
        const int oldHeight = m_navigationScroll->widget()->height() - oldMaximum;
        const int center = current->mapTo(m_navigationScroll->viewport(), current->rect().center()).y();
        if (center >= 0 && center < oldHeight)
            m_navigationScroll->ensureWidgetVisible(current, 0, 8);
    });
    updateNavigation();
    selectPage(initialPage); // Unknown preferences safely fall back to overview.
}

QString MainWindow::currentPageId() const
{
    return m_pages->currentWidget()->objectName();
}

bool MainWindow::selectPage(const QString& pageId)
{
    for (int index = 0; index < m_pages->count(); ++index) {
        if (m_pages->widget(index)->objectName() == pageId) {
            return selectRoute(pageId);
        }
    }
    return false;
}

bool MainWindow::selectRoute(const QString& routeId)
{
    auto* button = findChild<QPushButton*>(QStringLiteral("nav_%1").arg(routeId));
    if (!button || !m_navigation->buttons().contains(button)) return false;
    const QString pageId = pageForRoute(routeId);
    QWidget* target = nullptr;
    for (int i = 0; i < m_pages->count(); ++i)
        if (m_pages->widget(i)->objectName() == pageId) target = m_pages->widget(i);
    if (!target) return false;
    const QString previousPage = currentPageId();
    const bool changed = m_routeId != routeId;
    if (!changed && previousPage == pageId) return true;
    m_routeId = routeId;
    // Composition in main connects this to the existing persistent project workspace.
    // Select the child before showing its parent, avoiding an unintended old tab's showEvent.
    if (pageId == "projects") emit workspaceRequested(routeId);
    m_pages->setCurrentWidget(target);
    updateNavigation();
    if (previousPage != currentPageId()) emit navigationChanged(currentPageId());
    if (changed) emit routeChanged(m_routeId);
    return true;
}

void MainWindow::updateNavigation()
{
    auto* button = findChild<QPushButton*>(QStringLiteral("nav_%1").arg(m_routeId));
    button->setChecked(true);
    statusBar()->showMessage(QStringLiteral("当前页面：%1").arg(button->text()));
    m_navigationScroll->ensureWidgetVisible(button, 0, 8);
}
