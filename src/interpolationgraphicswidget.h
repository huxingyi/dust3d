#ifndef INTERPOLATION_GRAPHICS_WIDGET_H
#define INTERPOLATION_GRAPHICS_WIDGET_H
#include <QGraphicsView>
#include <QBrush>
#include <QPen>
#include <QGraphicsItemGroup>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QPointF>
#include <QResizeEvent>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsProxyWidget>
#include <QGraphicsWidget>
#include "curveutil.h"
#include "theme.h"

class InterpolationGraphicsProxyObject : public QObject
{
    Q_OBJECT
signals:
    void itemMoved(QPointF point);
    void itemHoverEnter();
    void itemHoverLeave();
};

class InterpolationGraphicsCircleItem : public QGraphicsEllipseItem
{
public:
    InterpolationGraphicsCircleItem(float toRadius, bool limitMoveRange=true) :
        m_limitMoveRange(limitMoveRange)
    {
        setAcceptHoverEvents(true);
        setRect(QRectF(-toRadius, -toRadius, toRadius * 2, toRadius * 2));
        updateAppearance();
    }
    ~InterpolationGraphicsCircleItem()
    {
        delete m_object;
    }
    InterpolationGraphicsProxyObject *proxy()
    {
        return m_object;
    }
    void setOrigin(QPointF point)
    {
        m_disableEmitSignal = true;
        setPos(point.x(), point.y());
        m_disableEmitSignal = false;
    }
    QPointF origin()
    {
        return QPointF(pos().x(), pos().y());
    }
    float radius()
    {
        return rect().width() / 2;
    }
    void setChecked(bool checked)
    {
        m_checked = checked;
        updateAppearance();
    }
protected:
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value) override
    {
        switch (change) {
        case ItemPositionHasChanged:
            {
                QPointF newPos = value.toPointF();
                if (m_limitMoveRange) {
                    QRectF rect = scene()->sceneRect();
                    if (!rect.contains(newPos)) {
                        newPos.setX(qMin(rect.right(), qMax(newPos.x(), rect.left())));
                        newPos.setY(qMin(rect.bottom(), qMax(newPos.y(), rect.top())));
                    }
                }
                if (!m_disableEmitSignal)
                    emit m_object->itemMoved(newPos);
                return newPos;
            }
            break;
        default:
            break;
        };
        return QGraphicsItem::itemChange(change, value);
    }
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override
    {
        QGraphicsItem::hoverEnterEvent(event);
        m_hovered = true;
        updateAppearance();
        emit m_object->itemHoverEnter();
    }
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override
    {
        QGraphicsItem::hoverLeaveEvent(event);
        m_hovered = false;
        updateAppearance();
        emit m_object->itemHoverLeave();
    }
private:
    InterpolationGraphicsProxyObject *m_object = new InterpolationGraphicsProxyObject();
    bool m_hovered = false;
    bool m_checked = false;
    bool m_limitMoveRange = true;
    bool m_disableEmitSignal = false;
    void updateAppearance()
    {
        QColor color = Theme::white;
        if (m_hovered || m_checked) {
            setBrush(color);
        } else {
            setBrush(Qt::transparent);
        }
        QPen pen;
        pen.setWidth(0);
        pen.setColor(color);
        setPen(pen);
    }
};

class InterpolationGraphicsEdgeItem : public QGraphicsLineItem
{
public:
    InterpolationGraphicsEdgeItem()
    {
        QPen linePen;
        linePen.setColor(Theme::white);
        linePen.setWidth(0);
        setPen(linePen);
    }
};

class InterpolationGraphicsLabelParentWidget : public QGraphicsWidget
{
public:
    InterpolationGraphicsLabelParentWidget()
    {
        setAcceptHoverEvents(true);
    }
    ~InterpolationGraphicsLabelParentWidget()
    {
        delete m_object;
    }
    InterpolationGraphicsProxyObject *proxy()
    {
        return m_object;
    }
    void setPosWithoutEmitSignal(QPointF pos)
    {
        m_disableEmitSignal = true;
        setPos(pos);
        m_disableEmitSignal = false;
    }
protected:
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value) override
    {
        switch (change) {
        case ItemPositionHasChanged:
            {
                QPointF newPos = value.toPointF();
                QRectF rect = scene()->sceneRect();
                if (!rect.contains(newPos)) {
                    newPos.setX(qMin(rect.right(), qMax(newPos.x(), rect.left())));
                    newPos.setY(qMin(rect.bottom(), qMax(newPos.y(), rect.top())));
                }
                if (!m_disableEmitSignal)
                    emit m_object->itemMoved(newPos);
                return newPos;
            }
            break;
        default:
            break;
        };
        return QGraphicsItem::itemChange(change, value);
    }
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override
    {
        QGraphicsWidget::hoverEnterEvent(event);
        emit m_object->itemHoverEnter();
    }
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override
    {
        QGraphicsWidget::hoverLeaveEvent(event);
        emit m_object->itemHoverLeave();
    }
