#include <QGraphicsPixmapItem>
#include <QXmlStreamWriter>
#include <QFile>
#include <QApplication>
#include <QGuiApplication>
#include <cmath>
#include <map>
#include <vector>
#include <assert.h>
#include "skeletoneditgraphicsview.h"
#include "skeletoneditnodeitem.h"
#include "skeletoneditedgeitem.h"
#include "theme.h"

qreal SkeletonEditGraphicsView::m_initialNodeSize = 128;
qreal SkeletonEditGraphicsView::m_minimalNodeSize = 8;

SkeletonEditGraphicsView::SkeletonEditGraphicsView(QWidget *parent) :
    QGraphicsView(parent),
    m_pendingNodeItem(NULL),
    m_pendingEdgeItem(NULL),
    m_inAddNodeMode(true),
    m_nextStartNodeItem(NULL),
    m_lastHoverNodeItem(NULL),
    m_lastMousePos(0, 0),
    m_isMovingNodeItem(false),
    m_backgroundLoaded(false),
    m_modelWidget(NULL),
    m_modelWidgetProxy(NULL),
    m_combineEnabled(false),
    m_unionEnabled(true),
    m_subdivEnabled(false)
{
    setScene(new QGraphicsScene());
    
    m_modelWidget = new ModelWidget(this);
    m_modelWidget->setMinimumSize(128, 128);
    m_modelWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_modelWidget->move(100, 100);
    m_modelWidget->setGraphicsView(this);
    m_modelWidgetProxy = scene()->addWidget(m_modelWidget);

    m_backgroundItem = new QGraphicsPixmapItem();
    m_backgroundItem->setOpacity(0.25);
    scene()->addItem(m_backgroundItem);
    
    m_pendingNodeItem = new QGraphicsEllipseItem(0, 0, m_initialNodeSize, m_initialNodeSize);
    m_pendingNodeItem->setVisible(false);
    scene()->addItem(m_pendingNodeItem);
    
    m_pendingEdgeItem = new QGraphicsLineItem(0, 0, 0, 0);
    m_pendingEdgeItem->setVisible(false);
    scene()->addItem(m_pendingEdgeItem);
}

ModelWidget *SkeletonEditGraphicsView::modelWidget()
{
    return m_modelWidget;
}

void SkeletonEditGraphicsView::toggleAddNodeMode()
{
    if (!m_backgroundLoaded)
        return;
    m_inAddNodeMode = !m_inAddNodeMode;
    applyAddNodeMode();
}

void SkeletonEditGraphicsView::applyAddNodeMode()
{
    m_pendingNodeItem->setVisible(m_inAddNodeMode);
    m_pendingEdgeItem->setVisible(m_inAddNodeMode && m_nextStartNodeItem);
    setMouseTracking(true);
}

void SkeletonEditGraphicsView::turnOffAddNodeMode()
{
    if (!m_backgroundLoaded)
        return;
    m_inAddNodeMode = false;
    applyAddNodeMode();
}

void SkeletonEditGraphicsView::turnOnAddNodeMode()
{
    if (!m_backgroundLoaded)
        return;
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
            if (nodeItem->shape().contains(pos)) {
                return nodeItem;
            }
        }
    }
    return NULL;
}

SkeletonEditEdgeItem *SkeletonEditGraphicsView::findEdgeItemByPos(QPointF pos)
{
    QList<QGraphicsItem *>::iterator it;
    QList<QGraphicsItem *> list = scene()->items();
    for (it = list.begin(); it != list.end(); ++it) {
        if ((*it)->data(0).toString() == "edge") {
            SkeletonEditEdgeItem *edgeItem = static_cast<SkeletonEditEdgeItem *>(*it);
            if (edgeItem->shape().contains(pos)) {
                return edgeItem;
            }
        }
    }
    return NULL;
}

