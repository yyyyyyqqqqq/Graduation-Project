#include "RiskPriority.h"
#include "RiskEvidenceController.h"
#include "VulnerabilityController.h"
#include "VulnerabilityPage.h"
#include "CveIdentity.h"
#include "AppDatabase.h"
#include "AppLogger.h"
#include "AppPaths.h"
#include "ProjectRepository.h"
#include "ComponentRepository.h"
#include "SbomDocument.h"

#include <QTest>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QFile>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QTextBrowser>
#include <QTableView>
#include <QTabWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QLibraryInfo>
#include <QElapsedTimer>
#include <QThreadPool>
#include <QUuid>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace {
using Priority = PriorityClass;
using Support = DecisionEvidenceSupport;
using Freshness = EffectiveDecisionFreshness;
using Reason = PriorityReasonCode;
const QString A = "CVE-2026-1000001", B = "CVE-2026-1000002";
constexpr qint64 Day = 86400000;
QDateTime fixedTime() { return QDateTime::fromString("2026-09-27T00:00:00.000Z", Qt::ISODateWithMs); }
QDateTime now() { return QDateTime::currentDateTimeUtc(); }
QByteArray json(const QJsonObject& object) { return QJsonDocument(object).toJson(QJsonDocument::Compact); }
QByteArray read(const QString& path) { QFile file(path); return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray(); }
bool write(const QString& path, const QByteArray& bytes) { QFile file(path); return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size(); }
EpssEvidence epssValue(QString cve = A, double percentile = .91, QDateTime fetched = fixedTime())
{
    EpssEvidence e; e.cve = cve; e.status = EpssStatus::Available; e.fetchedAt = fetched;
    e.acquisition = EvidenceAcquisition::Cache; e.freshness = EvidenceFreshness::Fresh;
    e.probability = .035; e.percentile = percentile; e.providerVersion = "1.0"; e.scoreDate = fetched.date(); return e;
}
KevEvidence kevValue(QString cve = A, KevStatus status = KevStatus::NotListed, QDateTime fetched = fixedTime())
{
    KevEvidence k; k.cve = cve; k.status = status; k.fetchedAt = fetched;
    k.acquisition = EvidenceAcquisition::Cache; k.freshness = EvidenceFreshness::Fresh;
    k.catalogVersion = "synthetic-catalog"; k.dateReleased = "2026-09-25T00:00:00Z"; return k;
}
RiskEvidenceProfile profile()
{
    RiskEvidenceProfile p; p.key.componentId = "synthetic-component"; p.key.osvId = "SYNTHETIC-P10";
    p.key.snapshotIdentity = {"npm", "synthetic-phase10", "1.2.3", 1};
    p.key.snapshotFetchedAt = fixedTime(); p.key.applicabilityGeneration = 7;
    p.generatedAt = fixedTime(); p.epss = {epssValue()}; p.kev = {kevValue()}; return p;
}
RiskPriorityAssessment evaluate(const RiskEvidenceProfile& p, QDateTime time = fixedTime().addSecs(3600))
{ return RiskPriorityEvaluator::evaluate(p, time); }
bool hasReason(const RiskPriorityAssessment& a, Reason reason)
{ return std::any_of(a.reasons.begin(), a.reasons.end(), [reason](const auto& r) { return r.code == reason; }); }
QJsonObject score(QString cve = A, double percentile = .91)
{ return {{"cve", cve}, {"epss", "0.035"}, {"percentile", QString::number(percentile, 'g', 17)}, {"date", "2026-09-25"}}; }
QJsonObject kevEntry(QString cve = A)
{ return {{"cveID", cve}, {"vendorProject", "Synthetic"}, {"product", "Fixture"}, {"vulnerabilityName", "Synthetic only"},
    {"dateAdded", "2026-09-01"}, {"shortDescription", "Fixture evidence"}, {"requiredAction", "Synthetic provider action"}, {"dueDate", "2026-09-30"}}; }
QJsonObject catalog(bool listed = false)
{ return {{"catalogVersion", "synthetic-catalog"}, {"dateReleased", "2026-09-25T00:00:00Z"},
    {"count", listed ? 1 : 0}, {"vulnerabilities", listed ? QJsonArray{kevEntry()} : QJsonArray{}}}; }
