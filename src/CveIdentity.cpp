#include "CveIdentity.h"
#include <QRegularExpression>
#include <QSet>
#include <algorithm>

bool validCveId(const QString& value)
{
    static const QRegularExpression pattern(QStringLiteral("\\ACVE-[0-9]{4}-[0-9]{4,}\\z"));
    return pattern.match(value).hasMatch();
}
QStringList sortedCveIds(const QStringList& input)
{
    QSet<QString> unique;
    for (const auto& value : input) if (validCveId(value)) unique.insert(value);
    auto result = unique.values();
    std::sort(result.begin(), result.end());
    return result;
}