SkeletonEditEdgeItem *SkeletonEditGraphicsView::findEdgeItemByNodePair(SkeletonEditNodeItem *first,
        SkeletonEditNodeItem *second)
{
    QList<QGraphicsItem *>::iterator it;
    QList<QGraphicsItem *> list = scene()->items();
    assert(first != second);
    for (it = list.begin(); it != list.end(); ++it) {
        if ((*it)->data(0).toString() == "edge") {
            SkeletonEditEdgeItem *edgeItem = static_cast<SkeletonEditEdgeItem *>(*it);
            if ((edgeItem->firstNode() == first || edgeItem->secondNode() == first) &&
                    (edgeItem->firstNode() == second || edgeItem->secondNode() == second)) {
                return edgeItem;
            }
        }
    }
    return NULL;
}

bool SkeletonEditGraphicsView::mousePress(QMouseEvent *event, const QPointF &scenePos)
{
    bool processed = false;
    if (!m_backgroundLoaded)
        return false;
    QPointF pos = scenePos;
    if (event->button() == Qt::LeftButton) {
        if (!m_inAddNodeMode) {
            if (m_lastHoverNodeItem) {
                setNextStartNodeItem(m_lastHoverNodeItem);
                m_lastHoverNodeItem = NULL;
                processed = true;
            } else {
                if (m_nextStartNodeItem) {
                    setNextStartNodeItem(NULL);
                    processed = true;
                }
            }
        }
    }
    m_lastMousePos = pos;
    return processed;
}

bool SkeletonEditGraphicsView::mouseDoubleClick(QMouseEvent *event, const QPointF &scenePos)
{
    bool processed = false;
    if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
        processed = true;
        emit changeTurnaroundTriggered();
    }
    return processed;
}

void SkeletonEditGraphicsView::removeSelectedItems()
{
    if (m_nextStartNodeItem) {
        SkeletonEditNodeItem *nodeItem = m_nextStartNodeItem;
        setNextStartNodeItem(NULL);
        removeNodeItem(nodeItem->nextSidePair());
        removeNodeItem(nodeItem);
        emit nodesChanged();
    }
}

void SkeletonEditGraphicsView::removeNodeItem(SkeletonEditNodeItem *nodeItem)
{
    scene()->removeItem(nodeItem);
    QList<QGraphicsItem *>::iterator it;
    QList<QGraphicsItem *> list = scene()->items();
    for (it = list.begin(); it != list.end(); ++it) {
        if ((*it)->data(0).toString() == "edge") {
            SkeletonEditEdgeItem *edgeItem = static_cast<SkeletonEditEdgeItem *>(*it);
            if (edgeItem->firstNode() == nodeItem || edgeItem->secondNode() == nodeItem) {
                scene()->removeItem(edgeItem);
            }
        }
    }
}

void SkeletonEditGraphicsView::fetchNodeItemAndAllSidePairs(SkeletonEditNodeItem *nodeItem, std::vector<SkeletonEditNodeItem *> *sidePairs)
{
    sidePairs->push_back(nodeItem);
    sidePairs->push_back(nodeItem->nextSidePair());
    sidePairs->push_back(nodeItem->nextSidePair()->nextSidePair());
}

void SkeletonEditGraphicsView::removeNodeItemAndSidePairs(SkeletonEditNodeItem *nodeItem)
{
    std::vector<SkeletonEditNodeItem *> nodes;
    fetchNodeItemAndAllSidePairs(nodeItem, &nodes);
    for (size_t i = 0; i < nodes.size(); i++) {
        removeNodeItem(nodes[i]);
    }
}

bool SkeletonEditGraphicsView::keyPress(QKeyEvent *event, const QPointF &scenePos)
{
    bool processed = false;
    if (!m_backgroundLoaded)
        return false;
    if (event->key() == Qt::Key_A) {
        toggleAddNodeMode();
        processed = true;
    } else if (event->key() == Qt::Key_C) {
        m_combineEnabled = !m_combineEnabled;
        emit nodesChanged();
    } else if (event->key() == Qt::Key_U) {
        m_unionEnabled = !m_unionEnabled;
        emit nodesChanged();
    } else if (event->key() == Qt::Key_S) {
        m_subdivEnabled = !m_subdivEnabled;
        emit nodesChanged();
    } else if (event->key() == Qt::Key_Delete || event->key() ==Qt::Key_Backspace) {
        removeSelectedItems();
        processed = true;
    }
    return processed;
}

