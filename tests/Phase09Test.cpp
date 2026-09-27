#include "VersionApplicability.h"
#include "RiskEvidenceController.h"
#include "CveIdentity.h"
#include <QUrlQuery>
#include <QTimer>
#include <QUuid>
#include "VulnerabilityController.h"
#include "VulnerabilityPage.h"
#include "AppDatabase.h"
#include "AppLogger.h"
#include "AppPaths.h"
#include "ComponentRepository.h"
#include "ProjectRepository.h"
#include "ProjectPage.h"
#include "SbomImportDialog.h"
#include <QTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QThreadPool>
#include <QSemaphore>
#include <QtConcurrentRun>
#include <QTableView>
#include <QTextBrowser>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTabWidget>
#include <QListWidget>
#include <QFileDialog>
#include <QSqlQuery>
#include <QElapsedTimer>
#include <QScrollArea>
#include <QScrollBar>
#include <QLibraryInfo>
#include <algorithm>
#include <cstring>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <future>
#endif

namespace {
using State = ApplicabilityState;
constexpr auto Source = CandidateSource::OsvPackageVersionQuery;
QueryIdentity identity(QString version = "1.2.3") { return {"npm","synthetic-phase09",version,1}; }
QJsonObject entry(QJsonArray ranges = {}, QJsonArray versions = {}, QString name = "synthetic-phase09", QString ecosystem = "npm")
{ return {{"package",QJsonObject{{"ecosystem",ecosystem},{"name",name}}},{"ranges",ranges},{"versions",versions}}; }
VulnerabilityCandidate candidate(QJsonArray affected, QString id = "SYNTHETIC-P09-1")
{ return {QJsonObject{{"id",id},{"modified","2026-01-01T00:00:00Z"},{"aliases",QJsonArray{"CVE-2026-1000001"}},{"affected",affected}}}; }
VulnerabilityCandidate exact(QString version = "1.2.3", QString id = "SYNTHETIC-P09-1") { return candidate({entry({}, {version})},id); }
QByteArray json(const QJsonObject& object) { return QJsonDocument(object).toJson(QJsonDocument::Compact); }
QByteArray read(const QString& path) { QFile f(path); if(!f.open(QIODevice::ReadOnly)) return {}; return f.readAll(); }
bool write(const QString& path,const QByteArray& bytes) { QFile f(path); return f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(); }
#ifdef Q_OS_WIN
class NativeHandle final {
public:
    explicit NativeHandle(HANDLE value):value(value){}
    ~NativeHandle(){close();}
    NativeHandle(const NativeHandle&)=delete;
    NativeHandle& operator=(const NativeHandle&)=delete;
    bool valid()const{return value && value!=INVALID_HANDLE_VALUE;}
    HANDLE get()const{return value;}
    void close(){if(valid())CloseHandle(value);value=INVALID_HANDLE_VALUE;}
private:
    HANDLE value;
};
HANDLE holdReplacement(const QString& path)
{
    return CreateFileW(reinterpret_cast<LPCWSTR>(path.utf16()),GENERIC_READ,
        FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
}
// Observe QSaveFile's failed temporary-file cleanup before releasing the target.
// This establishes a real first denial without guessing how quickly the writer runs.
class CacheReplacementWatch final {
public:
    explicit CacheReplacementWatch(const QString& path)
        :prefix(QFileInfo(path).fileName()+'.'),
         directory(CreateFileW(reinterpret_cast<LPCWSTR>(QFileInfo(path).absolutePath().utf16()),
             FILE_LIST_DIRECTORY,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,
             FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OVERLAPPED,nullptr)),
         event(CreateEventW(nullptr,TRUE,FALSE,nullptr)) {overlapped.hEvent=event.get();}
    ~CacheReplacementWatch(){
        if(pending){CancelIoEx(directory.get(),&overlapped);DWORD bytes=0;GetOverlappedResult(directory.get(),&overlapped,&bytes,TRUE);}
    }
    bool arm(){
        if(!directory.valid() || !event.valid() || pending)return false;
        ResetEvent(event.get());
        pending=ReadDirectoryChangesW(directory.get(),buffer,sizeof(buffer),FALSE,
            FILE_NOTIFY_CHANGE_FILE_NAME,nullptr,&overlapped,nullptr)!=FALSE;
        return pending;
    }
    bool waitForRemovals(int expected){
        QElapsedTimer timer;timer.start();
        while(removals<expected && timer.elapsed()<2000){
            if(!pending && !arm())return false;
            if(WaitForSingleObject(event.get(),DWORD(qMax<qint64>(1,2000-timer.elapsed())))!=WAIT_OBJECT_0)return false;
            DWORD bytes=0;const bool ok=GetOverlappedResult(directory.get(),&overlapped,&bytes,FALSE)!=FALSE;
            pending=false;if(!ok || !bytes)return false;
            DWORD offset=0;
            do {
                const auto* change=reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(buffer+offset);
                const auto name=QString::fromWCharArray(change->FileName,int(change->FileNameLength/sizeof(WCHAR)));
                if(change->Action==FILE_ACTION_REMOVED && name.startsWith(prefix))++removals;
                if(!change->NextEntryOffset)break;
                offset+=change->NextEntryOffset;
            }while(offset<bytes);
        }
        return removals==expected;
    }
private:
    QString prefix;
    NativeHandle directory,event;
    OVERLAPPED overlapped{};
    alignas(DWORD) unsigned char buffer[4096]{};
    bool pending=false;
    int removals=0;
};
#endif
struct Context {
    QTemporaryDir dir;AppDatabase db;AppLogger logger;ProjectRepository projects{db,logger};ComponentRepository components{db};
    QString path()const{return dir.filePath("synthetic.db");}QString cache()const{return dir.filePath("cache/osv-v1");}
    bool open(){QString error;return logger.open(dir.filePath("synthetic.log"))&&db.open(path(),error);}
    QString project(){Project p;if(!projects.create("synthetic phase09",{},p).ok())qFatal("fixture project");return p.id;}
    bool apply(const QString& id) {
        SbomDocument d;d.components={{"a","library","synthetic-display","1.2.3","pkg:npm/synthetic-phase09@1.2.3"},
            {"b","library","synthetic-display","1.2.3","pkg:npm/synthetic-phase09@1.2.3"}};
        return components.replaceForProject(id,d).ok();
    }
    bool seed(QList<VulnerabilityCandidate> candidates={exact()},int days=0) {
        return OsvCache(cache()).write({identity(),QDateTime::currentDateTimeUtc().addDays(days),candidates})==QueryError::None;
    }
};
// Deterministic queue control in tests only; destructor releases even on assertion failure.
class PoolBlock final {
public:
    PoolBlock():pool(QThreadPool::globalInstance()),previous(pool->maxThreadCount()) {
        pool->waitForDone();pool->setMaxThreadCount(1);
        task=QtConcurrent::run([this]{entered.release();release.acquire();});entered.acquire();
    }
    ~PoolBlock(){release.release();task.waitForFinished();pool->waitForDone();pool->setMaxThreadCount(previous);}
private:
    QThreadPool* pool;int previous;QSemaphore entered,release;QFuture<void> task;
};
}


namespace {
const QString CVE="CVE-2026-1000001";
QDateTime now(){return QDateTime::currentDateTimeUtc();}
QJsonObject score(QString cve=CVE){return {{"cve",cve},{"epss","0.125"},{"percentile","0.875"},{"date","2026-09-25"}};}
QJsonObject epss(QJsonArray rows={score()},int limit=1){return {{"status","OK"},{"status-code",200},{"version","1.0"},{"total",rows.size()},{"offset",0},{"limit",limit},{"data",rows}};}
QJsonObject kevEntry(QString cve=CVE){return {{"cveID",cve},{"vendorProject","Synthetic"},{"product","Fixture"},{"vulnerabilityName","Synthetic only"},{"dateAdded","2026-09-01"},{"shortDescription","Fixture evidence"},{"requiredAction","Synthetic provider action"},{"dueDate","2026-09-30"}};}
QJsonObject catalog(QJsonArray rows={kevEntry()}){return {{"catalogVersion","synthetic-2026.09.25"},{"dateReleased","2026-09-25T00:00:00Z"},{"count",rows.size()},{"vulnerabilities",rows}};}
QJsonObject severityItem(QString type="CVSS_V4"){return {{"type",type},{"score","CVSS:4.0/AV:N/AC:L/AT:N/PR:N/UI:N/VC:H/VI:H/VA:H/SC:N/SI:N/SA:N"}};}
Component component(QString id,QString ref,ComponentSourceRole role=ComponentSourceRole::Component){Component c;c.id=id;c.bomRef=ref;c.sourceRole=role;return c;}
DependencySnapshot graph(){return {{component("root","r",ComponentSourceRole::MetadataRoot),component("a","a"),component("b","b"),component("x","x")},true,
    {{"e1","p",0,"r",{{"t1","e1",0,"a"}}},{"e2","p",1,"a",{{"t2","e2",0,"b"}}}}};}
struct Plan {QByteArray body;int status=200;QNetworkReply::NetworkError error=QNetworkReply::NoError;bool hold=false;bool tls=false;qint64 length=-1;};
class Reply final:public QNetworkReply {
public:
    Reply(const QNetworkRequest& r,Plan plan,QObject* parent):QNetworkReply(parent),p(std::move(plan)){
        setRequest(r);setUrl(r.url());open(ReadOnly|Unbuffered);
        if(!p.hold)QTimer::singleShot(0,this,[this]{deliver();});
    }
    void deliver(){if(isFinished())return;setAttribute(QNetworkRequest::HttpStatusCodeAttribute,p.status);
        if(p.length>=0)setHeader(QNetworkRequest::ContentLengthHeader,p.length);
        if(p.tls){emit sslErrors({QSslError(QSslError::SelfSignedCertificate)});if(isFinished())return;}
        if(p.error!=NoError)setError(p.error,"synthetic transport failure");
        emit readyRead();if(isFinished())return;setFinished(true);emit finished();}
    void abort()override{if(isFinished())return;setError(OperationCanceledError,"cancelled");setFinished(true);emit finished();}
    qint64 bytesAvailable()const override{return p.body.size()-offset+QNetworkReply::bytesAvailable();}
protected:
    qint64 readData(char* data,qint64 max)override{const qint64 size=qMin(max,qint64(p.body.size()-offset));if(size<=0)return -1;std::memcpy(data,p.body.constData()+offset,size_t(size));offset+=size;return size;}
private:Plan p;qint64 offset=0;
};
class Network final:public QNetworkAccessManager {
public:QList<Plan> epssPlans,kevPlans;QList<QNetworkRequest> requests;QList<QPointer<Reply>> replies;bool automatic=false;
protected:
    QNetworkReply* createRequest(Operation op,const QNetworkRequest& r,QIODevice* outgoing)override{
        if(op!=GetOperation||outgoing)qFatal("Non-public request in Phase09 test");requests.append(r);Plan p;
        if(r.url().host()=="api.first.org"){
            if(!epssPlans.isEmpty())p=epssPlans.takeFirst();
            else if(automatic){QUrlQuery q(r.url());QJsonArray rows;const auto ids=q.queryItemValue("cve").split(',');for(const auto& id:ids)rows.append(score(id));p.body=json(epss(rows,ids.size()));}
            else qFatal("Unplanned EPSS network request");
        }else if(r.url()==QUrl(RiskEvidence::KevEndpoint)){
            if(!kevPlans.isEmpty())p=kevPlans.takeFirst();else if(automatic)p.body=json(catalog());else qFatal("Unplanned KEV request");
        }else qFatal("Unexpected network endpoint");
        auto* reply=new Reply(r,p,this);replies.append(reply);return reply;
    }
};
}
class Phase09Test:public QObject {
    Q_OBJECT
private slots:
    void initTestCase(){QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);}
    void ownership();void requestLookupScale();void kevCveBoundaries();void severityScopes();void severityValidation();void epssChunks();void epssEnvelope();void epssRows();
    void epssCache();void kevValidation();void kevCache();void dependencyStates();void transport();void privacy();
#ifdef Q_OS_WIN
    void epssCacheTransientRename();void epssCachePermanentRenameDenial();
#endif
    void providerFailures();void projectApply();void cacheOnly();void consent();void liveAtomic();void partialChunks();void fallback();void cancellation();
    void lifecycle();void staleDependency();void destruction();void ui();void scale();void manualFixtures();
};
void Phase09Test::ownership(){
    auto c=component("component","a");OsvSnapshot raw{identity(),now(),{exact()}};
    auto app=VersionApplicability::evaluateSnapshot(c.id,raw,42,Source);QCOMPARE(app.findings.size(),1);
    auto f=app.findings.first();auto valid=[&]{return RiskEvidenceRequest::validOwnership(c,raw,app,42,0,f);};QVERIFY(valid());
    QVERIFY(!RiskEvidenceRequest::validOwnership(c,raw,app,42,-1,f));
    QVERIFY(!RiskEvidenceRequest::validOwnership(c,raw,app,42,raw.candidates.size(),f));
    app.findings.append(f);QVERIFY(!valid());app.findings.removeLast();QVERIFY(valid());
    auto original=f;f.cveAliases={"CVE-2026-9999999"};QVERIFY(!valid());f=original;
    f.componentId="other";QVERIFY(!valid());f=original;f.snapshotIdentity.name="other";QVERIFY(!valid());f=original;
    f.queryIdentity.version="different";QVERIFY(!valid());f=original;f.fetchedAt=f.fetchedAt.addSecs(1);QVERIFY(!valid());f=original;
    f.osvId="missing";QVERIFY(!valid());f=original;f.applicability.evidence.clear();QVERIFY(!valid());f=original;
    app.generation++;QVERIFY(!valid());app.generation--;app.fetchedAt=app.fetchedAt.addSecs(1);QVERIFY(!valid());app.fetchedAt=raw.fetchedAt;
    app.candidates[0].published->state=State::Unknown;QVERIFY(!valid());app.candidates[0].published->state=State::Affected;
    raw.candidates.append(raw.candidates.first());app.candidates.append(app.candidates.first());QVERIFY(!valid());
    raw.candidates.removeLast();app.candidates.removeLast();app.findings.clear();QVERIFY(!valid());
}
void Phase09Test::requestLookupScale(){
    Context c;QVERIFY(c.open());const auto project=c.project();QVERIFY(c.apply(project));
    constexpr int count=10000;
    QList<VulnerabilityCandidate> records;records.reserve(count);
    for(int i=0;i<count;++i){
        auto record=exact("1.2.3",QString("SYNTHETIC-LOOKUP-%1").arg(i,5,10,QChar('0')));
        record.record["aliases"]=QJsonArray{QString("CVE-2026-%1").arg(1000000+i)};records.append(record);
    }
    QVERIFY(c.seed(records));Network network;
    VulnerabilityController owner(c.path(),c.cache(),c.logger,nullptr,nullptr,&network);
    owner.setProject(project);owner.reload();QTRY_VERIFY(!owner.loading());owner.select(0);
    owner.query(QueryMode::CacheOnly);QTRY_VERIFY_WITH_TIMEOUT(!owner.busy(),15000);
    QVERIFY(owner.applicability());QCOMPARE(owner.snapshot()->candidates.size(),count);
    QCOMPARE(owner.applicability()->findings.size(),count);
    QVERIFY(!owner.riskEvidenceRequest(-1));QVERIFY(!owner.riskEvidenceRequest(count));
    // Invoke the real synchronous GUI-thread entry point in separate event-loop
    // turns, alternating late candidates with distinct IDs and CVE aliases.
    int lookups=0,heartbeats=0;qint64 maxLookupNs=0,maxGapNs=0;
    QElapsedTimer gap;gap.start();QTimer heartbeat,lookup;
    connect(&heartbeat,&QTimer::timeout,&owner,[&]{++heartbeats;maxGapNs=qMax(maxGapNs,gap.nsecsElapsed());gap.restart();});
    connect(&lookup,&QTimer::timeout,&owner,[&]{
        const int index=count-1-(lookups%8);QElapsedTimer elapsed;elapsed.start();
        const auto request=owner.riskEvidenceRequest(index);maxLookupNs=qMax(maxLookupNs,elapsed.nsecsElapsed());
        if(++lookups==24)lookup.stop();
        QVERIFY(request);QCOMPARE(request->key().osvId,records[index].id());
        QCOMPARE(request->candidate().record,records[index].record);
        QCOMPARE(request->finding().osvId,records[index].id());
        QCOMPARE(request->finding().cveAliases,records[index].cveAliases());
        QCOMPARE(request->key().componentId,owner.rows()[0].component.id);
        QCOMPARE(request->key().snapshotIdentity,owner.snapshot()->identity);
        QCOMPARE(request->key().snapshotFetchedAt,owner.snapshot()->fetchedAt);
        QCOMPARE(request->key().applicabilityGeneration,owner.generation());
    });
    heartbeat.start(0);lookup.start(0);QTRY_COMPARE_WITH_TIMEOUT(lookups,24,5000);heartbeat.stop();
    QVERIFY(heartbeats>=24);
    QVERIFY2(maxLookupNs<100000000,qPrintable(QString::number(maxLookupNs/1000000.0)));
    QVERIFY2(maxGapNs<250000000,qPrintable(QString::number(maxGapNs/1000000.0)));
    QVERIFY(network.requests.isEmpty());
    qInfo()<<"10000 confirmed candidates/findings; 24 late lookups; max lookup ms="<<maxLookupNs/1000000.0
           <<"event-loop max gap ms="<<maxGapNs/1000000.0<<"heartbeats="<<heartbeats;
}
void Phase09Test::kevCveBoundaries(){
    const QString four="CVE-2026-1234",nineteen="CVE-2026-1234567890123456789";
    const QString twenty="CVE-2026-12345678901234567890";
    for(const auto& id:QStringList{four,nineteen}){
        const auto parsed=RiskEvidence::parseKev(json(catalog({kevEntry(id)})),now());QVERIFY(parsed);
        QCOMPARE(RiskEvidence::kevEvidence(*parsed,id,EvidenceAcquisition::Live,now()).status,KevStatus::Listed);
    }
    for(const auto& id:QStringList{"CVE-2026-123",twenty,"cve-2026-1234"}){
        // Even alongside a valid entry, this must invalidate the whole catalog.
        const auto body=json(catalog({kevEntry(four),kevEntry(id)}));
        QVERIFY(!RiskEvidence::parseKev(body,now()));
        Network network;network.kevPlans={{body}};KevClient client(nullptr,&network);
        QSignalSpy finished(&client,&KevClient::finished);QVERIFY(client.fetch());QTRY_COMPARE(finished.size(),1);
        const auto result=qvariant_cast<KevFetchResult>(finished.first()[0]);
        QCOMPARE(result.error,QueryError::ResponseInvalid);QVERIFY(!result.snapshot);
        // The same invalid catalog cannot enter the cache/NotListed path either.
        QTemporaryDir dir;KevCache cache(dir.path());
        QCOMPARE(cache.write({now(),QJsonDocument::fromJson(body).object(),{}}),QueryError::CacheInvalid);
        QVERIFY(!cache.read().snapshot);
    }
    // The sealed OSV alias rule deliberately still accepts 20 digits.
    QVERIFY(validCveId(twenty));auto record=exact();record.record["aliases"]=QJsonArray{twenty};
    QCOMPARE(record.cveAliases(),QStringList{twenty});
    const auto app=VersionApplicability::evaluateSnapshot("component",{identity(),now(),{record}},42,Source);
    QCOMPARE(app.findings.size(),1);QCOMPARE(app.findings[0].cveAliases,QStringList{twenty});
}
void Phase09Test::severityScopes(){
    auto c=exact();auto app=VersionApplicability::evaluateLocal(identity(),c);
    for(const auto& type:QStringList{"CVSS_V2","CVSS_V3","CVSS_V4"}){
        c.record["severity"]=QJsonArray{severityItem(type)};auto s=RiskEvidence::severity(c,app);QCOMPARE(s.status,SeverityStatus::Present);
        QCOMPARE(s.items.size(),1);QCOMPARE(s.items.first().type,type);QCOMPARE(s.items.first().provenance,SeverityProvenance::ImplicitHomeDatabase);
        QCOMPARE(s.items.first().recordId,c.id());QCOMPARE(s.items.first().scope,SeverityScope::TopLevel);
    }
    auto explicitItem=severityItem();explicitItem["source"]="https://example.test/synthetic";c.record["severity"]=QJsonArray{explicitItem};
    auto s=RiskEvidence::severity(c,app);QCOMPARE(s.items[0].provenance,SeverityProvenance::ExplicitSource);QCOMPARE(s.items[0].source,explicitItem["source"].toString());
    c.record.remove("severity");auto matching=entry({}, {"1.2.3"});matching["severity"]=QJsonArray{severityItem("CVSS_V3")};
    auto other=entry({}, {"1.2.3"},"other-package");other["severity"]=QJsonArray{severityItem("IGNORED")};
    c.record["affected"]=QJsonArray{matching,other,matching};app=VersionApplicability::evaluateLocal(identity(),c);s=RiskEvidence::severity(c,app);
    QCOMPARE(s.items.size(),2);QCOMPARE(s.items[0].scope,SeverityScope::MatchingAffected);QVERIFY(s.items[0].affectedIndex!=s.items[1].affectedIndex);
    for(const auto& item:s.items)QCOMPARE(item.type,QString("CVSS_V3"));
    c.record["severity"]=QJsonArray{severityItem()};QCOMPARE(RiskEvidence::severity(c,app).status,SeverityStatus::SchemaConflict);
    c.record["affected"]=QJsonArray{entry({}, {"1.2.3"}),other};app=VersionApplicability::evaluateLocal(identity(),c);
    QCOMPARE(RiskEvidence::severity(c,app).status,SeverityStatus::SchemaConflict);
    c.record.remove("severity");QCOMPARE(RiskEvidence::severity(c,app).status,SeverityStatus::Missing);
}
void Phase09Test::severityValidation(){
    auto c=exact();const auto app=VersionApplicability::evaluateLocal(identity(),c);QCOMPARE(RiskEvidence::severity(c,app).status,SeverityStatus::Missing);
    c.record["severity"]=QJsonArray{severityItem("OTHER_LEGAL")};QCOMPARE(RiskEvidence::severity(c,app).status,SeverityStatus::UnsupportedType);
    for(const auto& bad:QList<QJsonValue>{QJsonObject{},42,QJsonArray{42},QJsonArray{QJsonObject{{"type","CVSS_V4"}}},
        QJsonArray{QJsonObject{{"type",false},{"score","vector"}}},QJsonArray{QJsonObject{{"type","CVSS_V3"},{"score",QString(4097,'x')}}},
        QJsonArray{QJsonObject{{"type","CVSS_V3"},{"score","x"},{"source",42}}}}){
        c.record["severity"]=bad;QCOMPARE(RiskEvidence::severity(c,app).status,SeverityStatus::InvalidStructure);
    }
    auto structural=severityItem();structural["score"]="not semantically checked";c.record["severity"]=QJsonArray{structural};QCOMPARE(RiskEvidence::severity(c,app).status,SeverityStatus::Present);
}
void Phase09Test::epssChunks(){
    QStringList ids;for(int i=0;i<500;++i)ids.append(QString("CVE-2026-%1").arg(1000000+i));
    auto chunks=RiskEvidence::epssChunks(ids);QVERIFY(chunks.size()>1);QVERIFY(chunks.first().size()>100);
    QStringList flattened;for(const auto& chunk:chunks){QVERIFY(chunk.join(',').size()<=2000);flattened+=chunk;QUrlQuery q(EpssClient::url(chunk));QCOMPARE(q.queryItemValue("offset"),QString("0"));QCOMPARE(q.queryItemValue("limit").toInt(),chunk.size());}
    QCOMPARE(flattened,ids);std::reverse(ids.begin(),ids.end());ids.append(ids.first());ids.append("bad");QCOMPARE(RiskEvidence::epssChunks(ids),chunks);
    QVERIFY(!validCveId("CVE-2026-1000\n"));QVERIFY(!validCveId("cve-2026-1000"));QVERIFY(validCveId("CVE-2026-1000"));
    QVERIFY(RiskEvidence::epssChunks({"CVE-2026-"+QString(2000,'1')}).isEmpty());
}
void Phase09Test::epssEnvelope(){
    auto good=epss();auto parse=[&](QJsonObject o){return RiskEvidence::parseEpss(json(o),{CVE},now());};QVERIFY(parse(good));
    const QList<QPair<QString,QJsonValue>> bad{{"status","ERROR"},{"status-code",201},{"version",42},{"version",""},{"total",2},{"total",-1},{"total",0.5},{"offset",1},{"limit",0},{"limit",1.5},{"data",QJsonObject{}}};
    for(const auto& b:bad){auto o=good;o[b.first]=b.second;QVERIFY2(!parse(o),qPrintable(b.first));}
    for(const auto& key:good.keys()){auto o=good;o.remove(key);QVERIFY2(!parse(o),qPrintable(key));}
    QVERIFY(!RiskEvidence::parseEpss("{",{CVE},now()));QVERIFY(!RiskEvidence::parseEpss(QByteArray(RiskEvidence::MaxEpssBytes+1,'x'),{CVE},now()));
    auto missing=parse(epss({},1));QVERIFY(missing);QCOMPARE(RiskEvidence::epssEvidence(missing->first(),EvidenceAcquisition::Live,now()).status,EpssStatus::NotScored);
    auto incomplete=epss({},1);incomplete["total"]=1;QVERIFY(!parse(incomplete));
    auto multi=RiskEvidence::parseEpss(json(epss({score()},2)),{CVE,"CVE-2026-1000002"},now());QVERIFY(multi);QCOMPARE(multi->size(),2);QVERIFY(multi->last().record.isEmpty());
}
void Phase09Test::epssRows(){
    const QList<QPair<QString,QJsonValue>> bad{{"cve","CVE-2026-9999999"},{"epss","1.1"},{"epss","-0.1"},{"epss",0.1},{"epss","nan"},{"epss","0.1x"},{"percentile","1.001"},{"percentile","-1"},{"date","2026-02-30"},{"date","2026-9-1"}};
    for(const auto& b:bad){auto row=score();row[b.first]=b.second;QVERIFY2(!RiskEvidence::parseEpss(json(epss({row})),{CVE},now()),qPrintable(b.first));}
    QVERIFY(!RiskEvidence::parseEpss(json(epss({score(),score()},2)),{CVE,"CVE-2026-1000002"},now()));
    for(const auto& value:QStringList{"0","0.000","1.0"}){auto row=score();row["epss"]=value;auto parsed=RiskEvidence::parseEpss(json(epss({row})),{CVE},now());QVERIFY(parsed);auto e=RiskEvidence::epssEvidence(parsed->first(),EvidenceAcquisition::Live,now());QVERIFY(e.probability);QCOMPARE(*e.probability,value.toDouble());}
}
void Phase09Test::epssCache(){
    QTemporaryDir dir;EpssCache cache(dir.path());QCOMPARE(cache.read(CVE).error,QueryError::CacheMiss);
    for(int days:{0,-2})for(bool scored:{true,false}){
        auto s=RiskEvidence::parseEpss(json(epss(scored?QJsonArray{score()}:QJsonArray{})),{CVE},now().addDays(days))->first();QCOMPARE(cache.write(s),QueryError::None);
        auto r=cache.read(CVE);QVERIFY(r.snapshot);auto e=RiskEvidence::epssEvidence(*r.snapshot,EvidenceAcquisition::Cache,now());
        QCOMPARE(e.status,scored?EpssStatus::Available:EpssStatus::NotScored);QCOMPARE(e.freshness,days?EvidenceFreshness::Stale:EvidenceFreshness::Fresh);QCOMPARE(e.probability.has_value(),scored);
    }
    auto original=read(cache.filePath(CVE));auto invalid=RiskEvidence::parseEpss(json(epss()),{CVE},now())->first();invalid.fetchedAt=now().addDays(1);QCOMPARE(cache.write(invalid),QueryError::CacheInvalid);QCOMPARE(read(cache.filePath(CVE)),original);
    auto o=QJsonDocument::fromJson(original).object();o["fetchedAt"]=now().addDays(1).toString(Qt::ISODateWithMs);QVERIFY(write(cache.filePath(CVE),json(o)));QCOMPARE(cache.read(CVE).error,QueryError::CacheInvalid);
    QVERIFY(write(cache.filePath(CVE),"{"));QCOMPARE(cache.read(CVE).error,QueryError::CacheInvalid);
    EpssCache unsafe("relative-cache");QCOMPARE(unsafe.write(*RiskEvidence::parseEpss(json(epss()),{CVE},now())->begin()),QueryError::CacheIo);
}
#ifdef Q_OS_WIN
void Phase09Test::epssCacheTransientRename(){
    QTemporaryDir dir;QVERIFY(dir.isValid());EpssCache cache(dir.path());
    const auto old=RiskEvidence::parseEpss(json(epss({})),{CVE},now().addDays(-2))->first();
    const auto replacement=RiskEvidence::parseEpss(json(epss()),{CVE},now())->first();
    QCOMPARE(cache.write(old),QueryError::None);
    NativeHandle held(holdReplacement(cache.filePath(CVE)));QVERIFY(held.valid());
    CacheReplacementWatch watch(cache.filePath(CVE));QVERIFY(watch.arm());
    // std::async's future joins on every exit, including failed assertions.
    auto result=std::async(std::launch::async,[&]{return cache.write(replacement);});
    const bool firstDenied=watch.waitForRemovals(1);
    held.close();
    const auto error=result.get();
    QVERIFY(firstDenied);QCOMPARE(error,QueryError::None);
    const auto actual=cache.read(CVE);QCOMPARE(actual.error,QueryError::None);QVERIFY(actual.snapshot);
    QCOMPARE(actual.snapshot->cve,replacement.cve);QCOMPARE(actual.snapshot->record,replacement.record);
    QCOMPARE(actual.snapshot->providerVersion,replacement.providerVersion);QCOMPARE(actual.snapshot->fetchedAt,replacement.fetchedAt);
    QCOMPARE(QDir(QFileInfo(cache.filePath(CVE)).absolutePath()).entryList(QDir::Files),QStringList{QFileInfo(cache.filePath(CVE)).fileName()});
}
void Phase09Test::epssCachePermanentRenameDenial(){
    QTemporaryDir dir;QVERIFY(dir.isValid());EpssCache cache(dir.path());
    const auto old=RiskEvidence::parseEpss(json(epss()),{CVE},now().addDays(-2))->first();
    const auto replacement=RiskEvidence::parseEpss(json(epss({})),{CVE},now())->first();
    QCOMPARE(cache.write(old),QueryError::None);const auto original=read(cache.filePath(CVE));QVERIFY(!original.isEmpty());
    NativeHandle held(holdReplacement(cache.filePath(CVE)));QVERIFY(held.valid());
    CacheReplacementWatch watch(cache.filePath(CVE));QVERIFY(watch.arm());
    auto result=std::async(std::launch::async,[&]{return cache.write(replacement);});
    const bool threeDenied=watch.waitForRemovals(3);
    QCOMPARE(result.get(),QueryError::CacheIo);QVERIFY(threeDenied);
    QCOMPARE(read(cache.filePath(CVE)),original);
    auto actual=cache.read(CVE);QCOMPARE(actual.error,QueryError::None);QVERIFY(actual.snapshot);
    QCOMPARE(actual.snapshot->cve,old.cve);QCOMPARE(actual.snapshot->record,old.record);
    QCOMPARE(actual.snapshot->providerVersion,old.providerVersion);QCOMPARE(actual.snapshot->fetchedAt,old.fetchedAt);
    held.close();
    QCOMPARE(cache.write(replacement),QueryError::None);
    actual=cache.read(CVE);QCOMPARE(actual.error,QueryError::None);QVERIFY(actual.snapshot);
    QCOMPARE(actual.snapshot->cve,replacement.cve);QCOMPARE(actual.snapshot->record,replacement.record);
    QCOMPARE(actual.snapshot->providerVersion,replacement.providerVersion);QCOMPARE(actual.snapshot->fetchedAt,replacement.fetchedAt);
    QCOMPARE(QDir(QFileInfo(cache.filePath(CVE)).absolutePath()).entryList(QDir::Files),QStringList{QFileInfo(cache.filePath(CVE)).fileName()});
}
#endif
void Phase09Test::kevValidation(){
    auto good=catalog();auto parsed=RiskEvidence::parseKev(json(good),now());QVERIFY(parsed);QCOMPARE(parsed->entries.size(),1);
    QCOMPARE(RiskEvidence::kevEvidence(*parsed,CVE,EvidenceAcquisition::Live,now()).status,KevStatus::Listed);
    QCOMPARE(RiskEvidence::kevEvidence(*parsed,"CVE-2026-9999999",EvidenceAcquisition::Live,now()).status,KevStatus::NotListed);
    for(const auto& key:QStringList{"catalogVersion","dateReleased","count","vulnerabilities"}){auto o=good;o.remove(key);QVERIFY(!RiskEvidence::parseKev(json(o),now()));}
    for(const auto& key:kevEntry().keys()){auto row=kevEntry();row.remove(key);QVERIFY2(!RiskEvidence::parseKev(json(catalog({row})),now()),qPrintable(key));}
    auto row=kevEntry();row["knownRansomwareCampaignUse"]="Unknown";QVERIFY(RiskEvidence::parseKev(json(catalog({row})),now()));row["knownRansomwareCampaignUse"]=42;QVERIFY(!RiskEvidence::parseKev(json(catalog({row})),now()));
    QVERIFY(!RiskEvidence::parseKev(json(catalog({kevEntry(),kevEntry()})),now()));
    for(const auto& change:QList<QPair<QString,QJsonValue>>{{"count",2},{"count",-1},{"dateReleased","yesterday"},{"catalogVersion",""}}){auto o=good;o[change.first]=change.second;QVERIFY(!RiskEvidence::parseKev(json(o),now()));}
    QVERIFY(!RiskEvidence::parseKev("not JSON",now()));QVERIFY(!RiskEvidence::parseKev(QByteArray(RiskEvidence::MaxKevBytes+1,'x'),now()));
}
void Phase09Test::kevCache(){
    QTemporaryDir dir;KevCache cache(dir.path());QCOMPARE(cache.read().error,QueryError::CacheMiss);
    for(int days:{0,-2}){auto s=*RiskEvidence::parseKev(json(catalog()),now().addDays(days));QCOMPARE(cache.write(s),QueryError::None);auto r=cache.read();QVERIFY(r.snapshot);
        QCOMPARE(r.error,QueryError::None);QCOMPARE(r.snapshot->catalog,s.catalog);QCOMPARE(r.snapshot->entries,s.entries);QCOMPARE(r.snapshot->fetchedAt,s.fetchedAt);
        auto e=RiskEvidence::kevEvidence(*r.snapshot,"CVE-2026-9999999",EvidenceAcquisition::Cache,now());QCOMPARE(e.status,KevStatus::NotListed);QCOMPARE(e.freshness,days?EvidenceFreshness::Stale:EvidenceFreshness::Fresh);
        RiskEvidenceProfile p;p.kev={e};if(days)QVERIFY(RiskEvidence::profileText(p).contains("NotListed in Stale Complete KEV Snapshot"));}
    const auto old=read(cache.filePath());auto s=*RiskEvidence::parseKev(json(catalog()),now().addDays(1));QCOMPARE(cache.write(s),QueryError::CacheInvalid);QCOMPARE(read(cache.filePath()),old);
    auto o=QJsonDocument::fromJson(old).object();o["fetchedAt"]=now().addDays(1).toString(Qt::ISODateWithMs);QVERIFY(write(cache.filePath(),json(o)));QCOMPARE(cache.read().error,QueryError::CacheInvalid);
    QVERIFY(write(cache.filePath(),"{"));QCOMPARE(cache.read().error,QueryError::CacheInvalid);
}
void Phase09Test::dependencyStates(){
    auto g=graph();auto d=RiskEvidence::dependency(g,"root");QVERIFY(d);QCOMPARE(d->path,DependencyPathState::RootComponentSelf);QCOMPARE(d->depthFromRoot.value(),0);
    d=RiskEvidence::dependency(g,"a");QCOMPARE(d->path,DependencyPathState::ResolvedPathFound);QCOMPARE(d->depthFromRoot.value(),1);QCOMPARE(d->directDependents.value(),1);
    d=RiskEvidence::dependency(g,"b");QCOMPARE(d->depthFromRoot.value(),2);QCOMPARE(d->directDependents.value(),1);QCOMPARE(d->transitiveDependents.value(),2);QCOMPARE(d->referenceResolution.value(),ReferenceResolutionCompleteness::Complete);
    d=RiskEvidence::dependency(g,"x");QCOMPARE(d->path,DependencyPathState::NoResolvedPath);QVERIFY(!d->depthFromRoot);
    g.entries[0].targets.append({"bad","e1",1,"unknown"});d=RiskEvidence::dependency(g,"b");QCOMPARE(d->path,DependencyPathState::ResolvedPathFound);QCOMPARE(d->referenceResolution.value(),ReferenceResolutionCompleteness::Partial);
    g.components[0].sourceRole=ComponentSourceRole::Component;QCOMPARE(RiskEvidence::dependency(g,"b")->path,DependencyPathState::RootMissing);
    g.components[0].sourceRole=g.components[1].sourceRole=ComponentSourceRole::MetadataRoot;QCOMPARE(RiskEvidence::dependency(g,"b")->path,DependencyPathState::RootAmbiguous);
    g.captured=false;d=RiskEvidence::dependency(g,"b");QCOMPARE(d->path,DependencyPathState::NotCaptured);QVERIFY(!d->directDependents);QVERIFY(!d->referenceResolution);
    QVERIFY(!RiskEvidence::dependency(g,"gone"));
}

