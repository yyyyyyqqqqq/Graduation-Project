#include "AppDatabase.h"
#include "AppLogger.h"
#include "AppPaths.h"
#include "AppSettings.h"
#include "MainWindow.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QProcess>
#include <QPushButton>
#include <QScopeGuard>
#include <QSettings>
#include <QSignalSpy>
#include <QSqlError>
#include <QSqlQuery>
#include <QStackedWidget>
#include <QTemporaryDir>
#include <QTest>

class Phase01Test final : public QObject
{
    Q_OBJECT

private slots:
    void paths();
    void settings();
    void database();
    void databaseRejectsUnknownSchema();
    void logging();
    void navigation();
    void applicationStartup();
};

void Phase01Test::paths()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const AppPaths paths{temporary.filePath(QStringLiteral("应用 数据"))};
    QString error;
    QVERIFY2(paths.initialize(error), qPrintable(error));
    QVERIFY(QDir(paths.dataDirectory()).exists());
    QVERIFY(QDir(paths.logsDirectory()).exists());
    QCOMPARE(QFileInfo(paths.databaseFile()).absolutePath(), paths.dataDirectory());
    QVERIFY(paths.initialize(error)); // Idempotent startup.

    QFile blocker(temporary.filePath(QStringLiteral("file-not-directory")));
    QVERIFY(blocker.open(QIODevice::WriteOnly));
    blocker.close();
    QVERIFY(!AppPaths{blocker.fileName()}.initialize(error));
    QVERIFY(!error.isEmpty());
    QVERIFY(!AppPaths{QString()}.initialize(error));
    QVERIFY(!AppPaths{QStringLiteral("relative")}.initialize(error));
}

void Phase01Test::settings()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const QString file = temporary.filePath(QStringLiteral("settings.ini"));
    QString page;
    QString error;
    QVERIFY(AppSettings::readLastNavigationPage(file, page, error));
    QCOMPARE(page, QStringLiteral("overview"));
    QVERIFY(AppSettings::writeLastNavigationPage(file, QStringLiteral("settings"), error));
    QVERIFY(AppSettings::readLastNavigationPage(file, page, error));
    QCOMPARE(page, QStringLiteral("settings"));
    const QSettings inspection(file, QSettings::IniFormat);
    QCOMPARE(inspection.allKeys(), QStringList{QStringLiteral("ui/lastNavigationPage")});

    QFile blocker(temporary.filePath(QStringLiteral("blocker")));
    QVERIFY(blocker.open(QIODevice::WriteOnly));
    blocker.close();
    QVERIFY(!AppSettings::writeLastNavigationPage(blocker.fileName() + QStringLiteral("/settings.ini"), page, error));
    QVERIFY(!error.isEmpty());

    QFile malformed(temporary.filePath(QStringLiteral("malformed.ini")));
    QVERIFY(malformed.open(QIODevice::WriteOnly));
    malformed.write("[broken-section\nvalue=1\n");
    malformed.close();
    QVERIFY(!AppSettings::readLastNavigationPage(malformed.fileName(), page, error));
    QVERIFY(!error.isEmpty());
}

void Phase01Test::database()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const AppPaths paths{temporary.path()};
    QString error;
    QVERIFY(paths.initialize(error));
    QString connectionName;
    {
        AppDatabase database;
        connectionName = database.connectionName();
        QVERIFY2(database.open(paths.databaseFile(), error), qPrintable(error));
        QVERIFY(database.isOpen());
        QVERIFY(QFileInfo::exists(paths.databaseFile()));
        {
            const auto connection = database.connection();
            auto tables = connection.tables();
            tables.sort();
            QCOMPARE(tables, (QStringList{QStringLiteral("app_meta"), QStringLiteral("components"), QStringLiteral("projects")}));
            QSqlQuery query(connection);
            QVERIFY(query.exec(QStringLiteral("SELECT key, value FROM app_meta")));
            QVERIFY(query.next());
            QCOMPARE(query.value(0).toString(), QStringLiteral("schema_version"));
            QCOMPARE(query.value(1).toString(), QStringLiteral("3"));
            QVERIFY(!query.next());
        }
        database.close();
        QVERIFY(!database.isOpen());
        QVERIFY(!QSqlDatabase::contains(connectionName));
        QVERIFY2(database.open(paths.databaseFile(), error), qPrintable(error));
    }
    QVERIFY(!QSqlDatabase::contains(connectionName));

    AppDatabase invalid;
    QVERIFY(!invalid.open(paths.dataDirectory(), error)); // Directory cannot be a DB file.
    QVERIFY(!error.isEmpty());
    QVERIFY(!QSqlDatabase::contains(invalid.connectionName()));
}

void Phase01Test::databaseRejectsUnknownSchema()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const QString file = temporary.filePath(QStringLiteral("future.db"));
    QString error;
    {
        AppDatabase database;
        QVERIFY(database.open(file, error));
        QSqlQuery query(database.connection());
        QVERIFY(query.exec(QStringLiteral("UPDATE app_meta SET value='99' WHERE key='schema_version'")));
    }
    AppDatabase database;
    QVERIFY(!database.open(file, error));
    QVERIFY(error.contains(QStringLiteral("Unsupported schema_version")));
    QVERIFY(!QSqlDatabase::contains(database.connectionName()));

    const QString inspectionName = QStringLiteral("phase01-schema-inspection");
    const auto cleanup = qScopeGuard([&] { QSqlDatabase::removeDatabase(inspectionName); });
    auto connection = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), inspectionName);
    connection.setDatabaseName(file);
    connection.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
    QVERIFY(connection.open());
    QSqlQuery query(connection);
    QVERIFY(query.exec(QStringLiteral("SELECT value FROM app_meta WHERE key='schema_version'")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("99")); // No silent downgrade.
}

