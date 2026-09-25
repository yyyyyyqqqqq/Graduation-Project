#include "AppDatabase.h"
#include "AppLogger.h"
#include "ComponentRepository.h"
#include "CycloneDxParser.h"
#include "ProjectRepository.h"
#include "ProjectPage.h"
#include "MainWindow.h"
#include "SbomImportDialog.h"

#include <QElapsedTimer>
#include <QAbstractEventDispatcher>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QPointer>
#include <QPushButton>
#include <QSignalSpy>
#include <QTableView>
#include <QTabWidget>
#include <QThreadPool>
#include <QScopeGuard>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>

namespace {
struct Context {
    QTemporaryDir temporary;
    AppLogger logger;
    AppDatabase database;
    ProjectRepository projects{database, logger};
    ComponentRepository components{database};
    QString error;
    QString file() const { return temporary.filePath(QStringLiteral("synthetic.db")); }
    bool open() { return temporary.isValid() && logger.open(temporary.filePath(QStringLiteral("test.log")))
        && database.open(file(), error); }
};
SbomDocument largeDocument()
{
    SbomDocument document;
    for (int i = 0; i < 100000; ++i) {
        const auto name = QStringLiteral("synthetic-%1").arg(i);
        document.components.append({name, "library", name, "1.0", QStringLiteral("pkg:generic/%1@1.0").arg(name)});
    }
    return document;
}
QByteArray sample()
{
    return R"({"bomFormat":"CycloneDX","specVersion":"1.6",
        "metadata":{"component":{"name":"synthetic-root","components":[{"name":"root-child"}]}},
        "components":[{"name":"parent","components":[{"name":"child"}]},{"name":"last"}]})";
}
bool writeFile(const QString& path, const QByteArray& bytes)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
}
QVariant scalar(AppDatabase& database, const QString& sql)
{
    QSqlQuery query(database.connection());
    return query.exec(sql) && query.next() ? query.value(0) : QVariant();
}
bool execute(const QString& file, const QStringList& statements)
{
    const auto name = QUuid::createUuid().toString();
    const auto cleanup = qScopeGuard([&] { QSqlDatabase::removeDatabase(name); });
    auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
    database.setDatabaseName(file);
    if (!database.open()) return false;
    QSqlQuery query(database);
    for (const auto& sql : statements) if (!query.exec(sql)) return false;
    return true;
}
QVariant inspect(const QString& file, const QString& sql)
{
    const auto name = QUuid::createUuid().toString();
    const auto cleanup = qScopeGuard([&] { QSqlDatabase::removeDatabase(name); });
    auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
    database.setDatabaseName(file);
    database.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
    if (!database.open()) return {};
    QSqlQuery query(database);
    return query.exec(sql) && query.next() ? query.value(0) : QVariant();
}
bool createV2(const QString& file)
{
    return execute(file, {QStringLiteral("CREATE TABLE app_meta(key TEXT PRIMARY KEY NOT NULL,value TEXT NOT NULL)"),
        QStringLiteral("INSERT INTO app_meta VALUES('schema_version','2'),('preserved','synthetic metadata')"),
        QStringLiteral("CREATE TABLE projects(id TEXT PRIMARY KEY NOT NULL,name TEXT NOT NULL,description TEXT NOT NULL DEFAULT '',created_at INTEGER NOT NULL)"),
        QStringLiteral("INSERT INTO projects VALUES('synthetic-id','项目🙂',' original description ',12345)")});
}
QList<Component> rows(ComponentRepository& repository, const QString& id)
{
    QList<Component> output;
    if (!repository.listForProject(id, output).ok()) qFatal("Cannot inspect test components");
    return output;
}
}

class Phase05Test final : public QObject {
    Q_OBJECT
private slots:
    void schema();
    void migrateV2();
    void migrationFailures_data();
    void migrationFailures();
    void foreignKeys();
    void sourcePreservation();
    void replacement();
    void rollback();
    void repositoryErrors();
    void previewOnly();
    void explicitApply();
    void parserFailures_data();
    void parserFailures();
    void applyFailureRetry();
    void projectUi();
    void largeReplacement();
    void largeApply();
    void workerLifetime();
};

