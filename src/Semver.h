#pragma once

#include <QStringList>
#include <array>
#include <optional>

// Strict SemVer 2.0 value. Numeric identifiers stay as decimal strings: no
// machine-integer limit, ecosystem normalization or leading-v interpretation.
class Semver final
{
public:
    static std::optional<Semver> parse(const QString& text);
    int compare(const Semver& other) const; // Precedence only; build metadata is ignored.
private:
    std::array<QString, 3> m_core;
    QStringList m_prerelease;
};
