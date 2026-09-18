#pragma once

#include "Component.h"
#include <QList>

struct DependencyTarget
{
    QString id;
    QString dependencyEntryId;
    qint64 targetOrder = 0;
    QString targetRef;
    bool operator==(const DependencyTarget&) const = default;
};

struct DependencyEntry
{
    QString id;
    QString projectId;
    qint64 sourceOrder = 0;
    QString sourceRef;
    QList<DependencyTarget> targets; // Empty dependsOn still has a real entry.
    bool operator==(const DependencyEntry&) const = default;
};

// One consistent read transaction; all collections retain their stored source order.
// No derived component UUID edges or analysis results are persisted.
struct DependencySnapshot
{
    QList<Component> components;
    bool captured = false;
    QList<DependencyEntry> entries;
    bool operator==(const DependencySnapshot&) const = default;
};
