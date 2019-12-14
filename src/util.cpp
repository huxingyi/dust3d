#include <cmath>
#include <QtMath>
#include <unordered_set>
#include <unordered_map>
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

QVector3D polygonNormal(const std::vector<QVector3D> &vertices, const std::vector<size_t> &polygon)
{
    QVector3D normal;
    for (size_t i = 0; i < polygon.size(); ++i) {
        auto j = (i + 1) % polygon.size();
        auto k = (i + 2) % polygon.size();
        const auto &enter = vertices[polygon[i]];
        const auto &cone = vertices[polygon[j]];
        const auto &leave = vertices[polygon[k]];
        normal += QVector3D::normal(enter, cone, leave);
    }
    return normal.normalized();
}

bool pointInTriangle(const QVector3D &a, const QVector3D &b, const QVector3D &c, const QVector3D &p)
{
    auto u = b - a;
    auto v = c - a;
    auto w = p - a;
    auto vXw = QVector3D::crossProduct(v, w);
    auto vXu = QVector3D::crossProduct(v, u);
    if (QVector3D::dotProduct(vXw, vXu) < 0.0) {
        return false;
    }
    auto uXw = QVector3D::crossProduct(u, w);
    auto uXv = QVector3D::crossProduct(u, v);
    if (QVector3D::dotProduct(uXw, uXv) < 0.0) {
        return false;
    }
    auto denom = uXv.length();
    auto r = vXw.length() / denom;
    auto t = uXw.length() / denom;
    return r + t <= 1.0;
}

void angleSmooth(const std::vector<QVector3D> &vertices,
    const std::vector<std::vector<size_t>> &triangles,
    const std::vector<QVector3D> &triangleNormals,
    float thresholdAngleDegrees,
    std::vector<QVector3D> &triangleVertexNormals)
{
    std::vector<std::vector<std::pair<size_t, size_t>>> triangleVertexNormalsMapByIndices(vertices.size());
    std::vector<QVector3D> angleAreaWeightedNormals;
    for (size_t triangleIndex = 0; triangleIndex < triangles.size(); ++triangleIndex) {
        const auto &sourceTriangle = triangles[triangleIndex];
        if (sourceTriangle.size() != 3) {
            qDebug() << "Encounter non triangle";
            continue;
        }
        const auto &v1 = vertices[sourceTriangle[0]];
        const auto &v2 = vertices[sourceTriangle[1]];
        const auto &v3 = vertices[sourceTriangle[2]];
        float area = areaOfTriangle(v1, v2, v3);
        float angles[] = {angleBetweenVectors(v2-v1, v3-v1),
            angleBetweenVectors(v1-v2, v3-v2),
            angleBetweenVectors(v1-v3, v2-v3)};
        for (int i = 0; i < 3; ++i) {
            if (sourceTriangle[i] >= vertices.size()) {
                qDebug() << "Invalid vertex index" << sourceTriangle[i] << "vertices size" << vertices.size();
                continue;
            }
            triangleVertexNormalsMapByIndices[sourceTriangle[i]].push_back({triangleIndex, angleAreaWeightedNormals.size()});
            angleAreaWeightedNormals.push_back(triangleNormals[triangleIndex] * area * angles[i]);
        }
    }
    triangleVertexNormals = angleAreaWeightedNormals;
    std::map<std::pair<size_t, size_t>, float> degreesBetweenFacesMap;
    for (size_t vertexIndex = 0; vertexIndex < vertices.size(); ++vertexIndex) {
        const auto &triangleVertices = triangleVertexNormalsMapByIndices[vertexIndex];
        for (const auto &triangleVertex: triangleVertices) {
            for (const auto &otherTriangleVertex: triangleVertices) {
                if (triangleVertex.first == otherTriangleVertex.first)
                    continue;
                float degrees = 0;
                auto findDegreesResult = degreesBetweenFacesMap.find({triangleVertex.first, otherTriangleVertex.first});
                if (findDegreesResult == degreesBetweenFacesMap.end()) {
                    degrees = angleBetweenVectors(triangleNormals[triangleVertex.first], triangleNormals[otherTriangleVertex.first]);
                    degreesBetweenFacesMap.insert({{triangleVertex.first, otherTriangleVertex.first}, degrees});
                    degreesBetweenFacesMap.insert({{otherTriangleVertex.first, triangleVertex.first}, degrees});
                } else {
                    degrees = findDegreesResult->second;
                }
                if (degrees > thresholdAngleDegrees) {
                    continue;
                }
                triangleVertexNormals[triangleVertex.second] += angleAreaWeightedNormals[otherTriangleVertex.second];
            }
        }
    }
    for (auto &item: triangleVertexNormals)
        item.normalize();
}

