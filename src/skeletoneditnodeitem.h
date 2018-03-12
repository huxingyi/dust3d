#ifndef SKELETON_EDIT_NODE_ITEM_H
#define SKELETON_EDIT_NODE_ITEM_H
#include <QGraphicsEllipseItem>

class SkeletonEditEdgeItem;

class SkeletonEditNodeItem : public QGraphicsEllipseItem
{
public:
    SkeletonEditNodeItem(const QRectF &rect, QGraphicsItem *parent = 0);
    QPointF origin();
    float radius();
    void setHighlighted(bool highlited);
    void setIsNextStartNode(bool isNextStartNode);
    bool isSlave();
    bool isMaster();
    void setMaster(SkeletonEditNodeItem *nodeItem);
    void setSlave(SkeletonEditNodeItem *nodeItem);
    SkeletonEditNodeItem *master();
    SkeletonEditNodeItem *slave();
    SkeletonEditNodeItem *pair();
private:
    bool m_highlighted;
    bool m_isNextStartNode;
    SkeletonEditNodeItem *m_master;
    SkeletonEditNodeItem *m_slave;
    void updateBorder();
};

#endif

