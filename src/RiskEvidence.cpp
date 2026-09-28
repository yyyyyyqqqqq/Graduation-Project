#include "RiskEvidence.h"
#include "CveIdentity.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSet>
#include <algorithm>
#include <cmath>

namespace {
bool validKevCveId(const QString& value)
{
    // CISA KEV: CVE-YYYY- followed by 4..19 digits. Keep the shared
    // Phase07/08 alias validator (4+ digits) unchanged outside this boundary.
    return value.size()<=28 && validCveId(value);
}
bool sameApplicability(const ApplicabilityResult& a, const ApplicabilityResult& b)
{
    if (a.state != b.state || a.reason != b.reason || a.queryVersion != b.queryVersion
        || a.rulesVersion != b.rulesVersion || a.evidence.size() != b.evidence.size()) return false;
    for (qsizetype i=0; i<a.evidence.size(); ++i) {
        const auto& x=a.evidence[i]; const auto& y=b.evidence[i];
        if (x.kind!=y.kind || x.state!=y.state || x.reason!=y.reason || x.affectedIndex!=y.affectedIndex
            || x.rangeIndex!=y.rangeIndex || x.wildcard!=y.wildcard || x.detail!=y.detail) return false;
    }
    return true;
}
bool bounded(const QJsonValue& value, int max, bool nonempty=true)
{
    return value.isString() && value.toString().size()<=max
        && (!nonempty || !value.toString().trimmed().isEmpty());
}
bool integer(const QJsonValue& v, qint64 max)
{
    return v.isDouble() && std::isfinite(v.toDouble()) && v.toDouble()>=0
        && v.toDouble()<=max && std::floor(v.toDouble())==v.toDouble();
}
bool date(const QJsonValue& v)
{
    if (!v.isString() || v.toString().size()!=10) return false;
    const auto d=QDate::fromString(v.toString(),Qt::ISODate);
    return d.isValid() && d.toString(Qt::ISODate)==v.toString();
}
std::optional<double> decimal(const QJsonValue& v)
{
    static const QRegularExpression format(QStringLiteral("\\A(?:0|1)(?:\\.[0-9]+)?\\z"));
    if (!bounded(v,64) || !format.match(v.toString()).hasMatch()) return {};
    bool ok=false; const auto n=v.toString().toDouble(&ok);
    return ok && std::isfinite(n) && n>=0 && n<=1 ? std::optional<double>(n) : std::nullopt;
}
QString freshText(EvidenceFreshness f)
{
    switch(f) { case EvidenceFreshness::Fresh:return "Fresh";case EvidenceFreshness::Stale:return "Stale";default:return "NotApplicable"; }
}
QString acquisitionText(EvidenceAcquisition a)
{
    switch(a) {case EvidenceAcquisition::Live:return "Live";case EvidenceAcquisition::Cache:return "Cache";
    case EvidenceAcquisition::StaleFallback:return "StaleFallback";default:return "None";}
}
QString severityText(SeverityStatus s)
{
    switch(s) {case SeverityStatus::Present:return "Present";case SeverityStatus::UnsupportedType:return "UnsupportedSeverityType";
    case SeverityStatus::InvalidStructure:return "InvalidStructure";case SeverityStatus::SchemaConflict:return "SchemaConflict";default:return "Missing";}
}
}
bool RiskEvidenceRequest::validOwnership(const Component& component, const OsvSnapshot& raw,
        const ApplicabilitySnapshot& app, quint64 generation, qsizetype index, const VulnerabilityFinding& f)
{
    if (component.id.isEmpty() || index<0 || index>=raw.candidates.size() || app.candidates.size()!=raw.candidates.size()
        || app.generation!=generation || app.componentId!=component.id || f.componentId!=component.id
        || app.identity!=raw.identity || app.fetchedAt!=raw.fetchedAt || f.snapshotIdentity!=raw.identity
        || f.queryIdentity!=raw.identity || f.fetchedAt!=raw.fetchedAt || !app.candidates[index].hasFinding()
        || f.source!=CandidateSource::OsvPackageVersionQuery || f.osvId!=raw.candidates[index].id()
        || f.cveAliases!=raw.candidates[index].cveAliases()
        || !sameApplicability(f.applicability,*app.candidates[index].published)) return false;
    int matches=0, findings=0;
    for (const auto& c:raw.candidates) if(c.id()==f.osvId) ++matches;
    for (const auto& owned:app.findings)
        if(owned.osvId==f.osvId && owned.componentId==f.componentId && owned.snapshotIdentity==f.snapshotIdentity
            && owned.queryIdentity==f.queryIdentity && owned.fetchedAt==f.fetchedAt && owned.source==f.source
            && owned.cveAliases==f.cveAliases && sameApplicability(owned.applicability,f.applicability)) ++findings;
    return matches==1 && findings==1;
}
SeverityEvidence RiskEvidence::severity(const RiskEvidenceRequest& r) { return severity(r.candidate(),r.finding().applicability); }
SeverityEvidence RiskEvidence::severity(const VulnerabilityCandidate& candidate, const ApplicabilityResult& applicability)
{
    SeverityEvidence result;
    const auto affected=candidate.record["affected"].toArray();
    const bool top=candidate.record.contains("severity");
    bool package=false;
    for(const auto& a:affected) package |= a.toObject().contains("severity");
    if(top && package) {result.status=SeverityStatus::SchemaConflict;return result;}
    QSet<qsizetype> indices;
    for(const auto& e:applicability.evidence) {
        if(e.affectedIndex<0 || e.affectedIndex>=affected.size()) {result.status=SeverityStatus::InvalidStructure;return result;}
        indices.insert(e.affectedIndex);
    }
    bool invalid=false;
    auto extract=[&](QJsonValue value, std::optional<qsizetype> index) {
        if(!value.isArray() || value.toArray().size()>256 || result.items.size()+value.toArray().size()>1024) {invalid=true;return;}
        for(const auto& valueItem:value.toArray()) {
            const auto item=valueItem.toObject();
            SeverityEvidenceItem e;
            e.recordId=candidate.id();e.affectedIndex=index;
            e.scope=index ? SeverityScope::MatchingAffected:SeverityScope::TopLevel;
            if(!valueItem.isObject() || !bounded(item["type"],128) || !bounded(item["score"],4096)
                || (item.contains("source") && !bounded(item["source"],2048,false))) {
                e.status=SeverityStatus::InvalidStructure;invalid=true;
            } else {
                e.type=item["type"].toString();e.vector=item["score"].toString();
                if(item.contains("source")) {e.provenance=SeverityProvenance::ExplicitSource;e.source=item["source"].toString();}
                e.status=(e.type=="CVSS_V2" || e.type=="CVSS_V3" || e.type=="CVSS_V4")
                    ? SeverityStatus::Present:SeverityStatus::UnsupportedType;
            }
            result.items.append(e);
        }
    };
    if(top) extract(candidate.record["severity"],{});
    else {
        auto ordered=indices.values();std::sort(ordered.begin(),ordered.end());
        for(auto i:ordered) if(affected[i].toObject().contains("severity")) extract(affected[i].toObject()["severity"],i);
    }
    if(invalid) result.status=SeverityStatus::InvalidStructure;
    else for(const auto& item:result.items) {
        if(item.status==SeverityStatus::Present) {result.status=SeverityStatus::Present;break;}
        result.status=SeverityStatus::UnsupportedType;
    }
    return result;
}
std::optional<EvidenceDependencyContext> RiskEvidence::dependency(DependencySnapshot snapshot,const QString& id)
{
    int selected=-1, root=-1, roots=0;
    for(int i=0;i<snapshot.components.size();++i) {
        if(snapshot.components[i].id==id) {if(selected!=-1)return {};selected=i;}
        if(snapshot.components[i].sourceRole==ComponentSourceRole::MetadataRoot) {root=i;++roots;}
    }
    if(selected<0) return {};
    EvidenceDependencyContext result;result.captured=snapshot.captured;
    if(!snapshot.captured) return result;
    const auto graph=DependencyGraph::build(std::move(snapshot));
    const auto& metrics=graph.metrics();
    result.referenceResolution=(metrics.missing==0 && metrics.unknown==0 && metrics.ambiguous==0)
        ? ReferenceResolutionCompleteness::Complete:ReferenceResolutionCompleteness::Partial;
    result.directDependents=graph.direct(selected,true).size();
    result.transitiveDependents=graph.transitive(selected,true).size();
    result.root=roots==0?DependencyRootStatus::RootMissing:roots>1?DependencyRootStatus::RootAmbiguous:DependencyRootStatus::RootAvailable;
    if(roots==0) result.path=DependencyPathState::RootMissing;
    else if(roots>1) result.path=DependencyPathState::RootAmbiguous;
    else if(selected==root) {result.path=DependencyPathState::RootComponentSelf;result.depthFromRoot=0;}
    else {
        result.path=DependencyPathState::NoResolvedPath;
        for(const auto& reach:graph.transitive(root)) if(reach.component==selected) {
            result.path=DependencyPathState::ResolvedPathFound;result.depthFromRoot=reach.depth;break;
        }
    }
    return result;
}
QList<QStringList> RiskEvidence::epssChunks(const QStringList& input)
{
    const auto cves=sortedCveIds(input);
    if(cves.size()>MaxAliases) return {};
    QList<QStringList> result;QStringList chunk;qsizetype length=0;
    for(const auto& cve:cves) {
        if(cve.size()>2000) return {};
        if(length+cve.size()+(chunk.isEmpty()?0:1)>2000) {result.append(chunk);chunk.clear();length=0;}
        length+=cve.size()+(chunk.isEmpty()?0:1);chunk.append(cve);
    }
    if(!chunk.isEmpty()) result.append(chunk);
    return result;
}
std::optional<QList<EpssSnapshot>> RiskEvidence::parseEpss(const QByteArray& bytes,const QStringList& requested,QDateTime fetched)
{
    if(bytes.size()>MaxEpssBytes || !fetched.isValid() || requested.isEmpty() || requested.size()>MaxAliases) return {};
    const QSet<QString> wanted(requested.begin(),requested.end());
    if(wanted.size()!=requested.size()) return {};
    for(const auto& c:requested) if(!validCveId(c) || c.size()>2000) return {};
    QJsonParseError error;const auto doc=QJsonDocument::fromJson(bytes,&error);
    if(error.error!=QJsonParseError::NoError || !doc.isObject()) return {};
    const auto o=doc.object();
    if(o["status"]!="OK" || o["status-code"]!=QJsonValue(200) || !bounded(o["version"],128)
        || !integer(o["total"],MaxAliases) || o["offset"]!=QJsonValue(0)
        || !integer(o["limit"],1000000) || o["limit"].toDouble()<requested.size()
        || !o["data"].isArray()) return {};
    const auto data=o["data"].toArray();
    if(data.size()>requested.size() || o["total"].toDouble()!=data.size()) return {};
    QHash<QString,QJsonObject> found;
    for(const auto& value:data) {
        if(!value.isObject()) return {};
        const auto record=value.toObject();const auto cve=record["cve"].toString();
        if(!bounded(record["cve"],2000) || !wanted.contains(cve) || found.contains(cve)
            || !decimal(record["epss"]) || !decimal(record["percentile"]) || !date(record["date"])) return {};
        found.insert(cve,record);
    }
    QList<EpssSnapshot> result;
    for(const auto& cve:requested) result.append({cve,o["version"].toString(),fetched,found.value(cve)});
    return result;
}
std::optional<KevSnapshot> RiskEvidence::parseKev(const QByteArray& bytes,QDateTime fetched)
{
    if(bytes.size()>MaxKevBytes || !fetched.isValid()) return {};
    QJsonParseError error;const auto doc=QJsonDocument::fromJson(bytes,&error);
    if(error.error!=QJsonParseError::NoError || !doc.isObject()) return {};
    const auto o=doc.object();
    if(!bounded(o["catalogVersion"],128) || !bounded(o["dateReleased"],64)
        || !OsvResponseParser::validTimestamp(o["dateReleased"].toString())
        || !integer(o["count"],MaxKevRecords) || !o["vulnerabilities"].isArray()) return {};
    const auto entries=o["vulnerabilities"].toArray();
    // Application completeness check, not a JSON Schema cross-field constraint.
    if(entries.size()>MaxKevRecords || o["count"].toDouble()!=entries.size()) return {};
    KevSnapshot result{fetched,o,{}};
    for(const auto& value:entries) {
        if(!value.isObject()) return {};
        const auto record=value.toObject();
        for(const auto* key:{"cveID","vendorProject","product","vulnerabilityName","shortDescription","requiredAction"})
            if(!bounded(record[QLatin1String(key)],16384)) return {};
        const auto cve=record["cveID"].toString();
        if(!validKevCveId(cve) || result.entries.contains(cve)
            || !date(record["dateAdded"]) || !date(record["dueDate"])
            || (record.contains("knownRansomwareCampaignUse") && !bounded(record["knownRansomwareCampaignUse"],256,false))) return {};
        result.entries.insert(cve,record);
    }
    return result;
}
EvidenceFreshness RiskEvidence::freshness(QDateTime fetched,QDateTime now)
{ return fetched<=now && fetched.secsTo(now)<86400 ? EvidenceFreshness::Fresh:EvidenceFreshness::Stale; }
EpssEvidence RiskEvidence::epssEvidence(const EpssSnapshot& s,EvidenceAcquisition a,QDateTime now)
{
    EpssEvidence e;e.cve=s.cve;e.providerVersion=s.providerVersion;e.fetchedAt=s.fetchedAt;e.acquisition=a;
    e.freshness=a==EvidenceAcquisition::StaleFallback?EvidenceFreshness::Stale:freshness(s.fetchedAt,now);
    e.status=s.record.isEmpty()?EpssStatus::NotScored:EpssStatus::Available;
    if(e.status==EpssStatus::Available) {
        e.probability=decimal(s.record["epss"]);e.percentile=decimal(s.record["percentile"]);
        e.scoreDate=QDate::fromString(s.record["date"].toString(),Qt::ISODate);
    }
    return e;
}
KevEvidence RiskEvidence::kevEvidence(const KevSnapshot& s,const QString& cve,EvidenceAcquisition a,QDateTime now)
{
    KevEvidence e;e.cve=cve;e.acquisition=a;e.fetchedAt=s.fetchedAt;
    e.freshness=a==EvidenceAcquisition::StaleFallback?EvidenceFreshness::Stale:freshness(s.fetchedAt,now);
    e.catalogVersion=s.catalog["catalogVersion"].toString();e.dateReleased=s.catalog["dateReleased"].toString();
    e.entry=s.entries.value(cve);e.status=e.entry.isEmpty()?KevStatus::NotListed:KevStatus::Listed;return e;
}
QString RiskEvidence::operationText(EvidenceOperationState s)
{
    switch(s) {
#define STATE(x) case EvidenceOperationState::x:return QStringLiteral(#x);
    STATE(NotStarted) STATE(Loading) STATE(AwaitingConsent) STATE(Refreshing) STATE(Complete) STATE(Cancelled) STATE(Failed) STATE(Stale)
#undef STATE
    }
    Q_UNREACHABLE();
}
QString RiskEvidence::profileText(const RiskEvidenceProfile& p)
{
    QString text="Risk Evidence Profile Available\nGenerated: "+p.generatedAt.toUTC().toString(Qt::ISODate)+"\n";
    text+="Severity: "+severityText(p.severity.status)+"\n";
    text+="Provider CVSS vector preserved; full CVSS semantic calculation is not performed in Phase 09.\n";
    for(const auto& e:p.severity.items) {
        text+=QString("\nScope: %1 | Type: %2 | Status: %3\nProvider vector: %4\n")
            .arg(e.scope==SeverityScope::TopLevel?"TopLevel":"MatchingAffected",e.type,severityText(e.status),e.vector);
        if(e.affectedIndex) text+=QString("affected[%1]\n").arg(*e.affectedIndex);
        text+=e.provenance==SeverityProvenance::ExplicitSource?"Explicit Source: "+e.source+"\n"
            :"Implicit Home Database\nRecord: "+e.recordId+"\n";
    }
    text+="\nEPSS probability 是未来 30 天观察到在野利用活动的模型概率，不是综合风险评分；本页只展示 Provider Evidence。\n";
    text+="Freshness below is at Profile Generation. Current decision freshness is shown in the Priority assessment.\n";
    for(const auto& e:p.epss) {
        QString status;
        switch(e.status) {case EpssStatus::Available:status="Available";break;case EpssStatus::NotScored:status="NotScored";break;
        case EpssStatus::Failed:status="Failed";break;case EpssStatus::InvalidResponse:status="InvalidResponse";break;default:status="NotQueryable";}
        text+=QString("\nEPSS %1: %2 | %3 | %4\nProbability: %5 | Percentile: %6 | Score Date: %7\nFIRST API Version: %8 | fetchedAt: %9\nError: %10 | Cache: %11\n")
            .arg(e.cve,status,freshText(e.freshness),acquisitionText(e.acquisition),
                 e.probability?QString::number(*e.probability,'g',12):"Unavailable",
                 e.percentile?QString::number(*e.percentile,'g',12):"Unavailable",
                 e.scoreDate.toString(Qt::ISODate),e.providerVersion,e.fetchedAt.toUTC().toString(Qt::ISODate),
                 queryErrorCode(e.error),queryErrorCode(e.cacheError));
    }
    text+="\nKEV: Provider Evidence Only\nCISA 字段是 Catalog provider evidence，不是本系统针对当前用户制定的 SLA、整改期限或强制修复命令。\n";
    for(const auto& e:p.kev) {
        const auto status=e.status==KevStatus::Listed?"Listed":e.status==KevStatus::NotListed
            ?(e.freshness==EvidenceFreshness::Stale?"NotListed in Stale Complete KEV Snapshot":"NotListed in Complete KEV Snapshot")
            :e.status==KevStatus::Unknown?"Unknown":"NotQueryable";
        text+=QString("\nKEV %1: %2 | %3 | %4\nCatalog version: %5 | Date Released: %6 | fetchedAt: %7\nError: %8 | Cache: %9\n")
            .arg(e.cve,status,freshText(e.freshness),acquisitionText(e.acquisition),e.catalogVersion,e.dateReleased,
                 e.fetchedAt.toUTC().toString(Qt::ISODate),queryErrorCode(e.error),queryErrorCode(e.cacheError));
        if(e.status==KevStatus::Listed) text+=QString("Date Added: %1\nCISA Catalog Due Date: %2\nCISA Required Action: %3\nRansomware Campaign Use: %4\n")
            .arg(e.entry["dateAdded"].toString(),e.entry["dueDate"].toString(),e.entry["requiredAction"].toString(),
                e.entry.contains("knownRansomwareCampaignUse")?e.entry["knownRansomwareCampaignUse"].toString():"Unavailable");
    }
    const auto& d=p.dependency;
    QString path;
    switch(d.path) {
    case DependencyPathState::RootComponentSelf:path="RootComponentSelf";break;
    case DependencyPathState::ResolvedPathFound:path="ResolvedPathFound";break;
    case DependencyPathState::NoResolvedPath:path="No Resolved Dependency Path Observed";break;
    case DependencyPathState::RootMissing:path="RootMissing";break;
    case DependencyPathState::RootAmbiguous:path="RootAmbiguous";break;
    case DependencyPathState::NotCaptured:path="NotCaptured";break;
    }
    const auto root=d.root==DependencyRootStatus::RootAvailable?"RootAvailable":d.root==DependencyRootStatus::RootMissing?"RootMissing":
        d.root==DependencyRootStatus::RootAmbiguous?"RootAmbiguous":"NotCaptured";
    text+=QString("\nDependency Context\nCapture Status: %1\nRoot Status: %2\nPath State: %3\nDepth: %4\nDirect Dependents: %5\nTransitive Dependents: %6\nReference Resolution: %7\n")
        .arg(d.captured?"Captured":"NotCaptured",root,path,d.depthFromRoot?QString::number(*d.depthFromRoot):"Unavailable",
            d.directDependents?QString::number(*d.directDependents):"Unavailable",
            d.transitiveDependents?QString::number(*d.transitiveDependents):"Unavailable",
            d.referenceResolution?(*d.referenceResolution==ReferenceResolutionCompleteness::Complete?"Complete":"Partial"):"Unavailable");
    text+="Reference Resolution 只描述已声明引用的解析情况，不证明 dependency coverage 或 runtime reachability。\n";
    text+="\nQuality: Unavailable for persisted current state\n";
    // Display bound only; the complete value profile remains in memory.
    return text.left(262144)+(text.size()>262144?QStringLiteral("\n[显示已截断]"):QString());
}