void recoverQuads(const std::vector<QVector3D> &vertices, const std::vector<std::vector<size_t>> &triangles, const std::set<std::pair<PositionKey, PositionKey>> &sharedQuadEdges, std::vector<std::vector<size_t>> &triangleAndQuads)
{
    std::vector<PositionKey> verticesPositionKeys;
    for (const auto &position: vertices) {
        verticesPositionKeys.push_back(PositionKey(position));
    }
    std::map<std::pair<size_t, size_t>, std::pair<size_t, size_t>> triangleEdgeMap;
    for (size_t i = 0; i < triangles.size(); i++) {
        const auto &faceIndices = triangles[i];
        if (faceIndices.size() == 3) {
            triangleEdgeMap[std::make_pair(faceIndices[0], faceIndices[1])] = std::make_pair(i, faceIndices[2]);
            triangleEdgeMap[std::make_pair(faceIndices[1], faceIndices[2])] = std::make_pair(i, faceIndices[0]);
            triangleEdgeMap[std::make_pair(faceIndices[2], faceIndices[0])] = std::make_pair(i, faceIndices[1]);
        }
    }
    std::unordered_set<size_t> unionedFaces;
    std::vector<std::vector<size_t>> newUnionedFaceIndices;
    for (const auto &edge: triangleEdgeMap) {
        if (unionedFaces.find(edge.second.first) != unionedFaces.end())
            continue;
        auto pair = std::make_pair(verticesPositionKeys[edge.first.first], verticesPositionKeys[edge.first.second]);
        if (sharedQuadEdges.find(pair) != sharedQuadEdges.end()) {
            auto oppositeEdge = triangleEdgeMap.find(std::make_pair(edge.first.second, edge.first.first));
            if (oppositeEdge == triangleEdgeMap.end()) {
                qDebug() << "Find opposite edge failed";
            } else {
                if (unionedFaces.find(oppositeEdge->second.first) == unionedFaces.end()) {
                    unionedFaces.insert(edge.second.first);
                    unionedFaces.insert(oppositeEdge->second.first);
                    std::vector<size_t> indices;
                    indices.push_back(edge.second.second);
                    indices.push_back(edge.first.first);
                    indices.push_back(oppositeEdge->second.second);
                    indices.push_back(edge.first.second);
                    triangleAndQuads.push_back(indices);
                }
            }
        }
    }
    for (size_t i = 0; i < triangles.size(); i++) {
        if (unionedFaces.find(i) == unionedFaces.end()) {
            triangleAndQuads.push_back(triangles[i]);
        }
    }
}