void Phase05Test::schema()
{
    Context c;
    QVERIFY2(c.open(), qPrintable(c.error));
    QCOMPARE(AppDatabase::SchemaVersion, 4);
    QCOMPARE(scalar(c.database, "SELECT value FROM app_meta WHERE key='schema_version'").toString(), QStringLiteral("4"));
    auto tables = c.database.connection().tables(); tables.sort();
    QCOMPARE(tables, (QStringList{"app_meta", "components", "dependency_capture", "dependency_entries", "dependency_targets", "projects"}));
    QSqlQuery q(c.database.connection());
    QVERIFY(q.exec("PRAGMA table_info(components)"));
    QStringList columns;
    while (q.next()) {
        columns.append(q.value(1).toString());
        QCOMPARE(q.value(3).toInt(), 1);
        QCOMPARE(q.value(2).toString(), q.value(1).toString().startsWith(QStringLiteral("source_"))
            ? QStringLiteral("INTEGER") : QStringLiteral("TEXT"));
        if (q.value(0).toInt() >= 4) QCOMPARE(q.value(4).toString(), QStringLiteral("''"));
        if (q.value(1).toString() == QStringLiteral("id")) QCOMPARE(q.value(5).toInt(), 1);
    }
    QCOMPARE(columns, (QStringList{"id","project_id","source_role","source_order","bom_ref","type","name","version","purl"}));
    QVERIFY(q.exec("PRAGMA index_list(components)"));
    bool found = false;
    while (q.next()) if (q.value(1).toString() == QStringLiteral("components_project_source")) {
        found = true; QCOMPARE(q.value(2).toInt(), 1);
    }
    QVERIFY(found);
    QVERIFY(q.exec("PRAGMA index_info(components_project_source)"));
    columns.clear();
    while (q.next()) columns.append(q.value(2).toString());
    QCOMPARE(columns, (QStringList{"project_id","source_role","source_order"}));
    QVERIFY(q.exec("PRAGMA foreign_key_list(components)"));
    QVERIFY(q.next());
    QCOMPARE(q.value(2).toString(), QStringLiteral("projects"));
    QCOMPARE(q.value(3).toString(), QStringLiteral("project_id"));
    QCOMPARE(q.value(4).toString(), QStringLiteral("id"));
    QCOMPARE(q.value(6).toString(), QStringLiteral("CASCADE"));
    QVERIFY(!q.next());
    QVERIFY(q.exec("INSERT INTO projects VALUES('a','synthetic','',0)"));
    QVERIFY(q.exec("INSERT INTO components(id,project_id,source_role,source_order) VALUES('one','a',1,0)"));
    QVERIFY(!q.exec("INSERT INTO components(id,project_id,source_role,source_order) VALUES('two','a',1,0)"));
    QVERIFY(!q.exec("INSERT INTO components(id,project_id,source_role,source_order) VALUES('two','a',2,1)"));
    QVERIFY(!q.exec("INSERT INTO components(id,project_id,source_role,source_order) VALUES('two','a',0,1)"));
    QVERIFY(!q.exec("INSERT INTO components(id,project_id,source_role,source_order) VALUES('two','a',1,-1)"));
}

void Phase05Test::migrateV2()
{
    Context c;
    QVERIFY(createV2(c.file()));
    QVERIFY2(c.open(), qPrintable(c.error));
    QCOMPARE(scalar(c.database, "SELECT value FROM app_meta WHERE key='schema_version'").toString(), QStringLiteral("4"));
    QCOMPARE(scalar(c.database, "SELECT value FROM app_meta WHERE key='preserved'").toString(), QStringLiteral("synthetic metadata"));
    Project p;
    QVERIFY(c.projects.findById(QStringLiteral("synthetic-id"), p).ok());
    QCOMPARE(p.name, QStringLiteral("项目🙂"));
    QCOMPARE(p.description, QStringLiteral(" original description "));
    QCOMPARE(p.createdAt, 12345);
    QVERIFY(c.components.replaceForProject(p.id, CycloneDxParser::parse(sample()).document).ok());
    const auto before = rows(c.components, p.id);
    c.database.close();
    QVERIFY(c.database.open(c.file(), c.error));
    QCOMPARE(rows(c.components, p.id), before);
    QVERIFY(c.projects.findById(QStringLiteral("synthetic-id"), p).ok());
    QCOMPARE(p.name, QStringLiteral("项目🙂"));
    QCOMPARE(scalar(c.database, "PRAGMA foreign_keys").toInt(), 1);
    Context chain;
    QVERIFY(execute(chain.file(), {
        "CREATE TABLE app_meta(key TEXT PRIMARY KEY NOT NULL,value TEXT NOT NULL)",
        "INSERT INTO app_meta VALUES('schema_version','1'),('preserved','synthetic metadata')",
        "CREATE TRIGGER reject_v3 BEFORE UPDATE ON app_meta WHEN NEW.value='3' BEGIN SELECT RAISE(ABORT,'synthetic failure'); END"}));
    QVERIFY(!chain.open());
    QCOMPARE(inspect(chain.file(), "SELECT value FROM app_meta WHERE key='schema_version'").toString(), QStringLiteral("1"));
    QCOMPARE(inspect(chain.file(), "SELECT count(*) FROM sqlite_master WHERE name IN ('projects','components','components_project_source')").toInt(),0);
    QCOMPARE(inspect(chain.file(), "SELECT value FROM app_meta WHERE key='preserved'").toString(),QStringLiteral("synthetic metadata"));
    QVERIFY(execute(chain.file(), {"DROP TRIGGER reject_v3"}));
    QVERIFY(chain.database.open(chain.file(),chain.error));
    QCOMPARE(scalar(chain.database,"SELECT value FROM app_meta WHERE key='schema_version'").toString(),QStringLiteral("4"));
}

