#ifndef SKELETON_EDIT_EDGE_ITEM_H
#define SKELETON_EDIT_EDGE_ITEM_H
#include <QGraphicsEllipseItem>
#include "skeletoneditnodeitem.h"

class SkeletonEditEdgeItem : public QGraphicsLineItem
{
public:
    SkeletonEditEdgeItem(QGraphicsItem *parent = 0);
    void setNodes(SkeletonEditNodeItem *first, SkeletonEditNodeItem *second);
    void updatePosition();
    SkeletonEditNodeItem *firstNode();
    SkeletonEditNodeItem *secondNode();
    bool connects(SkeletonEditNodeItem *nodeItem);
private:
    SkeletonEditNodeItem *m_firstNode;
    SkeletonEditNodeItem *m_secondNode;
};

#endif
