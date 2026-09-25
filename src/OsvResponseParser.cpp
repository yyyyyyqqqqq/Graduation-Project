#include "OsvResponseParser.h"
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>

namespace {
bool strings(const QJsonValue& value)
{
    if (!value.isArray()) return false;
    for (const auto& item:value.toArray()) if (!item.isString()) return false;
    return true;
}
bool optionalString(const QJsonObject& o, const QString& key)
{ return !o.contains(key) || o.value(key).isString(); }
bool affectedValid(const QJsonValue& value)
{
    if (!value.isArray()) return false;
    for (const auto& item:value.toArray()) {
        if (!item.isObject()) return false;
        const auto o=item.toObject();
        if (o.contains("package")) {
            if (!o["package"].isObject()) return false;
            const auto p=o["package"].toObject();
            for (const auto* key:{"ecosystem","name","purl"}) if (!optionalString(p,QLatin1String(key))) return false;
        }
        if (o.contains("versions") && !strings(o["versions"])) return false;
        for (const auto* key:{"ecosystem_specific","database_specific"})
            if (o.contains(QLatin1String(key)) && !o[QLatin1String(key)].isObject()) return false;
        if (o.contains("ranges")) {
            if (!o["ranges"].isArray()) return false;
            for (const auto& range:o["ranges"].toArray()) {
                if (!range.isObject()) return false;
                const auto r=range.toObject();
                if (!optionalString(r,"type") || !optionalString(r,"repo")) return false;
                if (r.contains("events")) {
                    if (!r["events"].isArray()) return false;
                    for (const auto& event:r["events"].toArray()) {
                        if (!event.isObject()) return false;
                        for (const auto* key:{"introduced","fixed","last_affected","limit"})
                            if (!optionalString(event.toObject(),QLatin1String(key))) return false;
                    }
                }
            }
        }
    }
    return true;
}
}

