#include "PresentationText.h"
#include "RiskPresentationView.h"
#include "OverviewPage.h"
#include "ProjectPage.h"
#include "ProjectRepository.h"
#include "ComponentRepository.h"
#include "MainWindow.h"
#include "AppDatabase.h"
#include "AppLogger.h"
#include "SbomImportDialog.h"
#include "ValidationPage.h"
#include "VulnerabilityPage.h"
#include "VulnerabilityController.h"
#include "RiskEvidenceController.h"
#include "AppSettings.h"
#include "DependencyPage.h"
#include <QSettings>
#include <QStackedWidget>
#include <QStatusBar>
#include <QSplitter>
#include <QTest>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QLabel>
#include <QListWidget>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QPushButton>
#include <QFileDialog>
#include <QComboBox>
#include <QTableView>
#include <QHeaderView>
#include <QTabWidget>
#include <QScrollArea>
#include <QScrollBar>
#include <QScreen>
#include <QSqlQuery>
#include <QThreadPool>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QTextBlock>
#include <cstring>
#include <algorithm>

using namespace PresentationText;
namespace {
const QString A = "CVE-2026-100001", B = "CVE-2026-100002";
QDateTime time() { return QDateTime::fromString("2026-09-30T09:00:00.000Z", Qt::ISODateWithMs); }
RiskEvidenceProfile profile()
{
    RiskEvidenceProfile p;
    p.generatedAt = time(); p.key.osvId = "SYNTHETIC-P12";
    EpssEvidence e; e.cve = A; e.status = EpssStatus::Available;
    e.freshness = EvidenceFreshness::Fresh; e.acquisition = EvidenceAcquisition::Cache;
    e.fetchedAt = time(); e.probability = .035; e.percentile = .90;
    KevEvidence k; k.cve = A; k.status = KevStatus::NotListed;
    k.freshness = EvidenceFreshness::Fresh; k.acquisition = EvidenceAcquisition::Cache; k.fetchedAt = time();
    p.epss = {e}; p.kev = {k}; return p;
}
QString show(const RiskEvidenceProfile& p, const RiskPriorityAssessment& a)
{
    RiskPresentationView view; view.showResult(p, a); return view.toPlainText();
}
QString read(const QString& path)
{
    QFile f(path); return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()) : QString();
}
bool write(const QString& path, const QByteArray& bytes)
{
    QFile f(path); return f.open(QIODevice::WriteOnly) && f.write(bytes) == bytes.size();
}
struct Context {
    QTemporaryDir dir;
    AppDatabase db; AppLogger logger;
    ProjectRepository projects{db, logger}; ComponentRepository components{db};
    Project a, b;
    bool open() {
        QString error;
        return logger.open(dir.filePath("synthetic.log")) && db.open(dir.filePath("synthetic.db"), error)
            && projects.create("Synthetic A", "<b>literal description</b>", a).ok()
            && projects.create("Synthetic B", {}, b).ok();
    }
    QString cache() const { return dir.filePath("cache/osv-v1"); }
};
void select(ProjectPage& page, const QString& id)
{
    auto* list = page.findChild<QListWidget*>("projectList");
    if (id.isEmpty()) { list->setCurrentRow(-1); return; }
    for (int i = 0; i < list->count(); ++i)
        if (list->item(i)->data(Qt::UserRole).toString() == id) { list->setCurrentRow(i); return; }
    qFatal("Synthetic project not found");
}
QString projectText(const OverviewPage& page) { return page.findChild<QLabel*>("overviewProject")->text(); }
QString supplyText(const OverviewPage& page) { return page.findChild<QLabel*>("overviewSupplyChain")->text(); }

class FixtureReply final : public QNetworkReply {
public:
    FixtureReply(const QNetworkRequest& request, QByteArray bytes, QObject* parent)
        : QNetworkReply(parent), m_bytes(std::move(bytes)) {
        setRequest(request); setUrl(request.url()); open(ReadOnly | Unbuffered);
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, 200);
        // Queued completion models Qt async IO; no socket or live request exists.
        QMetaObject::invokeMethod(this, [this] { emit readyRead(); setFinished(true); emit finished(); }, Qt::QueuedConnection);
    }
    void abort() override { if (!isFinished()) { setFinished(true); emit finished(); } }
    qint64 bytesAvailable() const override { return m_bytes.size() - m_offset + QNetworkReply::bytesAvailable(); }
protected:
    qint64 readData(char* data, qint64 max) override {
        const auto n = qMin(max, qint64(m_bytes.size() - m_offset));
        if (n <= 0) return -1;
        std::memcpy(data, m_bytes.constData() + m_offset, size_t(n)); m_offset += n; return n;
    }
private:
    QByteArray m_bytes; qint64 m_offset = 0;
};
class FixtureNetwork final : public QNetworkAccessManager {
public:
    int requests = 0;
protected:
    QNetworkReply* createRequest(Operation, const QNetworkRequest& request, QIODevice*) override {
        ++requests;
        QByteArray bytes;
        if (request.url().host() == "api.osv.dev")
            bytes = R"({"vulns":[{"id":"SYNTHETIC-P12","modified":"2026-01-01T00:00:00Z","aliases":["CVE-2026-100001"],"affected":[{"package":{"ecosystem":"npm","name":"synthetic"},"versions":["1.0.0"]}]}]})";
        else if (request.url().host() == "api.first.org")
            bytes = R"({"status":"OK","status-code":200,"version":"1.0","total":1,"offset":0,"limit":100,"data":[{"cve":"CVE-2026-100001","epss":"0.035","percentile":"0.94","date":"2026-09-30"}]})";
        else if (request.url().host() == "raw.githubusercontent.com")
            bytes = R"({"catalogVersion":"synthetic","dateReleased":"2026-09-30T00:00:00Z","count":0,"vulnerabilities":[]})";
        else qFatal("Unexpected mock request");
        return new FixtureReply(request, bytes, this);
    }
};
}

class Phase12Test final : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() { QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs); }
    void vocabulary(); void exactThreshold(); void forbiddenSemantics(); void acquisition(); void kevNotQueryable();
    void freshnessDrift(); void statusFreshness(); void cveJoin(); void noCve(); void missingAssociation(); void multiCvePartial();
    void selectionOwnership(); void initialSynchronization(); void projectSwitch(); void createDelete(); void successfulApply();
    void overviewSummary(); void largeSummary(); void summaryError(); void overviewNavigation(); void settingsInfo();
    void validationVocabulary(); void uiLifetime(); void markupSafety(); void structuralContract(); void vulnerabilityPresentation();
    void workspaceRoutes(); void workspaceContext(); void workspaceRisk();
    void workspaceLayout(); void workspaceEmpty();
    void componentStates(); void componentLayout();
};

