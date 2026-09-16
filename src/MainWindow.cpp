#include "MainWindow.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("软件供应链漏洞风险评估系统 — Phase 00"));
    resize(760, 420);

    auto* content = new QWidget(this);
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(32, 32, 32, 32);
    layout->setSpacing(16);
    layout->addStretch();

    auto* title = new QLabel(QStringLiteral("软件供应链漏洞风险评估系统"), content);
    auto titleFont = title->font();
    titleFont.setPointSize(20);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);
    title->setWordWrap(true);
    layout->addWidget(title);

    auto* status = new QLabel(QStringLiteral("Phase 00 · 工程初始化\nQt Widgets 主窗口已启动"), content);
    status->setAlignment(Qt::AlignCenter);
    layout->addWidget(status);
    layout->addStretch();
    setCentralWidget(content);
}
