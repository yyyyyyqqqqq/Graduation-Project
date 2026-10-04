#include "FindingReport.h"
#include "PresentationText.h"
#include "RiskPresentationView.h"
#include "ProjectPage.h"
#include "ProjectRepository.h"
#include "ComponentRepository.h"
#include "SbomDocument.h"
#include "VulnerabilityPage.h"
#include "VulnerabilityController.h"
#include "RiskEvidenceController.h"
#include "AppDatabase.h"
#include "AppLogger.h"
#include <QTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QListWidget>
#include <QTableView>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QTextDocument>
#include <QJsonDocument>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QSaveFile>
#include <QSqlQuery>
#include <QSemaphore>
#include <QThreadPool>
#include <QElapsedTimer>
#include <QRegularExpression>
#include <QtConcurrentRun>
#include <QCryptographicHash>
#include <cstring>
#include <type_traits>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace {
const QString A = "CVE-2026-100001", B = "CVE-2026-100002";
QDateTime time() { return QDateTime::fromString("2026-10-04T08:00:00.000Z", Qt::ISODateWithMs); }
RiskEvidenceProfile profile()
{
    RiskEvidenceProfile p; p.generatedAt=time(); p.key.osvId="SYNTHETIC-REPORT-A";
    p.key.componentId="INTERNAL_COMPONENT_ID_SECRET_ABC";
    EpssEvidence e; e.cve=A; e.status=EpssStatus::Available; e.probability=.035; e.percentile=.94;
    e.fetchedAt=time(); e.freshness=EvidenceFreshness::Fresh; e.acquisition=EvidenceAcquisition::Cache;
    KevEvidence k; k.cve=A; k.status=KevStatus::NotListed; k.fetchedAt=time();
    k.freshness=EvidenceFreshness::Fresh; k.acquisition=EvidenceAcquisition::Cache;
    p.epss={e};p.kev={k};return p;
}
FindingReportContext context(const RiskEvidenceProfile& p, const RiskPriorityAssessment& a,
                             const QString& description="Synthetic description", const QString& acquisition="Live")
{
    QStringList aliases; for(const auto& e:p.epss) if(!e.cve.isEmpty()) aliases.append(e.cve);
    return {"Synthetic project",description,{"npm","synthetic","1.0.0"},"SYNTHETIC-REPORT-A",aliases,
        "ExplicitVersionMatch / exact supported version",acquisition,time(),p,a,time().addSecs(20),QStringLiteral(APPLICATION_VERSION),4};
}
QString plain(const QByteArray& html) { QTextDocument d; d.setHtml(QString::fromUtf8(html)); return d.toPlainText(); }
QByteArray read(const QString& path) { QFile f(path); return f.open(QIODevice::ReadOnly)?f.readAll():QByteArray(); }
bool write(const QString& path,const QByteArray& bytes) { QFile f(path);return f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(); }
QJsonObject candidate(const QString& id,const QString& cve)
{
    return {{"id",id},{"modified","2026-01-01T00:00:00Z"},{"aliases",QJsonArray{cve}},
        {"affected",QJsonArray{QJsonObject{{"package",QJsonObject{{"ecosystem","npm"},{"name","synthetic"}}},{"versions",QJsonArray{"1.0.0"}}}}}};
}
class Reply final : public QNetworkReply {
public:
    Reply(const QNetworkRequest& request,QByteArray data,bool fail,bool hold,QObject* parent)
        :QNetworkReply(parent),bytes(std::move(data)) {
        setRequest(request);setUrl(request.url());open(ReadOnly|Unbuffered);
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute,fail?503:200);
        if(fail)setError(ServiceUnavailableError,"Synthetic failure");
        if(!hold)QMetaObject::invokeMethod(this,[this]{if(isFinished())return;emit readyRead();setFinished(true);emit finished();},Qt::QueuedConnection);
    }
    void abort() override {if(!isFinished()){setError(OperationCanceledError,"Synthetic cancellation");setFinished(true);emit finished();}}
    qint64 bytesAvailable() const override {return bytes.size()-offset+QNetworkReply::bytesAvailable();}
protected:
    qint64 readData(char* out,qint64 n) override {n=qMin(n,qint64(bytes.size()-offset));if(n<=0)return -1;std::memcpy(out,bytes.constData()+offset,size_t(n));offset+=n;return n;}