void Phase12Test::vocabulary()
{
    // Independent frozen Chinese vocabulary, including every enum value. Switch exhaustiveness
    // is also a compiler error in PresentationCore, so a new domain value cannot silently pass.
    const QStringList priorities{QStringLiteral("存在已知利用证据"), QStringLiteral("达到或高于研究百分位阈值"),
        QStringLiteral("低于研究百分位阈值"), QStringLiteral("当前利用证据不足")};
    const QStringList priorityEnglish{"Known Exploited", "Above Research Percentile Threshold", "Below Research Percentile Threshold", "Insufficient Current Exploit Evidence"};
    for (int i = 0; i < priorities.size(); ++i) {
        QVERIFY(priorityLabel(PriorityClass(i)).startsWith(priorities[i]));
        QVERIFY(priorityLabel(PriorityClass(i)).contains(priorityEnglish[i]));
    }
    const QStringList support{QStringLiteral("完整"), QStringLiteral("部分"), QStringLiteral("不足")};
    const QStringList supportEnglish{"Complete", "Partial", "Insufficient"};
    for (int i = 0; i < 3; ++i) {
        QVERIFY(supportLabel(DecisionEvidenceSupport(i)).contains(support[i]));
        QVERIFY(supportLabel(DecisionEvidenceSupport(i)).contains(supportEnglish[i]));
    }
    QCOMPARE(priorityDriverLabel(PriorityDriverKind::Kev), QString("CISA KEV"));
    QCOMPARE(priorityDriverLabel(PriorityDriverKind::Epss), QString("FIRST EPSS"));
    QVERIFY(priorityDriverLabel(PriorityDriverKind::None).contains(QStringLiteral("无当前利用信号驱动")));
    const QStringList acquisitions{QStringLiteral("实时获取 (Live)"), QStringLiteral("缓存获取 (Cache)"),
        QStringLiteral("在线刷新失败后使用缓存 (Stale Fallback)"), QStringLiteral("无获取来源 (None)")};
    for (int i = 0; i < 4; ++i) QCOMPARE(acquisitionLabel(EvidenceAcquisition(i)), acquisitions[i]);
    const QStringList kev{QStringLiteral("该 KEV 快照已收录 (Listed)"), QStringLiteral("该 KEV 快照未收录 (NotListed)"),
        QStringLiteral("KEV 状态未知 (Unknown)"), QStringLiteral("无可用于 KEV 目录匹配的合法 CVE (NotQueryable)")};
    for (int i = 0; i < 4; ++i) QCOMPARE(kevStatusLabel(KevStatus(i)), kev[i]);
    const QStringList epss{QStringLiteral("存在评分记录 (Available)"), QStringLiteral("该 EPSS 查询快照未返回评分记录 (NotScored)"),
        QStringLiteral("EPSS 查询失败 (Failed)"), QStringLiteral("EPSS 响应无效 (Invalid Response)"), QStringLiteral("无可用于 EPSS 查询的合法 CVE (NotQueryable)")};
    for (int i = 0; i < 5; ++i) QCOMPARE(epssStatusLabel(EpssStatus(i)), epss[i]);
    const QStringList fresh{QStringLiteral("当前决策证据有效 (Fresh)"), QStringLiteral("当前决策证据为 Stale 上下文 (Stale)"),
        QStringLiteral("当前决策新鲜度不适用 (Not Applicable)"), QStringLiteral("当前决策不可用 (Ineligible)")};
    for (int i = 0; i < 4; ++i) QCOMPARE(effectiveFreshnessLabel(EffectiveDecisionFreshness(i)), fresh[i]);
    for (int i = 0; i < 3; ++i) {
        const auto text = profileSnapshotFreshnessLabel(EvidenceFreshness(i));
        QVERIFY(text.startsWith(QStringLiteral("Profile 生成时新鲜度：")));
        QVERIFY(text.contains(QStringList{"Fresh", "Stale", "Not Applicable"}[i]));
        QVERIFY(!text.contains(QStringLiteral("当前有效")));
    }
    const auto snapshotStale = profileSnapshotFreshnessLabel(EvidenceFreshness::Stale);
    QVERIFY(snapshotStale.contains(QStringLiteral("当时未满足 Fresh 条件")));
    QVERIFY(!snapshotStale.contains(QStringLiteral("刷新窗口")));
    QVERIFY(!snapshotStale.contains(QStringLiteral("回退")));
    const QStringList severity{QStringLiteral("存在严重性证据 (Present)"), QStringLiteral("严重性类型暂不支持解释 (Unsupported Type)"),
        QStringLiteral("严重性证据结构无效 (Invalid Structure)"), QStringLiteral("严重性证据存在结构冲突 (Schema Conflict)"), QStringLiteral("未提供严重性证据 (Missing)")};
    for (int i = 0; i < 5; ++i) QCOMPARE(severityStatusLabel(SeverityStatus(i)), severity[i]);
    QVERIFY(qualityLabel(QualityEvidenceStatus::UnavailableForPersistedCurrentState).startsWith(QStringLiteral("当前持久化状态无完整质量证据 (Unavailable for Persisted Current State)")));
    const QStringList path{QStringLiteral("当前组件即依赖根组件 (Root Component Self)"), QStringLiteral("已观察到解析依赖路径 (Resolved Path Found)"),
        QStringLiteral("未观察到解析依赖路径 (No Resolved Path)"), QStringLiteral("依赖根组件缺失 (Root Missing)"),
        QStringLiteral("依赖根组件存在歧义 (Root Ambiguous)"), QStringLiteral("未捕获依赖信息 (Not Captured)")};
    for (int i = 0; i < 6; ++i) QCOMPARE(dependencyPathLabel(DependencyPathState(i)), path[i]);
    const QStringList roots{QStringLiteral("依赖根组件可用 (Root Available)"), path[3], path[4], path[5]};
    for (int i = 0; i < 4; ++i) QCOMPARE(dependencyRootLabel(DependencyRootStatus(i)), roots[i]);
    QCOMPARE(referenceResolutionLabel(ReferenceResolutionCompleteness::Complete), QStringLiteral("引用解析完整 (Complete)"));
    QCOMPARE(referenceResolutionLabel(ReferenceResolutionCompleteness::Partial), QStringLiteral("引用解析不完整 (Partial)"));
    for (int i = 0; i <= int(PriorityReasonCode::QualityUnavailable); ++i) {
        const auto code = PriorityReasonCode(i);
        const auto human = reasonText({code, A, PriorityDriverKind::Epss}, .90);
        QVERIFY(human.contains(A)); QVERIFY(human.size() > A.size() + 15);
        QVERIFY(!human.contains(priorityReasonCode(code)));
    }
}
void Phase12Test::exactThreshold()
{
    const auto p = profile(); const auto a = RiskPriorityEvaluator::evaluate(p, time());
    QCOMPARE(a.priority, PriorityClass::AboveResearchPercentileThreshold);
    const auto text = show(p, a);
    QVERIFY(text.contains(QStringLiteral("达到或高于"))); QVERIFY(!text.contains(QStringLiteral("严格高于")));
    QVERIFY(text.contains("0.90"));
    QCOMPARE(a.rulesVersion, ProductionRulesVersion); QCOMPARE(a.epssPercentileThreshold, ProductionEpssPercentileThreshold);
    QCOMPARE(PackageIdentity::supportedEcosystems(), QStringList({"PyPI", "npm"}));
}
void Phase12Test::forbiddenSemantics()
{
    auto p = profile(); p.epss[0].percentile = .5;
    auto a = RiskPriorityEvaluator::evaluate(p, time());
    auto text = show(p, a);
    for (const auto& required : QStringList{QStringLiteral("低于阈值不代表安全、低风险或不可利用"),
         QStringLiteral("未收录不证明从未被利用"), QStringLiteral("不是预测置信度"),
         QStringLiteral("百分位 0.93 不等于 93% 利用概率"), QStringLiteral("依赖路径不证明运行时可达")})
        QVERIFY2(text.contains(required), qPrintable(required));
    p.epss[0].status = EpssStatus::NotScored; p.epss[0].probability.reset(); p.epss[0].percentile.reset();
    a = RiskPriorityEvaluator::evaluate(p, time()); text = show(p, a);
    QVERIFY(text.contains("NotScored")); QVERIFY(text.contains(QStringLiteral("未提供 (Unavailable)")));
    QVERIFY(!text.contains(QStringLiteral("概率 (Probability)：0")));
    for (const auto& forbidden : QStringList{"Risk Score:", "Risk Level:", "Coverage =", "confidence =", "Below = Safe", "NotListed = Not Exploited"})
        QVERIFY(!text.contains(forbidden));
    // This is a global interpretation boundary, independent of the current label.
    for (const auto priority : {PriorityClass::KnownExploited, PriorityClass::AboveResearchPercentileThreshold,
                               PriorityClass::BelowResearchPercentileThreshold, PriorityClass::InsufficientCurrentExploitEvidence}) {
        auto presentation = a; presentation.priority = priority;
        const auto human = show(p, presentation);
        QVERIFY(human.contains(priorityLabel(priority)));
        QVERIFY(human.contains(QStringLiteral("仅表示未达到本次研究百分位阈值")));
        QCOMPARE(human.count("Below ≠ Safe / Low Risk / Not Exploitable"), 1);
    }
}
void Phase12Test::acquisition()
{
    QCOMPARE(acquisitionLabel(EvidenceAcquisition::StaleFallback), QStringLiteral("在线刷新失败后使用缓存 (Stale Fallback)"));
    auto p = profile();
    p.epss[0].fetchedAt = time().addSecs(-3600); p.kev[0].fetchedAt = time().addSecs(-7200);
    p.epss[0].acquisition = EvidenceAcquisition::StaleFallback;
    p.kev[0].acquisition = EvidenceAcquisition::StaleFallback;
    QVERIFY(p.epss[0].fetchedAt.secsTo(time()) < 24 * 3600);
    QVERIFY(p.kev[0].fetchedAt.secsTo(time()) < 24 * 3600);
    const auto a = RiskPriorityEvaluator::evaluate(p, time());
    QCOMPARE(a.evidenceFreshness.size(), 1);
    QCOMPARE(a.evidenceFreshness[0].epss, EffectiveDecisionFreshness::Stale);
    QCOMPARE(a.evidenceFreshness[0].kev, EffectiveDecisionFreshness::Stale);
    const auto text = show(p, a);
    QVERIFY(text.contains(QStringLiteral("当前决策证据为 Stale 上下文 (Stale)")));
    QVERIFY(!text.contains(QStringLiteral("已过刷新窗口")));
    QVERIFY(text.contains(QStringLiteral("可能因为超过刷新窗口，也可能因为在线刷新失败后使用缓存")));
    QVERIFY(text.contains(QStringLiteral("不作为 Fresh 决策驱动")));
    QVERIFY(text.contains(QStringLiteral("在线刷新失败后使用缓存"))); QVERIFY(!text.contains(QStringLiteral("过期缓存回退")));
    QVERIFY(text.contains(QStringLiteral("缓存不必然过期"))); QVERIFY(text.contains(QStringLiteral("实时获取不保证当前有效")));
}
void Phase12Test::kevNotQueryable()
{
    KevEvidence k; k.status = KevStatus::NotQueryable;
    const DecisionFreshnessLookup lookup(RiskPriorityAssessment{});
    const auto text = kevText(k, lookup);
    QVERIFY(text.contains(QStringLiteral("无可用于 KEV 目录匹配的合法 CVE")));
    QVERIFY(text.contains(QStringLiteral("本地匹配"))); QVERIFY(!text.contains(QStringLiteral("用于 KEV 查询")));
}
void Phase12Test::freshnessDrift()
{
    const auto p = profile(); const auto a = RiskPriorityEvaluator::evaluate(p, time().addDays(1));
    const auto text = show(p, a);
    QVERIFY(text.contains(QStringLiteral("当前决策证据为 Stale 上下文 (Stale)")));
    QVERIFY(!text.contains(QStringLiteral("当前决策证据有效 (Fresh)")));
    QVERIFY(!text.contains(QStringLiteral("Profile 生成时新鲜度")));
    QCOMPARE(p.epss[0].freshness, EvidenceFreshness::Fresh);
    QVERIFY(profileSnapshotFreshnessLabel(p.epss[0].freshness).contains("Fresh"));
}
void Phase12Test::statusFreshness()
{
    const auto p = profile(); const auto a = RiskPriorityEvaluator::evaluate(p, time().addDays(1));
    const auto text = kevText(p.kev[0], DecisionFreshnessLookup(a));
    QVERIFY(text.contains(QStringLiteral("该 KEV 快照未收录"))); QVERIFY(text.contains("Stale"));
    QVERIFY(!text.contains(QStringLiteral("当前 KEV 未收录")));

    auto timed = profile();
    timed.epss[0].fetchedAt = time().addSecs(-60).toOffsetFromUtc(8 * 3600);
    timed.kev[0].fetchedAt = time().addSecs(-120);
    const auto rendered = show(timed, RiskPriorityEvaluator::evaluate(timed, time()));
    const QString fetchedLabel = QStringLiteral("获取时间 / Fetched At (UTC)：");
    QVERIFY(rendered.contains(fetchedLabel + "2026-09-30T08:59:00.000Z"));
    QVERIFY(rendered.contains(fetchedLabel + "2026-09-30T08:58:00.000Z"));
    QCOMPARE(rendered.count(fetchedLabel), 2);
    QCOMPARE(timed.epss[0].fetchedAt.offsetFromUtc(), 8 * 3600);
    timed.epss[0].fetchedAt = {}; timed.kev[0].fetchedAt = {};
    const auto invalid = show(timed, RiskPriorityEvaluator::evaluate(timed, time()));
    const auto fetchedLines = invalid.split('\n').filter(fetchedLabel);
    QCOMPARE(fetchedLines, QStringList({fetchedLabel + QStringLiteral("不可用 (Unavailable)"),
                                      fetchedLabel + QStringLiteral("不可用 (Unavailable)")}));
}
void Phase12Test::cveJoin()
{
    auto p = profile(); auto e = p.epss[0]; e.cve = B; p.epss.prepend(e);
    auto k = p.kev[0]; k.cve = B; p.kev.prepend(k);
    RiskPriorityAssessment a; a.evidenceFreshness = {
        {A, EffectiveDecisionFreshness::Fresh, EffectiveDecisionFreshness::Stale},
        {B, EffectiveDecisionFreshness::Ineligible, EffectiveDecisionFreshness::NotApplicable}};
    const DecisionFreshnessLookup lookup(a);
    QVERIFY(epssText(p.epss[0], lookup).contains("Not Applicable"));
    QVERIFY(kevText(p.kev[0], lookup).contains("Ineligible"));
    QVERIFY(epssText(p.epss[1], lookup).contains("Stale"));
    QVERIFY(kevText(p.kev[1], lookup).contains("Fresh"));
    std::reverse(a.evidenceFreshness.begin(), a.evidenceFreshness.end());
    QCOMPARE(epssText(p.epss[0], lookup), epssText(p.epss[0], DecisionFreshnessLookup(a)));
    a.evidenceFreshness.append(a.evidenceFreshness[0]);
    QVERIFY(DecisionFreshnessLookup(a).label(B, PriorityDriverKind::Kev, false).contains(QStringLiteral("关联信息不可用")));
}
void Phase12Test::noCve()
{
    RiskEvidenceProfile p; EpssEvidence e; e.status = EpssStatus::NotQueryable;
    KevEvidence k; k.status = KevStatus::NotQueryable; p.epss = {e}; p.kev = {k};
    const auto a = RiskPriorityEvaluator::evaluate(p, time()); QVERIFY(a.evidenceFreshness.isEmpty());
    const auto text = show(p, a);
    QCOMPARE(text.count(QStringLiteral("无可关联项")), 2);
    QVERIFY(!text.contains(QStringLiteral("关联信息不可用"))); QVERIFY(text.contains("NotQueryable"));
    QVERIFY(text.contains(QStringLiteral("CISA KEV 使用公开完整目录")));
    QVERIFY(text.contains(QStringLiteral("应用获取该目录后，在本地按 CVE 匹配")));
    QVERIFY(text.contains(QStringLiteral("不会将每个 CVE 逐条发送给 CISA 查询")));
}
void Phase12Test::missingAssociation()
{
    const auto p = profile(); auto a = RiskPriorityEvaluator::evaluate(p, time()); a.evidenceFreshness.clear();
    const auto text = show(p, a);
    QCOMPARE(text.count(QStringLiteral("关联信息不可用")), 2);
    const auto row = epssText(p.epss[0], DecisionFreshnessLookup(a));
    QVERIFY(!row.contains("Fresh")); QVERIFY(!row.contains("Not Applicable"));
    QVERIFY(!row.contains(QStringLiteral("无可关联项")));
}
void Phase12Test::multiCvePartial()
{
    auto p = profile(); p.epss[0].percentile = .94;
    EpssEvidence e; e.cve = B; e.status = EpssStatus::Failed; p.epss.append(e);
    auto k = p.kev[0]; k.cve = B; p.kev.append(k);
    const auto a = RiskPriorityEvaluator::evaluate(p, time());
    QCOMPARE(a.support, DecisionEvidenceSupport::Partial);
    const auto text = show(p, a);
    QVERIFY(text.contains(A)); QVERIFY(text.contains(B)); QVERIFY(text.contains("0.94"));
    QVERIFY(text.contains(QStringLiteral("决策证据支持：部分"))); QVERIFY(text.contains(QStringLiteral("EPSS 查询失败 (Failed)")));
    QVERIFY(text.contains(QStringLiteral("其他 CVE 或提供方的佐证不完整")));
    QVERIFY(text.contains(QStringLiteral("不代表“判断只有一半可信”")));
    QCOMPARE(a.priority, PriorityClass::AboveResearchPercentileThreshold);
    QCOMPARE(text.count("Below ≠ Safe / Low Risk / Not Exploitable"), 1);
    QCOMPARE(text.count(QStringLiteral("CISA KEV 使用公开完整目录")), 1);
    // D-style missing score also keeps the global boundary in the human view.
    p.epss[0].percentile = .91; p.epss[1].status = EpssStatus::NotScored;
    const auto d = RiskPriorityEvaluator::evaluate(p, time());
    QCOMPARE(d.priority, PriorityClass::AboveResearchPercentileThreshold);
    QCOMPARE(d.support, DecisionEvidenceSupport::Partial);
    const auto dText = show(p, d);
    QVERIFY(dText.contains("NotScored"));
    QVERIFY(dText.contains(QStringLiteral("低于阈值不代表安全、低风险或不可利用")));
}
void Phase12Test::selectionOwnership()
{
    Context c; QVERIFY(c.open()); ProjectPage page(c.projects, c.components, c.logger, c.cache());
    OverviewPage overview(c.projects, c.components); overview.follow(page);
    QCOMPARE(page.currentProjectId(), QString());
    QVERIFY(projectText(overview).contains(QStringLiteral("当前未选择项目")));
    QVERIFY(!projectText(overview).contains(c.a.name)); QVERIFY(supplyText(overview).isEmpty());
}
void Phase12Test::initialSynchronization()
{
    Context c; QVERIFY(c.open()); ProjectPage page(c.projects, c.components, c.logger, c.cache());
    select(page, c.a.id);
    OverviewPage overview(c.projects, c.components); overview.follow(page);
    QVERIFY(projectText(overview).contains(c.a.name));
}
void Phase12Test::projectSwitch()
{
    Context c; QVERIFY(c.open()); ProjectPage page(c.projects, c.components, c.logger, c.cache());
    OverviewPage overview(c.projects, c.components); overview.follow(page);
    QSignalSpy changed(&page, &ProjectPage::currentProjectChanged);
    select(page, c.a.id); QVERIFY(projectText(overview).contains(c.a.name));
    select(page, c.b.id); QVERIFY(projectText(overview).contains(c.b.name));
    QVERIFY(!projectText(overview).contains(c.a.name));
    select(page, {}); QVERIFY(projectText(overview).contains(QStringLiteral("当前未选择项目")));
    QCOMPARE(changed.count(), 3);
}
void Phase12Test::createDelete()
{
    Context c; QVERIFY(c.open()); ProjectPage page(c.projects, c.components, c.logger, c.cache());
    OverviewPage overview(c.projects, c.components); overview.follow(page); page.show();
    page.findChild<QPushButton*>("createProject")->click();
    auto* dialog = page.findChild<QDialog*>("createProjectDialog"); QVERIFY(dialog);
    dialog->findChild<QLineEdit*>("newProjectName")->setText("Synthetic created");
    dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
    QVERIFY(!page.currentProjectId().isEmpty()); QVERIFY(projectText(overview).contains("Synthetic created"));
    page.findChild<QPushButton*>("removeProject")->click();
    auto* confirm = page.findChild<QMessageBox*>("removeProjectDialog"); QVERIFY(confirm);
    confirm->button(QMessageBox::Yes)->click();
    QVERIFY(page.currentProjectId().isEmpty()); QVERIFY(projectText(overview).contains(QStringLiteral("当前未选择项目")));
    select(page, c.a.id); QVERIFY(c.projects.remove(c.a.id).ok());
    page.findChild<QPushButton*>("refreshProjects")->click();
    QVERIFY(page.currentProjectId().isEmpty()); QVERIFY(!projectText(overview).contains(c.a.name));
}
void Phase12Test::successfulApply()
{
    Context c; QVERIFY(c.open()); ProjectPage page(c.projects, c.components, c.logger, c.cache());
    OverviewPage overview(c.projects, c.components); overview.follow(page); select(page, c.a.id); page.show();
    QSignalSpy changed(&page, &ProjectPage::currentProjectChanged), state(&page, &ProjectPage::currentProjectStateChanged);
    QVERIFY(supplyText(overview).contains(QStringLiteral("未捕获")));
    page.findChild<QPushButton*>("importSbom")->click();
    auto* dialog = page.findChild<SbomImportDialog*>(); QVERIFY(dialog);
    QPointer<QFileDialog> picker = dialog->findChild<QFileDialog*>(); QVERIFY(picker); picker->reject(); QTRY_VERIFY(picker.isNull());
    const auto path = c.dir.filePath("synthetic.json");
    QVERIFY(write(path, R"({"bomFormat":"CycloneDX","specVersion":"1.6","metadata":{"component":{"name":"root"}},"components":[{},{}]})"));
    QSignalSpy parsed(dialog, &SbomImportDialog::importFinished), applied(dialog, &SbomImportDialog::applyFinished);
    dialog->importFile(path); QTRY_COMPARE(parsed.size(), 1);
    QVERIFY(supplyText(overview).contains(QStringLiteral("普通组件数：0"))); QVERIFY(state.isEmpty());
    dialog->applyToProject(); QTRY_COMPARE(applied.size(), 1); QVERIFY(applied[0][0].toBool());
    QCOMPARE(state.size(), 1); QVERIFY(changed.isEmpty());
    QCOMPARE(page.currentProjectId(), c.a.id);
    QVERIFY(supplyText(overview).contains(QStringLiteral("普通组件数：2"))); QVERIFY(supplyText(overview).contains(QStringLiteral("已捕获")));
    QVERIFY(page.findChild<QLabel*>("currentComponentsSummary")->text().contains(QStringLiteral("普通组件：2")));
    QVERIFY(page.findChild<QLabel*>("currentComponentsSummary")->text().contains(QStringLiteral("元数据根组件：已提供")));
    QVERIFY(write(path, R"({"bomFormat":"CycloneDX","specVersion":"1.6","components":[]})"));
    dialog->importFile(path); QTRY_COMPARE(parsed.size(), 2); dialog->applyToProject(); QTRY_COMPARE(applied.size(), 2);
    QVERIFY(supplyText(overview).contains(QStringLiteral("普通组件数：0"))); QCOMPARE(state.size(), 2);
    QVERIFY(page.findChild<QLabel*>("currentComponentsSummary")->text().contains(QStringLiteral("普通组件：0")));
    QVERIFY(page.findChild<QLabel*>("currentComponentsSummary")->text().contains(QStringLiteral("元数据根组件：未提供")));
    dialog->reject();
}
void Phase12Test::overviewSummary()
{
    Context c; QVERIFY(c.open()); ProjectOverviewSummary summary;
    QVERIFY(c.components.readOverviewSummary(c.a.id, summary).ok());
    QCOMPARE(summary.ordinaryComponentCount, 0); QVERIFY(!summary.dependencyCaptured);
    SbomDocument d; d.metadata.component.emplace(); d.components.resize(3);
    QVERIFY(c.components.replaceForProject(c.a.id, d).ok());
    QVERIFY(c.components.readOverviewSummary(c.a.id, summary).ok());
    QCOMPARE(summary.ordinaryComponentCount, 3); QVERIFY(summary.dependencyCaptured);
    QVERIFY(c.components.readOverviewSummary(c.b.id, summary).ok());
    QCOMPARE(summary.ordinaryComponentCount, 0); QVERIFY(!summary.dependencyCaptured);
    d.components.clear(); QVERIFY(c.components.replaceForProject(c.a.id, d).ok());
    QVERIFY(c.components.readOverviewSummary(c.a.id, summary).ok()); QCOMPARE(summary.ordinaryComponentCount, 0); QVERIFY(summary.dependencyCaptured);
    QCOMPARE(c.components.readOverviewSummary("missing", summary).error, ComponentError::ProjectNotFound);
    QCOMPARE(c.components.readOverviewSummary("' OR 1=1 --", summary).error, ComponentError::ProjectNotFound);
}
void Phase12Test::largeSummary()
{
    Context c; QVERIFY(c.open()); SbomDocument d; d.metadata.component.emplace(); d.components.resize(100000);
    QVERIFY(c.components.replaceForProject(c.a.id, d).ok());
    ProjectOverviewSummary summary; QVERIFY(c.components.readOverviewSummary(c.a.id, summary).ok());
    QCOMPARE(summary.ordinaryComponentCount, 100000); QVERIFY(summary.dependencyCaptured);
    // The result is a scalar pair. StructuralContract checks the actual COUNT/EXISTS implementation.
}
void Phase12Test::summaryError()
{
    Context c; QVERIFY(c.open()); ProjectPage page(c.projects, c.components, c.logger, c.cache());
    select(page, c.a.id); OverviewPage overview(c.projects, c.components); overview.follow(page);
    { QSqlQuery q(c.db.connection()); QVERIFY(q.exec("DROP TABLE dependency_capture")); }
    overview.follow(page);
    QVERIFY(supplyText(overview).contains(QStringLiteral("读取失败"))); QVERIFY(!supplyText(overview).contains("0"));
    c.db.close(); overview.follow(page);
    QVERIFY(projectText(overview).contains(QStringLiteral("读取失败"))); QVERIFY(!supplyText(overview).contains("0"));
    overview.resize(490, 500); overview.show();
    QVERIFY(QTest::qWaitForWindowExposed(&overview));
    QVERIFY(overview.grab().save(QCoreApplication::applicationDirPath()+"/ui01-overview-error.png"));
}
void Phase12Test::overviewNavigation()
{
    Context c; QVERIFY(c.open()); auto* page = new ProjectPage(c.projects, c.components, c.logger, c.cache());
    auto* overview = new OverviewPage(c.projects, c.components);
    MainWindow window(page, nullptr, "overview", new QWidget, overview); overview->follow(*page);
    connect(overview, &OverviewPage::navigationRequested, &window, &MainWindow::selectPage);
    auto* controller = page->findChild<VulnerabilityController*>(); QVERIFY(controller);
    QSignalSpy consent(controller, &VulnerabilityController::consentRequested);
    QSignalSpy riskConsent(&controller->riskEvidence(), &RiskEvidenceController::consentRequested);
    window.show();
    for (const auto* name : {"overviewManage", "overviewAnalyze", "overviewValidation"}) {
        QVERIFY(window.selectPage("overview")); overview->findChild<QPushButton*>(name)->click();
        QCOMPARE(window.currentPageId(), QString(name == QByteArray("overviewValidation") ? "validation" : "projects"));
    }
    QVERIFY(consent.isEmpty()); QVERIFY(riskConsent.isEmpty()); QVERIFY(!controller->snapshot());
    QCOMPARE(controller->state(), QueryState::NotStarted); QVERIFY(!controller->riskEvidence().profile());
}
void Phase12Test::settingsInfo()
{
    MainWindow window(new QWidget, nullptr, "settings");
    window.resize(680,420); window.show(); QVERIFY(QTest::qWaitForWindowExposed(&window));
    QCOMPARE(window.size(), QSize(680,420));
    auto* settings = window.findChild<QWidget*>("settings"); QVERIFY(settings);
    QString text; for (const auto* label : settings->findChildren<QLabel*>()) text += label->text();
    for (const auto& expected : QStringList{QStringLiteral(APPLICATION_VERSION), "Database Schema Version：4", "Rules Version：v1", "Production EPSS Percentile Threshold：0.90"})
        QVERIFY2(text.contains(expected), qPrintable(expected));
    QVERIFY(text.contains(QStringLiteral("页面偏好"))); QVERIFY(text.contains(QStringLiteral("应用信息（只读）")));
    QVERIFY(settings->findChildren<QLineEdit*>().isEmpty()); QVERIFY(settings->findChildren<QComboBox*>().isEmpty());
    QCOMPARE(AppDatabase::SchemaVersion, 4);
}
void Phase12Test::validationVocabulary()
{
    QString error; const auto d = Validation::loadRuntimeDirectory(":/validation", error); QVERIFY2(d, qPrintable(error));
    const auto experiment = Validation::runExperiment(*d); QCOMPARE(d->samples.size(), 40);
    const auto distributions = experiment.result["distributions"].toObject();
    const QList<QList<int>> expected{{6,22,6,6}, {6,14,14,6}, {6,6,22,6}};
    int t = 0;
    for (const auto& threshold : QStringList{"0.85", "0.90", "0.95"}) {
        const auto counts = distributions[threshold].toObject()["priority"].toObject();
        for (int i = 0; i < 4; ++i) QCOMPARE(counts[priorityClassText(PriorityClass(i))].toInt(), expected[t][i]);
        ++t;
    }
    QCOMPARE(experiment.result["sensitiveSamples"].toArray().size(), 16);
    QCOMPARE(experiment.result["syntheticConformance"].toObject()["caseCount"].toInt(), 54);
    QCOMPARE(experiment.result["syntheticConformance"].toObject()["failed"].toInt(), 0);
    QVERIFY(experiment.result["automatedPass"].toBool());
    ValidationPage page; page.show(); auto* samples = page.findChild<QComboBox*>("validationSamples");
    QTRY_VERIFY(samples->isEnabled());
    auto* view = page.findChild<RiskPresentationView*>("validationPresentation"); QVERIFY(view);
    auto* threshold = page.findChild<QComboBox*>("validationThreshold");
    for (int i = 0; i < 3; ++i) {
        threshold->setCurrentIndex(i);
        const auto& a = i == 0 ? experiment.lower[0] : i == 1 ? experiment.baseline[0] : experiment.upper[0];
        QVERIFY(view->toPlainText().contains(priorityLabel(a.priority)));
        QVERIFY(view->toPlainText().contains(supportLabel(a.support)));
        QVERIFY(page.findChild<QTextBrowser*>("validationDetail")->toPlainText().contains(priorityExplanation(a)));
    }
}
void Phase12Test::uiLifetime()
{
    Context c; QVERIFY(c.open());
    auto* page = new ProjectPage(c.projects, c.components, c.logger, c.cache());
    auto* overview = new OverviewPage(c.projects, c.components); overview->follow(*page);
    {
        MainWindow window(page, nullptr, "overview", new ValidationPage, overview);
        window.resize(680,420); window.show(); QVERIFY(QTest::qWaitForWindowExposed(&window));
        QCOMPARE(window.size(), QSize(680,420));
        auto* scroll = overview->findChild<QScrollArea*>(); QVERIFY(scroll);
        QTRY_VERIFY(scroll->verticalScrollBar()->maximum() > 0);
        scroll->verticalScrollBar()->setValue(scroll->verticalScrollBar()->maximum());
        for (const auto& id : QStringList{"projects", "overview", "settings", "validation", "overview"}) QVERIFY(window.selectPage(id));
        QVERIFY(window.grab().save(QCoreApplication::applicationDirPath()+"/phase12-small-ui.png"));
        QVERIFY(window.selectPage("overview"));
        window.resize(1000,640);
        scroll->verticalScrollBar()->setValue(0);
        QCoreApplication::processEvents();
        QVERIFY(window.grab().save(QCoreApplication::applicationDirPath()+"/ui01-overview-default-empty.png"));

        Project longProject;
        const QString longName = QStringLiteral("合成 UI01 Project ") + QString(80, QChar(0x957f));
        const QString longDescription = QStringLiteral("Synthetic <b>literal</b> 描述 ").repeated(15).trimmed();
        QVERIFY(c.projects.create(longName, longDescription, longProject).ok());
        page->findChild<QPushButton*>("refreshProjects")->click();
        select(*page, longProject.id);
        QVERIFY(projectText(*overview).contains(longName));
        QVERIFY(projectText(*overview).contains(longDescription));
        qInfo("UI01 DPR=%.3f logicalDpi=%.1f screen=%dx%d", window.devicePixelRatioF(),
              window.screen()->logicalDotsPerInch(), window.screen()->size().width(), window.screen()->size().height());
        for (const QSize size : {QSize(1000,640), QSize(680,420)}) {
            window.resize(size); QCoreApplication::processEvents();
            QCOMPARE(window.size(), size);
            QTRY_COMPARE(scroll->horizontalScrollBar()->maximum(), 0);
            auto* text = overview->findChild<QLabel*>("overviewProject");
            QTRY_VERIFY(text->height() >= text->heightForWidth(text->width()));
            for (const auto* name : {"overviewManage", "overviewAnalyze", "overviewValidation"}) {
                auto* button = overview->findChild<QPushButton*>(name); QVERIFY(button);
                scroll->ensureWidgetVisible(button); QCoreApplication::processEvents();
                const QRect bounds(button->mapTo(scroll->viewport(), QPoint()), button->size());
                QVERIFY(scroll->viewport()->rect().contains(bounds));
                QVERIFY(button->width() >= button->sizeHint().width());
            }
            scroll->verticalScrollBar()->setValue(0); QCoreApplication::processEvents();
            QVERIFY(window.grab().save(QCoreApplication::applicationDirPath()
                + QString("/ui01-overview-long-%1.png").arg(size.width())));
        }
    }
    QVERIFY(QThreadPool::globalInstance()->waitForDone(30000));
    ProjectPage surviving(c.projects, c.components, c.logger, c.cache());
    auto* observer = new OverviewPage(c.projects, c.components); observer->follow(surviving); delete observer;
    select(surviving, c.a.id); // Context-bound connections must no longer call the deleted page.
}
void Phase12Test::markupSafety()
{
    auto p = profile(); p.severity.items.append({"<script>type</script>", "<b>vector</b>", "", ""});
    auto a = RiskPriorityEvaluator::evaluate(p, time()); a.driverCve = "<img src='https://example.invalid/x'>";
    a.driverEpssApiVersion = "<b>external</b>";
    RiskPresentationView view; view.showResult(p, a);
    QVERIFY(view.toPlainText().contains(a.driverCve)); QVERIFY(view.toPlainText().contains("<b>vector</b>"));
    QVERIFY(view.toPlainText().contains("<b>external</b>"));
    for (auto block = view.document()->begin(); block.isValid(); block = block.next())
        for (auto it = block.begin(); !it.atEnd(); ++it) QVERIFY(!it.fragment().charFormat().isImageFormat());
    Context c; QVERIFY(c.open()); ProjectPage page(c.projects, c.components, c.logger, c.cache());
    OverviewPage overview(c.projects, c.components); overview.follow(page); select(page, c.a.id);
    QCOMPARE(overview.findChild<QLabel*>("overviewProject")->textFormat(), Qt::PlainText);
    QVERIFY(projectText(overview).contains("<b>literal description</b>"));
}
void Phase12Test::structuralContract()
{
    const QString root = QStringLiteral(PHASE12_SOURCE_DIR) + '/';
    const auto repository = read(root+"src/ComponentRepository.cpp");
    const auto start = repository.indexOf("ComponentResult ComponentRepository::readOverviewSummary");
    QVERIFY(start >= 0); const auto end = repository.indexOf("\nComponentResult ", start+1);
    const auto summary = repository.mid(start, end-start);
    QVERIFY(summary.contains("COUNT(*)")); QVERIFY(summary.contains("EXISTS(")); QVERIFY(summary.contains("ComponentSourceRole::Component"));
    QVERIFY(summary.contains("FROM projects p WHERE p.id=?"));
    const auto overview = read(root+"src/OverviewPage.cpp");
    for (const auto& forbidden : QStringList{"listForProject(", "readSnapshot(", "DependencyGraph", "RiskPriorityEvaluator", "QNetwork", "QtConcurrent"}) {
        QVERIFY(!summary.contains(forbidden)); QVERIFY(!overview.contains(forbidden));
    }
    const auto presentation = read(root+"src/PresentationText.cpp");
    for (const auto& forbidden : QStringList{"RiskPriorityEvaluator::", "currentDateTime", "addMSecs", "addSecs", "QRegularExpression", "toUpper()", "toLower()", "sortedCveIds", "QSql", "QNetwork"})
        QVERIFY2(!presentation.contains(forbidden), qPrintable(forbidden));
    QVERIFY(presentation.contains("validCveId(cve)"));
    const auto cmake = read(root+"CMakeLists.txt");
    QVERIFY(cmake.contains("target_link_libraries(PresentationCore PUBLIC RiskEvidenceCore Qt6::Core)"));
    QVERIFY(cmake.contains("target_link_libraries(SupplyChainRiskAssessment PRIVATE ValidationWidgets PresentationWidgets)"));
    QVERIFY(cmake.contains("-Werror=switch-enum"));
}
void Phase12Test::vulnerabilityPresentation()
{
    Context c; QVERIFY(c.open()); SbomDocument d;
    d.components = {{"one", "library", "synthetic", "1.0.0", "pkg:npm/synthetic@1.0.0"}};
    QVERIFY(c.components.replaceForProject(c.a.id, d).ok());
    FixtureNetwork net; VulnerabilityController owner(c.db.filePath(), c.cache(), c.logger, nullptr, &net, &net);
    VulnerabilityPage page(owner); page.resize(680,420); page.show();
    page.setProject(c.a.id); owner.reload(); QTRY_VERIFY(!owner.loading()); owner.select(0);
    connect(&owner, &VulnerabilityController::consentRequested, &owner, [&] { owner.consent(true); });
    owner.query(QueryMode::Refresh); QTRY_VERIFY(!owner.busy()); QVERIFY(owner.applicability());
    QCOMPARE(net.requests, 1);
    page.findChild<QTableView*>("candidateTable")->setCurrentIndex(page.findChild<QTableView*>("candidateTable")->model()->index(0,0));
    auto& risk = owner.riskEvidence(); QVERIFY(risk.hasRequest());
    connect(&risk, &RiskEvidenceController::consentRequested, &risk, [&] { risk.consent(true); });
    risk.load(EvidenceLoadMode::Refresh); QTRY_VERIFY(!risk.busy()); QVERIFY(risk.profile()); QVERIFY(risk.assessment()); QCOMPARE(net.requests, 3);
    auto* view = page.findChild<RiskPresentationView*>(); QVERIFY(view);
    QVERIFY(view->toPlainText().contains(QStringLiteral("达到或高于")));
    QVERIFY(view->toPlainText().contains(A)); QVERIFY(view->toPlainText().contains("0.94"));
    auto* priority = page.findChild<QTextBrowser*>("riskPriorityAssessment"); QVERIFY(priority);
    auto* snapshot = page.findChild<QTextBrowser*>("riskEvidence"); QVERIFY(snapshot);
    auto* riskViews = page.findChild<QTabWidget*>("riskViews"); QVERIFY(riskViews);
    QCOMPARE(riskViews->count(), 3);
    QCOMPARE(riskViews->widget(0), static_cast<QWidget*>(view));
    QCOMPARE(riskViews->widget(1), static_cast<QWidget*>(priority));
    QCOMPARE(riskViews->widget(2), static_cast<QWidget*>(snapshot));
    QCOMPARE(riskViews->tabText(0), QStringLiteral("结果解释"));
    QCOMPARE(riskViews->tabText(1), QStringLiteral("技术详情 / Priority"));
    QCOMPARE(riskViews->tabText(2), QStringLiteral("技术详情 / Provider Snapshot"));
    QCOMPARE(riskViews->currentWidget(), static_cast<QWidget*>(view));
    QVERIFY(view->toPlainText().contains(QStringLiteral("CISA KEV 使用公开完整目录")));
    QVERIFY(view->toPlainText().contains(QStringLiteral("不会将每个 CVE 逐条发送给 CISA 查询")));
    QVERIFY(view->toPlainText().contains("Below ≠ Safe / Low Risk / Not Exploitable"));
    QVERIFY(!riskViews->widget(0)->findChild<QTextBrowser*>("riskPriorityAssessment"));
    QVERIFY(!riskViews->widget(0)->isAncestorOf(priority));
    QCOMPARE(priority->toPlainText(), priorityExplanation(*risk.assessment()));
    QString snapshotText;
    for (const auto& row : risk.profile()->epss)
        snapshotText += "EPSS " + row.cve + " · " + profileSnapshotFreshnessLabel(row.freshness) + '\n';
    for (const auto& row : risk.profile()->kev)
        snapshotText += "KEV " + row.cve + " · " + profileSnapshotFreshnessLabel(row.freshness) + '\n';
    snapshotText += risk.displayText();
    QCOMPARE(snapshot->toPlainText(), snapshotText);
    QVERIFY(!view->toPlainText().contains(QStringLiteral("Profile 生成时新鲜度：")));
    page.findChild<QTabWidget*>("evidenceTabs")->setCurrentIndex(1);
    riskViews->setCurrentWidget(priority); QCoreApplication::processEvents();
    QVERIFY(priority->isVisible()); QVERIFY(!view->isVisible());
    QCOMPARE(priority->toPlainText(), priorityExplanation(*risk.assessment()));
    riskViews->setCurrentWidget(snapshot); QCoreApplication::processEvents();
    QVERIFY(snapshot->isVisible()); QCOMPARE(snapshot->toPlainText(), snapshotText);
    riskViews->setCurrentWidget(view);
    auto* scroll = page.findChild<QScrollArea*>(); scroll->ensureWidgetVisible(view); QCoreApplication::processEvents();
    QVERIFY(page.grab().save(QCoreApplication::applicationDirPath()+"/phase12-result-ui.png"));
    owner.select(-1); QVERIFY(view->toPlainText().isEmpty()); QCOMPARE(net.requests, 3);
}

