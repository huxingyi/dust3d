#include <QGraphicsPixmapItem>
#include "skeletoneditgraphicsview.h"
#include "skeletoneditnodeitem.h"
#include "skeletoneditedgeitem.h"

qreal SkeletonEditGraphicsView::m_initialNodeSize = 128;
qreal SkeletonEditGraphicsView::m_minimalNodeSize = 32;

SkeletonEditGraphicsView::SkeletonEditGraphicsView(QWidget *parent) :
    QGraphicsView(parent),
    m_pendingNodeItem(NULL),
    m_pendingEdgeItem(NULL),
    m_inAddNodeMode(true),
    m_nextStartNodeItem(NULL),
    m_lastHoverNodeItem(NULL),
    m_lastMousePos(0, 0),
    m_isMovingNodeItem(false)
{
    setScene(new QGraphicsScene());
    
    setMouseTracking(true);
    
    QImage image("../assets/male_werewolf_turnaround_lineart_by_jennette_brown.png");
    m_backgroundItem = new QGraphicsPixmapItem(QPixmap::fromImage(image));
    scene()->addItem(m_backgroundItem);
    
    m_pendingNodeItem = new QGraphicsEllipseItem(0, 0, m_initialNodeSize, m_initialNodeSize);
    scene()->addItem(m_pendingNodeItem);
    
    m_pendingEdgeItem = new QGraphicsLineItem(0, 0, 0, 0);
    scene()->addItem(m_pendingEdgeItem);
    
    applyAddNodeMode();
}

void SkeletonEditGraphicsView::toggleAddNodeMode()
{
    m_inAddNodeMode = !m_inAddNodeMode;
    applyAddNodeMode();
}

void SkeletonEditGraphicsView::applyAddNodeMode()
{
    m_pendingNodeItem->setVisible(m_inAddNodeMode);
    m_pendingEdgeItem->setVisible(m_inAddNodeMode);
    setMouseTracking(true);
}

void SkeletonEditGraphicsView::turnOffAddNodeMode()
{
    m_inAddNodeMode = false;
    applyAddNodeMode();
}

void SkeletonEditGraphicsView::turnOnAddNodeMode()
{
    m_inAddNodeMode = true;
    applyAddNodeMode();
}

SkeletonEditNodeItem *SkeletonEditGraphicsView::findNodeItemByPos(QPointF pos)
{
    QList<QGraphicsItem *>::iterator it;
    QList<QGraphicsItem *> list = scene()->items();
    for (it = list.begin(); it != list.end(); ++it) {
        if ((*it)->data(0).toString() == "node") {
            SkeletonEditNodeItem *nodeItem = static_cast<SkeletonEditNodeItem *>(*it);
            if (nodeItem->rect().contains(pos)) {
                return nodeItem;
            }
        }
    }
    return NULL;
}

void SkeletonEditGraphicsView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (!m_inAddNodeMode) {
            if (m_lastHoverNodeItem) {
                setNextStartNodeItem(m_lastHoverNodeItem->master());
                m_lastHoverNodeItem = NULL;
            }
        }
    }
    m_lastMousePos = mapToScene(event->pos());
}

float SkeletonEditGraphicsView::findXForSlave(float x)
{
    return x - m_backgroundItem->boundingRect().width() / 4;
}

void SkeletonEditGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_inAddNodeMode) {
            SkeletonEditNodeItem *masterNode = new SkeletonEditNodeItem(m_pendingNodeItem->rect());
            scene()->addItem(masterNode);
            QRectF slaveRect = m_pendingNodeItem->rect();
            float x = m_pendingNodeItem->x() + m_pendingNodeItem->rect().width() / 2;
            slaveRect.translate(findXForSlave(x) - x, 0);
            SkeletonEditNodeItem *slaveNode = new SkeletonEditNodeItem(slaveRect);
            scene()->addItem(slaveNode);
            masterNode->setSlave(slaveNode);
            slaveNode->setMaster(masterNode);
            if (m_nextStartNodeItem) {
                SkeletonEditEdgeItem *newEdge = new SkeletonEditEdgeItem();
                newEdge->setNodes(masterNode, m_nextStartNodeItem);
                scene()->addItem(newEdge);
            }
            setNextStartNodeItem(masterNode);
            emit nodesChanged();
        }
        m_isMovingNodeItem = false;
    } else if (event->button() == Qt::RightButton) {
        if (m_inAddNodeMode) {
            toggleAddNodeMode();
        }
    }
}

