// Explicit research utility, never linked into the product UI. Uses production provider transport/parsers.
#include "ValidationSelection.h"
#include "EvidenceClients.h"
#include "CveIdentity.h"
#include <QCryptographicHash>
#include <QCoreApplication>
#include <QEventLoop>
#include <QJsonDocument>
#include <QDir>
#include <QFile>
#include <QSet>
#include <QTextStream>
#include <algorithm>
#include <stdexcept>

using namespace Validation;
namespace
{
void check(bool b, const QString &s)
{
    if (!b)
        throw std::runtime_error(s.toStdString());
}
QString utc(QDateTime t)
{
    return t.toUTC().toString(Qt::ISODateWithMs);
}
QJsonObject jsonFile(const QString &p)
{
    return QJsonDocument::fromJson(readFile(p)).object();
}
// Existing QueryError categories describing transient failures; TLS/invalid data are never retried.
bool transient(QueryError e)
{
    return e == QueryError::Timeout || e == QueryError::ConnectionFailure || e == QueryError::RateLimited ||
           e == QueryError::ServiceUnavailable;
}
struct Capture
{
    QJsonObject json;
    QHash<QString, EpssSnapshot> epss;
    std::optional<KevSnapshot> kev;
    QDateTime start, end;
    QJsonArray failures;
};
Capture capture(const QList<CandidateUnit> &units)
{
    Capture c;
    c.start = QDateTime::currentDateTimeUtc();
    QStringList ids;
    for (const auto &u : units)
        ids += u.representative.cveAliases();
    ids = sortedCveIds(ids);
    const auto inWindow = [&] { return c.start.msecsTo(QDateTime::currentDateTimeUtc()) < 6 * 3600000; };
    KevClient kev;
    KevFetchResult kr;
    QEventLoop kl;
    QObject::connect(&kev, &KevClient::finished, &kl, [&](const auto &r) {
        kr = r;
        kl.quit();
    });
    int attempts = 0;
    do
    {
        check(inWindow(), "Capture window exceeded");
        ++attempts;
        check(kev.fetch(), "KEV start failed");
        kl.exec();
    } while (kr.error != QueryError::None && transient(kr.error) && attempts < 3);
    QJsonObject kj;
    if (kr.snapshot)
    {
        c.kev = kr.snapshot;
        kj = {{"fetchedAt", utc(kr.snapshot->fetchedAt)},
              {"response", kr.snapshot->catalog},
              {"attempts", attempts}};
    }
    else
        c.failures.append(
            QJsonObject{{"provider", "KEV"}, {"error", queryErrorCode(kr.error)}, {"attempts", attempts}});
    QJsonArray responses;
    EpssClient epss;
    EpssChunkResult er;
    QEventLoop el;
    QObject::connect(&epss, &EpssClient::finished, &el, [&](const auto &r) {
        er = r;
        el.quit();
    });
    // Respect the production per-call alias bound even for a larger research pool.
    QList<QStringList> chunks;
    for (int i = 0; i < ids.size(); i += 5000)
        chunks += RiskEvidence::epssChunks(ids.mid(i, 5000));
    int done = 0;
    for (const auto &chunk : chunks)
    {
        attempts = 0;
        do
        {
            check(inWindow(), "Capture window exceeded");
            ++attempts;
            check(epss.fetch(chunk), "EPSS start failed");
            el.exec();
        } while (er.error != QueryError::None && transient(er.error) && attempts < 3);
        if (er.error == QueryError::None)
        {
            QJsonArray data;
            for (const auto &row : er.snapshots)
            {
                c.epss.insert(row.cve, row);
                if (!row.record.isEmpty())
                    data.append(row.record);
            }
            // Lossless production parsed response records; this is not a claim to retain HTTP byte
            // formatting.
            responses.append(
                QJsonObject{{"requested", QJsonArray::fromStringList(chunk)},
                            {"fetchedAt", utc(er.snapshots.first().fetchedAt)},
                            {"attempts", attempts},
                            {"response", QJsonObject{{"status", "OK"},
                                                     {"status-code", 200},
                                                     {"version", er.snapshots.first().providerVersion},
                                                     {"total", data.size()},
                                                     {"offset", 0},
                                                     {"limit", chunk.size()},
                                                     {"data", data}}}});
        }
        else
            c.failures.append(QJsonObject{{"provider", "EPSS"},
                                          {"requested", QJsonArray::fromStringList(chunk)},
                                          {"error", queryErrorCode(er.error)},
                                          {"attempts", attempts}});
        QTextStream(stdout) << "Captured chunk " << ++done << "/" << chunks.size() << Qt::endl;
    }
    c.end = QDateTime::currentDateTimeUtc();
    check(inWindow(), "Capture window exceeded");
    c.json = {{"epss", responses}, {"kev", kj}, {"failures", c.failures}};
    return c;
}
void write(const QString &path, const QJsonObject &o)
{
    QString e;
    check(writeFrozen(path, o, e), e);
}
QJsonObject evidenceProjection(const QJsonObject &record)
{
    QJsonObject out;
    for (const auto *k :
         {"id", "modified", "published", "withdrawn", "schema_version", "aliases", "severity"})
        if (record.contains(k))
            out[k] = record[k];
    QJsonArray affected;
    for (const auto &v : record["affected"].toArray())
    {
        QJsonObject a;
        const auto source = v.toObject();
        for (const auto *k : {"package", "versions", "ranges", "severity"})
            if (source.contains(k))
                a[k] = source[k];
        affected.append(a);
    }
    out["affected"] = affected;
    return out;
}
} // namespace
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    const auto args = app.arguments();
    if (args.size() != 4 || (args[1] != "discover" && args[1] != "capture" && args[1] != "package"))
    {
        QTextStream(stderr)
            << "Usage: ValidationPrepare discover|capture|package INPUT_DIRECTORY OUTPUT_DIRECTORY\n";
        return 2;
    }
    try
    {
        const QDir input(args[2]), output(args[3]);
        check(QDir().mkpath(output.path()), "Cannot create output directory");
        const auto manifestPath = output.filePath("candidate-pool-manifest-v1.json");
        if (args[1] == "package")
        {
            // Representation only: no provider lookup, resampling, reference-time change, or replacement.
            // Preserve the preparation bundle and reject any pre-existing destination files.
            QString error;
            const auto original = loadDirectory(input.path(), error);
            check(original.has_value(), error);
            const auto pool = jsonFile(input.filePath("candidate-pool-manifest-v1.json"));
            write(manifestPath, pool);
            auto index = jsonFile(input.filePath("candidate-selection-index-v1.json"));
            index["candidatePoolSHA256"] = sha256(readFile(manifestPath));
            const auto indexPath = output.filePath("candidate-selection-index-v1.json");
            write(indexPath, index);
            auto dataset = jsonFile(input.filePath("phase11-validation-dataset-v1.json"));
            dataset["candidatePoolSHA256"] = index["candidatePoolSHA256"];
            dataset["selectionIndexSHA256"] = sha256(readFile(indexPath));
            write(output.filePath("phase11-validation-dataset-v1.json"), dataset);
            const auto packaged = loadDirectory(output.path(), error);
            check(packaged.has_value(), error);
            check(original->samples.size() == packaged->samples.size() &&
                      original->referenceTime == packaged->referenceTime,
                  "Packaging changed experiment inputs");
            for (int i = 0; i < original->samples.size(); ++i)
                check(profileJson(original->samples[i].profile) == profileJson(packaged->samples[i].profile),
                      "Packaging changed a profile");
            QTextStream(stdout) << "Packaged identical frozen inputs without resampling: "
                                << packaged->samples.size() << Qt::endl;
            return 0;
        }
        if (args[1] == "discover")
        {
            check(!QFile::exists(manifestPath), "Pool already frozen; refusing replacement");
            QJsonArray sources, exclusions;
            QList<CandidateUnit> frame;
            for (const auto &eco : QStringList{"PyPI", "npm"})
            {
                auto source = jsonFile(input.filePath(eco + "-source.json"));
                check(!source.isEmpty(), "Missing snapshot provenance");
                check(source["ecosystem"] == eco &&
                          source["sourceIdentity"] ==
                              "https://storage.googleapis.com/osv-vulnerabilities/" + eco + "/all.zip",
                      "Non-official snapshot");
                QFile archive(input.filePath(eco + "-all.zip"));
                check(archive.open(QIODevice::ReadOnly), "Missing exact source bytes");
                QCryptographicHash archiveHash(QCryptographicHash::Sha256);
                check(archiveHash.addData(&archive), "Cannot hash archive");
                check(QString::fromLatin1(archiveHash.result().toHex()) == source["sourceSHA256"] &&
                          archive.size() == source["sourceSizeBytes"].toInteger(),
                      "Archive provenance mismatch");
                QFile f(input.filePath(eco + "-records.json"));
                check(f.open(QIODevice::ReadOnly), "Missing snapshot records");
                const auto recordBytes = f.readAll();
                check(source["recordsSHA256"] == sha256(recordBytes), "Extracted record hash mismatch");
                sources.append(source);
                const auto records = QJsonDocument::fromJson(recordBytes).array();
                check(!records.isEmpty(), "Empty snapshot records");
                auto found = discover(eco, records);
                for (const auto &v : found.exclusions)
                    exclusions.append(v);
                int queryable = 0, absent = 0;
                // A fixed, outcome-blind discovery frame declared before any provider/experiment call.
                // Limit preparation cost; do not backfill based on sensitivity outcomes.
                for (auto u : found.units)
                {
                    auto &n = u.representative.cveAliases().isEmpty() ? absent : queryable;
                    const int limit = u.representative.cveAliases().isEmpty() ? 16 : 512;
                    if (n++ >= limit)
                    {
                        auto row = unitJson(u);
                        row.remove("record");
                        row["selectionReason"] = "DiscoveryFrameLimit";
                        exclusions.append(row);
                        continue;
                    }
                    u.representative.record = evidenceProjection(u.representative.record);
                    frame.append(u);
                }
                QTextStream(stdout) << eco << " eligible units=" << found.units.size()
                                    << "; frame=" << qMin(queryable, 512) + qMin(absent, 16) << Qt::endl;
            }
            std::sort(frame.begin(), frame.end(),
                      [](const auto &a, const auto &b) { return a.selectionHash < b.selectionHash; });
            const auto discovery = capture(frame);
            QJsonArray candidates;
            for (const auto &u : frame)
            {
                auto row = unitJson(u);
                const auto p =
                    prepareProfile(u, discovery.start, discovery.epss, discovery.kev, discovery.end);
                row["discoveryStratum"] = primaryStratum(p, discovery.end);
                candidates.append(row);
            }
            write(input.filePath("discovery-capture.json"), discovery.json);
            write(
                manifestPath,
                {{"candidatePoolVersion", "candidate-pool-v1"},
                 {"sources", sources},
                 {"frozenAtUtc", utc(QDateTime::currentDateTimeUtc())},
                 {"candidateCount", candidates.size()},
                 {"identifierOrdering", "QString UTF-16 code-unit lexicographic; compact UTF-8 JSON SHA-256"},
                 {"discoveryFrame",
                  "Per ecosystem: first 512 CVE-bearing and 16 no-CVE units by representative selectionHash, "
                  "before provider lookup; no outcome backfill"},
                 {"candidates", candidates},
                 {"constructionExclusions", exclusions}});
            QTextStream(stdout) << "Candidate pool frozen: " << candidates.size() << Qt::endl;
        }
        else
        {
            const auto manifestBytes = readFile(manifestPath);
            const auto manifest = jsonFile(manifestPath);
            check(sha256(manifestBytes).toLatin1() == readFile(manifestPath + ".sha256").trimmed(),
                  "Pool hash mismatch");
            QList<CandidateUnit> units;
            QHash<QString, QDateTime> times;
            for (const auto &v : manifest["sources"].toArray())
            {
                const auto s = v.toObject();
                times[s["ecosystem"].toString()] =
                    QDateTime::fromString(s["downloadedAtUtc"].toString(), Qt::ISODateWithMs);
            }
            for (const auto &v : manifest["candidates"].toArray())
            {
                const auto u = readUnit(v.toObject());
                check(u.has_value(), "Invalid frozen unit");
                units.append(*u);
            }
            const auto final = capture(units);
            write(input.filePath("final-capture.json"), final.json);
            const auto ref = final.end;
            QMap<QString, QStringList> strata;
            QHash<QString, RiskEvidenceProfile> profiles;
            QHash<QString, QString> classification;
            for (const auto &u : units)
            {
                auto p = prepareProfile(u, times[u.identity.ecosystem], final.epss, final.kev, ref);
                bool complete = final.kev.has_value();
                for (const auto &c : sortedCveIds(u.representative.cveAliases()))
                    complete &= final.epss.contains(c);
                const auto s = complete ? primaryStratum(p, ref) : QString();
                profiles[u.selectionHash] = p;
                classification[u.selectionHash] = s;
                if (!s.isEmpty())
                    strata[s].append(u.selectionHash);
            }
            QSet<QString> selected;
            const QMap<QString, int> targets{{"S1", 6}, {"S2", 6}, {"S3", 6},
                                             {"S4", 8}, {"S5", 8}, {"S6", 6}};
            for (auto it = strata.begin(); it != strata.end(); ++it)
            {
                std::sort(it.value().begin(), it.value().end());
                for (const auto &h : it.value().mid(0, targets[it.key()]))
                    selected.insert(h);
            }
            QJsonArray index, samples;
            for (const auto &u : units)
            {
                auto row = unitJson(u);
                row.remove("record");
                const auto s = classification[u.selectionHash];
                row["eligible"] = !s.isEmpty();
                row["selected"] = selected.contains(u.selectionHash);
                row["finalPrimaryStratum"] = s;
                row["finalCaptureStatus"] = s.isEmpty() ? "Failed" : "Success";
                row["selectionReason"] = s.isEmpty()                          ? "FinalCaptureFailed"
                                         : selected.contains(u.selectionHash) ? "Selected"
                                                                              : "QuotaExceeded";
                index.append(row);
                if (selected.contains(u.selectionHash))
                    samples.append(QJsonObject{{"sampleId", "sample-" + u.selectionHash},
                                               {"selectionHash", u.selectionHash},
                                               {"primaryStratum", s},
                                               {"unit", unitJson(u)},
                                               {"profile", profileJson(profiles[u.selectionHash])}});
            }
            const QJsonObject indexObject{
                {"candidatePoolVersion", "candidate-pool-v1"},
                {"candidatePoolSHA256", sha256(manifestBytes)},
                {"candidates", index},
                {"captureFailures", final.failures},
                {"constructionExclusions", manifest["constructionExclusions"]},
                {"selectionReasonVocabulary",
                 QJsonArray{"Selected", "QuotaExceeded", "AliasEquivalentDuplicate",
                            "NoDeterministicExactAffectedVersion", "DuplicateAdvisoryPackageUnit",
                            "FinalCaptureFailed", "NotConfirmedFinding", "FinalStratumIneligible",
                            "PrivacyRejected", "DatasetContractRejected", "DiscoveryFrameLimit"}}};
            const auto indexPath = output.filePath("candidate-selection-index-v1.json");
            write(indexPath, indexObject);
            check(final.kev.has_value(), "FinalCaptureFailed: KEV; dataset not frozen");
            check(samples.size() >= 30 && !strata["S4"].isEmpty() && !strata["S5"].isEmpty(),
                  "Final coverage gate failed; dataset not frozen");
            QJsonObject dataset{{"datasetSchemaVersion", 1},
                                {"datasetVersion", "phase11-v1"},
                                {"candidatePoolVersion", "candidate-pool-v1"},
                                {"candidatePoolSHA256", sha256(manifestBytes)},
                                {"selectionIndexSHA256", sha256(readFile(indexPath))},
                                {"captureWindowStartUtc", utc(final.start)},
                                {"captureWindowEndUtc", utc(final.end)},
                                {"evaluationReferenceTimeUtc", utc(ref)},
                                {"capture", final.json},
                                {"samples", samples}};
            const auto bytes = QJsonDocument(dataset).toJson(QJsonDocument::Indented);
            QString error;
            const auto verified = loadDataset(bytes, sha256(bytes).toLatin1(), manifestBytes,
                                              readFile(manifestPath + ".sha256"), readFile(indexPath),
                                              readFile(indexPath + ".sha256"), error);
            check(verified.has_value(), error);
            write(output.filePath("phase11-validation-dataset-v1.json"), dataset);
            QTextStream(stdout) << "Dataset frozen: "
                                << QJsonDocument(diversity(verified->samples)).toJson(QJsonDocument::Compact)
                                << Qt::endl;
        }
        return 0;
    }
    catch (const std::exception &e)
    {
        QTextStream(stderr) << "BLOCKED: " << e.what() << Qt::endl;
        return 1;
    }
}