private:
    InterpolationGraphicsProxyObject *m_object = new InterpolationGraphicsProxyObject();
    bool m_disableEmitSignal = false;
};

class InterpolationGraphicsCursorItem : public QGraphicsRectItem
{
public:
    InterpolationGraphicsCursorItem()
    {
        QPen linePen;
        linePen.setColor(Theme::red);
        linePen.setWidth(0);
        setPen(linePen);
    }
    void updateCursorPosition(const QPointF &pos)
    {
        QPointF newPos = pos;
        QRectF rect = scene()->sceneRect();
        if (!rect.contains(newPos)) {
            newPos.setX(qMin(rect.right(), qMax(newPos.x(), rect.left())));
            newPos.setY(qMin(rect.bottom(), qMax(newPos.y(), rect.top())));
        }
        setPos(newPos);
    }
};

class InterpolationGraphicsKeyframeItem : public QGraphicsRectItem
{
public:
    InterpolationGraphicsKeyframeItem()
    {
        QPen linePen;
        linePen.setColor(Theme::red);
        linePen.setWidth(0);
        setPen(linePen);
        
        setBrush(QBrush(Theme::red));
        
        setRect(-4, -4, 8, 8);
    }
};

class InterpolationGraphicsWidget : public QGraphicsView
{
    Q_OBJECT
signals:
    void controlNodesChanged();
    void keyframeKnotChanged(int index, float knot);
    void removeKeyframe(int index);
public:
    InterpolationGraphicsWidget(QWidget *parent=nullptr);
    void toNormalizedControlNodes(std::vector<HermiteControlNode> &snapshot);
    void fromNormalizedControlNodes(const std::vector<HermiteControlNode> &snapshot);
    float cursorKnot() const;
public slots:
    void setControlNodes(const std::vector<HermiteControlNode> &nodes);
    void setKeyframes(const std::vector<std::pair<float, QString>> &keyframes);
    void refresh();
    void refreshControlNode(int index);
    void refreshControlAnchor(int index);
    void refreshControlEdges(int index);
    void refreshControlHandles(int index);
    void refreshCurve();
    void refreshKeyframe(int index);
    void refreshKeyframeKnot(int index);
    void refreshKeyframeLabel(int index);
    void refreshKeyframes();
    void showContextMenu(const QPoint &pos);
    void deleteControlNode(int index);
    void deleteKeyframe(int index);
    void addControlNodeAfter(int index);
    void addControlNodeBefore(int index);
    void addControlNodeAtPosition(const QPointF &pos);
    void hoverControlNode(int index);
    void unhoverControlNode(int index);
    void selectControlNode(int index);
    void selectKeyframe(int index);
    void hoverKeyframe(int index);
    void unhoverKeyframe(int index);
    void invalidateControlSelection();
    void invalidateKeyframeSelection();
    void setPreviewOnly(bool previewOnly);
protected:
    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
private:
    std::vector<HermiteControlNode> m_controlNodes;
    std::vector<std::pair<float, QString>> m_keyframes;
    QGraphicsPathItem *m_pathItem = nullptr;
    std::vector<InterpolationGraphicsCircleItem *> m_anchorItems;
    std::vector<std::pair<InterpolationGraphicsEdgeItem *, InterpolationGraphicsEdgeItem *>> m_lineItems;
    std::vector<std::pair<InterpolationGraphicsCircleItem *, InterpolationGraphicsCircleItem *>> m_handleItems;
    std::vector<QGraphicsLineItem *> m_gridRowLineItems;
    std::vector<QGraphicsLineItem *> m_gridColumnLineItems;
    std::vector<InterpolationGraphicsKeyframeItem *> m_keyframeKnotItems;
    std::vector<QGraphicsProxyWidget *> m_keyframeNameItems;
    InterpolationGraphicsCursorItem *m_cursorItem = nullptr;
    bool m_gridEnabled = true;
    int m_currentHoveringControlIndex = -1;
    int m_currentSelectedControlIndex = -1;
    int m_currentHoveringKeyframeIndex = -1;
    int m_currentSelectedKeyframeIndex = -1;
    bool m_previewOnly = false;
    static const float m_anchorRadius;
    static const float m_handleRadius;
    static const int m_gridColumns;
    static const int m_gridRows;
    static const int m_sceneWidth;
    static const int m_sceneHeight;
    static const float m_tangentMagnitudeScaleFactor;
};

#endif