bool SkeletonEditGraphicsView::canNodeItemMoveTo(SkeletonEditNodeItem *item, QPointF moveTo)
{
    if (moveTo.x() < 0)
        return false;
    if (moveTo.y() < 0)
        return false;
    if (moveTo.x() + item->rect().width() >= m_backgroundItem->boundingRect().width())
        return false;
    if (moveTo.y() + item->rect().height() >= m_backgroundItem->boundingRect().height())
        return false;
    return true;
}

void SkeletonEditGraphicsView::mouseMoveEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);
    QPointF pos = mapToScene(event->pos());
    QPointF moveTo = QPointF(pos.x() - m_pendingNodeItem->rect().width() / 2, pos.y() - m_pendingNodeItem->rect().height() / 2);
    if (moveTo.x() < 0)
        moveTo.setX(0);
    if (moveTo.y() < 0)
        moveTo.setY(0);
    if (moveTo.x() + m_pendingNodeItem->rect().width() >= m_backgroundItem->boundingRect().width())
        moveTo.setX(m_backgroundItem->boundingRect().width() - m_pendingNodeItem->rect().width());
    if (moveTo.y() + m_pendingNodeItem->rect().height() >= m_backgroundItem->boundingRect().height())
        moveTo.setY(m_backgroundItem->boundingRect().height() - m_pendingNodeItem->rect().height());
    QSizeF oldSize = m_pendingNodeItem->rect().size();
    m_pendingNodeItem->setRect(moveTo.x(), moveTo.y(),
        oldSize.width(),
        oldSize.height());
    if (m_nextStartNodeItem) {
        m_pendingEdgeItem->setLine(QLineF(m_nextStartNodeItem->origin(), QPointF(moveTo.x() + m_pendingNodeItem->rect().width() / 2,
            moveTo.y() + m_pendingNodeItem->rect().height() / 2)));
    }
    if (!m_isMovingNodeItem) {
        if (!m_inAddNodeMode) {
            SkeletonEditNodeItem *hoverNodeItem = findNodeItemByPos(pos);
            if (hoverNodeItem) {
                hoverNodeItem->setHighlighted(true);
            }
            if (hoverNodeItem != m_lastHoverNodeItem) {
                if (m_lastHoverNodeItem)
                    m_lastHoverNodeItem->setHighlighted(false);
                m_lastHoverNodeItem = hoverNodeItem;
            }
        } else {
            if (m_lastHoverNodeItem) {
                m_lastHoverNodeItem->setHighlighted(false);
                m_lastHoverNodeItem = NULL;
            }
        }
    }
    QPointF curMousePos = pos;
    if (m_lastHoverNodeItem) {
        if ((event->buttons() & Qt::LeftButton) &&
                (curMousePos != m_lastMousePos || m_isMovingNodeItem)) {
            m_isMovingNodeItem = true;
            QRectF rect = m_lastHoverNodeItem->rect();
            QRectF slaveRect;
            if (m_lastHoverNodeItem->isMaster()) {
                rect.translate(curMousePos.x() - m_lastMousePos.x(), curMousePos.y() - m_lastMousePos.y());
                slaveRect = m_lastHoverNodeItem->slave()->rect();
                slaveRect.translate(0, curMousePos.y() - m_lastMousePos.y());
            } else {
                rect.translate(curMousePos.x() - m_lastMousePos.x(), 0);
            }
            if (canNodeItemMoveTo(m_lastHoverNodeItem, QPointF(rect.x(), rect.y()))) {
                if (m_lastHoverNodeItem->isMaster()) {
                    m_lastHoverNodeItem->slave()->setRect(slaveRect);
                }
                m_lastHoverNodeItem->setRect(rect);
                QList<QGraphicsItem *>::iterator it;
                QList<QGraphicsItem *> list = scene()->items();
                for (it = list.begin(); it != list.end(); ++it) {
                    if ((*it)->data(0).toString() == "edge") {
                        SkeletonEditEdgeItem *edgeItem = static_cast<SkeletonEditEdgeItem *>(*it);
                        if (edgeItem->connects(m_lastHoverNodeItem))
                            edgeItem->updatePosition();
                    }
                }
                emit nodesChanged();
            }
        }
    }
    m_lastMousePos = curMousePos;
}