void SkeletonEditGraphicsView::resizeEvent(QResizeEvent *event)
{
    QFrame::resizeEvent(event);
    emit sizeChanged();
}

SkeletonEditNodeItem *SkeletonEditGraphicsView::addNodeItemAndSidePairs(QRectF area, SkeletonEditNodeItem *fromNodeItem, const QString &sideColorName)
{
    float pairedX = 0;
    QRectF pairedRect = area;
    if (fromNodeItem) {
        pairedX = fromNodeItem->nextSidePair()->rect().x();
    } else {
        if (area.center().x() < scene()->sceneRect().width() / 2) {
            pairedX = area.center().x() + scene()->sceneRect().width() / 4;
        } else {
            pairedX = area.center().x() - scene()->sceneRect().width() / 4;
        }
    }
    pairedRect.translate(pairedX - area.x(), 0);
    SkeletonEditNodeItem *firstNode = new SkeletonEditNodeItem(area);
    scene()->addItem(firstNode);
    firstNode->setSideColorName(fromNodeItem ? fromNodeItem->sideColorName() : sideColorName);
    SkeletonEditNodeItem *secondNode = new SkeletonEditNodeItem(pairedRect);
    scene()->addItem(secondNode);
    secondNode->setSideColorName(firstNode->nextSideColorName());
    firstNode->setNextSidePair(secondNode);
    secondNode->setNextSidePair(firstNode);
    setNextStartNodeItem(firstNode);
    if (!fromNodeItem) {
        return firstNode;
    }
    addEdgeItem(fromNodeItem, firstNode);
    addEdgeItem(fromNodeItem->nextSidePair(), firstNode->nextSidePair());
    return firstNode;
}

SkeletonEditNodeItem *SkeletonEditGraphicsView::addNodeItem(float originX, float originY, float radius)
{
    QRectF area(originX - radius, originY - radius, radius * 2, radius * 2);
    SkeletonEditNodeItem *firstNode = new SkeletonEditNodeItem(area);
    scene()->addItem(firstNode);
    return firstNode;
}

void SkeletonEditGraphicsView::addEdgeItem(SkeletonEditNodeItem *first, SkeletonEditNodeItem *second)
{
    SkeletonEditEdgeItem *newEdge = new SkeletonEditEdgeItem();
    newEdge->setNodes(first, second);
    scene()->addItem(newEdge);
}

bool SkeletonEditGraphicsView::mouseRelease(QMouseEvent *event, const QPointF &scenePos)
{
    bool processed = false;
    if (!m_backgroundLoaded)
        return false;
    if (event->button() == Qt::LeftButton) {
        if (m_inAddNodeMode) {
            if (m_lastHoverNodeItem && m_nextStartNodeItem &&
                    m_lastHoverNodeItem != m_nextStartNodeItem &&
                    m_lastHoverNodeItem->sideColor() == m_nextStartNodeItem->sideColor()) {
                if (!findEdgeItemByNodePair(m_lastHoverNodeItem, m_nextStartNodeItem)) {
                    addEdgeItem(m_nextStartNodeItem, m_lastHoverNodeItem);
                    addEdgeItem(m_nextStartNodeItem->nextSidePair(), m_lastHoverNodeItem->nextSidePair());
                    processed = true;
                    emit nodesChanged();
                }
            } else {
                addNodeItemAndSidePairs(m_pendingNodeItem->rect(), m_nextStartNodeItem);
                processed = true;
                emit nodesChanged();
            }
        }
        m_isMovingNodeItem = false;
    }
    return processed;
}

