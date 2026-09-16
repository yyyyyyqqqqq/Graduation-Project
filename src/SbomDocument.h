#pragma once

#include <QList>
#include <QStringList>
#include <optional>

struct SbomComponent
{
    QString bomRef;
    QString type;
    QString name;
    QString version;
    QString purl;
};

struct SbomDependency
{
    QString ref;
    QStringList dependsOn;
};

struct SbomMetadata
{
    QString timestamp;
    std::optional<SbomComponent> component;
};

// Session-only values. Nested components are flattened in source order; metadata's
// root is separate from this collection. Containment never implies a dependency.
struct SbomDocument
{
    QString bomFormat;
    QString specVersion;
    QString serialNumber;
    std::optional<qint64> bomVersion;
    SbomMetadata metadata;
    QList<SbomComponent> components;
    QList<SbomDependency> dependencies;
};
