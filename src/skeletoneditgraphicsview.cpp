#include <QGraphicsPixmapItem>
#include <QXmlStreamWriter>
#include <QFile>
#include <QApplication>
#include <cmath>
#include <map>
#include <vector>
#include "skeletoneditgraphicsview.h"
#include "skeletoneditnodeitem.h"
#include "skeletoneditedgeitem.h"

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
    m_backgroundLoaded(false)
{
    setScene(new QGraphicsScene());

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
            if (nodeItem->rect().contains(pos)) {
                return nodeItem;
            }
        }
    }
    return NULL;
}

void SkeletonEditGraphicsView::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);
    if (!m_backgroundLoaded)
        return;
    if (event->button() == Qt::LeftButton) {
        if (!m_inAddNodeMode) {
            if (m_lastHoverNodeItem) {
                setNextStartNodeItem(m_lastHoverNodeItem->master());
                m_lastHoverNodeItem = NULL;
            } else {
                if (m_nextStartNodeItem) {
                    setNextStartNodeItem(NULL);
                }
            }
        }
    }
    m_lastMousePos = mapToScene(event->pos());
}

void SkeletonEditGraphicsView::mouseDoubleClickEvent(QMouseEvent *event)
{
    QWidget::mouseDoubleClickEvent(event);
    if (QApplication::keyboardModifiers() & Qt::ControlModifier)
        emit changeTurnaroundTriggered();
}

float SkeletonEditGraphicsView::findXForSlave(float x)
{
    return x - m_backgroundItem->boundingRect().width() / 4;
}

void SkeletonEditGraphicsView::removeSelectedItems()
{
    if (m_nextStartNodeItem) {
        SkeletonEditNodeItem *nodeItem = m_nextStartNodeItem;
        setNextStartNodeItem(NULL);
        removeNodeItem(nodeItem);
        emit nodesChanged();
    }
}

void SkeletonEditGraphicsView::removeNodeItem(SkeletonEditNodeItem *nodeItem)
{
    if (nodeItem->pair()) {
        scene()->removeItem(nodeItem->pair());
    }
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

void SkeletonEditGraphicsView::removeGroupByNodeItem(SkeletonEditNodeItem *nodeItem)
{
    if (nodeItem->pair()) {
        scene()->removeItem(nodeItem->pair());
    }
    scene()->removeItem(nodeItem);
    QList<QGraphicsItem *>::iterator it;
    QList<QGraphicsItem *> list = scene()->items();
    std::vector<SkeletonEditNodeItem *> delayRemoveList;
    for (it = list.begin(); it != list.end(); ++it) {
        if ((*it)->data(0).toString() == "edge") {
            SkeletonEditEdgeItem *edgeItem = static_cast<SkeletonEditEdgeItem *>(*it);
            if (edgeItem->firstNode() == nodeItem) {
                scene()->removeItem(edgeItem);
                delayRemoveList.push_back(edgeItem->secondNode());
            } else if (edgeItem->secondNode() == nodeItem) {
                scene()->removeItem(edgeItem);
                delayRemoveList.push_back(edgeItem->firstNode());
            }
        }
    }
    for (size_t i = 0; i < delayRemoveList.size(); i++) {
        removeNodeItem(delayRemoveList[i]);
    }
}

void SkeletonEditGraphicsView::keyPressEvent(QKeyEvent *event)
{
    QWidget::keyPressEvent(event);
    if (!m_backgroundLoaded)
        return;
    if (event->key() == Qt::Key_A)
        toggleAddNodeMode();
    else if (event->key() == Qt::Key_Delete || event->key() ==Qt::Key_Backspace) {
        removeSelectedItems();
    }
}

void SkeletonEditGraphicsView::resizeEvent(QResizeEvent *event)
{
    QFrame::resizeEvent(event);
    emit sizeChanged();
}

void SkeletonEditGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    QWidget::mouseReleaseEvent(event);
    if (!m_backgroundLoaded)
        return;
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
                masterNode->setGroup(m_nextStartNodeItem->group());
                slaveNode->setGroup(masterNode->group());
            } else {
                QGraphicsItemGroup *group = new QGraphicsItemGroup();
                scene()->addItem(group);
                masterNode->setGroup(group);
                slaveNode->setGroup(masterNode->group());
            }
            setNextStartNodeItem(masterNode);
            emit nodesChanged();
        }
        m_isMovingNodeItem = false;
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
    if (!m_backgroundLoaded)
        return;
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
    if (!m_backgroundLoaded)
        return;
    qreal delta = event->delta() / 10;
    if (fabs(delta) < 1)
        delta = delta < 0 ? -1.0 : 1.0;
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
    applyAddNodeMode();
}