bool SkeletonEditGraphicsView::canNodeItemMoveTo(SkeletonEditNodeItem *item, QPointF moveTo)
{
    if (moveTo.x() < 0)
        return false;
    if (moveTo.y() < 0)
        return false;
    if (moveTo.x() + item->rect().width() >= scene()->sceneRect().width())
        return false;
    if (moveTo.y() + item->rect().height() >= scene()->sceneRect().height())
        return false;
    return true;
}

bool SkeletonEditGraphicsView::mouseMove(QMouseEvent *event, const QPointF &scenePos)
{
    bool processed = false;
    if (!m_backgroundLoaded)
        return false;
    QPointF pos = scenePos;
    QPointF moveTo = QPointF(pos.x() - m_pendingNodeItem->rect().width() / 2, pos.y() - m_pendingNodeItem->rect().height() / 2);
    if (moveTo.x() < 0)
        moveTo.setX(0);
    if (moveTo.y() < 0)
        moveTo.setY(0);
    if (moveTo.x() + m_pendingNodeItem->rect().width() >= scene()->sceneRect().width())
        moveTo.setX(scene()->sceneRect().width() - m_pendingNodeItem->rect().width());
    if (moveTo.y() + m_pendingNodeItem->rect().height() >= scene()->sceneRect().height())
        moveTo.setY(scene()->sceneRect().height() - m_pendingNodeItem->rect().height());
    QSizeF oldSize = m_pendingNodeItem->rect().size();
    m_pendingNodeItem->setRect(moveTo.x(), moveTo.y(),
        oldSize.width(),
        oldSize.height());
    if (m_nextStartNodeItem) {
        m_pendingEdgeItem->setLine(QLineF(m_nextStartNodeItem->origin(), QPointF(moveTo.x() + m_pendingNodeItem->rect().width() / 2,
            moveTo.y() + m_pendingNodeItem->rect().height() / 2)));
    }
    if (!m_isMovingNodeItem) {
        SkeletonEditNodeItem *hoverNodeItem = findNodeItemByPos(pos);
        if (hoverNodeItem) {
            hoverNodeItem->setHovered(true);
            processed = true;
        }
        if (hoverNodeItem != m_lastHoverNodeItem) {
            if (m_lastHoverNodeItem)
                m_lastHoverNodeItem->setHovered(false);
            m_lastHoverNodeItem = hoverNodeItem;
            processed = true;
        }
    }
    QPointF curMousePos = pos;
    if (m_lastHoverNodeItem) {
        if ((event->buttons() & Qt::LeftButton) &&
                (curMousePos != m_lastMousePos || m_isMovingNodeItem)) {
            m_isMovingNodeItem = true;
            QRectF rect = m_lastHoverNodeItem->rect();
            QRectF pairedRect;
            
            rect.translate(curMousePos.x() - m_lastMousePos.x(), curMousePos.y() - m_lastMousePos.y());
            pairedRect = m_lastHoverNodeItem->nextSidePair()->rect();
            pairedRect.translate(0, curMousePos.y() - m_lastMousePos.y());

            m_lastHoverNodeItem->setRect(rect);
            m_lastHoverNodeItem->nextSidePair()->setRect(pairedRect);
            
            QList<QGraphicsItem *>::iterator it;
            QList<QGraphicsItem *> list = scene()->items();
            for (it = list.begin(); it != list.end(); ++it) {
                if ((*it)->data(0).toString() == "edge") {
                    SkeletonEditEdgeItem *edgeItem = static_cast<SkeletonEditEdgeItem *>(*it);
                    if (edgeItem->connects(m_lastHoverNodeItem) ||
                            edgeItem->connects(m_lastHoverNodeItem->nextSidePair()))
                        edgeItem->updatePosition();
                }
            }
            processed = true;
            emit nodesChanged();
        }
    }
    m_lastMousePos = curMousePos;
    return processed;
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
    if (newLeftTop.x() < 0 || newLeftTop.x() + newSize.width() >= scene()->sceneRect().width())
        return;
    if (newLeftTop.y() < 0 || newLeftTop.y() + newSize.height() >= scene()->sceneRect().height())
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
    if (newLeftTop.x() < 0 || newLeftTop.x() + newSize.width() >= scene()->sceneRect().width())
        return false;
    if (newLeftTop.y() < 0 || newLeftTop.y() + newSize.height() >= scene()->sceneRect().height())
        return false;
    return true;
}