void Phase09Test::transport(){
    for(const auto& pair:QList<QPair<Plan,QueryError>>{
        {{"{}",302},QueryError::UnexpectedRedirect},{{"{}",429},QueryError::RateLimited},{{"{}",503},QueryError::ServiceUnavailable},
        {{"{}",200,QNetworkReply::TimeoutError},QueryError::Timeout},{{"{}",200,QNetworkReply::NoError,false,true},QueryError::TlsFailure},
        {{"{}",200,QNetworkReply::ConnectionRefusedError},QueryError::ConnectionFailure},{{"{}",200,QNetworkReply::NoError,false,false,999},QueryError::ResponseInvalid},
        {{QByteArray(2048,'x'),200},QueryError::ResponseLimitExceeded}}){
        Network net;net.epssPlans={pair.first};EvidenceGet client(nullptr,&net);QSignalSpy spy(&client,&EvidenceGet::finished);
        QVERIFY(client.start(QUrl(RiskEvidence::EpssEndpoint),1024));QTRY_COMPARE(spy.size(),1);QCOMPARE(qvariant_cast<QueryError>(spy.first()[0]),pair.second);
        QVERIFY(spy.first()[1].toByteArray().isEmpty());QCOMPARE(net.requests.size(),1);
    }
    Network net;net.epssPlans={{"",200,QNetworkReply::NoError,true}};EvidenceGet client(nullptr,&net);QSignalSpy spy(&client,&EvidenceGet::finished);
    QVERIFY(!client.start(QUrl("http://api.first.org/data/v1/epss"),1024));QVERIFY(!client.start(QUrl("https://example.test/"),1024));
    QVERIFY(client.start(QUrl(RiskEvidence::EpssEndpoint),1024,5));QTRY_COMPARE(spy.size(),1);QCOMPARE(qvariant_cast<QueryError>(spy.first()[0]),QueryError::Timeout);
    Network large;large.kevPlans={{QByteArray(RiskEvidence::MaxKevBytes+1,'x')}};KevClient kev(nullptr,&large);QSignalSpy k(&kev,&KevClient::finished);
    QVERIFY(kev.fetch());QTRY_COMPARE(k.size(),1);QCOMPARE(qvariant_cast<KevFetchResult>(k.first()[0]).error,QueryError::ResponseLimitExceeded);
}
void Phase09Test::privacy(){
    Network net;net.automatic=true;EpssClient e(nullptr,&net);KevClient k(nullptr,&net);QSignalSpy a(&e,&EpssClient::finished),b(&k,&KevClient::finished);
    QVERIFY(e.fetch({CVE}));QVERIFY(k.fetch());QTRY_COMPARE(a.size(),1);QTRY_COMPARE(b.size(),1);QCOMPARE(net.requests.size(),2);
    for(const auto& request:net.requests){
        QCOMPARE(request.url().scheme(),QString("https"));QCOMPARE(request.attribute(QNetworkRequest::RedirectPolicyAttribute).toInt(),int(QNetworkRequest::ManualRedirectPolicy));
        QVERIFY(!request.hasRawHeader("Authorization"));QVERIFY(!request.hasRawHeader("Cookie"));
        if(request.url().host()=="api.first.org"){
            QUrlQuery q(request.url());QCOMPARE(q.queryItems().size(),3);QCOMPARE(q.queryItemValue("cve"),CVE);QCOMPARE(q.queryItemValue("offset"),QString("0"));QCOMPARE(q.queryItemValue("limit"),QString("1"));
        }else{QCOMPARE(request.url(),QUrl(RiskEvidence::KevEndpoint));QVERIFY(request.url().query().isEmpty());}
    }
}
// Use the authoritative Phase08 owner in every service/UI integration test.
#define LOAD_OWNER(c,p,owner) \
    owner.setProject(p);owner.reload();QTRY_VERIFY(!owner.loading());QVERIFY(!owner.rows().isEmpty());owner.select(0); \
    owner.query(QueryMode::CacheOnly);QTRY_VERIFY(!owner.busy());QVERIFY(owner.riskEvidenceRequest(0)); \
    auto& risk=owner.riskEvidence();risk.setRequest(owner.riskEvidenceRequest(0))
