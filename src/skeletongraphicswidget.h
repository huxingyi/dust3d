#ifndef SKELETON_GRAPHICS_VIEW_H
#define SKELETON_GRAPHICS_VIEW_H
#include <map>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QThread>
#include <cmath>
#include <set>
#include "skeletondocument.h"
#include "turnaroundloader.h"
#include "theme.h"

class SkeletonGraphicsSelectionItem : public QGraphicsRectItem
{
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

class SkeletonGraphicsNodeItem : public QGraphicsEllipseItem
{
public:
    SkeletonGraphicsNodeItem(SkeletonProfile profile=SkeletonProfile::Unknown) :
        m_profile(profile),
        m_hovered(false),
        m_checked(false)
    {
        setData(0, "node");
        setRadius(32);
    }
    void updateAppearance()
    {
        QColor color = Theme::white;
        
        switch (m_profile)
        {
        case SkeletonProfile::Unknown:
            break;
        case SkeletonProfile::Main:
            color = Theme::red;
            break;
        case SkeletonProfile::Side:
            color = Theme::green;
            break;
        }
        
        QColor penColor = color;
        penColor.setAlphaF(m_checked ? Theme::checkedAlpha : Theme::normalAlpha);
        QPen pen(penColor);
        pen.setWidth(0);
        setPen(pen);
        
        QColor brushColor = color;
        brushColor.setAlphaF((m_checked || m_hovered) ? Theme::fillAlpha : 0);
        QBrush brush(brushColor);
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
        if (radius < 4)
            radius = 4;
        QPointF oldOrigin = origin();
        setRect(oldOrigin.x() - radius, oldOrigin.y() - radius,
            radius * 2, radius * 2);
        updateAppearance();
    }
    SkeletonProfile profile()
    {
        return m_profile;
    }
    QUuid id()
    {
        return m_uuid;
    }
    void setId(QUuid id)
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
    bool checked()
    {
        return m_checked;
    }
    bool hovered()
    {
        return m_hovered;
    }
private:
    QUuid m_uuid;
    SkeletonProfile m_profile;
    bool m_hovered;
    bool m_checked;
};

class SkeletonGraphicsEdgeItem : public QGraphicsPolygonItem
{
public:
    SkeletonGraphicsEdgeItem() :
        m_firstItem(nullptr),
        m_secondItem(nullptr),
        m_hovered(false),
        m_checked(false),
        m_profile(SkeletonProfile::Unknown)
    {
        setData(0, "edge");
    }
    void setEndpoints(SkeletonGraphicsNodeItem *first, SkeletonGraphicsNodeItem *second)
    {
        m_firstItem = first;
        m_secondItem = second;
        updateAppearance();
    }
    SkeletonGraphicsNodeItem *firstItem()
    {
        return m_firstItem;
    }
    SkeletonGraphicsNodeItem *secondItem()
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
        polygon << line.p1() + offset1 << line.p1() + offset2 << line.p2() + offset2 << line.p2() + offset1;
        setPolygon(polygon);
        
        QColor color = Theme::white;
        
        switch (m_firstItem->profile())
        {
        case SkeletonProfile::Unknown:
            break;
        case SkeletonProfile::Main:
            color = Theme::red;
            break;
        case SkeletonProfile::Side:
            color = Theme::green;
            break;
        }
        
        QColor penColor = color;
        penColor.setAlphaF((m_checked || m_hovered) ? Theme::checkedAlpha : Theme::normalAlpha);
        QPen pen(penColor);
        pen.setWidth(0);
        setPen(pen);
    }
    QUuid id()
    {
        return m_uuid;
    }
    void setId(QUuid id)
    {
        m_uuid = id;
    }
    SkeletonProfile profile()
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
    QUuid m_uuid;
    SkeletonGraphicsNodeItem *m_firstItem;
    SkeletonGraphicsNodeItem *m_secondItem;
    QPolygonF m_selectionPolygon;
    bool m_hovered;
    bool m_checked;
    SkeletonProfile m_profile;
};

class SkeletonGraphicsFunctions
{
public:
    virtual bool mouseMove(QMouseEvent *event) = 0;
    virtual bool wheel(QWheelEvent *event) = 0;
    virtual bool mouseRelease(QMouseEvent *event) = 0;
    virtual bool mousePress(QMouseEvent *event) = 0;
    virtual bool mouseDoubleClick(QMouseEvent *event) = 0;
    virtual bool keyPress(QKeyEvent *event) = 0;
};