bool SkeletonEditGraphicsView::wheel(QWheelEvent *event, const QPointF &scenePos)
{
    bool processed = false;
    if (!m_backgroundLoaded)
        return false;
    qreal delta = event->delta() / 10;
    if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier)) {
        if (delta > 0)
            delta = 1;
        else
            delta = -1;
    } else {
        if (fabs(delta) < 1)
            delta = delta < 0 ? -1.0 : 1.0;
    }
    AddItemRadius(m_pendingNodeItem, delta);
    if (!m_inAddNodeMode && m_lastHoverNodeItem) {
        AddItemRadius(m_lastHoverNodeItem, delta);
        processed = true;
        emit nodesChanged();
    }
    if (m_inAddNodeMode)
        processed = true;
    return processed;
}

void SkeletonEditGraphicsView::setNextStartNodeItem(SkeletonEditNodeItem *item)
{
    if (m_nextStartNodeItem != item) {
        if (m_nextStartNodeItem)
            m_nextStartNodeItem->setChecked(false);
    }
    m_nextStartNodeItem = item;
    if (m_nextStartNodeItem)
        m_nextStartNodeItem->setChecked(true);
    applyAddNodeMode();
}

void SkeletonEditGraphicsView::updateBackgroundImage(const QImage &image)
{
    QSizeF oldSceneSize = scene()->sceneRect().size();
    QPixmap pixmap = QPixmap::fromImage(image);
    scene()->setSceneRect(pixmap.rect());
    m_backgroundItem->setPixmap(pixmap);
    adjustItems(oldSceneSize, scene()->sceneRect().size());
    if (!m_backgroundLoaded) {
        m_backgroundLoaded = true;
        applyAddNodeMode();
    }
}

QPixmap SkeletonEditGraphicsView::backgroundImage()
{
    return m_backgroundItem->pixmap();
}

bool SkeletonEditGraphicsView::hasBackgroundImage()
{
    return m_backgroundLoaded;
}

void SkeletonEditGraphicsView::adjustItems(QSizeF oldSceneSize, QSizeF newSceneSize)
{
    if (oldSceneSize == newSceneSize)
        return;
    float radiusMul = (float)newSceneSize.height() / oldSceneSize.height();
    float xMul = (float)newSceneSize.width() / oldSceneSize.width();
    float yMul = radiusMul;
    QList<QGraphicsItem *>::iterator it;
    QList<QGraphicsItem *> list = scene()->items();
    for (it = list.begin(); it != list.end(); ++it) {
        if ((*it)->data(0).toString() == "node") {
            SkeletonEditNodeItem *nodeItem = static_cast<SkeletonEditNodeItem *>(*it);
            nodeItem->setRadius(nodeItem->radius() * radiusMul);
            QPointF oldOrigin = nodeItem->origin();
            nodeItem->setOrigin(QPointF(oldOrigin.x() * xMul, oldOrigin.y() * yMul));
        }
    }
    for (it = list.begin(); it != list.end(); ++it) {
        if ((*it)->data(0).toString() == "edge") {
            SkeletonEditEdgeItem *edgeItem = static_cast<SkeletonEditEdgeItem *>(*it);
            edgeItem->updatePosition();
        }
    }
}

