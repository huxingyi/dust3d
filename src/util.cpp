#include <cmath>
#include <QtMath>
#include "util.h"
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
        return 360 - degrees;
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

// Sam Hocevar's answer
// https://gamedev.stackexchange.com/questions/98246/quaternion-slerp-and-lerp-implementation-with-overshoot
QQuaternion quaternionOvershootSlerp(const QQuaternion &q0, const QQuaternion &q1, float t)
{
    // If t is too large, divide it by two recursively
    if (t > 1.0) {
        auto tmp = quaternionOvershootSlerp(q0, q1, t / 2);
        return tmp * q0.inverted() * tmp;
    }

    // It’s easier to handle negative t this way
    if (t < 0.0)
        return quaternionOvershootSlerp(q1, q0, 1.0 - t);

    return QQuaternion::slerp(q0, q1, t);
}

float radianBetweenVectors(const QVector3D &first, const QVector3D &second)
{
    return std::acos(QVector3D::dotProduct(first.normalized(), second.normalized()));
};

float angleBetweenVectors(const QVector3D &first, const QVector3D &second)
{
    return radianBetweenVectors(first, second) * 180.0 / M_PI;
}

float areaOfTriangle(const QVector3D &a, const QVector3D &b, const QVector3D &c)
{
    auto ab = b - a;
    auto ac = c - a;
    return 0.5 * QVector3D::crossProduct(ab, ac).length();
}

QQuaternion eulerAnglesToQuaternion(double pitch, double yaw, double roll)
{
    return QQuaternion::fromEulerAngles(pitch, yaw, roll);
}

void quaternionToEulerAngles(const QQuaternion &q, double *pitch, double *yaw, double *roll)
{
    auto eulerAngles = q.toEulerAngles();
    *pitch = eulerAngles.x();
    *yaw = eulerAngles.y();
    *roll = eulerAngles.z();
}
