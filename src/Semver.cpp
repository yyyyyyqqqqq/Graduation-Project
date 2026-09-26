#include "Semver.h"

namespace {
bool numeric(const QString& value)
{
    if (value.isEmpty()) return false;
    for (const auto c : value) if (c < u'0' || c > u'9') return false;
    return true;
}
bool identifiers(const QString& text, bool prerelease)
{
    for (const auto& part : text.split(u'.')) {
        if (part.isEmpty()) return false;
        for (const auto c : part)
            if (!((c >= u'0' && c <= u'9') || (c >= u'A' && c <= u'Z')
                  || (c >= u'a' && c <= u'z') || c == u'-')) return false;
        if (prerelease && numeric(part) && part.size() > 1 && part.front() == u'0') return false;
    }
    return true;
}
int decimalCompare(const QString& a, const QString& b)
{
    if (a.size() != b.size()) return a.size() < b.size() ? -1 : 1;
    return QString::compare(a, b, Qt::CaseSensitive);
}
}

std::optional<Semver> Semver::parse(const QString& text)
{
    Semver result;
    auto version = text;
    const auto plus = version.indexOf(u'+');
    if (plus >= 0) {
        if (!identifiers(version.mid(plus + 1), false)) return {};
        version.truncate(plus);
    }
    const auto dash = version.indexOf(u'-');
    if (dash >= 0) {
        const auto prerelease = version.mid(dash + 1);
        if (!identifiers(prerelease, true)) return {};
        result.m_prerelease = prerelease.split(u'.');
        version.truncate(dash);
    }
    const auto core = version.split(u'.');
    if (core.size() != 3) return {};
    for (int i = 0; i < 3; ++i) {
        if (!numeric(core[i]) || (core[i].size() > 1 && core[i].front() == u'0')) return {};
        result.m_core[i] = core[i];
    }
    return result;
}

int Semver::compare(const Semver& other) const
{
    for (int i = 0; i < 3; ++i)
        if (const int order = decimalCompare(m_core[i], other.m_core[i])) return order;
    if (m_prerelease.isEmpty() != other.m_prerelease.isEmpty()) return m_prerelease.isEmpty() ? 1 : -1;
    for (qsizetype i = 0; i < qMin(m_prerelease.size(), other.m_prerelease.size()); ++i) {
        const auto& a = m_prerelease[i];
        const auto& b = other.m_prerelease[i];
        const bool an = numeric(a), bn = numeric(b);
        if (an != bn) return an ? -1 : 1;
        const int order = an ? decimalCompare(a, b) : QString::compare(a, b, Qt::CaseSensitive);
        if (order) return order;
    }
    return m_prerelease.size() == other.m_prerelease.size() ? 0
        : m_prerelease.size() < other.m_prerelease.size() ? -1 : 1;
}