void Phase05Test::migrationFailures_data()
{
    QTest::addColumn<QStringList>("fault");
    QTest::addColumn<QString>("version");
    QTest::newRow("components-conflict") << QStringList{"CREATE TABLE components(payload TEXT)",
        "INSERT INTO components VALUES('synthetic preserved')"} << QStringLiteral("2");
    QTest::newRow("index-conflict") << QStringList{"CREATE INDEX components_project_source ON projects(name)"} << QStringLiteral("2");
    QTest::newRow("version-update-fails") << QStringList{"CREATE TRIGGER reject_v3 BEFORE UPDATE ON app_meta WHEN NEW.value='3' BEGIN SELECT RAISE(ABORT,'synthetic failure'); END"} << QStringLiteral("2");
    QTest::newRow("future") << QStringList{"UPDATE app_meta SET value='99' WHERE key='schema_version'"} << QStringLiteral("99");
}

void Phase05Test::migrationFailures()
{
    QFETCH(QStringList, fault);
    QFETCH(QString, version);
    Context c;
    QVERIFY(createV2(c.file()));
    QVERIFY(execute(c.file(), fault));
    QVERIFY(!c.open());
    QVERIFY(!c.error.isEmpty());
    QVERIFY(!QSqlDatabase::contains(c.database.connectionName()));
    QCOMPARE(inspect(c.file(), "SELECT value FROM app_meta WHERE key='schema_version'").toString(), version);
    QCOMPARE(inspect(c.file(), "SELECT name FROM projects").toString(), QStringLiteral("项目🙂"));
    QCOMPARE(inspect(c.file(), "SELECT description FROM projects").toString(), QStringLiteral(" original description "));
    QCOMPARE(inspect(c.file(), "SELECT created_at FROM projects").toLongLong(), 12345);
    if (fault.first().startsWith(QStringLiteral("CREATE TABLE components")))
        QCOMPARE(inspect(c.file(), "SELECT payload FROM components").toString(), QStringLiteral("synthetic preserved"));
    else QCOMPARE(inspect(c.file(), "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='components'").toInt(), 0);
    if (!fault.first().startsWith(QStringLiteral("CREATE INDEX")))
        QCOMPARE(inspect(c.file(), "SELECT count(*) FROM sqlite_master WHERE name='components_project_source'").toInt(),0);
}

void Phase05Test::foreignKeys()
{
    Context c;
    QVERIFY(c.open());
    QCOMPARE(scalar(c.database, "PRAGMA foreign_keys").toInt(), 1);
    {
        QSqlQuery q(c.database.connection());
        QVERIFY(!q.exec("INSERT INTO components(id,project_id,source_role,source_order) VALUES('x','missing',1,0)"));
    }
    Project a,b;
    QVERIFY(c.projects.create(QStringLiteral("A"), {}, a).ok());
    QVERIFY(c.projects.create(QStringLiteral("B"), {}, b).ok());
    const auto document = CycloneDxParser::parse(sample()).document;
    QVERIFY(c.components.replaceForProject(a.id, document).ok());
    QVERIFY(c.components.replaceForProject(b.id, document).ok());
    const auto bRows = rows(c.components, b.id);
    QVERIFY(c.projects.remove(a.id).ok());
    QVERIFY(rows(c.components, a.id).isEmpty());
    QCOMPARE(rows(c.components, b.id), bRows);
    c.database.close();
    QVERIFY(c.database.open(c.file(), c.error));
    QVERIFY(rows(c.components, a.id).isEmpty());
    QCOMPARE(rows(c.components, b.id), bRows);
}

void Phase05Test::sourcePreservation()
{
    Context c;
    QVERIFY(c.open());
    Project a;
    QVERIFY(c.projects.create(QStringLiteral("synthetic"), {}, a).ok());
    auto parsed = CycloneDxParser::parse(sample());
    QVERIFY(parsed.ok());
    auto& doc = parsed.document;
    doc.components.append({QStringLiteral(" duplicate "), QStringLiteral(" LiBrArY "),
        QStringLiteral(" O'Reilly 中文🙂 <b>x</b> "), QStringLiteral(" 1.0 "), QStringLiteral(" PKG:Generic/X@1.0 ")});
    doc.components.append(doc.components.last());
    doc.components.append(SbomComponent{});
    QVERIFY(c.components.replaceForProject(a.id, doc).ok());
    const auto saved = rows(c.components, a.id);
    QCOMPARE(saved.size(), 8);
    QCOMPARE(saved[0].sourceRole, ComponentSourceRole::MetadataRoot);
    QCOMPARE(saved[0].sourceOrder, 0);
    QCOMPARE(saved[0].name, QStringLiteral("synthetic-root"));
    QCOMPARE(saved[1].name, QStringLiteral("root-child"));
    QCOMPARE(saved[2].name, QStringLiteral("parent"));
    QCOMPARE(saved[3].name, QStringLiteral("child"));
    QCOMPARE(saved[4].name, QStringLiteral("last"));
    QSet<QString> ids{saved[0].id};
    for (int i = 0; i < doc.components.size(); ++i) {
        const auto& row = saved[i + 1]; const auto& source = doc.components[i];
        QCOMPARE(row.projectId, a.id);
        QCOMPARE(row.sourceRole, ComponentSourceRole::Component);
        QCOMPARE(row.sourceOrder, i);
        QCOMPARE(row.bomRef, source.bomRef); QCOMPARE(row.type, source.type);
        QCOMPARE(row.name, source.name); QCOMPARE(row.version, source.version); QCOMPARE(row.purl, source.purl);
        QVERIFY(!QUuid(row.id).isNull()); ids.insert(row.id);
    }
    QCOMPARE(ids.size(), saved.size());
}

