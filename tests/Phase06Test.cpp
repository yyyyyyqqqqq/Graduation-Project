#include "AppDatabase.h"
#include "AppLogger.h"
#include "ComponentRepository.h"
#include "DependencyAnalyzer.h"
#include "DependencyPage.h"
#include "ProjectRepository.h"
#include "CycloneDxParser.h"
#include "SbomImportDialog.h"
#include "ProjectPage.h"
#include "MainWindow.h"

#include <QAbstractEventDispatcher>
#include <QElapsedTimer>
#include <QFile>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QComboBox>
#include <QFileDialog>
#include <QLabel>
#include <QListWidget>
#include <QPointer>
#include <QPushButton>
#include <QTableView>
#include <QTabWidget>
#include <QTextBrowser>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QSqlQuery>
#include <QSqlError>
#include <QTemporaryDir>
#include <QTest>
#include <QThreadPool>
#include <QUuid>
#include <QtConcurrentRun>
#include <atomic>

namespace {
struct Context {
    QTemporaryDir dir;
    AppLogger logger;
    AppDatabase db;
    ProjectRepository projects{db,logger};
    ComponentRepository current{db};
    QString error;
    QString path() const { return dir.filePath("synthetic.db"); }
    bool open() { return logger.open(dir.filePath("test.log")) && db.open(path(),error); }
    QString project() { Project p; if (!projects.create("synthetic",{},p).ok()) qFatal("Cannot create fixture"); return p.id; }
    DependencySnapshot snapshot(const QString& id) {
        DependencySnapshot s;
        const auto r = current.readSnapshot(id,s);
        if (!r.ok()) qFatal("Cannot read fixture snapshot");
        return s;
    }
};
bool sqlFile(const QString& path, const QStringList& statements)
{
    const auto name = QUuid::createUuid().toString();
    const auto cleanup = qScopeGuard([&] { QSqlDatabase::removeDatabase(name); });
    auto db = QSqlDatabase::addDatabase("QSQLITE",name);
    db.setDatabaseName(path);
    if (!db.open()) return false;
    QSqlQuery q(db);
    for (const auto& sql : statements) if (!q.exec(sql)) return false;
    return true;
}
QVariant scalar(AppDatabase& db, const QString& sql)
{
    QSqlQuery q(db.connection());
    return q.exec(sql) && q.next() ? q.value(0) : QVariant();
}
QVariant inspectFile(const QString& path,const QString& sql)
{
    const auto name=QUuid::createUuid().toString();
    const auto cleanup=qScopeGuard([&]{QSqlDatabase::removeDatabase(name);});
    auto db=QSqlDatabase::addDatabase("QSQLITE",name); db.setDatabaseName(path);
    db.setConnectOptions("QSQLITE_OPEN_READONLY");
    if(!db.open()) return {};
    QSqlQuery q(db); return q.exec(sql) && q.next()?q.value(0):QVariant();
}
bool legacy(const QString& path, int version)
{
    QStringList sql{"CREATE TABLE app_meta(key TEXT PRIMARY KEY NOT NULL,value TEXT NOT NULL)",
        QString("INSERT INTO app_meta VALUES('schema_version','%1'),('preserved','synthetic')").arg(version)};
    if (version >= 2) sql << "CREATE TABLE projects(id TEXT PRIMARY KEY NOT NULL,name TEXT NOT NULL,description TEXT NOT NULL DEFAULT '',created_at INTEGER NOT NULL)"
        << "INSERT INTO projects VALUES('legacy','synthetic Phase 05',' preserved ',12345)";
    if (version >= 3) sql << "CREATE TABLE components(id TEXT PRIMARY KEY NOT NULL,project_id TEXT NOT NULL,source_role INTEGER NOT NULL CHECK(source_role IN(0,1)),source_order INTEGER NOT NULL CHECK(source_order>=0 AND(source_role=1 OR source_order=0)),bom_ref TEXT NOT NULL DEFAULT '',type TEXT NOT NULL DEFAULT '',name TEXT NOT NULL DEFAULT '',version TEXT NOT NULL DEFAULT '',purl TEXT NOT NULL DEFAULT '',FOREIGN KEY(project_id) REFERENCES projects(id) ON DELETE CASCADE)"
        << "CREATE UNIQUE INDEX components_project_source ON components(project_id,source_role,source_order)"
        << "INSERT INTO components VALUES('legacy-component','legacy',1,0,'A','library','synthetic legacy','1.0','pkg:generic/synthetic@1.0')";
    return sqlFile(path,sql);
}
SbomDocument document(const QStringList& refs, const QList<SbomDependency>& deps)
{
    SbomDocument d;
    for (const auto& ref : refs) d.components.append({ref,"library",QStringLiteral("synthetic-")+ref,"1.0",QStringLiteral("pkg:generic/synthetic-")+ref+QStringLiteral("@1.0")});
    d.dependencies=deps;
    return d;
}
DependencySnapshot snapshotOf(const SbomDocument& d)
{
    DependencySnapshot s;
    s.captured = true;
    if (d.metadata.component) {
        const auto& c = *d.metadata.component;
        s.components.append({"root-id","project",ComponentSourceRole::MetadataRoot,0,c.bomRef,c.type,c.name,c.version,c.purl});
    }
    for (int i=0;i<d.components.size();++i) {
        const auto& c=d.components[i];
        s.components.append({QString::number(i),"project",ComponentSourceRole::Component,i,c.bomRef,c.type,c.name,c.version,c.purl});
    }
    for (int i=0;i<d.dependencies.size();++i) {
        DependencyEntry e{QString::number(i),"project",i,d.dependencies[i].ref,{}};
        for (int j=0;j<d.dependencies[i].dependsOn.size();++j)
            e.targets.append({QString("%1-%2").arg(i).arg(j),e.id,j,d.dependencies[i].dependsOn[j]});
        s.entries.append(e);
    }
    return s;
}
SbomDocument largeDocument()
{
    SbomDocument d;
    for (int i=0;i<100000;++i) {
        const auto ref=QString("synthetic-%1").arg(i);
        d.components.append({ref,"library",ref,"1.0",QString("pkg:generic/%1@1.0").arg(ref)});
        d.dependencies.append({ref,{QString("synthetic-%1").arg((i+1)%100000)}});
    }
    return d;
}
QByteArray json(const SbomDocument& d)
{
    const auto comp=[](const SbomComponent& c) { return QJsonObject{{"bom-ref",c.bomRef},{"type",c.type},{"name",c.name},{"version",c.version},{"purl",c.purl}}; };
    QJsonArray components,deps;
    for (const auto& c:d.components) components.append(comp(c));
    for (const auto& e:d.dependencies) deps.append(QJsonObject{{"ref",e.ref},{"dependsOn",QJsonArray::fromStringList(e.dependsOn)}});
    QJsonObject o{{"bomFormat","CycloneDX"},{"specVersion","1.6"},{"components",components},{"dependencies",deps}};
    if (d.metadata.component) o["metadata"]=QJsonObject{{"component",comp(*d.metadata.component)}};
    return QJsonDocument(o).toJson(QJsonDocument::Compact);
}
bool write(const QString& path,const QByteArray& bytes)
{
    QFile f(path); return f.open(QIODevice::WriteOnly) && f.write(bytes)==bytes.size();
}
}