QueryIdentity identity() { return {"npm", "synthetic-phase10", "1.2.3", 1}; }
VulnerabilityCandidate candidate(QString id = "SYNTHETIC-P10-1", QJsonArray aliases = {A})
{
    return {QJsonObject{{"id", id}, {"modified", "2026-01-01T00:00:00Z"}, {"aliases", aliases},
        {"affected", QJsonArray{QJsonObject{{"package", QJsonObject{{"ecosystem", "npm"}, {"name", "synthetic-phase10"}}},
            {"versions", QJsonArray{"1.2.3"}}}}},
        {"severity", QJsonArray{QJsonObject{{"type", "CVSS_V3"}, {"score", "CVSS:3.1/AV:N/AC:L/PR:N/UI:N/S:U/C:H/I:H/A:H"}}}}}};
}
struct Context {
    QTemporaryDir dir;
    AppDatabase db; AppLogger logger;
    ProjectRepository projects{db, logger}; ComponentRepository components{db};
    QString projectId;
    QString path() const { return dir.filePath("synthetic.db"); }
    QString cache() const { return dir.filePath("cache/osv-v1"); }
    QString riskCache() const { return dir.filePath("cache/risk-evidence-v1"); }
    bool open() {
        QString error; Project project;
        if (!logger.open(dir.filePath("synthetic.log")) || !db.open(path(), error)
            || !projects.create("Phase10 synthetic", {}, project).ok()) return false;
        projectId = project.id;
        SbomDocument d; d.components = {{"a", "library", "synthetic", "1.2.3", "pkg:npm/synthetic-phase10@1.2.3"},
            {"b", "library", "synthetic second", "1.2.3", "pkg:npm/synthetic-phase10@1.2.3"}};
        return components.replaceForProject(projectId, d).ok()
            && OsvCache(cache()).write({identity(), now(), {candidate(), candidate("SYNTHETIC-P10-2")}}) == QueryError::None;
    }
    bool seed(QDateTime epssAt, QDateTime kevAt, bool listed = false) {
        const auto parsed = RiskEvidence::parseKev(json(catalog(listed)), kevAt);
        return parsed && EpssCache(riskCache()).write({A, "1.0", epssAt, score()}) == QueryError::None
            && KevCache(riskCache()).write(*parsed) == QueryError::None;
    }
};
class HeldReply final : public QNetworkReply {
public:
    HeldReply(const QNetworkRequest& request, QObject* parent) : QNetworkReply(parent) {
        setRequest(request); setUrl(request.url()); open(ReadOnly | Unbuffered);
    }
    void abort() override { if (!isFinished()) { setError(OperationCanceledError, "synthetic cancellation"); setFinished(true); emit finished(); } }
    void deliver(QByteArray data) { bytes = std::move(data); setAttribute(QNetworkRequest::HttpStatusCodeAttribute, 200); emit readyRead(); setFinished(true); emit finished(); }
    qint64 bytesAvailable() const override { return bytes.size() - offset + QNetworkReply::bytesAvailable(); }
protected:
    qint64 readData(char* data, qint64 max) override { const auto n = qMin(max, qint64(bytes.size() - offset)); if (n <= 0) return -1; std::memcpy(data, bytes.constData() + offset, size_t(n)); offset += n; return n; }
private:
    QByteArray bytes; qint64 offset = 0;
};
class Network final : public QNetworkAccessManager {
public:
    QList<QPointer<HeldReply>> replies;
    bool allow = false;
protected:
    QNetworkReply* createRequest(Operation op, const QNetworkRequest& request, QIODevice* outgoing) override {
        if (!allow || op != GetOperation || outgoing) qFatal("Unexpected Phase10 network request");
        auto* reply = new HeldReply(request, this); replies.append(reply); return reply;
    }
};
}

#define LOAD_OWNER(owner, context) \
    owner.setProject(context.projectId); owner.reload(); QTRY_VERIFY(!owner.loading()); owner.select(0); \
    owner.query(QueryMode::CacheOnly); QTRY_VERIFY(!owner.busy()); QVERIFY(owner.riskEvidenceRequest(0)); \
    auto& risk = owner.riskEvidence(); risk.setRequest(owner.riskEvidenceRequest(0))
#define LOAD_PROFILE() risk.load(EvidenceLoadMode::CacheOnly); QTRY_VERIFY(!risk.busy()); QVERIFY(risk.profile()); QVERIFY(risk.assessment())

class Phase10Test final : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() { QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); }
    void priorityRules(); void unavailableEvidence(); void freshnessBoundaries(); void temporalInvalidity();
    void staleFallback(); void naturalExpiry(); void corroborationExpiry(); void multiCveDeterminism();
    void supportCoverage(); void provenance(); void reasonsAndExplanation(); void contextIsolation(); void defensiveInputs();
    void timerExpiry(); void timerCorroboration(); void refreshOverlap(); void publication(); void timerInvalidation();
    void staleTimerGeneration(); void lateTimer(); void destruction(); void ui(); void scale(); void manualFixtures();
};

