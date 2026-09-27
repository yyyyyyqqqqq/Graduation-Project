#pragma once
#include <QStringList>

// Same strict CVE syntax used by OSV aliases; no case folding or identity guessing.
bool validCveId(const QString&);
QStringList sortedCveIds(const QStringList&);
