#include "AppLogger.h"

#include <QDateTime>

#include <cstdio>

bool AppLogger::open(const QString& filePath)
{
    m_file.close();
    m_file.setFileName(filePath);
    return m_file.open(QIODevice::WriteOnly | QIODevice::Append);
}

bool AppLogger::write(Level level, const QString& message)
{
    const char* label = level == Level::Info ? "INFO" : level == Level::Warning ? "WARN" : "ERROR";
    QString oneLine = message;
    oneLine.replace(u'\r', u' ');
    oneLine.replace(u'\n', u' ');
    const QByteArray record = QStringLiteral("%1 [%2] %3\n")
        .arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs),
             QString::fromLatin1(label), oneLine).toUtf8();
    if (!m_file.isOpen() || m_file.write(record) != record.size() || !m_file.flush()) {
        // No Qt logging here: reporting a logger failure must not recurse.
        std::fputs("Application log unavailable: ", stderr);
        std::fwrite(record.constData(), 1, static_cast<size_t>(record.size()), stderr);
        return false;
    }
    return true;
}

QString AppLogger::errorString() const
{
    return m_file.errorString();
}