void SkeletonEditGraphicsView::updateBackgroundImage(const QImage &image)
{
    QSizeF oldSceneSize = scene()->sceneRect().size();
    QPixmap pixmap = QPixmap::fromImage(image);
    m_backgroundItem->setPixmap(pixmap);
    scene()->setSceneRect(pixmap.rect());
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

void SkeletonEditGraphicsView::loadFromXmlStream(QXmlStreamReader &reader)
{
    float radiusMul = 1.0;
    float xMul = 1.0;
    float yMul = radiusMul;
    
    std::vector<std::map<QString, QString>> pendingNodes;
    std::vector<std::map<QString, QString>> pendingEdges;
    std::map<QString, SkeletonEditNodeItem *> addedNodeMapById;
    
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement()) {
            if (reader.name() == "canvas") {
                QString canvasWidth = reader.attributes().value("width").toString();
                QString canvasHeight = reader.attributes().value("height").toString();
                float canvasHeightWidth = canvasWidth.toFloat();
                float canvasHeightVal = canvasHeight.toFloat();
                if (!hasBackgroundImage()) {
                    QPixmap emptyImage((int)canvasHeightWidth, (int)canvasHeightVal);
                    emptyImage.fill(QWidget::palette().color(QWidget::backgroundRole()));
                    updateBackgroundImage(emptyImage.toImage());
                }
                if (canvasHeightVal > 0)
                    radiusMul = (float)scene()->sceneRect().height() / canvasHeightVal;
                if (canvasHeightWidth > 0)
                    xMul = (float)scene()->sceneRect().width() / canvasHeightWidth;
                yMul = radiusMul;
            } else if (reader.name() == "origin") {
                if (pendingNodes.size() > 0) {
                    pendingNodes[pendingNodes.size() - 1]["x"] = QString("%1").arg(reader.attributes().value("x").toString().toFloat() * xMul);
                    pendingNodes[pendingNodes.size() - 1]["y"] = QString("%1").arg(reader.attributes().value("y").toString().toFloat() * yMul);
                }
            } else if (reader.name() == "node") {
                QString nodeId = reader.attributes().value("id").toString();
                QString nodeType = reader.attributes().value("type").toString();
                QString nodePairId = reader.attributes().value("pair").toString();
                QString nodeRadius = reader.attributes().value("radius").toString();
                std::map<QString, QString> pendingNode;
                pendingNode["id"] = nodeId;
                pendingNode["type"] = nodeType;
                pendingNode["pair"] = nodePairId;
                pendingNode["radius"] = QString("%1").arg(nodeRadius.toFloat() * radiusMul);
                pendingNode["x"] = "0";
                pendingNode["y"] = "0";
                pendingNodes.push_back(pendingNode);
            } else if (reader.name() == "edge") {
                QString edgeId = reader.attributes().value("id").toString();
                QString edgeFromNodeId = reader.attributes().value("from").toString();
                QString edgeToNodeId = reader.attributes().value("to").toString();
                if (!edgeFromNodeId.isEmpty() && !edgeToNodeId.isEmpty()) {
                    std::map<QString, QString> pendingEdge;
                    pendingEdge["id"] = edgeId;
                    pendingEdge["from"] = edgeFromNodeId;
                    pendingEdge["to"] = edgeToNodeId;
                    pendingEdges.push_back(pendingEdge);
                }
            }
        }
    }
    
    for (size_t i = 0; i < pendingNodes.size(); i++) {
        std::map<QString, QString> *pendingNode = &pendingNodes[i];
        float radius = (*pendingNode)["radius"].toFloat();
        QRectF nodeRect((*pendingNode)["x"].toFloat() - radius, (*pendingNode)["y"].toFloat() - radius,
            radius * 2, radius * 2);
        addedNodeMapById[(*pendingNode)["id"]] = new SkeletonEditNodeItem(nodeRect);
    }
    
    for (size_t i = 0; i < pendingNodes.size(); i++) {
        std::map<QString, QString> *pendingNode = &pendingNodes[i];
        if ((*pendingNode)["type"] == "master") {
            addedNodeMapById[(*pendingNode)["id"]]->setSlave(addedNodeMapById[(*pendingNode)["pair"]]);
        } else if ((*pendingNode)["type"] == "slave") {
            addedNodeMapById[(*pendingNode)["id"]]->setMaster(addedNodeMapById[(*pendingNode)["pair"]]);
        }
        scene()->addItem(addedNodeMapById[(*pendingNode)["id"]]);
    }
    
    for (size_t i = 0; i < pendingEdges.size(); i++) {
        std::map<QString, QString> *pendingEdge = &pendingEdges[i];
        SkeletonEditEdgeItem *newEdge = new SkeletonEditEdgeItem();
        newEdge->setNodes(addedNodeMapById[(*pendingEdge)["from"]], addedNodeMapById[(*pendingEdge)["to"]]);
        scene()->addItem(newEdge);
    }
    
    emit nodesChanged();
}

