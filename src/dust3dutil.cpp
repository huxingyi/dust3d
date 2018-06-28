#include <cmath>
#include "dust3dutil.h"

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

void qNormalizeAngle(int &angle)
{
    while (angle < 0)
        angle += 360 * 16;
    while (angle > 360 * 16)
        angle -= 360 * 16;
}

// https://en.wikibooks.org/wiki/Cg_Programming/Unity/Hermite_Curves
QVector3D pointInHermiteCurve(float t, QVector3D p0, QVector3D m0, QVector3D p1, QVector3D m1)
{
    return (2.0f * t * t * t - 3.0f * t * t + 1.0f) * p0
        + (t * t * t - 2.0f * t * t + t) * m0
        + (-2.0f * t * t * t + 3.0f * t * t) * p1
        + (t * t * t - t * t) * m1;
}