class Phase06Test final : public QObject {
    Q_OBJECT
private slots:
    void schema();
    void migration_data();
    void migration();
    void migrationFailures_data();
    void migrationFailures();
    void captureRaw();
    void atomicReplace();
    void resolution();
    void graphTraversal();
    void metricsSelf();
    void consistentSnapshot();
    void largeData();
    void previewApply();
    void dependencyUi();
    void graphReuse();
    void staleResults();
    void projectIntegration();
    void largeUi();
    void workerLifetime();
    void manualFixtures();
};

void Phase06Test::schema()
{
    Context c; QVERIFY2(c.open(),qPrintable(c.error));
    QCOMPARE(AppDatabase::SchemaVersion,4);
    QCOMPARE(scalar(c.db,"SELECT value FROM app_meta WHERE key='schema_version'").toString(),QString("4"));
    auto tables=c.db.connection().tables(); tables.sort();
    QCOMPARE(tables,(QStringList{"app_meta","components","dependency_capture","dependency_entries","dependency_targets","projects"}));
    QCOMPARE(scalar(c.db,"PRAGMA foreign_keys").toInt(),1);
    QSqlQuery q(c.db.connection());
    for (const auto& table: {QString("dependency_capture"),QString("dependency_entries"),QString("dependency_targets")}) {
        QVERIFY(q.exec("PRAGMA table_info("+table+")"));
        QStringList fields;
        while(q.next()) {fields.append(q.value(1).toString()); QCOMPARE(q.value(3).toInt(),1);}
        QCOMPARE(fields,(table=="dependency_capture" ? QStringList{"project_id"} : table=="dependency_entries"
            ? QStringList{"id","project_id","source_order","source_ref"} : QStringList{"id","dependency_entry_id","target_order","target_ref"}));
        QVERIFY(q.exec("PRAGMA foreign_key_list("+table+")")); QVERIFY(q.next());
        QCOMPARE(q.value(2).toString(),table=="dependency_targets" ? QString("dependency_entries") : QString("projects"));
        QCOMPARE(q.value(6).toString(),QString("CASCADE")); QVERIFY(!q.next());
    }
    for (const auto& pair: {qMakePair(QString("dependency_entries"),QString("project_id")),qMakePair(QString("dependency_targets"),QString("dependency_entry_id"))}) {
        QVERIFY(q.exec("PRAGMA index_list("+pair.first+")")); bool unique=false;
        const auto index=pair.first=="dependency_entries" ? QString("dependency_entries_project_order") : QString("dependency_targets_entry_order");
        while(q.next()) if(q.value(1).toString()==index) {unique=true; QCOMPARE(q.value(2).toInt(),1);}
        QVERIFY(unique); QVERIFY(q.exec("PRAGMA index_info("+index+")")); QVERIFY(q.next()); QCOMPARE(q.value(2).toString(),pair.second);
        QVERIFY(q.next()); QCOMPARE(q.value(2).toString(),pair.first=="dependency_entries" ? QString("source_order") : QString("target_order")); QVERIFY(!q.next());
    }
    QVERIFY(!q.exec("INSERT INTO dependency_capture VALUES('missing')"));
    QVERIFY(!q.exec("INSERT INTO dependency_entries VALUES('e','missing',0,'A')"));
    QVERIFY(!q.exec("INSERT INTO dependency_targets VALUES('t','missing',0,'A')"));
    QVERIFY(q.exec("INSERT INTO projects VALUES('p','synthetic','',0)"));
    QVERIFY(!q.exec("INSERT INTO dependency_entries VALUES('e','p',-1,'A')"));
    QVERIFY(q.exec("INSERT INTO dependency_entries VALUES('e','p',0,'A')"));
    QVERIFY(!q.exec("INSERT INTO dependency_entries VALUES('e2','p',0,'B')"));
    QVERIFY(!q.exec("INSERT INTO dependency_targets VALUES('t','e',-1,'A')"));
    QVERIFY(q.exec("INSERT INTO dependency_targets VALUES('t','e',0,'A')"));
    QVERIFY(!q.exec("INSERT INTO dependency_targets VALUES('t2','e',0,'B')"));
}

void Phase06Test::migration_data() { QTest::addColumn<int>("version"); for(int n=1;n<=3;++n) QTest::newRow(qPrintable(QString::number(n)))<<n; }
void Phase06Test::migration()
{
    QFETCH(int,version); Context c; QVERIFY(legacy(c.path(),version)); QVERIFY2(c.open(),qPrintable(c.error));
    QCOMPARE(scalar(c.db,"SELECT value FROM app_meta WHERE key='schema_version'").toString(),QString("4"));
    QCOMPARE(scalar(c.db,"SELECT value FROM app_meta WHERE key='preserved'").toString(),QString("synthetic"));
    if(version>=2) {
        Project p; QVERIFY(c.projects.findById("legacy",p).ok()); QCOMPARE(p.description,QString(" preserved ")); QCOMPARE(p.createdAt,12345);
        const auto s=c.snapshot("legacy"); QVERIFY(!s.captured); QVERIFY(s.entries.isEmpty());
        QCOMPARE(s.components.size(),version==3 ? 1 : 0);
        if(version==3) QCOMPARE(s.components[0],(Component{"legacy-component","legacy",ComponentSourceRole::Component,0,"A","library","synthetic legacy","1.0","pkg:generic/synthetic@1.0"}));
        c.db.close(); QVERIFY(c.db.open(c.path(),c.error)); QCOMPARE(c.snapshot("legacy"),s);
    }
    Context failed;
    QVERIFY(legacy(failed.path(),version));
    QVERIFY(sqlFile(failed.path(),{"CREATE TRIGGER reject_v4 BEFORE UPDATE ON app_meta WHEN NEW.value='4' BEGIN SELECT RAISE(ABORT,'synthetic'); END"}));
    QVERIFY(!failed.open());
    QCOMPARE(inspectFile(failed.path(),"SELECT value FROM app_meta WHERE key='schema_version'").toString(),QString::number(version));
    QCOMPARE(inspectFile(failed.path(),"SELECT count(*) FROM sqlite_master WHERE name LIKE 'dependency_%'").toInt(),0);
    QCOMPARE(inspectFile(failed.path(),"SELECT value FROM app_meta WHERE key='preserved'").toString(),QString("synthetic"));
    if(version==1) QCOMPARE(inspectFile(failed.path(),"SELECT count(*) FROM sqlite_master WHERE name IN('projects','components','components_project_source')").toInt(),0);
    else QCOMPARE(inspectFile(failed.path(),"SELECT description FROM projects WHERE id='legacy'").toString(),QString(" preserved "));
    if(version==2) QCOMPARE(inspectFile(failed.path(),"SELECT count(*) FROM sqlite_master WHERE name IN('components','components_project_source')").toInt(),0);
    if(version==3) QCOMPARE(inspectFile(failed.path(),"SELECT id FROM components").toString(),QString("legacy-component"));
}

