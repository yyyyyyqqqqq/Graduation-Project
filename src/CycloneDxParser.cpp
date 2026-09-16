#include "CycloneDxParser.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <cmath>

namespace
{
SbomParseResult failure(SbomError error) { return {error, {}}; }

bool readString(const QJsonObject& object, const QString& key, QString& output)
{
    const auto value = object.value(key);
    if (value.isUndefined()) return true;
    if (!value.isString()) return false;
    output = value.toString();
    return true;
}

bool readComponent(const QJsonObject& object, SbomComponent& component)
{
    return readString(object, QStringLiteral("bom-ref"), component.bomRef)
        && readString(object, QStringLiteral("type"), component.type)
        && readString(object, QStringLiteral("name"), component.name)
        && readString(object, QStringLiteral("version"), component.version)
        && readString(object, QStringLiteral("purl"), component.purl);
}

SbomError collectComponents(const QJsonValue& value, QList<SbomComponent>& output, int depth)
{
    if (value.isUndefined()) return SbomError::None;
    if (!value.isArray()) return SbomError::InvalidStructure;
    const auto array = value.toArray();
    if (array.isEmpty()) return SbomError::None;
    if (depth > CycloneDxParser::MaxComponentDepth
        || array.size() > CycloneDxParser::MaxEntries - output.size())
        return SbomError::StructureLimitExceeded;
    for (const auto& entry : array) {
        if (!entry.isObject()) return SbomError::InvalidStructure;
        const auto object = entry.toObject();
        SbomComponent component;
        if (!readComponent(object, component)) return SbomError::InvalidStructure;
        if (output.size() >= CycloneDxParser::MaxEntries) return SbomError::StructureLimitExceeded;
        output.append(std::move(component));
        const auto error = collectComponents(object.value(QStringLiteral("components")), output, depth + 1);
        if (error != SbomError::None) return error;
    }
    return SbomError::None;
}
}

SbomParseResult CycloneDxParser::parseFile(const QString& path)
{
    // Reject special devices/directories. Only a user-selected regular local file is supported.
    const QFileInfo info(path);
    if (!info.isFile()) return failure(SbomError::FileOpenError);
    if (info.size() > MaxFileBytes) return failure(SbomError::FileTooLarge);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return failure(SbomError::FileOpenError);
    if (file.isSequential()) return failure(SbomError::FileReadError);
    if (file.size() > MaxFileBytes) return failure(SbomError::FileTooLarge);
    // Bound the read as well, including when the file grows after the size check.
    const auto bytes = file.read(MaxFileBytes + 1);
    if (file.error() != QFileDevice::NoError) return failure(SbomError::FileReadError);
    if (bytes.size() > MaxFileBytes || !file.atEnd()) return failure(SbomError::FileTooLarge);
    return parse(bytes);
}