void Phase10Test::priorityRules()
{
    auto p = profile(); p.kev[0].status = KevStatus::Listed; p.epss[0].percentile = .01;
    auto a = evaluate(p); QCOMPARE(a.priority, Priority::KnownExploited); QCOMPARE(a.support, Support::Complete);
    QCOMPARE(a.driverKind, PriorityDriverKind::Kev);
    for (const auto state : {EpssStatus::Failed, EpssStatus::NotScored, EpssStatus::InvalidResponse}) {
        p.epss[0].status = state; a = evaluate(p); QCOMPARE(a.priority, Priority::KnownExploited); QCOMPARE(a.support, Support::Complete);
    }
    p.epss.clear(); QCOMPARE(evaluate(p).support, Support::Complete);
    p = profile();
    for (double percentile : {.90, .899999, .91, 0.0, 1.0}) {
        p.epss[0].percentile = percentile; a = evaluate(p);
        QCOMPARE(a.priority, percentile >= .90 ? Priority::AboveResearchPercentileThreshold : Priority::BelowResearchPercentileThreshold);
        QCOMPARE(a.support, Support::Complete); QCOMPARE(*a.driverEpssProbability, .035); QCOMPARE(*a.driverEpssPercentile, percentile);
        QCOMPARE(a.epssPercentileThreshold, .90); QCOMPARE(a.rulesVersion, 1);
    }
}
void Phase10Test::unavailableEvidence()
{
    for (auto state : {EpssStatus::NotScored, EpssStatus::Failed, EpssStatus::InvalidResponse, EpssStatus::NotQueryable}) {
        auto p = profile(); p.epss[0].status = state; p.epss[0].percentile.reset(); p.epss[0].probability.reset();
        const auto a = evaluate(p); QCOMPARE(a.priority, Priority::InsufficientCurrentExploitEvidence);
        QCOMPARE(a.support, Support::Insufficient); QCOMPARE(a.driverKind, PriorityDriverKind::None);
        QVERIFY(!a.driverEpssPercentile); QVERIFY(!a.driverEpssProbability); QVERIFY(hasReason(a, Reason::NoFreshExploitDriver));
    }
    auto p = profile(); p.epss.clear(); p.kev.clear(); const auto a = evaluate(p);
    QCOMPARE(a.support, Support::Insufficient); QVERIFY(!a.nextFreshnessExpiryUtc);
}
void Phase10Test::freshnessBoundaries()
{
    const auto p = profile();
    for (qint64 age : {Day - 1000, Day - 1, Day, Day + 1000}) {
        const auto a = evaluate(p, fixedTime().addMSecs(age));
        QCOMPARE(a.evidenceFreshness[0].epss, age < Day ? Freshness::Fresh : Freshness::Stale);
        QCOMPARE(a.evidenceFreshness[0].kev, age < Day ? Freshness::Fresh : Freshness::Stale);
        QCOMPARE(a.priority, age < Day ? Priority::AboveResearchPercentileThreshold : Priority::InsufficientCurrentExploitEvidence);
        QCOMPARE(a.nextFreshnessExpiryUtc.has_value(), age < Day);
    }
    QCOMPARE(p.epss[0].freshness, EvidenceFreshness::Fresh); QCOMPARE(p.kev[0].freshness, EvidenceFreshness::Fresh);
    auto staleLabel = p; staleLabel.epss[0].freshness = EvidenceFreshness::Stale;
    QCOMPARE(evaluate(staleLabel).priority, Priority::AboveResearchPercentileThreshold);
}
void Phase10Test::temporalInvalidity()
{
    auto p = profile(); p.kev[0].status = KevStatus::Listed;
    for (bool fallback : {false, true}) {
        if (fallback) { p.epss[0].acquisition = EvidenceAcquisition::StaleFallback; p.kev[0].acquisition = EvidenceAcquisition::StaleFallback; }
        const auto a = evaluate(p, fixedTime().addMSecs(-1));
        QCOMPARE(a.evidenceFreshness[0].epss, Freshness::Ineligible); QCOMPARE(a.evidenceFreshness[0].kev, Freshness::Ineligible);
        QCOMPARE(a.support, Support::Insufficient); QVERIFY(!a.nextFreshnessExpiryUtc);
        QVERIFY(hasReason(a, Reason::EvidenceTimeInvalidForCurrentDecision)); QVERIFY(!hasReason(a, Reason::StaleKevListedContextOnly));
    }
    p = profile(); p.epss[0].fetchedAt = {}; p.kev[0].fetchedAt = {};
    auto a = evaluate(p); QCOMPARE(a.evidenceFreshness[0].epss, Freshness::Ineligible);
    p.epss[0].status = EpssStatus::Failed; p.kev[0].status = KevStatus::Unknown;
    a = evaluate(p); QCOMPARE(a.evidenceFreshness[0].epss, Freshness::NotApplicable); QCOMPARE(a.evidenceFreshness[0].kev, Freshness::NotApplicable);
    QVERIFY(!hasReason(a, Reason::EvidenceTimeInvalidForCurrentDecision));
    a = evaluate(profile(), {}); QCOMPARE(a.support, Support::Insufficient); QVERIFY(!a.evaluatedAt.isValid());
}
void Phase10Test::staleFallback()
{
    auto p = profile(); p.kev[0].status = KevStatus::Listed;
    p.kev[0].acquisition = p.epss[0].acquisition = EvidenceAcquisition::StaleFallback;
    const auto a = evaluate(p); QCOMPARE(a.evidenceFreshness[0].kev, Freshness::Stale); QCOMPARE(a.evidenceFreshness[0].epss, Freshness::Stale);
    QCOMPARE(a.priority, Priority::InsufficientCurrentExploitEvidence); QVERIFY(!a.nextFreshnessExpiryUtc);
    QVERIFY(hasReason(a, Reason::StaleKevListedContextOnly)); QVERIFY(hasReason(a, Reason::StaleEpssContextOnly));
}
void Phase10Test::naturalExpiry()
{
    for (bool kev : {false, true}) {
        auto p = profile(); if (kev) p.kev[0].status = KevStatus::Listed;
        const auto before = evaluate(p), after = evaluate(p, fixedTime().addSecs(25 * 3600));
        QCOMPARE(before.priority, kev ? Priority::KnownExploited : Priority::AboveResearchPercentileThreshold);
        QCOMPARE(after.priority, Priority::InsufficientCurrentExploitEvidence); QCOMPARE(after.support, Support::Insufficient);
        QVERIFY(hasReason(after, Reason::StaleEpssContextOnly)); if (kev) QVERIFY(hasReason(after, Reason::StaleKevListedContextOnly));
    }
}
void Phase10Test::corroborationExpiry()
{
    auto p = profile(); p.epss[0].fetchedAt = fixedTime().addSecs(3600);
    auto a = evaluate(p); QCOMPARE(a.support, Support::Complete); QCOMPARE(*a.nextFreshnessExpiryUtc, fixedTime().addMSecs(Day));
    a = evaluate(p, fixedTime().addMSecs(Day)); QCOMPARE(a.priority, Priority::AboveResearchPercentileThreshold); QCOMPARE(a.support, Support::Partial);
    QCOMPARE(*a.nextFreshnessExpiryUtc, fixedTime().addMSecs(Day).addSecs(3600));
    a = evaluate(p, *a.nextFreshnessExpiryUtc); QCOMPARE(a.support, Support::Insufficient); QVERIFY(!a.nextFreshnessExpiryUtc);
}
void Phase10Test::multiCveDeterminism()
{
    auto p = profile(); p.epss.append(epssValue(B)); p.kev = {kevValue(B, KevStatus::Listed), kevValue(A, KevStatus::Listed)};
    auto a = evaluate(p); QCOMPARE(a.driverCve, A); QCOMPARE(a.priority, Priority::KnownExploited);
    std::reverse(p.epss.begin(), p.epss.end()); std::reverse(p.kev.begin(), p.kev.end());
    auto b = evaluate(p); QCOMPARE(b.driverCve, a.driverCve); QCOMPARE(b.reasons, a.reasons); QCOMPARE(b.support, a.support);
    for (auto& k : p.kev) k.status = KevStatus::NotListed;
    a = evaluate(p); QCOMPARE(a.driverCve, A);
    std::reverse(p.epss.begin(), p.epss.end()); std::reverse(p.kev.begin(), p.kev.end());
    b = evaluate(p); QCOMPARE(b.driverCve, a.driverCve); QCOMPARE(b.reasons, a.reasons); QCOMPARE(b.support, a.support);
    for (auto& e : p.epss) if (e.cve == B) e.percentile = .95;
    QCOMPARE(evaluate(p).driverCve, B);
    for (auto& e : p.epss) if (e.cve == B) { e.percentile = .99; e.fetchedAt = fixedTime().addDays(-2); }
    QCOMPARE(evaluate(p).driverCve, A);
    for (auto& e : p.epss) if (e.cve == B) { e.percentile = std::nextafter(.91, 1.0); e.fetchedAt = fixedTime(); }
    QCOMPARE(evaluate(p).driverCve, B); // Exact comparison, no epsilon tie.
}
void Phase10Test::supportCoverage()
{
    for (int change = 0; change < 9; ++change) {
        auto p = profile(); p.epss.append(epssValue(B, .5)); p.kev.append(kevValue(B));
        QCOMPARE(evaluate(p).support, Support::Complete);
        switch (change) {
        case 0: p.kev[1].fetchedAt = fixedTime().addDays(-2); break;
        case 1: p.kev[1].status = KevStatus::Unknown; break;
        case 2: p.kev[1].fetchedAt = fixedTime().addDays(2); break;
        case 3: p.epss[1].status = EpssStatus::NotScored; break;
        case 4: p.epss[1].status = EpssStatus::Failed; break;
        case 5: p.epss[1].status = EpssStatus::InvalidResponse; break;
        case 6: p.epss[1].fetchedAt = fixedTime().addDays(-2); break;
        case 7: p.epss[1].fetchedAt = fixedTime().addDays(2); break;
        case 8: p.epss.removeLast(); break;
        }
        auto a = evaluate(p); QCOMPARE(a.priority, Priority::AboveResearchPercentileThreshold); QCOMPARE(a.support, Support::Partial);
        p.kev[0].status = KevStatus::Listed; a = evaluate(p); QCOMPARE(a.priority, Priority::KnownExploited); QCOMPARE(a.support, Support::Complete);
    }
}
void Phase10Test::provenance()
{
    auto p = profile(); auto a = evaluate(p);
    QCOMPARE(a.key, p.key); QCOMPARE(a.profileGeneratedAt, p.generatedAt); QCOMPARE(a.driverCve, A);
    QCOMPARE(a.driverEpssApiVersion, p.epss[0].providerVersion); QCOMPARE(a.driverEpssScoreDate, p.epss[0].scoreDate);
    QCOMPARE(a.driverEpssFetchedAt, p.epss[0].fetchedAt); QCOMPARE(a.driverAcquisition, EvidenceAcquisition::Cache);
    QVERIFY(priorityExplanation(a).contains("FIRST API Version: 1.0")); QVERIFY(!priorityExplanation(a).contains("EPSS Model Version"));
    p.kev[0].status = KevStatus::Listed; a = evaluate(p);
    QCOMPARE(a.kevCatalogVersion, p.kev[0].catalogVersion); QCOMPARE(a.kevDateReleased, p.kev[0].dateReleased);
    QCOMPARE(a.kevFetchedAt, p.kev[0].fetchedAt); QCOMPARE(a.driverFreshness, Freshness::Fresh);
    const auto localTime = fixedTime().toOffsetFromUtc(8 * 3600);
    QCOMPARE(evaluate(p, localTime).evaluatedAt.timeSpec(), Qt::UTC);
}
void Phase10Test::reasonsAndExplanation()
{
    auto p = profile(); p.epss.append(epssValue(B, .99, fixedTime().addDays(-2)));
    p.kev.append(kevValue(B, KevStatus::Listed, fixedTime().addDays(-2)));
    const auto a = evaluate(p); auto reversed = p;
    std::reverse(reversed.epss.begin(), reversed.epss.end()); std::reverse(reversed.kev.begin(), reversed.kev.end());
    const auto b = evaluate(reversed); QCOMPARE(a.reasons, b.reasons); QCOMPARE(priorityExplanation(a), priorityExplanation(b));
    const QList<Reason> expected{Reason::EpssDriverSelected, Reason::FreshEpssAboveThreshold, Reason::DecisionEvidencePartial,
        Reason::KevCoverageIncompleteOrStale, Reason::EpssAliasCoverageIncomplete, Reason::StaleKevListedContextOnly,
        Reason::StaleEpssContextOnly, Reason::SeverityContextOnly, Reason::DependencyContextOnly, Reason::QualityUnavailable};
    QCOMPARE(a.reasons.size(), expected.size()); for (int i = 0; i < expected.size(); ++i) QCOMPARE(a.reasons[i].code, expected[i]);
    auto reordered = p; QHash<QString, EpssEvidence> hash; hash.insert(B, p.epss[1]); hash.insert(A, p.epss[0]); reordered.epss = hash.values();
    QCOMPARE(evaluate(reordered).reasons, a.reasons);
    p.epss[0].percentile = .5; QVERIFY(priorityExplanation(evaluate(p)).contains(QStringLiteral("这不是 Safe、Low Risk 或 Not Exploitable 结论")));
}
void Phase10Test::contextIsolation()
{
    auto p = profile(); const auto original = evaluate(p);
    for (auto status : {SeverityStatus::Missing, SeverityStatus::SchemaConflict}) {
        p.severity.status = status; p.dependency.path = DependencyPathState::NoResolvedPath;
        p.dependency.referenceResolution = ReferenceResolutionCompleteness::Partial;
        const auto a = evaluate(p); QCOMPARE(a.priority, original.priority); QCOMPARE(a.support, original.support);
        QVERIFY(hasReason(a, Reason::QualityUnavailable));
    }
}
void Phase10Test::defensiveInputs()
{
    auto p = profile(); p.epss.append(p.epss[0]); QCOMPARE(evaluate(p).support, Support::Insufficient);
    for (double value : {-1.0, 1.1, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()}) {
        p = profile(); p.epss[0].percentile = value; QCOMPARE(evaluate(p).support, Support::Insufficient);
    }
    p = profile(); p.epss[0].acquisition = EvidenceAcquisition::None; QCOMPARE(evaluate(p).evidenceFreshness[0].epss, Freshness::NotApplicable);
    p = profile(); p.epss[0].cve = p.kev[0].cve = "invalid"; QCOMPARE(evaluate(p).support, Support::Insufficient);
}

