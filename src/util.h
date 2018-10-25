#ifndef DUST3D_UTIL_H
#define DUST3D_UTIL_H
#include <QString>
#include <map>
#include <cmath>
#include <QVector3D>
#include <QQuaternion>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

QString valueOfKeyInMapOrEmpty(const std::map<QString, QString> &map, const QString &key);
bool isTrueValueString(const QString &str);
bool isFloatEqual(float a, float b);
void qNormalizeAngle(int &angle);
QVector3D pointInHermiteCurve(float t, QVector3D p0, QVector3D m0, QVector3D p1, QVector3D m1);
float angleInRangle360BetweenTwoVectors(QVector3D a, QVector3D b, QVector3D planeNormal);
QVector3D projectLineOnPlane(QVector3D line, QVector3D planeNormal);
QString unifiedWindowTitle(const QString &text);
QQuaternion quaternionOvershootSlerp(const QQuaternion &q0, const QQuaternion &q1, float t);
float radianBetweenVectors(const QVector3D &first, const QVector3D &second);
float angleBetweenVectors(const QVector3D &first, const QVector3D &second);
float areaOfTriangle(const QVector3D &a, const QVector3D &b, const QVector3D &c);

#endif