SbomParseResult CycloneDxParser::parse(const QByteArray& json)
{
    if (json.size() > MaxFileBytes) return failure(SbomError::FileTooLarge);
    QJsonParseError jsonError;
    const auto parsed = QJsonDocument::fromJson(json, &jsonError);
    if (jsonError.error != QJsonParseError::NoError) return failure(SbomError::InvalidJson);
    if (!parsed.isObject()) return failure(SbomError::InvalidStructure);
    const auto root = parsed.object();
    SbomDocument document;
    if (!readString(root, QStringLiteral("bomFormat"), document.bomFormat)
        || document.bomFormat != QStringLiteral("CycloneDX")) return failure(SbomError::NotCycloneDx);
    const auto spec = root.value(QStringLiteral("specVersion"));
    if (spec.isUndefined()) return failure(SbomError::MissingSpecVersion);
    if (!spec.isString()) return failure(SbomError::InvalidStructure);
    document.specVersion = spec.toString();
    if (document.specVersion != QStringLiteral("1.4") && document.specVersion != QStringLiteral("1.5")
        && document.specVersion != QStringLiteral("1.6")) return failure(SbomError::UnsupportedSpecVersion);
    if (!readString(root, QStringLiteral("serialNumber"), document.serialNumber))
        return failure(SbomError::InvalidStructure);
    const auto version = root.value(QStringLiteral("version"));
    if (!version.isUndefined()) {
        // Limit to exact JSON integer values rather than silently truncating or rounding.
        const double number = version.toDouble();
        if (!version.isDouble() || number < 1 || number > 9007199254740991.0 || std::floor(number) != number)
            return failure(SbomError::InvalidStructure);
        document.bomVersion = version.toInteger();
    }
    const auto metadata = root.value(QStringLiteral("metadata"));
    if (!metadata.isUndefined()) {
        if (!metadata.isObject()) return failure(SbomError::InvalidStructure);
        const auto object = metadata.toObject();
        if (!readString(object, QStringLiteral("timestamp"), document.metadata.timestamp))
            return failure(SbomError::InvalidStructure);
        const auto component = object.value(QStringLiteral("component"));
        if (!component.isUndefined()) {
            if (!component.isObject()) return failure(SbomError::InvalidStructure);
            document.metadata.component.emplace();
            if (!readComponent(component.toObject(), *document.metadata.component))
                return failure(SbomError::InvalidStructure);
            const auto error = collectComponents(component.toObject().value(QStringLiteral("components")),
                                                  document.components, 1);
            if (error != SbomError::None) return failure(error);
        }
    }
    const auto componentError = collectComponents(root.value(QStringLiteral("components")), document.components, 1);
    if (componentError != SbomError::None) return failure(componentError);
    const auto dependencies = root.value(QStringLiteral("dependencies"));
    if (!dependencies.isUndefined()) {
        if (!dependencies.isArray()) return failure(SbomError::InvalidStructure);
        const auto array = dependencies.toArray();
        if (array.size() > MaxEntries) return failure(SbomError::StructureLimitExceeded);
        qsizetype targetCount = 0;
        for (const auto& entry : array) {
            if (!entry.isObject()) return failure(SbomError::InvalidStructure);
            const auto object = entry.toObject();
            SbomDependency dependency;
            if (!readString(object, QStringLiteral("ref"), dependency.ref)) return failure(SbomError::InvalidStructure);
            const auto targets = object.value(QStringLiteral("dependsOn"));
            if (!targets.isUndefined()) {
                if (!targets.isArray()) return failure(SbomError::InvalidStructure);
                const auto targetArray = targets.toArray();
                if (targetArray.size() > MaxEntries - targetCount) return failure(SbomError::StructureLimitExceeded);
                targetCount += targetArray.size();
                for (const auto& target : targetArray) {
                    if (!target.isString()) return failure(SbomError::InvalidStructure);
                    dependency.dependsOn.append(target.toString());
                }
            }
            document.dependencies.append(std::move(dependency));
        }
    }
    return {SbomError::None, std::move(document)};
}

QString SbomParseResult::errorCode() const
{
    switch (error) {
    case SbomError::None: return QStringLiteral("None");
    case SbomError::FileOpenError: return QStringLiteral("FileOpenError");
    case SbomError::FileReadError: return QStringLiteral("FileReadError");
    case SbomError::FileTooLarge: return QStringLiteral("FileTooLarge");
    case SbomError::InvalidJson: return QStringLiteral("InvalidJson");
    case SbomError::NotCycloneDx: return QStringLiteral("NotCycloneDx");
    case SbomError::MissingSpecVersion: return QStringLiteral("MissingSpecVersion");
    case SbomError::UnsupportedSpecVersion: return QStringLiteral("UnsupportedSpecVersion");
    case SbomError::InvalidStructure: return QStringLiteral("InvalidStructure");
    case SbomError::StructureLimitExceeded: return QStringLiteral("StructureLimitExceeded");
    }
    return QStringLiteral("Unknown");
}

QString SbomParseResult::userMessage() const
{
    switch (error) {
    case SbomError::None: return {};
    case SbomError::FileOpenError: return QStringLiteral("无法打开 SBOM 文件，请检查文件是否存在及访问权限。");
    case SbomError::FileReadError: return QStringLiteral("无法完整读取 SBOM 文件，请检查文件及存储设备后重试。");
    case SbomError::FileTooLarge: return QStringLiteral("SBOM 文件过大，上限为 %1 MiB。")
            .arg(CycloneDxParser::MaxFileBytes / (1024 * 1024));
    case SbomError::InvalidJson: return QStringLiteral("文件不是有效的 UTF-8 JSON，请检查文件内容。");
    case SbomError::NotCycloneDx: return QStringLiteral("文件不是 CycloneDX JSON：bomFormat 必须为 CycloneDX。");
    case SbomError::MissingSpecVersion: return QStringLiteral("缺少 CycloneDX specVersion 字段。");
    case SbomError::UnsupportedSpecVersion: return QStringLiteral("不支持该 CycloneDX 版本，目前支持 1.4、1.5、1.6。");
    case SbomError::InvalidStructure: return QStringLiteral("SBOM 结构无效，请检查根对象、组件、依赖及字段类型。");
    case SbomError::StructureLimitExceeded: return QStringLiteral("SBOM 结构超出安全上限：组件嵌套最多 %1 层，组件、依赖条目及依赖目标总数各最多 %2。")
            .arg(CycloneDxParser::MaxComponentDepth).arg(CycloneDxParser::MaxEntries);
    }
    return QStringLiteral("SBOM 解析失败。");
}