void Phase05Test::replacement()
{
    Context c;
    QVERIFY(c.open());
    Project a,b;
    QVERIFY(c.projects.create(QStringLiteral("A"), {}, a).ok());
    QVERIFY(c.projects.create(QStringLiteral("B"), {}, b).ok());
    auto doc = CycloneDxParser::parse(sample()).document;
    QVERIFY(c.components.replaceForProject(a.id, doc).ok());
    QVERIFY(c.components.replaceForProject(b.id, doc).ok());
    const auto bRows = rows(c.components, b.id);
    const auto first = rows(c.components, a.id);
    QVERIFY(c.components.replaceForProject(a.id, doc).ok());
    const auto second = rows(c.components, a.id);
    QSet<QString> oldIds;
    for (const auto& row : first) oldIds.insert(row.id);
    for (const auto& row : second) QVERIFY(!oldIds.contains(row.id));
    doc.components = {{"new", "library", "replacement-only", "2", "pkg:generic/replacement@2"}};
    doc.metadata.component.reset();
    QVERIFY(c.components.replaceForProject(a.id, doc).ok());
    QCOMPARE(rows(c.components, a.id).size(), 1);
    QCOMPARE(rows(c.components, a.id).first().name, QStringLiteral("replacement-only"));
    doc.components.clear(); doc.metadata.component.emplace();
    QVERIFY(c.components.replaceForProject(a.id, doc).ok());
    QCOMPARE(rows(c.components, a.id).size(), 1);
    QCOMPARE(rows(c.components, a.id).first().sourceRole, ComponentSourceRole::MetadataRoot);
    doc.metadata.component.reset();
    QVERIFY(c.components.replaceForProject(a.id, doc).ok());
    QVERIFY(rows(c.components, a.id).isEmpty());
    QCOMPARE(rows(c.components, b.id), bRows);
}

void Phase05Test::rollback()
{
    Context c;
    QVERIFY(c.open());
    Project a;
    QVERIFY(c.projects.create(QStringLiteral("synthetic"), {}, a).ok());
    auto doc = CycloneDxParser::parse(sample()).document;
    QVERIFY(c.components.replaceForProject(a.id, doc).ok());
    const auto before = rows(c.components, a.id);
    QSqlQuery q(c.database.connection());
    // Failure occurs after DELETE and several successful INSERTs; no production test hook.
    QVERIFY(q.exec("CREATE TRIGGER reject_insert BEFORE INSERT ON components WHEN NEW.source_role=1 AND NEW.source_order=2 BEGIN SELECT RAISE(ABORT,'synthetic insert failure'); END"));
    const auto result = c.components.replaceForProject(a.id, doc);
    QCOMPARE(result.error, ComponentError::Database);
    QVERIFY(!result.diagnostic.isEmpty());
    QVERIFY(!result.userMessage().contains(result.diagnostic));
    QCOMPARE(rows(c.components, a.id), before);
    QVERIFY(q.exec("DROP TRIGGER reject_insert"));
    QVERIFY(c.components.replaceForProject(a.id, doc).ok());
}

void Phase05Test::repositoryErrors()
{
    Context c;
    QVERIFY(c.open());
    Project a;
    QVERIFY(c.projects.create(QStringLiteral("synthetic"), {}, a).ok());
    const auto doc = CycloneDxParser::parse(sample()).document;
    QVERIFY(c.components.replaceForProject(a.id, doc).ok());
    const auto before = rows(c.components, a.id);
    QCOMPARE(c.components.replaceForProject(QStringLiteral("' OR 1=1 --"), doc).error, ComponentError::ProjectNotFound);
    QCOMPARE(c.components.replaceForProject(QStringLiteral("missing"), {}).error, ComponentError::ProjectNotFound);
    QCOMPARE(rows(c.components, a.id), before);
    {
        QSqlQuery q(c.database.connection());
        QVERIFY(q.exec("PRAGMA query_only=ON"));
        QCOMPARE(c.components.replaceForProject(a.id, doc).error, ComponentError::Database);
        QCOMPARE(rows(c.components, a.id), before);
    }
    c.database.close();
    QList<Component> output = before;
    QCOMPARE(c.components.listForProject(a.id, output).error, ComponentError::Database);
    QVERIFY(output.isEmpty());
    QCOMPARE(c.components.replaceForProject(a.id, doc).error, ComponentError::Database);
    QVERIFY(!QSqlDatabase::contains());
}

