#ifndef UTIL_H
#define UTIL_H
#include <QString>
#include <map>

QString valueOfKeyInMapOrEmpty(const std::map<QString, QString> &map, const QString &key);
bool isTrueValueString(const QString &str);
bool isFloatEqual(float a, float b);

#endif
