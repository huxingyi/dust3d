#ifndef DUST3D_APPLICATION_SKELETON_GRAPHICS_VIEW_H_
#define DUST3D_APPLICATION_SKELETON_GRAPHICS_VIEW_H_

#include "document.h"
#include "model_widget.h"
#include "skeleton_ik_mover.h"
#include "theme.h"
#include "turnaround_loader.h"
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QThread>
#include <QTimer>
#include <cmath>
#include <map>
#include <set>

class SkeletonGraphicsOriginItem : public QGraphicsPolygonItem {
public:
    SkeletonGraphicsOriginItem(Document::Profile profile = Document::Profile::Unknown)
        : m_profile(profile)
        , m_hovered(false)
        , m_checked(false)
        , m_rotated(false)
    {
        setData(0, "origin");
    }
    void setRotated(bool rotated)
    {
        m_rotated = rotated;
    }
    void updateAppearance()
    {
        QColor color = Theme::white;

        switch (m_profile) {
        case Document::Profile::Unknown:
            break;
        case Document::Profile::Main:
            color = m_rotated ? Theme::blue : Theme::red;
            break;
        case Document::Profile::Side:
            color = Theme::green;
            break;
        }

        QColor penColor = color;
        penColor.setAlphaF(m_checked ? Theme::checkedAlpha : Theme::normalAlpha);
        QPen pen(penColor);
        pen.setWidth(0);
        setPen(pen);

        QColor brushColor = color;
        brushColor.setAlphaF((m_checked || m_hovered) ? Theme::checkedAlpha : Theme::normalAlpha);
        QBrush brush(brushColor);
        setBrush(brush);
    }
    void setOrigin(QPointF point)
    {
        m_origin = point;
        QPolygonF triangle;
        const int triangleRadius = 10;
        triangle.append(QPointF(point.x() - triangleRadius, point.y()));
        triangle.append(QPointF(point.x() + triangleRadius, point.y()));
        triangle.append(QPointF(point.x(), point.y() - triangleRadius));
        triangle.append(QPointF(point.x() - triangleRadius, point.y()));
        setPolygon(triangle);
        updateAppearance();
    }
    QPointF origin()
    {
        return m_origin;
    }
    Document::Profile profile()
    {
        return m_profile;
    }
    void setHovered(bool hovered)
    {
        m_hovered = hovered;
        updateAppearance();
    }
    void setChecked(bool checked)
    {
        m_checked = checked;
        updateAppearance();
    }
    bool checked()
    {
        return m_checked;
    }
    bool hovered()
    {
        return m_hovered;
    }

private:
    Document::Profile m_profile;
    bool m_hovered;
    bool m_checked;
    QPointF m_origin;
    bool m_rotated;
};

class SkeletonGraphicsSelectionItem : public QGraphicsRectItem {
public:
    SkeletonGraphicsSelectionItem()
    {
        QColor penColor = Theme::white;
        QPen pen(penColor);
        pen.setWidth(0);
        pen.setStyle(Qt::DashLine);
        setPen(pen);
    }
    void updateRange(QPointF beginPos, QPointF endPos)
    {
        setRect(QRectF(beginPos, endPos).normalized());
    }
};

class SkeletonGraphicsNodeItem : public QGraphicsEllipseItem {
public:
    SkeletonGraphicsNodeItem(Document::Profile profile = Document::Profile::Unknown)
        : m_profile(profile)
        , m_hovered(false)
        , m_checked(false)
        , m_markColor(Qt::transparent)
        , m_deactivated(false)
        , m_rotated(false)
    {
        setData(0, "node");
        setRadius(32);
    }
    void setRotated(bool rotated)
    {
        m_rotated = rotated;
    }
    void updateAppearance()
    {
        QColor color = Qt::gray;

        if (!m_deactivated) {
            switch (m_profile) {
            case Document::Profile::Unknown:
                break;
            case Document::Profile::Main:
                color = m_rotated ? Theme::blue : Theme::red;
                break;
            case Document::Profile::Side:
                color = Theme::green;
                break;
            }
        }

        QColor penColor = color;
        penColor.setAlphaF((m_checked || m_hovered) ? Theme::checkedAlpha : Theme::normalAlpha);
        QPen pen(penColor);
        pen.setWidth(0);
        setPen(pen);

        QColor brushColor;
        Qt::BrushStyle style;
        if (m_markColor == Qt::transparent) {
            brushColor = color;
            brushColor.setAlphaF((m_checked || m_hovered) ? Theme::fillAlpha : 0);
            style = Qt::SolidPattern;
        } else {
            brushColor = m_markColor;
            brushColor.setAlphaF((m_checked || m_hovered) ? Theme::fillAlpha * 4 : Theme::fillAlpha * 1.5);
            style = Qt::Dense4Pattern;
        }
        if (m_checked)
            brushColor.setAlphaF(brushColor.alphaF() * 1.2);
        QBrush brush(brushColor);
        brush.setStyle(style);
        setBrush(brush);
    }
    void setOrigin(QPointF point)
    {
        QPointF moveBy = point - origin();
        QRectF newRect = rect();
        newRect.adjust(moveBy.x(), moveBy.y(), moveBy.x(), moveBy.y());
        setRect(newRect);
        updateAppearance();
    }
    QPointF origin()
    {
        return QPointF(rect().x() + rect().width() / 2,
            rect().y() + rect().height() / 2);
    }
    float radius()
    {
        return rect().width() / 2;
    }
    void setRadius(float radius)
    {
        if (radius < 1)
            radius = 1;
        QPointF oldOrigin = origin();
        setRect(oldOrigin.x() - radius, oldOrigin.y() - radius,
            radius * 2, radius * 2);
        updateAppearance();
    }
    void setMarkColor(QColor color)
    {
        m_markColor = color;
        updateAppearance();
    }
    Document::Profile profile()
    {
        return m_profile;
    }
    dust3d::Uuid id()
    {
        return m_uuid;
    }
    void setId(dust3d::Uuid id)
    {
        m_uuid = id;
    }
    void setHovered(bool hovered)
    {
        m_hovered = hovered;
        updateAppearance();
    }
    void setChecked(bool checked)
    {
        m_checked = checked;
        updateAppearance();
    }
    void setDeactivated(bool deactivated)
    {
        m_deactivated = deactivated;
        updateAppearance();
    }
    bool deactivated()
    {
        return m_deactivated;
    }
    bool checked()
    {
        return m_checked;
    }
    bool hovered()
    {
        return m_hovered;
    }

private:
    dust3d::Uuid m_uuid;
    Document::Profile m_profile;
    bool m_hovered;
    bool m_checked;
    QColor m_markColor;
    bool m_deactivated;
    bool m_rotated;
};