void SkeletonEditGraphicsView::saveToXmlStream(QXmlStreamWriter *writer)
{
    writer->setAutoFormatting(true);
    writer->writeStartDocument();
    
    writer->writeStartElement("canvas");
        writer->writeAttribute("width", QString("%1").arg(scene()->sceneRect().width()));
        writer->writeAttribute("height", QString("%1").arg(scene()->sceneRect().height()));
    
        QList<QGraphicsItem *>::iterator it;
        QList<QGraphicsItem *> list = scene()->items();
        std::map<SkeletonEditNodeItem *, int> nodeIdMap;
        int nextNodeId = 1;
        for (it = list.begin(); it != list.end(); ++it) {
            if ((*it)->data(0).toString() == "node") {
                SkeletonEditNodeItem *nodeItem = static_cast<SkeletonEditNodeItem *>(*it);
                nodeIdMap[nodeItem] = nextNodeId;
                nextNodeId++;
            }
        }
        writer->writeStartElement("nodes");
        for (it = list.begin(); it != list.end(); ++it) {
            if ((*it)->data(0).toString() == "node") {
                SkeletonEditNodeItem *nodeItem = static_cast<SkeletonEditNodeItem *>(*it);
                writer->writeStartElement("node");
                    writer->writeAttribute("id", QString("node%1").arg(nodeIdMap[nodeItem]));
                    writer->writeAttribute("type", nodeItem->isMaster() ? "master" : "slave");
                    writer->writeAttribute("pair", QString("node%1").arg(nodeIdMap[nodeItem->pair()]));
                    writer->writeAttribute("radius", QString("%1").arg(nodeItem->radius()));
                    writer->writeStartElement("origin");
                        QPointF origin = nodeItem->origin();
                        writer->writeAttribute("x", QString("%1").arg(origin.x()));
                        writer->writeAttribute("y", QString("%1").arg(origin.y()));
                    writer->writeEndElement();
                writer->writeEndElement();
            }
        }
        writer->writeEndElement();
        writer->writeStartElement("edges");
        int nextEdgeId = 1;
        for (it = list.begin(); it != list.end(); ++it) {
            if ((*it)->data(0).toString() == "edge") {
                SkeletonEditEdgeItem *edgeItem = static_cast<SkeletonEditEdgeItem *>(*it);
                writer->writeStartElement("edge");
                    writer->writeAttribute("id", QString("edge%1").arg(nextEdgeId));
                    writer->writeAttribute("from", QString("node%1").arg(nodeIdMap[edgeItem->firstNode()]));
                    writer->writeAttribute("to", QString("node%1").arg(nodeIdMap[edgeItem->secondNode()]));
                writer->writeEndElement();
                nextEdgeId++;
            }
        }
        writer->writeEndElement();
    
    writer->writeEndElement();
    writer->writeEndDocument();
}

