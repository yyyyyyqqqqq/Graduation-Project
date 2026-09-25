#pragma once

#include "Component.h"
#include <optional>

struct QueryIdentity
{
    QString ecosystem, name, version;
    int rulesVersion = 1;
    bool operator==(const QueryIdentity&) const = default;
};

enum class IdentityState { Resolved, Insufficient, Ambiguous };
enum class VersionSource { None, Purl, Component, Both };
enum class IdentityReason {
    None, MissingPurl, MalformedPurl, PurlTooLong, UnsupportedEcosystem,
    UnsupportedQualifiers, UnsupportedSubpath, MissingVersion, InvalidVersionInput,
    VersionTooLong, VersionConflict
};
struct PackageIdentity
{
    IdentityState state = IdentityState::Insufficient;
    IdentityReason reason = IdentityReason::MissingPurl;
    VersionSource versionSource = VersionSource::None;
    QString ecosystem, name, version;
    bool nameNormalized = false;
    std::optional<QueryIdentity> query() const;
    static PackageIdentity resolve(const Component& component);
    static constexpr qsizetype MaxPurlLength = 4096;
    static constexpr qsizetype MaxVersionLength = 256;
};
QString identityStateText(IdentityState state);
QString identityReasonCode(IdentityReason reason);
QString identityReasonText(IdentityReason reason);
QString versionSourceText(VersionSource source);
