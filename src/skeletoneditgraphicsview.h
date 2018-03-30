#ifndef SKELETON_EDIT_GRAPHICS_VIEW_H
#define SKELETON_EDIT_GRAPHICS_VIEW_H
#include <QGraphicsView>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QGraphicsProxyWidget>
#include "skeletoneditnodeitem.h"
#include "skeletoneditedgeitem.h"
#include "skeletonsnapshot.h"
#include "modelwidget.h"

class SkeletonEditGraphicsView : public QGraphicsView
{
    Q_OBJECT
signals:
    void sizeChanged();
    void nodesChanged();
    void changeTurnaroundTriggered();
    void edgeCheckStateChanged();
public slots:
    void turnOffAddNodeMode();
    void turnOnAddNodeMode();
    void markAsBranch();
    void markAsTrunk();
    void markAsRoot();
    void markAsChild();
    bool mouseMove(QMouseEvent *event, const QPointF &scenePos);
    bool wheel(QWheelEvent *event, const QPointF &scenePos);
    bool mouseRelease(QMouseEvent *event, const QPointF &scenePos);
    bool mousePress(QMouseEvent *event, const QPointF &scenePos);
    bool mouseDoubleClick(QMouseEvent *event, const QPointF &scenePos);
    bool keyPress(QKeyEvent *event, const QPointF &scenePos);
public:
    SkeletonEditGraphicsView(QWidget *parent = 0);
    void updateBackgroundImage(const QImage &image);
    bool hasBackgroundImage();
    QPixmap backgroundImage();
    void saveToSnapshot(SkeletonSnapshot *snapshot);
    void loadFromSnapshot(SkeletonSnapshot *snapshot);
    ModelWidget *modelWidget();
    bool combineEnabled();
    bool unionEnabled();
    bool subdivEnabled();
protected:
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void resizeEvent(QResizeEvent *event);
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
    ModelWidget *m_modelWidget;
    QGraphicsProxyWidget *m_modelWidgetProxy;
    bool m_combineEnabled;
    bool m_unionEnabled;
    bool m_subdivEnabled;
private:
    void toggleAddNodeMode();
    void applyAddNodeMode();
    SkeletonEditNodeItem *findNodeItemByPos(QPointF pos);
    SkeletonEditEdgeItem *findEdgeItemByPos(QPointF pos);
    SkeletonEditEdgeItem *findEdgeItemByNodePair(SkeletonEditNodeItem *first,
        SkeletonEditNodeItem *second);
    void setNextStartNodeItem(SkeletonEditNodeItem *item);
    bool canNodeItemMoveTo(SkeletonEditNodeItem *item, QPointF moveTo);
    void AddItemRadius(QGraphicsEllipseItem *item, float delta);
    bool canAddItemRadius(QGraphicsEllipseItem *item, float delta);
    void adjustItems(QSizeF oldSceneSize, QSizeF newSceneSize);
    void removeSelectedItems();
    void removeNodeItem(SkeletonEditNodeItem *nodeItem);
    void removeNodeItemAndSidePairs(SkeletonEditNodeItem *nodeItem);
    void fetchNodeItemAndAllSidePairs(SkeletonEditNodeItem *nodeItem, std::vector<SkeletonEditNodeItem *> *sidePairs);
    SkeletonEditNodeItem *addNodeItemAndSidePairs(QRectF area, SkeletonEditNodeItem *fromNodeItem, const QString &sideColorName="red");
    SkeletonEditNodeItem *addNodeItem(float originX, float originY, float radius);
    void addEdgeItem(SkeletonEditNodeItem *first, SkeletonEditNodeItem *second);
};

#endif
