#include <QDebug>
#include <QScrollBar>
#include <QGuiApplication>
#include <cmath>
#include <QtGlobal>
#include <algorithm>
#include "skeletongraphicswidget.h"
#include "theme.h"
#include "util.h"

SkeletonGraphicsWidget::SkeletonGraphicsWidget(const SkeletonDocument *document) :
    m_document(document),
    m_turnaroundChanged(false),
    m_turnaroundLoader(nullptr),
    m_dragStarted(false),
    m_cursorNodeItem(nullptr),
    m_cursorEdgeItem(nullptr),
    m_checkedNodeItem(nullptr),
    m_moveStarted(false),
    m_hoveredNodeItem(nullptr),
    m_hoveredEdgeItem(nullptr),
    m_checkedEdgeItem(nullptr),
    m_lastAddedX(0),
    m_lastAddedY(0),
    m_lastAddedZ(0)
{
    setRenderHint(QPainter::Antialiasing, false);
    setBackgroundBrush(QBrush(QWidget::palette().color(QWidget::backgroundRole()), Qt::SolidPattern));
    setContentsMargins(0, 0, 0, 0);
    setFrameStyle(QFrame::NoFrame);
    
    setAlignment(Qt::AlignCenter);
    
    setScene(new QGraphicsScene());
    
    m_backgroundItem = new QGraphicsPixmapItem();
    m_backgroundItem->setOpacity(0.25);
    scene()->addItem(m_backgroundItem);
    
    m_cursorNodeItem = new SkeletonGraphicsNodeItem();
    m_cursorNodeItem->hide();
    scene()->addItem(m_cursorNodeItem);
    
    m_cursorEdgeItem = new SkeletonGraphicsEdgeItem();
    m_cursorEdgeItem->hide();
    scene()->addItem(m_cursorEdgeItem);
    
    scene()->setSceneRect(rect());
    
    setMouseTracking(true);
}

void SkeletonGraphicsWidget::updateItems()
{
    for (auto nodeItemIt = nodeItemMap.begin(); nodeItemIt != nodeItemMap.end(); nodeItemIt++) {
        nodeRadiusChanged(nodeItemIt->first);
        nodeOriginChanged(nodeItemIt->first);
    }
}

void SkeletonGraphicsWidget::canvasResized()
{
    updateTurnaround();
}

void SkeletonGraphicsWidget::turnaroundChanged()
{
    updateTurnaround();
    setTransform(QTransform());
}

