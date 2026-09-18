#include "DependencyAnalyzer.h"

#include <QElapsedTimer>
#include <QHash>
#include <QSet>
#include <QStringView>

namespace {
bool missing(const QString& text) { return QStringView(text).trimmed().isEmpty(); }
}

DependencyGraph DependencyGraph::build(DependencySnapshot snapshot, DependencyBuildTimings* timings)
{
    DependencyGraph graph;
    graph.m_snapshot = std::move(snapshot);
    const auto& data = graph.m_snapshot;
    graph.m_forward.resize(data.components.size());
    graph.m_reverse.resize(data.components.size());
    QElapsedTimer timer;
    timer.start();
    QHash<QString,int> refs;
    refs.reserve(data.components.size());
    for (int i = 0; i < data.components.size(); ++i) {
        const auto& ref = data.components.at(i).bomRef;
        if (missing(ref)) continue;
        auto it = refs.find(ref);
        if (it == refs.end()) refs.insert(ref,i);
        else *it = -1; // Two or more exact rows remain ambiguous, never choose one.
    }
    if (timings) timings->indexNs = timer.nsecsElapsed();
    timer.restart();
    auto& metrics = graph.m_metrics;
    const auto resolve = [&](const QString& ref) -> ReferenceResolution {
        if (missing(ref)) { ++metrics.missing; return {}; }
        const auto it = refs.constFind(ref);
        if (it == refs.cend()) { ++metrics.unknown; return {ReferenceState::Unknown,-1}; }
        if (*it < 0) { ++metrics.ambiguous; return {ReferenceState::Ambiguous,-1}; }
        return {ReferenceState::Resolved,*it};
    };
    metrics.entries = data.entries.size();
    graph.m_resolutions.reserve(data.entries.size());
    for (const auto& entry : data.entries) {
        EntryResolution resolution{resolve(entry.sourceRef),{}};
        resolution.targets.reserve(entry.targets.size());
        metrics.targets += entry.targets.size();
        for (const auto& target : entry.targets) {
            resolution.targets.append(resolve(target.targetRef));
            if (!missing(entry.sourceRef) && !missing(target.targetRef) && entry.sourceRef == target.targetRef)
                ++metrics.selfOccurrences; // Raw occurrence even if resolution is ambiguous/unknown.
        }
        graph.m_resolutions.append(std::move(resolution));
    }
    if (timings) timings->resolutionNs = timer.nsecsElapsed();
    timer.restart();
    QSet<quint64> edges;
    edges.reserve(metrics.targets);
    for (const auto& entry : graph.m_resolutions) {
        if (entry.source.state != ReferenceState::Resolved) continue;
        for (const auto& target : entry.targets) {
            if (target.state != ReferenceState::Resolved) continue;
            const int from = entry.source.component, to = target.component;
            const quint64 key = (quint64(quint32(from)) << 32) | quint32(to);
            if (edges.contains(key)) continue;
            edges.insert(key);
            // Never enumerate the hash set: first resolved occurrence defines both orders.
            graph.m_forward[from].append(to);
            graph.m_reverse[to].append(from);
            ++metrics.uniqueEdges;
        }
    }
    if (timings) timings->adjacencyNs = timer.nsecsElapsed();
    return graph;
}

const QList<int>& DependencyGraph::direct(int component, bool reverse) const
{
    static const QList<int> empty;
    const auto& adjacency = reverse ? m_reverse : m_forward;
    return component >= 0 && component < adjacency.size() ? adjacency.at(component) : empty;
}

QList<DependencyReach> DependencyGraph::transitive(int component, bool reverse) const
{
    QList<DependencyReach> result;
    if (component < 0 || component >= m_forward.size()) return result;
    // One selected origin only; queue order is BFS first discovery order.
    QList<bool> visited(m_forward.size(),false);
    visited[component] = true;
    QList<DependencyReach> queue{{component,0}};
    for (qsizetype head = 0; head < queue.size(); ++head) {
        const auto current = queue.at(head);
        for (const int next : direct(current.component,reverse)) {
            if (visited.at(next)) continue;
            visited[next] = true;
            const DependencyReach reach{next,current.depth+1};
            result.append(reach);
            queue.append(reach);
        }
    }
    return result;
}