void Phase09Test::cacheOnly(){
    Context c;QVERIFY(c.open());auto p=c.project();QVERIFY(c.apply(p));QVERIFY(c.seed());Network net;
    VulnerabilityController owner(c.path(),c.cache(),c.logger,nullptr,nullptr,&net);LOAD_OWNER(c,p,owner);
    risk.load(EvidenceLoadMode::CacheOnly);QTRY_VERIFY(!risk.busy());QVERIFY(risk.profile());QCOMPARE(risk.profile()->epss[0].status,EpssStatus::Failed);QCOMPARE(risk.profile()->kev[0].status,KevStatus::Unknown);QVERIFY(net.requests.isEmpty());
    auto noCve=exact();noCve.record["aliases"]=QJsonArray{};QVERIFY(c.seed({noCve}));owner.query(QueryMode::CacheOnly);QTRY_VERIFY(!owner.busy());risk.setRequest(owner.riskEvidenceRequest(0));
    risk.load(EvidenceLoadMode::PreferCache);QTRY_VERIFY(!risk.busy());QVERIFY(risk.profile());QCOMPARE(risk.profile()->epss[0].status,EpssStatus::NotQueryable);QCOMPARE(risk.profile()->epss[0].freshness,EvidenceFreshness::NotApplicable);QCOMPARE(risk.profile()->epss[0].acquisition,EvidenceAcquisition::None);QVERIFY(!risk.profile()->epss[0].probability);QCOMPARE(risk.profile()->kev[0].status,KevStatus::NotQueryable);QVERIFY(net.requests.isEmpty());
}
void Phase09Test::consent(){
    Context c;QVERIFY(c.open());auto p=c.project();QVERIFY(c.apply(p));QVERIFY(c.seed());Network net;
    VulnerabilityController owner(c.path(),c.cache(),c.logger,nullptr,nullptr,&net);LOAD_OWNER(c,p,owner);QSignalSpy consent(&risk,&RiskEvidenceController::consentRequested);
    risk.load(EvidenceLoadMode::Refresh);QTRY_COMPARE(consent.size(),1);QCOMPARE(consent.first()[0].toStringList(),QStringList{CVE});QVERIFY(net.requests.isEmpty());risk.consent(false);
    QTRY_VERIFY(!risk.busy());QCOMPARE(risk.state(),EvidenceOperationState::Cancelled);QVERIFY(!risk.profile());QVERIFY(net.requests.isEmpty());
    QCOMPARE(EpssCache(risk.cacheRoot()).write(RiskEvidence::parseEpss(json(epss()),{CVE},now())->first()),QueryError::None);
    QCOMPARE(KevCache(risk.cacheRoot()).write(*RiskEvidence::parseKev(json(catalog()),now())),QueryError::None);
    risk.load(EvidenceLoadMode::PreferCache);QTRY_VERIFY(!risk.busy());QVERIFY(risk.profile());QCOMPARE(consent.size(),1);QVERIFY(net.requests.isEmpty());QCOMPARE(risk.profile()->epss[0].acquisition,EvidenceAcquisition::Cache);
}
void Phase09Test::liveAtomic(){
    Context c;QVERIFY(c.open());auto p=c.project();QVERIFY(c.apply(p));auto record=exact();record.record["severity"]=QJsonArray{severityItem()};QVERIFY(c.seed({record}));Network net;
    net.epssPlans={{json(epss())}};net.kevPlans={{json(catalog()),200,QNetworkReply::NoError,true}};
    VulnerabilityController owner(c.path(),c.cache(),c.logger,nullptr,nullptr,&net);LOAD_OWNER(c,p,owner);
    connect(&risk,&RiskEvidenceController::consentRequested,&risk,[&]{risk.consent(true);});risk.load(EvidenceLoadMode::Refresh);
    QTRY_COMPARE(net.requests.size(),2);QTRY_VERIFY(net.replies[1].isNull());QVERIFY(risk.busy());QVERIFY(!risk.profile());
    QVERIFY(net.replies[0]);net.replies[0]->deliver();QTRY_VERIFY(!risk.busy());QVERIFY(risk.profile());QCOMPARE(risk.state(),EvidenceOperationState::Complete);
    QCOMPARE(risk.profile()->severity.status,SeverityStatus::Present);QCOMPARE(risk.profile()->epss[0].status,EpssStatus::Available);QCOMPARE(risk.profile()->epss[0].acquisition,EvidenceAcquisition::Live);QCOMPARE(risk.profile()->kev[0].status,KevStatus::Listed);
    QVERIFY(EpssCache(risk.cacheRoot()).read(CVE).snapshot);QVERIFY(KevCache(risk.cacheRoot()).read().snapshot);
    const auto generated=risk.profile()->generatedAt;net.epssPlans={{json(epss({}))}};net.kevPlans={{json(catalog({})),200,QNetworkReply::NoError,true}};
    risk.load(EvidenceLoadMode::Refresh);QVERIFY(risk.profile());QCOMPARE(risk.profile()->generatedAt,generated);QTRY_COMPARE(net.requests.size(),4);
    QTRY_VERIFY(net.replies[3].isNull());QCOMPARE(risk.profile()->generatedAt,generated);net.replies[2]->deliver();QTRY_VERIFY(!risk.busy());
    QCOMPARE(risk.profile()->epss[0].status,EpssStatus::NotScored);QVERIFY(!risk.profile()->epss[0].probability);QCOMPARE(risk.profile()->kev[0].status,KevStatus::NotListed);
}
void Phase09Test::partialChunks(){
    Context c;QVERIFY(c.open());auto p=c.project();QVERIFY(c.apply(p));auto record=exact();QJsonArray aliases;QStringList ids;
    for(int i=0;i<310;++i){ids.append(QString("CVE-2026-%1").arg(1000000+i));aliases.append(ids.last());}record.record["aliases"]=aliases;QVERIFY(c.seed({record}));auto chunks=RiskEvidence::epssChunks(ids);QCOMPARE(chunks.size(),3);
    Network net;net.automatic=true;QJsonArray first;for(const auto& id:chunks[0])first.append(score(id));net.epssPlans={{json(epss(first,chunks[0].size()))},{"",503}};
    VulnerabilityController owner(c.path(),c.cache(),c.logger,nullptr,nullptr,&net);LOAD_OWNER(c,p,owner);
    EpssCache cache(risk.cacheRoot());QCOMPARE(cache.write({chunks[1][0],"1.0",now().addDays(-2),score(chunks[1][0])}),QueryError::None);
    QCOMPARE(cache.write({chunks[1][1],"1.0",now().addDays(-2),{}}),QueryError::None);
    connect(&risk,&RiskEvidenceController::consentRequested,&risk,[&]{risk.consent(true);});int published=0;QDateTime publishedAt;
    connect(&risk,&RiskEvidenceController::changed,&risk,[&]{if(risk.profile()){if(publishedAt!=risk.profile()->generatedAt){++published;publishedAt=risk.profile()->generatedAt;}QCOMPARE(risk.profile()->epss.size(),ids.size());}});
    risk.load(EvidenceLoadMode::Refresh);QTRY_VERIFY_WITH_TIMEOUT(!risk.busy(),15000);QVERIFY(risk.profile());QCOMPARE(published,1);QCOMPARE(net.requests.size(),4);
    QHash<QString,EpssEvidence> rows;for(const auto& e:risk.profile()->epss)rows.insert(e.cve,e);
    QCOMPARE(rows[chunks[0][0]].acquisition,EvidenceAcquisition::Live);QCOMPARE(rows[chunks[1][0]].acquisition,EvidenceAcquisition::StaleFallback);
    QCOMPARE(rows[chunks[1][1]].status,EpssStatus::NotScored);QCOMPARE(rows[chunks[1][1]].freshness,EvidenceFreshness::Stale);QVERIFY(!rows[chunks[1][1]].probability);
    QCOMPARE(rows[chunks[1][2]].status,EpssStatus::Failed);QCOMPARE(rows[chunks[2][0]].status,EpssStatus::Available);
}
void Phase09Test::fallback(){
    for(bool scored:{false,true}){
        Context c;QVERIFY(c.open());auto p=c.project();QVERIFY(c.apply(p));QVERIFY(c.seed());Network net;net.epssPlans={{"",503}};net.kevPlans={{"",503}};
        VulnerabilityController owner(c.path(),c.cache(),c.logger,nullptr,nullptr,&net);LOAD_OWNER(c,p,owner);
        EpssCache ec(risk.cacheRoot());KevCache kc(risk.cacheRoot());QCOMPARE(ec.write({CVE,"1.0",now().addDays(-2),scored?score():QJsonObject{}}),QueryError::None);
        QCOMPARE(kc.write(*RiskEvidence::parseKev(json(catalog({})),now().addDays(-2))),QueryError::None);const auto oldE=read(ec.filePath(CVE)),oldK=read(kc.filePath());
        connect(&risk,&RiskEvidenceController::consentRequested,&risk,[&]{risk.consent(true);});risk.load(EvidenceLoadMode::Refresh);QTRY_VERIFY(!risk.busy());QVERIFY(risk.profile());
        const auto& e=risk.profile()->epss[0];QCOMPARE(e.status,scored?EpssStatus::Available:EpssStatus::NotScored);QCOMPARE(e.acquisition,EvidenceAcquisition::StaleFallback);QCOMPARE(e.freshness,EvidenceFreshness::Stale);
        QCOMPARE(risk.profile()->kev[0].status,KevStatus::NotListed);QCOMPARE(risk.profile()->kev[0].acquisition,EvidenceAcquisition::StaleFallback);QCOMPARE(read(ec.filePath(CVE)),oldE);QCOMPARE(read(kc.filePath()),oldK);
        QVERIFY(risk.displayText().contains("StaleFallback"));QVERIFY(risk.displayText().contains("NotListed in Stale Complete KEV Snapshot"));
    }
}
void Phase09Test::cancellation(){
    Context c;QVERIFY(c.open());auto p=c.project();QVERIFY(c.apply(p));QVERIFY(c.seed());Network net;net.automatic=true;
    VulnerabilityController owner(c.path(),c.cache(),c.logger,nullptr,nullptr,&net);LOAD_OWNER(c,p,owner);
    connect(&risk,&RiskEvidenceController::consentRequested,&risk,[&]{risk.consent(true);});risk.load(EvidenceLoadMode::Refresh);QTRY_VERIFY(!risk.busy());QVERIFY(risk.profile());
    const auto old=risk.profile()->generatedAt;const auto oldCache=read(EpssCache(risk.cacheRoot()).filePath(CVE));
    net.epssPlans={{"",200,QNetworkReply::NoError,true}};net.kevPlans={{"",200,QNetworkReply::NoError,true}};
    risk.load(EvidenceLoadMode::Refresh);QTRY_COMPARE(net.requests.size(),4);risk.cancel();QTRY_VERIFY(!risk.busy());QCOMPARE(risk.state(),EvidenceOperationState::Cancelled);QVERIFY(risk.profile());QCOMPARE(risk.profile()->generatedAt,old);QCOMPARE(read(EpssCache(risk.cacheRoot()).filePath(CVE)),oldCache);
    {PoolBlock blocked;risk.load(EvidenceLoadMode::CacheOnly);risk.cancel();QVERIFY(risk.busy());QCOMPARE(risk.profile()->generatedAt,old);}
    QTRY_VERIFY(!risk.busy());QCOMPARE(risk.profile()->generatedAt,old);
    // Finish valid provider parsing, but hold the final dependency/profile worker.
    // Cancelling at this boundary must not replace either cache or the old profile.
    net.epssPlans={{json(epss({})),200,QNetworkReply::NoError,true}};
    net.kevPlans={{json(catalog({})),200,QNetworkReply::NoError,true}};
    const auto oldKev=read(KevCache(risk.cacheRoot()).filePath());
    risk.load(EvidenceLoadMode::Refresh);QTRY_COMPARE(net.requests.size(),6);
    net.replies[4]->deliver();net.replies[5]->deliver();QThreadPool::globalInstance()->waitForDone();
    {PoolBlock blocked;QCoreApplication::processEvents();risk.cancel();QVERIFY(risk.busy());}
    QTRY_VERIFY(!risk.busy());QCOMPARE(risk.state(),EvidenceOperationState::Cancelled);QCOMPARE(risk.profile()->generatedAt,old);
    QCOMPARE(read(EpssCache(risk.cacheRoot()).filePath(CVE)),oldCache);QCOMPARE(read(KevCache(risk.cacheRoot()).filePath()),oldKev);
}
void Phase09Test::lifecycle(){
    for(int change=0;change<7;++change){
        Context c;QVERIFY(c.open());auto p=c.project();QVERIFY(c.apply(p));QVERIFY(c.seed({exact(),exact("1.2.3","SYNTHETIC-SECOND-FINDING")}));Network net;
        net.epssPlans={{"",200,QNetworkReply::NoError,true}};net.kevPlans={{"",200,QNetworkReply::NoError,true}};
        VulnerabilityController owner(c.path(),c.cache(),c.logger,nullptr,nullptr,&net);LOAD_OWNER(c,p,owner);
        connect(&risk,&RiskEvidenceController::consentRequested,&risk,[&]{risk.consent(true);});risk.load(EvidenceLoadMode::Refresh);QTRY_COMPARE(net.requests.size(),2);
        switch(change){case 0:risk.setRequest(owner.riskEvidenceRequest(1));break;case 1:risk.setRequest({});break;
        case 2:owner.select(1);break;case 3:owner.setProject(c.project());break;case 4:owner.reload();break;
        case 5:owner.query(QueryMode::CacheOnly);break;case 6:QVERIFY(c.apply(p));owner.setProject(p);break;}
        QTRY_VERIFY(!risk.busy());QVERIFY(!risk.profile());QVERIFY(risk.displayText().isEmpty());QTRY_VERIFY(!owner.loading()&&!owner.busy());
        for(const auto& reply:net.replies)if(reply)reply->deliver();QCoreApplication::processEvents();QVERIFY(!risk.profile());
    }
}
void Phase09Test::staleDependency(){
    Context c;QVERIFY(c.open());auto p=c.project();QVERIFY(c.apply(p));QVERIFY(c.seed());VulnerabilityController owner(c.path(),c.cache(),c.logger);LOAD_OWNER(c,p,owner);
    {PoolBlock blocked;risk.load(EvidenceLoadMode::CacheOnly);QVERIFY(c.apply(p));}
    QTRY_VERIFY(!risk.busy());QCOMPARE(risk.state(),EvidenceOperationState::Stale);QVERIFY(!risk.profile());
    owner.reload();QTRY_VERIFY(!owner.loading());owner.select(0);owner.query(QueryMode::CacheOnly);QTRY_VERIFY(!owner.busy());risk.setRequest(owner.riskEvidenceRequest(0));
    {PoolBlock blocked;risk.load(EvidenceLoadMode::CacheOnly);owner.select(1);QVERIFY(risk.busy());}
    QTRY_VERIFY(!risk.busy());QVERIFY(!risk.profile());QVERIFY(!risk.hasRequest());
}
void Phase09Test::destruction(){
    Context c;QVERIFY(c.open());auto p=c.project();QVERIFY(c.apply(p));QVERIFY(c.seed());Network net;
    net.epssPlans={{"",200,QNetworkReply::NoError,true}};net.kevPlans={{"",200,QNetworkReply::NoError,true}};
    auto* owner=new VulnerabilityController(c.path(),c.cache(),c.logger,nullptr,nullptr,&net);
    owner->setProject(p);owner->reload();QTRY_VERIFY(!owner->loading());owner->select(0);owner->query(QueryMode::CacheOnly);QTRY_VERIFY(!owner->busy());
    auto& risk=owner->riskEvidence();risk.setRequest(owner->riskEvidenceRequest(0));connect(&risk,&RiskEvidenceController::consentRequested,&risk,[&]{risk.consent(true);});risk.load(EvidenceLoadMode::Refresh);QTRY_COMPARE(net.requests.size(),2);
    delete owner;QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);for(const auto& reply:net.replies)QVERIFY(reply.isNull());
    auto* worker=new VulnerabilityController(c.path(),c.cache(),c.logger);worker->setProject(p);worker->reload();QTRY_VERIFY(!worker->loading());worker->select(0);worker->query(QueryMode::CacheOnly);QTRY_VERIFY(!worker->busy());
    {PoolBlock blocked;worker->riskEvidence().setRequest(worker->riskEvidenceRequest(0));worker->riskEvidence().load(EvidenceLoadMode::CacheOnly);delete worker;}
    QCoreApplication::processEvents();
}
void Phase09Test::ui(){
    Context c;QVERIFY(c.open());auto p=c.project();QVERIFY(c.apply(p));auto record=exact();record.record["severity"]=QJsonArray{severityItem()};QVERIFY(c.seed({record}));Network net;net.automatic=true;
    VulnerabilityController owner(c.path(),c.cache(),c.logger,nullptr,nullptr,&net);VulnerabilityPage page(owner);page.resize(700,550);page.show();
    auto* load=page.findChild<QPushButton*>("riskLoad");QVERIFY(load);QVERIFY(!load->isEnabled());
    LOAD_OWNER(c,p,owner);auto* candidates=page.findChild<QTableView*>("candidateTable");candidates->setCurrentIndex(candidates->model()->index(0,0));QVERIFY(load->isEnabled());
    auto* tabs=page.findChild<QTabWidget*>("evidenceTabs");QCOMPARE(tabs->tabText(0),QString("Finding Evidence"));QCOMPARE(tabs->tabText(1),QString("Risk Evidence"));tabs->setCurrentIndex(1);
    QCOMPARE(EpssCache(risk.cacheRoot()).write({CVE,"1.0",now(),score()}),QueryError::None);QCOMPARE(KevCache(risk.cacheRoot()).write(*RiskEvidence::parseKev(json(catalog()),now())),QueryError::None);
    page.findChild<QPushButton*>("riskCacheOnly")->click();QTRY_VERIFY(!risk.busy());auto* text=page.findChild<QTextBrowser*>("riskEvidence");QVERIFY(text);
    const auto displayed=text->toPlainText();for(const auto& word:QStringList{"Profile Available","Implicit Home Database","SYNTHETIC-P09-1","Available","Listed","CISA Catalog Due Date","Reference Resolution","Quality: Unavailable"})QVERIFY2(displayed.contains(word),qPrintable(word));
    for(const auto& word:QStringList{"Risk Score","Risk Level","High Risk","Medium Risk","Low Risk","RiskAssessmentComplete","RiskEvidenceReady","Runtime Unreachable"})QVERIFY(!displayed.contains(word));
    auto* scroll=page.findChild<QScrollArea*>();QVERIFY(scroll);scroll->ensureWidgetVisible(text);QCoreApplication::processEvents();
    QDir out(QCoreApplication::applicationDirPath());QVERIFY(page.grab().save(out.filePath("phase09-small-ui.png")));
    page.resize(1200,900);QCoreApplication::processEvents();QVERIFY(page.grab().save(out.filePath("phase09-ui.png")));
    page.findChild<QPushButton*>("riskRefresh")->click();
    QTRY_VERIFY(page.findChild<QMessageBox*>("riskConsent"));auto* consent=page.findChild<QMessageBox*>("riskConsent");
    QVERIFY(consent->text().contains(CVE));QVERIFY(!consent->text().contains("synthetic-phase09"));QVERIFY(net.requests.isEmpty());
    consent->reject();QTRY_VERIFY(!risk.busy());QCOMPARE(risk.state(),EvidenceOperationState::Cancelled);QVERIFY(risk.profile());QVERIFY(net.requests.isEmpty());
    owner.select(1);QVERIFY(text->toPlainText().isEmpty());QVERIFY(!load->isEnabled());
}
void Phase09Test::scale(){
    Context c;QVERIFY(c.open());auto p=c.project();SbomDocument doc;doc.metadata.component=SbomComponent{"r","application","root","1",""};
    for(int i=0;i<4000;++i){const auto ref=QString::number(i);doc.components.append({ref,"library","synthetic","1.2.3","pkg:npm/synthetic-phase09@1.2.3"});doc.dependencies.append({i?QString::number(i-1):QString("r"),{ref}});}
    QVERIFY(c.components.replaceForProject(p,doc).ok());auto record=exact();QJsonArray aliases;for(int i=0;i<1200;++i)aliases.append(QString("CVE-2026-%1").arg(1000000+i));record.record["aliases"]=aliases;QVERIFY(c.seed({record}));
    Network net;net.automatic=true;QJsonArray entries;for(int i=0;i<5000;++i)entries.append(kevEntry(QString("CVE-2026-%1").arg(1000000+i)));net.kevPlans={{json(catalog(entries))}};
    VulnerabilityController owner(c.path(),c.cache(),c.logger,nullptr,nullptr,&net);owner.setProject(p);owner.reload();QTRY_VERIFY(!owner.loading());owner.select(4000);owner.query(QueryMode::CacheOnly);QTRY_VERIFY(!owner.busy());QVERIFY(owner.riskEvidenceRequest(0));
    auto& risk=owner.riskEvidence();risk.setRequest(owner.riskEvidenceRequest(0));connect(&risk,&RiskEvidenceController::consentRequested,&risk,[&]{risk.consent(true);});
    int ticks=0;qint64 maxGap=0;QElapsedTimer interval;interval.start();QTimer timer;connect(&timer,&QTimer::timeout,&risk,[&]{++ticks;maxGap=qMax(maxGap,interval.restart());});timer.start(1);QElapsedTimer elapsed;elapsed.start();risk.load(EvidenceLoadMode::Refresh);
    QTRY_VERIFY_WITH_TIMEOUT(!risk.busy(),25000);QVERIFY(risk.profile());QCOMPARE(risk.profile()->epss.size(),1200);QCOMPARE(risk.profile()->kev.size(),1200);QCOMPARE(risk.profile()->dependency.depthFromRoot.value(),4000);QVERIFY(ticks>5);QVERIFY2(maxGap<1000,qPrintable(QString::number(maxGap)));QVERIFY(net.requests.size()>8);
    qInfo()<<"Synthetic 1200 CVEs / 5000 KEV / 4000 dependency nodes ms="<<elapsed.elapsed()<<"GUI ticks="<<ticks<<"maxGap ms="<<maxGap;
}


