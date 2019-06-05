#include <QStringList>
#include <QVector2D>
#include <QVector3D>
#include <QDebug>
#include "cutface.h"

IMPL_CutFaceFromString
IMPL_CutFaceToString
TMPL_CutFaceToPoints

static void correctFlippedNormal(std::vector<QVector2D> *points)
{
    if (points->size() < 3)
        return;
    std::vector<QVector3D> referenceFacePoints = {
        QVector3D((float)-1.0, (float)-1.0, (float)0.0),
        QVector3D((float)1.0, (float)-1.0, (float)0.0),
        QVector3D((float)1.0,  (float)1.0, (float)0.0)
    };
    QVector3D referenceNormal = QVector3D::normal(referenceFacePoints[0],
        referenceFacePoints[1], referenceFacePoints[2]);
    QVector3D normal;
    for (size_t i = 0; i < points->size(); ++i) {
        size_t j = (i + 1) % points->size();
        size_t k = (j + 1) % points->size();
        std::vector<QVector3D> facePoints = {
            QVector3D(((*points)[i]).x(), ((*points)[i]).y(), (float)0.0),
            QVector3D(((*points)[j]).x(), ((*points)[j]).y(), (float)0.0),
            QVector3D(((*points)[k]).x(), ((*points)[k]).y(), (float)0.0)
        };
        normal += QVector3D::normal(facePoints[0],
            facePoints[1], facePoints[2]);
    }
    normal.normalize();
    if (QVector3D::dotProduct(referenceNormal, normal) > 0)
        return;
    std::reverse(points->begin() + 1, points->end());
}

void normalizeCutFacePoints(std::vector<QVector2D> *points)
{
    QVector2D center;
    if (nullptr == points || points->empty())
        return;
    float xLow = std::numeric_limits<float>::max();
    float xHigh = std::numeric_limits<float>::lowest();
    float yLow = std::numeric_limits<float>::max();
    float yHigh = std::numeric_limits<float>::lowest();
    for (const auto &position: *points) {
        if (position.x() < xLow)
            xLow = position.x();
        else if (position.x() > xHigh)
            xHigh = position.x();
        if (position.y() < yLow)
            yLow = position.y();
        else if (position.y() > yHigh)
            yHigh = position.y();
    }
    float xMiddle = (xHigh + xLow) * 0.5;
    float yMiddle = (yHigh + yLow) * 0.5;
    float xSize = xHigh - xLow;
    float ySize = yHigh - yLow;
    float longSize = ySize;
    if (xSize > longSize)
        longSize = xSize;
    if (qFuzzyIsNull(longSize))
        longSize = 0.000001;
    for (auto &position: *points) {
        position.setX((position.x() - xMiddle) * 2 / longSize);
        position.setY((position.y() - yMiddle) * 2 / longSize);
    }
    correctFlippedNormal(points);
}

void cutFacePointsFromNodes(std::vector<QVector2D> &points, const std::vector<std::tuple<float, float, float>> &nodes, bool isRing)
{
    if (isRing) {
        if (nodes.size() < 3)
            return;
        for (const auto &it: nodes) {
            points.push_back(QVector2D(std::get<1>(it), std::get<2>(it)));
        }
        normalizeCutFacePoints(&points);
        return;
    }
    if (nodes.size() < 2)
        return;
    std::vector<QVector2D> edges;
    for (size_t i = 1; i < nodes.size(); ++i) {
        const auto &previous = nodes[i - 1];
        const auto &current = nodes[i];
        edges.push_back((QVector2D(std::get<1>(current), std::get<2>(current)) -
            QVector2D(std::get<1>(previous), std::get<2>(previous))).normalized());
    }
    std::vector<QVector2D> nodeDirections;
    nodeDirections.push_back(edges[0]);
    for (size_t i = 1; i < nodes.size() - 1; ++i) {
        const auto &previousEdge = edges[i - 1];
        const auto &nextEdge = edges[i];
        nodeDirections.push_back((previousEdge + nextEdge).normalized());
    }
    nodeDirections.push_back(edges[edges.size() - 1]);
    std::vector<std::pair<QVector2D, QVector2D>> cutPoints;
    for (size_t i = 0; i < nodes.size(); ++i) {
        const auto &current = nodes[i];
        const auto &direction = nodeDirections[i];
        const auto &radius = std::get<0>(current);
        QVector3D origin = QVector3D(std::get<1>(current), std::get<2>(current), 0);
        QVector3D pointer = QVector3D(direction.x(), direction.y(), 0);
        QVector3D rotateAxis = QVector3D(0, 0, 1);
        QVector3D u = QVector3D::crossProduct(pointer, rotateAxis).normalized();
        QVector3D upPoint = origin + u * radius;
        QVector3D downPoint = origin - u * radius;
        cutPoints.push_back({QVector2D(upPoint.x(), upPoint.y()),
            QVector2D(downPoint.x(), downPoint.y())});
    }
    for (const auto &it: cutPoints) {
        points.push_back(it.first);
    }
    for (auto it = cutPoints.rbegin(); it != cutPoints.rend(); ++it) {
        points.push_back(it->second);
    }
    normalizeCutFacePoints(&points);
}