void SkeletonGraphicsWidget::updateTurnaround()
{
    if (m_document->turnaround.isNull())
        return;
    
    m_turnaroundChanged = true;
    if (m_turnaroundLoader)
        return;
    
    qDebug() << "Fit turnaround to view size:" << parentWidget()->rect().size();
    
    m_turnaroundChanged = false;

    QThread *thread = new QThread;
    m_turnaroundLoader = new TurnaroundLoader(m_document->turnaround,
        parentWidget()->rect().size());
    m_turnaroundLoader->moveToThread(thread);
    connect(thread, SIGNAL(started()), m_turnaroundLoader, SLOT(process()));
    connect(m_turnaroundLoader, SIGNAL(finished()), this, SLOT(turnaroundImageReady()));
    connect(m_turnaroundLoader, SIGNAL(finished()), thread, SLOT(quit()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();
}

void SkeletonGraphicsWidget::turnaroundImageReady()
{
    QImage *backgroundImage = m_turnaroundLoader->takeResultImage();
    if (backgroundImage && backgroundImage->width() > 0 && backgroundImage->height() > 0) {
        qDebug() << "Fit turnaround finished with image size:" << backgroundImage->size();
        setFixedSize(backgroundImage->size());
        scene()->setSceneRect(rect());
        m_backgroundItem->setPixmap(QPixmap::fromImage(*backgroundImage));
        updateItems();
    } else {
        qDebug() << "Fit turnaround failed";
    }
    delete backgroundImage;
    delete m_turnaroundLoader;
    m_turnaroundLoader = nullptr;

    if (m_turnaroundChanged) {
        updateTurnaround();
    }
}

void SkeletonGraphicsWidget::updateCursor()
{
    if (SkeletonDocumentEditMode::Add != m_document->editMode) {
        m_cursorEdgeItem->hide();
        m_cursorNodeItem->hide();
    }
    
    switch (m_document->editMode) {
        case SkeletonDocumentEditMode::Add:
            setCursor(QCursor(Theme::awesome()->icon(fa::plus).pixmap(Theme::toolIconFontSize, Theme::toolIconFontSize)));
            break;
        case SkeletonDocumentEditMode::Select:
            setCursor(QCursor(Theme::awesome()->icon(fa::mousepointer).pixmap(Theme::toolIconFontSize, Theme::toolIconFontSize)));
            break;
        case SkeletonDocumentEditMode::Drag:
            setCursor(QCursor(Theme::awesome()->icon(m_dragStarted ? fa::handrocko : fa::handpapero).pixmap(Theme::toolIconFontSize, Theme::toolIconFontSize)));
            break;
        case SkeletonDocumentEditMode::ZoomIn:
            setCursor(QCursor(Theme::awesome()->icon(fa::searchplus).pixmap(Theme::toolIconFontSize, Theme::toolIconFontSize)));
            break;
        case SkeletonDocumentEditMode::ZoomOut:
            setCursor(QCursor(Theme::awesome()->icon(fa::searchminus).pixmap(Theme::toolIconFontSize, Theme::toolIconFontSize)));
            break;
    }
    
    emit cursorChanged();
}

void SkeletonGraphicsWidget::editModeChanged()
{
    if (m_checkedNodeItem) {
        if (!m_checkedNodeItem->checked()) {
            if (m_hoveredNodeItem == m_checkedNodeItem) {
                m_checkedNodeItem->setHovered(false);
                m_hoveredNodeItem = nullptr;
            }
            m_checkedNodeItem = nullptr;
        }
    }
    updateCursor();
}

void SkeletonGraphicsWidget::mouseMoveEvent(QMouseEvent *event)
{
    QGraphicsView::mouseMoveEvent(event);
    mouseMove(event);
}

void SkeletonGraphicsWidget::wheelEvent(QWheelEvent *event)
{
    if (SkeletonDocumentEditMode::ZoomIn == m_document->editMode ||
            SkeletonDocumentEditMode::ZoomOut == m_document->editMode ||
            SkeletonDocumentEditMode::Drag == m_document->editMode)
        QGraphicsView::wheelEvent(event);
    wheel(event);
}

void SkeletonGraphicsWidget::mouseReleaseEvent(QMouseEvent *event)
{
    QGraphicsView::mouseReleaseEvent(event);
    mouseRelease(event);
}

void SkeletonGraphicsWidget::mousePressEvent(QMouseEvent *event)
{
    QGraphicsView::mousePressEvent(event);
    mousePress(event);
}

void SkeletonGraphicsWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    QGraphicsView::mouseDoubleClickEvent(event);
    mouseDoubleClick(event);
}

void SkeletonGraphicsWidget::keyPressEvent(QKeyEvent *event)
{
    QGraphicsView::keyPressEvent(event);
    keyPress(event);
}

bool SkeletonGraphicsWidget::mouseMove(QMouseEvent *event)
{
    if (m_dragStarted) {
        QPoint currentGlobalPos = event->globalPos();
        if (verticalScrollBar())
            verticalScrollBar()->setValue(verticalScrollBar()->value() + m_lastGlobalPos.y() - currentGlobalPos.y());
        if (horizontalScrollBar())
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() + m_lastGlobalPos.x() - currentGlobalPos.x());
        m_lastGlobalPos = currentGlobalPos;
        return true;
    }
    
    if (SkeletonDocumentEditMode::Select == m_document->editMode ||
            SkeletonDocumentEditMode::Add == m_document->editMode) {
        SkeletonGraphicsNodeItem *newHoverNodeItem = nullptr;
        SkeletonGraphicsEdgeItem *newHoverEdgeItem = nullptr;
        QList<QGraphicsItem *> items = scene()->items(mouseEventScenePos(event));
        for (auto it = items.begin(); it != items.end(); it++) {
            QGraphicsItem *item = *it;
            if (item->data(0) == "node") {
                newHoverNodeItem = (SkeletonGraphicsNodeItem *)item;
                break;
            } else if (item->data(0) == "edge") {
                newHoverEdgeItem = (SkeletonGraphicsEdgeItem *)item;
            }
        }
        if (newHoverNodeItem) {
            newHoverEdgeItem = nullptr;
        }
        if (newHoverNodeItem != m_hoveredNodeItem) {
            if (nullptr != m_hoveredNodeItem) {
                m_hoveredNodeItem->setHovered(false);
            }
            m_hoveredNodeItem = newHoverNodeItem;
            if (nullptr != m_hoveredNodeItem) {
                m_hoveredNodeItem->setHovered(true);
            }
        }
        if (newHoverEdgeItem != m_hoveredEdgeItem) {
            if (nullptr != m_hoveredEdgeItem) {
                m_hoveredEdgeItem->setHovered(false);
            }
            m_hoveredEdgeItem = newHoverEdgeItem;
            if (nullptr != m_hoveredEdgeItem) {
                m_hoveredEdgeItem->setHovered(true);
            }
        }
    }
    
    if (SkeletonDocumentEditMode::Add == m_document->editMode) {
        QPointF mouseScenePos = mouseEventScenePos(event);
        m_cursorNodeItem->setOrigin(mouseScenePos);
        if (!m_cursorNodeItem->isVisible()) {
            m_cursorNodeItem->show();
        }
        if (m_checkedNodeItem) {
            m_cursorEdgeItem->setEndpoints(m_checkedNodeItem, m_cursorNodeItem);
            if (!m_cursorEdgeItem->isVisible()) {
                m_cursorEdgeItem->show();
            }
        }
        return true;
    }
    
    if (SkeletonDocumentEditMode::Select == m_document->editMode) {
        if (m_moveStarted && m_checkedNodeItem) {
            QPointF mouseScenePos = mouseEventScenePos(event);
            float byX = sceneRadiusToUnified(mouseScenePos.x() - m_lastScenePos.x());
            float byY = sceneRadiusToUnified(mouseScenePos.y() - m_lastScenePos.y());
            if (SkeletonProfile::Main == m_checkedNodeItem->profile()) {
                emit moveNodeBy(m_checkedNodeItem->id(), byX, byY, 0);
            } else {
                emit moveNodeBy(m_checkedNodeItem->id(), 0, byY, byX);
            }
            m_lastScenePos = mouseScenePos;
            return true;
        }
    }
    
    return false;
}

