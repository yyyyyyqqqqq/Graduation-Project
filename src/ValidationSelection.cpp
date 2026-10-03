#include "ValidationSelection.h"
#include "CveIdentity.h"
#include <QJsonDocument>
#include <QMap>
#include <QSet>
#include <QUrl>
#include <algorithm>
#include <numeric>

namespace Validation
{
namespace
{
PackageIdentity resolveIdentity(const QString &eco, const QString &name, const QString &version)
{
    Component component;
    component.version = version;
    component.purl = "pkg:" + (eco == "PyPI" ? QString("pypi/") : QString("npm/")) +
                     QString::fromLatin1(QUrl::toPercentEncoding(name, "/"));
    return PackageIdentity::resolve(component);
}
std::optional<QueryIdentity> exactIdentity(const QString &eco, const QString &name, const QString &version)
{
    const auto identity = resolveIdentity(eco, name, version);
    if (identity.state != IdentityState::Resolved || identity.ecosystem != eco ||
        identity.name != name || identity.version != version)
        return {};
    return identity.query();
}
std::optional<QString> packageName(const QString &eco, const QString &name)
{
    const auto identity = resolveIdentity(eco, name, "1");
    if (identity.state != IdentityState::Resolved || identity.ecosystem != eco)
        return {};
    return identity.name;
}
QStringList identifiers(const QJsonObject &record)
{
    QStringList ids{record["id"].toString()};
    for (const auto &a : record["aliases"].toArray())
        ids.append(a.toString());
    ids.removeDuplicates();
    // One comparator throughout: QString UTF-16 code-unit lexicographic order (ASCII IDs).
    std::sort(ids.begin(), ids.end());
    return ids;
}
} // namespace
Discovery discover(const QString &ecosystem, const QJsonArray &records)
{
    Discovery result;
    QMap<QString, QList<VulnerabilityCandidate>> packages;
    QHash<QString, qsizetype> seen;
    for (const auto &value : records)
    {
        const auto record = value.toObject();
        const auto parsed = OsvResponseParser::parse(
            QJsonDocument(QJsonObject{{"vulns", QJsonArray{record}}}).toJson(QJsonDocument::Compact));
        if (!parsed.ok() || parsed.candidates.size() != 1)
        {
            result.exclusions.append(QJsonObject{{"representativeOsvId", record["id"]},
                                                 {"selectionReason", "DatasetContractRejected"}});
            continue;
        }
        QSet<QString> names;
        for (const auto &a : record["affected"].toArray())
        {
            const auto package = a.toObject()["package"].toObject();
            if (package["ecosystem"] != ecosystem)
                continue;
            const auto name = packageName(ecosystem, package["name"].toString());
            if (name)
                names.insert(*name);
        }
        for (const auto &name : names)
        {
            const auto key = arrayHash({ecosystem, name, record["id"]});
            if (seen.contains(key))
            {
                auto &existing = packages[name][seen[key]];
                // Conflicting duplicate IDs also have an input-order-independent winner.
                if (sha256(QJsonDocument(record).toJson(QJsonDocument::Compact)) <
                    sha256(QJsonDocument(existing.record).toJson(QJsonDocument::Compact)))
                    existing = parsed.candidates.first();
                result.exclusions.append(QJsonObject{{"ecosystem", ecosystem},
                                                     {"canonicalPackageName", name},
                                                     {"representativeOsvId", record["id"]},
                                                     {"selectionReason", "DuplicateAdvisoryPackageUnit"}});
            }
            else
            {
                seen.insert(key, packages[name].size());
                packages[name].append(parsed.candidates.first());
            }
        }
    }
    for (auto it = packages.cbegin(); it != packages.cend(); ++it)
    {
        auto rows = it.value();
        std::sort(rows.begin(), rows.end(), [](const auto &a, const auto &b) { return a.id() < b.id(); });
        QList<int> parent(rows.size());
        std::iota(parent.begin(), parent.end(), 0);
        const auto root = [&](int i) {
            while (parent[i] != i)
            {
                parent[i] = parent[parent[i]];
                i = parent[i];
            }
            return i;
        };
        QHash<QString, int> owners;
        for (int i = 0; i < rows.size(); ++i)
            for (const auto &id : identifiers(rows[i].record))
            {
                if (owners.contains(id))
                    parent[root(i)] = root(owners[id]);
                else
                    owners.insert(id, i);
            }
        QMap<int, QList<int>> clusters;
        for (int i = 0; i < rows.size(); ++i)
            clusters[root(i)].append(i);
        for (const auto &members : clusters)
        {
            CandidateUnit unit;
            QStringList ids;
            QString best;
            QJsonArray rejected;
            for (int index : members)
            {
                const auto &candidate = rows[index];
                ids.append(identifiers(candidate.record));
                QSet<QString> versions;
                for (const auto &a : candidate.record["affected"].toArray())
                {
                    const auto entry = a.toObject(), package = entry["package"].toObject();
                    const auto name = packageName(ecosystem, package["name"].toString());
                    if (package["ecosystem"] != ecosystem || !name || *name != it.key())
                        continue;
                    for (const auto &v : entry["versions"].toArray())
                        if (v.isString() && exactIdentity(ecosystem, it.key(), v.toString()))
                            versions.insert(v.toString());
                }
                if (versions.isEmpty())
                    rejected.append(QJsonObject{{"representativeOsvId", candidate.id()},
                                                {"selectionReason", "NoDeterministicExactAffectedVersion"}});
                QList<QPair<QString, QString>> rankedVersions;
                for (const auto &version : versions)
                    rankedVersions.append(
                        {arrayHash({ecosystem, it.key(), candidate.id(), version}), version});
                std::sort(rankedVersions.begin(), rankedVersions.end());
                for (const auto &pair : rankedVersions)
                {
                    const auto &version = pair.second;
                    const auto identity = *exactIdentity(ecosystem, it.key(), version);
                    if (!VersionApplicability::assess(identity, candidate,
                                                      CandidateSource::OsvPackageVersionQuery)
                             .hasFinding())
                    {
                        rejected.append(QJsonObject{{"representativeOsvId", candidate.id()},
                                                    {"exactVersion", version},
                                                    {"selectionReason", "NotConfirmedFinding"}});
                        continue;
                    }
                    const auto &hash = pair.first;
                    if (best.isEmpty() || hash < best)
                    {
                        best = hash;
                        unit.identity = identity;
                        unit.representative = candidate;
                    }
                    break; // Later versions of this record have greater selection hashes.
                }
            }
            ids.removeDuplicates();
            std::sort(ids.begin(), ids.end());
            unit.clusterIds = ids;
            unit.clusterHash = arrayHash(QJsonArray::fromStringList(ids));
            unit.selectionHash = best;
            for (const auto &v : rejected)
            {
                auto e = v.toObject();
                e["ecosystem"] = ecosystem;
                e["canonicalPackageName"] = it.key();
                e["aliasClusterSHA256"] = unit.clusterHash;
                result.exclusions.append(e);
            }
            if (best.isEmpty())
                continue;
            result.units.append(unit);
            for (int index : members)
                if (rows[index].id() != unit.representative.id())
                    result.exclusions.append(QJsonObject{{"ecosystem", ecosystem},
                                                         {"canonicalPackageName", it.key()},
                                                         {"representativeOsvId", rows[index].id()},
                                                         {"aliasClusterSHA256", unit.clusterHash},
                                                         {"selectionReason", "AliasEquivalentDuplicate"}});
        }
    }
    std::sort(result.units.begin(), result.units.end(),
              [](const auto &a, const auto &b) { return a.selectionHash < b.selectionHash; });
    // Audit order is independent of QSet iteration too.
    auto exclusions = result.exclusions.toVariantList();
    std::sort(exclusions.begin(), exclusions.end(), [](const auto &a, const auto &b) {
        return QJsonDocument::fromVariant(a).toJson(QJsonDocument::Compact) <
               QJsonDocument::fromVariant(b).toJson(QJsonDocument::Compact);
    });
    result.exclusions = QJsonArray::fromVariantList(exclusions);
    return result;
}
QJsonObject unitJson(const CandidateUnit &u)
{
    return {{"ecosystem", u.identity.ecosystem},
            {"canonicalPackageName", u.identity.name},
            {"exactVersion", u.identity.version},
            {"representativeOsvId", u.representative.id()},
            {"record", u.representative.record},
            {"aliasClusterIds", QJsonArray::fromStringList(u.clusterIds)},
            {"aliasClusterSHA256", u.clusterHash},
            {"cveAliases", QJsonArray::fromStringList(sortedCveIds(u.representative.cveAliases()))},
            {"selectionHash", u.selectionHash}};
}
std::optional<CandidateUnit> readUnit(const QJsonObject &o)
{
    CandidateUnit u;
    const auto identity = exactIdentity(o["ecosystem"].toString(), o["canonicalPackageName"].toString(),
                                        o["exactVersion"].toString());
    if (!identity)
        return {};
    u.identity = *identity;
    u.representative = {o["record"].toObject()};
    const auto parsed =
        OsvResponseParser::parse(QJsonDocument(QJsonObject{{"vulns", QJsonArray{u.representative.record}}})
                                     .toJson(QJsonDocument::Compact));
    if (!parsed.ok() || parsed.candidates.size() != 1)
        return {};
    for (const auto &v : o["aliasClusterIds"].toArray())
        u.clusterIds.append(v.toString());
    u.clusterHash = o["aliasClusterSHA256"].toString();
    u.selectionHash = o["selectionHash"].toString();
    auto sorted = u.clusterIds;
    sorted.removeDuplicates();
    std::sort(sorted.begin(), sorted.end());
    if (sorted != u.clusterIds || sorted.isEmpty() || !sorted.contains(u.representative.id()) ||
        arrayHash(QJsonArray::fromStringList(sorted)) != u.clusterHash ||
        arrayHash({u.identity.ecosystem, u.identity.name, u.representative.id(), u.identity.version}) !=
            u.selectionHash ||
        o["representativeOsvId"] != u.representative.id() ||
        o["cveAliases"] != QJsonArray::fromStringList(sortedCveIds(u.representative.cveAliases())))
        return {};
    for (const auto &id : identifiers(u.representative.record))
        if (!sorted.contains(id))
            return {};
    const auto canonical = packageName(u.identity.ecosystem, u.identity.name);
    if (!canonical || *canonical != u.identity.name)
        return {};
    bool exact = false;
    for (const auto &v : u.representative.record["affected"].toArray())
    {
        const auto a = v.toObject(), p = a["package"].toObject();
        const auto n = packageName(u.identity.ecosystem, p["name"].toString());
        if (p["ecosystem"] == u.identity.ecosystem && n && *n == u.identity.name &&
            a["versions"].toArray().contains(u.identity.version))
            exact = true;
    }
    if (!exact ||
        !VersionApplicability::assess(u.identity, u.representative, CandidateSource::OsvPackageVersionQuery)
             .hasFinding())
        return {};
    return u;
}
RiskEvidenceProfile prepareProfile(const CandidateUnit &u, QDateTime snapshotTime,
                                   const QHash<QString, EpssSnapshot> &epss,
                                   const std::optional<KevSnapshot> &kev, QDateTime generatedAt)
{
    RiskEvidenceProfile p;
    // A research-only synthetic component identity; never read a user's DB or project UUID.
    p.key = {"validation-" + u.selectionHash, u.representative.id(), u.identity, snapshotTime, 1};
    p.generatedAt = generatedAt;
    const auto app =
        VersionApplicability::assess(u.identity, u.representative, CandidateSource::OsvPackageVersionQuery);
    if (app.hasFinding())
        p.severity = RiskEvidence::severity(u.representative, *app.published);
    // No public SBOM is claimed. This is the normal production NotCaptured/QualityUnavailable context.
    const auto cves = sortedCveIds(u.representative.cveAliases());
    if (cves.isEmpty())
    {
        EpssEvidence e;
        e.status = EpssStatus::NotQueryable;
        p.epss.append(e);
        KevEvidence k;
        k.status = KevStatus::NotQueryable;
        p.kev.append(k);
    }
    for (const auto &c : cves)
    {
        if (epss.contains(c))
            p.epss.append(RiskEvidence::epssEvidence(epss[c], EvidenceAcquisition::Live, generatedAt));
        else
        {
            EpssEvidence e;
            e.cve = c;
            e.error = QueryError::ConnectionFailure;
            p.epss.append(e);
        }
        if (kev)
            p.kev.append(RiskEvidence::kevEvidence(*kev, c, EvidenceAcquisition::Live, generatedAt));
        else
        {
            KevEvidence k;
            k.cve = c;
            k.error = QueryError::ConnectionFailure;
            p.kev.append(k);
        }
    }
    return p;
}
} // namespace Validation
