#include "MainWindow.h"
#include "AppDatabase.h"
#include "AppLogger.h"
#include "AppPaths.h"
#include "AppSettings.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QMessageBox>
#include <QStatusBar>

#include <cstdio>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("GraduationProject"));
    QApplication::setApplicationName(QStringLiteral("SupplyChainRiskAssessment"));
    QApplication::setApplicationVersion(QStringLiteral("0.2.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("软件供应链漏洞风险评估系统"));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption dataDirectoryOption(QStringLiteral("data-dir"),
        QStringLiteral("使用指定的绝对路径保存应用数据（默认使用系统应用数据目录）。"), QStringLiteral("path"));
    parser.addOption(dataDirectoryOption);
    parser.process(application);
    const AppPaths paths = parser.isSet(dataDirectoryOption)
        ? AppPaths{parser.value(dataDirectoryOption)} : AppPaths::forApplication();
    QString error;
    if (!paths.initialize(error)) {
        // No file logger can exist until the runtime directories are available.
        std::fprintf(stderr, "%s\n", qPrintable(error));
        QMessageBox::critical(nullptr, QStringLiteral("运行目录不可用"),
                              QStringLiteral("无法准备应用数据目录，请检查磁盘空间和访问权限后重试。"));
        return 1;
    }

    AppLogger logger;
    const bool logOpened = logger.open(paths.logFile());
    if (!logOpened) {
        std::fprintf(stderr, "Cannot open application log: %s\n", qPrintable(logger.errorString()));
    }
    bool loggingReady = logger.write(AppLogger::Level::Info, QStringLiteral("Application starting, version 0.2.0"));
    loggingReady = logger.write(AppLogger::Level::Info,
                               QStringLiteral("Runtime directories ready: %1").arg(paths.root)) && loggingReady;
    if (!logOpened || !loggingReady) {
        QMessageBox::warning(nullptr, QStringLiteral("日志不可用"),
                             QStringLiteral("无法写入本地日志。本次运行将继续，但诊断记录可能无法保存。"));
    }

    QString initialPage;
    if (!AppSettings::readLastNavigationPage(paths.settingsFile(), initialPage, error)) {
        logger.write(AppLogger::Level::Error, error);
        QMessageBox::critical(nullptr, QStringLiteral("配置读取失败"),
                              QStringLiteral("无法读取应用配置。请检查配置文件格式和访问权限后重试。"));
        return 1;
    }

    AppDatabase database;
    if (!database.open(paths.databaseFile(), error)) {
        logger.write(AppLogger::Level::Error, error);
        QMessageBox::critical(nullptr, QStringLiteral("数据库初始化失败"),
                              QStringLiteral("无法打开应用数据库或数据库版本不受支持。请检查访问权限，或使用与数据版本匹配的程序。详情见本地日志。"));
        return 1;
    }
    logger.write(AppLogger::Level::Info, QStringLiteral("Database initialized, schema_version=1"));

    MainWindow window(nullptr, initialPage);
    if (window.currentPageId() != initialPage) {
        logger.write(AppLogger::Level::Warning, QStringLiteral("Unknown last navigation page; using overview"));
    }
    const auto savePage = [&](const QString& pageId) {
        QString saveError;
        if (!AppSettings::writeLastNavigationPage(paths.settingsFile(), pageId, saveError)) {
            logger.write(AppLogger::Level::Error, saveError);
            window.statusBar()->showMessage(QStringLiteral("页面偏好保存失败，下次启动可能无法恢复。"));
            QMessageBox::warning(&window, QStringLiteral("配置保存失败"),
                                 QStringLiteral("无法保存页面偏好，请检查应用数据目录的访问权限。"));
        }
    };
    // Persist on navigation; MainWindow owns only UI state, never file settings.
    QObject::connect(&window, &MainWindow::navigationChanged, &window, savePage);
    savePage(window.currentPageId());
    window.show();
    logger.write(AppLogger::Level::Info, QStringLiteral("Main window shown, page=%1").arg(window.currentPageId()));
    const int result = application.exec();
    database.close();
    logger.write(AppLogger::Level::Info, QStringLiteral("Application closed normally, exit code %1").arg(result));
    return result;
}
