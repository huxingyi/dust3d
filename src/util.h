#ifndef DUST3D_UTIL_H
#define DUST3D_UTIL_H
#include <QString>
#include <map>
#include <cmath>
#include <QVector3D>
#include <QVector2D>
#include <QQuaternion>
#include <set>
#include "positionkey.h"

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
QQuaternion eulerAnglesToQuaternion(double pitch, double yaw, double roll);
void quaternionToEulerAngles(const QQuaternion &q, double *pitch, double *yaw, double *roll);
void quaternionToEulerAnglesXYZ(const QQuaternion &q, double *pitch, double *yaw, double *roll);
bool pointInTriangle(const QVector3D &a, const QVector3D &b, const QVector3D &c, const QVector3D &p);
QVector3D polygonNormal(const std::vector<QVector3D> &vertices, const std::vector<size_t> &polygon);
void angleSmooth(const std::vector<QVector3D> &vertices,
    const std::vector<std::vector<size_t>> &triangles,
    const std::vector<QVector3D> &triangleNormals,
    float thresholdAngleDegrees,
    std::vector<QVector3D> &triangleVertexNormals);
void recoverQuads(const std::vector<QVector3D> &vertices, const std::vector<std::vector<size_t>> &triangles, const std::set<std::pair<PositionKey, PositionKey>> &sharedQuadEdges, std::vector<std::vector<size_t>> &triangleAndQuads);
size_t weldSeam(const std::vector<QVector3D> &sourceVertices, const std::vector<std::vector<size_t>> &sourceTriangles,
    float allowedSmallestDistance, const std::set<PositionKey> &excludePositions,
    std::vector<QVector3D> &destVertices, std::vector<std::vector<size_t>> &destTriangles);
bool isManifold(const std::vector<std::vector<size_t>> &faces);
void trim(std::vector<QVector3D> *vertices, bool normalize=false);
void chamferFace2D(std::vector<QVector2D> *face);
void subdivideFace2D(std::vector<QVector2D> *face);

#endif