void SkeletonEditGraphicsView::loadFromSnapshot(SkeletonSnapshot *snapshot)
{
    float radiusMul = 1.0;
    float xMul = 1.0;
    float yMul = radiusMul;
    
    QString canvasWidth = snapshot->canvas["width"];
    QString canvasHeight = snapshot->canvas["height"];
    float canvasWidthVal = canvasWidth.toFloat();
    float canvasHeightVal = canvasHeight.toFloat();
    if (!hasBackgroundImage()) {
        QPixmap emptyImage((int)canvasWidthVal, (int)canvasHeightVal);
        emptyImage.fill(QWidget::palette().color(QWidget::backgroundRole()));
        updateBackgroundImage(emptyImage.toImage());
    }
    if (canvasHeightVal > 0)
        radiusMul = (float)scene()->sceneRect().height() / canvasHeightVal;
    if (canvasWidthVal > 0)
        xMul = (float)scene()->sceneRect().width() / canvasWidthVal;
    yMul = radiusMul;
    
    std::map<QString, SkeletonEditNodeItem *> nodeItemMap;
    std::map<QString, std::map<QString, QString>>::iterator nodeIterator;
    for (nodeIterator = snapshot->nodes.begin(); nodeIterator != snapshot->nodes.end(); nodeIterator++) {
        std::map<QString, QString> *snapshotNode = &nodeIterator->second;
        SkeletonEditNodeItem *nodeItem = addNodeItem((*snapshotNode)["x"].toFloat() * xMul,
            (*snapshotNode)["y"].toFloat() * yMul,
            (*snapshotNode)["radius"].toFloat() * radiusMul);
        nodeItem->setSideColorName((*snapshotNode)["sideColorName"]);
        nodeItem->markAsBranch("true" == (*snapshotNode)["isBranch"]);
        nodeItemMap[nodeIterator->first] = nodeItem;
    }
    for (nodeIterator = snapshot->nodes.begin(); nodeIterator != snapshot->nodes.end(); nodeIterator++) {
        std::map<QString, QString> *snapshotNode = &nodeIterator->second;
        SkeletonEditNodeItem *nodeItem = nodeItemMap[nodeIterator->first];
        nodeItem->setNextSidePair(nodeItemMap[(*snapshotNode)["nextSidePair"]]);
    }
    
    std::map<QString, std::map<QString, QString>>::iterator edgeIterator;
    for (edgeIterator = snapshot->edges.begin(); edgeIterator != snapshot->edges.end(); edgeIterator++) {
        std::map<QString, QString> *snapshotEdge = &edgeIterator->second;
        addEdgeItem(nodeItemMap[(*snapshotEdge)["from"]], nodeItemMap[(*snapshotEdge)["to"]]);
    }
    
    emit nodesChanged();
}

