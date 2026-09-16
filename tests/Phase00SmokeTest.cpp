#include "MainWindow.h"

#include <QFile>
#include <QLibraryInfo>
#include <QProcess>
#include <QScopeGuard>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>
#include <QXmlStreamReader>

#include <algorithm>
#include <array>
#include <span>

static_assert(__cplusplus >= 202002L, "Phase 00 requires C++20");
static_assert(QT_VERSION == QT_VERSION_CHECK(6, 11, 2), "Phase 00 requires Qt 6.11.2");

class Phase00SmokeTest final : public QObject
{
    Q_OBJECT

private slots:
    void qtCppSmoke();
    void sqliteReadWrite();
    void graphvizSvg();
};

void Phase00SmokeTest::qtCppSmoke()
{
    std::array values{3, 1, 2};
    std::span view{values};
    std::ranges::sort(view);
    QCOMPARE(values.front(), 1);
    QCOMPARE(values.back(), 3);

    QCOMPARE(QString::fromLatin1(qVersion()), QStringLiteral("6.11.2"));
    qInfo().noquote() << "Runtime Qt:" << qVersion()
                      << "Prefix:" << QLibraryInfo::path(QLibraryInfo::PrefixPath)
                      << "Platform:" << QGuiApplication::platformName()
                      << "C++:" << __cplusplus;

    MainWindow window(new QWidget); // Shell smoke uses an injected page; business UI is tested in Phase 02.
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(window.isVisible());
    QVERIFY(window.centralWidget() != nullptr);
    QVERIFY(window.close());
}

void Phase00SmokeTest::sqliteReadWrite()
{
    QVERIFY2(QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE")),
             "Qt QSQLITE driver is unavailable");
    const QString connectionName = QStringLiteral("phase00-smoke");
    {
        // Declared before database/query so removal happens after their destruction,
        // including when a Qt Test assertion returns early.
        const auto removeConnection = qScopeGuard([&connectionName] {
            QSqlDatabase::removeDatabase(connectionName);
        });
        auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        database.setDatabaseName(QStringLiteral(":memory:"));
        QVERIFY2(database.open(), qPrintable(database.lastError().text()));
        {
            QSqlQuery query(database);
            QVERIFY2(query.exec(QStringLiteral("CREATE TABLE smoke (id INTEGER PRIMARY KEY, value TEXT NOT NULL)")),
                     qPrintable(query.lastError().text()));
            QVERIFY(query.prepare(QStringLiteral("INSERT INTO smoke (id, value) VALUES (?, ?)")));
            query.addBindValue(1);
            query.addBindValue(QStringLiteral("SQLite 中文验证"));
            QVERIFY2(query.exec(), qPrintable(query.lastError().text()));
            QVERIFY2(query.exec(QStringLiteral("SELECT id, value FROM smoke")),
                     qPrintable(query.lastError().text()));
            QVERIFY(query.next());
            QCOMPARE(query.value(0).toInt(), 1);
            QCOMPARE(query.value(1).toString(), QStringLiteral("SQLite 中文验证"));
            QVERIFY(!query.next());
            QVERIFY(!query.lastError().isValid());
        }
        database.close();
    }
    QVERIFY(!QSqlDatabase::contains(connectionName));
}

void Phase00SmokeTest::graphvizSvg()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString inputPath = directory.filePath(QStringLiteral("依赖测试.dot"));
    const QString outputPath = directory.filePath(QStringLiteral("依赖测试.svg"));
    QFile input(inputPath);
    QVERIFY(input.open(QIODevice::WriteOnly));
    const QByteArray dot("digraph G { A -> B; }\n");
    QCOMPARE(input.write(dot), dot.size());
    input.close();

    QProcess process;
    process.start(QString::fromUtf8(GRAPHVIZ_DOT_EXECUTABLE),
                  {QStringLiteral("-Tsvg"), inputPath, QStringLiteral("-o"), outputPath});
    QVERIFY2(process.waitForStarted(5000), qPrintable(process.errorString()));
    QVERIFY2(process.waitForFinished(15000), qPrintable(process.errorString()));
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    const QByteArray errors = process.readAllStandardError();
    QVERIFY2(process.exitCode() == 0, errors.constData());

    QFile output(outputPath);
    QVERIFY(output.open(QIODevice::ReadOnly));
    QVERIFY(output.size() > 0);
    QXmlStreamReader xml(output.readAll());
    QVERIFY(xml.readNextStartElement());
    QCOMPARE(xml.name().toString(), QStringLiteral("svg"));
    while (!xml.atEnd()) {
        xml.readNext();
    }
    QVERIFY2(!xml.hasError(), qPrintable(xml.errorString()));
    output.close();
    QVERIFY(directory.remove());
}

QTEST_MAIN(Phase00SmokeTest)
#include "Phase00SmokeTest.moc"
