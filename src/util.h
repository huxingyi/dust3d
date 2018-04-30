#ifndef UTIL_H
#define UTIL_H
#include <QString>
#include <map>
#include <cmath>
#include <QVector3D>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

QString valueOfKeyInMapOrEmpty(const std::map<QString, QString> &map, const QString &key);
bool isTrueValueString(const QString &str);
bool isFloatEqual(float a, float b);
void qNormalizeAngle(int &angle);

#endif