size_t weldSeam(const std::vector<QVector3D> &sourceVertices, const std::vector<std::vector<size_t>> &sourceTriangles,
    float allowedSmallestDistance, const std::set<PositionKey> &excludePositions,
    std::vector<QVector3D> &destVertices, std::vector<std::vector<size_t>> &destTriangles)
{
    std::unordered_set<int> excludeVertices;
    for (size_t i = 0; i < sourceVertices.size(); ++i) {
        if (excludePositions.find(sourceVertices[i]) != excludePositions.end())
            excludeVertices.insert(i);
    }
    float squareOfAllowedSmallestDistance = allowedSmallestDistance * allowedSmallestDistance;
    std::map<int, int> weldVertexToMap;
    std::unordered_set<int> weldTargetVertices;
    std::unordered_set<int> processedFaces;
    std::map<std::pair<int, int>, std::pair<int, int>> triangleEdgeMap;
    std::unordered_map<int, int> vertexAdjFaceCountMap;
    for (int i = 0; i < (int)sourceTriangles.size(); i++) {
        const auto &faceIndices = sourceTriangles[i];
        if (faceIndices.size() == 3) {
            vertexAdjFaceCountMap[faceIndices[0]]++;
            vertexAdjFaceCountMap[faceIndices[1]]++;
            vertexAdjFaceCountMap[faceIndices[2]]++;
            triangleEdgeMap[std::make_pair(faceIndices[0], faceIndices[1])] = std::make_pair(i, faceIndices[2]);
            triangleEdgeMap[std::make_pair(faceIndices[1], faceIndices[2])] = std::make_pair(i, faceIndices[0]);
            triangleEdgeMap[std::make_pair(faceIndices[2], faceIndices[0])] = std::make_pair(i, faceIndices[1]);
        }
    }
    for (int i = 0; i < (int)sourceTriangles.size(); i++) {
        if (processedFaces.find(i) != processedFaces.end())
            continue;
        const auto &faceIndices = sourceTriangles[i];
        if (faceIndices.size() == 3) {
            bool indicesSeamCheck[3] = {
                excludeVertices.find(faceIndices[0]) == excludeVertices.end(),
                excludeVertices.find(faceIndices[1]) == excludeVertices.end(),
                excludeVertices.find(faceIndices[2]) == excludeVertices.end()
            };
            for (int j = 0; j < 3; j++) {
                int next = (j + 1) % 3;
                int nextNext = (j + 2) % 3;
                if (indicesSeamCheck[j] && indicesSeamCheck[next]) {
                    std::pair<int, int> edge = std::make_pair(faceIndices[j], faceIndices[next]);
                    int thirdVertexIndex = faceIndices[nextNext];
                    if ((sourceVertices[edge.first] - sourceVertices[edge.second]).lengthSquared() < squareOfAllowedSmallestDistance) {
                        auto oppositeEdge = std::make_pair(edge.second, edge.first);
                        auto findOppositeFace = triangleEdgeMap.find(oppositeEdge);
                        if (findOppositeFace == triangleEdgeMap.end()) {
                            qDebug() << "Find opposite edge failed";
                            continue;
                        }
                        int oppositeFaceIndex = findOppositeFace->second.first;
                        if (((sourceVertices[edge.first] - sourceVertices[thirdVertexIndex]).lengthSquared() <
                                    (sourceVertices[edge.second] - sourceVertices[thirdVertexIndex]).lengthSquared()) &&
                                vertexAdjFaceCountMap[edge.second] <= 4 &&
                                weldVertexToMap.find(edge.second) == weldVertexToMap.end()) {
                            weldVertexToMap[edge.second] = edge.first;
                            weldTargetVertices.insert(edge.first);
                            processedFaces.insert(i);
                            processedFaces.insert(oppositeFaceIndex);
                            break;
                        } else if (vertexAdjFaceCountMap[edge.first] <= 4 &&
                                weldVertexToMap.find(edge.first) == weldVertexToMap.end()) {
                            weldVertexToMap[edge.first] = edge.second;
                            weldTargetVertices.insert(edge.second);
                            processedFaces.insert(i);
                            processedFaces.insert(oppositeFaceIndex);
                            break;
                        }
                    }
                }
            }
        }
    }
    int weldedCount = 0;
    int faceCountAfterWeld = 0;
    std::map<int, int> oldToNewVerticesMap;
    for (int i = 0; i < (int)sourceTriangles.size(); i++) {
        const auto &faceIndices = sourceTriangles[i];
        std::vector<int> mappedFaceIndices;
        bool errored = false;
        for (const auto &index: faceIndices) {
            int finalIndex = index;
            int mapTimes = 0;
            while (mapTimes < 500) {
                auto findMapResult = weldVertexToMap.find(finalIndex);
                if (findMapResult == weldVertexToMap.end())
                    break;
                finalIndex = findMapResult->second;
                mapTimes++;
            }
            if (mapTimes >= 500) {
                qDebug() << "Map too much times";
                errored = true;
                break;
            }
            mappedFaceIndices.push_back(finalIndex);
        }
        if (errored || mappedFaceIndices.size() < 3)
            continue;
        bool welded = false;
        for (decltype(mappedFaceIndices.size()) j = 0; j < mappedFaceIndices.size(); j++) {
            int next = (j + 1) % 3;
            if (mappedFaceIndices[j] == mappedFaceIndices[next]) {
                welded = true;
                break;
            }
        }
        if (welded) {
            weldedCount++;
            continue;
        }
        faceCountAfterWeld++;
        std::vector<size_t> newFace;
        for (const auto &index: mappedFaceIndices) {
            auto findMap = oldToNewVerticesMap.find(index);
            if (findMap == oldToNewVerticesMap.end()) {
                size_t newIndex = destVertices.size();
                newFace.push_back(newIndex);
                destVertices.push_back(sourceVertices[index]);
                oldToNewVerticesMap.insert({index, newIndex});
            } else {
                newFace.push_back(findMap->second);
            }
        }
        destTriangles.push_back(newFace);
    }
    return weldedCount;
}

