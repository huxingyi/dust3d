#include <QPen>
#include "skeletoneditnodeitem.h"
#include "theme.h"

SkeletonEditNodeItem::SkeletonEditNodeItem(const QRectF &rect, QGraphicsItem *parent) :
    QGraphicsEllipseItem(rect, parent),
    m_highlighted(false),
    m_isNextStartNode(false),
    m_master(NULL),
    m_slave(NULL)
{
    setData(0, "node");
    updateBorder();
}

bool SkeletonEditNodeItem::isSlave()
{
    return NULL != m_master;
}

bool SkeletonEditNodeItem::isMaster()
{
    return NULL == m_master;
}

SkeletonEditNodeItem *SkeletonEditNodeItem::master()
{
    return m_master ? m_master : this;
}

void SkeletonEditNodeItem::setMaster(SkeletonEditNodeItem *nodeItem)
{
    m_master = nodeItem;
}

void SkeletonEditNodeItem::setSlave(SkeletonEditNodeItem *nodeItem)
{
    m_slave = nodeItem;
}

SkeletonEditNodeItem *SkeletonEditNodeItem::slave()
{
    return m_slave;
}

QPointF SkeletonEditNodeItem::origin()
{
    return QPointF(rect().x() + rect().width() / 2,
        rect().y() + rect().height() / 2);
}

float SkeletonEditNodeItem::radius()
{
    return rect().width() / 2;
}

void SkeletonEditNodeItem::setHighlighted(bool highlighted)
{
    m_highlighted = highlighted;
    if (m_highlighted) {
        setBrush(QBrush(isMaster() ? Theme::skeletonMasterNodeFillColor :  Theme::skeletonSlaveNodeFillColor));
    } else {
        setBrush(QBrush(Qt::transparent));
    }
    if (m_slave)
    {
        m_slave->setHighlighted(highlighted);
    }
}

void SkeletonEditNodeItem::setIsNextStartNode(bool isNextStartNode)
{
    m_isNextStartNode = isNextStartNode;
    updateBorder();
}

void SkeletonEditNodeItem::updateBorder()
{
    QPen pen(m_isNextStartNode ?
        (isMaster() ? Theme::skeletonMasterNodeBorderHighlightColor : Theme::skeletonSlaveNodeBorderHighlightColor) :
        (isMaster() ? Theme::skeletonMasterNodeBorderColor : Theme::skeletonSlaveNodeBorderColor));
    pen.setWidth(isMaster() ? Theme::skeletonMasterNodeBorderSize : Theme::skeletonSlaveNodeBorderSize);
    setPen(pen);
}