bool SkeletonGraphicsWidget::wheel(QWheelEvent *event)
{
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
    if (SkeletonDocumentEditMode::Add == m_document->editMode) {
        if (m_cursorNodeItem->isVisible()) {
            m_cursorNodeItem->setRadius(m_cursorNodeItem->radius() + delta);
            return true;
        }
    } else if (SkeletonDocumentEditMode::Select == m_document->editMode) {
        if (m_hoveredNodeItem) {
            emit scaleNodeByAddRadius(m_hoveredNodeItem->id(), sceneRadiusToUnified(delta));
            return true;
        } else if (m_checkedNodeItem) {
            emit scaleNodeByAddRadius(m_checkedNodeItem->id(), sceneRadiusToUnified(delta));
            return true;
        }
    }
    return false;
}

bool SkeletonGraphicsWidget::mouseRelease(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        bool processed = m_dragStarted || m_moveStarted;
        if (m_dragStarted) {
            m_dragStarted = false;
            updateCursor();
        }
        if (m_moveStarted) {
            m_moveStarted = false;
        }
        return processed;
    }
    return false;
}

QPointF SkeletonGraphicsWidget::mouseEventScenePos(QMouseEvent *event)
{
    return mapToScene(mapFromGlobal(event->globalPos()));
}

bool SkeletonGraphicsWidget::mousePress(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (SkeletonDocumentEditMode::ZoomIn == m_document->editMode) {
            ViewportAnchor lastAnchor = transformationAnchor();
            setTransformationAnchor(QGraphicsView::AnchorUnderMouse) ;
            scale(1.5, 1.5);
            setTransformationAnchor(lastAnchor);
            return true;
        } else if (SkeletonDocumentEditMode::ZoomOut == m_document->editMode) {
            ViewportAnchor lastAnchor = transformationAnchor();
            setTransformationAnchor(QGraphicsView::AnchorUnderMouse) ;
            scale(0.5, 0.5);
            setTransformationAnchor(lastAnchor);
            if ((!verticalScrollBar() || !verticalScrollBar()->isVisible()) &&
                    (!horizontalScrollBar() || !horizontalScrollBar()->isVisible())) {
                setTransform(QTransform());
            }
            return true;
        } else if (SkeletonDocumentEditMode::Drag == m_document->editMode) {
            if (!m_dragStarted) {
                m_lastGlobalPos = event->globalPos();
                m_dragStarted = true;
                updateCursor();
            }
        } else if (SkeletonDocumentEditMode::Add == m_document->editMode) {
            if (m_cursorNodeItem->isVisible()) {
                if (m_checkedNodeItem) {
                    if (m_hoveredNodeItem && m_checkedNodeItem &&
                            m_hoveredNodeItem != m_checkedNodeItem &&
                            m_hoveredNodeItem->profile() == m_checkedNodeItem->profile()) {
                        if (m_document->findEdgeByNodes(m_checkedNodeItem->id(), m_hoveredNodeItem->id()))
                            return true;
                        emit addEdge(m_checkedNodeItem->id(), m_hoveredNodeItem->id());
                        return true;
                    }
                }
                QPointF mainProfile = m_cursorNodeItem->origin();
                QPointF sideProfile = mainProfile;
                if (mainProfile.x() >= scene()->width() / 2) {
                    sideProfile.setX(mainProfile.x() - scene()->width() / 4);
                } else {
                    sideProfile.setX(mainProfile.x() +scene()->width() / 4);
                }
                QPointF unifiedMainPos = scenePosToUnified(mainProfile);
                QPointF unifiedSidePos = scenePosToUnified(sideProfile);
                if (isFloatEqual(m_lastAddedX, unifiedMainPos.x()) && isFloatEqual(m_lastAddedY, unifiedMainPos.y()) && isFloatEqual(m_lastAddedZ, unifiedSidePos.x()))
                    return true;
                m_lastAddedX = unifiedMainPos.x();
                m_lastAddedY = unifiedMainPos.y();
                m_lastAddedZ = unifiedSidePos.x();
                qDebug() << "Emit add node " << m_lastAddedX << m_lastAddedY << m_lastAddedZ;
                emit addNode(unifiedMainPos.x(), unifiedMainPos.y(), unifiedSidePos.x(), sceneRadiusToUnified(m_cursorNodeItem->radius()), nullptr == m_checkedNodeItem ? QUuid() : m_checkedNodeItem->id());
                return true;
            }
        } else if (SkeletonDocumentEditMode::Select == m_document->editMode) {
            bool processed = false;
            if (m_hoveredNodeItem) {
                if (m_checkedNodeItem != m_hoveredNodeItem) {
                    if (m_checkedNodeItem) {
                        emit uncheckNode(m_checkedNodeItem->id());
                        m_checkedNodeItem->setChecked(false);
                    }
                    m_checkedNodeItem = m_hoveredNodeItem;
                    m_checkedNodeItem->setChecked(true);
                    emit checkNode(m_checkedNodeItem->id());
                }
                m_moveStarted = true;
                m_lastScenePos = mouseEventScenePos(event);
                processed = true;
            } else {
                if (m_checkedNodeItem) {
                    m_checkedNodeItem->setChecked(false);
                    emit uncheckNode(m_checkedNodeItem->id());
                    m_checkedNodeItem = nullptr;
                    processed = true;
                }
            }
            if (m_hoveredEdgeItem) {
                if (m_checkedEdgeItem != m_hoveredEdgeItem) {
                    if (m_checkedEdgeItem) {
                        emit uncheckEdge(m_checkedEdgeItem->id());
                        m_checkedEdgeItem->setChecked(false);
                    }
                    m_checkedEdgeItem = m_hoveredEdgeItem;
                    m_checkedEdgeItem->setChecked(true);
                    emit checkEdge(m_checkedEdgeItem->id());
                }
            } else {
                if (m_checkedEdgeItem) {
                    m_checkedEdgeItem->setChecked(false);
                    emit uncheckEdge(m_checkedEdgeItem->id());
                    m_checkedEdgeItem = nullptr;
                    processed = true;
                }
            }
            if (processed) {
                return true;
            }
        }
    }
    return false;
}