void Phase05Test::previewOnly()
{
    Context c;
    QVERIFY(c.open());
    Project a;
    QVERIFY(c.projects.create(QStringLiteral("synthetic"), {}, a).ok());
    QVERIFY(c.components.replaceForProject(a.id, CycloneDxParser::parse(sample()).document).ok());
    const auto before = rows(c.components, a.id);
    const auto file = c.temporary.filePath(QStringLiteral("preview.json"));
    QVERIFY(writeFile(file, R"({"bomFormat":"CycloneDX","specVersion":"1.6","components":[{}]})"));
    {
        SbomImportDialog dialog(a.id,a.name,c.file(),c.logger);
        QSignalSpy finished(&dialog, &SbomImportDialog::importFinished);
        QVERIFY(!dialog.findChild<QPushButton*>(QStringLiteral("applySbom"))->isEnabled());
        dialog.importFile(file);
        QTRY_COMPARE(finished.count(), 1);
        QVERIFY(finished.first()[0].toBool());
        QCOMPARE(rows(c.components, a.id), before);
        dialog.reject();
    }
    QCOMPARE(rows(c.components, a.id), before);
}

void Phase05Test::explicitApply()
{
    Context c;
    QVERIFY(c.open());
    Project a;
    QVERIFY(c.projects.create(QStringLiteral("synthetic"), {}, a).ok());
    const auto file = c.temporary.filePath(QStringLiteral("input.json"));
    QVERIFY(writeFile(file, R"({"bomFormat":"CycloneDX","specVersion":"1.6","components":[{},{}]})"));
    SbomImportDialog dialog(a.id,a.name,c.file(),c.logger);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    QSignalSpy parsed(&dialog, &SbomImportDialog::importFinished);
    QSignalSpy applied(&dialog, &SbomImportDialog::applyFinished);
    auto* apply = dialog.findChild<QPushButton*>(QStringLiteral("applySbom"));
    dialog.importFile(file);
    QTRY_COMPARE(parsed.count(), 1);
    QVERIFY(dialog.findChild<QLabel*>(QStringLiteral("sbomStatus"))->text().contains(QStringLiteral("存在质量 Error")));
    QVERIFY(apply->isEnabled());
    QTest::mouseClick(apply, Qt::LeftButton);
    QVERIFY(!apply->isEnabled());
    QCOMPARE(apply->text(), QStringLiteral("正在应用…"));
    QTest::mouseDClick(apply, Qt::LeftButton);
    dialog.applyToProject(); // The operation itself also guards reentrancy.
    dialog.importFile(file);
    QTRY_COMPARE(applied.count(), 1);
    QVERIFY(applied.first()[0].toBool());
    QCOMPARE(parsed.count(), 1);
    const auto persisted = rows(c.components, a.id);
    QCOMPARE(persisted.size(), 2);
    QVERIFY(!apply->isEnabled());
    QCOMPARE(apply->text(), QStringLiteral("已应用"));
    dialog.applyToProject(); apply->click();
    QCOMPARE(rows(c.components, a.id), persisted);
    QCOMPARE(applied.count(), 1);
    // A new successful empty preview is a new candidate and clears the current set.
    QVERIFY(writeFile(file, R"({"bomFormat":"CycloneDX","specVersion":"1.6","components":[]})"));
    dialog.importFile(file);
    QTRY_COMPARE(parsed.count(), 2);
    QVERIFY(apply->isEnabled());
    apply->click();
    QTRY_COMPARE(applied.count(), 2);
    QVERIFY(applied.last()[0].toBool());
    QVERIFY(rows(c.components, a.id).isEmpty());
}

void Phase05Test::parserFailures_data()
{
    QTest::addColumn<QByteArray>("bad");
    QTest::addColumn<bool>("missingFile");
    QTest::newRow("invalid-json") << QByteArray("{") << false;
    QTest::newRow("not-cyclonedx") << QByteArray("{}") << false;
    QTest::newRow("missing-spec") << QByteArray(R"({"bomFormat":"CycloneDX"})") << false;
    QTest::newRow("unsupported") << QByteArray(R"({"bomFormat":"CycloneDX","specVersion":"2.0"})") << false;
    QTest::newRow("structure") << QByteArray(R"({"bomFormat":"CycloneDX","specVersion":"1.6","components":{}})") << false;
    QTest::newRow("file-open") << QByteArray() << true;
}

void Phase05Test::parserFailures()
{
    QFETCH(QByteArray, bad);
    QFETCH(bool, missingFile);
    Context c;
    QVERIFY(c.open());
    Project a;
    QVERIFY(c.projects.create(QStringLiteral("synthetic"), {}, a).ok());
    QVERIFY(c.components.replaceForProject(a.id, CycloneDxParser::parse(sample()).document).ok());
    const auto before = rows(c.components, a.id);
    const auto good = c.temporary.filePath(QStringLiteral("good.json"));
    const auto file = c.temporary.filePath(QStringLiteral("bad.json"));
    QVERIFY(writeFile(good, sample()));
    if (!missingFile) QVERIFY(writeFile(file, bad));
    SbomImportDialog dialog(a.id,a.name,c.file(),c.logger);
    QSignalSpy parsed(&dialog, &SbomImportDialog::importFinished);
    QSignalSpy applied(&dialog, &SbomImportDialog::applyFinished);
    dialog.importFile(good);
    QTRY_COMPARE(parsed.count(), 1);
    dialog.applyToProject();
    QTRY_COMPARE(applied.count(), 1);
    QVERIFY(applied.first()[0].toBool());
    const auto appliedRows = rows(c.components, a.id);
    QVERIFY(appliedRows != before);
    dialog.importFile(file);
    QTRY_COMPARE(parsed.count(), 2);
    QVERIFY(!parsed.last()[0].toBool());
    QCOMPARE(rows(c.components, a.id), appliedRows);
    QCOMPARE(dialog.findChild<QTableView*>(QStringLiteral("sbomComponents"))->model()->rowCount(), 4);
    QVERIFY(!dialog.findChild<QPushButton*>(QStringLiteral("applySbom"))->isEnabled());
    dialog.applyToProject();
    QCOMPARE(applied.count(), 1);
}

