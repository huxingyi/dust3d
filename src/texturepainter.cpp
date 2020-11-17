#include <QDebug>
#include <QRadialGradient>
#include <QBrush>
#include <QPainter>
#include <QGuiApplication>
#include <QPolygon>
#include "texturepainter.h"
#include "util.h"

TexturePainter::TexturePainter(const QVector3D &mouseRayNear, const QVector3D &mouseRayFar) :
    m_mouseRayNear(mouseRayNear),
    m_mouseRayFar(mouseRayFar)
{
}

void TexturePainter::setContext(TexturePainterContext *context)
{
    m_context = context;
}

TexturePainter::~TexturePainter()
{
    delete m_colorImage;
}

void TexturePainter::setPaintMode(PaintMode paintMode)
{
    m_paintMode = paintMode;
}

void TexturePainter::setMaskNodeIds(const std::set<QUuid> &nodeIds)
{
    m_mousePickMaskNodeIds = nodeIds;
}

void TexturePainter::setRadius(float radius)
{
    m_radius = radius;
}

void TexturePainter::setBrushColor(const QColor &color)
{
    m_brushColor = color;
}

QImage *TexturePainter::takeColorImage()
{
    QImage *colorImage = m_colorImage;
    m_colorImage = nullptr;
    return colorImage;
}

/*
void TexturePainter::buildFaceAroundVertexMap()
{
    if (nullptr != m_context->faceAroundVertexMap)
        return;
    
    m_context->faceAroundVertexMap = new std::unordered_map<size_t, std::unordered_set<size_t>>;
    for (size_t triangleIndex = 0; 
            triangleIndex < m_context->object->triangles.size(); 
            ++triangleIndex) {
        for (const auto &it: m_context->object->triangles[triangleIndex])
            (*m_context->faceAroundVertexMap)[it].insert(triangleIndex);
    }
}

void TexturePainter::collectNearbyTriangles(size_t triangleIndex, std::unordered_set<size_t> *triangleIndices)
{
    for (const auto &vertex: m_context->object->triangles[triangleIndex])
        for (const auto &it: (*m_context->faceAroundVertexMap)[vertex])
            triangleIndices->insert(it);
}
*/

void TexturePainter::paintStroke(QPainter &painter, const TexturePainterStroke &stroke)
{
    size_t targetTriangleIndex = 0;
    if (!intersectRayAndPolyhedron(stroke.mouseRayNear,
            stroke.mouseRayFar,
            m_context->object->vertices,
            m_context->object->triangles,
            m_context->object->triangleNormals,
            &m_targetPosition,
            &targetTriangleIndex)) {
        return; 
    }
    
    if (PaintMode::None == m_paintMode)
        return;
    
    if (nullptr == m_context->colorImage) {
        qDebug() << "TexturePainter paint color image is null";
        return;
    }

    const std::vector<std::vector<QVector2D>> *uvs = m_context->object->triangleVertexUvs();
    if (nullptr == uvs) {
        qDebug() << "TexturePainter paint uvs is null";
        return;
    }
    
    const std::vector<std::pair<QUuid, QUuid>> *sourceNodes = m_context->object->triangleSourceNodes();
    if (nullptr == sourceNodes) {
        qDebug() << "TexturePainter paint source nodes is null";
        return;
    }
    
    const std::map<QUuid, std::vector<QRectF>> *uvRects = m_context->object->partUvRects();
    if (nullptr == uvRects)
        return;
    
    const auto &triangle = m_context->object->triangles[targetTriangleIndex];
    QVector3D coordinates = barycentricCoordinates(m_context->object->vertices[triangle[0]],
        m_context->object->vertices[triangle[1]],
        m_context->object->vertices[triangle[2]],
        m_targetPosition);
        
    double triangleArea = areaOfTriangle(m_context->object->vertices[triangle[0]],
        m_context->object->vertices[triangle[1]],
        m_context->object->vertices[triangle[2]]);
    
    auto &uvCoords = (*uvs)[targetTriangleIndex];
    QVector2D target2d = uvCoords[0] * coordinates[0] +
            uvCoords[1] * coordinates[1] +
            uvCoords[2] * coordinates[2];
            
    double uvArea = areaOfTriangle(QVector3D(uvCoords[0].x(), uvCoords[0].y(), 0.0),
        QVector3D(uvCoords[1].x(), uvCoords[1].y(), 0.0),
        QVector3D(uvCoords[2].x(), uvCoords[2].y(), 0.0));
    
    double radiusFactor = std::sqrt(uvArea) / std::sqrt(triangleArea);
            
    //QPolygon polygon;
    //polygon << QPoint(uvCoords[0].x() * m_context->colorImage->height(), uvCoords[0].y() * m_context->colorImage->height()) << 
    //    QPoint(uvCoords[1].x() * m_context->colorImage->height(), uvCoords[1].y() * m_context->colorImage->height()) <<
    //    QPoint(uvCoords[2].x() * m_context->colorImage->height(), uvCoords[2].y() * m_context->colorImage->height());
    //QRegion clipRegion(polygon);
    //painter.setClipRegion(clipRegion);
    
    std::vector<QRect> rects;
    const auto &sourceNode = (*sourceNodes)[targetTriangleIndex];
    auto findRects = uvRects->find(sourceNode.first);
    const int paddingSize = 2;
    if (findRects != uvRects->end()) {
        for (const auto &it: findRects->second) {
            if (!it.contains({target2d.x(), target2d.y()}))
                continue;
            rects.push_back(QRect(it.left() * m_context->colorImage->height() - paddingSize,
                it.top() * m_context->colorImage->height() - paddingSize,
                it.width() * m_context->colorImage->height() + paddingSize + paddingSize,
                it.height() * m_context->colorImage->height() + paddingSize + paddingSize));
            break;
        }
    }
    QRegion clipRegion;
    if (!rects.empty()) {
        std::sort(rects.begin(), rects.end(), [](const QRect &first, const QRect &second) {
            return first.top() < second.top();
        });
        clipRegion.setRects(&rects[0], rects.size());
        painter.setClipRegion(clipRegion);
    }
    
    double radius = m_radius * radiusFactor * m_context->colorImage->height();
    QVector2D middlePoint = QVector2D(target2d.x() * m_context->colorImage->height(), 
        target2d.y() * m_context->colorImage->height());
    
    QRadialGradient gradient(QPointF(middlePoint.x(), middlePoint.y()), radius);
    gradient.setColorAt(0.0, m_brushColor);
    gradient.setColorAt(1.0, Qt::transparent);
                    
    painter.fillRect(middlePoint.x() - radius, 
        middlePoint.y() - radius, 
        radius + radius, 
        radius + radius,
        gradient);
}

void TexturePainter::paint()
{
    if (nullptr == m_context) {
        qDebug() << "TexturePainter paint context is null";
        return;
    }
    
    QPainter painter(m_context->colorImage);
    painter.setPen(Qt::NoPen);
    
    TexturePainterStroke stroke = {m_mouseRayNear, m_mouseRayFar};
    paintStroke(painter, stroke);
    
    m_colorImage = new QImage(*m_context->colorImage);
}

void TexturePainter::process()
{
    paint();

    emit finished();
}

const QVector3D &TexturePainter::targetPosition()
{
    return m_targetPosition;
}