namespace {
void wireWorkspace(MainWindow& window, ProjectPage& page)
{
    QObject::connect(&window, &MainWindow::workspaceRequested, &page, &ProjectPage::showWorkspace);
    QObject::connect(&page, &ProjectPage::workspaceChanged, &window, [&window](const QString& route) {
        if (window.currentPageId() == "projects" && window.currentRouteId() != route) window.selectRoute(route);
    });
}
}
void Phase12Test::workspaceRoutes()
{
    Context c; QVERIFY(c.open()); FixtureNetwork net;
    auto* page = new ProjectPage(c.projects,c.components,c.logger,c.cache(),nullptr,&net);
    MainWindow window(page,nullptr,"overview",new QWidget); wireWorkspace(window,*page);
    window.show(); QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto* stack=window.findChild<QStackedWidget*>("pages"); QCOMPARE(stack->count(),4);
    const QStringList routes{"overview","projects","components","dependencies","vulnerabilities","risk","validation","settings"};
    auto* nav=window.findChild<QScrollArea*>("navigationScroll");
    QSignalSpy top(&window,&MainWindow::navigationChanged), changed(&window,&MainWindow::routeChanged);
    for(const auto size:{QSize(1000,640),QSize(680,420)}) {
        window.resize(size); QCoreApplication::processEvents(); QCOMPARE(window.size(),size);
        for(const auto& route:routes) {
            auto* b=window.findChild<QPushButton*>("nav_"+route); QVERIFY(b);
            nav->ensureWidgetVisible(b); QCoreApplication::processEvents();
            QVERIFY(nav->viewport()->rect().contains(b->mapTo(nav->viewport(),b->rect().center())));
            QTest::mouseClick(b,Qt::LeftButton); QCoreApplication::processEvents();
            QCOMPARE(window.currentRouteId(),route); QVERIFY(b->isChecked());
            const auto topId=routes.indexOf(route)>=1&&routes.indexOf(route)<=5?QString("projects"):route;
            QCOMPARE(window.currentPageId(),topId);
            if(topId=="projects") QCOMPARE(page->currentWorkspace(),route);
            QVERIFY(window.statusBar()->currentMessage().contains(b->text()));
            const auto n=changed.count(), t=top.count();
            b->setFocus(); QTest::keyClick(b,Qt::Key_Space);
            QCOMPARE(changed.count(),n); QCOMPARE(top.count(),t);
            QCOMPARE(page->currentProjectId(),QString());
        }
        QVERIFY(window.grab().save(QCoreApplication::applicationDirPath()+QString("/ui02a-navigation-%1.png").arg(size.width())));
    }
    QVERIFY(nav->verticalScrollBar()->maximum()>0);
    window.resize(1000,640); QCoreApplication::processEvents();
    window.selectRoute("settings");
    window.resize(680,420); QCoreApplication::processEvents();
    auto* settingsButton=window.findChild<QPushButton*>("nav_settings");
    QVERIFY(nav->viewport()->rect().contains(settingsButton->mapTo(nav->viewport(),settingsButton->rect().center())));
    window.selectRoute("overview");
    window.findChild<QPushButton*>("nav_overview")->setFocus();
    QSet<QString> reached;
    for(int i=0;i<80 && reached.size()<routes.size();++i) {
        auto* focus=window.focusWidget();
        if(focus && focus->objectName().startsWith("nav_")) {
            reached.insert(focus->objectName().mid(4));
            QVERIFY(nav->viewport()->rect().contains(focus->mapTo(nav->viewport(),focus->rect().center())));
        }
        // Exclusive Qt button groups use arrows within the group; Tab leaves the group.
        QTest::keyClick(window.focusWidget(),Qt::Key_Down); QCoreApplication::processEvents();
    }
    QCOMPARE(reached.size(),routes.size());
    QVERIFY(!window.selectPage("risk")); QVERIFY(!window.selectRoute("unknown"));
    window.selectRoute("risk"); QCOMPARE(window.currentPageId(),QString("projects"));
    QString error, saved;
    QVERIFY(AppSettings::writeLastNavigationPage(c.dir.filePath("route.ini"),window.currentPageId(),error));
    QVERIFY(AppSettings::readLastNavigationPage(c.dir.filePath("route.ini"),saved,error));
    QCOMPARE(saved,QString("projects"));
    QCOMPARE(QSettings(c.dir.filePath("route.ini"),QSettings::IniFormat).allKeys(),QStringList{"ui/lastNavigationPage"});
    MainWindow restored(new QWidget,nullptr,saved); QCOMPARE(restored.currentRouteId(),QString("projects"));
    MainWindow unknown(new QWidget,nullptr,"risk"); QCOMPARE(unknown.currentRouteId(),QString("overview"));
    QCOMPARE(net.requests,0);
}
void Phase12Test::workspaceContext()
{
    Context c; QVERIFY(c.open()); FixtureNetwork net; SbomDocument d;
    d.components={{"a","library","synthetic","1.0.0","pkg:npm/synthetic@1.0.0"}};
    QVERIFY(c.components.replaceForProject(c.a.id,d).ok());
    d.components.append({"b","library","other","2.0.0","pkg:npm/other@2.0.0"});
    QVERIFY(c.components.replaceForProject(c.b.id,d).ok());
    auto* page=new ProjectPage(c.projects,c.components,c.logger,c.cache(),nullptr,&net);
    auto* overview=new OverviewPage(c.projects,c.components);
    MainWindow window(page,nullptr,"projects",new QWidget,overview); wireWorkspace(window,*page); overview->follow(*page);
    connect(overview,&OverviewPage::navigationRequested,&window,&MainWindow::selectPage);
    window.show(); QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto* owner=page->findChild<VulnerabilityController*>(); auto* risk=&owner->riskEvidence();
    auto* dep=page->findChild<DependencyPage*>(); auto* view=page->findChild<VulnerabilityPage*>();
    auto* tabs=page->findChild<QStackedWidget*>("projectWorkspaceStack");
    auto* evidence=page->findChild<QTabWidget*>("evidenceTabs");
    auto* components=page->findChild<QTableView*>("currentComponents");
    auto* ownerParent=owner->parent(); auto* viewParent=view->parent(); auto* depParent=dep->parent();
    for(const auto& id:{c.a.id,c.b.id}) {
        select(*page,id);
        for(const auto& route:QStringList{"components","dependencies","vulnerabilities","risk","projects"}) {
            QVERIFY(window.selectRoute(route)); QCOMPARE(page->currentProjectId(),id);
            QCOMPARE(components->model()->rowCount(),id==c.a.id?1:2);
            QCOMPARE(page->findChildren<VulnerabilityController*>().size(),1);
            QCOMPARE(page->findChildren<RiskEvidenceController*>().size(),1);
            QCOMPARE(page->findChild<VulnerabilityController*>(),owner); QCOMPARE(&owner->riskEvidence(),risk);
            QCOMPARE(owner->parent(),ownerParent); QCOMPARE(view->parent(),viewParent); QCOMPARE(dep->parent(),depParent);
        }
        QTRY_VERIFY(owner->loaded()&&!owner->loading());
        for(const auto& row:owner->rows()) QCOMPARE(row.component.projectId,id);
        tabs->setCurrentWidget(page->findChild<QWidget*>("componentsWorkspace")); QCOMPARE(window.currentRouteId(),QString("components"));
        tabs->setCurrentWidget(page->findChild<QWidget*>("dependenciesWorkspace")); QCOMPARE(window.currentRouteId(),QString("dependencies"));
        tabs->setCurrentWidget(page->findChild<QWidget*>("vulnerabilityWorkspace")); QCOMPARE(window.currentRouteId(),QString("risk"));
        evidence->setCurrentIndex(0); QCOMPARE(window.currentRouteId(),QString("vulnerabilities"));
        evidence->setCurrentIndex(1); QCOMPARE(window.currentRouteId(),QString("risk"));
        QVERIFY(page->findChild<QLabel*>("riskStatus")->text().contains(QStringLiteral("请选择当前已确认")));
        QVERIFY(!page->captureFindingReport(0).context); QVERIFY(!page->findChild<QPushButton*>("exportFindingReport")->isEnabled());
        window.selectPage("overview"); overview->findChild<QPushButton*>("overviewAnalyze")->click();
        QCOMPARE(window.currentRouteId(),QString("projects")); QCOMPARE(tabs->currentWidget(),page->findChild<QWidget*>("managementWorkspace"));
    }
    window.selectRoute("risk");
    page->findChild<QPushButton*>("removeProject")->click();
    auto* confirm=page->findChild<QMessageBox*>("removeProjectDialog"); QVERIFY(confirm); confirm->button(QMessageBox::Yes)->click();
    QCOMPARE(page->currentProjectId(),QString()); QVERIFY(!risk->hasRequest()); QVERIFY(!owner->snapshot());
    QVERIFY(!page->captureFindingReport(0).context); QVERIFY(!page->findChild<QPushButton*>("importSbom")->isEnabled());
    QCOMPARE(net.requests,0); QVERIFY(QThreadPool::globalInstance()->waitForDone(10000));
}
void Phase12Test::workspaceRisk()
{
    Context c; QVERIFY(c.open()); FixtureNetwork net; SbomDocument d;
    d.components={{"a","library","synthetic","1.0.0","pkg:npm/synthetic@1.0.0"}};
    QVERIFY(c.components.replaceForProject(c.a.id,d).ok());
    auto* page=new ProjectPage(c.projects,c.components,c.logger,c.cache(),nullptr,&net);
    MainWindow window(page,nullptr,"projects",new QWidget); wireWorkspace(window,*page);
    window.show(); QVERIFY(QTest::qWaitForWindowExposed(&window)); select(*page,c.a.id);
    window.selectRoute("vulnerabilities"); auto* owner=page->findChild<VulnerabilityController*>();
    QTRY_VERIFY(owner->loaded()&&!owner->loading());
    page->findChild<QTableView*>("identityTable")->selectRow(0);
    connect(owner,&VulnerabilityController::consentRequested,owner,[owner]{owner->consent(true);});
    owner->query(QueryMode::Refresh);
    window.selectRoute("risk"); window.selectRoute("vulnerabilities");
    QTRY_VERIFY(!owner->busy()); QCOMPARE(net.requests,1); QVERIFY(owner->applicability());
    auto* candidates=page->findChild<QTableView*>("candidateTable"); candidates->selectRow(0);
    auto& risk=owner->riskEvidence(); QVERIFY(risk.hasRequest());
    connect(&risk,&RiskEvidenceController::consentRequested,&risk,[&risk]{risk.consent(true);});
    risk.load(EvidenceLoadMode::Refresh);
    window.selectRoute("risk"); window.selectRoute("components");
    QTRY_VERIFY(!risk.busy()); QCOMPARE(net.requests,3); QVERIFY(risk.profile()); QVERIFY(risk.assessment());
    const auto profileText=RiskEvidence::profileText(*risk.profile()); const auto assessment=risk.assessmentText();
    const auto generation=owner->generation(), riskGeneration=risk.generation();
    const auto captured=page->captureFindingReport(0); QVERIFY(captured.context);
    for(const auto& route:QStringList{"risk","vulnerabilities","dependencies","overview","risk","projects","risk"}) {
        window.selectRoute(route); QCoreApplication::processEvents();
        QCOMPARE(owner->generation(),generation); QCOMPARE(risk.generation(),riskGeneration);
        QCOMPARE(owner->selected(),qsizetype(0)); QCOMPARE(candidates->currentIndex().row(),0);
        QCOMPARE(RiskEvidence::profileText(*risk.profile()),profileText); QCOMPARE(risk.assessmentText(),assessment);
        const auto now=page->captureFindingReport(0); QVERIFY(now.context); QCOMPARE(now.context->osvId,captured.context->osvId);
        QCOMPARE(net.requests,3);
    }
    window.selectRoute("validation");
    window.resize(680,420); QCoreApplication::processEvents(); window.selectRoute("risk"); QCoreApplication::processEvents();
    auto* scroll=page->findChild<VulnerabilityPage*>()->findChild<QScrollArea*>();
    auto* load=page->findChild<QPushButton*>("riskLoad");
    QVERIFY(scroll->viewport()->rect().contains(load->mapTo(scroll->viewport(),load->rect().center())));
    QVERIFY(window.grab().save(QCoreApplication::applicationDirPath()+"/ui02a-risk-small.png"));
    select(*page,c.b.id); QVERIFY(!risk.hasRequest()); QVERIFY(!risk.profile()); QVERIFY(!risk.assessment());
    QVERIFY(!page->captureFindingReport(0).context); QVERIFY(!page->findChild<QPushButton*>("exportFindingReport")->isEnabled());
    QCOMPARE(net.requests,3); QVERIFY(QThreadPool::globalInstance()->waitForDone(10000));
}