void Phase05Test::applyFailureRetry()
{
    Context c;
    QVERIFY(c.open());
    Project a;
    QVERIFY(c.projects.create(QStringLiteral("synthetic"), {}, a).ok());
    const auto doc = CycloneDxParser::parse(sample()).document;
    QVERIFY(c.components.replaceForProject(a.id, doc).ok());
    const auto before = rows(c.components, a.id);
    const auto file = c.temporary.filePath(QStringLiteral("candidate.json"));
    QVERIFY(writeFile(file, sample()));
    SbomImportDialog dialog(a.id,a.name,c.file(),c.logger);
    QSignalSpy parsed(&dialog, &SbomImportDialog::importFinished);
    QSignalSpy applied(&dialog, &SbomImportDialog::applyFinished);
    dialog.importFile(file);
    QTRY_COMPARE(parsed.count(), 1);
    {
        QSqlQuery q(c.database.connection());
        QVERIFY(q.exec("CREATE TRIGGER reject_insert BEFORE INSERT ON components WHEN NEW.source_order=2 BEGIN SELECT RAISE(ABORT,'synthetic sensitive sentinel'); END"));
    }
    dialog.applyToProject();
    QTRY_COMPARE(applied.count(), 1);
    QVERIFY(!applied.first()[0].toBool());
    QCOMPARE(rows(c.components, a.id), before);
    QVERIFY(dialog.findChild<QPushButton*>(QStringLiteral("applySbom"))->isEnabled());
    QVERIFY(!dialog.findChild<QLabel*>(QStringLiteral("sbomStatus"))->text().contains(QStringLiteral("sentinel")));
    QCOMPARE(dialog.findChild<QTableView*>(QStringLiteral("sbomComponents"))->model()->rowCount(), 4);
    {
        QSqlQuery q(c.database.connection()); QVERIFY(q.exec("DROP TRIGGER reject_insert"));
    }
    dialog.applyToProject();
    QTRY_COMPARE(applied.count(), 2);
    QVERIFY(applied.last()[0].toBool());
    QVERIFY(rows(c.components, a.id) != before);
    QFile log(c.temporary.filePath(QStringLiteral("test.log")));
    QVERIFY(log.open(QIODevice::ReadOnly));
    QVERIFY(!log.readAll().contains("sentinel"));
    // Project can disappear between preview and Apply; no orphan rows can be created.
    dialog.importFile(file);
    QTRY_COMPARE(parsed.count(), 2);
    QVERIFY(c.projects.remove(a.id).ok());
    dialog.applyToProject();
    QTRY_COMPARE(applied.count(), 3);
    QVERIFY(!applied.last()[0].toBool());
    QVERIFY(rows(c.components, a.id).isEmpty());
    QVERIFY(dialog.findChild<QLabel*>(QStringLiteral("sbomStatus"))->text().contains(QStringLiteral("项目已不存在")));
}

void Phase05Test::largeReplacement()
{
    Context c;
    QVERIFY2(c.open(), qPrintable(c.error));
    Project a;
    QVERIFY(c.projects.create(QStringLiteral("synthetic-large"), {}, a).ok());
    const auto document = largeDocument();
    QVERIFY(c.components.replaceForProject(a.id, document).ok());
    QList<Component> before;
    QVERIFY(c.components.listForProject(a.id, before).ok());
    QCOMPARE(before.size(), 100000);
    QElapsedTimer timer;
    timer.start();
    const auto result = c.components.replaceForProject(a.id, document);
    const auto ms = timer.elapsed();
    QVERIFY2(result.ok(), qPrintable(result.diagnostic));
    qInfo() << "100000 components: BEGIN + project check + DELETE 100000 + INSERT 100000 + COMMIT ms=" << ms;
    QVERIFY(ms < 20000);
    QList<Component> after;
    QVERIFY(c.components.listForProject(a.id, after).ok());
    QCOMPARE(after.size(), 100000);
    QSet<QString> ids;
    for (int i = 0; i < after.size(); ++i) {
        QCOMPARE(after[i].sourceOrder, i);
        QCOMPARE(after[i].name, document.components[i].name);
        QVERIFY(!QUuid(after[i].id).isNull());
        QVERIFY(after[i].id != before[i].id);
        ids.insert(after[i].id);
    }
    QCOMPARE(ids.size(), 100000);
}

