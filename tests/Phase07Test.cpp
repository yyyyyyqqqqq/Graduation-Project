#include "PackageIdentity.h"
#include "OsvCache.h"
#include "OsvClient.h"
#include "VulnerabilityController.h"
#include "VulnerabilityPage.h"
#include "AppPaths.h"
#include "AppDatabase.h"
#include "AppLogger.h"
#include "ComponentRepository.h"
#include "ProjectRepository.h"
#include "ProjectPage.h"
#include "SbomImportDialog.h"
#include "CycloneDxParser.h"
#include <QTest>
#include <QSignalSpy>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTemporaryDir>
#include <QFile>
#include <QFileDialog>
#include <QDir>
#include <QLabel>
#include <QPushButton>
#include <QTableView>
#include <QTabWidget>
#include <QListWidget>
#include <QTextBrowser>
#include <QMessageBox>
#include <QScrollBar>
#include <QScrollArea>
#include <QElapsedTimer>
#include <QAbstractEventDispatcher>
#include <QThreadPool>
#include <QSemaphore>
#include <QScopeGuard>
#include <QSqlQuery>
#include <QtConcurrentRun>
#include <cstring>

namespace {
QByteArray json(const QJsonObject& o) { return QJsonDocument(o).toJson(QJsonDocument::Compact); }
QJsonObject record(QString id="SYNTHETIC-1",QString modified="2026-01-01T00:00:00Z") { return {{"id",id},{"modified",modified}}; }
QByteArray page(const QJsonArray& records={},const QString& token={}) {
    QJsonObject o{{"vulns",records}};if(!token.isEmpty())o["next_page_token"]=token;return json(o);
}
Component component(QString purl="pkg:pypi/synthetic-sensitive-pkg@1.0",QString version={}) {
    Component c;c.id="synthetic-private-uuid";c.name="synthetic-sensitive-display";c.bomRef="synthetic-private-ref";c.purl=purl;c.version=version;return c;
}
QueryIdentity identity() { return {"PyPI","synthetic-sensitive-pkg","1.0",1}; }
bool write(const QString& path,const QByteArray& bytes) { QFile f(path);return f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(); }
QByteArray read(const QString& path) { QFile f(path);if(!f.open(QIODevice::ReadOnly))return {};return f.readAll(); }
struct Response { QByteArray body="{}";int status=200;QNetworkReply::NetworkError error=QNetworkReply::NoError;bool hold=false;QByteArray retryAfter; };
class ControlledReply final : public QNetworkReply {
public:
    ControlledReply(const QNetworkRequest& request,Response response,QObject* parent):QNetworkReply(parent),response(std::move(response)) {
        setRequest(request);setUrl(request.url());setOperation(QNetworkAccessManager::PostOperation);open(QIODevice::ReadOnly|QIODevice::Unbuffered);
        if(!this->response.hold)QMetaObject::invokeMethod(this,[this]{release();},Qt::QueuedConnection);
    }
    void release() {
        if(isFinished())return;
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute,response.status);
        if(!response.retryAfter.isEmpty())setRawHeader("Retry-After",response.retryAfter);
        if(response.error!=NoError)setError(response.error,"SYNTHETIC-SECRET-NETWORK-DIAGNOSTIC");
        available=true;emit readyRead();if(isFinished())return;setFinished(true);emit finished();
    }
    void abort() override { if(isFinished())return;setError(OperationCanceledError,"aborted");setFinished(true);emit finished(); }
    qint64 bytesAvailable() const override {return (available?response.body.size()-offset:0)+QNetworkReply::bytesAvailable();}
protected:
    qint64 readData(char* data,qint64 maximum) override {
        if(!available)return 0;const auto count=qMin(maximum,qint64(response.body.size()-offset));
        if(count<=0)return -1;std::memcpy(data,response.body.constData()+offset,size_t(count));offset+=count;return count;
    }
private:
    Response response;qint64 offset=0;bool available=false;
};
class FakeNetwork final : public QNetworkAccessManager {
public:
    QList<Response> responses;QList<QByteArray> bodies;QList<QNetworkRequest> requests;QList<QPointer<ControlledReply>> replies;
protected:
    QNetworkReply* createRequest(Operation operation,const QNetworkRequest& request,QIODevice* outgoing) override {
        if(operation!=PostOperation)qFatal("Unexpected operation");
        requests.append(request);bodies.append(outgoing?outgoing->readAll():QByteArray());
        if(responses.isEmpty())qFatal("Unexpected request: offline tests must explicitly supply a response");
        auto* reply=new ControlledReply(request,responses.takeFirst(),this);replies.append(reply);return reply;
    }
};
struct Context {
    QTemporaryDir dir;AppLogger logger;AppDatabase db;ProjectRepository projects{db,logger};ComponentRepository components{db};QString error;
    QString database()const{return dir.filePath("synthetic.db");}QString cache()const{return dir.filePath("cache/osv-v1");}
    bool open(){return logger.open(dir.filePath("test.log"))&&db.open(database(),error);}
    QString project(){Project p;if(!projects.create("synthetic project","synthetic-private-description",p).ok())qFatal("fixture");return p.id;}
    bool apply(const QString& id,int count=2){SbomDocument d;for(int n=0;n<count;++n)d.components.append({QString::number(n),"library","synthetic-sensitive-display","1.0","pkg:pypi/synthetic-sensitive-pkg@1.0"});return components.replaceForProject(id,d).ok();}
};
}
class Phase07Test : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() { QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); }
    void identityRules();void identityInvalid();void versions();void parser();void parserInvalid();void merge();
    void networkErrors();void pagination();void paginationFailure();void limits();void cancellation();void requestPrivacy();
    void cache();void cacheInvalid();void cacheAtomicClear();void controllerStates();void cacheRefresh();void staleResults();
    void clearLifecycle();void workerLifecycle();void hiddenProjectSwitch();void destruction();void ui();void projectIntegration();void largeInput();void manualFixtures();
    void liveSmoke();
};
void Phase07Test::identityRules() {
    auto c=component("PKG:PyPI/Friendly__Bard@1.0");const auto before=c;auto i=PackageIdentity::resolve(c);
    QCOMPARE(c,before);QCOMPARE(i.state,IdentityState::Resolved);QCOMPARE(i.name,"friendly-bard");QVERIFY(i.nameNormalized);QCOMPARE(i.ecosystem,"PyPI");
    i=PackageIdentity::resolve(component("pkg:npm/Mixed.Name@v1.2.3"));QCOMPARE(i.name,"Mixed.Name");QCOMPARE(i.ecosystem,"npm");
    i=PackageIdentity::resolve(component("pkg:npm/%40scope/name@1.0%2Bbuild"));QCOMPARE(i.name,"@scope/name");QCOMPARE(i.version,"1.0+build");
    i=PackageIdentity::resolve(component("pkg:pypi/%66riendly%2Dbard@1.0+local"));QCOMPARE(i.name,"friendly-bard");QCOMPARE(i.version,"1.0+local");
    i=PackageIdentity::resolve(component("pkg:pypi/a@%2520"));QCOMPARE(i.version,"%20");
}
void Phase07Test::identityInvalid() {
    const QList<QPair<QString,IdentityReason>> cases{
        {{},IdentityReason::MissingPurl},{"  ",IdentityReason::MissingPurl},{"x",IdentityReason::MalformedPurl},
        {" pkg:pypi/a@1",IdentityReason::MalformedPurl},{"pkg:maven/a@1",IdentityReason::UnsupportedEcosystem},
        {"pkg:pypi/a@1?repository_url=x",IdentityReason::UnsupportedQualifiers},{"pkg:npm/a@1#x",IdentityReason::UnsupportedSubpath},
        {"pkg:pypi/a%2Fb@1",IdentityReason::MalformedPurl},{"pkg:pypi/%@1",IdentityReason::MalformedPurl},
        {"pkg:pypi/%GG@1",IdentityReason::MalformedPurl},{"pkg:pypi/%C3%28@1",IdentityReason::MalformedPurl},
        {"pkg:pypi/a@%FF",IdentityReason::MalformedPurl},{"pkg:pypi/a b@1",IdentityReason::MalformedPurl},
        {"pkg:pypi/scope/a@1",IdentityReason::MalformedPurl},{"pkg:npm/@scope/a@1",IdentityReason::MalformedPurl},
        {"pkg:npm/%40scope/a/b@1",IdentityReason::MalformedPurl},{"pkg:pypi/@1",IdentityReason::MalformedPurl},
        {"pkg:pypi/a@1@2",IdentityReason::MalformedPurl},{"pkg:pypi/a@",IdentityReason::MalformedPurl},
        {"pkg:pypi/a@1%00",IdentityReason::InvalidVersionInput},{"pkg:pypi/a@1\n",IdentityReason::MalformedPurl},
        {"pkg:pypi/"+QString(4096,'a'),IdentityReason::PurlTooLong}};
    for(const auto& [purl,reason]:cases){const auto i=PackageIdentity::resolve(component(purl));QVERIFY2(i.reason==reason,qPrintable(purl));QVERIFY(!i.query());}
}
void Phase07Test::versions() {
    auto i=PackageIdentity::resolve(component("pkg:pypi/a@1"));QCOMPARE(i.versionSource,VersionSource::Purl);
    i=PackageIdentity::resolve(component("pkg:pypi/a","1"));QCOMPARE(i.versionSource,VersionSource::Component);
    i=PackageIdentity::resolve(component("pkg:pypi/a@1","1"));QCOMPARE(i.versionSource,VersionSource::Both);
    for(const auto& version:QStringList{"1!2.0","1.0+vendor.1","1.0RC1","v1.2.3","release-2026.09"," 1.0 ","version/opaque"}) {
        i=PackageIdentity::resolve(component("pkg:pypi/a",version));QCOMPARE(i.state,IdentityState::Resolved);QCOMPARE(i.version,version);
    }
    for(const auto& pair:QList<QPair<QString,QString>>{{"1.0","1.0.0"},{"1.0rc1","1.0RC1"},{"1"," 1"}}) {
        i=PackageIdentity::resolve(component("pkg:pypi/a@"+pair.first,pair.second));QCOMPARE(i.state,IdentityState::Ambiguous);QCOMPARE(i.reason,IdentityReason::VersionConflict);
    }
    for(const auto& v:QStringList{"","  ","\t"})QCOMPARE(PackageIdentity::resolve(component("pkg:pypi/a",v)).reason,IdentityReason::MissingVersion);
    QCOMPARE(PackageIdentity::resolve(component("pkg:pypi/a",QString(257,'x'))).reason,IdentityReason::VersionTooLong);
    QCOMPARE(PackageIdentity::resolve(component("pkg:pypi/a","1\n")).reason,IdentityReason::InvalidVersionInput);
    QCOMPARE(PackageIdentity::resolve(component("pkg:pypi/a",QString(QChar(0xd800)))).reason,IdentityReason::InvalidVersionInput);
    QCOMPARE(PackageIdentity::resolve(component("pkg:pypi/a@%201%20"," 1 ")).version,QString(" 1 "));
}
void Phase07Test::parser() {
    QVERIFY(OsvResponseParser::parse("{}").ok());QVERIFY(OsvResponseParser::parse("{\"vulns\":[]}").candidates.isEmpty());
    auto o=record();o["aliases"]=QJsonArray{"OTHER-1","CVE-2026-1234","CVE-2026-1234567"};o["summary"]="<b>plain</b>";
    o["published"]="2025-12-31T00:00:00Z";o["withdrawn"]="2026-01-02T00:00:00.123456789Z";
    o["affected"]=QJsonArray{QJsonObject{{"package",QJsonObject{{"ecosystem","PyPI"},{"name","synthetic"}}},
        {"ranges",QJsonArray{QJsonObject{{"type","ECOSYSTEM"},{"events",QJsonArray{QJsonObject{{"introduced","0"}},QJsonObject{{"fixed","2"}}}}}}},
        {"versions",QJsonArray{"1!2.0"}},{"database_specific",QJsonObject{{"future",true}}}}};o["future-field"]=QJsonArray{1,2};
    auto r=OsvResponseParser::parse(page({o,record("SYNTHETIC-2")}));QVERIFY(r.ok());QCOMPARE(r.candidates.size(),2);
    QCOMPARE(r.candidates[0].record,o);QCOMPARE(r.candidates[0].cveAliases().size(),2);QVERIFY(r.candidates[1].cveAliases().isEmpty());
    o["aliases"]=QJsonArray{"OTHER-1"};QVERIFY(OsvResponseParser::parse(page({o})).candidates[0].cveAliases().isEmpty());
    o["aliases"]=QJsonArray{"CVE-2026-1234"};QCOMPARE(OsvResponseParser::parse(page({o})).candidates[0].cveAliases().size(),1);
}
void Phase07Test::parserInvalid() {
    for(const auto& bytes:QList<QByteArray>{"[", "[]","null","{\"vulns\":null}","{\"vulns\":[1]}","{\"next_page_token\":2}"})QCOMPARE(OsvResponseParser::parse(bytes).error,QueryError::ResponseInvalid);
    const QList<QPair<QString,QJsonValue>> cases{{"id",QJsonValue()},{"id","bad id"},{"modified",42},{"modified","2026-02-30T00:00:00Z"},
        {"aliases",QJsonArray{1}},{"summary",true},{"withdrawn","yesterday"},{"affected",QJsonArray{1}},{"schema_version","2.0.0"},
        {"affected",QJsonArray{QJsonObject{{"ranges",QJsonArray{QJsonObject{{"events",QJsonArray{QJsonObject{{"fixed",42}}}}}}}}}}};
    for(const auto& [key,value]:cases){auto o=record();o[key]=value;QCOMPARE(OsvResponseParser::parse(page({o})).error,QueryError::ResponseInvalid);}
    auto o=record();o.remove("id");QCOMPARE(OsvResponseParser::parse(page({o})).error,QueryError::ResponseInvalid);
    o=record();o.remove("modified");QCOMPARE(OsvResponseParser::parse(page({o})).error,QueryError::ResponseInvalid);
}
void Phase07Test::merge() {
    auto a=record("A","2026-01-01T00:00:00.123456781Z"),b=record("A","2026-01-01T00:00:00.123456789Z");
    QList<VulnerabilityCandidate> list{{a}};QCOMPARE(OsvResponseParser::merge(list,{{b}}),QueryError::None);QCOMPARE(list[0].record,b);
    QCOMPARE(OsvResponseParser::merge(list,{{a}}),QueryError::None);QCOMPARE(list[0].record,b);
    auto conflict=b;conflict["summary"]="different";QCOMPARE(OsvResponseParser::merge(list,{{record("NEW")},{conflict}}),QueryError::ResponseInvalid);QCOMPARE(list.size(),1);
    a["aliases"]=QJsonArray{"CVE-2026-1234"};b=record("B");b["aliases"]=a["aliases"];list.clear();
    QCOMPARE(OsvResponseParser::merge(list,{{a},{b}}),QueryError::None);QCOMPARE(list.size(),2);
    QCOMPARE(OsvResponseParser::compareTimestamp("2026-01-01T00:00:00Z","2026-01-01T00:00:00.000Z"),0);
}
void Phase07Test::networkErrors() {
    const QList<QPair<Response,QueryError>> cases{
        {{"secret",400},QueryError::RequestRejected},{{"secret",401},QueryError::AccessDenied},{{"secret",403},QueryError::AccessDenied},
        {{"secret",404},QueryError::EndpointNotFound},{{"secret",429,QNetworkReply::NoError,false,"30"},QueryError::RateLimited},
        {{"secret",503},QueryError::ServiceUnavailable},{{"secret",302},QueryError::UnexpectedRedirect},
        {{"secret",0,QNetworkReply::TimeoutError},QueryError::Timeout},{{"secret",0,QNetworkReply::HostNotFoundError},QueryError::ConnectionFailure},
        {{"secret",0,QNetworkReply::SslHandshakeFailedError},QueryError::TlsFailure},{{"invalid",200},QueryError::ResponseInvalid}};
    for(const auto& [response,expected]:cases) {
        FakeNetwork network;network.responses={response};OsvClient client(nullptr,&network);QueryError error=QueryError::None;OsvSnapshot snapshot;
        QSignalSpy done(&client,&OsvClient::finished);connect(&client,&OsvClient::finished,this,[&](QueryError e,const OsvSnapshot& s){error=e;snapshot=s;});
        QVERIFY(client.start(identity()));QTRY_COMPARE(done.count(),1);QCOMPARE(error,expected);QVERIFY(snapshot.candidates.isEmpty());QVERIFY(!client.busy());
        if(expected==QueryError::RateLimited){QVERIFY(client.retryNotBefore()>QDateTime::currentDateTimeUtc());QVERIFY(client.start(identity()));QCOMPARE(done.count(),2);QCOMPARE(network.requests.size(),1);}
        QTRY_VERIFY(network.replies[0].isNull());
    }
}
void Phase07Test::pagination() {
    FakeNetwork network;network.responses={{page({record("A")},"first")},{"{\"next_page_token\":\"second\"}"},{page({record("B")})}};
    OsvClient client(nullptr,&network);OsvSnapshot snapshot;QueryError error=QueryError::Database;
    QSignalSpy done(&client,&OsvClient::finished);connect(&client,&OsvClient::finished,this,[&](QueryError e,const OsvSnapshot& s){error=e;snapshot=s;});
    QVERIFY(client.start(identity()));QVERIFY(!client.start(identity()));QTRY_COMPARE(done.count(),1);QCOMPARE(error,QueryError::None);QCOMPARE(snapshot.candidates.size(),2);QCOMPARE(network.requests.size(),3);
    QCOMPARE(QJsonDocument::fromJson(network.bodies[1]).object()["page_token"].toString(),"first");QCOMPARE(QJsonDocument::fromJson(network.bodies[2]).object()["page_token"].toString(),"second");
}
void Phase07Test::paginationFailure() {
    for(bool repeated:{false,true}) {
        FakeNetwork network;network.responses={{page({record()},"same")},{repeated?page({},"same"):QByteArray("error"),repeated?200:503}};
        OsvClient client(nullptr,&network);QueryError error=QueryError::None;QSignalSpy done(&client,&OsvClient::finished);
        connect(&client,&OsvClient::finished,this,[&](QueryError e,const OsvSnapshot& s){error=e;QVERIFY(s.candidates.isEmpty());});
        client.start(identity());QTRY_COMPARE(done.count(),1);QCOMPARE(error,repeated?QueryError::ResponseLimitExceeded:QueryError::ServiceUnavailable);
    }
}
void Phase07Test::limits() {
    QList<QList<Response>> cases;QList<Response> manyPages;for(int n=0;n<20;++n)manyPages.append({page({},QString::number(n))});cases.append(manyPages);
    cases.append(QList<Response>{{QByteArray(OsvResponseParser::MaxResponseBytes+1,' ')}});
    QJsonArray records;for(int n=0;n<10000;++n)records.append(record(QString::number(n)));
    cases.append(QList<Response>{{page(records,"next")},{page({record("last")})}});
    auto duplicateRecords=records;duplicateRecords.append(record("last"));cases.append(QList<Response>{{page(duplicateRecords)}});
    auto padding=record();padding["details"]=QString(9*1024*1024,'a');cases.append(QList<Response>{{page({padding},"next")},{page({padding})}});
    for(const auto& responses:cases) {
        FakeNetwork network;network.responses=responses;OsvClient client(nullptr,&network);QueryError error=QueryError::None;
        QSignalSpy done(&client,&OsvClient::finished);connect(&client,&OsvClient::finished,this,[&](QueryError e,const OsvSnapshot&){error=e;});
        client.start(identity());QTRY_COMPARE_WITH_TIMEOUT(done.count(),1,10000);QCOMPARE(error,QueryError::ResponseLimitExceeded);QVERIFY(network.requests.size()<=20);
    }
}
void Phase07Test::cancellation() {
    FakeNetwork network;network.responses={{page({record()},"next")},{"{}",200,QNetworkReply::NoError,true}};
    OsvClient client(nullptr,&network);QueryError error=QueryError::None;QSignalSpy done(&client,&OsvClient::finished);
    connect(&client,&OsvClient::finished,this,[&](QueryError e,const OsvSnapshot& s){error=e;QVERIFY(s.candidates.isEmpty());});
    client.start(identity());QTRY_COMPARE(network.requests.size(),2);client.cancel();QTRY_COMPARE(done.count(),1);QCOMPARE(error,QueryError::Cancelled);QVERIFY(!client.busy());
    QTRY_VERIFY(network.replies.last().isNull());
}
void Phase07Test::requestPrivacy() {
    FakeNetwork network;network.responses={{"{}"}};OsvClient client(nullptr,&network);QSignalSpy done(&client,&OsvClient::finished);
    client.start(identity());QTRY_COMPARE(done.count(),1);
    QCOMPARE(QJsonDocument::fromJson(network.bodies.first()).object(),QJsonObject({{"package",QJsonObject{{"ecosystem","PyPI"},{"name",identity().name}}},{"version",identity().version}}));
    const auto request=network.requests.first();QCOMPARE(request.url(),QUrl(OsvEndpoint));QCOMPARE(request.transferTimeout(),30000);
    QCOMPARE(request.attribute(QNetworkRequest::RedirectPolicyAttribute).toInt(),int(QNetworkRequest::ManualRedirectPolicy));
    for(const auto& forbidden:QList<QByteArray>{"purl","bom-ref","uuid","description","path","dependencies"})QVERIFY(!network.bodies.first().contains(forbidden));
}
void Phase07Test::cache() {
    QTemporaryDir dir;AppPaths paths{dir.path()};QCOMPARE(paths.osvCacheDirectory(),dir.filePath("cache/osv-v1"));OsvCache cache(paths.osvCacheDirectory());
    QCOMPARE(cache.read(identity()).error,QueryError::CacheMiss);
    const auto now=QDateTime::currentDateTimeUtc();OsvSnapshot snapshot{identity(),now,{}};QCOMPARE(cache.write(snapshot),QueryError::None);
    auto result=OsvCache(paths.osvCacheDirectory()).read(identity(),now);QVERIFY(result.snapshot);QVERIFY(result.fresh);QVERIFY(result.snapshot->candidates.isEmpty());
    QVERIFY(!cache.filePath(identity()).contains(identity().name));QCOMPARE(QFileInfo(cache.filePath(identity())).fileName().size(),69);
    result=cache.read(identity(),now.addSecs(86400));QVERIFY(result.snapshot);QVERIFY(!result.fresh);
    QVERIFY(!cache.read(identity(),now.addSecs(-1)).fresh);
    QVERIFY(!cache.read(identity(),now.addMSecs(-1)).fresh);
    auto changed=identity();changed.version="1.0.0";QCOMPARE(cache.read(changed).error,QueryError::CacheMiss);
}
void Phase07Test::cacheInvalid() {
    QTemporaryDir dir;OsvCache cache(dir.filePath("cache"));QCOMPARE(cache.write({identity(),QDateTime::currentDateTimeUtc(),{{record()}}}),QueryError::None);
    const auto path=cache.filePath(identity());const auto original=read(path);const auto good=QJsonDocument::fromJson(original).object();
    for(const auto& pair:QList<QPair<QString,QJsonValue>>{{"cacheFormatVersion",2},{"complete",false},{"identityRulesVersion",2},{"queryIdentity",QJsonObject{}},{"fetchedAt","bad"},{"vulns",QJsonArray{QJsonObject{}}},{"endpoint","http://invalid"}}) {
        auto o=good;o[pair.first]=pair.second;QVERIFY(write(path,json(o)));QCOMPARE(cache.read(identity()).error,QueryError::CacheInvalid);
    }
    QVERIFY(write(path,"{"));QCOMPARE(cache.read(identity()).error,QueryError::CacheInvalid);
    QFile huge(path);QVERIFY(huge.open(QIODevice::WriteOnly));QVERIFY(huge.resize(OsvCache::MaxFileBytes+1));huge.close();QCOMPARE(cache.read(identity()).error,QueryError::CacheInvalid);
}
void Phase07Test::cacheAtomicClear() {
    QTemporaryDir dir;OsvCache cache(dir.filePath("cache"));OsvSnapshot s{identity(),QDateTime::currentDateTimeUtc(),{{record("old")}}};
    QCOMPARE(cache.write(s),QueryError::None);const auto old=read(cache.filePath(identity()));
    s.candidates={{QJsonObject{}}};QCOMPARE(cache.write(s),QueryError::CacheInvalid);QCOMPARE(read(cache.filePath(identity())),old);
    s.candidates={{record("new")}};QCOMPARE(cache.write(s),QueryError::None);QCOMPARE(cache.read(identity()).snapshot->candidates[0].id(),"new");
    const auto sentinel=dir.filePath("cache/user.json");QVERIFY(write(sentinel,"preserve"));QVERIFY(write(dir.filePath("database.db"),"preserve"));
    QCOMPARE(cache.clear(),QueryError::None);QCOMPARE(cache.read(identity()).error,QueryError::CacheMiss);QCOMPARE(read(sentinel),QByteArray("preserve"));QVERIFY(QFile::exists(dir.filePath("database.db")));
    const auto blocked=dir.filePath("file");QVERIFY(write(blocked,"file"));QCOMPARE(OsvCache(blocked).write(s),QueryError::CacheIo);
}
void Phase07Test::controllerStates() {
    Context c;QVERIFY(c.open());const auto project=c.project();QVERIFY(c.apply(project));FakeNetwork network;
    network.responses={{"private response",400},{"{}"},{page({record()})}};
    VulnerabilityController controller(c.database(),c.cache(),c.logger,nullptr,&network);QSignalSpy consent(&controller,&VulnerabilityController::consentRequested);
    controller.setProject(project);controller.reload();QTRY_VERIFY(controller.loaded());QCOMPARE(controller.rows().size(),2);QCOMPARE(network.requests.size(),0);
    controller.select(0);QCOMPARE(controller.state(),QueryState::NotStarted);QVERIFY(!controller.snapshot());QCOMPARE(network.requests.size(),0);
    controller.query(QueryMode::Refresh);QCOMPARE(controller.state(),QueryState::Working);QTRY_COMPARE(consent.count(),1);QCOMPARE(network.requests.size(),0);
    controller.consent(false);QCOMPARE(controller.state(),QueryState::Cancelled);QCOMPARE(network.requests.size(),0);
    controller.query(QueryMode::Refresh);QTRY_COMPARE(consent.count(),2);controller.consent(true);QTRY_VERIFY(!controller.busy());
    QCOMPARE(controller.state(),QueryState::Failed);QCOMPARE(controller.error(),QueryError::RequestRejected);QCOMPARE(controller.rows()[0].identity.state,IdentityState::Resolved);QVERIFY(!controller.snapshot());
    controller.query(QueryMode::Refresh);QTRY_VERIFY(!controller.busy());QCOMPARE(controller.state(),QueryState::Success);QVERIFY(controller.snapshot()->candidates.isEmpty());
    controller.query(QueryMode::Refresh);QTRY_VERIFY(!controller.busy());QCOMPARE(controller.snapshot()->candidates.size(),1);QCOMPARE(consent.count(),2);
    const auto log=read(c.dir.filePath("test.log"));for(const auto& secret:QList<QByteArray>{"synthetic-sensitive","pkg:pypi","private response","SYNTHETIC-1","1.0","page_token"})QVERIFY(!log.contains(secret));
}
void Phase07Test::cacheRefresh() {
    Context c;QVERIFY(c.open());const auto id=c.project();QVERIFY(c.apply(id));OsvCache cache(c.cache());
    QCOMPARE(cache.write({identity(),QDateTime::currentDateTimeUtc().addDays(-2),{{record("HISTORICAL")}}}),QueryError::None);
    FakeNetwork network;network.responses={{page({record("PARTIAL")},"later")},{"error",503},{"{}"}};
    VulnerabilityController controller(c.database(),c.cache(),c.logger,nullptr,&network);
    connect(&controller,&VulnerabilityController::consentRequested,&controller,[&]{controller.consent(true);});controller.setProject(id);controller.reload();QTRY_VERIFY(controller.loaded());controller.select(0);
    controller.query(QueryMode::CacheOnly);QTRY_VERIFY(!controller.busy());QCOMPARE(controller.state(),QueryState::Success);QCOMPARE(controller.source(),ResultSource::StaleCache);QCOMPARE(network.requests.size(),0);
    const auto old=read(cache.filePath(identity()));controller.query(QueryMode::Refresh);QTRY_VERIFY(!controller.busy());QCOMPARE(controller.state(),QueryState::Failed);QCOMPARE(controller.snapshot()->candidates[0].id(),"HISTORICAL");QCOMPARE(read(cache.filePath(identity())),old);
    controller.query(QueryMode::Refresh);QTRY_VERIFY(!controller.busy());QCOMPARE(controller.source(),ResultSource::Live);QVERIFY(controller.snapshot()->candidates.isEmpty());
    controller.query(QueryMode::PreferCache);QTRY_VERIFY(!controller.busy());QCOMPARE(controller.source(),ResultSource::FreshCache);QCOMPARE(network.requests.size(),3);
}
void Phase07Test::staleResults() {
    Context c;QVERIFY(c.open());const auto id=c.project(),other=c.project();QVERIFY(c.apply(id));QVERIFY(c.apply(other,1));FakeNetwork network;
    network.responses={{page({record()}),200,QNetworkReply::NoError,true},{page({record()}),200,QNetworkReply::NoError,true}};
    VulnerabilityController controller(c.database(),c.cache(),c.logger,nullptr,&network);
    connect(&controller,&VulnerabilityController::consentRequested,&controller,[&]{controller.consent(true);});controller.setProject(id);controller.reload();QTRY_VERIFY(controller.loaded());controller.select(0);
    controller.query(QueryMode::Refresh);QTRY_COMPARE(network.requests.size(),1);controller.select(1);QTRY_VERIFY(!controller.busy());QCOMPARE(controller.state(),QueryState::NotStarted);QVERIFY(!controller.snapshot());
    controller.query(QueryMode::Refresh);QTRY_COMPARE(network.requests.size(),2);controller.setProject(other);controller.reload();QTRY_VERIFY(controller.loaded());QCOMPARE(controller.rows().size(),1);QVERIFY(!controller.snapshot());QCOMPARE(controller.selected(),-1);
    for(int n=0;n<20;++n)controller.reload();QTRY_VERIFY(controller.loaded()&&!controller.loading());QCOMPARE(controller.rows().size(),1);QCOMPARE(network.requests.size(),2);
    QVERIFY(c.projects.remove(other).ok());controller.setProject({});QVERIFY(controller.rows().isEmpty());QVERIFY(!controller.snapshot());
}
void Phase07Test::clearLifecycle() {
    Context c;QVERIFY(c.open());const auto id=c.project();QVERIFY(c.apply(id));FakeNetwork network;
    VulnerabilityController controller(c.database(),c.cache(),c.logger,nullptr,&network);controller.setProject(id);controller.reload();QTRY_VERIFY(controller.loaded());controller.select(0);
    OsvCache cache(c.cache());QCOMPARE(cache.write({identity(),QDateTime::currentDateTimeUtc(),{{record()}}}),QueryError::None);
    // Deterministically queue a cache read behind a blocked worker. Cancellation
    // must not let a clear race that worker; no timing workaround is involved.
    auto* pool=QThreadPool::globalInstance();pool->waitForDone();const auto oldMax=pool->maxThreadCount();pool->setMaxThreadCount(1);
    QSemaphore started,release;auto blocker=QtConcurrent::run([&]{started.release();release.acquire();});started.acquire();
    auto cleanup=qScopeGuard([&]{release.release();blocker.waitForFinished();pool->waitForDone();pool->setMaxThreadCount(oldMax);});
    controller.query(QueryMode::CacheOnly);QVERIFY(controller.busy());controller.cancel();controller.clearCache();QVERIFY(controller.busy());QVERIFY(QFile::exists(cache.filePath(identity())));
    release.release();blocker.waitForFinished();QTRY_VERIFY(!controller.busy());QCOMPARE(controller.state(),QueryState::Cancelled);QVERIFY(!controller.snapshot());
    controller.clearCache();QVERIFY(controller.busy());controller.query(QueryMode::Refresh);QTRY_VERIFY(!controller.busy());QCOMPARE(cache.read(identity()).error,QueryError::CacheMiss);QCOMPARE(network.requests.size(),0);
    pool->waitForDone();QVERIFY(!QFile::exists(cache.filePath(identity())));
}
void Phase07Test::destruction() {
    Context c;QVERIFY(c.open());const auto id=c.project();QVERIFY(c.apply(id));FakeNetwork network;network.responses={{"{}",200,QNetworkReply::NoError,true}};
    auto* controller=new VulnerabilityController(c.database(),c.cache(),c.logger,nullptr,&network);
    connect(controller,&VulnerabilityController::consentRequested,controller,[controller]{controller->consent(true);});
    controller->setProject(id);controller->reload();QTRY_VERIFY(controller->loaded());controller->select(0);controller->query(QueryMode::Refresh);QTRY_COMPARE(network.requests.size(),1);
    delete controller;QTRY_VERIFY(network.replies[0].isNull());
    controller=new VulnerabilityController(c.database(),c.cache(),c.logger,nullptr,&network);controller->setProject(id);controller->reload();delete controller;
    QThreadPool::globalInstance()->waitForDone();QCoreApplication::processEvents();QCOMPARE(network.requests.size(),1);
}
void Phase07Test::workerLifecycle() {
    Context c;QVERIFY(c.open());const auto id=c.project();QVERIFY(c.apply(id));FakeNetwork network;
    network.responses={{page({record("FIRST")}),200,QNetworkReply::NoError,true},{page({record("SECOND")}),200,QNetworkReply::NoError,true}};
    VulnerabilityController controller(c.database(),c.cache(),c.logger,nullptr,&network);
    connect(&controller,&VulnerabilityController::consentRequested,&controller,[&]{controller.consent(true);});
    controller.setProject(id);controller.reload();QTRY_VERIFY(controller.loaded());controller.select(0);
    auto* pool=QThreadPool::globalInstance();pool->waitForDone();const auto oldMax=pool->maxThreadCount();pool->setMaxThreadCount(1);
    auto restore=qScopeGuard([&]{pool->waitForDone();pool->setMaxThreadCount(oldMax);});
    controller.query(QueryMode::Refresh);QTRY_COMPARE(network.requests.size(),1);
    // Cancel while the parser is queued; no partial result or cache may escape.
    {
        QSemaphore entered,release;auto blocker=QtConcurrent::run([&]{entered.release();release.acquire();});entered.acquire();
        auto cleanup=qScopeGuard([&]{release.release();blocker.waitForFinished();});
        network.replies[0]->release();controller.cancel();controller.clearCache();QVERIFY(controller.busy());QVERIFY(!controller.snapshot());
    }
    QTRY_VERIFY(!controller.busy());QCOMPARE(controller.state(),QueryState::Cancelled);
    OsvCache cache(c.cache());QCOMPARE(cache.read(identity()).error,QueryError::CacheMiss);
    controller.query(QueryMode::Refresh);QTRY_COMPARE(network.requests.size(),2);network.replies[1]->release();
    // Finish parsing without delivering its GUI callback, then occupy the pool.
    // The next callback queues a complete-result write behind our worker.
    pool->waitForDone();
    {
        QSemaphore entered,release;auto blocker=QtConcurrent::run([&]{entered.release();release.acquire();});entered.acquire();
        auto cleanup=qScopeGuard([&]{release.release();blocker.waitForFinished();});
        QTRY_VERIFY(controller.snapshot().has_value());QVERIFY(controller.busy());
        controller.select(1);controller.clearCache();QVERIFY(controller.busy());QVERIFY(!controller.snapshot());
    }
    QTRY_VERIFY(!controller.busy());QCOMPARE(controller.state(),QueryState::NotStarted);
    QVERIFY(cache.read(identity()).snapshot);controller.clearCache();QTRY_VERIFY(!controller.busy());
    pool->waitForDone();QCOMPARE(cache.read(identity()).error,QueryError::CacheMiss);QCOMPARE(network.requests.size(),2);
}
void Phase07Test::hiddenProjectSwitch() {
    Context c;QVERIFY(c.open());const auto a=c.project(),b=c.project();QVERIFY(c.apply(a,2));QVERIFY(c.apply(b,1));FakeNetwork network;
    VulnerabilityController controller(c.database(),c.cache(),c.logger,nullptr,&network);VulnerabilityPage widget(controller);
    auto* pool=QThreadPool::globalInstance();pool->waitForDone();const auto oldMax=pool->maxThreadCount();pool->setMaxThreadCount(1);
    QSemaphore entered,release;auto blocker=QtConcurrent::run([&]{entered.release();release.acquire();});entered.acquire();
    auto cleanup=qScopeGuard([&]{release.release();blocker.waitForFinished();pool->waitForDone();pool->setMaxThreadCount(oldMax);});
    widget.setProject(a);widget.show();QVERIFY(controller.loading());widget.hide();widget.setProject(b);widget.show();
    release.release();blocker.waitForFinished();QTRY_VERIFY(controller.loaded()&&!controller.loading());
    QCOMPARE(controller.rows().size(),1);QCOMPARE(controller.rows()[0].component.projectId,b);QCOMPARE(network.requests.size(),0);
}
void Phase07Test::ui() {
    Context c;QVERIFY(c.open());const auto id=c.project();QVERIFY(c.apply(id));FakeNetwork network;network.responses={{"{}"},{page({record()})},{"error",400},{"{}",200,QNetworkReply::NoError,true}};
    VulnerabilityController controller(c.database(),c.cache(),c.logger,nullptr,&network);VulnerabilityPage pageWidget(controller);pageWidget.resize(980,760);pageWidget.setProject(id);pageWidget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&pageWidget));QTRY_VERIFY(controller.loaded());auto* table=pageWidget.findChild<QTableView*>("identityTable");table->selectRow(0);
    auto* status=pageWidget.findChild<QLabel*>("vulnerabilityStatus");auto* clear=pageWidget.findChild<QPushButton*>("vulnerabilityClear");
    QVERIFY(status->text().contains("—"));QCOMPARE(network.requests.size(),0);
    pageWidget.findChild<QPushButton*>("vulnerabilityRefresh")->click();QTRY_VERIFY(pageWidget.findChild<QMessageBox*>());
    auto* dialog=pageWidget.findChild<QMessageBox*>();QVERIFY(dialog->text().contains(identity().name));QVERIFY(!clear->isEnabled());
    for(auto* b:dialog->buttons())if(dialog->buttonRole(b)==QMessageBox::AcceptRole)b->click();
    QTRY_VERIFY(!controller.busy());QVERIFY(status->text().contains("Count：0"));QVERIFY(!status->text().contains(QStringLiteral("安全")));
    controller.query(QueryMode::Refresh);QTRY_VERIFY(!controller.busy());QVERIFY(status->text().contains("Count：1"));
    auto* candidates=pageWidget.findChild<QTableView*>("candidateTable");candidates->selectRow(0);QTRY_VERIFY(pageWidget.findChild<QTextBrowser*>("candidateEvidence")->toPlainText().contains("SYNTHETIC-1"));
    QVERIFY(pageWidget.grab().save(QDir(QCoreApplication::applicationDirPath()).filePath("phase07-ui.png")));
    controller.query(QueryMode::Refresh);QTRY_VERIFY(!controller.busy());QVERIFY(status->text().contains("Failed"));QVERIFY(status->text().contains("Count：—"));QVERIFY(status->text().contains("RequestRejected"));
    controller.query(QueryMode::Refresh);QTRY_COMPARE(network.requests.size(),4);controller.cancel();QTRY_VERIFY(!controller.busy());QVERIFY(status->text().contains("Cancelled"));QVERIFY(clear->isEnabled());
    pageWidget.resize(400,220);QCoreApplication::processEvents();
    auto* scroll=pageWidget.findChild<QScrollArea*>();QVERIFY(scroll);QVERIFY(scroll->verticalScrollBar()->maximum()>0);
    scroll->verticalScrollBar()->setValue(scroll->verticalScrollBar()->maximum());QCoreApplication::processEvents();
    QVERIFY(pageWidget.grab().save(QDir(QCoreApplication::applicationDirPath()).filePath("phase07-ui-small.png")));
}
void Phase07Test::projectIntegration() {
    Context c;QVERIFY(c.open());const auto id=c.project();QVERIFY(c.apply(id));ProjectPage pageWidget(c.projects,c.components,c.logger,c.cache());pageWidget.resize(1150,850);pageWidget.show();
    auto* list=pageWidget.findChild<QListWidget*>();QVERIFY(list);list->setCurrentRow(0);auto* tabs=pageWidget.findChild<QTabWidget*>("projectDetailTabs");
    if(!tabs)tabs=pageWidget.findChild<QTabWidget*>();QVERIFY(tabs);QCOMPARE(tabs->count(),4);QCOMPARE(tabs->tabText(3),QStringLiteral("漏洞匹配"));tabs->setCurrentIndex(3);
    auto* controller=pageWidget.findChild<VulnerabilityController*>();QVERIFY(controller);QTRY_VERIFY(controller->loaded());QCOMPARE(controller->rows().size(),2);
    const auto old=controller->rows()[0].component.id;controller->select(0);controller->query(QueryMode::CacheOnly);QTRY_VERIFY(!controller->busy());QCOMPARE(controller->error(),QueryError::CacheMiss);
    pageWidget.findChild<QPushButton*>("importSbom")->click();auto* dialog=pageWidget.findChild<SbomImportDialog*>();QVERIFY(dialog);
    QPointer<QFileDialog> picker=dialog->findChild<QFileDialog*>();QVERIFY(picker);picker->reject();QTRY_VERIFY(picker.isNull());
    const auto source=c.dir.filePath("preview.json");QVERIFY(write(source,"{\"bomFormat\":\"CycloneDX\",\"specVersion\":\"1.6\",\"components\":[{\"name\":\"preview\"}]}"));
    QSignalSpy imported(dialog,&SbomImportDialog::importFinished),applied(dialog,&SbomImportDialog::applyFinished);
    dialog->importFile(source);QTRY_COMPARE(imported.count(),1);QVERIFY(imported[0][0].toBool());QCOMPARE(controller->rows()[0].component.id,old);
    QSqlQuery sql(c.db.connection());QVERIFY(sql.exec("CREATE TRIGGER synthetic_failure BEFORE INSERT ON components BEGIN SELECT RAISE(ABORT,'synthetic'); END"));
    dialog->applyToProject();QTRY_COMPARE(applied.count(),1);QVERIFY(!applied[0][0].toBool());QCOMPARE(controller->rows()[0].component.id,old);
    QVERIFY(sql.exec("DROP TRIGGER synthetic_failure"));dialog->applyToProject();QTRY_COMPARE(applied.count(),2);QVERIFY(applied[1][0].toBool());
    QTRY_VERIFY(controller->loaded());QCOMPARE(controller->rows().size(),1);QVERIFY(controller->rows()[0].component.id!=old);QVERIFY(!controller->snapshot());
    dialog->reject();
}
void Phase07Test::largeInput() {
    Context c;QVERIFY(c.open());const auto id=c.project();QElapsedTimer timer;timer.start();QVERIFY(c.apply(id,100000));const auto writeMs=timer.elapsed();
    FakeNetwork network;VulnerabilityController controller(c.database(),c.cache(),c.logger,nullptr,&network);VulnerabilityPage widget(controller);widget.resize(1000,780);
    int awake=0;const auto connection=connect(QAbstractEventDispatcher::instance(),&QAbstractEventDispatcher::awake,this,[&]{++awake;});timer.restart();widget.setProject(id);widget.show();
    QTRY_VERIFY_WITH_TIMEOUT(controller.loaded(),15000);const auto loadMs=timer.elapsed();QCOMPARE(controller.rows().size(),100000);
    auto* table=widget.findChild<QTableView*>("identityTable");QCOMPARE(table->model()->rowCount(),100000);timer.restart();table->scrollToBottom();table->selectRow(99999);QCoreApplication::processEvents();const auto scrollMs=timer.elapsed();
    QCOMPARE(controller.rows().last().identity.state,IdentityState::Resolved);QCOMPARE(network.requests.size(),0);QVERIFY(awake>1);QVERIFY(scrollMs<1000);
    qInfo("100000 components: apply=%lld ms, read+identity+install=%lld ms, scroll/select=%lld ms, GUI wakeups=%d, HTTP=0",writeMs,loadMs,scrollMs,awake);disconnect(connection);
}
void Phase07Test::manualFixtures() {
    QJsonArray components;const QList<QPair<QString,QString>> entries{{"jinja2","pkg:pypi/jinja2@2.4.1"},{"six","pkg:pypi/six@1.17.0"},{"lodash","pkg:npm/lodash@4.17.20"},{"is-number","pkg:npm/is-number@7.0.0"},{"scoped npm","pkg:npm/%40babel/core@7.0.0"},{"missing PURL",""},{"missing version","pkg:pypi/six"},{"unsupported","pkg:maven/example@1.0"},{"conflict","pkg:pypi/six@1.0"},{"epoch","pkg:pypi/six@1!2.0"},{"local","pkg:pypi/six@1.0+vendor.1"}};
    for(int n=0;n<entries.size();++n){const auto& e=entries[n];QJsonObject o{{"bom-ref",QString::number(n)},{"type","library"},{"name",e.first}};if(!e.second.isEmpty())o["purl"]=e.second;if(e.first=="conflict")o["version"]="1.0.0";components.append(o);}
    const auto root=QCoreApplication::applicationDirPath();QVERIFY(write(QDir(root).filePath("phase07-public-sbom.json"),json({{"bomFormat","CycloneDX"},{"specVersion","1.6"},{"components",components}})));
    components=QJsonArray();for(int n=0;n<100000;++n)components.append(QJsonObject{{"bom-ref",QString::number(n)},{"type","library"},{"name",QStringLiteral("synthetic-%1").arg(n)},{"purl","pkg:pypi/synthetic-example@1.0"}});
    QVERIFY(write(QDir(root).filePath("phase07-large-sbom.json"),json({{"bomFormat","CycloneDX"},{"specVersion","1.6"},{"components",components}})));
    QVERIFY(write(QDir(root).filePath("phase07-empty-sbom.json"),"{\"bomFormat\":\"CycloneDX\",\"specVersion\":\"1.6\",\"components\":[]}"));
    QVERIFY(write(QDir(root).filePath("phase07-invalid-sbom.json"),"{ invalid synthetic JSON"));
}
// Deliberately excluded from CTest. Run explicitly only after offline regressions.
void Phase07Test::liveSmoke() {
    if(!QCoreApplication::arguments().contains("liveSmoke"))QSKIP("Live smoke requires explicit invocation; normal runs stay offline.");
    OsvClient client;QSignalSpy done(&client,&OsvClient::finished);QueryError result=QueryError::Database;qsizetype count=-1;
    connect(&client,&OsvClient::finished,this,[&](QueryError e,const OsvSnapshot& s){result=e;count=e==QueryError::None?s.candidates.size():-1;});
    client.start({"PyPI","six","1.17.0",1});QTRY_COMPARE_WITH_TIMEOUT(done.count(),1,130000);
    qInfo("Public PyPI six 1.17.0: %s; candidates=%lld; UTC=%s",qPrintable(queryErrorCode(result)),qint64(count),qPrintable(QDateTime::currentDateTimeUtc().toString(Qt::ISODate)));
    QCOMPARE(result,QueryError::None);
}
QTEST_MAIN(Phase07Test)
#include "Phase07Test.moc"