void Phase12Test::workspaceLayout()
{
    Context c; QVERIFY(c.open()); FixtureNetwork net; Project longProject;
    const QString name = QStringLiteral("长项目名称<&>").repeated(10);
    const QString description = QStringLiteral("<b>必须按文字显示</b>") + QStringLiteral("\n长描述").repeated(80);
    QVERIFY(c.projects.create(name,description,longProject).ok());
    auto* page = new ProjectPage(c.projects,c.components,c.logger,c.cache(),nullptr,&net);
    MainWindow window(page,nullptr,"projects",new QWidget); wireWorkspace(window,*page);
    window.resize(1000,640); window.show(); QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(page->currentProjectId().isEmpty());
    window.selectRoute("components");
    QTest::mouseClick(page->findChild<QPushButton*>("switchProject"),Qt::LeftButton);
    QVERIFY(page->currentProjectId().isEmpty());
    QVERIFY(page->findChild<QPushButton*>("createProject")->hasFocus());
    select(*page,longProject.id);
    auto* list = page->findChild<QListWidget*>("projectList");
    auto* details = page->findChild<QTextBrowser*>("projectDetails");
    auto* split = page->findChild<QSplitter*>("projectManagementSplitter");
    auto* stack = page->findChild<QStackedWidget*>("projectWorkspaceStack");
    auto* context = page->findChild<QLineEdit*>("currentProjectContext");
    auto* dep = page->findChild<DependencyPage*>();
    auto* vuln = page->findChild<VulnerabilityPage*>();
    auto* owner = page->findChild<VulnerabilityController*>();
    QCOMPARE(stack->count(),4); QCOMPARE(split->count(),2);
    QCOMPARE(split->widget(0),list); QCOMPARE(split->widget(1),details);
    QVERIFY(details->toPlainText().contains(description)); QVERIFY(context->text().contains(name));
    context->setFocus(); QTest::keyClick(context,Qt::Key_A,Qt::ControlModifier);
    QVERIFY(context->selectedText().contains(name));
    QSignalSpy selection(page,&ProjectPage::currentProjectChanged);
    for (const auto size : {QSize(1000,640),QSize(680,420),QSize(1200,760),QSize(680,420)}) {
        window.selectRoute("settings"); window.resize(size); QCoreApplication::processEvents();
        window.selectRoute("projects"); QCoreApplication::processEvents(); QCOMPARE(window.size(),size);
        QCOMPARE(split->orientation(),page->contentsRect().width()-40 >= 700 ? Qt::Horizontal : Qt::Vertical);
        QVERIFY(list->isVisible()); QVERIFY(details->isVisible());
        QVERIFY(list->height() >= 60); QVERIFY(details->height() >= 60);
        QVERIFY(details->verticalScrollBar()->maximum()>0);
        split->setSizes({100,300}); QCoreApplication::processEvents();
        for (const auto& route : QStringList{"components","dependencies","vulnerabilities","risk"}) {
            window.selectRoute(route); QCoreApplication::processEvents();
            QVERIFY(!list->isVisible()); QVERIFY(context->isVisible());
            QCOMPARE(page->currentProjectId(),longProject.id);
            QCOMPARE(page->findChildren<QListWidget*>("projectList").size(),1);
            QCOMPARE(page->findChildren<DependencyPage*>().size(),1); QCOMPARE(page->findChild<DependencyPage*>(),dep);
            QCOMPARE(page->findChildren<VulnerabilityPage*>().size(),1); QCOMPARE(page->findChild<VulnerabilityPage*>(),vuln);
            QCOMPARE(page->findChildren<VulnerabilityController*>().size(),1); QCOMPARE(page->findChild<VulnerabilityController*>(),owner);
            QCOMPARE(owner->parent(),page); QCOMPARE(owner->riskEvidence().parent(),owner);
            for (const auto& button : {"switchProject","importSbom","refreshProjects"}) {
                auto* b = page->findChild<QPushButton*>(button); QVERIFY(b->isVisible());
                QVERIFY(page->rect().contains(b->mapTo(page,b->rect().center())));
            }
            QTest::mouseClick(page->findChild<QPushButton*>("switchProject"),Qt::LeftButton);
            QCOMPARE(window.currentRouteId(),QString("projects")); QVERIFY(list->hasFocus());
        }
    }
    QCOMPARE(selection.count(),0); QCOMPARE(net.requests,0);
    QVERIFY(QThreadPool::globalInstance()->waitForDone(10000));
}