void SkeletonEditGraphicsView::AddItemRadius(QGraphicsEllipseItem *item, float delta)
{
    QSizeF oldSize = item->rect().size();
    QPointF originPt = QPointF(item->rect().left() + oldSize.width() / 2,
        item->rect().top() + oldSize.height() / 2);
    QSizeF newSize = QSizeF(oldSize.width() + delta, oldSize.height() + delta);
    if (newSize.width() < m_minimalNodeSize || newSize.height() < m_minimalNodeSize) {
        newSize.setWidth(m_minimalNodeSize);
        newSize.setHeight(m_minimalNodeSize);
    }
    QPointF newLeftTop = QPointF(originPt.x() - newSize.width() / 2,
        originPt.y() - newSize.height() / 2);
    if (newLeftTop.x() < 0 || newLeftTop.x() + newSize.width() >= m_backgroundItem->boundingRect().width())
        return;
    if (newLeftTop.y() < 0 || newLeftTop.y() + newSize.height() >= m_backgroundItem->boundingRect().height())
        return;
    item->setRect(newLeftTop.x(),
        newLeftTop.y(),
        newSize.width(),
        newSize.height());
}

bool SkeletonEditGraphicsView::canAddItemRadius(QGraphicsEllipseItem *item, float delta)
{
    QSizeF oldSize = item->rect().size();
    QPointF originPt = QPointF(item->rect().left() + oldSize.width() / 2,
        item->rect().top() + oldSize.height() / 2);
    QSizeF newSize = QSizeF(oldSize.width() + delta, oldSize.height() + delta);
    if (newSize.width() < m_minimalNodeSize || newSize.height() < m_minimalNodeSize) {
        newSize.setWidth(m_minimalNodeSize);
        newSize.setHeight(m_minimalNodeSize);
    }
    QPointF newLeftTop = QPointF(originPt.x() - newSize.width() / 2,
        originPt.y() - newSize.height() / 2);
    if (newLeftTop.x() < 0 || newLeftTop.x() + newSize.width() >= m_backgroundItem->boundingRect().width())
        return false;
    if (newLeftTop.y() < 0 || newLeftTop.y() + newSize.height() >= m_backgroundItem->boundingRect().height())
        return false;
    return true;
}

void SkeletonEditGraphicsView::wheelEvent(QWheelEvent *event)
{
    QWidget::wheelEvent(event);
    qreal delta = event->delta();
    AddItemRadius(m_pendingNodeItem, delta);
    if (!m_inAddNodeMode && m_lastHoverNodeItem) {
        if (canAddItemRadius(m_lastHoverNodeItem, delta) &&
                canAddItemRadius(m_lastHoverNodeItem->pair(), delta)) {
            AddItemRadius(m_lastHoverNodeItem, delta);
            AddItemRadius(m_lastHoverNodeItem->pair(), delta);
        }
        emit nodesChanged();
    }
}

void SkeletonEditGraphicsView::setNextStartNodeItem(SkeletonEditNodeItem *item)
{
    if (m_nextStartNodeItem != item) {
        if (m_nextStartNodeItem)
            m_nextStartNodeItem->setIsNextStartNode(false);
    }
    m_nextStartNodeItem = item;
    if (m_nextStartNodeItem)
        m_nextStartNodeItem->setIsNextStartNode(true);
}