bool isManifold(const std::vector<std::vector<size_t>> &faces)
{
    std::set<std::pair<size_t, size_t>> halfEdges;
    for (const auto &face: faces) {
        for (size_t i = 0; i < face.size(); ++i) {
            size_t j = (i + 1) % face.size();
            auto insertResult = halfEdges.insert({face[i], face[j]});
            if (!insertResult.second)
                return false;
        }
    }
    for (const auto &it: halfEdges) {
        if (halfEdges.find({it.second, it.first}) == halfEdges.end())
            return false;
    }
    return true;
}

void trim(std::vector<QVector3D> *vertices, bool normalize)
{
    float xLow = std::numeric_limits<float>::max();
    float xHigh = std::numeric_limits<float>::lowest();
    float yLow = std::numeric_limits<float>::max();
    float yHigh = std::numeric_limits<float>::lowest();
    float zLow = std::numeric_limits<float>::max();
    float zHigh = std::numeric_limits<float>::lowest();
    for (const auto &position: *vertices) {
        if (position.x() < xLow)
            xLow = position.x();
        else if (position.x() > xHigh)
            xHigh = position.x();
        if (position.y() < yLow)
            yLow = position.y();
        else if (position.y() > yHigh)
            yHigh = position.y();
        if (position.z() < zLow)
            zLow = position.z();
        else if (position.z() > zHigh)
            zHigh = position.z();
    }
    float xMiddle = (xHigh + xLow) * 0.5;
    float yMiddle = (yHigh + yLow) * 0.5;
    float zMiddle = (zHigh + zLow) * 0.5;
    if (normalize) {
        float xSize = xHigh - xLow;
        float ySize = yHigh - yLow;
        float zSize = zHigh - zLow;
        float longSize = ySize;
        if (xSize > longSize)
            longSize = xSize;
        if (zSize > longSize)
            longSize = zSize;
        if (qFuzzyIsNull(longSize))
            longSize = 0.000001;
        for (auto &position: *vertices) {
            position.setX((position.x() - xMiddle) / longSize);
            position.setY((position.y() - yMiddle) / longSize);
            position.setZ((position.z() - zMiddle) / longSize);
        }
    } else {
        for (auto &position: *vertices) {
            position.setX((position.x() - xMiddle));
            position.setY((position.y() - yMiddle));
            position.setZ((position.z() - zMiddle));
        }
    }
}

void chamferFace2D(std::vector<QVector2D> *face)
{
    auto oldFace = *face;
    face->clear();
    for (size_t i = 0; i < oldFace.size(); ++i) {
        size_t j = (i + 1) % oldFace.size();
        face->push_back(oldFace[i] * 0.8 + oldFace[j] * 0.2);
        face->push_back(oldFace[i] * 0.2 + oldFace[j] * 0.8);
    }
}

void subdivideFace2D(std::vector<QVector2D> *face)
{
    auto oldFace = *face;
    face->resize(oldFace.size() * 2);
    for (size_t i = 0, n = 0; i < oldFace.size(); ++i) {
        size_t h = (i + oldFace.size() - 1) % oldFace.size();
        size_t j = (i + 1) % oldFace.size();
        (*face)[n++] = oldFace[h] * 0.125 + oldFace[i] * 0.75 + oldFace[j] * 0.125;
        (*face)[n++] = (oldFace[i] + oldFace[j]) * 0.5;
    }
}