void Phase05Test::projectUi()
{
    const bool native = QCoreApplication::testAttribute(Qt::AA_DontUseNativeDialogs);
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    const auto restore = qScopeGuard([&] { QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs, native); });
    Context c;
    QVERIFY(c.open());
    Project a,b;
    QVERIFY(c.projects.create(QStringLiteral("synthetic A"), {}, a).ok());
    QVERIFY(c.projects.create(QStringLiteral("synthetic B"), {}, b).ok());
    const auto file = c.temporary.filePath(QStringLiteral("candidate.json"));
    QVERIFY(writeFile(file, sample()));
    {
        MainWindow window(new ProjectPage(c.projects,c.components,c.logger,c.temporary.filePath("cache/osv-v1")),nullptr,QStringLiteral("projects"));
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* list = window.findChild<QListWidget*>(QStringLiteral("projectList"));
        const auto select = [&](const QString& id) {
            for (int i = 0; i < list->count(); ++i) if (list->item(i)->data(Qt::UserRole).toString() == id) list->setCurrentRow(i);
        };
        select(a.id);
        auto* table = window.findChild<QTableView*>(QStringLiteral("currentComponents"));
        QCOMPARE(table->model()->rowCount(), 0);
        window.findChild<QPushButton*>(QStringLiteral("importSbom"))->click();
        QPointer<SbomImportDialog> dialog = window.findChild<SbomImportDialog*>();
        QVERIFY(dialog);
        QPointer<QFileDialog> picker = dialog->findChild<QFileDialog*>();
        QVERIFY(picker); picker->reject(); QTRY_VERIFY(picker.isNull());
        QSignalSpy parsed(dialog, &SbomImportDialog::importFinished);
        QSignalSpy applied(dialog, &SbomImportDialog::applyFinished);
        dialog->importFile(file);
        QTRY_COMPARE(parsed.count(), 1);
        QCOMPARE(table->model()->rowCount(), 0);
        dialog->findChild<QPushButton*>(QStringLiteral("applySbom"))->click();
        QTRY_COMPARE(applied.count(), 1);
        QVERIFY(applied.first()[0].toBool());
        QCOMPARE(table->model()->rowCount(), 5);
        QCOMPARE(table->model()->columnCount(), 6);
        QCOMPARE(table->editTriggers(), QAbstractItemView::NoEditTriggers);
        QCOMPARE(table->model()->data(table->model()->index(0,0)).toString(), QStringLiteral("元数据根组件"));
        QCOMPARE(table->model()->data(table->model()->index(0,1)).toString(), QStringLiteral("synthetic-root"));
        dialog->reject(); QTRY_VERIFY(dialog.isNull());
        select(b.id); QCOMPARE(table->model()->rowCount(), 0);
        select(a.id); QCOMPARE(table->model()->rowCount(), 5);
        window.findChild<QTabWidget*>(QStringLiteral("projectTabs"))->setCurrentIndex(1);
        QVERIFY(window.grab().save(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("phase05-components.png"))));
        window.resize(680,420);
        QTRY_COMPARE(window.size(), QSize(680,420));
        QVERIFY(table->viewport()->height() > 20);
        QVERIFY(window.grab().save(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("phase05-small.png"))));
    }
    c.database.close();
    QVERIFY(c.database.open(c.file(), c.error));
    MainWindow reopened(new ProjectPage(c.projects,c.components,c.logger,c.temporary.filePath("cache/osv-v1")),nullptr,QStringLiteral("projects"));
    auto* list = reopened.findChild<QListWidget*>(QStringLiteral("projectList"));
    for (int i = 0; i < list->count(); ++i) if (list->item(i)->data(Qt::UserRole).toString() == a.id) list->setCurrentRow(i);
    QCOMPARE(reopened.findChild<QTableView*>(QStringLiteral("currentComponents"))->model()->rowCount(),5);
    QVERIFY(c.projects.remove(a.id).ok());
    reopened.findChild<QPushButton*>(QStringLiteral("refreshProjects"))->click();
    QCOMPARE(reopened.findChild<QTableView*>(QStringLiteral("currentComponents"))->model()->rowCount(),0);
    QCOMPARE(list->count(),1);
    QCOMPARE(list->item(0)->data(Qt::UserRole).toString(),b.id);
}

