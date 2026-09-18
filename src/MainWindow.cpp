#include "MainWindow.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget* projectPage, QWidget* parent, const QString& initialPage)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("软件供应链漏洞风险评估系统"));
    resize(1000, 640);
    setMinimumSize(680, 420);

    auto* content = new QWidget(this);
    auto* layout = new QHBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    auto* sidebar = new QWidget(content);
    sidebar->setObjectName(QStringLiteral("sidebar"));
    sidebar->setFixedWidth(190);
    auto* navigationLayout = new QVBoxLayout(sidebar);
    navigationLayout->setContentsMargins(18, 28, 18, 24);
    navigationLayout->setSpacing(10);
    auto* brand = new QLabel(QStringLiteral("供应链风险评估\n应用工作台"), sidebar);
    brand->setObjectName(QStringLiteral("brand"));
    navigationLayout->addWidget(brand);
    navigationLayout->addSpacing(26);

    m_navigation = new QButtonGroup(this);
    m_navigation->setExclusive(true);
    m_pages = new QStackedWidget(content);
    m_pages->setObjectName(QStringLiteral("pages"));
    const auto addPage = [&](QWidget* page, const QString& id, const QString& heading) {
        page->setObjectName(id);
        const int index = m_pages->addWidget(page);
        auto* button = new QPushButton(heading, sidebar);
        button->setObjectName(QStringLiteral("nav_%1").arg(id));
        button->setCheckable(true);
        button->setMinimumHeight(46);
        m_navigation->addButton(button, index);
        navigationLayout->addWidget(button);
    };
    const auto addTextPage = [&](const QString& id, const QString& heading, const QString& description) {
        auto* page = new QWidget(m_pages);
        auto* pageLayout = new QVBoxLayout(page);
        pageLayout->setContentsMargins(32, 32, 32, 32);
        pageLayout->setSpacing(20);
        auto* title = new QLabel(heading, page);
        title->setObjectName(QStringLiteral("pageTitle"));
        title->setWordWrap(true);
        auto* body = new QLabel(description, page);
        body->setWordWrap(true);
        body->setTextFormat(Qt::PlainText);
        body->setTextInteractionFlags(Qt::TextSelectableByMouse);
        pageLayout->addWidget(title);
        pageLayout->addWidget(body);
        pageLayout->addStretch();
        addPage(page, id, heading);
    };
    addTextPage(QStringLiteral("overview"), QStringLiteral("概览"),
            QStringLiteral("软件供应链漏洞风险评估系统\n\n欢迎使用应用工作台。\n\n"
                           "当前已提供项目创建、查看与删除，以及 CycloneDX SBOM 预览、质量诊断、组件与依赖持久化、依赖关系分析。\n\n"
                           "业务分析功能将随后续阶段逐步开放。"));
    addPage(projectPage, QStringLiteral("projects"), QStringLiteral("项目"));
    addTextPage(QStringLiteral("settings"), QStringLiteral("设置"),
            QStringLiteral("页面偏好\n\n应用自动保存最后访问的页面，并在下次启动时恢复。\n\n"
                           "验证方法：停留在本页，正常关闭程序后再次启动，应回到“设置”。"));
    navigationLayout->addStretch();
    auto* stage = new QLabel(QStringLiteral("Phase 06 · 依赖关系分析"), sidebar);
    stage->setWordWrap(true);
    navigationLayout->addWidget(stage);
    layout->addWidget(sidebar);
    layout->addWidget(m_pages, 1);
    setCentralWidget(content);

    setStyleSheet(QStringLiteral(
        "QWidget#sidebar { background: #edf2f7; border-right: 1px solid #d5dee8; }"
        "QLabel#brand { color: #243b53; font-size: 17px; font-weight: 600; }"
        "QLabel#pageTitle { color: #243b53; font-size: 26px; font-weight: 600; }"
        "QWidget#sidebar QPushButton { text-align: left; padding: 8px 14px; border: 1px solid transparent; border-radius: 6px; }"
        "QWidget#sidebar QPushButton:checked { background: #234c74; color: white; }"
        "QWidget#sidebar QPushButton:hover:!checked { background: #dbe5ef; }"));

    connect(m_navigation, &QButtonGroup::idClicked, m_pages, &QStackedWidget::setCurrentIndex);
    connect(m_pages, &QStackedWidget::currentChanged, this, [this](int index) {
        auto* button = m_navigation->button(index);
        button->setChecked(true);
        statusBar()->showMessage(QStringLiteral("当前页面：%1").arg(button->text()));
        emit navigationChanged(currentPageId());
    });
    m_navigation->button(0)->setChecked(true);
    statusBar()->showMessage(QStringLiteral("当前页面：概览"));
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
            m_pages->setCurrentIndex(index);
            return true;
        }
    }
    return false;
}
