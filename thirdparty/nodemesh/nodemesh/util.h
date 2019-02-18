#ifndef NODEMESH_UTIL_H
#define NODEMESH_UTIL_H
#include <QVector3D>
#include <vector>
#include <set>
#include <QString>
#include <nodemesh/positionkey.h>

namespace nodemesh
{

float angleBetween(const QVector3D &v1, const QVector3D &v2);
float radianToDegree(float r);
float degreeBetween(const QVector3D &v1, const QVector3D &v2);
float degreeBetweenIn360(const QVector3D &a, const QVector3D &b, const QVector3D &direct);
bool pointInTriangle(const QVector3D &a, const QVector3D &b, const QVector3D &c, const QVector3D &p);
QVector3D polygonNormal(const std::vector<QVector3D> &vertices, const std::vector<size_t> &polygon);
void triangulate(const std::vector<QVector3D> &vertices, const std::vector<std::vector<size_t>> &faces, std::vector<std::vector<size_t>> &triangles);
void exportMeshAsObj(const std::vector<QVector3D> &vertices, const std::vector<std::vector<size_t>> &faces, const QString &filename, const std::set<size_t> *excludeFacesOfVertices=nullptr);
void exportMeshAsObjWithNormals(const std::vector<QVector3D> &vertices, const std::vector<std::vector<size_t>> &faces, const QString &filename,
    const std::vector<QVector3D> &triangleVertexNormals);
float areaOfTriangle(const QVector3D &a, const QVector3D &b, const QVector3D &c);
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

}

#endif

