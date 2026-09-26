#include "VersionApplicability.h"
#include "Semver.h"
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

namespace {
using State = ApplicabilityState;
using Reason = ApplicabilityReason;
constexpr auto Source = CandidateSource::OsvPackageVersionQuery;
QueryIdentity identity(QString version = "1.2.3") { return {"npm","synthetic-phase08",version,1}; }
QJsonObject boundaryEvent(const char* type, const QString& value) { return {{QLatin1String(type),value}}; }
QJsonObject range(QJsonArray events, QString type = "SEMVER") { return {{"type",type},{"events",events}}; }
QJsonObject entry(QJsonArray ranges = {}, QJsonArray versions = {}, QString name = "synthetic-phase08", QString ecosystem = "npm")
{ return {{"package",QJsonObject{{"ecosystem",ecosystem},{"name",name}}},{"ranges",ranges},{"versions",versions}}; }
VulnerabilityCandidate candidate(QJsonArray affected, QString id = "SYNTHETIC-P08-1")
{ return {QJsonObject{{"id",id},{"modified","2026-01-01T00:00:00Z"},{"aliases",QJsonArray{"CVE-2026-1000001"}},{"affected",affected}}}; }
VulnerabilityCandidate exact(QString version = "1.2.3", QString id = "SYNTHETIC-P08-1") { return candidate({entry({}, {version})},id); }
ApplicabilityResult evaluate(QJsonArray events, QString version = "1.2.3")
{ return VersionApplicability::evaluateLocal(identity(version),candidate({entry({range(events)})})); }
QByteArray json(const QJsonObject& object) { return QJsonDocument(object).toJson(QJsonDocument::Compact); }
QByteArray read(const QString& path) { QFile f(path); if(!f.open(QIODevice::ReadOnly)) return {}; return f.readAll(); }
bool write(const QString& path,const QByteArray& bytes) { QFile f(path); return f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(); }
QByteArray payload(const QList<VulnerabilityCandidate>& candidates) {
    QJsonArray records; for(const auto& c:candidates) records.append(c.record); return json({{"vulns",records}});
}
// Offline transport with queued delivery, allowing real parser/cache/controller
// integration without live service availability or production test switches.
class Reply final : public QNetworkReply {
public:
    Reply(const QNetworkRequest& request,QByteArray bytes,int status,QObject* parent)
        :QNetworkReply(parent),m_bytes(std::move(bytes)) {
        setRequest(request);setUrl(request.url());open(QIODevice::ReadOnly|QIODevice::Unbuffered);
        QMetaObject::invokeMethod(this,[this,status]{
            if(isFinished())return;setAttribute(QNetworkRequest::HttpStatusCodeAttribute,status);
            emit readyRead();if(isFinished())return;setFinished(true);emit finished();
        },Qt::QueuedConnection);
    }
    void abort() override { if(isFinished())return;setError(OperationCanceledError,"synthetic cancel");setFinished(true);emit finished(); }
    qint64 bytesAvailable() const override { return m_bytes.size()-m_offset+QNetworkReply::bytesAvailable(); }
protected:
    qint64 readData(char* data,qint64 max) override {
        const auto size=qMin(max,qint64(m_bytes.size()-m_offset));if(size<=0)return -1;
        std::memcpy(data,m_bytes.constData()+m_offset,size_t(size));m_offset+=size;return size;
    }
private:
    QByteArray m_bytes; qint64 m_offset=0;
};
class Network final : public QNetworkAccessManager {
public:
    QList<QByteArray> responses;QList<int> statuses;QList<QByteArray> requests;
protected:
    QNetworkReply* createRequest(Operation op,const QNetworkRequest& request,QIODevice* outgoing) override {
        if(op!=PostOperation||responses.isEmpty())qFatal("Unexpected network request in offline Phase08 test");
        requests.append(outgoing->readAll());return new Reply(request,responses.takeFirst(),statuses.isEmpty()?200:statuses.takeFirst(),this);
    }
};
struct Context {
    QTemporaryDir dir;AppDatabase db;AppLogger logger;ProjectRepository projects{db,logger};ComponentRepository components{db};
    QString path()const{return dir.filePath("synthetic.db");}QString cache()const{return dir.filePath("cache/osv-v1");}
    bool open(){QString error;return logger.open(dir.filePath("synthetic.log"))&&db.open(path(),error);}
    QString project(){Project p;if(!projects.create("synthetic phase08",{},p).ok())qFatal("fixture project");return p.id;}
    bool apply(const QString& id) {
        SbomDocument d;d.components={{"a","library","synthetic-display","1.2.3","pkg:npm/synthetic-phase08@1.2.3"},
            {"b","library","synthetic-display","1.2.3","pkg:npm/synthetic-phase08@1.2.3"}};
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

class Phase08Test : public QObject {
    Q_OBJECT
private slots:
    void initTestCase(){QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);}
    void semver();void exactVersions();void semverBoundaries();void invalidEvents();void eventOrdering();void limits();
    void packageMatching();void wildcard();void aggregation();void providerGate();void withdrawn();void findings();
    void cacheAndLive();void lifecycle();void cancellation();void destruction();void projectApply();void largeSnapshot();void ui();void manualFixtures();
};

void Phase08Test::semver() {
    const QStringList ordered{"0.0.0-0","0.0.0","1.0.0-alpha","1.0.0-alpha.1","1.0.0-alpha.beta","1.0.0-beta",
        "1.0.0-beta.2","1.0.0-beta.11","1.0.0-rc.1","1.0.0","1.0.1","1.1.0","2.0.0"};
    for(qsizetype i=0;i<ordered.size();++i) for(qsizetype j=0;j<ordered.size();++j) {
        auto a=Semver::parse(ordered[i]),b=Semver::parse(ordered[j]);QVERIFY(a&&b);
        const int actual=a->compare(*b);QCOMPARE(actual==0,i==j);QCOMPARE(actual<0,i<j);QCOMPARE(actual>0,i>j);
    }
    for(const auto& invalid:QStringList{"","1","1.2","v1.2.3","01.2.3","1.02.3","1.2.03","1.2.3-01","1.2.3-", "1.2.3+",
        "1.2.3-a..b","1.2.3+a..b","1.2.3+a+b"," 1.2.3","1.2.3\n","1.2.3-中文","1.2.3.4"})QVERIFY2(!Semver::parse(invalid),qPrintable(invalid));
    QVERIFY(Semver::parse("1.2.3-0.01a+001.build"));
    QCOMPARE(Semver::parse("1.2.3+one")->compare(*Semver::parse("1.2.3+two")),0);
    const QString huge(500,'9'),larger="1"+QString(500,'0');
    for(int position=0;position<4;++position) {
        QStringList a{"1","2","3"},b=a;
        QString av,bv;
        if(position<3){a[position]=huge;b[position]=larger;av=a.join('.');bv=b.join('.');}
        else {av="1.2.3-"+huge;bv="1.2.3-"+larger;}
        QVERIFY(Semver::parse(av));QVERIFY(Semver::parse(bv));QVERIFY(Semver::parse(av)->compare(*Semver::parse(bv))<0);
    }
}
void Phase08Test::exactVersions() {
    const auto hit=VersionApplicability::evaluateLocal(identity(),exact());QCOMPARE(hit.state,State::Affected);QCOMPARE(hit.reason,Reason::ExplicitVersionMatch);
    for(const auto& text:QStringList{"1.2.3+other","v1.2.3","1.2.3 ","1.2.03"}) {
        const auto miss=VersionApplicability::evaluateLocal(identity(text),exact());QCOMPARE(miss.state,State::NotAffected);
    }
    const auto opaque=VersionApplicability::evaluateLocal(identity("1!2.0+vendor"),exact("1!2.0+vendor"));QCOMPARE(opaque.state,State::Affected);
    const auto build=VersionApplicability::evaluateLocal(identity("1.2.3+a"),exact("1.2.3+b"));QCOMPARE(build.state,State::NotAffected);
    QCOMPARE(evaluate({boundaryEvent("introduced","1.2.3+b")},"1.2.3+a").state,State::Affected);
}
void Phase08Test::semverBoundaries() {
    const QJsonArray fixed{boundaryEvent("introduced","1.0.0"),boundaryEvent("fixed","2.0.0")};
    for(const auto& [v,s]:QList<QPair<QString,State>>{{"0.9.9",State::NotAffected},{"1.0.0",State::Affected},{"1.9.9",State::Affected},
        {"2.0.0-alpha",State::Affected},{"2.0.0",State::NotAffected},{"2.0.0+build",State::NotAffected}})QCOMPARE(evaluate(fixed,v).state,s);
    const QJsonArray last{boundaryEvent("introduced","1.0.0"),boundaryEvent("last_affected","2.0.0")};
    QCOMPARE(evaluate(last,"2.0.0").state,State::Affected);QCOMPARE(evaluate(last,"2.0.1").state,State::NotAffected);
    QCOMPARE(evaluate({boundaryEvent("introduced","0")},"0.0.0-0").state,State::Affected);
    QCOMPARE(evaluate({boundaryEvent("introduced","0.0.0")},"0.0.0-0").state,State::NotAffected);
    const QJsonArray multi{boundaryEvent("introduced","1.0.0"),boundaryEvent("fixed","2.0.0"),boundaryEvent("introduced","3.0.0"),boundaryEvent("fixed","4.0.0")};
    for(const auto& [v,s]:QList<QPair<QString,State>>{{"1.2.3",State::Affected},{"2.5.0",State::NotAffected},{"3.0.0",State::Affected},{"4.0.0",State::NotAffected}})QCOMPARE(evaluate(multi,v).state,s);
    QCOMPARE(evaluate({boundaryEvent("introduced","1.0.0-alpha.2"),boundaryEvent("fixed","1.0.0-beta")},"1.0.0-alpha.11").state,State::Affected);
    const QString huge(200,'9');QCOMPARE(evaluate({boundaryEvent("introduced","0"),boundaryEvent("fixed",huge+".0.0")}).state,State::Affected);
}
void Phase08Test::invalidEvents() {
    const QList<QPair<QJsonArray,Reason>> cases{
        {{QJsonObject{{"introduced","1.0.0"},{"fixed","2.0.0"}}},Reason::InvalidRangeEvents},
        {{boundaryEvent("fixed","2.0.0")},Reason::MissingIntroducedEvent},{{},Reason::MissingIntroducedEvent},
        {{boundaryEvent("introduced","0"),boundaryEvent("fixed","2.0.0"),boundaryEvent("last_affected","3.0.0")},Reason::ConflictingRangeEvents},
        {{boundaryEvent("introduced","1.0")},Reason::InvalidSemver},{{boundaryEvent("introduced","0"),boundaryEvent("fixed","*")},Reason::InvalidSemver},
        {{boundaryEvent("introduced","0"),QJsonObject{}},Reason::InvalidRangeEvents},
        {{boundaryEvent("introduced","0"),boundaryEvent("limit","bad")},Reason::InvalidSemver},
        {{boundaryEvent("introduced","1.2.3+a"),boundaryEvent("fixed","1.2.3+b")},Reason::InvalidRangeEvents}};
    for(const auto& [events,reason]:cases){auto r=evaluate(events);QCOMPARE(r.state,State::Unknown);QCOMPARE(r.reason,reason);}
    QCOMPARE(evaluate({boundaryEvent("introduced","0")},"v1.2.3").reason,Reason::InvalidSemver);
    for(const auto& type:QStringList{"","FUTURE"}) {
        auto c=candidate({entry({range({boundaryEvent("introduced","0")},type)})});
        QCOMPARE(VersionApplicability::evaluateLocal(identity(),c).reason,Reason::UnsupportedRangeType);
    }
    auto r=range({boundaryEvent("introduced","0")});r.remove("type");
    QCOMPARE(VersionApplicability::evaluateLocal(identity(),candidate({entry({r})})).reason,Reason::UnsupportedRangeType);
    // Parser accepts structurally valid but semantically invalid events; evaluator owns the judgment.
    auto c=candidate({entry({range(cases.first().first)})});QVERIFY(OsvResponseParser::parse(payload({c})).ok());
}
void Phase08Test::eventOrdering() {
    const QList<QJsonObject> events{boundaryEvent("introduced","1.0.0"),boundaryEvent("fixed","2.0.0"),boundaryEvent("introduced","3.0.0"),boundaryEvent("fixed","4.0.0"),boundaryEvent("limit","5.0.0")};
    QList<int> indices{0,1,2,3,4};int permutations=0;
    do {
        QJsonArray shuffled;for(int i:indices)shuffled.append(events[i]);
        QCOMPARE(evaluate(shuffled,"1.2.3").state,State::Affected);QCOMPARE(evaluate(shuffled,"2.5.0").state,State::NotAffected);
        QCOMPARE(evaluate(shuffled,"3.5.0").reason,Reason::SemverRangeMatch);++permutations;
    }while(std::next_permutation(indices.begin(),indices.end()));QCOMPARE(permutations,120);
    QCOMPARE(evaluate({boundaryEvent("introduced","1.0.0+a"),boundaryEvent("introduced","1.0.0+b")}).state,State::Affected);
}
void Phase08Test::limits() {
    for(const auto& wildcard:QStringList{"*","1.*","future*"}) {
        QVERIFY(!Semver::parse(wildcard)); // Infinity limits bypass the strict SemVer parser.
        for(const auto& limits:QList<QStringList>{{wildcard},{"1.0.0",wildcard},{wildcard,"1.0.0"}}) {
            QJsonArray events{boundaryEvent("introduced","0")};for(const auto& l:limits)events.append(boundaryEvent("limit",l));
            const auto result=evaluate(events,"999.0.0");
            QCOMPARE(result.state,State::Affected);QCOMPARE(result.reason,Reason::SemverRangeMatch);
        }
        const auto invalid=evaluate({boundaryEvent("introduced","0"),boundaryEvent("limit",wildcard),boundaryEvent("limit","invalid")});
        QCOMPARE(invalid.state,State::Unknown);QCOMPARE(invalid.reason,Reason::InvalidSemver);
    }
    for(const auto& limits:QList<QStringList>{{},{"*"},{"1.0.0","3.0.0","2.0.0"},{"2.0.0","1.0.0","3.0.0"},{"1.0.0","*"}}) {
        QJsonArray events{boundaryEvent("introduced","0")};for(const auto& l:limits)events.append(boundaryEvent("limit",l));
        QCOMPARE(evaluate(events,"2.5.0").state,State::Affected);
    }
    const QJsonArray events{boundaryEvent("limit","1.0.0"),boundaryEvent("introduced","0"),boundaryEvent("limit","2.0.0")};
    QCOMPARE(evaluate(events,"2.0.0").state,State::NotAffected);QCOMPARE(evaluate(events,"1.5.0").state,State::Affected);
    QCOMPARE(evaluate({boundaryEvent("introduced","0"),boundaryEvent("limit","*"),boundaryEvent("limit","invalid")}).state,State::Unknown);
}
void Phase08Test::packageMatching() {
    auto c=candidate({entry({}, {"1.2.3"},"Friendly__Bard","PyPI")});
    QCOMPARE(VersionApplicability::evaluateLocal({"PyPI","friendly-bard","1.2.3",1},c).state,State::Affected);
    QCOMPARE(VersionApplicability::evaluateLocal(identity(),c).reason,Reason::NoMatchingAffectedPackage);
    c=candidate({entry({}, {"1.2.3"},"@scope/name")});
    QCOMPARE(VersionApplicability::evaluateLocal({"npm","@scope/name","1.2.3",1},c).state,State::Affected);
    for(const auto& name:QStringList{"name","@other/name","@scope/Name"})
        QCOMPARE(VersionApplicability::evaluateLocal({"npm",name,"1.2.3",1},c).reason,Reason::NoMatchingAffectedPackage);
    c=candidate({entry({range({boundaryEvent("introduced","bad")},"FUTURE")}, {},"different"),entry({}, {"9.9.9"})});
    QCOMPARE(VersionApplicability::evaluateLocal(identity(),c).state,State::NotAffected);
    for(const auto& invalid:QList<QJsonObject>{range({QJsonObject{{"introduced","0"},{"fixed","2.0.0"}}}),
        range({boundaryEvent("introduced","invalid")}),range({},"ECOSYSTEM"),range({},"GIT")}) {
        c=candidate({entry({invalid},{},"different"),entry({}, {"9.9.9"})});
        QCOMPARE(VersionApplicability::evaluateLocal(identity(),c).state,State::NotAffected);
        QCOMPARE(VersionApplicability::assess(identity(),c,Source).published->reason,Reason::ProviderEvidenceConflict);
    }
    c=candidate({entry({}, {},"synthetic-display")});QCOMPARE(VersionApplicability::evaluateLocal(identity(),c).reason,Reason::NoMatchingAffectedPackage);
}
void Phase08Test::wildcard() {
    for(bool semver:{false,true}) {
        const auto c=candidate({entry(semver?QJsonArray{range({boundaryEvent("introduced","0")})}:QJsonArray{},semver?QJsonArray{}:QJsonArray{"1.2.3"},"*")});
        const auto before=c.record;const auto r=VersionApplicability::assess(identity(),c,Source);
        QVERIFY(r.hasFinding());QVERIFY(r.local->evidence.first().wildcard);QCOMPARE(c.record,before);
        QVERIFY(applicabilityExplanation(r).contains("wildcard"));
        QCOMPARE(VersionApplicability::evaluateLocal({"PyPI","synthetic-phase08","1.2.3",1},c).reason,Reason::NoMatchingAffectedPackage);
    }
    auto unsupported=entry({range({boundaryEvent("introduced","0")},"GIT")},{},"*");
    QCOMPARE(VersionApplicability::evaluateLocal(identity(),candidate({unsupported})).reason,Reason::GitRangeRequiresCommitGraph);
    unsupported["package"]=QJsonObject{{"ecosystem","PyPI"},{"name","*"}};
    QCOMPARE(VersionApplicability::evaluateLocal(identity(),candidate({unsupported,entry({}, {"9.9.9"})})).state,State::NotAffected);
    for(const auto& name:QStringList{"synthetic-*","*phase08",".*"})
        QCOMPARE(VersionApplicability::evaluateLocal(identity(),candidate({entry({}, {"1.2.3"},name)})).reason,Reason::NoMatchingAffectedPackage);
}
void Phase08Test::aggregation() {
    for(const auto& [type,reason]:QList<QPair<QString,Reason>>{{"ECOSYSTEM",Reason::UnsupportedEcosystemRange},{"GIT",Reason::GitRangeRequiresCommitGraph},{"FUTURE",Reason::UnsupportedRangeType}}) {
        const auto unsupported=entry({range({boundaryEvent("introduced","0")},type)});
        QCOMPARE(VersionApplicability::evaluateLocal(identity(),candidate({unsupported})).reason,reason);
        for(bool reverse:{false,true}) {
            const auto positive=entry({}, {"1.2.3"});
            QCOMPARE(VersionApplicability::evaluateLocal(identity(),candidate(reverse?QJsonArray{positive,unsupported}:QJsonArray{unsupported,positive})).state,State::Affected);
        }
        QCOMPARE(VersionApplicability::evaluateLocal(identity(),candidate({unsupported,entry({}, {"9.9.9"})})).state,State::Unknown);
        QCOMPARE(VersionApplicability::evaluateLocal(identity(),candidate({entry({range({boundaryEvent("introduced","0")},type)}, {"1.2.3"})})).state,State::Affected);
    }
    QCOMPARE(VersionApplicability::evaluateLocal(identity(),candidate({entry()})).reason,Reason::MissingUsableEvidence);
    QCOMPARE(VersionApplicability::evaluateLocal(identity(),candidate({entry({}, {"9.9.9"}),entry()})).state,State::Unknown);
    QCOMPARE(VersionApplicability::evaluateLocal(identity(),candidate({entry({range({boundaryEvent("introduced","0")})}),entry()})).state,State::Affected);
    QCOMPARE(VersionApplicability::evaluateLocal(identity(),candidate({entry({range({boundaryEvent("introduced","invalid")})},{"1.2.3"})})).state,State::Affected);
}
void Phase08Test::providerGate() {
    const auto a=VersionApplicability::assess(identity(),exact("9.9.9"),Source);
    QCOMPARE(a.source,Source);QCOMPARE(a.local->state,State::NotAffected);QCOMPARE(a.local->reason,Reason::EvaluatedNoMatch);
    QCOMPARE(a.published->state,State::Unknown);QCOMPARE(a.published->reason,Reason::ProviderEvidenceConflict);QVERIFY(!a.hasFinding());
    QCOMPARE(a.published->queryVersion,"1.2.3");QCOMPARE(a.published->rulesVersion,1);QVERIFY(!a.published->evidence.isEmpty());
    QVERIFY(applicabilityExplanation(a).contains("fuzzy upstream-version matching"));
    const auto positive=VersionApplicability::assess(identity(),exact(),Source);QCOMPARE(positive.published->state,State::Affected);
}
void Phase08Test::withdrawn() {
    auto c=exact();c.record["withdrawn"]="2026-02-01T00:00:00Z";
    auto a=VersionApplicability::assess(identity(),c,Source);QCOMPARE(a.eligibility,CandidateEligibility::Excluded);
    QVERIFY(!a.local);QVERIFY(!a.published);QVERIFY(!a.hasFinding());QVERIFY(applicabilityExplanation(a).contains("RecordWithdrawn"));
    auto s=VersionApplicability::evaluateSnapshot("row",{identity(),QDateTime::currentDateTimeUtc(),{c}},1,Source);
    QCOMPARE(s.excludedCount,1);QCOMPARE(s.unknownCount,0);QVERIFY(s.findings.isEmpty());
}
void Phase08Test::findings() {
    const OsvSnapshot raw{identity(),QDateTime::currentDateTimeUtc(),{exact(),exact("9.9.9","MISS")}};
    const auto a=VersionApplicability::evaluateSnapshot("component-a",raw,10,Source),b=VersionApplicability::evaluateSnapshot("component-b",raw,11,Source);
    QCOMPARE(a.findings.size(),1);QCOMPARE(b.findings.size(),1);QVERIFY(a.findings[0].componentId!=b.findings[0].componentId);
    const auto& f=a.findings[0];QCOMPARE(f.queryIdentity,identity());QCOMPARE(f.snapshotIdentity,raw.identity);QCOMPARE(f.fetchedAt,raw.fetchedAt);
    QCOMPARE(f.osvId,raw.candidates[0].id());QCOMPARE(f.cveAliases,raw.candidates[0].cveAliases());QCOMPARE(f.applicability.state,State::Affected);
    QCOMPARE(a.generation,quint64(10));QCOMPARE(a.affectedCount,1);QCOMPARE(a.unknownCount,1);
    QCOMPARE(VersionApplicability::evaluateSnapshot("component-a",raw,12,Source).findings.size(),1);
}

void Phase08Test::cacheAndLive() {
    Context c;QVERIFY(c.open());const auto id=c.project();QVERIFY(c.apply(id));Network network;
    auto withdrawnRecord=exact("1.2.3","WITHDRAWN");withdrawnRecord.record["withdrawn"]="2026-02-01T00:00:00Z";
    const QList<VulnerabilityCandidate> records{exact(),exact("9.9.9","CONFLICT"),candidate({entry({range({},"ECOSYSTEM")})},"UNKNOWN"),withdrawnRecord};
    network.responses={payload(records),"failure","{}"};network.statuses={200,503,200};
    VulnerabilityController controller(c.path(),c.cache(),c.logger,nullptr,&network);
    connect(&controller,&VulnerabilityController::consentRequested,&controller,[&]{controller.consent(true);});
    controller.setProject(id);controller.reload();QTRY_VERIFY(controller.loaded());controller.select(0);
    controller.query(QueryMode::Refresh);QTRY_VERIFY(!controller.busy());QCOMPARE(controller.state(),QueryState::Success);
    QVERIFY(controller.applicability());QCOMPARE(controller.source(),ResultSource::Live);
    const auto first=*controller.applicability();QCOMPARE(first.findings.size(),1);QCOMPARE(first.unknownCount,2);QCOMPARE(first.excludedCount,1);
    QCOMPARE(first.generation,controller.generation());QCOMPARE(first.componentId,controller.rows()[0].component.id);
    QCOMPARE(first.identity,controller.snapshot()->identity);QCOMPARE(first.fetchedAt,controller.snapshot()->fetchedAt);
    const auto cacheFile=OsvCache(c.cache()).filePath(identity());const auto raw=read(cacheFile);
    for(const auto& forbidden:QList<QByteArray>{"applicability","Finding","ProviderEvidenceConflict","rulesVersion"})QVERIFY(!raw.contains(forbidden));
    for(const auto mode:{QueryMode::CacheOnly,QueryMode::PreferCache}) {
        controller.query(mode);QVERIFY(!controller.applicability());QTRY_VERIFY(!controller.busy());
        QCOMPARE(controller.source(),ResultSource::FreshCache);QCOMPARE(controller.applicability()->findings.size(),1);
        QCOMPARE(controller.applicability()->candidates[1].published->reason,Reason::ProviderEvidenceConflict);
        QCOMPARE(read(cacheFile),raw);
    }
    QVERIFY(c.seed(records,-2));controller.query(QueryMode::CacheOnly);QTRY_VERIFY(!controller.busy());
    QCOMPARE(controller.source(),ResultSource::StaleCache);QCOMPARE(controller.applicability()->findings.size(),1);
    QCOMPARE(controller.applicability()->candidates[0].published->reason,first.candidates[0].published->reason);
    controller.query(QueryMode::Refresh);QVERIFY(!controller.applicability());QTRY_VERIFY(!controller.busy());
    QCOMPARE(controller.state(),QueryState::Failed);QCOMPARE(controller.applicability()->findings.size(),1);
    QCOMPARE(controller.source(),ResultSource::StaleCache); // Historical evidence remains explicitly historical.
    controller.query(QueryMode::Refresh);QTRY_VERIFY(!controller.busy());QCOMPARE(controller.state(),QueryState::Success);
    QVERIFY(controller.snapshot()->candidates.isEmpty());QVERIFY(controller.applicability()->findings.isEmpty());
    QCOMPARE(controller.applicability()->unknownCount,0);QCOMPARE(controller.source(),ResultSource::Live);
    QCOMPARE(network.requests.size(),3);
    QCOMPARE(QJsonDocument::fromJson(network.requests[0]).object(),QJsonObject({{"package",QJsonObject{{"ecosystem","npm"},{"name",identity().name}}},{"version","1.2.3"}}));
    const auto log=read(c.dir.filePath("synthetic.log"));QVERIFY(!log.contains("SYNTHETIC-P08"));QVERIFY(!log.contains("synthetic-phase08"));
}

void Phase08Test::lifecycle() {
    for(const auto action:{QString("component"),QString("project"),QString("reload")}) {
        Context c;QVERIFY(c.open());const auto id=c.project(),other=c.project();QVERIFY(c.apply(id));QVERIFY(c.apply(other));QVERIFY(c.seed());Network network;
        VulnerabilityController controller(c.path(),c.cache(),c.logger,nullptr,&network);
        controller.setProject(id);controller.reload();QTRY_VERIFY(controller.loaded());controller.select(0);
        controller.query(QueryMode::CacheOnly);QTRY_VERIFY(!controller.busy());QVERIFY(controller.applicability());
        const auto old=controller.generation();
        controller.query(QueryMode::CacheOnly);QThreadPool::globalInstance()->waitForDone();
        {
            PoolBlock block;
            QTRY_VERIFY(controller.evaluating());QVERIFY(!controller.applicability());
            if(action=="component")controller.select(1);
            else if(action=="project"){controller.setProject(other);controller.reload();}
            else controller.reload();
            QVERIFY(controller.generation()>old);QVERIFY(!controller.applicability());QVERIFY(!controller.snapshot());QVERIFY(controller.busy());
        }
        QTRY_VERIFY(!controller.busy()&&!controller.loading());QVERIFY(!controller.applicability());
        if(action!="component")QTRY_VERIFY(controller.loaded());
        controller.select(action=="component"?1:0);controller.query(QueryMode::CacheOnly);QTRY_VERIFY(!controller.busy());
        QVERIFY(controller.applicability());QCOMPARE(controller.applicability()->componentId,controller.rows()[controller.selected()].component.id);
        QCOMPARE(controller.applicability()->generation,controller.generation());QCOMPARE(controller.applicability()->findings.size(),1);
        QCOMPARE(network.requests.size(),0);
    }
}
void Phase08Test::cancellation() {
    Context c;QVERIFY(c.open());const auto id=c.project();QVERIFY(c.apply(id));QVERIFY(c.seed());Network network;
    VulnerabilityController controller(c.path(),c.cache(),c.logger,nullptr,&network);
    controller.setProject(id);controller.reload();QTRY_VERIFY(controller.loaded());controller.select(0);
    controller.query(QueryMode::CacheOnly);QThreadPool::globalInstance()->waitForDone();
    {
        PoolBlock block;QTRY_VERIFY(controller.evaluating());QVERIFY(!controller.applicability());
        controller.cancel();controller.clearCache();QVERIFY(controller.busy());QVERIFY(!controller.applicability());
        QCOMPARE(controller.state(),QueryState::Cancelled);
    }
    QTRY_VERIFY(!controller.busy());QVERIFY(!controller.applicability());QVERIFY(!controller.evaluating());
    QVERIFY(OsvCache(c.cache()).read(identity()).snapshot);
    controller.query(QueryMode::CacheOnly);QTRY_VERIFY(!controller.busy());QCOMPARE(controller.applicability()->findings.size(),1);
    controller.clearCache();QVERIFY(!controller.applicability());QTRY_VERIFY(!controller.busy());QVERIFY(!controller.snapshot());
    QCOMPARE(network.requests.size(),0);
}
void Phase08Test::destruction() {
    Context c;QVERIFY(c.open());const auto id=c.project();QVERIFY(c.apply(id));QVERIFY(c.seed());Network network;
    QPointer<VulnerabilityController> controller=new VulnerabilityController(c.path(),c.cache(),c.logger,nullptr,&network);
    controller->setProject(id);controller->reload();QTRY_VERIFY(controller->loaded());controller->select(0);
    controller->query(QueryMode::CacheOnly);QThreadPool::globalInstance()->waitForDone();
    {
        PoolBlock block;QTRY_VERIFY(controller->evaluating());delete controller.data();QVERIFY(controller.isNull());
    }
    QCoreApplication::processEvents();QCOMPARE(network.requests.size(),0);
}
void Phase08Test::projectApply() {
    Context c;QVERIFY(c.open());const auto id=c.project();QVERIFY(c.apply(id));QVERIFY(c.seed());
    ProjectPage widget(c.projects,c.components,c.logger,c.cache());widget.resize(1100,850);widget.show();
    widget.findChild<QListWidget*>("projectList")->setCurrentRow(0);widget.findChild<QTabWidget*>("projectTabs")->setCurrentIndex(3);
    auto* controller=widget.findChild<VulnerabilityController*>();QVERIFY(controller);QTRY_VERIFY(controller->loaded());
    controller->select(0);controller->query(QueryMode::CacheOnly);QTRY_VERIFY(!controller->busy());QVERIFY(controller->applicability());
    const auto old=controller->applicability()->componentId;
    widget.findChild<QPushButton*>("importSbom")->click();auto* dialog=widget.findChild<SbomImportDialog*>();QVERIFY(dialog);
    QPointer<QFileDialog> picker=dialog->findChild<QFileDialog*>();QVERIFY(picker);picker->reject();QTRY_VERIFY(picker.isNull());
    const auto file=c.dir.filePath("replacement.json");
    QVERIFY(write(file,json({{"bomFormat","CycloneDX"},{"specVersion","1.6"},{"components",QJsonArray{QJsonObject{{"name","synthetic new"},{"purl","pkg:npm/synthetic-phase08@1.2.3"}}}}})));
    QSignalSpy imported(dialog,&SbomImportDialog::importFinished),applied(dialog,&SbomImportDialog::applyFinished);
    dialog->importFile(file);QTRY_COMPARE(imported.count(),1);QVERIFY(imported[0][0].toBool());QCOMPARE(controller->applicability()->componentId,old);
    QSqlQuery sql(c.db.connection());QVERIFY(sql.exec("CREATE TRIGGER fail_phase08 BEFORE INSERT ON components BEGIN SELECT RAISE(ABORT,'synthetic'); END"));
    dialog->applyToProject();QTRY_COMPARE(applied.count(),1);QVERIFY(!applied[0][0].toBool());QCOMPARE(controller->applicability()->componentId,old);
    QVERIFY(sql.exec("DROP TRIGGER fail_phase08"));dialog->applyToProject();QTRY_COMPARE(applied.count(),2);QVERIFY(applied[1][0].toBool());
    QTRY_VERIFY(controller->loaded());QVERIFY(!controller->applicability());QVERIFY(!controller->snapshot());QVERIFY(controller->rows()[0].component.id!=old);
    controller->select(0);controller->query(QueryMode::CacheOnly);QTRY_VERIFY(!controller->busy());
    QCOMPARE(controller->applicability()->findings.size(),1);QCOMPARE(controller->applicability()->findings[0].componentId,controller->rows()[0].component.id);
    QVERIFY(sql.exec("SELECT value FROM app_meta WHERE key='schema_version'"));QVERIFY(sql.next());QCOMPARE(sql.value(0).toInt(),4);sql.finish();
    QVERIFY(sql.exec("SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'"));QVERIFY(sql.next());QCOMPARE(sql.value(0).toInt(),6);sql.finish();
    dialog->reject();
}

void Phase08Test::largeSnapshot() {
    Context c;QVERIFY(c.open());const auto id=c.project();QVERIFY(c.apply(id));
    QJsonArray events;for(int i=0;i<10;++i){events.append(boundaryEvent("introduced",QString::number(i*2)+".0.0"));events.append(boundaryEvent("fixed",QString::number(i*2+1)+".0.0"));}
    QList<VulnerabilityCandidate> records;records.reserve(10000);
    for(int i=0;i<10000;++i)records.append(candidate({entry({range(events)})},QStringLiteral("SYNTHETIC-LARGE-%1").arg(i)));
    QVERIFY(c.seed(records));Network network;VulnerabilityController controller(c.path(),c.cache(),c.logger,nullptr,&network);
    VulnerabilityPage widget(controller);widget.resize(1050,780);widget.setProject(id);widget.show();QVERIFY(QTest::qWaitForWindowExposed(&widget));QTRY_VERIFY(controller.loaded());controller.select(0);
    QElapsedTimer total,interval;total.start();interval.start();qint64 maxGap=0;int ticks=0,evaluationTicks=0;
    QTimer timer;timer.setInterval(1);connect(&timer,&QTimer::timeout,this,[&]{++ticks;if(controller.evaluating())++evaluationTicks;maxGap=qMax(maxGap,interval.restart());});timer.start();
    controller.query(QueryMode::CacheOnly);QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(),15000);timer.stop();
    QVERIFY(controller.applicability());QCOMPARE(controller.applicability()->candidates.size(),10000);QCOMPARE(controller.applicability()->unknownCount,10000);
    QCOMPARE(controller.applicability()->findings.size(),0);QVERIFY(ticks>1);QVERIFY(evaluationTicks>0);QVERIFY2(maxGap<1000,qPrintable(QString::number(maxGap)));
    auto* table=widget.findChild<QTableView*>("candidateTable");QCOMPARE(table->model()->rowCount(),10000);table->scrollToBottom();table->selectRow(9999);
    QTRY_VERIFY(widget.findChild<QTextBrowser*>("candidateEvidence")->toPlainText().contains("SYNTHETIC-LARGE-9999"));
    QCOMPARE(network.requests.size(),0);qInfo("10000 candidates / 200000 events: %lld ms; GUI ticks=%d, evaluation ticks=%d, maximum gap=%lld ms; HTTP=0",total.elapsed(),ticks,evaluationTicks,maxGap);
}
void Phase08Test::ui() {
    Context c;QVERIFY(c.open());const auto id=c.project();QVERIFY(c.apply(id));
    auto withdrawnRecord=exact("1.2.3","WITHDRAWN");withdrawnRecord.record["withdrawn"]="2026-02-01T00:00:00Z";
    QVERIFY(c.seed({exact(),exact("9.9.9","CONFLICT"),candidate({entry({range({},"ECOSYSTEM")})},"UNKNOWN"),withdrawnRecord,
        candidate({entry({range({boundaryEvent("introduced","0")})},{},"*")},"WILDCARD")}));
    Network network;VulnerabilityController controller(c.path(),c.cache(),c.logger,nullptr,&network);
    VulnerabilityPage widget(controller);widget.resize(1100,820);widget.setProject(id);widget.show();QVERIFY(QTest::qWaitForWindowExposed(&widget));QTRY_VERIFY(controller.loaded());
    auto* identities=widget.findChild<QTableView*>("identityTable");identities->selectRow(0);controller.query(QueryMode::CacheOnly);QTRY_VERIFY(!controller.busy());
    auto* table=widget.findChild<QTableView*>("candidateTable");auto* evidence=widget.findChild<QTextBrowser*>("candidateEvidence");auto* status=widget.findChild<QLabel*>("vulnerabilityStatus");
    QCOMPARE(table->model()->columnCount(),8);QCOMPARE(table->model()->rowCount(),5);
    QCOMPARE(table->model()->index(1,6).data().toString(),"ProviderEvidenceConflict");QCOMPARE(table->model()->index(1,7).data().toString(),"No");
    QCOMPARE(table->model()->index(3,5).data().toString(),"Withdrawn / Excluded");
    QVERIFY(status->text().contains("Finding Count：2"));QVERIFY(!status->text().contains("NotAffected Count"));
    for(const auto& bad:QStringList{"Safe",QStringLiteral("无风险"),QStringLiteral("已安全"),QStringLiteral("无漏洞")})QVERIFY(!status->text().contains(bad));
    table->selectRow(1);QTRY_VERIFY(evidence->toPlainText().contains("ProviderEvidenceConflict"));QVERIFY(evidence->toPlainText().contains("Local: NotAffected"));
    QVERIFY(evidence->toPlainText().contains("Published: Unknown"));QVERIFY(evidence->toPlainText().contains("Raw OSV JSON"));
    table->selectRow(4);QTRY_VERIFY(evidence->toPlainText().contains("wildcard"));QVERIFY(evidence->toPlainText().contains("\"name\": \"*\""));
    QVERIFY(widget.grab().save(QDir(QCoreApplication::applicationDirPath()).filePath("phase08-ui.png")));
    for(int i=0;i<3;++i){controller.query(QueryMode::CacheOnly);QTRY_VERIFY(!controller.busy());QCOMPARE(controller.applicability()->findings.size(),2);}
    widget.resize(400,220);QCoreApplication::processEvents();auto* scroll=widget.findChild<QScrollArea*>();QVERIFY(scroll->verticalScrollBar()->maximum()>0);
    QVERIFY(widget.grab().save(QDir(QCoreApplication::applicationDirPath()).filePath("phase08-ui-small.png")));
    {
        PoolBlock block;
        table->selectRow(0);table->selectRow(4); // Queue two evidence explanations.
        identities->selectRow(1);QVERIFY(!controller.applicability());QVERIFY(evidence->toPlainText().isEmpty());QCOMPARE(table->model()->rowCount(),0);
    }
    QCoreApplication::processEvents();QVERIFY(evidence->toPlainText().isEmpty());
    QCOMPARE(network.requests.size(),0);
}

void Phase08Test::manualFixtures() {
    const auto root=QCoreApplication::applicationDirPath();const QDir out(root);
    QJsonArray components;
    for(const auto& [name,version]:QList<QPair<QString,QString>>{{"synthetic-phase08","1.2.3"},{"synthetic-phase08","2.0.0"},
        {"synthetic-phase08","2.0.1"},{"synthetic-phase08-stale","1.2.3"},{"synthetic-phase08","1.2.3"}})
        components.append(QJsonObject{{"bom-ref",QString::number(components.size())},{"name",name},{"type","library"},
            {"version",version},{"purl","pkg:npm/"+name+"@"+version}});
    const auto sbom=json({{"bomFormat","CycloneDX"},{"specVersion","1.6"},{"components",components}});
    QVERIFY(write(out.filePath("phase08-demo-sbom.json"),sbom));
    QVERIFY(write(out.filePath("phase08-empty-sbom.json"),"{\"bomFormat\":\"CycloneDX\",\"specVersion\":\"1.6\",\"components\":[]}"));
    AppPaths paths{out.filePath("phase08-manual-data")};QString error;QVERIFY(paths.initialize(error));
    // Never replace a user's existing manual-acceptance database or its cache.
    if(!QFileInfo::exists(paths.databaseFile())) {
        AppDatabase db;AppLogger logger;QVERIFY(logger.open(paths.logFile()));QVERIFY(db.open(paths.databaseFile(),error));
        ProjectRepository projects(db,logger);ComponentRepository current(db);Project a,b;
        QVERIFY(projects.create("Phase08 synthetic demo",{},a).ok());QVERIFY(projects.create("Phase08 synthetic second project",{},b).ok());
        const auto parsed=CycloneDxParser::parse(sbom);QVERIFY(parsed.ok());QVERIFY(current.replaceForProject(a.id,parsed.document).ok());
        QVERIFY(current.replaceForProject(b.id,parsed.document).ok());
        for(const auto& component:parsed.document.components) {
            Component c;c.purl=component.purl;c.version=component.version;const auto query=PackageIdentity::resolve(c).query();QVERIFY(query);
            const auto named=[&](QJsonArray ranges,QJsonArray versions={}){return entry(ranges,versions,query->name);};
            auto withdrawnRecord=candidate({named({}, {"1.2.3"})},"SYNTHETIC-WITHDRAWN");withdrawnRecord.record["withdrawn"]="2026-02-01T00:00:00Z";
            QList<VulnerabilityCandidate> records{
                candidate({named({}, {"1.2.3"})},"SYNTHETIC-EXPLICIT"),
                candidate({named({range({boundaryEvent("introduced","1.0.0"),boundaryEvent("fixed","2.0.0")})})},"SYNTHETIC-SEMVER"),
                candidate({named({range({boundaryEvent("introduced","0"),boundaryEvent("last_affected","2.0.0")})})},"SYNTHETIC-LAST-AFFECTED"),
                candidate({named({range({boundaryEvent("introduced","0")},"ECOSYSTEM")})},"SYNTHETIC-ECOSYSTEM"),
                candidate({named({range({boundaryEvent("introduced","abc")},"GIT")})},"SYNTHETIC-GIT"),
                candidate({named({}, {"9.9.9"})},"SYNTHETIC-CONFLICT"),
                candidate({entry({range({boundaryEvent("introduced","0"),boundaryEvent("limit","*")})},{},"*")},"SYNTHETIC-WILDCARD"),withdrawnRecord};
            const auto fetched=QDateTime::currentDateTimeUtc().addDays(query->name.endsWith("-stale")?-2:0);
            QCOMPARE(OsvCache(paths.osvCacheDirectory()).write({*query,fetched,records}),QueryError::None);
        }
    }
    const auto launch=QStringLiteral("@echo off\r\nset \"PATH=%1;%2;%PATH%\"\r\n\"%~dp0SupplyChainRiskAssessment.exe\" --data-dir \"%~dp0phase08-manual-data\"\r\n")
        .arg(QDir::toNativeSeparators(QLibraryInfo::path(QLibraryInfo::BinariesPath)),QStringLiteral(PHASE08_COMPILER_BIN));
    QVERIFY(write(out.filePath("phase08-manual.cmd"),launch.toLocal8Bit()));
    const auto checklist=QStringLiteral(
        "# Phase 08 Debug GUI Manual Acceptance Checklist\n\n"
        "Manual Acceptance: PENDING USER. All records and projects here are synthetic.\n"
        "运行本目录 phase08-manual.cmd，使用隔离 phase08-manual-data。已有数据库和缓存不会被测试覆盖。\n"
        "缓存超过24小时会显示 Stale；仍可用‘仅本地缓存’验证。不要对 synthetic identity 点击‘发送并查询’。\n\n"
        "1. 进入项目，选择 Phase08 synthetic demo，打开‘漏洞匹配’。选择第一行 1.2.3，点击‘仅本地缓存’。\n"
        "   预期8 Candidates / 4 Affected / 3 Unknown / 1 Excluded / 4 Findings。首次生成24小时内显示 Fresh Cache。\n"
        "2. SYNTHETIC-EXPLICIT：Affected / ExplicitVersionMatch / Finding Yes；证据保留 exact versions。\n"
        "3. SYNTHETIC-SEMVER：Affected / SemverRangeMatch；选择组件2.0.0并查询，固定边界不命中，Published Unknown / ProviderEvidenceConflict。\n"
        "4. SYNTHETIC-LAST-AFFECTED：2.0.0仍 Affected；2.0.1为 Unknown / ProviderEvidenceConflict。\n"
        "5. SYNTHETIC-ECOSYSTEM：Unknown / UnsupportedEcosystemRange / No。\n"
        "6. SYNTHETIC-GIT：Unknown / GitRangeRequiresCommitGraph / No。\n"
        "7. SYNTHETIC-CONFLICT：Local NotAffected / Published Unknown / ProviderEvidenceConflict / No；阅读原因解释。\n"
        "8. SYNTHETIC-WILDCARD：Affected / Yes；解释说明 ecosystem-wide wildcard，Raw JSON仍保留name为*。\n"
        "9. SYNTHETIC-WITHDRAWN：Withdrawn / Excluded / RecordWithdrawn / No，无普通版本判断。\n"
        "10. 选择 synthetic-phase08-stale 1.2.3，点击‘仅本地缓存’，预期Stale Cache，仍为4/3/1及4 Findings。\n"
        "11. 连续重复查询同一行，Finding Count保持4；选择重复的1.2.3组件，先清空旧结果，查询后该当前组件有4 Findings。\n"
        "12. 点击‘刷新在线结果’，在首次发送确认框点‘取消’（不发送）。预期Cancelled，本次Count未知，Finding未发布。\n"
        "13. 切换另一个项目：旧Candidate/适用性/Finding/解释立即清空；选择组件并仅本地查询后重新产生结果。\n"
        "14. 点击‘重读当前组件’：旧结果清空，重新选行查询后恢复；关闭重启后须重新派生Finding。\n"
        "15. 导入本目录 phase08-empty-sbom.json，只预览时旧结果保留；点击‘应用到项目’后当前组件与所有旧结果清空。\n"
        "16. 导入 phase08-demo-sbom.json 并Apply，重建当前组件，选择1.2.3仅本地查询恢复8候选/4Finding。\n"
        "17. 正常/最小窗口检查滚动、新增列、完整状态说明、原始JSON和解释；没有Safe/无风险/无漏洞结论。\n"
        "18. GUI仍可操作、无崩溃；后台评价取消/销毁/过期交付由Phase08自动测试另行验证，不能代替本清单人工操作。\n\n"
        "Phase 08 Manual Acceptance: PENDING USER\n");
    QVERIFY(write(out.filePath("phase08-manual-checklist.md"),checklist.toUtf8()));
}

QTEST_MAIN(Phase08Test)
#include "Phase08Test.moc"