void Phase06Test::migrationFailures_data()
{
    QTest::addColumn<QStringList>("fault");
    QTest::newRow("table")<<QStringList{"CREATE TABLE dependency_targets(payload TEXT)","INSERT INTO dependency_targets VALUES('synthetic preserved')"};
    QTest::newRow("index")<<QStringList{"CREATE INDEX dependency_targets_entry_order ON projects(name)"};
    QTest::newRow("version")<<QStringList{"CREATE TRIGGER fail BEFORE UPDATE ON app_meta WHEN NEW.value='4' BEGIN SELECT RAISE(ABORT,'synthetic failure'); END"};
    QTest::newRow("future")<<QStringList{"UPDATE app_meta SET value='99' WHERE key='schema_version'"};
}
void Phase06Test::migrationFailures()
{
    QFETCH(QStringList,fault); Context c; QVERIFY(legacy(c.path(),3)); QVERIFY(sqlFile(c.path(),fault)); QVERIFY(!c.open());
    const auto name=QUuid::createUuid().toString(); const auto cleanup=qScopeGuard([&]{QSqlDatabase::removeDatabase(name);});
    auto db=QSqlDatabase::addDatabase("QSQLITE",name); db.setDatabaseName(c.path()); QVERIFY(db.open()); QSqlQuery q(db);
    QVERIFY(q.exec("SELECT value FROM app_meta WHERE key='schema_version'")); QVERIFY(q.next()); QCOMPARE(q.value(0).toString(),fault.first().startsWith("UPDATE")?QString("99"):QString("3"));
    QVERIFY(q.exec("SELECT id,name FROM components")); QVERIFY(q.next()); QCOMPARE(q.value(0).toString(),QString("legacy-component")); QCOMPARE(q.value(1).toString(),QString("synthetic legacy"));
    QVERIFY(q.exec("SELECT count(*) FROM sqlite_master WHERE name IN('dependency_capture','dependency_entries','dependency_entries_project_order')")); QVERIFY(q.next()); QCOMPARE(q.value(0).toInt(),0);
    if(fault.first().startsWith("CREATE TABLE")) {QVERIFY(q.exec("SELECT payload FROM dependency_targets")); QVERIFY(q.next()); QCOMPARE(q.value(0).toString(),QString("synthetic preserved"));}
}

void Phase06Test::captureRaw()
{
    Context c; QVERIFY(c.open()); const auto a=c.project(),b=c.project(); QVERIFY(!c.snapshot(a).captured);
    const auto longRef=QString(10000,u'长');
    const auto d=document({"A"," A ","根🙂"},{{"A",{}},{"",{""," \t\n"," A ","根🙂",longRef}},{"A",{"A","A"}},{"A",{}}});
    QVERIFY(c.current.replaceForProject(a,d).ok()); const auto s=c.snapshot(a); QVERIFY(s.captured); QCOMPARE(s.entries.size(),4);
    for(int i=0;i<d.dependencies.size();++i) {
        const auto& e=s.entries[i]; QCOMPARE(e.sourceOrder,i); QCOMPARE(e.projectId,a); QCOMPARE(e.sourceRef,d.dependencies[i].ref); QVERIFY(!QUuid(e.id).isNull());
        QCOMPARE(e.targets.size(),d.dependencies[i].dependsOn.size());
        for(int j=0;j<e.targets.size();++j) { QCOMPARE(e.targets[j].targetOrder,j); QCOMPARE(e.targets[j].dependencyEntryId,e.id); QCOMPARE(e.targets[j].targetRef,d.dependencies[i].dependsOn[j]); QVERIFY(!QUuid(e.targets[j].id).isNull()); }
    }
    c.db.close(); QVERIFY(c.db.open(c.path(),c.error)); QCOMPARE(c.snapshot(a),s); QVERIFY(!c.snapshot(b).captured);
    QVERIFY(c.current.replaceForProject(a,document({"A"},{})).ok()); const auto empty=c.snapshot(a); QVERIFY(empty.captured); QVERIFY(empty.entries.isEmpty()); QCOMPARE(empty.components.size(),1);
    QVERIFY(c.current.replaceForProject(b,d).ok()); const auto other=c.snapshot(b);
    QVERIFY(c.projects.remove(a).ok()); QCOMPARE(c.snapshot(b),other);
    QCOMPARE(scalar(c.db,"SELECT count(*) FROM dependency_capture").toInt(),1);
    QCOMPARE(scalar(c.db,"SELECT count(*) FROM dependency_entries").toInt(),4);
    QCOMPARE(scalar(c.db,"SELECT count(*) FROM dependency_targets").toInt(),7);
    QVERIFY(c.projects.remove(b).ok());
    for(const auto& table: {"components","dependency_capture","dependency_entries","dependency_targets"}) QCOMPARE(scalar(c.db,QString("SELECT count(*) FROM %1").arg(table)).toInt(),0);
}