void SkeletonEditGraphicsView::saveToSnapshot(SkeletonSnapshot *snapshot)
{
    snapshot->canvas["width"] = QString("%1").arg(scene()->sceneRect().width());
    snapshot->canvas["height"] = QString("%1").arg(scene()->sceneRect().height());
    snapshot->canvas["combine"] = m_combineEnabled ? "true" : "false";
    snapshot->canvas["union"] = m_unionEnabled ? "true" : "false";
    snapshot->canvas["subdiv"] = m_subdivEnabled ? "true" : "false";
    
    QList<QGraphicsItem *>::iterator it;
    QList<QGraphicsItem *> list = scene()->items();
    
    int nextNodeId = 1;
    std::map<SkeletonEditNodeItem *, QString> nodeIdMap;
    for (it = list.begin(); it != list.end(); ++it) {
        if ((*it)->data(0).toString() == "node") {
            SkeletonEditNodeItem *nodeItem = static_cast<SkeletonEditNodeItem *>(*it);
            QString nodeId = QString("node%1").arg(nextNodeId);
            std::map<QString, QString> *snapshotNode = &snapshot->nodes[nodeId];
            (*snapshotNode)["id"] = nodeId;
            (*snapshotNode)["sideColorName"] = nodeItem->sideColorName();
            (*snapshotNode)["radius"] = QString("%1").arg(nodeItem->radius());
            (*snapshotNode)["isBranch"] = QString("%1").arg(nodeItem->isBranch() ? "true" : "false");
            (*snapshotNode)["isRoot"] = QString("%1").arg(nodeItem->isRoot() ? "true" : "false");
            QPointF origin = nodeItem->origin();
            (*snapshotNode)["x"] = QString("%1").arg(origin.x());
            (*snapshotNode)["y"] = QString("%1").arg(origin.y());
            nodeIdMap[nodeItem] = nodeId;
            nextNodeId++;
        }
    }
    
    for (it = list.begin(); it != list.end(); ++it) {
        if ((*it)->data(0).toString() == "node") {
            SkeletonEditNodeItem *nodeItem = static_cast<SkeletonEditNodeItem *>(*it);
            QString nodeId = nodeIdMap[nodeItem];
            std::map<QString, QString> *snapshotNode = &snapshot->nodes[nodeId];
            (*snapshotNode)["nextSidePair"] = nodeIdMap[nodeItem->nextSidePair()];
        }
    }
    
    int nextEdgeId = 1;
    for (it = list.begin(); it != list.end(); ++it) {
        if ((*it)->data(0).toString() == "edge") {
            SkeletonEditEdgeItem *edgeItem = static_cast<SkeletonEditEdgeItem *>(*it);
            QString edgeId = QString("edge%1").arg(nextEdgeId);
            std::map<QString, QString> *snapshotEdge = &snapshot->edges[edgeId];
            (*snapshotEdge)["id"] = edgeId;
            (*snapshotEdge)["from"] = nodeIdMap[edgeItem->firstNode()];
            (*snapshotEdge)["to"] = nodeIdMap[edgeItem->secondNode()];
            nextEdgeId++;
        }
    }
}

void SkeletonEditGraphicsView::markAsBranch()
{
    if (m_nextStartNodeItem) {
        m_nextStartNodeItem->markAsBranch(true);
        m_nextStartNodeItem->nextSidePair()->markAsBranch(true);
        emit nodesChanged();
    }
}

void SkeletonEditGraphicsView::markAsTrunk()
{
    if (m_nextStartNodeItem) {
        m_nextStartNodeItem->markAsBranch(false);
        m_nextStartNodeItem->nextSidePair()->markAsBranch(false);
        emit nodesChanged();
    }
}

void SkeletonEditGraphicsView::markAsRoot()
{
    if (m_nextStartNodeItem) {
        m_nextStartNodeItem->markAsRoot(true);
        m_nextStartNodeItem->nextSidePair()->markAsRoot(true);
        emit nodesChanged();
    }
}

void SkeletonEditGraphicsView::markAsChild()
{
    if (m_nextStartNodeItem) {
        m_nextStartNodeItem->markAsRoot(false);
        m_nextStartNodeItem->nextSidePair()->markAsRoot(false);
        emit nodesChanged();
    }
}

void SkeletonEditGraphicsView::mouseMoveEvent(QMouseEvent *event)
{
    mouseMove(event, mapToScene(event->pos()));
}

void SkeletonEditGraphicsView::wheelEvent(QWheelEvent *event)
{
    wheel(event, mapToScene(event->pos()));
}

void SkeletonEditGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    mouseRelease(event, mapToScene(event->pos()));
}

void SkeletonEditGraphicsView::mousePressEvent(QMouseEvent *event)
{
    mousePress(event, mapToScene(event->pos()));
}

void SkeletonEditGraphicsView::mouseDoubleClickEvent(QMouseEvent *event)
{
    mouseDoubleClick(event, mapToScene(event->pos()));
}

void SkeletonEditGraphicsView::keyPressEvent(QKeyEvent *event)
{
    keyPress(event, QPointF());
}

bool SkeletonEditGraphicsView::combineEnabled()
{
    return m_combineEnabled;
}

bool SkeletonEditGraphicsView::unionEnabled()
{
    return m_unionEnabled;
}

bool SkeletonEditGraphicsView::subdivEnabled()
{
    return m_subdivEnabled;
}

