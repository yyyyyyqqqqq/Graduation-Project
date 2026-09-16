#pragma once

#include <QString>

enum class ComponentSourceRole { MetadataRoot = 0, Component = 1 };

struct Component
{
    QString id; // Generated UUID: row identity only; replaced on every successful Apply.
    QString projectId;
    ComponentSourceRole sourceRole = ComponentSourceRole::Component;
    qint64 sourceOrder = 0; // Zero-based PARSER FLATTENED order, not JSON array index/path; root uses 0.
    QString bomRef;
    QString type;
    QString name;
    QString version;
    QString purl;
    bool operator==(const Component&) const = default;
};