void Phase10Test::timerExpiry()
{
    for (bool listed : {false, true}) {
        Context c; QVERIFY(c.open()); Network net;
        VulnerabilityController owner(c.path(), c.cache(), c.logger, nullptr, nullptr, &net); LOAD_OWNER(owner, c);
        const auto fetched = now().addMSecs(-Day + 1200); QVERIFY(c.seed(fetched, fetched, listed)); LOAD_PROFILE();
        QCOMPARE(risk.assessment()->priority, listed ? Priority::KnownExploited : Priority::AboveResearchPercentileThreshold);
        const auto generated = risk.profile()->generatedAt;
        const auto ep = EpssCache(c.riskCache()).filePath(A), kp = KevCache(c.riskCache()).filePath();
        const auto eb = read(ep), kb = read(kp); const auto et = QFileInfo(ep).lastModified(), kt = QFileInfo(kp).lastModified();
        QSignalSpy consent(&risk, &RiskEvidenceController::consentRequested);
        QTRY_COMPARE_WITH_TIMEOUT(risk.assessment()->support, Support::Insufficient, 5000);
        QVERIFY(risk.assessment()->evaluatedAt >= fetched.addMSecs(Day)); QCOMPARE(risk.profile()->generatedAt, generated);
        QCOMPARE(risk.profile()->epss[0].freshness, EvidenceFreshness::Fresh);
        QCOMPARE(read(ep), eb); QCOMPARE(read(kp), kb); QCOMPARE(QFileInfo(ep).lastModified(), et); QCOMPARE(QFileInfo(kp).lastModified(), kt);
        QVERIFY(net.replies.isEmpty()); QVERIFY(consent.isEmpty()); QVERIFY(!risk.assessment()->nextFreshnessExpiryUtc);
    }
}
void Phase10Test::timerCorroboration()
{
    Context c; QVERIFY(c.open()); Network net;
    VulnerabilityController owner(c.path(), c.cache(), c.logger, nullptr, nullptr, &net); LOAD_OWNER(owner, c);
    const auto epssAt = now(); const auto kevAt = now().addMSecs(-Day + 1200); QVERIFY(c.seed(epssAt, kevAt)); LOAD_PROFILE();
    QCOMPARE(risk.assessment()->support, Support::Complete);
    QTRY_COMPARE_WITH_TIMEOUT(risk.assessment()->support, Support::Partial, 5000);
    QCOMPARE(risk.assessment()->priority, Priority::AboveResearchPercentileThreshold);
    QCOMPARE(*risk.assessment()->nextFreshnessExpiryUtc, epssAt.addMSecs(Day)); QVERIFY(net.replies.isEmpty());
}
void Phase10Test::refreshOverlap()
{
    Context c; QVERIFY(c.open()); Network net; net.allow = true;
    VulnerabilityController owner(c.path(), c.cache(), c.logger, nullptr, nullptr, &net); LOAD_OWNER(owner, c);
    const auto fetched = now().addMSecs(-Day + 1500); QVERIFY(c.seed(fetched, fetched)); LOAD_PROFILE();
    const auto generated = risk.profile()->generatedAt;
    const auto eb = read(EpssCache(c.riskCache()).filePath(A)), kb = read(KevCache(c.riskCache()).filePath());
    connect(&risk, &RiskEvidenceController::consentRequested, &risk, [&] { risk.consent(true); });
    risk.load(EvidenceLoadMode::Refresh); QTRY_COMPARE(net.replies.size(), 2);
    QCOMPARE(risk.profile()->generatedAt, generated); QVERIFY(risk.assessment());
    QTRY_COMPARE_WITH_TIMEOUT(risk.assessment()->support, Support::Insufficient, 5000); QVERIFY(risk.busy());
    // Deliver only EPSS; a partially received operation must never install an assessment.
    QPointer<HeldReply> epssReply;
    for (auto reply : net.replies) if (reply && reply->url().host() == "api.first.org") epssReply = reply;
    QVERIFY(epssReply);
    epssReply->deliver(json({{"status", "OK"}, {"status-code", 200}, {"version", "1.0"}, {"total", 1}, {"offset", 0}, {"limit", 1}, {"data", QJsonArray{score()}}}));
    QTRY_VERIFY(epssReply.isNull()); QCOMPARE(risk.profile()->generatedAt, generated); QCOMPARE(risk.assessment()->support, Support::Insufficient);
    risk.cancel(); QTRY_VERIFY(!risk.busy()); QCOMPARE(risk.state(), EvidenceOperationState::Cancelled);
    QCOMPARE(risk.profile()->generatedAt, generated); QCOMPARE(risk.assessment()->support, Support::Insufficient);
    QCOMPARE(read(EpssCache(c.riskCache()).filePath(A)), eb); QCOMPARE(read(KevCache(c.riskCache()).filePath()), kb);
    QCOMPARE(net.replies.size(), 2);
}
void Phase10Test::publication()
{
    Context c; QVERIFY(c.open()); Network net;
    VulnerabilityController owner(c.path(), c.cache(), c.logger, nullptr, nullptr, &net); LOAD_OWNER(owner, c);
    bool mismatch = false;
    connect(&risk, &RiskEvidenceController::changed, &risk, [&] {
        if (risk.profile().has_value() != risk.assessment().has_value()) mismatch = true;
        if (risk.profile() && risk.assessment() && (risk.profile()->key != risk.assessment()->key
            || risk.profile()->generatedAt != risk.assessment()->profileGeneratedAt)) mismatch = true;
    });
    const auto fetched = now().addMSecs(-Day + 1500); QVERIFY(c.seed(fetched, fetched)); LOAD_PROFILE();
    const auto oldExpiry = *risk.assessment()->nextFreshnessExpiryUtc;
    QVERIFY(c.seed(now(), now(), true)); LOAD_PROFILE();
    const auto evaluated = risk.assessment()->evaluatedAt;
    QCOMPARE(risk.assessment()->priority, Priority::KnownExploited);
    QTRY_VERIFY_WITH_TIMEOUT(now() >= oldExpiry.addMSecs(200), 5000);
    QCOMPARE(risk.assessment()->evaluatedAt, evaluated); // Old timer was replaced, not merely its result hidden.
    QVERIFY(!mismatch); QVERIFY(net.replies.isEmpty());
}
void Phase10Test::timerInvalidation()
{
    for (int change = 0; change < 7; ++change) {
        Context c; QVERIFY(c.open()); Network net;
        VulnerabilityController owner(c.path(), c.cache(), c.logger, nullptr, nullptr, &net); LOAD_OWNER(owner, c);
        QVERIFY(c.seed(now(), now())); LOAD_PROFILE();
        auto* timer = risk.findChild<QTimer*>("assessmentFreshnessTimer"); QVERIFY(timer); QVERIFY(timer->isActive());
        switch (change) {
        case 0: risk.setRequest(owner.riskEvidenceRequest(1)); break;
        case 1: risk.invalidate(); break;
        case 2: owner.select(1); break;
        case 3: owner.setProject({}); break;
        case 4: owner.reload(); break;
        case 5: owner.query(QueryMode::CacheOnly); break;
        case 6: { SbomDocument empty; QVERIFY(c.components.replaceForProject(c.projectId, empty).ok()); owner.setProject(c.projectId); break; }
        }
        QVERIFY(!risk.assessment()); QVERIFY(!risk.profile()); QVERIFY(!timer->isActive()); QVERIFY(risk.assessmentText().isEmpty());
        QTRY_VERIFY(!owner.loading() && !owner.busy()); QVERIFY(net.replies.isEmpty());
    }
}
void Phase10Test::staleTimerGeneration()
{
    Context c; QVERIFY(c.open()); Network net;
    VulnerabilityController owner(c.path(), c.cache(), c.logger, nullptr, nullptr, &net); LOAD_OWNER(owner, c);
    QVERIFY(c.seed(now(), now())); LOAD_PROFILE();
    auto* timer = risk.findChild<QTimer*>("assessmentFreshnessTimer"); QVERIFY(timer);
    // Timeout queues the production callback with the old captured generation. Disconnecting does
    // not retract an already queued callback; the generation guard must reject it after invalidation.
    QVERIFY(QMetaObject::invokeMethod(timer, "timeout", Qt::DirectConnection));
    risk.setRequest(owner.riskEvidenceRequest(1));
    QSignalSpy changes(&risk, &RiskEvidenceController::changed);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QVERIFY(!risk.assessment()); QVERIFY(changes.isEmpty()); QVERIFY(!timer->isActive());
    LOAD_PROFILE(); QCOMPARE(risk.assessment()->key.osvId, QString("SYNTHETIC-P10-2"));
}
void Phase10Test::lateTimer()
{
    Context c; QVERIFY(c.open()); Network net;
    VulnerabilityController owner(c.path(), c.cache(), c.logger, nullptr, nullptr, &net); LOAD_OWNER(owner, c);
    const auto fetched = now().addMSecs(-Day + 1000); QVERIFY(c.seed(fetched, fetched)); LOAD_PROFILE();
    auto* timer = risk.findChild<QTimer*>("assessmentFreshnessTimer"); QVERIFY(timer); timer->stop();
    const auto planned = *risk.assessment()->nextFreshnessExpiryUtc;
    QTRY_VERIFY_WITH_TIMEOUT(now() >= planned.addMSecs(350), 5000);
    const auto actual = now(); QVERIFY(QMetaObject::invokeMethod(timer, "timeout", Qt::DirectConnection));
    QTRY_COMPARE(risk.assessment()->support, Support::Insufficient);
    QVERIFY(risk.assessment()->evaluatedAt >= actual); QVERIFY(risk.assessment()->evaluatedAt > planned); QVERIFY(net.replies.isEmpty());
}
void Phase10Test::destruction()
{
    Context c; QVERIFY(c.open()); Network net;
    auto owner = std::make_unique<VulnerabilityController>(c.path(), c.cache(), c.logger, nullptr, nullptr, &net);
    auto& current = *owner; LOAD_OWNER(current, c); QVERIFY(c.seed(now(), now())); LOAD_PROFILE();
    QPointer<QTimer> timer = risk.findChild<QTimer*>("assessmentFreshnessTimer"); QVERIFY(timer);
    QVERIFY(QMetaObject::invokeMethod(timer, "timeout", Qt::DirectConnection));
    owner.reset(); QVERIFY(timer.isNull()); QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall); QVERIFY(net.replies.isEmpty());
}
void Phase10Test::ui()
{
    Context c; QVERIFY(c.open()); Network net;
    VulnerabilityController owner(c.path(), c.cache(), c.logger, nullptr, nullptr, &net);
    VulnerabilityPage page(owner); page.resize(680, 420); page.show(); LOAD_OWNER(owner, c);
    auto* candidates = page.findChild<QTableView*>("candidateTable"); QVERIFY(candidates);
    candidates->setCurrentIndex(candidates->model()->index(0, 0));
    auto* tabs = page.findChild<QTabWidget*>("evidenceTabs"); tabs->setCurrentIndex(1);
    auto* text = page.findChild<QTextBrowser*>("riskPriorityAssessment"); QVERIFY(text);
    auto* riskViews = page.findChild<QTabWidget*>("riskViews"); QVERIFY(riskViews);
    QVERIFY(riskViews->currentWidget() != text);
    const int priorityIndex = riskViews->indexOf(text); QVERIFY(priorityIndex >= 0);
    riskViews->setCurrentIndex(priorityIndex);
    QCoreApplication::processEvents();
    QCOMPARE(riskViews->currentWidget(), static_cast<QWidget*>(text));
    QVERIFY(c.seed(now(), now())); page.findChild<QPushButton*>("riskCacheOnly")->click(); QTRY_VERIFY(!risk.busy()); QVERIFY(risk.assessment());
    const auto displayed = text->toPlainText(); QCOMPARE(displayed, priorityExplanation(*risk.assessment()));
    for (const auto& word : QStringList{"Exploit-Signal Priority Assessment", "Decision Evidence Support: Complete", "Rules: v1", "Percentile >= 0.90",
        "EPSS Probability:", "EPSS Percentile:", "FIRST API Version: 1.0", "Driver CVE: " + A, "Evaluated At:", "Fetched At:", "Score Date:", "Decision Path",
        "Technical severity context only", "Does not prove runtime reachability", "Quality: Unavailable"}) QVERIFY2(displayed.contains(word), qPrintable(word));
    for (const auto& forbidden : QStringList{"Risk Score:", "Risk Level:", "High Risk", "Medium Risk", "Assessment Confidence:", "EPSS Model Version"}) QVERIFY(!displayed.contains(forbidden));
    auto* raw = page.findChild<QTextBrowser*>("riskEvidence"); QVERIFY(raw->toPlainText().contains("CVSS:3.1/"));
    QVERIFY(raw->toPlainText().contains("Freshness below is at Profile Generation"));
    auto* scroll = page.findChild<QScrollArea*>(); QVERIFY(scroll); QCoreApplication::processEvents(); QVERIFY(scroll->verticalScrollBar()->maximum() > 0);
    QVERIFY(text->verticalScrollBar()->maximum() > 0);
    scroll->ensureWidgetVisible(text); QCoreApplication::processEvents();
    QVERIFY(scroll->verticalScrollBar()->value() > 0);
    const QRect textViewport(text->mapTo(scroll->viewport(), QPoint(0, 0)), text->size());
    QVERIFY(textViewport.intersects(scroll->viewport()->rect()));
    text->verticalScrollBar()->setValue(text->verticalScrollBar()->maximum());
    QVERIFY(page.grab().save(QDir(QCoreApplication::applicationDirPath()).filePath("phase10-small-ui.png")));
    text->verticalScrollBar()->setValue(0); page.resize(1200, 900); QCoreApplication::processEvents();
    QVERIFY(page.grab().save(QDir(QCoreApplication::applicationDirPath()).filePath("phase10-ui.png")));
    QVERIFY(c.seed(now(), now(), true)); LOAD_PROFILE();
    for (const auto& word : QStringList{"Known Exploited", "Driver: CISA KEV", "Catalog Version: synthetic-catalog", "Date Released:"}) QVERIFY(text->toPlainText().contains(word));
    QVERIFY(c.seed(now().addDays(-2), now().addDays(-2), true)); LOAD_PROFILE();
    QVERIFY(text->toPlainText().contains("Historical / Stale Evidence Available")); QVERIFY(text->toPlainText().contains("Insufficient Current Exploit Evidence"));
    // Defensive time warning cannot arrive through the Phase09 cache parser (it rejects future
    // fetchedAt). Exercise the exact same assessment renderer used by this widget with pure input.
    auto future = profile(); text->setPlainText(priorityExplanation(evaluate(future, fixedTime().addSecs(-1))));
    QVERIFY(text->toPlainText().contains("Evidence time invalid for current decision")); QVERIFY(text->toPlainText().contains("Ineligible"));
    owner.select(1); QVERIFY(text->toPlainText().isEmpty()); QVERIFY(net.replies.isEmpty());
}
void Phase10Test::scale()
{
    RiskEvidenceProfile p; p.generatedAt = fixedTime();
    for (int i = 0; i < 10000; ++i) { const auto cve = QString("CVE-2026-%1").arg(1000000 + i); p.epss.append(epssValue(cve, i == 9999 ? .99 : .5)); p.kev.append(kevValue(cve)); }
    QElapsedTimer elapsed; elapsed.start(); const auto a = evaluate(p); const auto millis = elapsed.elapsed();
    QCOMPARE(a.driverCve, QString("CVE-2026-1009999")); QCOMPARE(a.support, Support::Complete); QCOMPARE(a.evidenceFreshness.size(), 10000);
    QVERIFY2(millis < 1000, qPrintable(QString::number(millis))); // Generous responsiveness bound on the frozen workstation.
    qInfo() << "Synthetic 10000 CVE assessment ms=" << millis;
    const auto text = priorityExplanation(a); QVERIFY(text.size() < 263000); QVERIFY(text.contains("0.99"));
    std::reverse(p.epss.begin(), p.epss.end()); std::reverse(p.kev.begin(), p.kev.end()); QCOMPARE(evaluate(p).reasons, a.reasons);
}

