#include "RiskEvidenceCache.h"
#include "CveIdentity.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QSaveFile>
#ifdef Q_OS_WIN
#include <QThread>
#endif

namespace {
bool safePath(const QString& path)
{
    if(path.isEmpty() || !QDir::isAbsolutePath(path)) return false;
    auto at=QDir::cleanPath(path);
    while(!at.isEmpty()) {
        const QFileInfo info(at);
        if(info.isSymLink() || info.isJunction()) return false;
        const auto parent=info.absolutePath();if(parent==at)break;at=parent;
    }
    return true;
}
QueryError readFile(const QString& path,qsizetype max,QJsonObject& object)
{
    if(!safePath(path))return QueryError::CacheInvalid;
    const QFileInfo info(path);
    if(!info.exists())return QueryError::CacheMiss;
    if(!info.isFile() || info.size()>max)return QueryError::CacheInvalid;
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly))return QueryError::CacheIo;
    const auto bytes=file.read(max+1);
    if(file.error()!=QFileDevice::NoError)return QueryError::CacheIo;
    if(bytes.size()>max)return QueryError::CacheInvalid;
    QJsonParseError error;const auto doc=QJsonDocument::fromJson(bytes,&error);
    if(error.error!=QJsonParseError::NoError || !doc.isObject())return QueryError::CacheInvalid;
    object=doc.object();return QueryError::None;
}
QueryError writeFile(const QString& path,const QJsonObject& object,qsizetype max)
{
    if(!safePath(path))return QueryError::CacheIo;
    const auto bytes=QJsonDocument(object).toJson(QJsonDocument::Compact);
    if(bytes.size()>max)return QueryError::CacheInvalid;
#ifdef Q_OS_WIN
    constexpr int attempts=3;
#else
    constexpr int attempts=1;
#endif
    for(int attempt=0;attempt<attempts;++attempt) {
#ifdef Q_OS_WIN
        // A short-lived Windows handle can deny atomic replacement of an existing file.
        if(attempt>0)QThread::msleep(attempt==1?25:75);
#endif
        if(!safePath(path))return QueryError::CacheIo;
        const QFileInfo info(path);
        if((info.exists() && !info.isFile()) || !QDir().mkpath(info.absolutePath()) || !safePath(path))return QueryError::CacheIo;
        QSaveFile file(path);
        if(!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size())return QueryError::CacheIo;
        if(file.commit())return QueryError::None;
#ifdef Q_OS_WIN
        if(file.error()!=QFileDevice::RenameError)return QueryError::CacheIo;
#endif
    }
    return QueryError::CacheIo;
}
std::optional<QDateTime> metadata(const QJsonObject& o,const char* endpoint,QDateTime now)
{
    const auto text=o["fetchedAt"].toString();
    const auto fetched=QDateTime::fromString(text,Qt::ISODateWithMs);
    if(o["format"]!=QJsonValue(1) || o["endpoint"]!=QLatin1String(endpoint) || o["complete"]!=QJsonValue(true)
        || !OsvResponseParser::validTimestamp(text) || !fetched.isValid() || fetched>now)return {};
    return fetched;
}
QJsonObject envelope(const EpssSnapshot& s)
{
    return {{"status","OK"},{"status-code",200},{"version",s.providerVersion},{"total",s.record.isEmpty()?0:1},
            {"offset",0},{"limit",1},{"data",s.record.isEmpty()?QJsonArray{}:QJsonArray{s.record}}};
}
}
QString EpssCache::filePath(const QString& cve) const
{
    const auto key=QCryptographicHash::hash(cve.toUtf8(),QCryptographicHash::Sha256).toHex();
    return QDir(m_root).filePath("epss-v1/"+QString::fromLatin1(key)+".json");
}
EpssCacheRead EpssCache::read(const QString& cve,QDateTime now) const
{
    if(!validCveId(cve) || cve.size()>2000)return {QueryError::CacheInvalid,{}};
    QJsonObject o;const auto error=readFile(filePath(cve),65536,o);
    if(error!=QueryError::None)return {error,{}};
    const auto fetched=metadata(o,RiskEvidence::EpssEndpoint,now);
    if(!fetched || o["cve"]!=cve || !o["response"].isObject())return {QueryError::CacheInvalid,{}};
    const auto parsed=RiskEvidence::parseEpss(QJsonDocument(o["response"].toObject()).toJson(QJsonDocument::Compact),{cve},*fetched);
    if(!parsed || parsed->size()!=1)return {QueryError::CacheInvalid,{}};
    const auto status=parsed->first().record.isEmpty()?"NotScored":"Available";
    if(o["evidenceStatus"]!=QLatin1String(status))return {QueryError::CacheInvalid,{}};
    return {QueryError::None,parsed->first()};
}
QueryError EpssCache::write(const EpssSnapshot& s) const
{
    const auto raw=envelope(s);
    if(s.fetchedAt>QDateTime::currentDateTimeUtc() ||
        !RiskEvidence::parseEpss(QJsonDocument(raw).toJson(QJsonDocument::Compact),{s.cve},s.fetchedAt))return QueryError::CacheInvalid;
    return writeFile(filePath(s.cve),{{"format",1},{"endpoint",RiskEvidence::EpssEndpoint},{"complete",true},
        {"cve",s.cve},{"evidenceStatus",s.record.isEmpty()?"NotScored":"Available"},
        {"fetchedAt",s.fetchedAt.toUTC().toString(Qt::ISODateWithMs)},{"response",raw}},65536);
}
QString KevCache::filePath() const { return QDir(m_root).filePath("kev-v1/catalog.json"); }
KevCacheRead KevCache::read(QDateTime now) const
{
    QJsonObject o;const auto error=readFile(filePath(),RiskEvidence::MaxKevBytes+4096,o);
    if(error!=QueryError::None)return {error,{}};
    const auto fetched=metadata(o,RiskEvidence::KevEndpoint,now);
    if(!fetched || !o["catalog"].isObject())return {QueryError::CacheInvalid,{}};
    const auto parsed=RiskEvidence::parseKev(QJsonDocument(o["catalog"].toObject()).toJson(QJsonDocument::Compact),*fetched);
    return parsed?KevCacheRead{QueryError::None,parsed}:KevCacheRead{QueryError::CacheInvalid,{}};
}
QueryError KevCache::write(const KevSnapshot& s) const
{
    if(s.fetchedAt>QDateTime::currentDateTimeUtc() ||
        !RiskEvidence::parseKev(QJsonDocument(s.catalog).toJson(QJsonDocument::Compact),s.fetchedAt))return QueryError::CacheInvalid;
    return writeFile(filePath(),{{"format",1},{"endpoint",RiskEvidence::KevEndpoint},{"complete",true},
        {"fetchedAt",s.fetchedAt.toUTC().toString(Qt::ISODateWithMs)},{"catalog",s.catalog}},RiskEvidence::MaxKevBytes+4096);
}
