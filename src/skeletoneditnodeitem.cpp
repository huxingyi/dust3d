#include <QPen>
#include "skeletoneditnodeitem.h"

SkeletonEditNodeItem::SkeletonEditNodeItem(const QRectF &rect, QGraphicsItem *parent) :
    QGraphicsEllipseItem(rect, parent),
    m_highlighted(false),
    m_isNextStartNode(false)
{
    setData(0, "node");
    updateBorder();
}

QPointF SkeletonEditNodeItem::origin()
{
    return QPointF(rect().left() + rect().width() / 2,
        rect().top() + rect().height() / 2);
}

float SkeletonEditNodeItem::radius()
{
    return rect().width() / 2;
}

void SkeletonEditNodeItem::setHighlighted(bool highlighted)
{
    m_highlighted = highlighted;
    if (m_highlighted) {
        setBrush(QBrush(Qt::gray));
    } else {
        setBrush(QBrush(Qt::transparent));
    }
}

void SkeletonEditNodeItem::setIsNextStartNode(bool isNextStartNode)
{
    m_isNextStartNode = isNextStartNode;
    updateBorder();
}

void SkeletonEditNodeItem::updateBorder()
{
    QPen pen(m_isNextStartNode ? Qt::black : Qt::darkGray);
    pen.setWidth(15);
    setPen(pen);
}