void Phase06Test::atomicReplace()
{
    Context c; QVERIFY(c.open()); const auto id=c.project();
    const auto a=document({"A","B"},{{"A",{"B"}}});
    const auto b=document({"X","Y"},{{"X",{"Y","X"}}});
    for(bool captured:{false,true}) {
        if(captured) QVERIFY(c.current.replaceForProject(id,a).ok());
        const auto before=c.snapshot(id);
        for(const auto& fault: {QString("CREATE TRIGGER reject BEFORE INSERT ON dependency_targets WHEN NEW.target_order=1 BEGIN SELECT RAISE(ABORT,'synthetic'); END"),
                               QString("CREATE TRIGGER reject BEFORE INSERT ON dependency_capture BEGIN SELECT RAISE(ABORT,'synthetic'); END")}) {
            QSqlQuery q(c.db.connection()); QVERIFY(q.exec(fault));
            QCOMPARE(c.current.replaceForProject(id,b).error,ComponentError::Database);
            QCOMPARE(c.snapshot(id),before); QVERIFY(q.exec("DROP TRIGGER reject"));
        }
    }
    const auto old=c.snapshot(id); QVERIFY(c.current.replaceForProject(id,b).ok()); const auto now=c.snapshot(id);
    QCOMPARE(now.components.size(),2); QCOMPARE(now.components[0].bomRef,QString("X")); QVERIFY(now.components[0].id!=old.components[0].id);
    QCOMPARE(now.entries.size(),1); QCOMPARE(now.entries[0].sourceRef,QString("X")); QCOMPARE(now.entries[0].targets.size(),2); QVERIFY(now.captured);
}

void Phase06Test::resolution()
{
    auto d=document({"A","A"," B ","B",""," \t"},{{"root",{"A"," B ","B","b",""," \n","pkg:generic/synthetic-B@1.0","synthetic-B"}},{"missing",{"B"}}});
    d.metadata.component=SbomComponent{"root","application","synthetic-root","1.0",""};
    const auto s=snapshotOf(d); const auto before=s; const auto g=DependencyGraph::build(s); QCOMPARE(s,before);
    const auto& e=g.resolutions()[0]; QCOMPARE(e.source,(ReferenceResolution{ReferenceState::Resolved,0}));
    QCOMPARE(e.targets[0].state,ReferenceState::Ambiguous);
    QCOMPARE(e.targets[1],(ReferenceResolution{ReferenceState::Resolved,3}));
    QCOMPARE(e.targets[2],(ReferenceResolution{ReferenceState::Resolved,4}));
    for(int i:{3,6,7}) QCOMPARE(e.targets[i].state,ReferenceState::Unknown);
    for(int i:{4,5}) QCOMPARE(e.targets[i].state,ReferenceState::Missing);
    QCOMPARE(g.resolutions()[1].source.state,ReferenceState::Unknown);
    QCOMPARE(g.metrics().uniqueEdges,2);
}

void Phase06Test::graphTraversal()
{
    auto d=document({"A","B","C","D","E"},{{"A",{"B","C","B"}},{"C",{"D"}},{"B",{"E"}},{"D",{"A"}}});
    const auto g=DependencyGraph::build(snapshotOf(d));
    QCOMPARE(g.direct(0),(QList<int>{1,2})); QCOMPARE(g.direct(0,true),(QList<int>{3}));
    const QList<DependencyReach> closure{{1,1},{2,1},{4,2},{3,2}};
    QCOMPARE(g.transitive(0),closure); QCOMPARE(g.transitive(0,true),(QList<DependencyReach>{{3,1},{2,2}}));
    QCOMPARE(g.metrics().uniqueEdges,5); QCOMPARE(g.metrics().targets,6);
    for(int n=0;n<10;++n) { const auto again=DependencyGraph::build(snapshotOf(d)); QCOMPARE(again.transitive(0),closure); QCOMPARE(again.direct(0),g.direct(0)); }
    d.dependencies={{"A",{"C","B","C"}},{"D",{"B"}},{"C",{"B"}}};
    const auto ordered=DependencyGraph::build(snapshotOf(d)); QCOMPARE(ordered.direct(0),(QList<int>{2,1})); QCOMPARE(ordered.direct(1,true),(QList<int>{0,3,2}));
    QVERIFY(g.transitive(-1).isEmpty()); QVERIFY(g.direct(999).isEmpty());
}

void Phase06Test::metricsSelf()
{
    auto d=document({"A"},{{"A",{"A","A"}},{"unknown",{"unknown","unknown"}},{" ",{ "", " \t"}}});
    auto g=DependencyGraph::build(snapshotOf(d)); QCOMPARE(g.metrics(),(DependencyMetrics{3,6,3,3,0,4,1}));
    QCOMPARE(g.direct(0),(QList<int>{0})); QCOMPARE(g.direct(0,true),(QList<int>{0})); QVERIFY(g.transitive(0).isEmpty()); QVERIFY(g.transitive(0,true).isEmpty());
    d.components.append(d.components[0]); g=DependencyGraph::build(snapshotOf(d));
    QCOMPARE(g.metrics(),(DependencyMetrics{3,6,3,3,3,4,0}));
    QVERIFY(g.direct(0).isEmpty());
}

void Phase06Test::consistentSnapshot()
{
    Context c; QVERIFY(c.open()); const auto id=c.project();
    const auto a=document({"A","B"},{{"A",{"B"}}}), b=document({"X","Y"},{{"X",{"Y"}}});
    QVERIFY(c.current.replaceForProject(id,a).ok());
    // WAL here permits a writer to commit while a reader retains its old snapshot.
    { QSqlQuery q(c.db.connection()); QVERIFY(q.exec("PRAGMA journal_mode=WAL")); }
    auto writer=QtConcurrent::run([path=c.path(),id,a,b] {
        for(int n=0;n<40;++n) if(!ComponentRepository::replaceInFile(path,id,n%2? a:b).ok()) return false;
        return true;
    });
    for(int n=0;n<80;++n) {
        DependencySnapshot s; const auto r=c.current.readSnapshot(id,s); QVERIFY2(r.ok(),qPrintable(r.diagnostic));
        QVERIFY(s.captured); QCOMPARE(s.components.size(),2); QCOMPARE(s.entries.size(),1);
        QCOMPARE(s.components[0].bomRef,s.entries[0].sourceRef); QCOMPARE(s.components[1].bomRef,s.entries[0].targets[0].targetRef);
    }
    writer.waitForFinished(); QVERIFY(writer.result());
    // Actual SQLite read transaction behavior, including an intervening successful Apply.
    QVERIFY(c.current.replaceForProject(id,a).ok());
    auto db=c.db.connection(); QVERIFY(db.transaction());
    QCOMPARE(scalar(c.db,"SELECT bom_ref FROM components ORDER BY source_order LIMIT 1").toString(),QString("A"));
    auto replace=QtConcurrent::run([path=c.path(),id,b]{return ComponentRepository::replaceInFile(path,id,b);});
    replace.waitForFinished(); QVERIFY(replace.result().ok());
    QCOMPARE(scalar(c.db,"SELECT source_ref FROM dependency_entries").toString(),QString("A")); QVERIFY(db.commit());
    QCOMPARE(c.snapshot(id).entries[0].sourceRef,QString("X"));
}