class SkeletonGraphicsWidget : public QGraphicsView, public SkeletonGraphicsFunctions
{
    Q_OBJECT
signals:
    void addNode(float x, float y, float z, float radius, QUuid fromNodeId);
    void scaleNodeByAddRadius(QUuid nodeId, float amount);
    void moveNodeBy(QUuid nodeId, float x, float y, float z);
    void removeNode(QUuid nodeId);
    void setEditMode(SkeletonDocumentEditMode mode);
    void removeEdge(QUuid edgeId);
    void addEdge(QUuid fromNodeId, QUuid toNodeId);
    void cursorChanged();
    void groupOperationAdded();
    void undo();
    void redo();
    void paste();
    void changeTurnaround();
    void batchChangeBegin();
    void batchChangeEnd();
public:
    SkeletonGraphicsWidget(const SkeletonDocument *document);
    std::map<QUuid, std::pair<SkeletonGraphicsNodeItem *, SkeletonGraphicsNodeItem *>> nodeItemMap;
    std::map<QUuid, std::pair<SkeletonGraphicsEdgeItem *, SkeletonGraphicsEdgeItem *>> edgeItemMap;
    bool mouseMove(QMouseEvent *event);
    bool wheel(QWheelEvent *event);
    bool mouseRelease(QMouseEvent *event);
    bool mousePress(QMouseEvent *event);
    bool mouseDoubleClick(QMouseEvent *event);
    bool keyPress(QKeyEvent *event);
    static bool checkSkeletonItem(QGraphicsItem *item, bool checked);
    static SkeletonProfile readSkeletonItemProfile(QGraphicsItem *item);
    void readMergedSkeletonNodeSetFromRangeSelection(std::set<SkeletonGraphicsNodeItem *> *nodeItemSet);
    void readSkeletonNodeAndEdgeIdSetFromRangeSelection(std::set<QUuid> *nodeIdSet, std::set<QUuid> *edgeIdSet);
    bool readSkeletonNodeAndAnyEdgeOfNodeFromRangeSelection(SkeletonGraphicsNodeItem **nodeItem, SkeletonGraphicsEdgeItem **edgeItem);
protected:
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);
public slots:
    void nodeAdded(QUuid nodeId);
    void edgeAdded(QUuid edgeId);
    void nodeRemoved(QUuid nodeId);
    void edgeRemoved(QUuid edgeId);
    void nodeRadiusChanged(QUuid nodeId);
    void nodeOriginChanged(QUuid nodeId);
    void edgeChanged(QUuid edgeId);
    void turnaroundChanged();
    void canvasResized();
    void editModeChanged();
    void updateCursor();
    void partVisibleStateChanged(QUuid partId);
    void showContextMenu(const QPoint &pos);
    void deleteSelected();
    void selectAll();
    void unselectAll();
    void selectPartAll();
    void cut();
    void copy();
    void flipHorizontally();
    void flipVertically();
private slots:
    void turnaroundImageReady();
private:
    QPointF mouseEventScenePos(QMouseEvent *event);
    QPointF scenePosToUnified(QPointF pos);
    QPointF scenePosFromUnified(QPointF pos);
    float sceneRadiusToUnified(float radius);
    float sceneRadiusFromUnified(float radius);
    void updateTurnaround();
    void updateItems();
    void checkRangeSelection();
    void clearRangeSelection();
    void removeItem(QGraphicsItem *item);
    QVector2D centerOfNodeItemSet(const std::set<SkeletonGraphicsNodeItem *> &set);
private:
    QGraphicsPixmapItem *m_backgroundItem;
    const SkeletonDocument *m_document;
    bool m_turnaroundChanged;
    TurnaroundLoader *m_turnaroundLoader;
    bool m_dragStarted;
    QPoint m_lastGlobalPos;
    bool m_moveStarted;
    QPointF m_lastScenePos;
    SkeletonGraphicsNodeItem *m_cursorNodeItem;
    SkeletonGraphicsEdgeItem *m_cursorEdgeItem;
    SkeletonGraphicsNodeItem *m_addFromNodeItem;
    SkeletonGraphicsNodeItem *m_hoveredNodeItem;
    SkeletonGraphicsEdgeItem *m_hoveredEdgeItem;
    float m_lastAddedX;
    float m_lastAddedY;
    float m_lastAddedZ;
    SkeletonGraphicsSelectionItem *m_selectionItem;
    QPointF m_rangeSelectionStartPos;
    bool m_rangeSelectionStarted;
    std::set<QGraphicsItem *> m_rangeSelectionSet;
    bool m_mouseEventFromSelf;
    bool m_moveHappened;
};

class SkeletonGraphicsContainerWidget : public QWidget
{
    Q_OBJECT
signals:
    void containerSizeChanged(QSize size);
public:
    SkeletonGraphicsContainerWidget() :
        m_graphicsWidget(nullptr)
    {
    }
    void resizeEvent(QResizeEvent *event) override
    {
        if (m_graphicsWidget && m_graphicsWidget->size() != event->size())
            emit containerSizeChanged(event->size());
    }
    void setGraphicsWidget(SkeletonGraphicsWidget *graphicsWidget)
    {
        m_graphicsWidget = graphicsWidget;
    }
private:
    SkeletonGraphicsWidget *m_graphicsWidget;
};

#endif
