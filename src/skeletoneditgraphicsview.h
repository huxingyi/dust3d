#ifndef SKELETON_EDIT_GRAPHICS_VIEW_H
#define SKELETON_EDIT_GRAPHICS_VIEW_H
#include <QGraphicsView>
#include <QMouseEvent>
#include "skeletoneditnodeitem.h"
#include "skeletoneditedgeitem.h"

class SkeletonEditGraphicsView : public QGraphicsView
{
    Q_OBJECT
signals:
    void nodesChanged();
public slots:
    void turnOffAddNodeMode();
    void turnOnAddNodeMode();
public:
    SkeletonEditGraphicsView(QWidget *parent = 0);
protected:
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
private:
    QGraphicsPixmapItem *m_backgroundItem;
    QGraphicsEllipseItem *m_pendingNodeItem;
    QGraphicsLineItem *m_pendingEdgeItem;
    static qreal m_initialNodeSize;
    static qreal m_minimalNodeSize;
    bool m_inAddNodeMode;
    SkeletonEditNodeItem *m_nextStartNodeItem;
    SkeletonEditNodeItem *m_lastHoverNodeItem;
    QPointF m_lastMousePos;
    void toggleAddNodeMode();
    void applyAddNodeMode();
    SkeletonEditNodeItem *findNodeItemByPos(QPointF pos);
    void setNextStartNodeItem(SkeletonEditNodeItem *item);
    float findXForSlave(float x);
    
};

#endif