void Phase06Test::largeData()
{
    Context c; QVERIFY(c.open()); const auto id=c.project(); const auto d=largeDocument();
    QElapsedTimer timer; timer.start(); const auto applied=c.current.replaceForProject(id,d); const auto persistMs=timer.elapsed(); QVERIFY2(applied.ok(),qPrintable(applied.diagnostic));
    timer.restart(); auto s=c.snapshot(id); const auto loadMs=timer.elapsed();
    QCOMPARE(s.components.size(),100000); QCOMPARE(s.entries.size(),100000);
    DependencyBuildTimings timings; timer.restart(); const auto g=DependencyGraph::build(std::move(s),&timings); const auto graphMs=timer.elapsed();
    QCOMPARE(g.metrics().targets,100000); QCOMPARE(g.metrics().uniqueEdges,100000);
    timer.restart(); const auto reach=g.transitive(0),reverse=g.transitive(0,true); const auto bfsMs=timer.elapsed();
    QCOMPARE(reach.size(),99999); QCOMPARE(reverse.size(),99999); QCOMPARE(reach.last(),(DependencyReach{99999,99999})); QCOMPARE(reverse.last(),(DependencyReach{1,99999}));
    qInfo()<<"100000 components/entries/targets persistence ms="<<persistMs<<"snapshot ms="<<loadMs<<"graph total ms="<<graphMs
        <<"ref index ms="<<timings.indexNs/1000000.0<<"resolution+metrics ms="<<timings.resolutionNs/1000000.0
        <<"forward+reverse adjacency ms="<<timings.adjacencyNs/1000000.0<<"two BFS ms="<<bfsMs;
    QVERIFY(persistMs<20000); QVERIFY(loadMs<10000); QVERIFY(graphMs<10000); QVERIFY(bfsMs<10000);
    QVERIFY(write(QDir(QCoreApplication::applicationDirPath()).filePath("phase06-large-sbom.json"),json(d)));
}

void Phase06Test::previewApply()
{
    Context c; QVERIFY(c.open()); const auto id=c.project();
    const auto a=document({"A","B"},{{"A",{"B"}}});
    auto b=document({"X","X"},{{"X",{"unknown","X",""}},{"",{"X"}}});
    QVERIFY(c.current.replaceForProject(id,a).ok()); const auto before=c.snapshot(id);
    const auto file=c.dir.filePath("candidate.json"); QVERIFY(write(file,json(b)));
    {
        SbomImportDialog preview(id,"synthetic",c.path(),c.logger);
        QSignalSpy done(&preview,&SbomImportDialog::importFinished);
        preview.importFile(file); QTRY_COMPARE(done.count(),1); QVERIFY(done.last()[0].toBool());
        QCOMPARE(c.snapshot(id),before); preview.reject();
    }
    QCOMPARE(c.snapshot(id),before);
    SbomImportDialog dialog(id,"synthetic",c.path(),c.logger);
    QSignalSpy parsed(&dialog,&SbomImportDialog::importFinished),applied(&dialog,&SbomImportDialog::applyFinished);
    dialog.importFile(file); QTRY_COMPARE(parsed.count(),1); QVERIFY(parsed.last()[0].toBool());
    QCOMPARE(c.snapshot(id),before);
    QVERIFY(dialog.findChild<QLabel*>("sbomStatus")->text().contains(QStringLiteral("存在质量 Error")));
    // An actual mid-target failure must retain all three parts and the candidate.
    {QSqlQuery q(c.db.connection()); QVERIFY(q.exec("CREATE TRIGGER reject BEFORE INSERT ON dependency_targets WHEN NEW.target_order=1 BEGIN SELECT RAISE(ABORT,'synthetic private sentinel'); END"));}
    dialog.applyToProject(); QTRY_COMPARE(applied.count(),1); QVERIFY(!applied.last()[0].toBool()); QCOMPARE(c.snapshot(id),before);
    QVERIFY(dialog.findChild<QPushButton*>("applySbom")->isEnabled());
    QVERIFY(!dialog.findChild<QLabel*>("sbomStatus")->text().contains("sentinel"));
    {QSqlQuery q(c.db.connection()); QVERIFY(q.exec("DROP TRIGGER reject"));}
    dialog.applyToProject(); dialog.applyToProject(); QTRY_COMPARE(applied.count(),2); QVERIFY(applied.last()[0].toBool());
    const auto after=c.snapshot(id); QCOMPARE(after.components.size(),2); QCOMPARE(after.entries.size(),2); QVERIFY(after.captured); QVERIFY(after.components[0].id!=before.components[0].id);
    dialog.applyToProject(); QCOMPARE(applied.count(),2); QCOMPARE(c.snapshot(id),after);
    for(const auto& bad:{QByteArray("{"),QByteArray("{}"),QByteArray(R"({"bomFormat":"CycloneDX","specVersion":"9"})"),QByteArray(R"({"bomFormat":"CycloneDX","specVersion":"1.6","dependencies":{}})")}) {
        const int expected=parsed.count()+1; QVERIFY(write(file,bad)); dialog.importFile(file); QTRY_COMPARE(parsed.count(),expected);
        QVERIFY(!parsed.last()[0].toBool()); QCOMPARE(c.snapshot(id),after);
    }
    QFile log(c.dir.filePath("test.log")); QVERIFY(log.open(QIODevice::ReadOnly)); const auto bytes=log.readAll();
    QVERIFY(!bytes.contains("sentinel")); QVERIFY(!bytes.contains("unknown")); QVERIFY(!bytes.contains("pkg:generic"));
}