class SkeletonGraphicsEdgeItem : public QGraphicsPolygonItem {
public:
    SkeletonGraphicsEdgeItem()
        : m_firstItem(nullptr)
        , m_secondItem(nullptr)
        , m_hovered(false)
        , m_checked(false)
        , m_profile(Document::Profile::Unknown)
        , m_deactivated(false)
        , m_rotated(false)
    {
        setData(0, "edge");
    }
    void setRotated(bool rotated)
    {
        m_rotated = rotated;
    }
    void setEndpoints(SkeletonGraphicsNodeItem* first, SkeletonGraphicsNodeItem* second)
    {
        m_firstItem = first;
        m_secondItem = second;
        updateAppearance();
    }
    SkeletonGraphicsNodeItem* firstItem()
    {
        return m_firstItem;
    }
    SkeletonGraphicsNodeItem* secondItem()
    {
        return m_secondItem;
    }
    void updateAppearance()
    {
        if (nullptr == m_firstItem || nullptr == m_secondItem)
            return;

        m_profile = m_firstItem->profile();

        QLineF line(m_firstItem->origin(), m_secondItem->origin());

        QPolygonF polygon;
        float radAngle = line.angle() * M_PI / 180;
        float dx = 2 * sin(radAngle);
        float dy = 2 * cos(radAngle);
        QPointF offset1 = QPointF(dx, dy);
        QPointF offset2 = QPointF(-dx, -dy);
        //polygon << line.p1() + offset1 << line.p1() + offset2 << line.p2() + offset2 << line.p2() + offset1;
        polygon << line.p1() + offset1 << line.p1() + offset2 << line.p2();
        setPolygon(polygon);

        QColor color = Qt::gray;

        if (!m_deactivated) {
            switch (m_firstItem->profile()) {
            case Document::Profile::Unknown:
                break;
            case Document::Profile::Main:
                color = m_rotated ? Theme::blue : Theme::red;
                break;
            case Document::Profile::Side:
                color = Theme::green;
                break;
            }
        }

        QColor penColor = color;
        penColor.setAlphaF((m_checked || m_hovered) ? Theme::checkedAlpha : Theme::normalAlpha);
        QPen pen(penColor);
        pen.setWidth(0);
        setPen(pen);
    }
    dust3d::Uuid id()
    {
        return m_uuid;
    }
    void setId(dust3d::Uuid id)
    {
        m_uuid = id;
    }
    Document::Profile profile()
    {
        return m_profile;
    }
    void setHovered(bool hovered)
    {
        m_hovered = hovered;
        updateAppearance();
    }
    void setChecked(bool checked)
    {
        m_checked = checked;
        updateAppearance();
    }
    void setDeactivated(bool deactivated)
    {
        m_deactivated = deactivated;
        updateAppearance();
    }
    void reverse()
    {
        std::swap(m_firstItem, m_secondItem);
        updateAppearance();
    }
    bool deactivated()
    {
        return m_deactivated;
    }
    bool checked()
    {
        return m_checked;
    }
    bool hovered()
    {
        return m_hovered;
    }

private:
    dust3d::Uuid m_uuid;
    SkeletonGraphicsNodeItem* m_firstItem;
    SkeletonGraphicsNodeItem* m_secondItem;
    QPolygonF m_selectionPolygon;
    bool m_hovered;
    bool m_checked;
    Document::Profile m_profile;
    bool m_deactivated;
    bool m_rotated;
};

class SkeletonGraphicsWidget : public QGraphicsView {
    Q_OBJECT
public:
    ~SkeletonGraphicsWidget();
signals:
    void addNode(float x, float y, float z, float radius, dust3d::Uuid fromNodeId);
    void scaleNodeByAddRadius(dust3d::Uuid nodeId, float amount);
    void moveNodeBy(dust3d::Uuid nodeId, float x, float y, float z);
    void setNodeBoneJointState(const dust3d::Uuid& nodeId, bool boneJoint);
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
    void shortcutPaintMode();
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
    void markSelectedAsBoneJoint();
    void markSelectedAsNotBoneJoint();
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
    void clearRangeSelection();
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
    void setNodeBoneJointStates(const std::vector<dust3d::Uuid>& nodeIds, bool boneJoint);

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
    bool m_inTempDragMode = false;
    Document::EditMode m_modeBeforeEnterTempDragMode = Document::EditMode::Select;
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