float SkeletonGraphicsWidget::sceneRadiusToUnified(float radius)
{
    if (0 == scene()->height())
        return 0;
    return radius / scene()->height();
}

float SkeletonGraphicsWidget::sceneRadiusFromUnified(float radius)
{
    return radius * scene()->height();
}

QPointF SkeletonGraphicsWidget::scenePosToUnified(QPointF pos)
{
    if (0 == scene()->height())
        return QPointF(0, 0);
    return QPointF(pos.x() / scene()->height(), pos.y() / scene()->height());
}

QPointF SkeletonGraphicsWidget::scenePosFromUnified(QPointF pos)
{
    return QPointF(pos.x() * scene()->height(), pos.y() * scene()->height());
}

bool SkeletonGraphicsWidget::mouseDoubleClick(QMouseEvent *event)
{
    return false;
}

bool SkeletonGraphicsWidget::keyPress(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete || event->key() ==Qt::Key_Backspace) {
        bool processed = false;
        if (m_checkedNodeItem) {
            emit removeNode(m_checkedNodeItem->id());
            processed = true;
        }
        if (m_checkedEdgeItem) {
            emit removeEdge(m_checkedEdgeItem->id());
            processed = true;
        }
        if (processed) {
            return true;
        }
    } else if (event->key() == Qt::Key_A) {
        if (SkeletonDocumentEditMode::Add == m_document->editMode) {
            emit setEditMode(SkeletonDocumentEditMode::Select);
        } else {
            emit setEditMode(SkeletonDocumentEditMode::Add);
        }
        return true;
    }
    return false;
}

