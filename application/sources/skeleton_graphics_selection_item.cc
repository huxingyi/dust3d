#include "skeleton_graphics_selection_item.h"
#include "theme.h"
#include <QLineF>

SkeletonGraphicsSelectionItem::SkeletonGraphicsSelectionItem()
{
    QColor penColor = Theme::white;
    QPen pen(penColor);
    pen.setWidth(0);
    pen.setStyle(Qt::DashLine);
    setPen(pen);

    QColor fillColor = penColor;
    fillColor.setAlphaF(0.1);
    setBrush(QBrush(fillColor));
}

void SkeletonGraphicsSelectionItem::reset(QPointF startPos)
{
    m_polygon.clear();
    m_polygon.append(startPos);
    updatePath();
}

void SkeletonGraphicsSelectionItem::addPoint(QPointF pos)
{
    // Skip points that are too close to the previous one to keep the polygon light.
    if (!m_polygon.isEmpty() && QLineF(m_polygon.last(), pos).length() < 2.0)
        return;
    m_polygon.append(pos);
    updatePath();
}

void SkeletonGraphicsSelectionItem::updatePath()
{
    QPainterPath path;
    if (!m_polygon.isEmpty()) {
        path.moveTo(m_polygon.first());
        for (int i = 1; i < m_polygon.size(); ++i)
            path.lineTo(m_polygon[i]);
        // Draw the closing segment so the freehand loop reads as a closed region.
        path.lineTo(m_polygon.first());
    }
    setPath(path);
}

QPainterPath SkeletonGraphicsSelectionItem::selectionShape() const
{
    QPainterPath path;
    if (m_polygon.size() >= 3) {
        path.addPolygon(m_polygon);
        path.closeSubpath();
    } else if (!m_polygon.isEmpty()) {
        // Too few points to form an area; fall back to the bounding rectangle.
        path.addRect(m_polygon.boundingRect());
    }
    return path;
}