void Phase06Test::dependencyUi()
{
    Context c; QVERIFY(legacy(c.path(),3)); QVERIFY(c.open());
    DependencyPage page(c.path()); page.resize(850,520); page.show(); QVERIFY(QTest::qWaitForWindowExposed(&page));
    QSignalSpy finished(&page,&DependencyPage::analysisFinished),queried(&page,&DependencyPage::relationshipFinished);
    page.setProject("legacy"); QTRY_COMPARE(finished.count(),1); QVERIFY(finished.last()[0].toBool());
    auto* summary=page.findChild<QTextBrowser*>("dependencySummary");
    auto* status=page.findChild<QLabel*>("dependencyStatus");
    auto* entries=page.findChild<QTableView*>("dependencyEntries");
    auto* targets=page.findChild<QTableView*>("dependencyTargets");
    auto* nodes=page.findChild<QTableView*>("dependencyComponents");
    auto* result=page.findChild<QTableView*>("dependencyRelationships");
    QVERIFY(status->text().contains("Not Captured")); QVERIFY(summary->toPlainText().contains(QStringLiteral("当前组件存在")));
    QVERIFY(!summary->toPlainText().contains("Dependency Entries")); QCOMPARE(entries->model()->rowCount(),0);
    const auto d=document({"A","B","C","D","E"},{{"A",{"B","C","B","A"}},{"C",{"D"}},{"B",{"E"}},{"D",{"A"}},{"unknown",{""," A "}}});
    QVERIFY(c.current.replaceForProject("legacy",d).ok()); page.reload(); QTRY_COMPARE(finished.count(),2);
    QVERIFY(status->text().startsWith("Captured")); QVERIFY(summary->toPlainText().contains("Resolved Unique Edges：6"));
    QCOMPARE(entries->model()->rowCount(),5); QCOMPARE(entries->editTriggers(),QAbstractItemView::NoEditTriggers);
    entries->setCurrentIndex(entries->model()->index(4,0)); QCOMPARE(targets->model()->rowCount(),2);
    QCOMPARE(entries->model()->data(entries->model()->index(4,2)).toString(),QString("Unknown / 未找到"));
    QCOMPARE(targets->model()->data(targets->model()->index(0,2)).toString(),QString("Missing / 缺失"));
    QCOMPARE(targets->model()->data(targets->model()->index(1,2)).toString(),QString("Unknown / 未找到"));
    QCOMPARE(targets->model()->data(targets->model()->index(1,3)).toString(),QString("—"));
    entries->setCurrentIndex(entries->model()->index(0,0)); QCOMPARE(targets->model()->rowCount(),4);
    QVERIFY(targets->model()->data(targets->model()->index(0,3)).toString().contains("synthetic-B"));
    nodes->setCurrentIndex(nodes->model()->index(0,0)); QTRY_COMPARE(queried.count(),1); QCOMPARE(result->model()->rowCount(),3);
    QCOMPARE(result->model()->data(result->model()->index(2,4)).toString(),QString("Self dependency / 自依赖"));
    QCOMPARE(result->model()->data(result->model()->index(2,3)).toString(),QString("—"));
    auto* direction=page.findChild<QComboBox*>("dependencyDirection"); direction->setCurrentIndex(2); QTRY_COMPARE(queried.count(),2);
    QCOMPARE(result->model()->rowCount(),4);
    for(int row=0;row<4;++row) {QVERIFY(result->model()->data(result->model()->index(row,1)).toString()!="A"); QCOMPARE(result->model()->data(result->model()->index(row,3)).toInt(),row<2?1:2);}
    direction->setCurrentIndex(1); QTRY_COMPARE(queried.count(),3); QCOMPARE(result->model()->rowCount(),2);
    direction->setCurrentIndex(3); QTRY_COMPARE(queried.count(),4); QCOMPARE(result->model()->rowCount(),2);
    QCOMPARE(result->model()->data(result->model()->index(0,1)).toString(),QString("D"));
    QCOMPARE(result->model()->data(result->model()->index(1,3)).toInt(),2);
    page.findChild<QTabWidget*>("dependencyTabs")->setCurrentIndex(2);
    QVERIFY(page.grab().save(QDir(QCoreApplication::applicationDirPath()).filePath("phase06-relationships.png")));
    QVERIFY(c.current.replaceForProject("legacy",document({"A"},{})).ok()); page.reload(); QTRY_COMPARE(finished.count(),3);
    QVERIFY(summary->toPlainText().contains(QStringLiteral("未声明 dependency entries"))); QVERIFY(status->text().startsWith("Captured"));
    QCOMPARE(entries->model()->rowCount(),0); QCOMPARE(result->model()->rowCount(),0);
    c.db.close(); QVERIFY(c.db.open(c.path(),c.error)); page.reload(); QTRY_COMPARE(finished.count(),4); QVERIFY(summary->toPlainText().contains("Dependency Entries：0"));
}

void Phase06Test::graphReuse()
{
    Context c; QVERIFY(c.open()); const auto id=c.project(); QVERIFY(c.current.replaceForProject(id,document({"A","B","C"},{{"A",{"B"}},{"B",{"C"}}})).ok());
    DependencyPage page(c.path()); page.show(); QSignalSpy built(&page,&DependencyPage::analysisFinished),queried(&page,&DependencyPage::relationshipFinished);
    page.setProject(id); QTRY_COMPARE(built.count(),1);
    // Make DB reads fail after the graph has been installed. Selection must still
    // query the existing graph; an explicit Reload must actually read the DB again.
    {QSqlQuery q(c.db.connection()); QVERIFY(q.exec("DROP TABLE dependency_targets"));}
    auto* nodes=page.findChild<QTableView*>("dependencyComponents"); auto* result=page.findChild<QTableView*>("dependencyRelationships");
    nodes->setCurrentIndex(nodes->model()->index(0,0)); QTRY_COMPARE(queried.count(),1); QCOMPARE(result->model()->rowCount(),1);
    nodes->setCurrentIndex(nodes->model()->index(1,0)); QTRY_COMPARE(queried.count(),2); QCOMPARE(result->model()->data(result->model()->index(0,1)).toString(),QString("C"));
    QCOMPARE(built.count(),1);
    page.reload(); QCOMPARE(nodes->model()->rowCount(),0); QCOMPARE(result->model()->rowCount(),0);
    QTRY_COMPARE(built.count(),2); QVERIFY(!built.last()[0].toBool()); QCOMPARE(result->model()->rowCount(),0);
    QVERIFY(page.findChild<QTextBrowser*>("dependencySummary")->toPlainText().contains(QStringLiteral("无法读写")));
}