void Phase12Test::workspaceEmpty()
{
    Context c; QVERIFY(c.open()); QVERIFY(c.projects.remove(c.a.id).ok()); QVERIFY(c.projects.remove(c.b.id).ok());
    FixtureNetwork net; auto* page = new ProjectPage(c.projects,c.components,c.logger,c.cache(),nullptr,&net);
    MainWindow window(page,nullptr,"projects",new QWidget); wireWorkspace(window,*page);
    window.resize(680,420); window.show(); QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto* stack = page->findChild<QStackedWidget*>("projectWorkspaceStack");
    auto* create = page->findChild<QPushButton*>("createProject");
    for (const auto& route : QStringList{"components","dependencies","vulnerabilities","risk"}) {
        window.selectRoute(route); QCoreApplication::processEvents();
        QCOMPARE(window.currentRouteId(),route); QCOMPARE(page->currentWorkspace(),route);
        QVERIFY(stack->isVisible()); QVERIFY(page->currentProjectId().isEmpty());
        QVERIFY(!page->findChild<QPushButton*>("importSbom")->isEnabled());
        QVERIFY(!page->findChild<DependencyPage*>()->isVisible());
        QVERIFY(!page->findChild<VulnerabilityPage*>()->isVisible());
        const auto hint = route=="components" ? "componentsWorkspaceHint" : route=="dependencies" ? "dependenciesWorkspaceHint" : "vulnerabilityWorkspaceHint";
        QVERIFY(page->findChild<QLabel*>(hint)->isVisible());
        QSignalSpy changes(page,&ProjectPage::workspaceChanged);
        QVERIFY(page->showWorkspace(route)); QCOMPARE(changes.count(),0);
        QVERIFY(!page->showWorkspace("invalid")); QCOMPARE(page->currentWorkspace(),route);
        QTest::mouseClick(page->findChild<QPushButton*>("switchProject"),Qt::LeftButton);
        QVERIFY(create->isVisible()); QVERIFY(create->hasFocus()); QCOMPARE(window.currentRouteId(),QString("projects"));
    }
    QTest::mouseClick(create,Qt::LeftButton);
    auto* dialog = page->findChild<QDialog*>("createProjectDialog"); QVERIFY(dialog);
    dialog->findChild<QLineEdit*>()->setText("Created from empty workspace");
    QTest::mouseClick(dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok),Qt::LeftButton);
    QVERIFY(!page->currentProjectId().isEmpty());
    window.selectRoute("components"); QVERIFY(page->findChild<QTableView*>("currentComponents")->isVisible());
    // A failed project read keeps the requested route and a reachable recovery action.
    c.db.close(); QTest::mouseClick(page->findChild<QPushButton*>("refreshProjects"),Qt::LeftButton);
    QCOMPARE(window.currentRouteId(),QString("components")); QVERIFY(stack->isVisible());
    QVERIFY(page->findChild<QLabel*>("projectError")->isVisible());
    QVERIFY(page->findChild<QLabel*>("componentsWorkspaceHint")->isVisible());
    QTest::mouseClick(page->findChild<QPushButton*>("switchProject"),Qt::LeftButton);
    QVERIFY(create->isVisible()); QCOMPARE(net.requests,0);
}

