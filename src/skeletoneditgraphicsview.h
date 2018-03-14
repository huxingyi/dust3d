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
    void changeTurnaroundTriggered();
public slots:
    void turnOffAddNodeMode();
    void turnOnAddNodeMode();
public:
    SkeletonEditGraphicsView(QWidget *parent = 0);
    void updateBackgroundImage(const QImage &image);
protected:
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
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
    bool m_isMovingNodeItem;
    bool m_backgroundLoaded;
private:
    void toggleAddNodeMode();
    void applyAddNodeMode();
    SkeletonEditNodeItem *findNodeItemByPos(QPointF pos);
    void setNextStartNodeItem(SkeletonEditNodeItem *item);
    float findXForSlave(float x);
    bool canNodeItemMoveTo(SkeletonEditNodeItem *item, QPointF moveTo);
    void AddItemRadius(QGraphicsEllipseItem *item, float delta);
    bool canAddItemRadius(QGraphicsEllipseItem *item, float delta);
    void adjustItems(QSizeF oldSceneSize, QSizeF newSceneSize);
};

#endif