void Phase09Test::providerFailures(){
    for(bool cached:{false,true})for(int failure=0;failure<3;++failure){
        Context c;QVERIFY(c.open());auto p=c.project();QVERIFY(c.apply(p));QVERIFY(c.seed());Network net;
        Plan plan;plan.body="{";if(failure==1)plan.error=QNetworkReply::TimeoutError;if(failure==2)plan.status=302;net.epssPlans={plan};net.kevPlans={plan};
        VulnerabilityController owner(c.path(),c.cache(),c.logger,nullptr,nullptr,&net);LOAD_OWNER(c,p,owner);
        if(cached){QCOMPARE(EpssCache(risk.cacheRoot()).write({CVE,"1.0",now(),score()}),QueryError::None);QCOMPARE(KevCache(risk.cacheRoot()).write(*RiskEvidence::parseKev(json(catalog()),now())),QueryError::None);}
        const auto before=read(EpssCache(risk.cacheRoot()).filePath(CVE));
        connect(&risk,&RiskEvidenceController::consentRequested,&risk,[&]{risk.consent(true);});risk.load(EvidenceLoadMode::Refresh);QTRY_VERIFY(!risk.busy());QVERIFY(risk.profile());
        auto e=risk.profile()->epss[0];QVERIFY(e.status!=EpssStatus::NotScored);QVERIFY(e.error!=QueryError::None);
        if(cached){QCOMPARE(e.status,EpssStatus::Available);QCOMPARE(e.freshness,EvidenceFreshness::Stale);QCOMPARE(e.acquisition,EvidenceAcquisition::StaleFallback);QCOMPARE(risk.profile()->kev[0].status,KevStatus::Listed);}
        else{QCOMPARE(e.status,failure==0?EpssStatus::InvalidResponse:EpssStatus::Failed);QCOMPARE(risk.profile()->kev[0].status,KevStatus::Unknown);QVERIFY(!e.probability);}
        QCOMPARE(read(EpssCache(risk.cacheRoot()).filePath(CVE)),before);
    }
}
void Phase09Test::projectApply(){
    Context c;QVERIFY(c.open());const auto id=c.project();QVERIFY(c.apply(id));QVERIFY(c.seed());
    ProjectPage widget(c.projects,c.components,c.logger,c.cache());widget.resize(1100,850);widget.show();
    widget.findChild<QListWidget*>("projectList")->setCurrentRow(0);widget.findChild<QTabWidget*>("projectTabs")->setCurrentIndex(3);
    auto* owner=widget.findChild<VulnerabilityController*>();QVERIFY(owner);QTRY_VERIFY(owner->loaded());owner->select(0);owner->query(QueryMode::CacheOnly);QTRY_VERIFY(!owner->busy());
    auto& risk=owner->riskEvidence();risk.setRequest(owner->riskEvidenceRequest(0));risk.load(EvidenceLoadMode::CacheOnly);QTRY_VERIFY(!risk.busy());QVERIFY(risk.profile());const auto key=risk.profile()->key;
    widget.findChild<QPushButton*>("importSbom")->click();auto* dialog=widget.findChild<SbomImportDialog*>();QVERIFY(dialog);
    QPointer<QFileDialog> picker=dialog->findChild<QFileDialog*>();QVERIFY(picker);picker->reject();QTRY_VERIFY(picker.isNull());
    const auto file=c.dir.filePath("replacement.json");QVERIFY(write(file,json({{"bomFormat","CycloneDX"},{"specVersion","1.6"},{"components",QJsonArray{QJsonObject{{"name","synthetic replacement"},{"purl","pkg:npm/synthetic-phase09@1.2.3"}}}}})));
    QSignalSpy imported(dialog,&SbomImportDialog::importFinished),applied(dialog,&SbomImportDialog::applyFinished);dialog->importFile(file);QTRY_COMPARE(imported.size(),1);QVERIFY(imported[0][0].toBool());QVERIFY(risk.profile());QCOMPARE(risk.profile()->key,key);
    QSqlQuery sql(c.db.connection());QVERIFY(sql.exec("CREATE TRIGGER fail_phase09 BEFORE INSERT ON components BEGIN SELECT RAISE(ABORT,'synthetic'); END"));dialog->applyToProject();QTRY_COMPARE(applied.size(),1);QVERIFY(!applied[0][0].toBool());QVERIFY(risk.profile());QCOMPARE(risk.profile()->key,key);
    QVERIFY(sql.exec("DROP TRIGGER fail_phase09"));dialog->applyToProject();QTRY_COMPARE(applied.size(),2);QVERIFY(applied[1][0].toBool());QTRY_VERIFY(owner->loaded());QVERIFY(!risk.profile());QVERIFY(!risk.hasRequest());QVERIFY(!owner->applicability());
    QVERIFY(sql.exec("SELECT value FROM app_meta WHERE key='schema_version'"));QVERIFY(sql.next());QCOMPARE(sql.value(0).toInt(),4);sql.finish();
    QVERIFY(sql.exec("SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'"));QVERIFY(sql.next());QCOMPARE(sql.value(0).toInt(),6);sql.finish();dialog->reject();
}
void Phase09Test::manualFixtures(){
    QDir out(QCoreApplication::applicationDirPath());const QString session="phase09-manual-"+QUuid::createUuid().toString(QUuid::WithoutBraces);
    QStringList launchers;
    for(int fixture=0;fixture<5;++fixture){
        const auto letter=QString(QChar('A'+fixture));const auto base=session+"/"+letter;AppPaths paths{out.filePath(base)};QString error;QVERIFY(paths.initialize(error));
        AppDatabase db;AppLogger logger;QVERIFY(logger.open(paths.logFile()));QVERIFY(db.open(paths.databaseFile(),error));ProjectRepository projects(db,logger);ComponentRepository components(db);Project project;
        QVERIFY(projects.create("Phase09 synthetic Finding "+letter,{},project).ok());
        SbomDocument document;document.metadata.component=SbomComponent{"root","application","synthetic root","1.0.0",""};
        document.components.append({"target","library","Finding "+letter,"1.2.3","pkg:npm/synthetic-phase09@1.2.3"});
        if(fixture==1){document.components.append({"mid","library","synthetic intermediate","1",""});document.dependencies={{"root",{"mid"}},{"mid",{"target"}}};}
        else document.dependencies={{"root",{"target"}}};
        if(fixture==3)document.dependencies.append(SbomDependency{"target",{"undeclared-target"}});
        QVERIFY(components.replaceForProject(project.id,document).ok());
        if(fixture==2){QSqlQuery query(db.connection());query.prepare("DELETE FROM dependency_targets WHERE dependency_entry_id IN (SELECT id FROM dependency_entries WHERE project_id=?)");query.addBindValue(project.id);QVERIFY(query.exec());query.prepare("DELETE FROM dependency_entries WHERE project_id=?");query.addBindValue(project.id);QVERIFY(query.exec());query.prepare("DELETE FROM dependency_capture WHERE project_id=?");query.addBindValue(project.id);QVERIFY(query.exec());}
        auto record=exact("1.2.3","SYNTHETIC-P09-MULTI-DASH-"+letter);const QString cve=QString("CVE-2026-%1").arg(1000001+fixture);
        record.record["aliases"]=fixture==2?QJsonArray{}:QJsonArray{cve};
        if(fixture!=3)record.record["severity"]=QJsonArray{severityItem()};
        if(fixture==1||fixture==4){auto a=record.record["affected"].toArray();auto entry=a[0].toObject();auto item=severityItem("CVSS_V3");item["score"]="CVSS:3.1/AV:N/AC:L/PR:N/UI:N/S:U/C:H/I:H/A:H";item["source"]="https://example.test/synthetic-provider";entry["severity"]=QJsonArray{item};a[0]=entry;record.record["affected"]=a;if(fixture==1)record.record.remove("severity");}
        QCOMPARE(OsvCache(paths.osvCacheDirectory()).write({identity(),now(),{record}}),QueryError::None);
        const auto cache=QDir(paths.root).filePath("cache/risk-evidence-v1");const auto fetched=now().addDays(fixture==3?-2:0);
        if(fixture!=2){QCOMPARE(EpssCache(cache).write({cve,"1.0",fetched,fixture==4?QJsonObject{}:score(cve)}),QueryError::None);
            QCOMPARE(KevCache(cache).write(*RiskEvidence::parseKev(json(catalog(fixture==0?QJsonArray{kevEntry(cve)}:QJsonArray{})),fetched)),QueryError::None);}
        const auto filename="phase09-"+letter+".cmd";
        const auto launch=QStringLiteral("@echo off\r\nset \"PATH=%1;%2;%PATH%\"\r\n\"%~dp0SupplyChainRiskAssessment.exe\" --data-dir \"%~dp0%3\"\r\n")
            .arg(QDir::toNativeSeparators(QLibraryInfo::path(QLibraryInfo::BinariesPath)),QStringLiteral(PHASE09_COMPILER_BIN),QDir::toNativeSeparators(base));
        QVERIFY(write(out.filePath(filename),launch.toLocal8Bit()));launchers.append(filename);
        QJsonArray comps;for(const auto& component:document.components)comps.append(QJsonObject{{"bom-ref",component.bomRef},{"type",component.type},{"name",component.name},{"version",component.version},{"purl",component.purl}});
        QJsonArray deps;for(const auto& dep:document.dependencies)deps.append(QJsonObject{{"ref",dep.ref},{"dependsOn",QJsonArray::fromStringList(dep.dependsOn)}});
        QVERIFY(write(out.filePath("phase09-"+letter+"-sbom.json"),json({{"bomFormat","CycloneDX"},{"specVersion","1.6"},{"version",1},{"metadata",QJsonObject{{"component",QJsonObject{{"bom-ref","root"},{"type","application"},{"name","synthetic root"},{"version","1.0.0"}}}}},{"components",comps},{"dependencies",deps}})));
    }
    QVERIFY(write(out.filePath("phase09-empty-sbom.json"),json({{"bomFormat","CycloneDX"},{"specVersion","1.6"},{"version",1},{"components",QJsonArray{}}})));
    const auto checklist=QStringLiteral(
        "# Phase 09 Debug GUI Manual Acceptance Checklist\n\nPhase 09 Manual Acceptance: PENDING USER\n\n"
        "所有项目、组件、OSV、EPSS、KEV 都是 synthetic fixture，不代表真实漏洞。各启动器使用独立新目录，不覆盖用户数据库。\n"
        "普通 CTest 只使用 fake transport。下列手工在线刷新步骤仅发送显示的合成 CVE ID，KEV 下载公开目录。\n\n"
        "通用操作：运行对应 phase09-A/B/C/D/E.cmd → 项目 → 选择 Phase09 synthetic Finding 项目 → 漏洞匹配 → 选择 Finding A/B/C/D/E 组件 → 仅本地缓存 → 选择 Candidate → Risk Evidence → Cache Only。\n"
        "1. A：TopLevel CVSS_V4，Implicit Home Database，完整多横杠 OSV ID；EPSS Available/Fresh/Cache；KEV Listed/Fresh/Cache，显示 CISA Catalog Due Date、Required Action 及 provider-only 说明；depth=1，Reference Resolution Complete。\n"
        "2. B：MatchingAffected CVSS_V3，Explicit Source；EPSS Available；KEV NotListed in Complete KEV Snapshot；depth=2。\n"
        "3. C：有 Severity，无 CVE；EPSS 与 KEV NotQueryable，Freshness NotApplicable，Acquisition None，无概率；Dependency NotCaptured；Quality Unavailable。\n"
        "4. D：Severity Missing；Cache Only 显示 EPSS Stale/Cache、KEV Stale Complete Catalog；ResolvedPathFound(depth=1)+Partial。\n"
        "   断开网络后点击 Refresh Online，在 CVE-only 确认框选择发送，等待超时或连接失败。预期旧 EPSS Available/Stale/StaleFallback、KEV stale NotListed/StaleFallback，原始 fetchedAt 不变。网络可用时结果会依真实公开服务变化，不能用该方式判断 fixture 预期。\n"
        "5. E：Severity SchemaConflict，其他证据仍有结果；EPSS NotScored/Fresh/Cache，无概率0；KEV NotListed；Dependency 正常。\n"
        "6. Fresh 在生成24小时后自然变 Stale；Cache Only 仍可读。需要新 Fresh fixture 可重新运行 Phase09.manualFixtures；新目录独立保留原验收数据。\n"
        "7. A 的 Refresh Online 确认框只列 CVE，点击取消，预期 Cancelled 且保留上次完整 Profile，不发送 EPSS。再次尝试且操作进行时点 Cancel，同样保留旧完整 Profile。\n"
        "8. 切换 Candidate/组件/项目、重读组件、重新加载 OSV 快照后，旧 Risk Evidence 清空且无 Finding 时按钮禁用。网络中的旧回复不能填回新选择。\n"
        "9. 导入 phase09-empty-sbom.json：仅预览时原状态不变；Apply 后组件和旧 Finding/Profile 清空。导入对应 phase09-A/B/D/E-sbom.json 并 Apply，再仅本地查询重新生成 Finding。C 的 SBOM Apply 后是 Captured，这与导入前 legacy NotCaptured fixture 的区别符合预期。\n"
        "10. 大窗口与小窗口检查 Finding Evidence / Risk Evidence 标签、按钮、状态及内外滚动。没有 Risk Score、High/Medium/Low Risk、Safe 或本地 SLA 结论。\n"
        "11. 关闭并重启后仅从当前状态、OSV 原始缓存和 Provider 原始缓存重新派生 Profile；不持久化 Finding/Profile/Quality。\n\n"
        "自动测试包含取消、切换、Apply、后台销毁、分批失败及 stale replies；这些自动结果不能代替用户人工验收。\n\nPhase 09 Manual Acceptance: PENDING USER\n");
    QVERIFY(write(out.filePath("phase09-manual-checklist.md"),checklist.toUtf8()));
}
QTEST_MAIN(Phase09Test)
#include "Phase09Test.moc"