void Phase01Test::logging()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const QString file = temporary.filePath(QStringLiteral("application.log"));
    {
        AppLogger logger;
        QVERIFY(logger.open(file));
        QVERIFY(logger.write(AppLogger::Level::Info, QStringLiteral("应用启动")));
        QVERIFY(logger.write(AppLogger::Level::Warning, QStringLiteral("warning record")));
        QVERIFY(logger.write(AppLogger::Level::Error, QStringLiteral("error record")));
    }
    {
        AppLogger logger;
        QVERIFY(logger.open(file));
        QVERIFY(logger.write(AppLogger::Level::Info, QStringLiteral("应用关闭")));
    }
    QFile log(file);
    QVERIFY(log.open(QIODevice::ReadOnly));
    const QByteArray contents = log.readAll();
    QVERIFY(contents.contains(QStringLiteral("应用启动").toUtf8()));
    QVERIFY(contents.contains(QStringLiteral("应用关闭").toUtf8()));
    QVERIFY(contents.contains("[WARN] warning record"));
    QVERIFY(contents.contains("[ERROR] error record"));
    AppLogger invalid;
    QVERIFY(!invalid.open(temporary.path()));
    QVERIFY(!invalid.write(AppLogger::Level::Error, QStringLiteral("Expected test: logging unavailable")));
}

void Phase01Test::navigation()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const QString settingsFile = temporary.filePath(QStringLiteral("settings.ini"));
    QString error;
    QString page;
    QVERIFY(AppSettings::readLastNavigationPage(settingsFile, page, error));
    {
        MainWindow window(new QWidget, nullptr, page);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QCOMPARE(window.currentPageId(), QStringLiteral("overview"));
        auto* pages = window.findChild<QStackedWidget*>(QStringLiteral("pages"));
        QVERIFY(pages);
        QCOMPARE(pages->count(), 3);
        QSignalSpy changes(&window, &MainWindow::navigationChanged);
        QString saveError;
        connect(&window, &MainWindow::navigationChanged, &window, [&](const QString& id) {
            AppSettings::writeLastNavigationPage(settingsFile, id, saveError);
        });
        int expectedSignals = 0;
        for (const QString& id : {QStringLiteral("projects"), QStringLiteral("overview"), QStringLiteral("settings")}) {
            auto* button = window.findChild<QPushButton*>(QStringLiteral("nav_%1").arg(id));
            QVERIFY(button);
            QTest::mouseClick(button, Qt::LeftButton);
            QCOMPARE(window.currentPageId(), id);
            QVERIFY(button->isChecked());
            QCOMPARE(pages->currentWidget()->objectName(), id);
            QCOMPARE(changes.count(), ++expectedSignals);
            QVERIFY2(saveError.isEmpty(), qPrintable(saveError));
        }
        QVERIFY(!window.selectPage(QStringLiteral("unknown")));
        QCOMPARE(window.currentPageId(), QStringLiteral("settings"));
        QCOMPARE(changes.count(), 3);
        window.resize(680, 420);
        QVERIFY(window.close());
    }
    QVERIFY(AppSettings::readLastNavigationPage(settingsFile, page, error));
    QCOMPARE(page, QStringLiteral("settings"));
    MainWindow restored(new QWidget, nullptr, page);
    QCOMPARE(restored.currentPageId(), QStringLiteral("settings"));
    QVERIFY(restored.findChild<QPushButton*>(QStringLiteral("nav_settings"))->isChecked());
    MainWindow unknown(new QWidget, nullptr, QStringLiteral("obsolete-page"));
    QCOMPARE(unknown.currentPageId(), QStringLiteral("overview"));
}

void Phase01Test::applicationStartup()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const AppPaths paths{temporary.filePath(QStringLiteral("真实程序 隔离运行"))};
    QProcess process;
    process.start(QString::fromUtf8(APPLICATION_EXECUTABLE), {QStringLiteral("--data-dir"), paths.root});
    QVERIFY2(process.waitForStarted(5000), qPrintable(process.errorString()));
    const auto stopOnFailure = qScopeGuard([&] {
        if (process.state() != QProcess::NotRunning) {
            process.kill();
            process.waitForFinished(5000);
        }
    });
    const auto readLog = [&] {
        QFile file(paths.logFile());
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    };
    QTRY_VERIFY_WITH_TIMEOUT(readLog().contains("Database initialized, schema_version=3"), 10000);
    QTRY_VERIFY_WITH_TIMEOUT(readLog().contains("Main window shown, page=overview"), 5000);
    QVERIFY(QFileInfo::exists(paths.settingsFile()));
    QCOMPARE(process.state(), QProcess::Running);
    // On Windows Qt sends WM_CLOSE to the application's windows, exercising normal shutdown.
    process.terminate();
    QVERIFY(process.waitForFinished(5000));
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
    const QByteArray log = readLog();
    QVERIFY(log.contains("Application starting"));
    QVERIFY(log.contains("Runtime directories ready"));
    QVERIFY(log.contains("Application closed normally"));
    QVERIFY(!log.contains("[ERROR]"));
}

QTEST_MAIN(Phase01Test)
#include "Phase01Test.moc"
