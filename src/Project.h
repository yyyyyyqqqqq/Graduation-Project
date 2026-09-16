#pragma once

#include <QString>

struct Project
{
    QString id;
    QString name;
    QString description;
    qint64 createdAt = 0; // UTC Unix epoch milliseconds; UI may display local time.
};
