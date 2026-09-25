#include "PackageIdentity.h"
#include <QRegularExpression>
#include <QStringConverter>

namespace {
bool validText(const QString& value)
{
    if (!value.isValidUtf16()) return false;
    for (const auto ch : value) {
        const auto u = ch.unicode();
        if (u < 32 || (u >= 127 && u <= 159)) return false;
    }
    return true;
}
bool decode(const QString& input, QString& output)
{
    QByteArray bytes;
    const auto hex = [](QChar c) -> int {
        if (c >= u'0' && c <= u'9') return c.unicode()-u'0';
        if (c >= u'a' && c <= u'f') return c.unicode()-u'a'+10;
        if (c >= u'A' && c <= u'F') return c.unicode()-u'A'+10;
        return -1;
    };
    for (qsizetype i=0; i<input.size(); ++i) {
        const auto c=input[i];
        if (c==u'%') {
            if (i+2>=input.size() || hex(input[i+1])<0 || hex(input[i+2])<0) return false;
            bytes.append(char(hex(input[i+1])*16+hex(input[i+2]))); i+=2;
        } else {
            // PURL is an ASCII URL; Unicode must be percent-encoded UTF-8.
            if (c.unicode()>127 || c.isSpace()) return false;
            bytes.append(char(c.unicode()));
        }
    }
    QStringDecoder decoder(QStringDecoder::Utf8);
    output=decoder(bytes);
    return !decoder.hasError() && output.isValidUtf16();
}
}

std::optional<QueryIdentity> PackageIdentity::query() const
{
    if (state!=IdentityState::Resolved) return {};
    return QueryIdentity{ecosystem,name,version,1};
}

PackageIdentity PackageIdentity::resolve(const Component& c)
{
    PackageIdentity r;
    const auto fail=[&](IdentityReason reason) { r.reason=reason; return r; };
    const auto& p=c.purl;
    if (p.trimmed().isEmpty()) return r;
    if (p.size()>MaxPurlLength) return fail(IdentityReason::PurlTooLong);
    if (!validText(p) || p!=p.trimmed() || !p.startsWith("pkg:",Qt::CaseInsensitive))
        return fail(IdentityReason::MalformedPurl);
    if (p.contains(u'?')) return fail(IdentityReason::UnsupportedQualifiers);
    if (p.contains(u'#')) return fail(IdentityReason::UnsupportedSubpath);
    const auto slash=p.indexOf(u'/',4);
    if (slash<0) return fail(IdentityReason::MalformedPurl);
    const auto type=p.mid(4,slash-4).toLower();
    static const QRegularExpression typePattern(QStringLiteral("\\A[a-z][a-z0-9.+-]*\\z"));
    if (!typePattern.match(type).hasMatch()) return fail(IdentityReason::MalformedPurl);
    if (type!="pypi" && type!="npm") return fail(IdentityReason::UnsupportedEcosystem);
    r.ecosystem=type=="pypi" ? QStringLiteral("PyPI") : QStringLiteral("npm");
    auto path=p.mid(slash+1);
    QString purlVersion;
    const auto at=path.indexOf(u'@');
    if (at>=0) {
        if (path.indexOf(u'@',at+1)>=0 || at==path.size()-1 || !decode(path.mid(at+1),purlVersion))
            return fail(IdentityReason::MalformedPurl);
        path=path.left(at);
    }
    const auto parts=path.split(u'/');
    if (parts.isEmpty() || parts.size()>2 || (type=="pypi" && parts.size()!=1))
        return fail(IdentityReason::MalformedPurl);
    QString name,scope;
    if (!decode(parts.last(),name) || !validText(name)) return fail(IdentityReason::MalformedPurl);
    if (type=="pypi") {
        static const QRegularExpression pattern(QStringLiteral("\\A[A-Za-z0-9](?:[A-Za-z0-9._-]*[A-Za-z0-9])?\\z"));
        static const QRegularExpression separators(QStringLiteral("[-_.]+"));
        if (!pattern.match(name).hasMatch()) return fail(IdentityReason::MalformedPurl);
        r.name=name.toLower().replace(separators,QStringLiteral("-"));
        r.nameNormalized=r.name!=name;
    } else {
        static const QRegularExpression pattern(QStringLiteral("\\A[A-Za-z0-9][A-Za-z0-9._-]*\\z"));
        if (!pattern.match(name).hasMatch()) return fail(IdentityReason::MalformedPurl);
        if (parts.size()==2) {
            if (!decode(parts.first(),scope) || !scope.startsWith(u'@') || !pattern.match(scope.mid(1)).hasMatch())
                return fail(IdentityReason::MalformedPurl);
            r.name=scope+u'/'+name;
        } else r.name=name;
        if (r.name.size()>214) return fail(IdentityReason::MalformedPurl);
    }
    const bool hasPurl=!purlVersion.trimmed().isEmpty(), hasComponent=!c.version.trimmed().isEmpty();
    if (hasPurl && hasComponent && purlVersion!=c.version) {
        r.state=IdentityState::Ambiguous; return fail(IdentityReason::VersionConflict);
    }
    if (!hasPurl && !hasComponent) return fail(IdentityReason::MissingVersion);
    r.versionSource=hasPurl ? (hasComponent ? VersionSource::Both : VersionSource::Purl) : VersionSource::Component;
    r.version=hasPurl ? purlVersion : c.version;
    if (r.version.size()>MaxVersionLength) return fail(IdentityReason::VersionTooLong);
    if (!validText(r.version)) return fail(IdentityReason::InvalidVersionInput);
    // Deliberately no PEP 440/SemVer normalization or applicability interpretation.
    r.state=IdentityState::Resolved; r.reason=IdentityReason::None;
    return r;
}