void Phase12Test::componentStates()
{
    Context c; QVERIFY(c.open()); FixtureNetwork net;
    auto* page = new ProjectPage(c.projects,c.components,c.logger,c.cache(),nullptr,&net);
    MainWindow window(page,nullptr,"projects",new QWidget); wireWorkspace(window,*page);
    window.show(); QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(window.selectRoute("components"));
    auto* table = page->findChild<QTableView*>("currentComponents");
    auto* summary = page->findChild<QLabel*>("currentComponentsSummary");
    auto* state = page->findChild<QLabel*>("currentComponentsState");
    auto* refresh = page->findChild<QPushButton*>("refreshProjects");
    auto* model = table->model();
    QVERIFY(page->findChild<QLabel*>("componentsWorkspaceHint")->isVisible());
    QCOMPARE(window.currentRouteId(),QString("components")); QVERIFY(page->currentProjectId().isEmpty());
    auto* title=page->findChild<QWidget*>("componentsWorkspace")->findChild<QLabel*>("pageTitle");
    auto* subtitle=page->findChild<QLabel*>("componentsSubtitle");
    QVERIFY(title->height()<=2*title->fontMetrics().lineSpacing());
    QVERIFY(subtitle->height()<=2*subtitle->fontMetrics().lineSpacing());
    QVERIFY(subtitle->y()-title->geometry().bottom()<=20);
    select(*page,c.a.id);
    QVERIFY(summary->text().contains(QStringLiteral("普通组件：0")));
    QVERIFY(summary->text().contains(QStringLiteral("元数据根组件：未提供")));
    QVERIFY(state->text().contains(QStringLiteral("没有已保存组件")));
    QVERIFY(!state->text().contains(QStringLiteral("从未导入")));
    SbomDocument d; d.metadata.component = SbomComponent{"root","application","<b>root %5</b>","1.0",""};
    QVERIFY(c.components.replaceForProject(c.a.id,d).ok()); refresh->click();
    QCOMPARE(table->model(),model); QCOMPARE(model->rowCount(),1);
    QVERIFY(summary->text().contains(QStringLiteral("普通组件：0")));
    QVERIFY(summary->text().contains(QStringLiteral("元数据根组件：已提供")));
    QCOMPARE(model->data(model->index(0,0)).toString(),QStringLiteral("元数据根组件"));
    QCOMPARE(model->data(model->index(0,1)).toString(),QString("<b>root %5</b>"));
    d.components = {{"z","library","first","1",""},{"a","library","second","2",""}};
    QVERIFY(c.components.replaceForProject(c.a.id,d).ok()); refresh->click();
    QVERIFY(summary->text().contains(QStringLiteral("普通组件：2"))); QCOMPARE(model->rowCount(),3);
    QCOMPARE(model->data(model->index(1,0)).toString(),QStringLiteral("组件 #1"));
    QCOMPARE(model->data(model->index(2,1)).toString(),QString("second"));
    d.metadata.component.reset();
    QVERIFY(c.components.replaceForProject(c.b.id,d).ok()); select(*page,c.b.id);
    QCOMPARE(model->rowCount(),2); QVERIFY(summary->text().contains(QStringLiteral("元数据根组件：未提供")));
    select(*page,c.a.id); QCOMPARE(model->rowCount(),3);
    // Isolated SQL schema fault makes only the component read fail; the selected
    // project still exists, so recovery actions must remain usable.
    { QSqlQuery q(c.db.connection()); QVERIFY(q.exec("ALTER TABLE components RENAME TO components_unavailable")); }
    refresh->click();
    QCOMPARE(model->rowCount(),0); QVERIFY(summary->text().contains(QStringLiteral("不可用")));
    QVERIFY(!summary->text().contains(QStringLiteral("普通组件：")));
    QVERIFY(page->findChild<QLabel*>("projectError")->isVisible());
    QVERIFY(page->findChild<QPushButton*>("importSbom")->isEnabled());
    QCOMPARE(window.currentRouteId(),QString("components"));
    { QSqlQuery q(c.db.connection()); QVERIFY(q.exec("ALTER TABLE components_unavailable RENAME TO components")); }
    refresh->click(); QCOMPARE(model->rowCount(),3); QVERIFY(!page->findChild<QLabel*>("projectError")->isVisible());
    QVERIFY(c.components.replaceForProject(c.a.id,{}).ok()); refresh->click();
    QCOMPARE(model->rowCount(),0); QVERIFY(summary->text().contains(QStringLiteral("普通组件：0")));
    QVERIFY(c.projects.remove(c.a.id).ok()); refresh->click();
    QCOMPARE(model->rowCount(),0); QVERIFY(summary->text().contains(QStringLiteral("不可用")));
    QCOMPARE(window.currentRouteId(),QString("components"));
    page->findChild<QPushButton*>("switchProject")->click(); QCOMPARE(window.currentRouteId(),QString("projects"));
    select(*page,c.b.id); window.selectRoute("components");
    c.db.close(); refresh->click();
    QVERIFY(summary->text().contains(QStringLiteral("不可用"))); QCOMPARE(model->rowCount(),0);
    QVERIFY(page->findChild<QLabel*>("projectError")->isVisible());
    QCOMPARE(net.requests,0);
}

