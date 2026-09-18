#pragma once

#include "DependencySnapshot.h"

enum class ReferenceState { Missing, Unknown, Resolved, Ambiguous };
struct ReferenceResolution
{
    ReferenceState state = ReferenceState::Missing;
    int component = -1; // Index into this graph's immutable snapshot, only valid when Resolved.
    bool operator==(const ReferenceResolution&) const = default;
};
struct EntryResolution
{
    ReferenceResolution source;
    QList<ReferenceResolution> targets;
    bool operator==(const EntryResolution&) const = default;
};
struct DependencyMetrics
{
    qint64 entries = 0, targets = 0;
    qint64 missing = 0, unknown = 0, ambiguous = 0, selfOccurrences = 0;
    qint64 uniqueEdges = 0;
    bool operator==(const DependencyMetrics&) const = default;
};
struct DependencyReach
{
    int component = -1;
    int depth = 0;
    bool operator==(const DependencyReach&) const = default;
};
struct DependencyBuildTimings
{
    qint64 indexNs = 0, resolutionNs = 0, adjacencyNs = 0;
};

// Immutable after construction; SQLite remains authoritative. Only O(V+E) graph
// state is retained. No all-pairs closures, identity fallback, UI, SQL or logger.
class DependencyGraph final
{
public:
    static DependencyGraph build(DependencySnapshot snapshot, DependencyBuildTimings* timings = nullptr);
    const DependencySnapshot& snapshot() const { return m_snapshot; }
    const QList<EntryResolution>& resolutions() const { return m_resolutions; }
    const DependencyMetrics& metrics() const { return m_metrics; }
    const QList<int>& direct(int component, bool reverse = false) const;
    // BFS includes direct neighbors, excludes the origin (even through cycles).
    QList<DependencyReach> transitive(int component, bool reverse = false) const;
private:
    DependencySnapshot m_snapshot;
    QList<EntryResolution> m_resolutions;
    DependencyMetrics m_metrics;
    QList<QList<int>> m_forward, m_reverse;
};
