#include "OsvCache.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSaveFile>

namespace {
QJsonObject identityObject(const QueryIdentity& i) {
    return {{"ecosystem",i.ecosystem},{"name",i.name},{"version",i.version},{"identityRulesVersion",i.rulesVersion}};
}
}
QString OsvCache::filePath(const QueryIdentity& i) const {
    const auto key = QJsonDocument(QJsonArray{OsvEndpoint,i.rulesVersion,i.ecosystem,i.name,i.version}).toJson(QJsonDocument::Compact);
    return QDir(m_directory).filePath(QString::fromLatin1(QCryptographicHash::hash(key,QCryptographicHash::Sha256).toHex()) + ".json");
}
bool OsvCache::safeDirectory() const {
    if (m_directory.isEmpty() || !QDir::isAbsolutePath(m_directory)) return false;
    // Never follow a directory link when writing or clearing managed cache files.
    auto dir = QDir::cleanPath(m_directory);
    while (!dir.isEmpty()) {
        const QFileInfo info(dir);
        if (info.isSymLink() || info.isJunction()) return false;
        const auto parent = info.absolutePath();
        if (parent == dir) break;
        dir = parent;
    }
    return true;
}
CacheResult OsvCache::read(const QueryIdentity& i, QDateTime now) const {
    if (!safeDirectory()) return {QueryError::CacheIo,{},false};
    const auto path = filePath(i);
    QFileInfo info(path);
    if (!info.exists()) return {};
    if (!info.isFile() || info.isSymLink() || info.size() > MaxFileBytes) return {QueryError::CacheInvalid,{},false};
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {QueryError::CacheIo,{},false};
    const auto bytes = file.read(MaxFileBytes + 1);
    if (file.error() != QFileDevice::NoError) return {QueryError::CacheIo,{},false};
    if (bytes.size() > MaxFileBytes) return {QueryError::CacheInvalid,{},false};
    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(bytes,&error);
    const auto o = doc.object();
    const auto dateText = o["fetchedAt"].toString();
    const auto date = QDateTime::fromString(dateText,Qt::ISODateWithMs);
    if (error.error != QJsonParseError::NoError || !doc.isObject()
        || o["cacheFormatVersion"] != QJsonValue(1) || o["identityRulesVersion"] != QJsonValue(i.rulesVersion)
        || o["endpoint"] != QJsonValue(OsvEndpoint) || o["queryIdentity"] != QJsonValue(identityObject(i))
        || o["complete"] != QJsonValue(true) || !OsvResponseParser::validTimestamp(dateText)
        || !date.isValid() || !o["vulns"].isArray()) return {QueryError::CacheInvalid,{},false};
    const auto parsed = OsvResponseParser::parse(QJsonDocument(QJsonObject{{"vulns",o["vulns"]}}).toJson(QJsonDocument::Compact));
    if (!parsed.ok()) return {QueryError::CacheInvalid,{},false};
    QList<VulnerabilityCandidate> merged;
    if (OsvResponseParser::merge(merged,parsed.candidates) != QueryError::None) return {QueryError::CacheInvalid,{},false};
    const auto age = date.msecsTo(now);
    return {QueryError::None,OsvSnapshot{i,date,merged},date <= now && age < 24*60*60*1000};
}
QueryError OsvCache::write(const OsvSnapshot& s) const {
    if (!safeDirectory() || !s.fetchedAt.isValid() || s.identity.rulesVersion != 1) return QueryError::CacheIo;
    QJsonArray vulns;
    for (const auto& c : s.candidates) vulns.append(c.record);
    const auto payload = QJsonDocument(QJsonObject{{"vulns",vulns}}).toJson(QJsonDocument::Compact);
    if (!OsvResponseParser::parse(payload).ok()) return QueryError::CacheInvalid;
    const auto bytes = QJsonDocument(QJsonObject{{"cacheFormatVersion",1},{"identityRulesVersion",s.identity.rulesVersion},
        {"endpoint",OsvEndpoint},{"queryIdentity",identityObject(s.identity)},
        {"fetchedAt",s.fetchedAt.toUTC().toString(Qt::ISODateWithMs)},{"complete",true},{"vulns",vulns}}).toJson(QJsonDocument::Compact);
    if (bytes.size() > MaxFileBytes) return QueryError::CacheInvalid;
    if (!QDir().mkpath(m_directory)) return QueryError::CacheIo;
    const QFileInfo info(filePath(s.identity));
    if (info.isSymLink() || (info.exists() && !info.isFile())) return QueryError::CacheIo;
    QSaveFile file(info.absoluteFilePath());
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) return QueryError::CacheIo;
    return QueryError::None;
}
QueryError OsvCache::clear() const {
    if (!safeDirectory()) return QueryError::CacheIo;
    const QDir dir(m_directory);
    if (!dir.exists()) return QueryError::None;
    static const QRegularExpression pattern("\\A[0-9a-f]{64}\\.json\\z");
    for (const auto& info : dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden)) {
        if (!info.isSymLink() && pattern.match(info.fileName()).hasMatch() && !QFile::remove(info.absoluteFilePath()))
            return QueryError::CacheIo;
    }
    return QueryError::None;
}