void SkeletonGraphicsWidget::nodeAdded(QUuid nodeId)
{
    const SkeletonNode *node = m_document->findNode(nodeId);
    if (nullptr == node) {
        qDebug() << "New node added but node id not exist:" << nodeId;
        return;
    }
    SkeletonGraphicsNodeItem *mainProfileItem = new SkeletonGraphicsNodeItem(SkeletonProfile::Main);
    SkeletonGraphicsNodeItem *sideProfileItem = new SkeletonGraphicsNodeItem(SkeletonProfile::Side);
    mainProfileItem->setOrigin(scenePosFromUnified(QPointF(node->x, node->y)));
    sideProfileItem->setOrigin(scenePosFromUnified(QPointF(node->z, node->y)));
    mainProfileItem->setRadius(sceneRadiusFromUnified(node->radius));
    sideProfileItem->setRadius(sceneRadiusFromUnified(node->radius));
    mainProfileItem->setId(nodeId);
    sideProfileItem->setId(nodeId);
    scene()->addItem(mainProfileItem);
    scene()->addItem(sideProfileItem);
    nodeItemMap[nodeId] = std::make_pair(mainProfileItem, sideProfileItem);
    
    if (nullptr == m_checkedNodeItem) {
        m_checkedNodeItem = mainProfileItem;
    } else {
        if (SkeletonProfile::Main == m_checkedNodeItem->profile()) {
            m_checkedNodeItem = mainProfileItem;
        } else {
            m_checkedNodeItem = sideProfileItem;
        }
        m_cursorEdgeItem->setEndpoints(m_checkedNodeItem, m_cursorNodeItem);
    }
}

void SkeletonGraphicsWidget::edgeAdded(QUuid edgeId)
{
    const SkeletonEdge *edge = m_document->findEdge(edgeId);
    if (nullptr == edge) {
        qDebug() << "New edge added but edge id not exist:" << edgeId;
        return;
    }
    if (edge->nodeIds.size() != 2) {
        qDebug() << "Invalid node count of edge:" << edgeId;
        return;
    }
    QUuid fromNodeId = edge->nodeIds[0];
    QUuid toNodeId = edge->nodeIds[1];
    auto fromIt = nodeItemMap.find(fromNodeId);
    if (fromIt == nodeItemMap.end()) {
        qDebug() << "Node not found:" << fromNodeId;
        return;
    }
    auto toIt = nodeItemMap.find(toNodeId);
    if (toIt == nodeItemMap.end()) {
        qDebug() << "Node not found:" << toNodeId;
        return;
    }
    SkeletonGraphicsEdgeItem *mainProfileEdgeItem = new SkeletonGraphicsEdgeItem();
    SkeletonGraphicsEdgeItem *sideProfileEdgeItem = new SkeletonGraphicsEdgeItem();
    mainProfileEdgeItem->setId(edgeId);
    sideProfileEdgeItem->setId(edgeId);
    mainProfileEdgeItem->setEndpoints(fromIt->second.first, toIt->second.first);
    sideProfileEdgeItem->setEndpoints(fromIt->second.second, toIt->second.second);
    scene()->addItem(mainProfileEdgeItem);
    scene()->addItem(sideProfileEdgeItem);
    edgeItemMap[edgeId] = std::make_pair(mainProfileEdgeItem, sideProfileEdgeItem);
}

