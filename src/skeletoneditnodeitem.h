#ifndef SKELETON_EDIT_NODE_ITEM_H
#define SKELETON_EDIT_NODE_ITEM_H
#include <QGraphicsEllipseItem>

class SkeletonEditNodeItem : public QGraphicsEllipseItem
{
public:
    SkeletonEditNodeItem(const QRectF &rect, QGraphicsItem *parent = 0);
    QPointF origin();
    float radius();
    void setHighlighted(bool highlited);
    void setIsNextStartNode(bool isNextStartNode);
private:
    bool m_highlighted;
    bool m_isNextStartNode;
    void updateBorder();
};

#endif

