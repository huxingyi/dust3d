#include <cmath>
#include "dust3dutil.h"
#include "version.h"

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

float angleInRangle360BetweenTwoVectors(QVector3D a, QVector3D b, QVector3D planeNormal)
{
    float degrees = acos(QVector3D::dotProduct(a, b)) * 180.0 / M_PI;
    QVector3D direct = QVector3D::crossProduct(a, b);
    if (QVector3D::dotProduct(direct, planeNormal) < 0)
        return 180 + degrees;
    return degrees;
}

QVector3D projectLineOnPlane(QVector3D line, QVector3D planeNormal)
{
    const auto verticalOffset = QVector3D::dotProduct(line, planeNormal) * planeNormal;
    return line - verticalOffset;
}

QString unifiedWindowTitle(const QString &text)
{
    return text + QObject::tr(" - ") + APP_NAME;
}
