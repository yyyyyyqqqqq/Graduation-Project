#pragma once

#include "SbomDocument.h"
#include <QByteArray>

enum class SbomError {
    None, FileOpenError, FileReadError, FileTooLarge, InvalidJson,
    NotCycloneDx, MissingSpecVersion, UnsupportedSpecVersion, InvalidStructure,
    StructureLimitExceeded
};

struct SbomParseResult
{
    SbomError error = SbomError::None;
    SbomDocument document; // Empty on failure; never expose a partially parsed document.
    bool ok() const { return error == SbomError::None; }
    QString userMessage() const;
    QString errorCode() const; // Fixed identifier, safe for logs; no input or file path.
};

// No UI, SQL, logger or retained state. Validates the supported field types, not
// full schema conformance or SBOM quality. Unknown fields are intentionally ignored.
class CycloneDxParser final
{
public:
    static constexpr qint64 MaxFileBytes = 50 * 1024 * 1024;
    static constexpr int MaxComponentDepth = 128;
    static constexpr qsizetype MaxEntries = 100000; // Per components / dependencies / dependency targets.
    static SbomParseResult parse(const QByteArray& json);
    static SbomParseResult parseFile(const QString& path);
};
