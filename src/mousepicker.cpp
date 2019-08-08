#include <QDebug>
#include "mousepicker.h"

MousePicker::MousePicker(const Outcome &outcome, const QVector3D &mouseRayNear, const QVector3D &mouseRayFar) :
    m_outcome(outcome),
    m_mouseRayNear(mouseRayNear),
    m_mouseRayFar(mouseRayFar)
{
}

MousePicker::~MousePicker()
{
}

void MousePicker::process()
{
    float minDistance2 = std::numeric_limits<float>::max();
    for (size_t i = 0; i < m_outcome.triangles.size(); ++i) {
        const auto &triangleIndices = m_outcome.triangles[i];
        std::vector<QVector3D> triangle = {
            m_outcome.vertices[triangleIndices[0]],
            m_outcome.vertices[triangleIndices[1]],
            m_outcome.vertices[triangleIndices[2]],
        };
        const auto &triangleNormal = m_outcome.triangleNormals[i];
        QVector3D intersection;
        if (intersectSegmentAndTriangle(m_mouseRayNear, m_mouseRayFar,
                triangle,
                triangleNormal,
                &intersection)) {
            float distance2 = (intersection - m_mouseRayNear).lengthSquared();
            if (distance2 < minDistance2) {
                m_targetPosition = intersection;
                minDistance2 = distance2;
            }
        }
    }

    emit finished();
}

const QVector3D &MousePicker::targetPosition()
{
    return m_targetPosition;
}

bool MousePicker::intersectSegmentAndPlane(const QVector3D &segmentPoint0, const QVector3D &segmentPoint1,
    const QVector3D &pointOnPlane, const QVector3D &planeNormal,
    QVector3D *intersection)
{
    auto u = segmentPoint1 - segmentPoint0;
    auto w = segmentPoint0 - pointOnPlane;
    auto d = QVector3D::dotProduct(planeNormal, u);
    auto n = QVector3D::dotProduct(-planeNormal, w);
    if (qAbs(d) < 0.00000001)
        return false;
    auto s = n / d;
    if (s < 0 || s > 1 || qIsNaN(s) || qIsInf(s))
        return false;
    if (nullptr != intersection)
        *intersection = segmentPoint0 + s * u;
    return true;
}

bool MousePicker::intersectSegmentAndTriangle(const QVector3D &segmentPoint0, const QVector3D &segmentPoint1,
    const std::vector<QVector3D> &triangle,
    const QVector3D &triangleNormal,
    QVector3D *intersection)
{
    QVector3D possibleIntersection;
    if (!intersectSegmentAndPlane(segmentPoint0, segmentPoint1,
            triangle[0], triangleNormal, &possibleIntersection)) {
        return false;
    }
    auto ray = (segmentPoint0 - segmentPoint1).normalized();
    std::vector<QVector3D> normals;
    for (size_t i = 0; i < 3; ++i) {
        size_t j = (i + 1) % 3;
        normals.push_back(QVector3D::normal(possibleIntersection, triangle[i], triangle[j]));
    }
    if (QVector3D::dotProduct(normals[0], ray) <= 0)
        return false;
    if (QVector3D::dotProduct(normals[0], normals[1]) <= 0)
        return false;
    if (QVector3D::dotProduct(normals[0], normals[2]) <= 0)
        return false;
    if (nullptr != intersection)
        *intersection = possibleIntersection;
    return true;
}