void Phase10Test::manualFixtures()
{
    QDir out(QCoreApplication::applicationDirPath());
    const QString session = "phase10-manual-" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    for (int fixture = 0; fixture < 7; ++fixture) {
        const QString letter(QChar('A' + fixture)); const QString base = session + '/' + letter;
        AppPaths paths{out.filePath(base)}; QString error; QVERIFY(paths.initialize(error));
        AppDatabase db; AppLogger logger; QVERIFY(logger.open(paths.logFile())); QVERIFY(db.open(paths.databaseFile(), error));
        ProjectRepository projects(db, logger); ComponentRepository components(db); Project project;
        QVERIFY(projects.create("Phase10 synthetic " + letter, {}, project).ok());
        SbomDocument doc; doc.components = {{"target", "library", "Finding " + letter, "1.2.3", "pkg:npm/synthetic-phase10@1.2.3"}};
        QVERIFY(components.replaceForProject(project.id, doc).ok());
        const QJsonArray aliases = fixture == 3 ? QJsonArray{A, B} : fixture == 5 ? QJsonArray{} : QJsonArray{A};
        QCOMPARE(OsvCache(paths.osvCacheDirectory()).write({identity(), now(), {candidate("SYNTHETIC-P10-" + letter, aliases), candidate("SYNTHETIC-P10-SECOND-" + letter, aliases)}}), QueryError::None);
        const auto root = QDir(paths.root).filePath("cache/risk-evidence-v1");
        const auto fetched = fixture == 4 ? now().addDays(-2) : fixture == 6 ? now().addMSecs(-Day + 180000) : now();
        if (fixture != 5) {
            QCOMPARE(EpssCache(root).write({A, "1.0", fetched, score(A, fixture == 2 ? .50 : .91)}), QueryError::None);
            if (fixture == 3) QCOMPARE(EpssCache(root).write({B, "1.0", fetched, {}}), QueryError::None);
            const auto kev = RiskEvidence::parseKev(json(catalog(fixture == 0 || fixture == 4)), fetched); QVERIFY(kev);
            QCOMPARE(KevCache(root).write(*kev), QueryError::None);
        }
        const auto launcher = QStringLiteral("@echo off\r\nset \"PATH=%1;%2;%PATH%\"\r\n\"%~dp0SupplyChainRiskAssessment.exe\" --data-dir \"%~dp0%3\"\r\n")
            .arg(QDir::toNativeSeparators(QLibraryInfo::path(QLibraryInfo::BinariesPath)), QStringLiteral(PHASE10_COMPILER_BIN), QDir::toNativeSeparators(base));
        QVERIFY(write(out.filePath("phase10-" + letter + ".cmd"), launcher.toLocal8Bit()));
    }
    const auto prepare = QStringLiteral("@echo off\r\nset \"PATH=%1;%2;%PATH%\"\r\n\"%~dp0Phase10Test.exe\" manualFixtures -o \"%~dp0phase10-fixture-generation.txt,txt\"\r\n")
        .arg(QDir::toNativeSeparators(QLibraryInfo::path(QLibraryInfo::BinariesPath)), QStringLiteral(PHASE10_COMPILER_BIN));
    QVERIFY(write(out.filePath("phase10-prepare.cmd"), prepare.toLocal8Bit()));
    QVERIFY(write(out.filePath("phase10-manual-checklist.md"), QStringLiteral(
        "# Phase 10 手工验收\n\nNOT YET USER-VERIFIED\n\n"
        "所有数据为 synthetic，Cache Only 操作不会联网。准备器每次创建独立新目录，不覆盖旧验收数据库。\n"
        "先运行 phase10-prepare.cmd，尤其 G 必须刚生成后立即开始。再运行 phase10-A/B/C/D/E/F/G.cmd。\n"
        "项目 → 选择对应项目 → 漏洞匹配 → 选组件 → 仅本地缓存 → 选 Candidate → Risk Evidence → Cache Only → Priority。\n\n"
        "A：Known Exploited / Complete；CISA KEV driver、catalog/date/fetchedAt/acquisition。\n"
        "B：Above / Complete；probability 0.035、percentile 0.91、FIRST API Version 1.0、score date、Rules v1 0.90、Evaluated At。\n"
        "C：Below / Complete；percentile 0.50，明确不是 Safe、Low Risk 或 Not Exploitable。\n"
        "D：Above / Partial；第二 CVE NotScored，不解释为零。\n"
        "E：Insufficient / Insufficient；历史 KEV Listed 与 EPSS 为 context-only。\n"
        "F：无 CVE，Insufficient / Insufficient，无 driver。\n"
        "G：生成后约 3 分钟自然跨过生产 24h fetchedAt 边界；先 Above / Complete，随后本地变 Insufficient。"
        "无需点击刷新、不弹确认、不联网；Provider Snapshot 保留生成时 Fresh，Priority 显示当前 Stale。过期前未打开则重新运行准备器。\n\n"
        "每组有两个合法 Candidate，可验证 Candidate switch 清空 Assessment 并重新加载。"
        "验证重读组件、重启重新派生、小窗口内外滚动、Provider CVSS vector 及 context-only 文案。\n"
        "刷新/取消/竞态已有自动测试；不要将这些自动结果记录为人工 PASS。手工联网仅按原 EPSS consent 流程主动触发，"
        "真实服务不保证 synthetic CVE 的结果与 fixture 一致。future timestamp 因 Phase09 cache 校验不可通过正常缓存注入，使用自动纯 Core/UI renderer 覆盖。\n\n"
        "NOT YET USER-VERIFIED\n").toUtf8()));
}
QTEST_MAIN(Phase10Test)
#include "Phase10Test.moc"