void SkeletonGraphicsWidget::nodeRemoved(QUuid nodeId)
{
    m_lastAddedX = 0;
    m_lastAddedY = 0;
    m_lastAddedZ = 0;
    auto nodeItemIt = nodeItemMap.find(nodeId);
    if (nodeItemIt == nodeItemMap.end()) {
        qDebug() << "Node removed but node id not exist:" << nodeId;
        return;
    }
    if (m_hoveredNodeItem == nodeItemIt->second.first)
        m_hoveredNodeItem = nullptr;
    if (m_hoveredNodeItem == nodeItemIt->second.second)
        m_hoveredNodeItem = nullptr;
    if (m_checkedNodeItem == nodeItemIt->second.first)
        m_checkedNodeItem = nullptr;
    if (m_checkedNodeItem == nodeItemIt->second.second)
        m_checkedNodeItem = nullptr;
    delete nodeItemIt->second.first;
    delete nodeItemIt->second.second;
    nodeItemMap.erase(nodeItemIt);
}

void SkeletonGraphicsWidget::edgeRemoved(QUuid edgeId)
{
    auto edgeItemIt = edgeItemMap.find(edgeId);
    if (edgeItemIt == edgeItemMap.end()) {
        qDebug() << "Edge removed but edge id not exist:" << edgeId;
        return;
    }
    if (m_hoveredEdgeItem == edgeItemIt->second.first)
        m_hoveredEdgeItem = nullptr;
    if (m_hoveredEdgeItem == edgeItemIt->second.second)
        m_hoveredEdgeItem = nullptr;
    if (m_checkedEdgeItem == edgeItemIt->second.first)
        m_checkedEdgeItem = nullptr;
    if (m_checkedEdgeItem == edgeItemIt->second.second)
        m_checkedEdgeItem = nullptr;
    delete edgeItemIt->second.first;
    delete edgeItemIt->second.second;
    edgeItemMap.erase(edgeItemIt);
}

void SkeletonGraphicsWidget::nodeRadiusChanged(QUuid nodeId)
{
    const SkeletonNode *node = m_document->findNode(nodeId);
    if (nullptr == node) {
        qDebug() << "Node changed but node id not exist:" << nodeId;
        return;
    }
    auto it = nodeItemMap.find(nodeId);
    if (it == nodeItemMap.end()) {
        qDebug() << "Node not found:" << nodeId;
        return;
    }
    float sceneRadius = sceneRadiusFromUnified(node->radius);
    it->second.first->setRadius(sceneRadius);
    it->second.second->setRadius(sceneRadius);
}

void SkeletonGraphicsWidget::nodeOriginChanged(QUuid nodeId)
{
    const SkeletonNode *node = m_document->findNode(nodeId);
    if (nullptr == node) {
        qDebug() << "Node changed but node id not exist:" << nodeId;
        return;
    }
    auto it = nodeItemMap.find(nodeId);
    if (it == nodeItemMap.end()) {
        qDebug() << "Node not found:" << nodeId;
        return;
    }
    QPointF mainPos = scenePosFromUnified(QPointF(node->x, node->y));
    QPointF sidePos = scenePosFromUnified(QPointF(node->z, node->y));
    it->second.first->setOrigin(mainPos);
    it->second.second->setOrigin(sidePos);
    for (auto edgeIt = node->edgeIds.begin(); edgeIt != node->edgeIds.end(); edgeIt++) {
        auto edgeItemIt = edgeItemMap.find(*edgeIt);
        if (edgeItemIt == edgeItemMap.end()) {
            qDebug() << "Edge item not found:" << *edgeIt;
            continue;
        }
        edgeItemIt->second.first->updateAppearance();
        edgeItemIt->second.second->updateAppearance();
    }
}

void SkeletonGraphicsWidget::edgeChanged(QUuid edgeId)
{
}