QStringList VulnerabilityCandidate::cveAliases() const
{
    static const QRegularExpression pattern(QStringLiteral("\\ACVE-[0-9]{4}-[0-9]{4,}\\z"));
    QStringList result;
    for (const auto& alias:record.value("aliases").toArray())
        if (pattern.match(alias.toString()).hasMatch()) result.append(alias.toString());
    return result;
}
bool OsvResponseParser::validTimestamp(const QString& value)
{
    static const QRegularExpression pattern(QStringLiteral("\\A[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}(?:\\.[0-9]+)?Z\\z"));
    return value.size()<=64 && pattern.match(value).hasMatch()
        && QDateTime::fromString(value.left(19)+u'Z',Qt::ISODate).isValid();
}
int OsvResponseParser::compareTimestamp(const QString& a,const QString& b)
{
    const int seconds=QString::compare(a.left(19),b.left(19));
    if (seconds) return seconds;
    const auto fraction=[](const QString& s) { return s.size()>20 ? s.mid(20,s.size()-21) : QString(); };
    auto af=fraction(a),bf=fraction(b);
    const auto size=qMax(af.size(),bf.size());
    return QString::compare(af.leftJustified(size,u'0'),bf.leftJustified(size,u'0'));
}
OsvPageResult OsvResponseParser::parse(const QByteArray& bytes)
{
    const auto invalid=[] { return OsvPageResult{QueryError::ResponseInvalid,{},{}}; };
    if (bytes.size()>MaxResponseBytes) return {QueryError::ResponseLimitExceeded,{},{}};
    QJsonParseError error;
    const auto document=QJsonDocument::fromJson(bytes,&error);
    if (error.error!=QJsonParseError::NoError || !document.isObject()) return invalid();
    const auto object=document.object();
    if (!optionalString(object,"next_page_token")) return invalid();
    OsvPageResult result;
    result.nextToken=object.value("next_page_token").toString();
    if (result.nextToken.size()>16384) return {QueryError::ResponseLimitExceeded,{},{}};
    if (!object.contains("vulns")) return result;
    if (!object["vulns"].isArray()) return invalid();
    const auto array=object["vulns"].toArray();
    if (array.size()>MaxRecords) return {QueryError::ResponseLimitExceeded,{},{}};
    static const QRegularExpression schema(QStringLiteral("\\A1\\.[0-9]+\\.[0-9]+\\z"));
    for (const auto& item:array) {
        if (!item.isObject()) return invalid();
        const auto o=item.toObject();
        if (!o["id"].isString() || o["id"].toString().trimmed().isEmpty() || o["id"].toString().size()>512
            || !o["modified"].isString() || !validTimestamp(o["modified"].toString())) return invalid();
        for (const auto c:o["id"].toString()) if (c.isSpace() || c.category()==QChar::Other_Control) return invalid();
        for (const auto* key:{"summary","details","schema_version"}) if (!optionalString(o,QLatin1String(key))) return invalid();
        if (o.contains("schema_version") && !schema.match(o["schema_version"].toString()).hasMatch()) return invalid();
        for (const auto* key:{"published","withdrawn"})
            if (o.contains(QLatin1String(key)) && (!o[QLatin1String(key)].isString() || !validTimestamp(o[QLatin1String(key)].toString()))) return invalid();
        if (o.contains("aliases") && !strings(o["aliases"])) return invalid();
        if (o.contains("affected") && !affectedValid(o["affected"])) return invalid();
        result.candidates.append({o});
    }
    return result;
}
QueryError OsvResponseParser::merge(QList<VulnerabilityCandidate>& current,const QList<VulnerabilityCandidate>& page)
{
    // Candidate copy is intentional: a conflict never exposes a partial merge.
    auto candidate=current;
    QHash<QString,qsizetype> positions;
    for (qsizetype i=0;i<candidate.size();++i) positions.insert(candidate[i].id(),i);
    for (const auto& value:page) {
        const auto found=positions.constFind(value.id());
        if (found==positions.cend()) { positions.insert(value.id(),candidate.size()); candidate.append(value); }
        else {
            auto& old=candidate[*found];
            const int order=compareTimestamp(value.text("modified"),old.text("modified"));
            if (order>0) old=value;
            else if (order==0 && value.record!=old.record) return QueryError::ResponseInvalid;
        }
    }
    current=std::move(candidate);
    return QueryError::None;
}
QString queryErrorCode(QueryError e)
{
    switch(e) {
#define ERROR_CODE(x) case QueryError::x:return QStringLiteral(#x);
    ERROR_CODE(None) ERROR_CODE(RequestRejected) ERROR_CODE(AccessDenied) ERROR_CODE(EndpointNotFound)
    ERROR_CODE(Timeout) ERROR_CODE(TlsFailure) ERROR_CODE(ConnectionFailure) ERROR_CODE(RateLimited)
    ERROR_CODE(ServiceUnavailable) ERROR_CODE(ResponseInvalid) ERROR_CODE(ResponseLimitExceeded)
    ERROR_CODE(UnexpectedRedirect) ERROR_CODE(Cancelled) ERROR_CODE(CacheMiss) ERROR_CODE(CacheInvalid)
    ERROR_CODE(CacheIo) ERROR_CODE(Database)
#undef ERROR_CODE
    }
    Q_UNREACHABLE();
}
QString queryErrorText(QueryError e)
{
    switch(e) {
    case QueryError::None:return {};
    case QueryError::RequestRejected:return QStringLiteral("OSV 拒绝请求；身份仍保持原判断，未修改版本。");
    case QueryError::AccessDenied:return QStringLiteral("OSV 访问被拒绝。");
    case QueryError::EndpointNotFound:return QStringLiteral("OSV 请求地址不可用。");
    case QueryError::Timeout:return QStringLiteral("查询超时，请稍后主动重试。");
    case QueryError::TlsFailure:return QStringLiteral("TLS 连接或证书验证失败，未绕过验证。");
    case QueryError::ConnectionFailure:return QStringLiteral("无法连接 OSV，请检查网络。");
    case QueryError::RateLimited:return QStringLiteral("OSV 限流，请等待后重试。");
    case QueryError::ServiceUnavailable:return QStringLiteral("OSV 服务暂不可用。");
    case QueryError::ResponseInvalid:return QStringLiteral("OSV 响应格式无效或记录冲突，本次结果不完整。");
    case QueryError::ResponseLimitExceeded:return QStringLiteral("响应超过分页、大小或记录限制，本次结果不完整。");
    case QueryError::UnexpectedRedirect:return QStringLiteral("拒绝非预期重定向，未向其他地址转发身份。");
    case QueryError::Cancelled:return QStringLiteral("已取消，本次未产生完整查询结果。");
    case QueryError::CacheMiss:return QStringLiteral("没有此查询身份的本地缓存。");
    case QueryError::CacheInvalid:return QStringLiteral("本地缓存损坏或格式不受支持。");
    case QueryError::CacheIo:return QStringLiteral("本地缓存读写或清理失败，请检查磁盘与权限。");
    case QueryError::Database:return QStringLiteral("无法读取当前组件，请重新加载项目。");
    }
    Q_UNREACHABLE();
}
