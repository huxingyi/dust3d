#include <cmath>
#include "util.h"

QString valueOfKeyInMapOrEmpty(const std::map<QString, QString> &map, const QString &key)
{
    auto it = map.find(key);
    if (it == map.end())
        return QString();
    return it->second;
}

bool isTrueValueString(const QString &str)
{
    return "true" == str || "True" == str || "1" == str;
}

bool isFloatEqual(float a, float b)
{
    return fabs(a - b) <= 0.000001;
}
