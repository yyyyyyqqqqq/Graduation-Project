#pragma once

#include <QFile>

class AppLogger final
{
public:
    enum class Level { Info, Warning, Error };

    bool open(const QString& filePath);
    bool write(Level level, const QString& message);
    QString errorString() const;

private:
    QFile m_file;
};
