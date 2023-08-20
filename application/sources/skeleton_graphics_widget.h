#ifndef DUST3D_APPLICATION_SKELETON_GRAPHICS_WIDGET_H_
#define DUST3D_APPLICATION_SKELETON_GRAPHICS_WIDGET_H_

#include "document.h"
#include "model_widget.h"
#include "skeleton_ik_mover.h"
#include "theme.h"
#include "turnaround_loader.h"
#include <QGraphicsPixmapItem>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QThread>
#include <QTimer>
#include <cmath>
#include <map>
#include <set>

class SkeletonGraphicsEdgeItem;
class SkeletonGraphicsNodeItem;
class SkeletonGraphicsOriginItem;
class SkeletonGraphicsSelectionItem;

class SkeletonGraphicsWidget : public QGraphicsView {
    Q_OBJECT
public:
    ~SkeletonGraphicsWidget();
signals:
    void addNode(float x, float y, float z, float radius, dust3d::Uuid fromNodeId);
    void scaleNodeByAddRadius(dust3d::Uuid nodeId, float amount);
    void moveNodeBy(dust3d::Uuid nodeId, float x, float y, float z);
    void removeNode(dust3d::Uuid nodeId);
    void removePart(dust3d::Uuid partId);
    void setEditMode(Document::EditMode mode);
    void removeEdge(dust3d::Uuid edgeId);
    void addEdge(dust3d::Uuid fromNodeId, dust3d::Uuid toNodeId);
    void cursorChanged();
    void groupOperationAdded();
    void undo();
    void redo();
    void paste();
    void changeTurnaround();
    void batchChangeBegin();
    void batchChangeEnd();
    void open();
    void exportResult();
    void breakEdge(dust3d::Uuid edgeId);
    void reduceNode(dust3d::Uuid nodeId);
    void reverseEdge(dust3d::Uuid edgeId);
    void moveOriginBy(float x, float y, float z);
    void partChecked(dust3d::Uuid partId);
    void partUnchecked(dust3d::Uuid partId);
    void setPartLockState(dust3d::Uuid partId, bool locked);
    void setPartVisibleState(dust3d::Uuid partId, bool visible);
    void setPartSubdivState(dust3d::Uuid partId, bool subdived);
    void setPartDisableState(dust3d::Uuid partId, bool disabled);
    void setPartXmirrorState(dust3d::Uuid partId, bool mirrored);
    void setPartRoundState(dust3d::Uuid partId, bool rounded);
    void setPartWrapState(dust3d::Uuid partId, bool wrapped);
    void setPartChamferState(dust3d::Uuid partId, bool chamfered);
    void setPartColorState(dust3d::Uuid partId, bool hasColor, QColor color);
    void setXlockState(bool locked);
    void setYlockState(bool locked);
    void setZlockState(bool locked);
    void setNodeOrigin(dust3d::Uuid nodeId, float x, float y, float z);
    void zoomRenderedModelBy(float delta);
    void switchNodeXZ(dust3d::Uuid nodeId);
    void enableAllPositionRelatedLocks();
    void disableAllPositionRelatedLocks();
    void shortcutToggleWireframe();
    void partComponentChecked(dust3d::Uuid partId);
    void showOrHideAllComponents();
    void shortcutToggleFlatShading();
    void shortcutToggleRotation();
    void loadedTurnaroundImageChanged();
    void nodePicked(const dust3d::Uuid& nodeId);

public:
    SkeletonGraphicsWidget(const Document* document);
    std::map<dust3d::Uuid, std::pair<SkeletonGraphicsNodeItem*, SkeletonGraphicsNodeItem*>> nodeItemMap;
    std::map<dust3d::Uuid, std::pair<SkeletonGraphicsEdgeItem*, SkeletonGraphicsEdgeItem*>> edgeItemMap;
    bool mouseMove(QMouseEvent* event);
    bool wheel(QWheelEvent* event);
    bool mouseRelease(QMouseEvent* event);
    bool mousePress(QMouseEvent* event);
    bool mouseDoubleClick(QMouseEvent* event);
    bool keyPress(QKeyEvent* event);
    bool keyRelease(QKeyEvent* event);
    bool checkSkeletonItem(QGraphicsItem* item, bool checked);
    dust3d::Uuid querySkeletonItemPartId(QGraphicsItem* item);
    static Document::Profile readSkeletonItemProfile(QGraphicsItem* item);
    void getOtherProfileNodeItems(const std::set<SkeletonGraphicsNodeItem*>& nodeItemSet,
        std::set<SkeletonGraphicsNodeItem*>* otherProfileNodeItemSet);
    void readMergedSkeletonNodeSetFromRangeSelection(std::set<SkeletonGraphicsNodeItem*>* nodeItemSet);
    void readSkeletonNodeAndEdgeIdSetFromRangeSelection(std::set<dust3d::Uuid>* nodeIdSet, std::set<dust3d::Uuid>* edgeIdSet = nullptr);
    bool readSkeletonNodeAndAnyEdgeOfNodeFromRangeSelection(SkeletonGraphicsNodeItem** nodeItem, SkeletonGraphicsEdgeItem** edgeItem);
    bool hasSelection();
    bool hasItems();
    bool hasMultipleSelection();
    bool hasEdgeSelection();
    bool hasNodeSelection();
    bool hasTwoDisconnectedNodesSelection();
    void setModelWidget(ModelWidget* modelWidget);
    bool inputWheelEventFromOtherWidget(QWheelEvent* event);
    bool rotated();
    const QImage* loadedTurnaroundImage() const;

protected:
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
public slots:
    void nodeAdded(dust3d::Uuid nodeId);
    void edgeAdded(dust3d::Uuid edgeId);
    void nodeRemoved(dust3d::Uuid nodeId);
    void edgeRemoved(dust3d::Uuid edgeId);
    void nodeRadiusChanged(dust3d::Uuid nodeId);
    void nodeOriginChanged(dust3d::Uuid nodeId);
    void edgeReversed(dust3d::Uuid edgeId);
    void edgeNodeChanged(const dust3d::Uuid& edgeId);
    void turnaroundChanged();
    void canvasResized();
    void editModeChanged();
    void updateCursor();
    void partVisibleStateChanged(dust3d::Uuid partId);
    void showContextMenu(const QPoint& pos);
    void deleteSelected();
    void selectAll();
    void unselectAll();
    void selectPartAll();
    void selectConnectedAll();
    void addPartToSelection(dust3d::Uuid partId);
    void addNodeToSelection(const dust3d::Uuid& nodeId);
    void cut();
    void copy();
    void flipHorizontally();
    void flipVertically();
    void rotateClockwise90Degree();
    void rotateCounterclockwise90Degree();
    void rotateAllMainProfileClockwise90DegreeAlongOrigin();
    void rotateAllMainProfileCounterclockwise90DegreeAlongOrigin();
    void removeAllContent();
    void fadeSelected();
    void colorizeSelected();
    void breakSelected();
    void reduceSelected();
    void reverseSelectedEdges();
    void connectSelected();
    void rotateSelected(int degree);
    void zoomSelected(float delta);
    void scaleSelected(float delta);
    void moveSelected(float byX, float byY);
    void moveCheckedOrigin(float byX, float byY);
    void originChanged();
    void alignSelectedToGlobalCenter();
    void alignSelectedToGlobalVerticalCenter();
    void alignSelectedToGlobalHorizontalCenter();
    void alignSelectedToLocalCenter();
    void alignSelectedToLocalVerticalCenter();
    void alignSelectedToLocalHorizontalCenter();
    void selectPartAllById(dust3d::Uuid partId);
    void addSelectNode(dust3d::Uuid nodeId);
    void addSelectEdge(dust3d::Uuid edgeId);
    void enableBackgroundBlur();
    void disableBackgroundBlur();
    void setBackgroundBlur(float turnaroundOpacity);
    void ikMove(dust3d::Uuid endEffectorId, QVector3D target);
    void ikMoveReady();
    void timeToRemoveDeferredNodesAndEdges();
    void switchSelectedXZ();
    void setRotated(bool rotated);
    void shortcutDelete();
    void shortcutAddMode();
    void shortcutUndo();
    void shortcutRedo();
    void shortcutXlock();
    void shortcutYlock();
    void shortcutZlock();
    void shortcutCut();
    void shortcutCopy();
    void shortcutPaste();
    void shortcutSelectMode();
    void shortcutZoomRenderedModelByMinus10();
    void shortcutZoomSelectedByMinus1();
    void shortcutZoomRenderedModelBy10();
    void shortcutZoomSelectedBy1();
    void shortcutRotateSelectedByMinus1();
    void shortcutRotateSelectedBy1();
    void shortcutMoveSelectedToLeft();
    void shortcutMoveSelectedToRight();
    void shortcutMoveSelectedToUp();
    void shortcutMoveSelectedToDown();
    void shortcutScaleSelectedByMinus1();
    void shortcutScaleSelectedBy1();
    void shortcutSwitchProfileOnSelected();
    void shortcutShowOrHideSelectedPart();
    void shortcutEnableOrDisableSelectedPart();
    void shortcutLockOrUnlockSelectedPart();
    void shortcutXmirrorOnOrOffSelectedPart();
    void shortcutSubdivedOrNotSelectedPart();
    void shortcutRoundEndOrNotSelectedPart();
    void shortcutCheckPartComponent();
    void shortcutChamferedOrNotSelectedPart();
    void shortcutSelectAll();
    void shortcutEscape();
    void clearRangeSelection();
private slots:
    void turnaroundImageReady();

private:
    QPointF mouseEventScenePos(QMouseEvent* event);
    QPointF scenePosToUnified(QPointF pos);
    QPointF scenePosFromUnified(QPointF pos);
    float sceneRadiusToUnified(float radius);
    float sceneRadiusFromUnified(float radius);
    void updateTurnaround();
    void updateItems();
    void checkRangeSelection();
    void removeItem(QGraphicsItem* item);
    QVector2D centerOfNodeItemSet(const std::set<SkeletonGraphicsNodeItem*>& set);
    bool isSingleNodeSelected();
    void addItemToRangeSelection(QGraphicsItem* item);
    void removeItemFromRangeSelection(QGraphicsItem* item);
    void hoverPart(dust3d::Uuid partId);
    void switchProfileOnRangeSelection();
    void setItemHoveredOnAllProfiles(QGraphicsItem* item, bool hovered);
    void alignSelectedToGlobal(bool alignToVerticalCenter, bool alignToHorizontalCenter);
    void alignSelectedToLocal(bool alignToVerticalCenter, bool alignToHorizontalCenter);
    void rotateItems(const std::set<SkeletonGraphicsNodeItem*>& nodeItems, int degree, QVector2D center);
    void rotateAllSideProfile(int degree);
    bool isFloatEqual(float a, float b);

private:
    const Document* m_document = nullptr;
    QGraphicsPixmapItem* m_backgroundItem = nullptr;
    bool m_turnaroundChanged = false;
    TurnaroundLoader* m_turnaroundLoader = nullptr;
    bool m_dragStarted = false;
    bool m_moveStarted = false;
    SkeletonGraphicsNodeItem* m_cursorNodeItem = nullptr;
    SkeletonGraphicsEdgeItem* m_cursorEdgeItem = nullptr;
    SkeletonGraphicsNodeItem* m_addFromNodeItem = nullptr;
    SkeletonGraphicsNodeItem* m_hoveredNodeItem = nullptr;
    SkeletonGraphicsEdgeItem* m_hoveredEdgeItem = nullptr;
    float m_lastAddedX = false;
    float m_lastAddedY = false;
    float m_lastAddedZ = false;
    SkeletonGraphicsSelectionItem* m_selectionItem = nullptr;
    bool m_rangeSelectionStarted = false;
    bool m_mouseEventFromSelf = false;
    bool m_moveHappened = false;
    int m_lastRot = 0;
    SkeletonGraphicsOriginItem* m_mainOriginItem = nullptr;
    SkeletonGraphicsOriginItem* m_sideOriginItem = nullptr;
    SkeletonGraphicsOriginItem* m_hoveredOriginItem = nullptr;
    SkeletonGraphicsOriginItem* m_checkedOriginItem = nullptr;
    unsigned long long m_ikMoveUpdateVersion = 0;
    SkeletonIkMover* m_ikMover = nullptr;
    QTimer* m_deferredRemoveTimer = nullptr;
    bool m_eventForwardingToModelWidget = false;
    ModelWidget* m_modelWidget = nullptr;
    float m_turnaroundOpacity = 0.25f;
    bool m_rotated = false;
    QImage* m_backgroundImage = nullptr;
    QVector3D m_ikMoveTarget;
    dust3d::Uuid m_ikMoveEndEffectorId;
    std::set<QGraphicsItem*> m_rangeSelectionSet;
    QPoint m_lastGlobalPos;
    QPointF m_lastScenePos;
    QPointF m_rangeSelectionStartPos;
    dust3d::Uuid m_lastCheckedPart;
    std::set<dust3d::Uuid> m_deferredRemoveNodeIds;
    std::set<dust3d::Uuid> m_deferredRemoveEdgeIds;
    std::unique_ptr<QMenu> m_contextMenu;
};

class SkeletonGraphicsContainerWidget : public QWidget {
    Q_OBJECT
signals:
    void containerSizeChanged(QSize size);

public:
    SkeletonGraphicsContainerWidget()
        : m_graphicsWidget(nullptr)
    {
    }
    void resizeEvent(QResizeEvent* event) override
    {
        if (m_graphicsWidget && m_graphicsWidget->size() != event->size())
            emit containerSizeChanged(event->size());
    }
    void setGraphicsWidget(SkeletonGraphicsWidget* graphicsWidget)
    {
        m_graphicsWidget = graphicsWidget;
    }

private:
    SkeletonGraphicsWidget* m_graphicsWidget;
};

#endif