private:
    QByteArray bytes;qint64 offset=0;
};
class Network final : public QNetworkAccessManager {
public:
    int requests=0;bool fail=false,hold=false;
protected:
    QNetworkReply* createRequest(Operation,const QNetworkRequest& request,QIODevice*) override {
        ++requests;QByteArray bytes;
        if(request.url().host()=="api.osv.dev") bytes=QJsonDocument(QJsonObject{{"vulns",QJsonArray{
            candidate("SYNTHETIC-REPORT-A",A),candidate("SYNTHETIC-REPORT-B",B)}}}).toJson();
        else if(request.url().host()=="api.first.org") bytes=R"({"status":"OK","status-code":200,"version":"1.0","total":0,"offset":0,"limit":100,"data":[]})";
        else if(request.url().host()=="raw.githubusercontent.com") bytes=R"({"catalogVersion":"synthetic","dateReleased":"2026-10-04T00:00:00Z","count":0,"vulnerabilities":[]})";
        else qFatal("Unexpected synthetic transport URL");
        return new Reply(request,bytes,fail,hold,this);
    }
};
void selectProject(ProjectPage& page,const QString& id)
{
    auto* list=page.findChild<QListWidget*>("projectList");
    if(id.isEmpty()){list->setCurrentRow(-1);return;}
    for(int i=0;i<list->count();++i)if(list->item(i)->data(Qt::UserRole).toString()==id){list->setCurrentRow(i);return;}
    qFatal("Synthetic project missing");
}
struct Fixture {
    QTemporaryDir dir;AppDatabase db;AppLogger logger;ProjectRepository projects{db,logger};ComponentRepository components{db};
    Project a,b;Network net;std::unique_ptr<ProjectPage> page;
    VulnerabilityController* owner=nullptr;VulnerabilityPage* view=nullptr;
    bool open() {
        QString error;
        if(!logger.open(dir.filePath("synthetic.log"))||!db.open(dir.filePath("INTERNAL_DB_PATH_SECRET_123.db"),error)
           ||!projects.create("Synthetic A","<b>literal</b>",a).ok()||!projects.create("Synthetic B",{},b).ok())return false;
        SbomDocument d;d.components={{"one","library","synthetic","1.0.0","pkg:npm/synthetic@1.0.0"}};
        if(!components.replaceForProject(a.id,d).ok()||!components.replaceForProject(b.id,d).ok())return false;
        page=std::make_unique<ProjectPage>(projects,components,logger,cache(),nullptr,&net);
        owner=page->findChild<VulnerabilityController*>();view=page->findChild<VulnerabilityPage*>();
        QObject::connect(owner,&VulnerabilityController::consentRequested,owner,[this]{owner->consent(true);});
        QObject::connect(&owner->riskEvidence(),&RiskEvidenceController::consentRequested,owner,[this]{owner->riskEvidence().consent(true);});
        selectProject(*page,a.id);owner->reload();return owner&&view;
    }
    QString cache() const {return dir.filePath("INTERNAL_CACHE_SECRET_456/osv-v1");}
    void selectCandidate(int row) {
        auto* table=page->findChild<QTableView*>("candidateTable");
        table->setCurrentIndex(row<0?QModelIndex():table->model()->index(row,0));
    }
    QPushButton* button() const {return page->findChild<QPushButton*>("exportFindingReport");}
    QFileDialog* dialog() const {return page->findChild<QFileDialog*>("saveFindingReport");}
    void acceptSave(const QString& path) {auto* d=dialog();d->selectFile(path);QMetaObject::invokeMethod(d,"accept",Qt::DirectConnection);}
    ~Fixture(){page.reset();QThreadPool::globalInstance()->waitForDone();}
};
// Deterministically occupy the pool without adding a production delay or fault hook.
struct BlockPool {
    QThreadPool* pool=QThreadPool::globalInstance();int previous=pool->maxThreadCount();
    QSemaphore started,release;QFuture<void> blocker;
    BlockPool(){pool->waitForDone();pool->setMaxThreadCount(1);blocker=QtConcurrent::run([this]{started.release();release.acquire();});started.acquire();}
    void unblock(){if(!blocker.isFinished()){release.release();blocker.waitForFinished();}}
    ~BlockPool(){unblock();pool->waitForDone();pool->setMaxThreadCount(previous);}
};
}

class Phase13Test final : public QObject {
    Q_OBJECT
private:
    void ready(Fixture& f) {
        QTRY_VERIFY(f.owner->loaded()&&!f.owner->loading());
        f.owner->select(0);f.owner->query(QueryMode::Refresh);QTRY_VERIFY(!f.owner->busy());
        QCOMPARE(f.owner->state(),QueryState::Success);QVERIFY(f.owner->applicability());
        f.selectCandidate(0);QVERIFY(f.owner->riskEvidence().hasRequest());
        f.owner->riskEvidence().load(EvidenceLoadMode::CacheOnly);QTRY_VERIFY(!f.owner->riskEvidence().busy());
        QCOMPARE(f.owner->riskEvidence().state(),EvidenceOperationState::Complete);
        QVERIFY(f.button()->isEnabled());
    }
private slots:
    void initTestCase(){QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);qRegisterMetaType<ReportWriteResult>();}
    void semantics();void presentationConsistency();void multiCve();void noCve();void freshness();void htmlSafety();void privacy();
    void staticTime();void provenance();void candidateCapture();void eligibility();void retainedRisk();void ownership();void projectFailure();
    void saveCancel();void saveSnapshot();void singleFlight();void workerLifetime();void fileSafety();void noSideEffects();
    void resolvedPath();void scale();void architecture();
};