void Phase12Test::componentLayout()
{
    Context c; QVERIFY(c.open()); FixtureNetwork net; SbomDocument d;
    const QString literal = QStringLiteral("<b>%5 & 组件</b>") + QString(800,u'长');
    d.metadata.component = SbomComponent{"root","application","root","1",""};
    for (int i=0;i<120;++i) d.components.append({QString("ref-%1").arg(i),"library",i==0?literal:QString("item-%1").arg(i),"1.0.0",QString(700,u'p')});
    QVERIFY(c.components.replaceForProject(c.a.id,d).ok());
    auto* page = new ProjectPage(c.projects,c.components,c.logger,c.cache(),nullptr,&net);
    MainWindow window(page,nullptr,"projects",new QWidget); wireWorkspace(window,*page);
    window.show(); QVERIFY(QTest::qWaitForWindowExposed(&window)); select(*page,c.a.id);
    auto* table = page->findChild<QTableView*>("currentComponents"); auto* model=table->model();
    auto* owner = page->findChild<VulnerabilityController*>(); auto* risk = &owner->riskEvidence();
    const auto generation=owner->generation(); QSignalSpy resets(model,&QAbstractItemModel::modelReset);
    QCOMPARE(model->columnCount(),6); QCOMPARE(table->editTriggers(),QAbstractItemView::NoEditTriggers);
    QCOMPARE(model->data(model->index(1,1)).toString(),literal.left(512)+QStringLiteral("…"));
    QVERIFY(!(model->flags(model->index(1,1)) & Qt::ItemIsEditable));
    for (const QSize size : {QSize(1000,640),QSize(680,420),QSize(1500,900),QSize(680,420)}) {
        window.selectRoute("settings"); window.resize(size); QTRY_COMPARE(window.size(),size);
        window.selectRoute("components"); QCoreApplication::processEvents();
        QCOMPARE(table->model(),model); QCOMPARE(resets.count(),0);
        QVERIFY(table->viewport()->height() >= 3*table->rowHeight(0));
        QVERIFY(table->viewport()->width() >= 200);
        QVERIFY(table->visibleRegion().contains(table->rect().center()));
        for (const char* name : {"switchProject","importSbom","refreshProjects"}) {
            auto* b=page->findChild<QPushButton*>(name); QVERIFY(b->isEnabled());
            QVERIFY(b->visibleRegion().contains(b->rect().center()));
            QVERIFY(window.rect().contains(b->mapTo(&window,b->rect().center())));
        }
        table->setFocus(); QTest::keyClick(table,Qt::Key_End,Qt::ControlModifier);
        QTRY_COMPARE(table->currentIndex().row(),120);
        table->scrollTo(model->index(120,5),QAbstractItemView::PositionAtBottom); QCoreApplication::processEvents();
        QVERIFY(table->viewport()->rect().intersects(table->visualRect(model->index(120,5))));
        QVERIFY(table->verticalScrollBar()->maximum()>0);
        if(size.width()==680) QVERIFY(table->horizontalScrollBar()->maximum()>0);
        qInfo() << "UI02B2 component geometry" << size << "viewport" << table->viewport()->size() << "rowHeight" << table->rowHeight(0) << "DPR" << window.devicePixelRatioF();
        QVERIFY(window.grab().save(QDir(QCoreApplication::applicationDirPath()).filePath(QString("ui02b2-components-%1x%2.png").arg(size.width()).arg(size.height()))));
    }
    table->horizontalHeader()->resizeSection(1,220); QCOMPARE(table->columnWidth(1),220);
    window.selectRoute("settings"); window.resize(1000,640); window.selectRoute("components");
    QCOMPARE(table->columnWidth(1),220); QCOMPARE(resets.count(),0); QCOMPARE(owner->generation(),generation);
    QCOMPARE(page->findChildren<VulnerabilityController*>().size(),1); QCOMPARE(owner->parent(),page);
    QCOMPARE(page->findChildren<RiskEvidenceController*>().size(),1); QCOMPARE(risk->parent(),owner); QCOMPARE(net.requests,0);
}

QTEST_MAIN(Phase12Test)
#include "Phase12Test.moc"
