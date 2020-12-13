#include <QPainter>
#include <QBrush>
#include "silhouetteimagegenerator.h"
#include "util.h"

SilhouetteImageGenerator::SilhouetteImageGenerator(int width, int height, Snapshot *snapshot) :
    m_width(width),
    m_height(height),
    m_snapshot(snapshot)
{
}

SilhouetteImageGenerator::~SilhouetteImageGenerator()
{
    delete m_snapshot;
    delete m_resultImage;
}

QImage *SilhouetteImageGenerator::takeResultImage()
{
    QImage *resultImage = m_resultImage;
    m_resultImage = nullptr;
    return resultImage;
}

void SilhouetteImageGenerator::generate()
{
    if (m_width <= 0 || m_height <= 0 || nullptr == m_snapshot)
        return;
    
    //float originX = valueOfKeyInMapOrEmpty(m_snapshot->canvas, "originX").toFloat();
    //float originY = valueOfKeyInMapOrEmpty(m_snapshot->canvas, "originY").toFloat();
    //float originZ = valueOfKeyInMapOrEmpty(m_snapshot->canvas, "originZ").toFloat();
    
    delete m_resultImage;
    m_resultImage = new QImage(m_width, m_height, QImage::Format_ARGB32);
    m_resultImage->fill(QColor(0xE1, 0xD2, 0xBD));
    
    struct NodeInfo
    {
        QVector3D position;
        float radius;
    };
    
    std::map<QString, NodeInfo> nodePositionMap;
    for (const auto &node: m_snapshot->nodes) {
        NodeInfo nodeInfo;
        nodeInfo.position = QVector3D(valueOfKeyInMapOrEmpty(node.second, "x").toFloat(),
            valueOfKeyInMapOrEmpty(node.second, "y").toFloat(),
            valueOfKeyInMapOrEmpty(node.second, "z").toFloat());
        nodeInfo.radius = valueOfKeyInMapOrEmpty(node.second, "radius").toFloat();
        nodePositionMap.insert({node.first, nodeInfo});
    }
    
    QPainter painter;
    painter.begin(m_resultImage);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::HighQualityAntialiasing);

    painter.setPen(Qt::NoPen);
    
    QBrush brush;
    brush.setColor(QColor(0x88, 0x80, 0x73));
    brush.setStyle(Qt::SolidPattern);
    
    painter.setBrush(brush);
    
    for (const auto &it: nodePositionMap) {
        const auto &nodeInfo = it.second;
        
        painter.drawEllipse((nodeInfo.position.x() - nodeInfo.radius) * m_height,
            (nodeInfo.position.y() - nodeInfo.radius) * m_height,
            nodeInfo.radius * m_height * 2.0,
            nodeInfo.radius * m_height * 2.0);
            
        painter.drawEllipse((nodeInfo.position.z() - nodeInfo.radius) * m_height,
            (nodeInfo.position.y() - nodeInfo.radius) * m_height,
            nodeInfo.radius * m_height * 2.0,
            nodeInfo.radius * m_height * 2.0);
    }
    
    for (int round = 0; round < 2; ++round) {
        if (1 == round)
            painter.setCompositionMode(QPainter::CompositionMode_Multiply);
        for (const auto &edge: m_snapshot->edges) {
            QString partId = valueOfKeyInMapOrEmpty(edge.second, "partId");
            QString fromNodeId = valueOfKeyInMapOrEmpty(edge.second, "from");
            QString toNodeId = valueOfKeyInMapOrEmpty(edge.second, "to");
            const auto &fromNodeInfo = nodePositionMap[fromNodeId];
            const auto &toNodeInfo = nodePositionMap[toNodeId];
      
            {
                QVector3D pointerOut = QVector3D(0.0, 0.0, 1.0);
                QVector3D direction = (QVector3D(toNodeInfo.position.x(), toNodeInfo.position.y(), 0.0) - 
                    QVector3D(fromNodeInfo.position.x(), fromNodeInfo.position.y(), 0.0)).normalized();
                QVector3D fromBaseDirection = QVector3D::crossProduct(pointerOut, direction);
                QVector3D fromBaseRadius = fromBaseDirection * fromNodeInfo.radius;
                QVector3D fromBaseFirstPoint = QVector3D(fromNodeInfo.position.x(), fromNodeInfo.position.y(), 0.0) - fromBaseRadius;
                QVector3D fromBaseSecondPoint = QVector3D(fromNodeInfo.position.x(), fromNodeInfo.position.y(), 0.0) + fromBaseRadius;
                QVector3D tobaseDirection = -fromBaseDirection;
                QVector3D toBaseRadius = tobaseDirection * toNodeInfo.radius;
                QVector3D toBaseFirstPoint = QVector3D(toNodeInfo.position.x(), toNodeInfo.position.y(), 0.0) - toBaseRadius;
                QVector3D toBaseSecondPoint = QVector3D(toNodeInfo.position.x(), toNodeInfo.position.y(), 0.0) + toBaseRadius;
                QPolygon polygon;
                polygon.append(QPoint(fromBaseFirstPoint.x() * m_height, fromBaseFirstPoint.y() * m_height));
                polygon.append(QPoint(fromBaseSecondPoint.x() * m_height, fromBaseSecondPoint.y() * m_height));
                polygon.append(QPoint(toBaseFirstPoint.x() * m_height, toBaseFirstPoint.y() * m_height));
                polygon.append(QPoint(toBaseSecondPoint.x() * m_height, toBaseSecondPoint.y() * m_height));
                QPainterPath path;
                path.addPolygon(polygon);
                painter.fillPath(path, brush);
            }
            
            {
                QVector3D pointerOut = QVector3D(0.0, 0.0, 1.0);
                QVector3D direction = (QVector3D(toNodeInfo.position.z(), toNodeInfo.position.y(), 0.0) - 
                    QVector3D(fromNodeInfo.position.z(), fromNodeInfo.position.y(), 0.0)).normalized();
                QVector3D fromBaseDirection = QVector3D::crossProduct(pointerOut, direction);
                QVector3D fromBaseRadius = fromBaseDirection * fromNodeInfo.radius;
                QVector3D fromBaseFirstPoint = QVector3D(fromNodeInfo.position.z(), fromNodeInfo.position.y(), 0.0) - fromBaseRadius;
                QVector3D fromBaseSecondPoint = QVector3D(fromNodeInfo.position.z(), fromNodeInfo.position.y(), 0.0) + fromBaseRadius;
                QVector3D tobaseDirection = -fromBaseDirection;
                QVector3D toBaseRadius = tobaseDirection * toNodeInfo.radius;
                QVector3D toBaseFirstPoint = QVector3D(toNodeInfo.position.z(), toNodeInfo.position.y(), 0.0) - toBaseRadius;
                QVector3D toBaseSecondPoint = QVector3D(toNodeInfo.position.z(), toNodeInfo.position.y(), 0.0) + toBaseRadius;
                QPolygon polygon;
                polygon.append(QPoint(fromBaseFirstPoint.x() * m_height, fromBaseFirstPoint.y() * m_height));
                polygon.append(QPoint(fromBaseSecondPoint.x() * m_height, fromBaseSecondPoint.y() * m_height));
                polygon.append(QPoint(toBaseFirstPoint.x() * m_height, toBaseFirstPoint.y() * m_height));
                polygon.append(QPoint(toBaseSecondPoint.x() * m_height, toBaseSecondPoint.y() * m_height));
                QPainterPath path;
                path.addPolygon(polygon);
                painter.fillPath(path, brush);
            }
        }
    }

    painter.end();
}

void SilhouetteImageGenerator::process()
{
    generate();

    emit finished();
}