void Phase13Test::semantics()
{
    for(int i=0;i<4;++i){auto p=profile();if(i==0)p.kev[0].status=KevStatus::Listed;
        if(i==2)p.epss[0].percentile=.5;if(i==3){p.epss[0].status=EpssStatus::Failed;p.epss[0].percentile.reset();}
        const auto a=RiskPriorityEvaluator::evaluate(p,time());QCOMPARE(a.priority,PriorityClass(i));
        const auto out=plain(FindingReport::render(context(p,a)));
        QVERIFY(out.contains(PresentationText::priorityLabel(a.priority)));QVERIFY(out.contains(PresentationText::supportLabel(a.support)));
        QVERIFY(out.contains("Current Finding Analysis Report"));QVERIFY(out.contains(QStringLiteral("一个 confirmed Finding")));
        QVERIFY(out.contains("Affected"));QVERIFY(out.contains(QStringLiteral(APPLICATION_VERSION)));
    }
    static_assert(!std::is_copy_assignable_v<FindingReportContext>);
}
void Phase13Test::presentationConsistency()
{
    const auto p=profile();const auto a=RiskPriorityEvaluator::evaluate(p,time());
    RiskPresentationView view;view.showResult(p,a);const auto out=plain(FindingReport::render(context(p,a)));
    for(const auto& text:{PresentationText::conclusionText(a),PresentationText::whyText(a),PresentationText::driverText(a),PresentationText::boundaryText(a)})
        for(const auto& line:text.split('\n')){QVERIFY(out.contains(line));QVERIFY(view.toPlainText().contains(line));}
}
void Phase13Test::multiCve()
{
    auto p=profile();auto e=p.epss.first();e.cve=B;e.status=EpssStatus::NotScored;e.probability.reset();e.percentile.reset();p.epss.append(e);
    auto k=p.kev.first();k.cve=B;p.kev.append(k);const auto a=RiskPriorityEvaluator::evaluate(p,time());
    QCOMPARE(a.support,DecisionEvidenceSupport::Partial);QCOMPARE(a.driverCve,A);
    const auto out=plain(FindingReport::render(context(p,a)));QVERIFY(out.contains(A));QVERIFY(out.contains(B));QVERIFY(out.contains("NotScored"));
    QVERIFY(out.contains("Partial"));QVERIFY(out.contains("Driver CVE："+A));
    QVERIFY(!out.contains("Coverage Score"));QVERIFY(!out.contains("Confidence Score"));
}
void Phase13Test::noCve()
{
    auto p=profile();p.epss={};p.kev={};EpssEvidence e;e.status=EpssStatus::NotQueryable;KevEvidence k;k.status=KevStatus::NotQueryable;p.epss={e};p.kev={k};
    const auto a=RiskPriorityEvaluator::evaluate(p,time());const auto out=plain(FindingReport::render(context(p,a)));
    QVERIFY(out.contains("No-CVE"));QVERIFY(out.count("NotQueryable")>=2);QVERIFY(out.contains(QStringLiteral("无可关联项")));
    QVERIFY(out.contains(QStringLiteral("缺失、未评分和失败均不等于零")));
    QVERIFY(out.contains(QStringLiteral("概率 (Probability)：未提供")));QVERIFY(!out.contains("Fresh)"));
}
void Phase13Test::freshness()
{
    for(int i=0;i<3;++i){auto p=profile();const auto evaluated=i==0?time():i==1?time().addSecs(90000):time().addSecs(-60);
        const auto a=RiskPriorityEvaluator::evaluate(p,evaluated);const auto out=plain(FindingReport::render(context(p,a)));
        QVERIFY(out.contains(PresentationText::effectiveFreshnessLabel(a.evidenceFreshness[0].epss)));
        QVERIFY(out.contains(PresentationText::profileSnapshotFreshnessLabel(EvidenceFreshness::Fresh)));
    }
    auto p=profile();p.epss[0].acquisition=EvidenceAcquisition::StaleFallback;
    const auto a=RiskPriorityEvaluator::evaluate(p,time().addSecs(3600));
    QCOMPARE(a.evidenceFreshness[0].epss,EffectiveDecisionFreshness::Stale);
    const auto out=plain(FindingReport::render(context(p,a)));QVERIFY(out.contains("Stale Fallback"));QVERIFY(out.contains("Stale"));
}
void Phase13Test::htmlSafety()
{
    const QString malicious="<script>alert('x')</script><img src=\"https://example.invalid/x\"><b>literal</b> & \" '";
    auto p=profile();p.severity.items.append({malicious,malicious,malicious,malicious});p.kev[0].entry.insert("notes",malicious);
    p.epss[0].providerVersion=malicious;auto a=RiskPriorityEvaluator::evaluate(p,time());a.driverCve=malicious;
    const auto c=context(p,a,malicious);const auto html=FindingReport::render(c);const auto out=plain(html);
    QVERIFY(out.contains(malicious));QVERIFY(html.contains("&lt;script&gt;"));QVERIFY(html.contains("&#39;"));QVERIFY(html.contains("&quot;"));
    for(const auto& forbidden:{"<script","<img","<iframe","<link","<a ","<b>literal"})QVERIFY(!html.contains(forbidden));
    QVERIFY(html.contains("<meta charset=\"utf-8\">"));QVERIFY(html.contains("<style>"));QVERIFY(html.contains("@media print"));
}
void Phase13Test::privacy()
{
    auto p=profile();p.key.componentId="INTERNAL_COMPONENT_ID_SECRET_ABC";
    p.kev[0].entry.insert("databasePath","INTERNAL_DB_PATH_SECRET_123");p.kev[0].entry.insert("cachePath","INTERNAL_CACHE_SECRET_456");
    p.kev[0].entry.insert("projectId","INTERNAL_PROJECT_ID_SECRET_789");
    const QString description="<script>alert('x')</script>\nC:\\Users\\Alice\\Desktop\\demo\ntoken text abc123\n<b>literal</b>";
    const auto a=RiskPriorityEvaluator::evaluate(p,time());const auto html=FindingReport::render(context(p,a,description));
    for(const auto* marker:{"INTERNAL_DB_PATH_SECRET_123","INTERNAL_CACHE_SECRET_456","INTERNAL_PROJECT_ID_SECRET_789","INTERNAL_COMPONENT_ID_SECRET_ABC"})QVERIFY(!html.contains(marker));
    for(const auto& line:description.split('\n'))QVERIFY(plain(html).contains(line));
    QVERIFY(!html.contains("<script>"));QVERIFY(html.contains("token text abc123"));QVERIFY(html.contains("C:\\Users\\Alice\\Desktop\\demo"));
}
void Phase13Test::staticTime()
{
    auto p=profile();const auto a=RiskPriorityEvaluator::evaluate(p,time().addSecs(90000));const auto c=context(p,a);
    const auto one=FindingReport::render(c);QTest::qWait(5);QCOMPARE(FindingReport::render(c),one);
    const auto out=plain(one);for(const auto& t:{c.profile.generatedAt,c.assessment.evaluatedAt,c.capturedAt})QVERIFY(out.contains(t.toUTC().toString(Qt::ISODateWithMs)));
    for(const auto* label:{"Profile Generated At","Assessment Evaluated At","Report Snapshot Captured At","Time Scope"})QVERIFY(out.contains(label));
    QVERIFY(out.contains(QStringLiteral("不表示之后阅读该 HTML 文件时的实时状态")));
}
void Phase13Test::resolvedPath()
{
    auto p=profile();p.dependency.captured=true;p.dependency.path=DependencyPathState::ResolvedPathFound;p.dependency.root=DependencyRootStatus::RootAvailable;
    p.dependency.referenceResolution=ReferenceResolutionCompleteness::Complete;p.dependency.depthFromRoot=2;
    const auto a=RiskPriorityEvaluator::evaluate(p,time());const auto out=plain(FindingReport::render(context(p,a)));
    QVERIFY(out.contains(PresentationText::dependencyPathLabel(DependencyPathState::ResolvedPathFound)));
    QVERIFY(out.contains(QStringLiteral("依赖路径不证明运行时可达")));QVERIFY(out.contains(QStringLiteral("不保证 SBOM 依赖覆盖完整")));
}

