#include <QPen>
#include "skeletoneditedgeitem.h"

SkeletonEditEdgeItem::SkeletonEditEdgeItem(QGraphicsItem *parent) :
    QGraphicsLineItem(parent),
    m_firstNode(NULL),
    m_secondNode(NULL)
{
    QPen pen(Qt::darkGray);
    pen.setWidth(15);
    setPen(pen);
}

void SkeletonEditEdgeItem::setNodes(SkeletonEditNodeItem *first, SkeletonEditNodeItem *second)
{
    m_firstNode = first;
    m_secondNode = second;
    updatePosition();
}

void SkeletonEditEdgeItem::updatePosition()
{
    if (m_firstNode && m_secondNode) {
        QLineF line(m_firstNode->origin(), m_secondNode->origin());
        setLine(line);
    }
}