void Phase06Test::staleResults()
{
    Context c; QVERIFY(c.open()); const auto a=c.project(),b=c.project();
    QVERIFY(c.current.replaceForProject(a,document({"A","B"},{{"A",{"B"}}})).ok());
    QVERIFY(c.current.replaceForProject(b,document({"X","Y"},{{"X",{"Y"}}})).ok());
    DependencyPage page(c.path()); page.show(); QSignalSpy built(&page,&DependencyPage::analysisFinished),queried(&page,&DependencyPage::relationshipFinished);
    page.setProject(a);
    // Complete worker A without processing GUI delivery. Its old result arrives
    // only after the project/request has changed, deterministically (no sleep hook).
    QVERIFY(QThreadPool::globalInstance()->waitForDone(10000)); page.setProject(b);
    QTRY_COMPARE(built.count(),1); QVERIFY(built.last()[0].toBool());
    auto* entries=page.findChild<QTableView*>("dependencyEntries"); auto* nodes=page.findChild<QTableView*>("dependencyComponents");
    QCOMPARE(entries->model()->data(entries->model()->index(0,1)).toString(),QString("X"));
    page.setProject(a); QVERIFY(QThreadPool::globalInstance()->waitForDone(10000));
    QVERIFY(c.current.replaceForProject(a,document({"N","M"},{{"N",{"M"}}})).ok()); page.setProject(a);
    QTRY_COMPARE(built.count(),2); QCOMPARE(entries->model()->data(entries->model()->index(0,1)).toString(),QString("N"));
    page.reload(); QVERIFY(QThreadPool::globalInstance()->waitForDone(10000));
    for(int i=0;i<20;++i) page.reload(); QTRY_COMPARE(built.count(),3); // Coalesced; old requests never installed.
    nodes->setCurrentIndex(nodes->model()->index(0,0)); QVERIFY(QThreadPool::globalInstance()->waitForDone(10000));
    nodes->setCurrentIndex(nodes->model()->index(1,0)); QTRY_COMPARE(queried.count(),1);
    QCOMPARE(page.findChild<QTableView*>("dependencyRelationships")->model()->rowCount(),0);
    page.reload(); QVERIFY(QThreadPool::globalInstance()->waitForDone(10000)); page.setProject({});
    QVERIFY(QThreadPool::globalInstance()->waitForDone(10000)); QCoreApplication::sendPostedEvents();
    QCOMPARE(entries->model()->rowCount(),0); QVERIFY(page.findChild<QLabel*>("dependencyStatus")->text().contains(QStringLiteral("请选择")));
}

void Phase06Test::projectIntegration()
{
    const bool native=QCoreApplication::testAttribute(Qt::AA_DontUseNativeDialogs); QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    const auto restore=qScopeGuard([&]{QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs,native);});
    Context c; QVERIFY(c.open()); const auto a=c.project(),b=c.project();
    QVERIFY(c.current.replaceForProject(a,document({"A","B"},{{"A",{"B"}}})).ok());
    MainWindow window(new ProjectPage(c.projects,c.current,c.logger),nullptr,"projects"); window.show(); QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto* list=window.findChild<QListWidget*>("projectList"); const auto select=[&](const QString& id){for(int i=0;i<list->count();++i) if(list->item(i)->data(Qt::UserRole).toString()==id) list->setCurrentRow(i);};
    auto* dep=window.findChild<DependencyPage*>(); QSignalSpy built(dep,&DependencyPage::analysisFinished);
    select(a); window.findChild<QTabWidget*>("projectTabs")->setCurrentIndex(2); QTRY_COMPARE(built.count(),1);
    auto* entries=dep->findChild<QTableView*>("dependencyEntries"); QCOMPARE(entries->model()->rowCount(),1);
    const auto file=c.dir.filePath("new.json"); QVERIFY(write(file,json(document({"X","Y"},{{"X",{"Y","X"}}}))));
    window.findChild<QPushButton*>("importSbom")->click(); QPointer<SbomImportDialog> dialog=window.findChild<SbomImportDialog*>(); QVERIFY(dialog);
    QPointer<QFileDialog> picker=dialog->findChild<QFileDialog*>(); QVERIFY(picker); picker->reject(); QTRY_VERIFY(picker.isNull());
    QSignalSpy parsed(dialog,&SbomImportDialog::importFinished),applied(dialog,&SbomImportDialog::applyFinished);
    dialog->importFile(file); QTRY_COMPARE(parsed.count(),1); QCOMPARE(entries->model()->data(entries->model()->index(0,1)).toString(),QString("A"));
    dialog->applyToProject(); QTRY_COMPARE(applied.count(),1); QVERIFY(applied.last()[0].toBool()); QTRY_COMPARE(built.count(),2);
    QCOMPARE(entries->model()->data(entries->model()->index(0,1)).toString(),QString("X")); dialog->reject(); QTRY_VERIFY(dialog.isNull());
    select(b); QTRY_COMPARE(built.count(),3); QVERIFY(dep->findChild<QLabel*>("dependencyStatus")->text().contains("Not Captured"));
    select(a); QTRY_COMPARE(built.count(),4); QCOMPARE(entries->model()->rowCount(),1);
    window.findChild<QPushButton*>("refreshProjects")->click(); QTRY_COMPARE(built.count(),5);
    QVERIFY(window.grab().save(QDir(QCoreApplication::applicationDirPath()).filePath("phase06-summary.png")));
    window.resize(680,420); QTRY_COMPARE(window.size(),QSize(680,420));
    QVERIFY(window.grab().save(QDir(QCoreApplication::applicationDirPath()).filePath("phase06-small.png")));
    dep->findChild<QTabWidget*>("dependencyTabs")->setCurrentIndex(1);
    entries->setCurrentIndex(entries->model()->index(0,0));
    QTRY_VERIFY(entries->viewport()->height()>20);
    QTRY_VERIFY(dep->findChild<QTableView*>("dependencyTargets")->viewport()->height()>20);
    QVERIFY(window.grab().save(QDir(QCoreApplication::applicationDirPath()).filePath("phase06-small-raw.png")));
    dep->findChild<QTabWidget*>("dependencyTabs")->setCurrentIndex(2);
    QTRY_VERIFY(dep->findChild<QTableView*>("dependencyComponents")->viewport()->height()>20);
    QTRY_VERIFY(dep->findChild<QTableView*>("dependencyRelationships")->viewport()->height()>20);
    QVERIFY(window.grab().save(QDir(QCoreApplication::applicationDirPath()).filePath("phase06-small-relations.png")));
    QVERIFY(c.projects.remove(a).ok()); window.findChild<QPushButton*>("refreshProjects")->click(); QCOMPARE(entries->model()->rowCount(),0);
    QVERIFY(c.snapshot(b).entries.isEmpty()); QVERIFY(!c.snapshot(b).captured);
    QVERIFY(QThreadPool::globalInstance()->waitForDone(10000));
}

