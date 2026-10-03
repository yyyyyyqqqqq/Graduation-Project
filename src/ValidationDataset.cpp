#include "ValidationDataset.h"
#include "ValidationSelection.h"
#include "CveIdentity.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace Validation
{
namespace
{
void require(bool ok, const char *message)
{
    if (!ok)
        throw std::runtime_error(message);
}
void allowedKeys(const QJsonObject &object, const QStringList &keys)
{
    for (auto it = object.begin(); it != object.end(); ++it)
        require(keys.contains(it.key()), "Unknown dataset contract field");
}
QJsonValue timeJson(QDateTime t)
{
    return t.isValid() ? QJsonValue(t.toUTC().toString(Qt::ISODateWithMs)) : QJsonValue();
}
QDateTime timeValue(const QJsonValue &v, bool nullable = false)
{
    if (nullable && v.isNull())
        return {};
    require(v.isString() && OsvResponseParser::validTimestamp(v.toString()), "Invalid timestamp");
    auto t = QDateTime::fromString(v.toString(), Qt::ISODateWithMs);
    require(t.isValid(), "Invalid timestamp");
    return t.toUTC();
}
template <class T> QJsonValue optionalJson(const std::optional<T> &v)
{
    return v ? QJsonValue(double(*v)) : QJsonValue();
}
template <class T> std::optional<T> optionalNumber(const QJsonValue &v, double max, bool integral = false)
{
    if (v.isNull())
        return {};
    require(v.isDouble() && std::isfinite(v.toDouble()) && v.toDouble() >= 0 && v.toDouble() <= max &&
                (!integral || std::floor(v.toDouble()) == v.toDouble()),
            "Invalid numeric field");
    return T(v.toDouble());
}
template <class T> T enumValue(const QJsonValue &v, int max)
{
    const auto n = optionalNumber<int>(v, max, true);
    require(n.has_value(), "Missing enum");
    return T(*n);
}
QJsonObject objectBytes(const QByteArray &bytes, const QByteArray &sidecar)
{
    require(!bytes.isEmpty() && bytes.size() <= 128 * 1024 * 1024, "Invalid file size");
    require(sidecar.trimmed() == sha256(bytes).toLatin1(), "Sidecar hash mismatch");
    QJsonParseError error;
    const auto d = QJsonDocument::fromJson(bytes, &error);
    require(error.error == QJsonParseError::NoError && d.isObject(), "Invalid JSON object");
    return d.object();
}
bool hashString(const QJsonValue &value)
{
    static const QRegularExpression r("\\A[0-9a-f]{64}\\z");
    return value.isString() && r.match(value.toString()).hasMatch();
}
void publicKeys(const QJsonValue &v)
{
    if (v.isString())
    {
        static const QRegularExpression path("\\A(?:[A-Za-z]:[\\\\/]|file:|\\\\\\\\|/(?:home|Users|tmp)/)");
        require(!path.match(v.toString()).hasMatch(), "Local path is not public evidence");
    }
    if (v.isArray())
        for (const auto &child : v.toArray())
            publicKeys(child);
    if (!v.isObject())
        return;
    const auto o = v.toObject();
    for (auto it = o.begin(); it != o.end(); ++it)
    {
        const auto k = it.key().toLower();
        require(k != "projectid" && k != "databasepath" && k != "sbompath" && k != "authorization" &&
                    k != "api_key" && k != "apikey" && k != "password" && k != "cookie" && k != "token" &&
                    k != "credential" && k != "localpath",
                "Forbidden/private-like field");
        publicKeys(it.value());
    }
}
} // namespace
QString sha256(const QByteArray &b)
{
    return QString::fromLatin1(QCryptographicHash::hash(b, QCryptographicHash::Sha256).toHex());
}
QString arrayHash(const QJsonArray &a)
{
    return sha256(QJsonDocument(a).toJson(QJsonDocument::Compact));
}
QByteArray readFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly) || f.size() > 128 * 1024 * 1024)
        return {};
    return f.readAll();
}
bool writeFrozen(const QString &path, const QJsonObject &object, QString &error)
{
    if (QFile::exists(path) || QFile::exists(path + ".sha256"))
    {
        error = "Frozen output already exists";
        return false;
    }
    // Compact public fixtures keep the complete construction audit within Git hosting file limits.
    // The sidecar always hashes the exact bytes written, never a normalized view of an existing file.
    const auto bytes = QJsonDocument(object).toJson(QJsonDocument::Compact);
    for (const auto &pair : QList<QPair<QString, QByteArray>>{
             {path, bytes}, {path + ".sha256", sha256(bytes).toLatin1() + '\n'}})
    {
        QSaveFile f(pair.first);
        if (!f.open(QIODevice::WriteOnly) || f.write(pair.second) != pair.second.size() || !f.commit())
        {
            error = "Frozen file write failed";
            return false;
        }
    }
    return true;
}
QJsonObject profileJson(const RiskEvidenceProfile &p)
{
    const auto &k = p.key;
    const auto &d = p.dependency;
    QJsonArray eRows, kRows, items;
    for (const auto &e : p.epss)
        eRows.append(
            QJsonObject{{"cve", e.cve},
                        {"status", int(e.status)},
                        {"freshness", int(e.freshness)},
                        {"acquisition", int(e.acquisition)},
                        {"probability", optionalJson(e.probability)},
                        {"percentile", optionalJson(e.percentile)},
                        {"scoreDate", e.scoreDate.isValid() ? QJsonValue(e.scoreDate.toString(Qt::ISODate))
                                                            : QJsonValue()},
                        {"fetchedAt", timeJson(e.fetchedAt)},
                        {"providerVersion", e.providerVersion},
                        {"error", int(e.error)},
                        {"cacheError", int(e.cacheError)}});
    for (const auto &e : p.kev)
        kRows.append(QJsonObject{{"cve", e.cve},
                                 {"status", int(e.status)},
                                 {"freshness", int(e.freshness)},
                                 {"acquisition", int(e.acquisition)},
                                 {"catalogVersion", e.catalogVersion},
                                 {"dateReleased", e.dateReleased},
                                 {"fetchedAt", timeJson(e.fetchedAt)},
                                 {"entry", e.entry},
                                 {"error", int(e.error)},
                                 {"cacheError", int(e.cacheError)}});
    for (const auto &e : p.severity.items)
        items.append(QJsonObject{{"type", e.type},
                                 {"vector", e.vector},
                                 {"source", e.source},
                                 {"recordId", e.recordId},
                                 {"scope", int(e.scope)},
                                 {"provenance", int(e.provenance)},
                                 {"affectedIndex", optionalJson(e.affectedIndex)},
                                 {"status", int(e.status)}});
    return {{"key",
             QJsonObject{{"componentId", k.componentId},
                         {"osvId", k.osvId},
                         {"snapshotIdentity", QJsonObject{{"ecosystem", k.snapshotIdentity.ecosystem},
                                                          {"name", k.snapshotIdentity.name},
                                                          {"version", k.snapshotIdentity.version},
                                                          {"rulesVersion", k.snapshotIdentity.rulesVersion}}},
                         {"snapshotFetchedAt", timeJson(k.snapshotFetchedAt)},
                         {"applicabilityGeneration", QString::number(k.applicabilityGeneration)}}},
            {"generatedAt", timeJson(p.generatedAt)},
            {"operation", int(p.operation)},
            {"quality", int(p.quality)},
            {"severity", QJsonObject{{"status", int(p.severity.status)}, {"items", items}}},
            {"epss", eRows},
            {"kev", kRows},
            {"dependency",
             QJsonObject{{"captured", d.captured},
                         {"root", int(d.root)},
                         {"path", int(d.path)},
                         {"depthFromRoot", optionalJson(d.depthFromRoot)},
                         {"directDependents", optionalJson(d.directDependents)},
                         {"transitiveDependents", optionalJson(d.transitiveDependents)},
                         {"referenceResolution",
                          d.referenceResolution ? QJsonValue(int(*d.referenceResolution)) : QJsonValue()}}}};
}
std::optional<RiskEvidenceProfile> readProfile(const QJsonObject &o, QString &error)
{
    try
    {
        RiskEvidenceProfile p;
        const auto k = o["key"].toObject(), i = k["snapshotIdentity"].toObject();
        p.key.componentId = k["componentId"].toString();
        p.key.osvId = k["osvId"].toString();
        p.key.snapshotIdentity = {i["ecosystem"].toString(), i["name"].toString(), i["version"].toString(),
                                  i["rulesVersion"].toInt()};
        p.key.snapshotFetchedAt = timeValue(k["snapshotFetchedAt"]);
        bool ok = false;
        p.key.applicabilityGeneration = k["applicabilityGeneration"].toString().toULongLong(&ok);
        require(ok, "Invalid generation");
        p.generatedAt = timeValue(o["generatedAt"]);
        p.operation = enumValue<EvidenceOperationState>(o["operation"], 7);
        p.quality = enumValue<QualityEvidenceStatus>(o["quality"], 0);
        const auto severity = o["severity"].toObject();
        p.severity.status = enumValue<SeverityStatus>(severity["status"], 4);
        require(severity["items"].isArray() && o["epss"].isArray() && o["kev"].isArray(),
                "Missing evidence rows");
        for (const auto &v : severity["items"].toArray())
        {
            const auto e = v.toObject();
            SeverityEvidenceItem x;
            x.type = e["type"].toString();
            x.vector = e["vector"].toString();
            x.source = e["source"].toString();
            x.recordId = e["recordId"].toString();
            x.scope = enumValue<SeverityScope>(e["scope"], 1);
            x.provenance = enumValue<SeverityProvenance>(e["provenance"], 1);
            x.status = enumValue<SeverityStatus>(e["status"], 4);
            x.affectedIndex = optionalNumber<qsizetype>(e["affectedIndex"], 100000, true);
            p.severity.items.append(x);
        }
        for (const auto &v : o["epss"].toArray())
        {
            const auto e = v.toObject();
            EpssEvidence x;
            x.cve = e["cve"].toString();
            x.status = enumValue<EpssStatus>(e["status"], 4);
            x.freshness = enumValue<EvidenceFreshness>(e["freshness"], 2);
            x.acquisition = enumValue<EvidenceAcquisition>(e["acquisition"], 3);
            x.probability = optionalNumber<double>(e["probability"], 1);
            x.percentile = optionalNumber<double>(e["percentile"], 1);
            if (!e["scoreDate"].isNull())
            {
                x.scoreDate = QDate::fromString(e["scoreDate"].toString(), Qt::ISODate);
                require(x.scoreDate.isValid(), "Invalid score date");
            }
            x.fetchedAt = timeValue(e["fetchedAt"], true);
            x.providerVersion = e["providerVersion"].toString();
            x.error = enumValue<QueryError>(e["error"], int(QueryError::Database));
            x.cacheError = enumValue<QueryError>(e["cacheError"], int(QueryError::Database));
            p.epss.append(x);
        }
        for (const auto &v : o["kev"].toArray())
        {
            const auto e = v.toObject();
            KevEvidence x;
            x.cve = e["cve"].toString();
            x.status = enumValue<KevStatus>(e["status"], 3);
            x.freshness = enumValue<EvidenceFreshness>(e["freshness"], 2);
            x.acquisition = enumValue<EvidenceAcquisition>(e["acquisition"], 3);
            x.catalogVersion = e["catalogVersion"].toString();
            x.dateReleased = e["dateReleased"].toString();
            x.entry = e["entry"].toObject();
            x.fetchedAt = timeValue(e["fetchedAt"], true);
            x.error = enumValue<QueryError>(e["error"], int(QueryError::Database));
            x.cacheError = enumValue<QueryError>(e["cacheError"], int(QueryError::Database));
            p.kev.append(x);
        }
        const auto d = o["dependency"].toObject();
        require(d["captured"].isBool(), "Invalid dependency");
        p.dependency.captured = d["captured"].toBool();
        p.dependency.root = enumValue<DependencyRootStatus>(d["root"], 3);
        p.dependency.path = enumValue<DependencyPathState>(d["path"], 5);
        p.dependency.depthFromRoot = optionalNumber<int>(d["depthFromRoot"], 100000, true);
        p.dependency.directDependents = optionalNumber<qsizetype>(d["directDependents"], 100000, true);
        p.dependency.transitiveDependents =
            optionalNumber<qsizetype>(d["transitiveDependents"], 100000, true);
        if (!d["referenceResolution"].isNull())
            p.dependency.referenceResolution =
                enumValue<ReferenceResolutionCompleteness>(d["referenceResolution"], 1);
        require(profileJson(p) == o, "Profile reconstruction is not lossless / unknown field");
        return p;
    }
    catch (const std::exception &e)
    {
        error = QString::fromUtf8(e.what());
        return {};
    }
}
QString primaryStratum(const RiskEvidenceProfile &p, QDateTime time)
{
    // Classification is for sampling, never a replacement Priority/Support algorithm.
    const auto a = RiskPriorityEvaluator::evaluate(p, time);
    for (const auto &e : p.epss)
        if (e.status == EpssStatus::Failed || e.status == EpssStatus::InvalidResponse ||
            e.error != QueryError::None || e.cacheError != QueryError::None)
            return {};
    for (const auto &k : p.kev)
        if (k.status == KevStatus::Unknown || k.error != QueryError::None || k.cacheError != QueryError::None)
            return {};
    if (a.driverKind == PriorityDriverKind::Kev)
        return "S1";
    for (const auto &row : a.evidenceFreshness)
        if (row.epss == EffectiveDecisionFreshness::Stale ||
            row.epss == EffectiveDecisionFreshness::Ineligible ||
            row.kev != EffectiveDecisionFreshness::Fresh)
            return {};
    for (const auto &e : p.epss)
        if (e.status == EpssStatus::NotScored || e.status == EpssStatus::NotQueryable)
            return "S2";
    if (!a.driverEpssPercentile)
        return {};
    const double percentile = *a.driverEpssPercentile;
    return percentile >= .95 ? "S3" : percentile >= .90 ? "S4" : percentile >= .85 ? "S5" : "S6";
}
QJsonObject diversity(const QList<Sample> &samples)
{
    QSet<QString> packages, osv, cves, clusters;
    int multi = 0;
    QJsonObject strata;
    for (const auto &s : samples)
    {
        packages.insert(
            arrayHash({s.profile.key.snapshotIdentity.ecosystem, s.profile.key.snapshotIdentity.name}));
        osv.insert(s.candidate.id());
        clusters.insert(arrayHash(
            {s.profile.key.snapshotIdentity.ecosystem, s.profile.key.snapshotIdentity.name, s.clusterHash}));
        const auto aliases = sortedCveIds(s.candidate.cveAliases());
        for (const auto &c : aliases)
            cves.insert(c);
        if (aliases.size() > 1)
            ++multi;
        strata[s.stratum] = strata[s.stratum].toInt() + 1;
    }
    return {{"sampleCount", samples.size()},
            {"uniquePackageCount", packages.size()},
            {"uniqueOsvIdCount", osv.size()},
            {"uniqueCveAliasCount", cves.size()},
            {"uniqueAliasClusterCount", clusters.size()},
            {"multiCveFindingCount", multi},
            {"strata", strata}};
}
namespace
{
void captureAttempts(const QJsonValue &v)
{
    require(v.isDouble() && std::isfinite(v.toDouble()) && v.toDouble() >= 1 &&
                v.toDouble() <= 3 && std::floor(v.toDouble()) == v.toDouble(),
            "Invalid capture attempts");
}
struct RuntimeInput
{
    Dataset dataset;
    QDateTime start;
    QHash<QString, EpssSnapshot> epss;
    std::optional<KevSnapshot> kev;
    QSet<QString> failedCves;
};
// Shared Dataset-local contract. Full audit additionally proves pool/selection history.
RuntimeInput runtimeInput(const QJsonObject &o, const QString &hash)
{
    allowedKeys(o, {"datasetSchemaVersion", "datasetVersion", "candidatePoolVersion",
                    "candidatePoolSHA256", "selectionIndexSHA256", "captureWindowStartUtc",
                    "captureWindowEndUtc", "evaluationReferenceTimeUtc", "capture", "samples"});
    publicKeys(o);
    require(o["datasetSchemaVersion"] == 1 && o["datasetVersion"] == "phase11-v1",
            "Dataset schema/version mismatch");
    require(o["candidatePoolVersion"] == "candidate-pool-v1" &&
                hashString(o["candidatePoolSHA256"]) && hashString(o["selectionIndexSHA256"]),
            "Invalid pool/index metadata");
    RuntimeInput input;
    input.start = timeValue(o["captureWindowStartUtc"]);
    const auto start = input.start, end = timeValue(o["captureWindowEndUtc"]),
               ref = timeValue(o["evaluationReferenceTimeUtc"]);
    require(start <= end && start.msecsTo(end) <= 6 * 3600000 && end <= ref &&
                start.msecsTo(ref) < 86400000,
            "Capture window / reference time invalid");
    require(o["capture"].isObject() && o["samples"].isArray(), "Missing dataset capture/samples");
    const auto capture = o["capture"].toObject();
    allowedKeys(capture, {"epss", "kev", "failures"});
    require(capture["epss"].isArray() && capture["kev"].isObject() && capture["failures"].isArray(),
            "Invalid capture structure");
    for (const auto &v : capture["epss"].toArray())
    {
        require(v.isObject(), "Invalid EPSS capture");
        const auto e = v.toObject();
        allowedKeys(e, {"requested", "fetchedAt", "attempts", "response"});
        captureAttempts(e["attempts"]);
        const auto fetched = timeValue(e["fetchedAt"]);
        require(start <= fetched && fetched <= end, "EPSS outside final capture");
        require(e["requested"].isArray() && e["response"].isObject(), "Invalid EPSS capture");
        QStringList requested;
        for (const auto &c : e["requested"].toArray())
        {
            require(c.isString(), "Invalid requested CVE");
            requested.append(c.toString());
        }
        const auto parsed = RiskEvidence::parseEpss(
            QJsonDocument(e["response"].toObject()).toJson(QJsonDocument::Compact), requested, fetched);
        require(parsed.has_value(), "Invalid EPSS capture");
        for (const auto &row : *parsed)
        {
            require(!input.epss.contains(row.cve), "Duplicate captured CVE");
            input.epss.insert(row.cve, row);
        }
    }
    const auto kevObject = capture["kev"].toObject();
    allowedKeys(kevObject, {"fetchedAt", "attempts", "response"});
    captureAttempts(kevObject["attempts"]);
    const auto kevTime = timeValue(kevObject["fetchedAt"]);
    require(start <= kevTime && kevTime <= end, "KEV outside final capture");
    input.kev = RiskEvidence::parseKev(
        QJsonDocument(kevObject["response"].toObject()).toJson(QJsonDocument::Compact), kevTime);
    require(input.kev.has_value(), "Invalid KEV complete catalog");
    for (const auto &v : capture["failures"].toArray())
    {
        require(v.isObject(), "Invalid capture failure");
        const auto failure = v.toObject();
        allowedKeys(failure, {"provider", "requested", "error", "attempts"});
        captureAttempts(failure["attempts"]);
        bool knownError = false;
        for (int i = int(QueryError::RequestRejected); i <= int(QueryError::Database); ++i)
            knownError |= failure["error"] == queryErrorCode(QueryError(i));
        require(knownError, "Invalid capture failure error");
        // Builder records final failures, not failed intermediate attempts. A formal Dataset
        // always requires a successful KEV catalog, so a final KEV failure is contradictory.
        require(failure["provider"] == "EPSS", "Failure conflicts with successful KEV capture");
        require(failure["requested"].isArray() && !failure["requested"].toArray().isEmpty(),
                "Invalid failed EPSS request");
        for (const auto &c : failure["requested"].toArray())
        {
            const auto cve = c.toString();
            require(c.isString() && validCveId(cve) && !input.epss.contains(cve) &&
                        !input.failedCves.contains(cve),
                    "Contradictory/duplicate EPSS failure");
            input.failedCves.insert(cve);
        }
    }
    auto &dataset = input.dataset;
    dataset.metadata = o;
    dataset.metadata.remove("samples");
    dataset.metadata.remove("capture");
    dataset.referenceTime = ref;
    dataset.hash = hash;
    dataset.poolHash = o["candidatePoolSHA256"].toString();
    dataset.indexHash = o["selectionIndexSHA256"].toString();
    QSet<QString> ids, clusters;
    QString previous;
    for (const auto &v : o["samples"].toArray())
    {
        require(v.isObject(), "Invalid sample");
        const auto s = v.toObject();
        allowedKeys(s, {"sampleId", "selectionHash", "primaryStratum", "unit", "profile"});
        const auto id = s["sampleId"].toString(), selectionHash = s["selectionHash"].toString();
        require(hashString(s["selectionHash"]) && id == "sample-" + selectionHash && !ids.contains(id),
                "Duplicate/invalid sampleId");
        require(previous.isEmpty() || previous < id, "Non-deterministic sample ordering");
        previous = id;
        ids.insert(id);
        const auto u = readUnit(s["unit"].toObject());
        require(u && u->selectionHash == selectionHash && s["unit"] == unitJson(*u), "Invalid sample unit");
        const auto cluster = arrayHash({u->identity.ecosystem, u->identity.name, u->clusterHash});
        require(!clusters.contains(cluster), "Duplicate alias cluster");
        clusters.insert(cluster);
        QString error;
        const auto p = readProfile(s["profile"].toObject(), error);
        require(p.has_value(), "Invalid profile reconstruction");
        require(p->key.snapshotFetchedAt <= start, "Snapshot after capture");
        const auto actual = prepareProfile(*u, p->key.snapshotFetchedAt, input.epss, input.kev, ref);
        require(profileJson(*p) == profileJson(actual), "Profile differs from representative production evidence");
        const auto stratum = primaryStratum(*p, ref);
        require(!stratum.isEmpty() && s["primaryStratum"] == stratum, "Sample stratum mismatch");
        dataset.samples.append({id, stratum, u->clusterHash, u->clusterIds, u->representative, *p});
    }
    const auto stats = diversity(dataset.samples), strata = stats["strata"].toObject();
    require(dataset.samples.size() >= 30 && strata["S4"].toInt() >= 1 && strata["S5"].toInt() >= 1,
            "Dataset coverage gate failed");
    require(stats["uniqueAliasClusterCount"] == stats["sampleCount"], "Duplicate alias cluster");
    return input;
}
} // namespace
std::optional<Dataset> loadRuntimeDataset(const QByteArray &bytes, const QByteArray &sidecar, QString &error)
{
    try
    {
        return runtimeInput(objectBytes(bytes, sidecar), sha256(bytes)).dataset;
    }
    catch (const std::exception &e)
    {
        error = QString::fromUtf8(e.what());
        return {};
    }
}
std::optional<Dataset> loadRuntimeDirectory(const QString &directory, QString &error)
{
    const QDir d(directory);
    return loadRuntimeDataset(readFile(d.filePath("phase11-validation-dataset-v1.json")),
                              readFile(d.filePath("phase11-validation-dataset-v1.json.sha256")), error);
}
std::optional<Dataset> loadDataset(const QByteArray &bytes, const QByteArray &sidecar,
                                   const QByteArray &poolBytes, const QByteArray &poolSidecar,
                                   const QByteArray &indexBytes, const QByteArray &indexSidecar,
                                   QString &error)
{
    try
    {
        const auto o = objectBytes(bytes, sidecar);
        auto input = runtimeInput(o, sha256(bytes));
        const auto pool = objectBytes(poolBytes, poolSidecar), index = objectBytes(indexBytes, indexSidecar);
        allowedKeys(pool, {"candidatePoolVersion", "sources", "frozenAtUtc", "candidateCount",
                           "identifierOrdering", "discoveryFrame", "candidates", "constructionExclusions"});
        allowedKeys(index, {"candidatePoolVersion", "candidatePoolSHA256", "candidates", "captureFailures",
                            "constructionExclusions", "selectionReasonVocabulary"});
        publicKeys(pool);
        publicKeys(index);
        require(pool["candidatePoolVersion"] == o["candidatePoolVersion"], "Pool version mismatch");
        // Builder copies the same deterministically ordered final-failure array into both files.
        require(index["captureFailures"].isArray() &&
                    index["captureFailures"] == o["capture"].toObject()["failures"],
                "Capture failure audit mismatch");
        require(o["candidatePoolSHA256"] == sha256(poolBytes) &&
                    o["selectionIndexSHA256"] == sha256(indexBytes),
                "Provenance hash mismatch");
        require(index["candidatePoolSHA256"] == sha256(poolBytes) &&
                    index["candidatePoolVersion"] == "candidate-pool-v1",
                "Selection index provenance mismatch");
        require(index["constructionExclusions"] == pool["constructionExclusions"],
                "Construction audit mismatch");
        const QStringList reasons{"Selected",
                                  "QuotaExceeded",
                                  "AliasEquivalentDuplicate",
                                  "NoDeterministicExactAffectedVersion",
                                  "DuplicateAdvisoryPackageUnit",
                                  "FinalCaptureFailed",
                                  "NotConfirmedFinding",
                                  "FinalStratumIneligible",
                                  "PrivacyRejected",
                                  "DatasetContractRejected",
                                  "DiscoveryFrameLimit"};
        require(index["selectionReasonVocabulary"] == QJsonArray::fromStringList(reasons),
                "Selection reason vocabulary mismatch");
        for (const auto &v : pool["constructionExclusions"].toArray())
            require(reasons.contains(v.toObject()["selectionReason"].toString()), "Invalid exclusion reason");
        const auto start = input.start, ref = input.dataset.referenceTime;
        require(timeValue(pool["frozenAtUtc"]) <= start, "Pool was not frozen before final capture");
        const auto sources = pool["sources"].toArray();
        require(sources.size() == 2, "Missing upstream provenance");
        QHash<QString, QDateTime> sourceTimes;
        for (const auto &v : sources)
        {
            const auto s = v.toObject();
            const auto eco = s["ecosystem"].toString();
            require((eco == "PyPI" || eco == "npm") && !sourceTimes.contains(eco) &&
                        s["sourceIdentity"] ==
                            "https://storage.googleapis.com/osv-vulnerabilities/" + eco + "/all.zip" &&
                        hashString(s["sourceSHA256"]) && s["sourceSizeBytes"].toDouble() > 0,
                    "Missing upstream provenance");
            sourceTimes[eco] = timeValue(s["downloadedAtUtc"]);
            require(sourceTimes[eco] <= start, "Snapshot after capture");
        }
        QHash<QString, CandidateUnit> units;
        QSet<QString> poolClusters;
        QString previousUnit;
        for (const auto &v : pool["candidates"].toArray())
        {
            const auto u = readUnit(v.toObject());
            require(u.has_value(), "Invalid pool candidate");
            require(previousUnit.isEmpty() || previousUnit < u->selectionHash, "Pool ordering mismatch");
            previousUnit = u->selectionHash;
            const auto cluster = arrayHash({u->identity.ecosystem, u->identity.name, u->clusterHash});
            require(!poolClusters.contains(cluster), "Duplicate pool alias cluster");
            poolClusters.insert(cluster);
            require(!units.contains(u->selectionHash), "Duplicate pool candidate");
            units.insert(u->selectionHash, *u);
        }
        require(pool["candidateCount"].toInteger() == units.size(), "Pool count mismatch");
        require(!units.isEmpty(), "Empty pool");
        const auto &epss = input.epss;
        const auto &kev = input.kev;
        QSet<QString> requestedCves;
        for (const auto &u : units)
            for (const auto &c : sortedCveIds(u.representative.cveAliases()))
                requestedCves.insert(c);
        auto auditedCves = input.failedCves;
        for (auto it = epss.cbegin(); it != epss.cend(); ++it)
            auditedCves.insert(it.key());
        require(auditedCves == requestedCves, "Incomplete/unrelated final capture audit");
        QHash<QString, QJsonObject> selected;
        QMap<QString, QStringList> eligible;
        QSet<QString> indexed;
        previousUnit.clear();
        for (const auto &v : index["candidates"].toArray())
        {
            const auto row = v.toObject();
            const auto id = row["selectionHash"].toString();
            require(previousUnit.isEmpty() || previousUnit < id, "Index ordering mismatch");
            previousUnit = id;
            require(units.contains(id) && !indexed.contains(id), "Invalid selection index identity");
            indexed.insert(id);
            const auto &u = units[id];
            auto p = prepareProfile(u, sourceTimes[u.identity.ecosystem], epss, kev, ref);
            const auto stratum = primaryStratum(p, ref);
            bool complete = true;
            for (const auto &c : sortedCveIds(u.representative.cveAliases()))
                complete &= epss.contains(c);
            const bool valid = complete && !stratum.isEmpty();
            require(row["eligible"].isBool() && row["selected"].isBool() && row["eligible"].toBool() == valid,
                    "Capture failure / eligibility mismatch");
            require(row["aliasClusterSHA256"] == u.clusterHash &&
                        row["representativeOsvId"] == u.representative.id() &&
                        row["ecosystem"] == u.identity.ecosystem &&
                        row["canonicalPackageName"] == u.identity.name &&
                        row["exactVersion"] == u.identity.version &&
                        row["aliasClusterIds"] == QJsonArray::fromStringList(u.clusterIds) &&
                        row["cveAliases"] ==
                            QJsonArray::fromStringList(sortedCveIds(u.representative.cveAliases())),
                    "Selection provenance mismatch");
            if (valid)
            {
                require(row["finalCaptureStatus"] == "Success" && row["finalPrimaryStratum"] == stratum,
                        "Stratum mismatch");
                require(row["selectionReason"] == (row["selected"].toBool() ? "Selected" : "QuotaExceeded"),
                        "Selection reason mismatch");
                eligible[stratum].append(id);
            }
            else
                require(!row["selected"].toBool() && row["selectionReason"] == "FinalCaptureFailed" &&
                            row["finalCaptureStatus"] == "Failed" && row["finalPrimaryStratum"] == "",
                        "Failed capture metadata mismatch");
            if (row["selected"].toBool())
                selected.insert(id, row);
        }
        require(indexed.size() == units.size(), "Incomplete selection index");
        QSet<QString> expected;
        const QMap<QString, int> quota{{"S1", 6}, {"S2", 6}, {"S3", 6}, {"S4", 8}, {"S5", 8}, {"S6", 6}};
        for (auto it = eligible.begin(); it != eligible.end(); ++it)
        {
            std::sort(it.value().begin(), it.value().end());
            for (const auto &id : it.value().mid(0, quota[it.key()]))
                expected.insert(id);
        }
        require(expected == QSet<QString>(selected.keyBegin(), selected.keyEnd()),
                "Selection quota/hash mismatch");
        for (const auto &v : o["samples"].toArray())
        {
            const auto s = v.toObject();
            const auto hash = s["selectionHash"].toString();
            require(selected.contains(hash), "Sample not selected by full audit");
            const auto &u = units[hash];
            require(s["unit"] == unitJson(u), "Dataset unit differs from frozen pool");
            require(timeValue(s["profile"].toObject()["key"].toObject()["snapshotFetchedAt"]) ==
                        sourceTimes[u.identity.ecosystem],
                    "Profile snapshot differs from frozen source");
        }
        require(input.dataset.samples.size() == selected.size(), "Dataset coverage gate failed");
        return input.dataset;
    }
    catch (const std::exception &e)
    {
        error = QString::fromUtf8(e.what());
        return {};
    }
}
std::optional<Dataset> loadDirectory(const QString &directory, QString &error)
{
    const QDir d(directory);
    const auto read = [&](const QString &name) { return readFile(d.filePath(name)); };
    return loadDataset(
        read("phase11-validation-dataset-v1.json"), read("phase11-validation-dataset-v1.json.sha256"),
        read("candidate-pool-manifest-v1.json"), read("candidate-pool-manifest-v1.json.sha256"),
        read("candidate-selection-index-v1.json"), read("candidate-selection-index-v1.json.sha256"), error);
}
} // namespace Validation
