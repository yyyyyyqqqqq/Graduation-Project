#include "ValidationExperiment.h"
#include "ValidationSelection.h"
#include "ValidationPage.h"
#include "MainWindow.h"
#include <QTest>
#include <QComboBox>
#include <QTextBrowser>
#include <QStackedWidget>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QThreadPool>
#include <QUrl>
#include <QElapsedTimer>
#include <algorithm>
#include <limits>

using namespace Validation;
namespace
{
QJsonObject record(QString id, QString name = "example", QJsonArray aliases = {},
                   QJsonArray versions = {"1.0.0"}, QString ecosystem = "npm")
{
    return {{"id", id},
            {"modified", "2026-01-01T00:00:00Z"},
            {"aliases", aliases},
            {"affected",
             QJsonArray{QJsonObject{{"package", QJsonObject{{"ecosystem", ecosystem}, {"name", name}}},
                                    {"versions", versions}}}}};
}
QByteArray bytes(QJsonObject o)
{
    return QJsonDocument(o).toJson(QJsonDocument::Indented);
}
struct Files
{
    QJsonObject dataset, pool, index;
    Files()
    {
        const QDir d(":/validation");
        dataset =
            QJsonDocument::fromJson(readFile(d.filePath("phase11-validation-dataset-v1.json"))).object();
        pool = QJsonDocument::fromJson(readFile(d.filePath("candidate-pool-manifest-v1.json"))).object();
        index = QJsonDocument::fromJson(readFile(d.filePath("candidate-selection-index-v1.json"))).object();
    }
    std::optional<Dataset> load(QString &error)
    {
        const auto p = bytes(pool);
        index["candidatePoolSHA256"] = sha256(p);
        const auto i = bytes(index);
        dataset["candidatePoolSHA256"] = sha256(p);
        dataset["selectionIndexSHA256"] = sha256(i);
        const auto d = bytes(dataset);
        return loadDataset(d, sha256(d).toLatin1(), p, sha256(p).toLatin1(), i, sha256(i).toLatin1(), error);
    }
    std::optional<Dataset> runtime(QString &error) const
    {
        const auto d = bytes(dataset);
        return loadRuntimeDataset(d, sha256(d).toLatin1(), error);
    }
    QString addFailedCandidate()
    {
        const QString cve = "CVE-2026-9999999";
        const auto u = discover("npm", {record("GHSA-failed-audit", "audit-example", {cve})}).units.first();
        const auto ordered = [](QJsonArray rows, QJsonObject extra) {
            rows.append(extra);
            QMap<QString, QJsonObject> sorted;
            for (const auto &v : rows)
                sorted.insert(v.toObject()["selectionHash"].toString(), v.toObject());
            QJsonArray out;
            for (const auto &row : sorted)
                out.append(row);
            return out;
        };
        auto row = unitJson(u);
        row["discoveryStratum"] = "";
        pool["candidates"] = ordered(pool["candidates"].toArray(), row);
        pool["candidateCount"] = pool["candidates"].toArray().size();
        row.remove("record");
        row.remove("discoveryStratum");
        row["eligible"] = false;
        row["selected"] = false;
        row["finalCaptureStatus"] = "Failed";
        row["finalPrimaryStratum"] = "";
        row["selectionReason"] = "FinalCaptureFailed";
        index["candidates"] = ordered(index["candidates"].toArray(), row);
        const QJsonArray failures{QJsonObject{{"provider", "EPSS"}, {"requested", QJsonArray{cve}},
                                              {"error", "Timeout"}, {"attempts", 3}}};
        auto capture = dataset["capture"].toObject();
        capture["failures"] = failures;
        dataset["capture"] = capture;
        index["captureFailures"] = failures;
        return u.selectionHash;
    }
    void profileMutation(const std::function<void(QJsonObject &)> &f)
    {
        auto rows = dataset["samples"].toArray();
        auto s = rows[0].toObject(), p = s["profile"].toObject();
        f(p);
        s["profile"] = p;
        rows[0] = s;
        dataset["samples"] = rows;
    }
};
} // namespace
class Phase11Test final : public QObject
{
    Q_OBJECT
  private slots:
    void sharedEvaluator();
    void synthetic();
    void explanation();
    void profileCodec();
    void contractFailures();
    void hashContract();
    void timeContract();
    void provenanceContract();
    void duplicateSamples();
    void coverageGate();
    void aliasShared();
    void aliasPrimary();
    void aliasTransitive();
    void aliasPackageScope();
    void aliasRelations();
    void aliasOrder();
    void exactVersions();
    void productionVersionValidity();
    void frozenIdentityCompatibility();
    void captureAttempts();
    void failureAuditConsistency();
    void failedCandidateMetadata();
    void successCandidateMetadata();
    void coverageN29();
    void coverageMissingS4();
    void coverageMissingS5();
    void runtimeLoader();
    void runtimeContract();
    void representativeDeterminism();
    void noAliasEnrichment();
    void duplicateAdvisory();
    void strataFailures();
    void orderingContract();
    void frozenDataset();
    void sensitivity();
    void replay();
    void exportArtifact();
    void ui();
    void uiRejected();
    void uiLifetime();
};
void Phase11Test::sharedEvaluator()
{
    for (const auto &c : syntheticCases())
        QVERIFY(RiskPriorityEvaluator::evaluate(c.profile, c.time) ==
                *RiskPriorityEvaluator::evaluate(c.profile, c.time, {.90}));
    const auto c = syntheticCases().first();
    for (double t :
         {0., 1., .91, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()})
        QVERIFY(!RiskPriorityEvaluator::evaluate(c.profile, c.time, {t}));
}
void Phase11Test::synthetic()
{
    const auto r = syntheticConformance();
    QVERIFY2(r["pass"].toBool(), qPrintable(QString::fromUtf8(bytes(r))));
    QVERIFY(r["caseCount"].toInt() >= 50);
}
void Phase11Test::explanation()
{
    for (const auto &c : syntheticCases())
    {
        const auto a = *RiskPriorityEvaluator::evaluate(c.profile, c.time, {c.threshold});
        const auto t = priorityExplanation(a);
        QVERIFY(t.contains(QString("Percentile >= %1").arg(c.threshold, 0, 'f', 2)));
        QVERIFY(t.contains("not 90% exploitation probability"));
        QVERIFY(t.contains("0.90"));
        QVERIFY(t.contains("Support describes"));
        QCOMPARE(t, priorityExplanation(a));
    }
}
void Phase11Test::profileCodec()
{
    QString error;
    for (const auto &c : syntheticCases())
    {
        const auto j = profileJson(c.profile);
        const auto p = readProfile(j, error);
        QVERIFY2(p.has_value(), qPrintable(error));
        QCOMPARE(profileJson(*p), j);
    }
    auto p = syntheticCases().first().profile;
    p.key.applicabilityGeneration = std::numeric_limits<quint64>::max();
    p.dependency = {
        true, DependencyRootStatus::RootAvailable,     DependencyPathState::ResolvedPathFound, 3, 7,
        11,   ReferenceResolutionCompleteness::Partial};
    p.severity.items.append({"CVSS_V3", "CVSS:3.1/AV:N", "NVD", "SYNTHETIC", SeverityScope::MatchingAffected,
                             SeverityProvenance::ExplicitSource, 2, SeverityStatus::Present});
    p.operation = EvidenceOperationState::Cancelled;
    p.epss[0].cacheError = QueryError::CacheIo;
    p.epss[0].freshness = EvidenceFreshness::Stale;
    const auto j = profileJson(p);
    const auto reconstructed = readProfile(j, error);
    QVERIFY2(reconstructed.has_value(), qPrintable(error));
    QCOMPARE(profileJson(*reconstructed), j);
    auto invalid = j;
    invalid["unknown"] = 1;
    QVERIFY(!readProfile(invalid, error));
}
void Phase11Test::contractFailures()
{
    QString error;
    for (int mode = 0; mode < 6; ++mode)
    {
        Files f;
        if (mode == 0)
            f.dataset["datasetSchemaVersion"] = 2;
        if (mode == 1)
            f.profileMutation([](auto &p) {
                auto rows = p["epss"].toArray();
                auto e = rows[0].toObject();
                e["percentile"] = 1.1;
                rows[0] = e;
                p["epss"] = rows;
            });
        if (mode == 2)
            f.profileMutation([](auto &p) { p["operation"] = 99; });
        if (mode == 3)
            f.profileMutation([](auto &p) { p["projectId"] = "private-project"; });
        if (mode == 4)
            f.profileMutation([](auto &p) { p["generatedAt"] = "invalid"; });
        if (mode == 5)
            f.profileMutation([](auto &p) {
                auto rows = p["kev"].toArray();
                auto k = rows[0].toObject();
                k["catalogVersion"] = "forged";
                rows[0] = k;
                p["kev"] = rows;
            });
        QVERIFY2(!f.load(error), qPrintable(QString::number(mode)));
        QVERIFY(!error.isEmpty());
    }
}
void Phase11Test::hashContract()
{
    const QDir d(":/validation");
    const auto data = readFile(d.filePath("phase11-validation-dataset-v1.json"));
    QString error;
    QVERIFY(!loadDataset(data + ' ', sha256(data).toLatin1(), {}, {}, {}, {}, error));
    QVERIFY(error.contains("hash"));
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QVERIFY(writeFrozen(tmp.filePath("f.json"), {{"public", true}}, error));
    QVERIFY(!writeFrozen(tmp.filePath("f.json"), {{"public", false}}, error));
}
void Phase11Test::timeContract()
{
    QString error;
    for (int mode = 0; mode < 4; ++mode)
    {
        Files f;
        const auto time =
            QDateTime::fromString(f.dataset["evaluationReferenceTimeUtc"].toString(), Qt::ISODateWithMs);
        if (mode == 0)
            f.dataset["evaluationReferenceTimeUtc"] = time.addDays(2).toString(Qt::ISODateWithMs);
        if (mode == 1)
            f.dataset["captureWindowStartUtc"] = time.addSecs(1).toString(Qt::ISODateWithMs);
        if (mode == 2)
            f.dataset["captureWindowStartUtc"] = time.addSecs(-21601).toString(Qt::ISODateWithMs);
        if (mode == 3)
            f.pool["frozenAtUtc"] = time.addSecs(1).toString(Qt::ISODateWithMs);
        QVERIFY(!f.load(error));
    }
}
void Phase11Test::provenanceContract()
{
    QString error;
    Files f;
    auto sources = f.pool["sources"].toArray();
    auto s = sources[0].toObject();
    s["sourceIdentity"] = "https://example.invalid/data";
    sources[0] = s;
    f.pool["sources"] = sources;
    QVERIFY(!f.load(error));
    Files other;
    auto rows = other.index["candidates"].toArray();
    auto row = rows[0].toObject();
    row["selectionReason"] = "Other";
    rows[0] = row;
    other.index["candidates"] = rows;
    QVERIFY(!other.load(error));
}
void Phase11Test::duplicateSamples()
{
    QString error;
    Files f;
    auto rows = f.dataset["samples"].toArray();
    rows.append(rows[0]);
    f.dataset["samples"] = rows;
    QVERIFY(!f.load(error));
    Files g;
    auto candidates = g.pool["candidates"].toArray();
    candidates.append(candidates[0]);
    g.pool["candidates"] = candidates;
    QVERIFY(!g.load(error));
}
void Phase11Test::coverageGate()
{
    QString error;
    Files f;
    f.dataset["samples"] = QJsonArray{};
    QVERIFY(!f.load(error));
    QVERIFY(error.contains("coverage"));
}
void Phase11Test::captureAttempts()
{
    Files base;
    const QJsonArray invalid{0, -1, 4, 1.5, "2", true, QJsonValue(), QJsonObject{}, QJsonArray{}};
    QString error;
    for (const auto &value : invalid)
        for (int provider = 0; provider < 2; ++provider)
        {
            auto f = base;
            auto capture = f.dataset["capture"].toObject();
            if (provider == 0)
            {
                auto kev = capture["kev"].toObject();
                kev["attempts"] = value;
                capture["kev"] = kev;
            }
            else
            {
                auto chunks = capture["epss"].toArray();
                auto chunk = chunks[0].toObject();
                chunk["attempts"] = value;
                chunks[0] = chunk;
                capture["epss"] = chunks;
            }
            f.dataset["capture"] = capture;
            QVERIFY(!f.runtime(error));
            QCOMPARE(error, QString("Invalid capture attempts"));
            // Exercise the full entry as well, without serializing two large audit files
            // for an error that must be rejected by the shared Dataset-local contract first.
            const auto d = bytes(f.dataset);
            QVERIFY(!loadDataset(d, sha256(d).toLatin1(), {}, {}, {}, {}, error));
            QCOMPARE(error, QString("Invalid capture attempts"));
        }
    auto failed = base;
    failed.addFailedCandidate();
    for (const auto &value : invalid)
    {
        auto f = failed;
        auto capture = f.dataset["capture"].toObject();
        auto failures = capture["failures"].toArray();
        auto failure = failures[0].toObject();
        failure["attempts"] = value;
        failures[0] = failure;
        capture["failures"] = failures;
        f.dataset["capture"] = capture;
        QVERIFY(!f.runtime(error));
        QCOMPARE(error, QString("Invalid capture attempts"));
    }
}
void Phase11Test::failureAuditConsistency()
{
    Files base;
    base.addFailedCandidate();
    QString error;
    QVERIFY2(base.load(error).has_value(), qPrintable(error));
    auto mismatch = base;
    mismatch.index["captureFailures"] = QJsonArray{};
    QVERIFY(!mismatch.load(error));
    QCOMPARE(error, QString("Capture failure audit mismatch"));
    for (int mode = 0; mode < 6; ++mode)
    {
        auto f = base;
        auto capture = f.dataset["capture"].toObject();
        auto failures = capture["failures"].toArray();
        auto failure = failures[0].toObject();
        if (mode == 0)
            failure["requested"] = capture["epss"].toArray()[0].toObject()["requested"];
        if (mode == 1)
        {
            failure["provider"] = "KEV";
            failure.remove("requested");
        }
        if (mode == 2)
            failure["error"] = "None";
        if (mode == 3)
            failure["requested"] = QJsonArray{};
        failures[0] = failure;
        if (mode == 4)
            failures.append(failure);
        if (mode == 5)
            failures = QJsonArray{}; // Missing provider result must have a final-failure audit.
        capture["failures"] = failures;
        f.dataset["capture"] = capture;
        f.index["captureFailures"] = failures;
        QVERIFY2(!f.load(error), qPrintable(QString::number(mode)));
        QVERIFY(!error.isEmpty());
    }
}
void Phase11Test::failedCandidateMetadata()
{
    Files base;
    const auto hash = base.addFailedCandidate();
    QString error;
    QVERIFY2(base.load(error).has_value(), qPrintable(error));
    const QList<QPair<QString, QJsonValue>> mutations{{"finalCaptureStatus", "Success"},
        {"finalPrimaryStratum", "S2"}, {"selected", true}, {"selectionReason", "QuotaExceeded"},
        {"eligible", true}, {"finalPrimaryStratum", QJsonValue()}};
    for (const auto &mutation : mutations)
    {
        auto f = base;
        auto rows = f.index["candidates"].toArray();
        for (int i = 0; i < rows.size(); ++i)
        {
            auto row = rows[i].toObject();
            if (row["selectionHash"] != hash)
                continue;
            row[mutation.first] = mutation.second;
            rows[i] = row;
        }
        f.index["candidates"] = rows;
        QVERIFY2(!f.load(error), qPrintable(mutation.first));
        QVERIFY(error.contains("metadata") || error.contains("eligibility"));
    }
}
void Phase11Test::successCandidateMetadata()
{
    Files base;
    QString error;
    for (const auto &mutation : QList<QPair<QString, QJsonValue>>{{"finalCaptureStatus", "Failed"},
             {"finalPrimaryStratum", "S0"}, {"selectionReason", "FinalCaptureFailed"}, {"eligible", false}})
    {
        auto f = base;
        auto rows = f.index["candidates"].toArray();
        auto row = rows[0].toObject();
        row[mutation.first] = mutation.second;
        rows[0] = row;
        f.index["candidates"] = rows;
        QVERIFY2(!f.load(error), qPrintable(mutation.first));
        QVERIFY(error.contains("Stratum") || error.contains("reason") || error.contains("eligibility"));
    }
}
namespace
{
void coverageMutation(const QString &omit)
{
    Files f;
    QJsonArray rows;
    for (const auto &v : f.dataset["samples"].toArray())
        if (omit.isEmpty() ? rows.size() < 29 : v.toObject()["primaryStratum"] != omit)
            rows.append(v);
    if (omit.isEmpty())
        QCOMPARE(rows.size(), 29);
    else
    {
        QVERIFY(rows.size() >= 30);
        for (const auto &v : rows)
            QVERIFY(v.toObject()["primaryStratum"] != omit);
    }
    f.dataset["samples"] = rows;
    QString error;
    QVERIFY(!f.runtime(error));
    QCOMPARE(error, QString("Dataset coverage gate failed"));
    QVERIFY(!f.load(error));
    QCOMPARE(error, QString("Dataset coverage gate failed"));
}
} // namespace
void Phase11Test::coverageN29() { coverageMutation({}); }
void Phase11Test::coverageMissingS4() { coverageMutation("S4"); }
void Phase11Test::coverageMissingS5() { coverageMutation("S5"); }
void Phase11Test::runtimeLoader()
{
    QString error;
    const auto full = loadDirectory(":/validation", error);
    QVERIFY2(full.has_value(), qPrintable(error));
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    // This directory deliberately has no Manifest/Index: it is the product's two-file contract.
    for (const auto &name : QStringList{"phase11-validation-dataset-v1.json",
                                       "phase11-validation-dataset-v1.json.sha256"})
        QVERIFY(QFile::copy(":/validation/" + name, directory.filePath(name)));
    const auto runtime = loadRuntimeDirectory(directory.path(), error);
    QVERIFY2(runtime.has_value(), qPrintable(error));
    QCOMPARE(runtime->samples.size(), 40);
    QCOMPARE(runExperiment(*runtime).result, runExperiment(*full).result);
    QVERIFY(!loadDirectory(directory.path(), error));
    ValidationPage page(nullptr, directory.path());
    QElapsedTimer elapsed;
    elapsed.start();
    page.show();
    auto *combo = page.findChild<QComboBox *>("validationSamples");
    QTRY_VERIFY_WITH_TIMEOUT(combo->isEnabled(), 30000);
    qInfo() << "ValidationPage formal load + experiment + UI ready elapsed (ms):" << elapsed.elapsed();
    QCOMPARE(combo->count(), 40);
}
void Phase11Test::runtimeContract()
{
    Files base;
    QString error;
    for (int mode = 0; mode < 14; ++mode)
    {
        auto f = base;
        if (mode == 0) f.dataset["datasetSchemaVersion"] = 2;
        if (mode == 1) f.dataset["datasetVersion"] = "other";
        if (mode == 2) f.dataset["candidatePoolVersion"] = "other";
        if (mode == 3) f.dataset["candidatePoolSHA256"] = "bad";
        if (mode == 4) f.dataset["selectionIndexSHA256"] = "bad";
        if (mode == 5) f.dataset["evaluationReferenceTimeUtc"] = "2000-01-01T00:00:00Z";
        if (mode == 6) f.profileMutation([](auto &p) { p["operation"] = 99; });
        if (mode == 7) f.profileMutation([](auto &p) { p["projectId"] = "synthetic-private-marker"; });
        if (mode == 8) f.profileMutation([](auto &p) { p["generatedAt"] = "2000-01-01T00:00:00Z"; });
        if (mode >= 9)
        {
            auto rows = f.dataset["samples"].toArray();
            auto row = rows[0].toObject();
            if (mode == 9) row["primaryStratum"] = "S0";
            if (mode == 10) row["sampleId"] = "incorrect";
            if (mode == 11)
            {
                const QJsonValue first = rows[0];
                rows[0] = rows[1];
                rows[1] = first;
            }
            else if (mode == 12) rows.append(rows[0]);
            else if (mode == 13)
            {
                auto profile = row["profile"].toObject();
                auto epss = profile["epss"].toArray();
                auto e = epss[0].toObject();
                e["percentile"] = 1.1;
                epss[0] = e;
                profile["epss"] = epss;
                row["profile"] = profile;
            }
            if (mode != 11 && mode != 12) rows[0] = row;
            f.dataset["samples"] = rows;
        }
        QVERIFY2(!f.runtime(error), qPrintable(QString::number(mode)));
        QVERIFY(!error.isEmpty());
    }
    const auto d = bytes(base.dataset);
    QVERIFY(!loadRuntimeDataset(d + ' ', sha256(d).toLatin1(), error));
    QVERIFY(error.contains("hash"));
}
void Phase11Test::aliasShared()
{
    const auto d = discover("npm", {record("GHSA-a", "example", {"CVE-2026-1234"}),
                                    record("GHSA-b", "example", {"CVE-2026-1234"})});
    QCOMPARE(d.units.size(), 1);
    QCOMPARE(d.units[0].clusterIds.size(), 3);
}
void Phase11Test::aliasPrimary()
{
    const auto d = discover("npm", {record("GHSA-a", "example", {"GHSA-b"}), record("GHSA-b")});
    QCOMPARE(d.units.size(), 1);
}
void Phase11Test::aliasTransitive()
{
    const auto d = discover("npm", {record("GHSA-a", "example", {"CVE-2026-1234"}),
                                    record("GHSA-b", "example", {"CVE-2026-1234", "CVE-2026-5678"}),
                                    record("GHSA-c", "example", {"CVE-2026-5678"})});
    QCOMPARE(d.units.size(), 1);
    QCOMPARE(d.units[0].clusterIds.size(), 5);
}
void Phase11Test::aliasPackageScope()
{
    const auto d = discover(
        "npm", {record("GHSA-a", "one", {"CVE-2026-1234"}), record("GHSA-b", "two", {"CVE-2026-1234"})});
    QCOMPARE(d.units.size(), 2);
}
void Phase11Test::aliasRelations()
{
    auto a = record("GHSA-a"), b = record("GHSA-b");
    a["related"] = QJsonArray{"GHSA-b"};
    a["upstream"] = QJsonArray{"GHSA-b"};
    QCOMPARE(discover("npm", {a, b}).units.size(), 2);
}
void Phase11Test::aliasOrder()
{
    auto a = record("GHSA-a", "example", {"CVE-2026-5678", "CVE-2026-1234"}),
         b = record("GHSA-b", "example", {"CVE-2026-1234"});
    const auto x = discover("npm", {a, b});
    a["aliases"] = QJsonArray{"CVE-2026-1234", "CVE-2026-5678"};
    const auto y = discover("npm", {b, a});
    QCOMPARE(x.units[0].clusterHash, y.units[0].clusterHash);
    QCOMPARE(x.units[0].selectionHash, y.units[0].selectionHash);
    QCOMPARE(x.units[0].clusterHash, arrayHash(QJsonArray::fromStringList(x.units[0].clusterIds)));
}
void Phase11Test::exactVersions()
{
    auto a = record("GHSA-a", "example", {}, {});
    a["summary"] = "affected 1.2.3";
    auto affected = a["affected"].toArray();
    auto entry = affected[0].toObject();
    entry["ranges"] = QJsonArray{QJsonObject{
        {"type", "SEMVER"},
        {"events", QJsonArray{QJsonObject{{"introduced", "0"}}, QJsonObject{{"fixed", "2.0.0"}}}}}};
    affected[0] = entry;
    affected.append(record("GHSA-b", "other")["affected"].toArray()[0]);
    a["affected"] = affected;
    const auto d = discover("npm", {a});
    QCOMPARE(d.units.size(), 1);
    QCOMPARE(d.units[0].identity.name, QString("other"));
    QVERIFY(QString::fromUtf8(QJsonDocument(d.exclusions).toJson())
                .contains("NoDeterministicExactAffectedVersion"));
}
void Phase11Test::productionVersionValidity()
{
    const QStringList versions{"1.2.3", "1.0rc1", QString("1") + QChar(1),
                               QString("1") + QChar(0x85), QString(QChar(0xd800)),
                               QString(PackageIdentity::MaxVersionLength + 1, u'x'), "", "   "};
    for (const auto &eco : QStringList{"PyPI", "npm"})
        for (const auto &version : versions)
        {
            Component component;
            component.purl = eco == "PyPI" ? "pkg:pypi/example" : "pkg:npm/example";
            component.version = version;
            const auto production = PackageIdentity::resolve(component);
            const auto found = discover(eco, {record("GHSA-version", "example", {}, {version}, eco)});
            QCOMPARE(!found.units.isEmpty(), production.state == IdentityState::Resolved);
            CandidateUnit unit{{eco, "example", version, 1},
                               {record("GHSA-version", "example", {}, {version}, eco)},
                               {"GHSA-version"}, arrayHash({"GHSA-version"}),
                               arrayHash({eco, "example", "GHSA-version", version})};
            const auto loaded = readUnit(unitJson(unit));
            QCOMPARE(loaded.has_value(), production.state == IdentityState::Resolved);
            if (loaded)
                QCOMPARE(loaded->identity, *production.query());
        }
}
void Phase11Test::frozenIdentityCompatibility()
{
    Files f;
    const auto pool = f.pool["candidates"].toArray(), samples = f.dataset["samples"].toArray();
    QCOMPARE(pool.size(), 776);
    QCOMPARE(samples.size(), 40);
    QSet<QString> selected;
    for (const auto &v : samples)
        selected.insert(v.toObject()["selectionHash"].toString());
    for (const auto &rows : {pool, samples})
        for (const auto &v : rows)
        {
            const auto row = v.toObject();
            const auto u = row.contains("unit") ? row["unit"].toObject() : row;
            Component component;
            component.version = u["exactVersion"].toString();
            component.purl = "pkg:" + (u["ecosystem"] == "PyPI" ? QString("pypi/") : QString("npm/")) +
                             QString::fromLatin1(QUrl::toPercentEncoding(u["canonicalPackageName"].toString(), "/"));
            const auto production = PackageIdentity::resolve(component);
            auto diagnostic = u;
            diagnostic.remove("record");
            diagnostic["selected"] = selected.contains(u["selectionHash"].toString());
            diagnostic["identityReason"] = identityReasonCode(production.reason);
            const auto loaded = readUnit(u);
            QVERIFY2(loaded && production.state == IdentityState::Resolved,
                     qPrintable(QString::fromUtf8(bytes(diagnostic))));
            QCOMPARE(loaded->identity, *production.query());
        }
}
void Phase11Test::representativeDeterminism()
{
    auto a = record("GHSA-a", "example", {"CVE-2026-1234"}, {"1.0.0", "2.0.0"}),
         b = record("GHSA-b", "example", {"CVE-2026-1234"}, {"3.0.0"});
    const auto d = discover("npm", {a, b});
    QStringList hashes{arrayHash({"npm", "example", "GHSA-a", "1.0.0"}),
                       arrayHash({"npm", "example", "GHSA-a", "2.0.0"}),
                       arrayHash({"npm", "example", "GHSA-b", "3.0.0"})};
    std::sort(hashes.begin(), hashes.end());
    QCOMPARE(d.units[0].selectionHash, hashes[0]);
    QCOMPARE(discover("npm", {b, a}).units[0].selectionHash, hashes[0]);
}
void Phase11Test::noAliasEnrichment()
{
    const auto d = discover("npm", {record("GHSA-a", "example", {"GHSA-b", "CVE-2026-1234"}),
                                    record("GHSA-b", "example", {"CVE-2026-5678"})});
    QCOMPARE(d.units.size(), 1);
    const auto &u = d.units[0];
    const auto p = prepareProfile(u, syntheticCases()[0].time, {}, std::nullopt, syntheticCases()[0].time);
    QCOMPARE(p.epss.size(), u.representative.cveAliases().size());
    for (const auto &e : p.epss)
        QVERIFY(u.representative.cveAliases().contains(e.cve));
    QCOMPARE(u.clusterIds.size(), 4);
}
void Phase11Test::duplicateAdvisory()
{
    const auto a = record("GHSA-a");
    const auto d = discover("npm", {a, a});
    QCOMPARE(d.units.size(), 1);
    QVERIFY(QString::fromUtf8(QJsonDocument(d.exclusions).toJson()).contains("DuplicateAdvisoryPackageUnit"));
}
void Phase11Test::strataFailures()
{
    auto p = syntheticCases().first().profile;
    const auto time = syntheticCases().first().time;
    for (const auto &pair : QList<QPair<double, QString>>{{.99, "S3"}, {.92, "S4"}, {.87, "S5"}, {.5, "S6"}})
    {
        p.epss[0].percentile = pair.first;
        QCOMPARE(primaryStratum(p, time), pair.second);
    }
    p.epss[0].status = EpssStatus::NotScored;
    p.epss[0].probability.reset();
    p.epss[0].percentile.reset();
    QCOMPARE(primaryStratum(p, time), QString("S2"));
    for (auto error : {QueryError::Timeout, QueryError::TlsFailure, QueryError::RateLimited,
                       QueryError::ResponseInvalid, QueryError::Cancelled, QueryError::CacheIo})
    {
        auto failed = p;
        failed.epss[0].error = error;
        QVERIFY(primaryStratum(failed, time).isEmpty());
    }
    p.kev[0].status = KevStatus::Listed;
    QCOMPARE(primaryStratum(p, time), QString("S1"));
    p.epss[0].status = EpssStatus::Failed;
    QVERIFY(primaryStratum(p, time).isEmpty());
}
void Phase11Test::orderingContract()
{
    Files f;
    auto rows = f.dataset["samples"].toArray();
    const QJsonValue first = rows[0];
    rows[0] = rows[1];
    rows[1] = first;
    f.dataset["samples"] = rows;
    QString error;
    QVERIFY(!f.load(error));
    QVERIFY(error.contains("ordering"));
}
void Phase11Test::frozenDataset()
{
    QString error;
    const auto d = loadDirectory(":/validation", error);
    QVERIFY2(d.has_value(), qPrintable(error));
    const auto stats = diversity(d->samples);
    QVERIFY(d->samples.size() >= 30);
    QCOMPARE(stats["sampleCount"], stats["uniqueAliasClusterCount"]);
    QVERIFY(stats["strata"].toObject()["S4"].toInt() > 0);
    QVERIFY(stats["strata"].toObject()["S5"].toInt() > 0);
    for (const auto &s : d->samples)
        QVERIFY(RiskPriorityEvaluator::evaluate(s.profile, d->referenceTime) ==
                *RiskPriorityEvaluator::evaluate(s.profile, d->referenceTime, {.90}));
}
void Phase11Test::sensitivity()
{
    QString error;
    const auto d = loadDirectory(":/validation", error);
    QVERIFY2(d.has_value(), qPrintable(error));
    const auto e = runExperiment(*d);
    QVERIFY2(e.result["automatedPass"].toBool(),
             qPrintable(QString::fromUtf8(bytes(e.result["invariants"].toObject()))));
    QVERIFY(e.result["sensitiveSamples"].toArray().size() >= 2);
    QCOMPARE(e.baseline.size(), d->samples.size());
}
void Phase11Test::replay()
{
    QString error;
    const auto d = loadDirectory(":/validation", error);
    QVERIFY(d);
    const auto a = runExperiment(*d), b = runExperiment(*d);
    QCOMPARE(a.result, b.result);
}
void Phase11Test::exportArtifact()
{
    QString error;
    const auto d = loadDirectory(":/validation", error);
    QVERIFY(d);
    const auto e = runExperiment(*d);
    QTemporaryDir tmp;
    QVERIFY(exportResult(e, tmp.filePath("phase11-validation-result.json"), error));
    const auto out =
        QJsonDocument::fromJson(readFile(tmp.filePath("phase11-validation-result.json"))).object();
    QVERIFY(out["runTimestampUtc"].isString());
    QCOMPARE(out["sourceGitCommit"].toString().size(), 40);
    QCOMPARE(out["applicationVersion"].toString(), QStringLiteral(APPLICATION_VERSION));
    QCOMPARE(out["schemaVersion"].toInt(), 4);
}
void Phase11Test::ui()
{
    MainWindow window(new QWidget, nullptr, "validation", new ValidationPage);
    window.resize(1000, 700);
    window.show();
    QCOMPARE(window.currentPageId(), QString("validation"));
    QCOMPARE(window.findChild<QStackedWidget *>("pages")->count(), 4);
    auto *combo = window.findChild<QComboBox *>("validationSamples");
    QTRY_VERIFY_WITH_TIMEOUT(combo->isEnabled(), 30000);
    auto *summary = window.findChild<QTextBrowser *>("validationSummary");
    QVERIFY(summary->toPlainText().contains(datasetDisclaimer()));
    QVERIFY(summary->toPlainText().contains(QStringLiteral("正式实验可离线复现")));
    QVERIFY(summary->toPlainText().contains("Baseline 0.90"));
    QVERIFY(summary->toPlainText().contains("Threshold 0.85"));
    QVERIFY(summary->toPlainText().contains("Transitions"));
    QVERIFY(!summary->toPlainText().contains("Accuracy"));
    QVERIFY(!summary->toPlainText().contains(QStringLiteral("92%被利用")));
    auto *threshold = window.findChild<QComboBox *>("validationThreshold");
    auto *detail = window.findChild<QTextBrowser *>("validationDetail");
    threshold->setCurrentIndex(0);
    QVERIFY(detail->toPlainText().contains("Percentile >= 0.85"));
    threshold->setCurrentIndex(2);
    QVERIFY(detail->toPlainText().contains("Percentile >= 0.95"));
    QVERIFY(window.selectPage("projects"));
    QVERIFY(window.selectPage("settings"));
    QVERIFY(window.selectPage("overview"));
    QVERIFY(window.selectPage("validation"));
}
void Phase11Test::uiRejected()
{
    QTemporaryDir tmp;
    ValidationPage page(nullptr, tmp.path());
    page.show();
    auto *summary = page.findChild<QTextBrowser *>("validationSummary");
    QTRY_VERIFY(summary->toPlainText().contains("rejected"));
    QVERIFY(!page.findChild<QComboBox *>("validationSamples")->isEnabled());
}
void Phase11Test::uiLifetime()
{
    auto *page = new ValidationPage;
    page->show();
    delete page;
    QVERIFY(QThreadPool::globalInstance()->waitForDone(30000));
}
QTEST_MAIN(Phase11Test)
#include "Phase11Test.moc"
