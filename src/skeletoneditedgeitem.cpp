#include <QPen>
#include "skeletoneditedgeitem.h"
#include "theme.h"

SkeletonEditEdgeItem::SkeletonEditEdgeItem(QGraphicsItem *parent) :
    QGraphicsLineItem(parent),
    m_firstNode(NULL),
    m_secondNode(NULL)
{
    setData(0, "edge");
    QPen pen(Theme::skeletonMasterNodeBorderColor);
    pen.setWidth(Theme::skeletonMasterNodeBorderSize);
    setPen(pen);
}

void SkeletonEditEdgeItem::setNodes(SkeletonEditNodeItem *first, SkeletonEditNodeItem *second)
{
    m_firstNode = first;
    m_secondNode = second;
    updatePosition();
}

bool SkeletonEditEdgeItem::connects(SkeletonEditNodeItem *nodeItem)
{
    return m_firstNode == nodeItem || m_secondNode == nodeItem;
}

void SkeletonEditEdgeItem::updatePosition()
{
    if (m_firstNode && m_secondNode) {
        QLineF line(m_firstNode->origin(), m_secondNode->origin());
        setLine(line);
    }
}

SkeletonEditNodeItem *SkeletonEditEdgeItem::firstNode()
{
    return m_firstNode;
}

SkeletonEditNodeItem *SkeletonEditEdgeItem::secondNode()
{
    return m_secondNode;
}