void Phase06Test::largeUi()
{
    Context c; QVERIFY(c.open()); const auto id=c.project(); const auto d=largeDocument();
    const auto file=c.dir.filePath("large.json"); QVERIFY(write(file,json(d)));
    SbomImportDialog apply(id,"synthetic large",c.path(),c.logger); apply.show();
    QSignalSpy parsed(&apply,&SbomImportDialog::importFinished),applied(&apply,&SbomImportDialog::applyFinished);
    apply.importFile(file); QTRY_COMPARE_WITH_TIMEOUT(parsed.count(),1,15000); QVERIFY(parsed.last()[0].toBool());
    QElapsedTimer timer; timer.start(); apply.applyToProject(); const auto applyDispatch=timer.elapsed(); QVERIFY(applyDispatch<500);
    bool applyEvent=false; QMetaObject::invokeMethod(&apply,[&]{applyEvent=applied.isEmpty();},Qt::QueuedConnection);
    QTRY_VERIFY(applyEvent); QTRY_COMPARE_WITH_TIMEOUT(applied.count(),1,20000); QVERIFY(applied.last()[0].toBool()); const auto applyMs=timer.elapsed();
    DependencyPage page(c.path()); page.resize(850,520); page.show(); QVERIFY(QTest::qWaitForWindowExposed(&page));
    QSignalSpy built(&page,&DependencyPage::analysisFinished),queried(&page,&DependencyPage::relationshipFinished);
    timer.restart(); page.setProject(id); const auto dispatch=timer.elapsed(); QVERIFY(dispatch<500);
    bool delivered=false; QMetaObject::invokeMethod(&page,[&]{delivered=built.isEmpty(); page.resize(800,500);},Qt::QueuedConnection);
    int wakeups=0; qint64 gap=0,previous=timer.elapsed();
    const auto connection=connect(QAbstractEventDispatcher::instance(),&QAbstractEventDispatcher::awake,&page,[&]{const auto now=timer.elapsed(); gap=qMax(gap,now-previous); previous=now; ++wakeups;});
    QTRY_VERIFY(delivered); QTRY_COMPARE_WITH_TIMEOUT(built.count(),1,15000); QVERIFY(built.last()[0].toBool()); const auto analysisMs=timer.elapsed();
    auto* nodes=page.findChild<QTableView*>("dependencyComponents"); QCOMPARE(nodes->model()->rowCount(),100000);
    QCOMPARE(page.findChild<QTableView*>("dependencyEntries")->model()->rowCount(),100000);
    page.findChild<QComboBox*>("dependencyDirection")->setCurrentIndex(2);
    timer.restart(); previous=0; nodes->setCurrentIndex(nodes->model()->index(0,0)); const auto queryDispatch=timer.elapsed(); QVERIFY(queryDispatch<500);
    QTRY_COMPARE_WITH_TIMEOUT(queried.count(),1,10000); const auto queryMs=timer.elapsed();
    auto* results=page.findChild<QTableView*>("dependencyRelationships"); QCOMPARE(results->model()->rowCount(),99999);
    QCOMPARE(results->model()->data(results->model()->index(99998,3)).toInt(),99999);
    disconnect(connection); QVERIFY(wakeups>1); QVERIFY2(gap<1000,"Dependency GUI event loop stalled");
    qInfo()<<"100k GUI Apply dispatch/completion ms="<<applyDispatch<<applyMs<<"analysis dispatch/completion ms="<<dispatch<<analysisMs
        <<"query dispatch/completion ms="<<queryDispatch<<queryMs<<"wakeups="<<wakeups<<"max gap ms="<<gap;
    page.findChild<QTabWidget*>("dependencyTabs")->setCurrentIndex(2); results->scrollToBottom();
    QVERIFY(page.grab().save(QDir(QCoreApplication::applicationDirPath()).filePath("phase06-large.png")));
}

void Phase06Test::workerLifetime()
{
    Context c; QVERIFY(c.open()); const auto id=c.project(); QVERIFY(c.current.replaceForProject(id,document({"A","B"},{{"A",{"B"}}})).ok());
    const auto connections=QSqlDatabase::connectionNames();
    auto* page=new DependencyPage(c.path()); page->show(); page->setProject(id); delete page;
    QVERIFY(QThreadPool::globalInstance()->waitForDone(10000)); QCOMPARE(QSqlDatabase::connectionNames(),connections);
    page=new DependencyPage(c.path()); page->show(); QSignalSpy built(page,&DependencyPage::analysisFinished);
    page->setProject(id); QTRY_COMPARE(built.count(),1);
    auto* nodes=page->findChild<QTableView*>("dependencyComponents"); nodes->setCurrentIndex(nodes->model()->index(0,0)); delete page;
    c.db.close(); QVERIFY(QThreadPool::globalInstance()->waitForDone(10000)); QVERIFY(c.db.open(c.path(),c.error));
    QCOMPARE(QSqlDatabase::connectionNames(),connections); QCOMPARE(c.snapshot(id).entries.size(),1);
}

void Phase06Test::manualFixtures()
{
    const QDir output(QCoreApplication::applicationDirPath());
    auto normal=document({"A","B","C","D","E"},{{"root",{"A"}},{"A",{"B","C"}},{"B",{"E"}},{"C",{"D"}},{"D",{}},{"E",{}}});
    normal.metadata.component=SbomComponent{"root","application","synthetic-root","1.0","pkg:generic/synthetic-root@1.0"};
    const auto issues=document({"A","B","duplicate","duplicate"},{{"A",{"B","B","A","A","unknown"}},{"A",{"B"}},{"duplicate",{"duplicate"}},{"",{""," \t"}},{"unknown",{"unknown"}}});
    const auto cycle=document({"A","B","C"},{{"A",{"B"}},{"B",{"C"}},{"C",{"A"}}});
    const auto replacement=document({"X","Y"},{{"X",{"Y"}},{"Y",{}}});
    QVERIFY(write(output.filePath("phase06-normal-sbom.json"),json(normal)));
    QVERIFY(write(output.filePath("phase06-issues-sbom.json"),json(issues)));
    QVERIFY(write(output.filePath("phase06-cycle-sbom.json"),json(cycle)));
    QVERIFY(write(output.filePath("phase06-empty-sbom.json"),json(document({"A"},{}))));
    QVERIFY(write(output.filePath("phase06-replacement-sbom.json"),json(replacement)));
    QVERIFY(write(output.filePath("phase06-parser-error.json"),"{not-json"));
    const auto metrics=DependencyGraph::build(snapshotOf(issues)).metrics();
    QCOMPARE(metrics,(DependencyMetrics{5,10,3,3,2,4,2}));
    // An isolated, synthetic v3 starting point. Never overwrite prior manual work.
    const auto root=output.filePath("phase06-manual-data"); QVERIFY(QDir().mkpath(root+"/data"));
    const auto db=root+"/data/supply_chain_risk.db";
    if(!QFile::exists(db)) QVERIFY(legacy(db,3));
    QVERIFY(write(output.filePath("phase06-manual.cmd"),"@echo off\r\nsetlocal\r\nset \"PATH=D:\\program\\Qt\\Tools\\mingw1310_64\\bin;D:\\program\\Qt\\6.11.2\\mingw_64\\bin;%PATH%\"\r\n\"%~dp0SupplyChainRiskAssessment.exe\" --data-dir \"%~dp0phase06-manual-data\"\r\nendlocal\r\n"));
}

QTEST_MAIN(Phase06Test)
#include "Phase06Test.moc"
