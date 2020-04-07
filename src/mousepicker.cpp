#include <QDebug>
#include <QQuaternion>
#include <QRadialGradient>
#include <QBrush>
#include <QPainter>
#include <QGuiApplication>
#include "mousepicker.h"
#include "util.h"
#include "imageforever.h"

MousePicker::MousePicker(const Outcome &outcome, const QVector3D &mouseRayNear, const QVector3D &mouseRayFar) :
    m_outcome(outcome),
    m_mouseRayNear(mouseRayNear),
    m_mouseRayFar(mouseRayFar)
{
}

const std::set<QUuid> &MousePicker::changedPartIds()
{
    return m_changedPartIds;
}

void MousePicker::setPaintMode(PaintMode paintMode)
{
    m_paintMode = paintMode;
}

void MousePicker::setMaskNodeIds(const std::set<QUuid> &nodeIds)
{
    m_mousePickMaskNodeIds = nodeIds;
}

void MousePicker::setRadius(float radius)
{
    m_radius = radius;
}

MousePicker::~MousePicker()
{
}

bool MousePicker::calculateMouseModelPosition(QVector3D &mouseModelPosition)
{
    bool foundPosition = false;
    auto ray = (m_mouseRayNear - m_mouseRayFar).normalized();
    float minDistance2 = std::numeric_limits<float>::max();
    for (size_t i = 0; i < m_outcome.triangles.size(); ++i) {
        const auto &triangleIndices = m_outcome.triangles[i];
        std::vector<QVector3D> triangle = {
            m_outcome.vertices[triangleIndices[0]],
            m_outcome.vertices[triangleIndices[1]],
            m_outcome.vertices[triangleIndices[2]],
        };
        const auto &triangleNormal = m_outcome.triangleNormals[i];
        if (QVector3D::dotProduct(triangleNormal, ray) <= 0)
            continue;
        QVector3D intersection;
        if (intersectSegmentAndTriangle(m_mouseRayNear, m_mouseRayFar,
                triangle,
                triangleNormal,
                &intersection)) {
            float distance2 = (intersection - m_mouseRayNear).lengthSquared();
            if (distance2 < minDistance2) {
                mouseModelPosition = intersection;
                minDistance2 = distance2;
                foundPosition = true;
            }
        }
    }
    return foundPosition;
}

void MousePicker::pick()
{
    if (!calculateMouseModelPosition(m_targetPosition))
        return;
    
    if (PaintMode::None == m_paintMode)
        return;
    
    float distance2 = m_radius * m_radius;
    
    for (const auto &map: m_outcome.paintMaps) {
        for (const auto &node: map.paintNodes) {
            if (!m_mousePickMaskNodeIds.empty() && m_mousePickMaskNodeIds.find(node.originNodeId) == m_mousePickMaskNodeIds.end())
                continue;
            size_t intersectedNum = 0;
            QVector3D sumOfDirection;
            QVector3D referenceDirection = (m_targetPosition - node.origin).normalized();
            float sumOfRadius = 0;
            for (const auto &vertexPosition: node.vertices) {
                // >0.866 = <30 degrees
                auto direction = (vertexPosition - node.origin).normalized();
                if (QVector3D::dotProduct(referenceDirection, direction) > 0.866 &&
                        (vertexPosition - m_targetPosition).lengthSquared() <= distance2) {
                    float distance = vertexPosition.distanceToPoint(m_targetPosition);
                    float radius = (m_radius - distance) / node.radius;
                    sumOfRadius += radius;
                    sumOfDirection += direction * radius;
                    ++intersectedNum;
                }
            }
            if (intersectedNum > 0) {
                float paintRadius = sumOfRadius / intersectedNum;
                QVector3D paintDirection = sumOfDirection.normalized();
                float degrees = angleInRangle360BetweenTwoVectors(node.baseNormal, paintDirection, node.direction);
                float offset = (float)node.order / map.paintNodes.size();
                m_changedPartIds.insert(map.partId);
                paintToImage(map.partId, offset, degrees / 360.0, paintRadius, PaintMode::Push == m_paintMode);
            }
        }
    }
}

void MousePicker::process()
{
    pick();
    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}

void MousePicker::paintToImage(const QUuid &partId, float x, float y, float radius, bool inverted)
{
    QUuid oldImageId;
    QImage image(72, 36, QImage::Format_Grayscale8);
    image.fill(QColor(127, 127, 127));
    const auto &findImageId = m_paintImages.find(partId);
    if (findImageId != m_paintImages.end()) {
        const QImage *oldImage = ImageForever::get(findImageId->second);
        if (nullptr != oldImage) {
            if (oldImage->size() == image.size() &&
                    oldImage->format() == image.format()) {
                image = *oldImage;
            }
        }
    }
    float destX = image.width() * x;
    float destY = image.height() * y;
    float destRadius = image.height() * radius;
    {
        QRadialGradient gradient(destX, destY, destRadius / 2);
        if (inverted) {
            gradient.setColorAt(0, QColor(0, 0, 0, 3));
            gradient.setColorAt(1, Qt::transparent);
        } else {
            gradient.setColorAt(0, QColor(255, 255, 255, 3));
            gradient.setColorAt(1, Qt::transparent);
        }
        QBrush brush(gradient);
        QPainter paint(&image);
        paint.setRenderHint(QPainter::HighQualityAntialiasing);
        paint.setBrush(brush);
        paint.setPen(Qt::NoPen);
        paint.drawEllipse(destX - destRadius / 2, destY - destRadius / 2, destRadius, destRadius);
    }
    QUuid imageId = ImageForever::add(&image);
    m_paintImages[partId] = imageId;
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

void MousePicker::setPaintImages(const std::map<QUuid, QUuid> &paintImages)
{
    m_paintImages = paintImages;
}

const std::map<QUuid, QUuid> &MousePicker::resultPaintImages()
{
    return m_paintImages;
}