QString identityStateText(IdentityState s)
{
    switch(s) {
    case IdentityState::Resolved:return QStringLiteral("Resolved / 可查询身份");
    case IdentityState::Insufficient:return QStringLiteral("Insufficient / 身份不足");
    case IdentityState::Ambiguous:return QStringLiteral("Ambiguous / 身份冲突");
    }
    Q_UNREACHABLE();
}
QString versionSourceText(VersionSource s)
{
    switch(s) {
    case VersionSource::None:return QStringLiteral("—");
    case VersionSource::Purl:return QStringLiteral("PURL");
    case VersionSource::Component:return QStringLiteral("Component");
    case VersionSource::Both:return QStringLiteral("Both");
    }
    Q_UNREACHABLE();
}
QString identityReasonCode(IdentityReason r)
{
    switch(r) {
#define REASON(x) case IdentityReason::x:return QStringLiteral(#x);
    REASON(None) REASON(MissingPurl) REASON(MalformedPurl) REASON(PurlTooLong)
    REASON(UnsupportedEcosystem) REASON(UnsupportedQualifiers) REASON(UnsupportedSubpath)
    REASON(MissingVersion) REASON(InvalidVersionInput) REASON(VersionTooLong) REASON(VersionConflict)
#undef REASON
    }
    Q_UNREACHABLE();
}
QString identityReasonText(IdentityReason r)
{
    switch(r) {
    case IdentityReason::None:return QStringLiteral("身份足以查询；尚未证明版本存在或完成版本适用性判断。");
    case IdentityReason::MissingPurl:return QStringLiteral("缺少 PURL，不能从名称或类型猜测软件包身份。");
    case IdentityReason::MalformedPurl:return QStringLiteral("PURL 结构、包名称或编码不在支持范围内。");
    case IdentityReason::PurlTooLong:return QStringLiteral("PURL 超过 4096 字符上限。");
    case IdentityReason::UnsupportedEcosystem:return QStringLiteral("本阶段仅支持 PyPI 与 npm。");
    case IdentityReason::UnsupportedQualifiers:return QStringLiteral("本阶段不支持 PURL qualifiers，不会静默删除后查询。");
    case IdentityReason::UnsupportedSubpath:return QStringLiteral("本阶段不支持 PURL subpath。");
    case IdentityReason::MissingVersion:return QStringLiteral("PURL 与组件均未提供非空版本。");
    case IdentityReason::InvalidVersionInput:return QStringLiteral("版本包含控制字符或无效 Unicode。");
    case IdentityReason::VersionTooLong:return QStringLiteral("版本超过 256 个 UTF-16 单元上限。");
    case IdentityReason::VersionConflict:return QStringLiteral("PURL 版本与组件版本字符串不同，禁止猜测或查询。");
    }
    Q_UNREACHABLE();
}