void Phase13Test::provenance()
{
    Fixture f;QVERIFY(f.open());ready(f);if(QTest::currentTestFailed())return;
    QCOMPARE(f.page->captureFindingReport(0).context->osvAcquisition,QString("Live"));
    for(bool stale:{false,true}){
        auto snapshot=*f.owner->snapshot();snapshot.fetchedAt=QDateTime::currentDateTimeUtc().addSecs(stale?-90000:0);
        QCOMPARE(OsvCache(f.cache()).write(snapshot),QueryError::None);
        f.owner->query(QueryMode::CacheOnly);QTRY_VERIFY(!f.owner->busy());QCOMPARE(f.owner->state(),QueryState::Success);
        f.selectCandidate(0);f.owner->riskEvidence().load(EvidenceLoadMode::CacheOnly);QTRY_VERIFY(!f.owner->riskEvidence().busy());
        const auto captured=f.page->captureFindingReport(0);QVERIFY(captured.context);
        const auto out=plain(FindingReport::render(*captured.context));
        QVERIFY(out.contains("Analysis Source Type：OSV Package-Version Query"));
        QVERIFY(out.contains(stale?"OSV Snapshot Acquisition：Stale Cache":"OSV Snapshot Acquisition：Fresh Cache"));
        QVERIFY(out.contains(snapshot.fetchedAt.toUTC().toString(Qt::ISODateWithMs)));
    }
}
void Phase13Test::candidateCapture()
{
    Fixture f;QVERIFY(f.open());ready(f);if(QTest::currentTestFailed())return;
    const auto first=f.page->captureFindingReport(f.view->currentCandidateIndex());QVERIFY(first.context);
    QCOMPARE(first.context->osvId,QString("SYNTHETIC-REPORT-A"));
    const auto bytes=FindingReport::render(*first.context);
    f.selectCandidate(1);QCOMPARE(f.owner->selected(),qsizetype(0)); // Component row is not Candidate row.
    QVERIFY(!f.button()->isEnabled());f.owner->riskEvidence().load(EvidenceLoadMode::CacheOnly);QTRY_VERIFY(!f.owner->riskEvidence().busy());
    const auto second=f.page->captureFindingReport(f.view->currentCandidateIndex());QVERIFY(second.context);
    QCOMPARE(second.context->osvId,QString("SYNTHETIC-REPORT-B"));QVERIFY(second.context->cveAliases.contains(B));
    QCOMPARE(FindingReport::render(*first.context),bytes);
    QVERIFY(!f.page->captureFindingReport(-1).context);QVERIFY(!f.page->captureFindingReport(99).context);
    // Passing A while B owns the completed profile is rejected, never silently changed to B.
    QCOMPARE(f.page->captureFindingReport(0).error,ReportError::StateChanged);
}
void Phase13Test::eligibility()
{
    Fixture f;QVERIFY(f.open());QVERIFY(!f.button()->isEnabled());QVERIFY(!f.page->captureFindingReport(0).context);
    ready(f);if(QTest::currentTestFailed())return;
    f.net.hold=true;const auto count=f.net.requests;f.owner->query(QueryMode::Refresh);
    QVERIFY(!f.button()->isEnabled());QVERIFY(!f.page->captureFindingReport(0).context);
    QTRY_VERIFY(f.net.requests>count);QVERIFY(f.owner->snapshot());f.owner->cancel();QTRY_VERIFY(!f.owner->busy());
    QCOMPARE(f.owner->state(),QueryState::Cancelled);QVERIFY(f.owner->snapshot());QVERIFY(!f.page->captureFindingReport(0).context);
    f.net.hold=false;f.net.fail=true;f.owner->query(QueryMode::Refresh);QTRY_VERIFY(!f.owner->busy());
    QCOMPARE(f.owner->state(),QueryState::Failed);QVERIFY(f.owner->snapshot());QVERIFY(!f.button()->isEnabled());
    QVERIFY(!f.page->captureFindingReport(0).context);
}
void Phase13Test::retainedRisk()
{
    for(int mode=0;mode<3;++mode){
        Fixture f;QVERIFY(f.open());ready(f);if(QTest::currentTestFailed())return;
        auto& risk=f.owner->riskEvidence();const auto previous=RiskEvidence::profileText(*risk.profile());
        if(mode==0){f.net.hold=true;risk.load(EvidenceLoadMode::Refresh);
            QVERIFY(!f.button()->isEnabled());QVERIFY(!f.page->captureFindingReport(0).context);
            QTRY_VERIFY(f.net.requests>1);risk.cancel();QTRY_VERIFY(!risk.busy());QCOMPARE(risk.state(),EvidenceOperationState::Cancelled);
        }else if(mode==1){
            QSqlQuery q(f.db.connection());QVERIFY(q.exec("DROP TABLE dependency_capture"));
            risk.load(EvidenceLoadMode::CacheOnly);QTRY_VERIFY(!risk.busy());QCOMPARE(risk.state(),EvidenceOperationState::Failed);
        }else{
            SbomDocument d;d.components={{"replacement","library","synthetic","1.0.0","pkg:npm/synthetic@1.0.0"}};
            QVERIFY(f.components.replaceForProject(f.a.id,d).ok());risk.load(EvidenceLoadMode::CacheOnly);
            QTRY_VERIFY(!risk.busy());QCOMPARE(risk.state(),EvidenceOperationState::Stale);
        }
        if(mode<2){QVERIFY(risk.profile());QVERIFY(risk.assessment());QCOMPARE(RiskEvidence::profileText(*risk.profile()),previous);}
        QVERIFY(!f.button()->isEnabled());QVERIFY(!f.page->captureFindingReport(0).context);
    }
}
void Phase13Test::ownership()
{
    Fixture f;QVERIFY(f.open());ready(f);if(QTest::currentTestFailed())return;
    auto& risk=f.owner->riskEvidence();
    // Fault-inject published value copies through the fixture's non-const owner; no production bypass or setter.
    auto& p=const_cast<RiskEvidenceProfile&>(*risk.profile());auto& a=const_cast<RiskPriorityAssessment&>(*risk.assessment());
    const auto savedP=p;const auto savedA=a;
    p.key.osvId="WRONG-PROFILE";QCOMPARE(f.page->captureFindingReport(0).error,ReportError::StateChanged);p=savedP;
    a.key.componentId="WRONG-ASSESSMENT";QCOMPARE(f.page->captureFindingReport(0).error,ReportError::StateChanged);a=savedA;
    a.profileGeneratedAt=a.profileGeneratedAt.addSecs(1);QCOMPARE(f.page->captureFindingReport(0).error,ReportError::StateChanged);a=savedA;
    // Controller deliberately attached to B while ProjectPage authority remains A.
    f.owner->setProject(f.b.id);f.owner->reload();QTRY_VERIFY(f.owner->loaded()&&!f.owner->loading());
    f.owner->select(0);f.owner->query(QueryMode::CacheOnly);QTRY_VERIFY(!f.owner->busy());
    f.selectCandidate(0);risk.load(EvidenceLoadMode::CacheOnly);QTRY_VERIFY(!risk.busy());QVERIFY(risk.profile());
    QCOMPARE(f.page->currentProjectId(),f.a.id);QCOMPARE(f.page->captureFindingReport(0).error,ReportError::StateChanged);
}
void Phase13Test::projectFailure()
{
    Fixture f;QVERIFY(f.open());ready(f);if(QTest::currentTestFailed())return;
    const auto databasePath=f.db.filePath();f.db.close();QCOMPARE(f.page->captureFindingReport(0).error,ReportError::ProjectRead);
    f.view->reportExportRequested(0);QVERIFY(!f.dialog());
    const auto message=f.page->findChild<QLabel*>("reportExportStatus")->text();
    QVERIFY(!message.contains(databasePath));QVERIFY(!message.contains("SQL"));
    selectProject(*f.page,{});QCOMPARE(f.page->captureFindingReport(0).error,ReportError::NoAnalysis);
}
void Phase13Test::saveCancel()
{
    Fixture f;QVERIFY(f.open());ready(f);if(QTest::currentTestFailed())return;
    QSignalSpy finished(f.page.get(),&ProjectPage::reportExportFinished);f.button()->click();QVERIFY(f.dialog());
    QVERIFY(f.page->reportExporting());QVERIFY(!f.button()->isEnabled());
    const auto path=f.dir.filePath("cancelled.html");f.dialog()->selectFile(path);f.dialog()->reject();
    QTRY_VERIFY(!f.page->reportExporting());QVERIFY(f.button()->isEnabled());QVERIFY(finished.isEmpty());QVERIFY(!QFile::exists(path));
    QVERIFY(f.page->findChild<QLabel*>("reportExportStatus")->text().isEmpty());
}
void Phase13Test::saveSnapshot()
{
    Fixture f;QVERIFY(f.open());ready(f);if(QTest::currentTestFailed())return;
    QSignalSpy finished(f.page.get(),&ProjectPage::reportExportFinished);
    f.button()->click();QVERIFY(f.dialog());f.selectCandidate(1);
    const auto path=f.dir.filePath("snapshot.html");f.acceptSave(path);QTRY_COMPARE(finished.size(),1);
    const auto result=qvariant_cast<ReportWriteResult>(finished[0][0]);QCOMPARE(result.error,ReportError::None);
    const auto bytes=read(path);QVERIFY(bytes.contains("SYNTHETIC-REPORT-A"));QVERIFY(!bytes.contains("SYNTHETIC-REPORT-B"));
    QCOMPARE(f.view->currentCandidateIndex(),qsizetype(1));QCOMPARE(result.outputBytes,bytes.size());
}
void Phase13Test::singleFlight()
{
    Fixture f;QVERIFY(f.open());ready(f);if(QTest::currentTestFailed())return;
    QSignalSpy finished(f.page.get(),&ProjectPage::reportExportFinished);BlockPool blocked;
    f.button()->click();QVERIFY(f.dialog());const auto first=f.dir.filePath("one.html");f.acceptSave(first);
    QTRY_VERIFY(f.dialog()==nullptr);QVERIFY(f.page->reportExporting());QVERIFY(!f.button()->isEnabled());
    f.view->reportExportRequested(0);QVERIFY(!f.dialog());QVERIFY(finished.isEmpty());QVERIFY(!QFile::exists(first));
    int ticks=0;QTimer timer;connect(&timer,&QTimer::timeout,this,[&]{++ticks;});timer.start(1);QTRY_VERIFY(ticks>2);
    blocked.unblock();QTRY_COMPARE(finished.size(),1);QVERIFY(!f.page->reportExporting());QVERIFY(f.button()->isEnabled());
    f.button()->click();QVERIFY(f.dialog());const auto second=f.dir.filePath("two.html");f.acceptSave(second);
    QTRY_COMPARE(finished.size(),2);QVERIFY(QFile::exists(second));
}
void Phase13Test::workerLifetime()
{
    Fixture f;QVERIFY(f.open());ready(f);if(QTest::currentTestFailed())return;
    QSignalSpy finished(f.page.get(),&ProjectPage::reportExportFinished);BlockPool blocked;
    f.button()->click();QVERIFY(f.dialog());const auto path=f.dir.filePath("detached.html");f.acceptSave(path);
    QTRY_VERIFY(f.dialog()==nullptr);QVERIFY(!QFile::exists(path));QPointer<ProjectPage> receiver=f.page.get();f.page.reset();QVERIFY(receiver.isNull());
    blocked.unblock();QTRY_VERIFY(QFile::exists(path));QVERIFY(read(path).endsWith("</body></html>"));QVERIFY(finished.isEmpty());
}
void Phase13Test::fileSafety()
{
    const auto p=profile();const auto a=RiskPriorityEvaluator::evaluate(p,time());const auto c=context(p,a);QTemporaryDir dir;
    const auto path=dir.filePath("target.html");QVERIFY(write(path,"old-target"));
    QCOMPARE(FindingReport::write(c,path).error,ReportError::None);QCOMPARE(read(path),FindingReport::render(c));
    QCOMPARE(FindingReport::write(c,dir.filePath("missing/path.html")).error,ReportError::FileOpen);
    class FailingFile final : public QSaveFile {
    public:using QSaveFile::QSaveFile;
    protected:qint64 writeData(const char* data,qint64 length) override {QSaveFile::writeData(data,qMin(length,qint64(8)));return -1;}
    };
    const auto previous=read(path);
    {FailingFile file(path);QCOMPARE(FindingReport::saveBytes(file,"will-fail"),ReportError::FileWrite);}
    QCOMPARE(read(path),previous);
#ifdef Q_OS_WIN
    // Deny replacement of this isolated target while allowing temp-file creation/write.
    const HANDLE handle=CreateFileW(reinterpret_cast<LPCWSTR>(path.utf16()),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    QVERIFY(handle!=INVALID_HANDLE_VALUE);const auto result=FindingReport::write(c,path);CloseHandle(handle);
    QCOMPARE(result.error,ReportError::FileCommit);QCOMPARE(read(path),previous);
#endif
    for(int i=0;i<=int(ReportError::FileCommit);++i){const auto text=FindingReport::userMessage(ReportError(i));QVERIFY(!text.isEmpty());QVERIFY(!text.contains(path));}
}
void Phase13Test::noSideEffects()
{
    Fixture f;QVERIFY(f.open());ready(f);if(QTest::currentTestFailed())return;QThreadPool::globalInstance()->waitForDone();
    const auto dbBytes=read(f.db.filePath());const auto snapshot=f.owner->snapshot();const auto profileBefore=RiskEvidence::profileText(*f.owner->riskEvidence().profile());
    const auto assessmentBefore=*f.owner->riskEvidence().assessment();const auto generation=f.owner->generation();const int requests=f.net.requests;
    const auto id=f.page->currentProjectId();const auto row=f.view->currentCandidateIndex();const auto c=f.page->captureFindingReport(row);QVERIFY(c.context);
    const auto path=f.dir.filePath("local-only.html");QCOMPARE(FindingReport::write(*c.context,path).error,ReportError::None);
    QCOMPARE(f.net.requests,requests);QCOMPARE(read(f.db.filePath()),dbBytes);QCOMPARE(f.owner->generation(),generation);
    QCOMPARE(f.owner->snapshot()->fetchedAt,snapshot->fetchedAt);QCOMPARE(f.page->currentProjectId(),id);QCOMPARE(f.view->currentCandidateIndex(),row);
    QCOMPARE(RiskEvidence::profileText(*f.owner->riskEvidence().profile()),profileBefore);QCOMPARE(*f.owner->riskEvidence().assessment(),assessmentBefore);
    const auto bytes=read(path);QVERIFY(!bytes.contains(f.a.id.toUtf8()));QVERIFY(!bytes.contains(f.db.filePath().toUtf8()));
    QVERIFY(!bytes.contains(f.cache().toUtf8()));QVERIFY(!bytes.contains(f.owner->riskEvidence().profile()->key.componentId.toUtf8()));
}

void Phase13Test::scale()
{
    Fixture f;QVERIFY(f.open());ready(f);if(QTest::currentTestFailed())return;
    auto& raw=const_cast<OsvSnapshot&>(*f.owner->snapshot());auto& app=const_cast<ApplicabilitySnapshot&>(*f.owner->applicability());
    auto& risk=f.owner->riskEvidence();auto p=*risk.profile();p.epss.clear();p.kev.clear();QJsonArray aliases;
    for(int i=0;i<10000;++i){
        const auto cve=QString("CVE-2026-%1").arg(100001+i);aliases.append(cve);
        EpssEvidence e;e.cve=cve;e.status=EpssStatus::Available;e.probability=.03;e.percentile=.94;e.fetchedAt=p.generatedAt;
        e.freshness=EvidenceFreshness::Fresh;e.acquisition=EvidenceAcquisition::Cache;e.providerVersion="API-"+cve;p.epss.append(e);
        KevEvidence k;k.cve=cve;k.status=KevStatus::NotListed;k.fetchedAt=p.generatedAt;k.freshness=EvidenceFreshness::Fresh;
        k.acquisition=EvidenceAcquisition::Cache;k.entry.insert("notes","NOTE-"+cve);p.kev.append(k);
    }
    raw.candidates[0].record.insert("aliases",aliases);app.findings[0].cveAliases=raw.candidates[0].cveAliases();
    auto a=RiskPriorityEvaluator::evaluate(p,p.generatedAt);a.reasons.clear();
    for(const auto& e:p.epss)a.reasons.append({PriorityReasonCode::EpssDriverSelected,e.cve,PriorityDriverKind::Epss});
    const_cast<RiskEvidenceProfile&>(*risk.profile())=p;const_cast<RiskPriorityAssessment&>(*risk.assessment())=a;
    QElapsedTimer elapsed;elapsed.start();const auto captured=f.page->captureFindingReport(0);const auto captureNs=elapsed.nsecsElapsed();
    QVERIFY(captured.context);QCOMPARE(captured.context->cveAliases.size(),10000);QCOMPARE(captured.context->profile.epss.size(),10000);
    QSignalSpy finished(f.page.get(),&ProjectPage::reportExportFinished);BlockPool blocked;
    f.button()->click();QVERIFY(f.dialog());const auto path=f.dir.filePath("large.html");f.acceptSave(path);QTRY_VERIFY(f.dialog()==nullptr);
    const int requests=f.net.requests;selectProject(*f.page,f.b.id); // GUI authority can change; captured report must stay A.
    int ticks=0;QTimer timer;connect(&timer,&QTimer::timeout,this,[&]{++ticks;});timer.start(1);
    QTRY_VERIFY(ticks>=3);ticks=0;blocked.unblock();QTRY_COMPARE_WITH_TIMEOUT(finished.size(),1,30000);QVERIFY(ticks>0);
    const auto result=qvariant_cast<ReportWriteResult>(finished[0][0]);QCOMPARE(result.error,ReportError::None);
    const auto bytes=read(path);QCOMPARE(result.outputBytes,bytes.size());QVERIFY(bytes.endsWith("</body></html>"));
    QCOMPARE(bytes.count("<h3>FIRST EPSS</h3>"),10000);QCOMPARE(bytes.count("<h3>CISA KEV</h3>"),10000);
    // Driver repeats its provenance; count complete rows only inside the Evidence section.
    const auto evidenceStart=bytes.indexOf("<h2>9");const auto evidenceEnd=bytes.indexOf("<h2>10");
    QVERIFY(evidenceStart>=0&&evidenceEnd>evidenceStart);
    const auto evidenceBytes=bytes.mid(evidenceStart,evidenceEnd-evidenceStart);
    QCOMPARE(evidenceBytes.count(QStringLiteral("FIRST API Version：API-CVE-2026-").toUtf8()),10000);
    QCOMPARE(evidenceBytes.count(QStringLiteral("notes：NOTE-CVE-2026-").toUtf8()),10000);
    QCOMPARE(bytes.count("<h3>EpssDriverSelected</h3>"),10000);
    QSet<QString> seen;const QRegularExpression pattern("CVE-2026-[0-9]+");auto matches=pattern.globalMatch(QString::fromUtf8(bytes));
    while(matches.hasNext())seen.insert(matches.next().captured());QCOMPARE(seen.size(),10000);
    QVERIFY(bytes.contains("Synthetic A"));QVERIFY(!bytes.contains("Synthetic B"));QCOMPARE(f.net.requests,requests);
    QCOMPARE(f.page->currentProjectId(),f.b.id);
    const QJsonObject metrics{{"aliasCount",10000},{"epssRows",10000},{"kevRows",10000},{"reasonCount",10000},
        {"captureMilliseconds",captureNs/1e6},{"renderMilliseconds",result.renderNanoseconds/1e6},
        {"writeMilliseconds",result.writeNanoseconds/1e6},{"outputBytes",double(result.outputBytes)},
        {"guiTicksDuringWorker",ticks},{"workerQueuedOffGui",true},{"complete",true},{"projectSwitchSafe",true}};
    QVERIFY(write(QCoreApplication::applicationDirPath()+"/phase13-scale.json",QJsonDocument(metrics).toJson()));
    qInfo().noquote()<<QJsonDocument(metrics).toJson(QJsonDocument::Compact);
}
void Phase13Test::architecture()
{
    const QString root=QStringLiteral(PHASE13_SOURCE_DIR)+"/";
    const auto core=read(root+"src/FindingReport.cpp");const auto header=read(root+"src/FindingReport.h");
    const auto renderer=core.mid(core.indexOf("QByteArray FindingReport::render"),core.indexOf("ReportError FindingReport::saveBytes")-core.indexOf("QByteArray FindingReport::render"));
    for(const auto* forbidden:{"QWidget","QSql","Controller","Repository","QNetwork","currentDateTime","RiskPriorityEvaluator","toHtml()","QFile","QSaveFile",".key","QJsonDocument"})QVERIFY2(!renderer.contains(forbidden),forbidden);
    for(const auto* forbidden:{"databasePath","projectId","componentId","candidateIndex","RiskEvidenceRequest"})QVERIFY2(!header.contains(forbidden),forbidden);
    const auto project=read(root+"src/ProjectPage.cpp");const auto pageHeader=read(root+"src/ProjectPage.h");
    const auto capture=project.mid(project.indexOf("ReportCaptureResult ProjectPage::captureFindingReport"),project.indexOf("void ProjectPage::updateReportAvailability")-project.indexOf("ReportCaptureResult ProjectPage::captureFindingReport"));
    for(const auto* forbidden:{"QFileDialog","QtConcurrent","RiskPriorityEvaluator","processEvents","invokeMethod"})QVERIFY2(!capture.contains(forbidden),forbidden);
    QVERIFY(project.contains("[context, destination = paths.first()]"));QVERIFY(project.contains("Qt::DirectConnection"));
    QVERIFY(!pageHeader.contains("m_reportCandidate"));QVERIFY(!read(root+"src/VulnerabilityPage.h").contains("m_project"));
    QVERIFY(project.indexOf("const auto captured = captureFindingReport(candidateIndex)")<project.indexOf("new QFileDialog"));
    QVERIFY(!project.contains("DontConfirmOverwrite"));QVERIFY(core.contains("file.setDirectWriteFallback(false)"));
    QVERIFY(!renderer.contains("switch"));QVERIFY(!renderer.contains(".left("));
    QCOMPARE(AppDatabase::SchemaVersion,4);
}

QTEST_MAIN(Phase13Test)
#include "Phase13Test.moc"