void Phase05Test::largeApply()
{
    Context c;
    QVERIFY(c.open());
    Project a;
    QVERIFY(c.projects.create(QStringLiteral("synthetic large"),{},a).ok());
    const auto document = largeDocument();
    QVERIFY(c.components.replaceForProject(a.id,document).ok());
    QJsonArray array;
    for (const auto& component : document.components)
        array.append(QJsonObject{{"bom-ref",component.bomRef},{"type",component.type},{"name",component.name},
            {"version",component.version},{"purl",component.purl}});
    const auto bytes = QJsonDocument(QJsonObject{{"bomFormat","CycloneDX"},{"specVersion","1.6"},{"components",array}}).toJson(QJsonDocument::Compact);
    const auto file = c.temporary.filePath(QStringLiteral("large.json"));
    QVERIFY(writeFile(file,bytes));
    SbomImportDialog dialog(a.id,a.name,c.file(),c.logger);
    dialog.show(); QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    QSignalSpy parsed(&dialog,&SbomImportDialog::importFinished);
    QSignalSpy applied(&dialog,&SbomImportDialog::applyFinished);
    dialog.importFile(file); QTRY_COMPARE_WITH_TIMEOUT(parsed.count(),1,15000);
    QVERIFY(parsed.first()[0].toBool());
    QElapsedTimer timer; timer.start();
    dialog.applyToProject();
    const auto dispatchMs = timer.elapsed();
    QVERIFY2(dispatchMs < 500,"Apply dispatch blocked the GUI thread");
    auto* apply = dialog.findChild<QPushButton*>(QStringLiteral("applySbom"));
    QVERIFY(!apply->isEnabled());
    QVERIFY(!dialog.findChild<QPushButton*>(QStringLiteral("chooseSbomFile"))->isEnabled());
    dialog.reject(); QVERIFY(dialog.isVisible());
    QVERIFY(!dialog.close());
    dialog.applyToProject(); // Repeated request while working is ignored.
    bool deliveredWhileWorking = false;
    QMetaObject::invokeMethod(&dialog,[&] {
        deliveredWhileWorking = applied.isEmpty() && !apply->isEnabled();
        dialog.resize(800,550); // GUI can handle events while real SQLite replacement is running.
    },Qt::QueuedConnection);
    QTRY_VERIFY(deliveredWhileWorking);
    int wakeups = 0;
    qint64 maximumGap = 0;
    qint64 previous = timer.elapsed();
    const auto connection = connect(QAbstractEventDispatcher::instance(),&QAbstractEventDispatcher::awake,&dialog,[&] {
        const auto now = timer.elapsed(); maximumGap = qMax(maximumGap,now-previous); previous=now; ++wakeups;
    });
    QTRY_COMPARE_WITH_TIMEOUT(applied.count(),1,20000);
    disconnect(connection);
    QVERIFY(applied.first()[0].toBool());
    QVERIFY(wakeups > 1);
    QVERIFY2(maximumGap < 1000,"GUI event dispatch was stalled during Apply");
    qInfo() << "100000 GUI Apply: dispatch ms=" << dispatchMs << "completion ms=" << timer.elapsed()
        << "event-loop wakeups=" << wakeups << "max observed gap ms=" << maximumGap;
    const auto persisted = rows(c.components,a.id);
    QCOMPARE(persisted.size(),100000);
    dialog.applyToProject();
    QCOMPARE(rows(c.components,a.id),persisted);
    // Measure actual persisted-view selection as well, rather than claiming a repository benchmark proves UI performance.
    ProjectPage page(c.projects,c.components,c.logger,c.temporary.filePath("cache/osv-v1"));
    page.show();
    QVERIFY(QTest::qWaitForWindowExposed(&page));
    timer.restart();
    page.findChild<QListWidget*>(QStringLiteral("projectList"))->setCurrentRow(0);
    const auto viewMs = timer.elapsed();
    QCOMPARE(page.findChild<QTableView*>(QStringLiteral("currentComponents"))->model()->rowCount(),100000);
    qInfo() << "100000 persisted component view load ms=" << viewMs;
    QVERIFY(viewMs < 1000);
    const QDir output(QCoreApplication::applicationDirPath());
    QVERIFY(writeFile(output.filePath(QStringLiteral("phase05-large-sbom.json")),bytes));
    QVERIFY(writeFile(output.filePath(QStringLiteral("phase05-replacement-sbom.json")),R"({"bomFormat":"CycloneDX","specVersion":"1.6","components":[{"bom-ref":"replacement-only","type":"library","name":"synthetic-replacement-only","version":"2.0","purl":"pkg:generic/synthetic-replacement-only@2.0"}]})"));
    QVERIFY(writeFile(output.filePath(QStringLiteral("phase05-empty-sbom.json")),R"({"bomFormat":"CycloneDX","specVersion":"1.6","components":[]})"));
}

void Phase05Test::workerLifetime()
{
    Context c;
    QVERIFY(c.open());
    Project a;
    QVERIFY(c.projects.create(QStringLiteral("synthetic"),{},a).ok());
    const auto file = c.temporary.filePath(QStringLiteral("candidate.json"));
    QVERIFY(writeFile(file,sample()));
    const auto connections = QSqlDatabase::connectionNames();
    auto* dialog = new SbomImportDialog(a.id,a.name,c.file(),c.logger);
    QSignalSpy parsed(dialog,&SbomImportDialog::importFinished);
    dialog->importFile(file); QTRY_COMPARE(parsed.count(),1);
    dialog->applyToProject();
    delete dialog; // Forced parent destruction/shutdown: worker cannot touch destroyed UI or logger.
    c.database.close();
    QVERIFY(QThreadPool::globalInstance()->waitForDone(15000));
    QVERIFY(c.database.open(c.file(),c.error));
    QCOMPARE(rows(c.components,a.id).size(),5);
    QCOMPARE(QSqlDatabase::connectionNames(),connections);
}

QTEST_MAIN(Phase05Test)
#include "Phase05Test.moc"
