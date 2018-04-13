#include <QDebug>
#include <QScrollBar>
#include <QGuiApplication>
#include <cmath>
#include <QtGlobal>
#include <algorithm>
#include <QVector2D>
#include <QMenu>
#include <QApplication>
#include <QClipboard>
#include <QMatrix4x4>
#include "skeletongraphicswidget.h"
#include "theme.h"
#include "util.h"
#include "skeletonxml.h"

SkeletonGraphicsWidget::SkeletonGraphicsWidget(const SkeletonDocument *document) :
    m_document(document),
    m_turnaroundChanged(false),
    m_turnaroundLoader(nullptr),
    m_dragStarted(false),
    m_cursorNodeItem(nullptr),
    m_cursorEdgeItem(nullptr),
    m_addFromNodeItem(nullptr),
    m_moveStarted(false),
    m_hoveredNodeItem(nullptr),
    m_hoveredEdgeItem(nullptr),
    m_lastAddedX(0),
    m_lastAddedY(0),
    m_lastAddedZ(0),
    m_selectionItem(nullptr),
    m_rangeSelectionStarted(false),
    m_mouseEventFromSelf(false),
    m_lastRot(0)
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
    m_cursorNodeItem->setData(0, "cursorNode");
    scene()->addItem(m_cursorNodeItem);
    
    m_cursorEdgeItem = new SkeletonGraphicsEdgeItem();
    m_cursorEdgeItem->hide();
    m_cursorEdgeItem->setData(0, "cursorEdge");
    scene()->addItem(m_cursorEdgeItem);
    
    m_selectionItem = new SkeletonGraphicsSelectionItem();
    m_selectionItem->hide();
    scene()->addItem(m_selectionItem);
    
    scene()->setSceneRect(rect());
    
    setMouseTracking(true);
    
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &SkeletonGraphicsWidget::customContextMenuRequested, this, &SkeletonGraphicsWidget::showContextMenu);
}

void SkeletonGraphicsWidget::showContextMenu(const QPoint &pos)
{
    if (SkeletonDocumentEditMode::Add == m_document->editMode) {
        emit setEditMode(SkeletonDocumentEditMode::Select);
        return;
    }
    
    if (SkeletonDocumentEditMode::Select != m_document->editMode) {
        return;
    }
    
    QMenu contextMenu(this);
    
    QAction addAction(tr("Add..."), this);
    connect(&addAction, &QAction::triggered, [=]() {
        emit setEditMode(SkeletonDocumentEditMode::Add);
    });
    contextMenu.addAction(&addAction);
    
    QAction undoAction(tr("Undo"), this);
    if (m_document->undoable()) {
        connect(&undoAction, &QAction::triggered, m_document, &SkeletonDocument::undo);
        contextMenu.addAction(&undoAction);
    }
    
    QAction redoAction(tr("Redo"), this);
    if (m_document->redoable()) {
        connect(&redoAction, &QAction::triggered, m_document, &SkeletonDocument::redo);
        contextMenu.addAction(&redoAction);
    }
    
    QAction deleteAction(tr("Delete"), this);
    if (hasSelection()) {
        connect(&deleteAction, &QAction::triggered, this, &SkeletonGraphicsWidget::deleteSelected);
        contextMenu.addAction(&deleteAction);
    }
    
    QAction breakAction(tr("Break"), this);
    if (hasEdgeSelection()) {
        connect(&breakAction, &QAction::triggered, this, &SkeletonGraphicsWidget::breakSelected);
        contextMenu.addAction(&breakAction);
    }
    
    QAction connectAction(tr("Connect"), this);
    if (hasTwoDisconnectedNodesSelection()) {
        connect(&connectAction, &QAction::triggered, this, &SkeletonGraphicsWidget::connectSelected);
        contextMenu.addAction(&connectAction);
    }
    
    QAction cutAction(tr("Cut"), this);
    if (hasSelection()) {
        connect(&cutAction, &QAction::triggered, this, &SkeletonGraphicsWidget::cut);
        contextMenu.addAction(&cutAction);
    }
    
    QAction copyAction(tr("Copy"), this);
    if (hasSelection()) {
        connect(&copyAction, &QAction::triggered, this, &SkeletonGraphicsWidget::copy);
        contextMenu.addAction(&copyAction);
    }
    
    QAction pasteAction(tr("Paste"), this);
    if (m_document->hasPastableContentInClipboard()) {
        connect(&pasteAction, &QAction::triggered, m_document, &SkeletonDocument::paste);
        contextMenu.addAction(&pasteAction);
    }
    
    QAction flipHorizontallyAction(tr("H Flip"), this);
    if (hasMultipleSelection()) {
        connect(&flipHorizontallyAction, &QAction::triggered, this, &SkeletonGraphicsWidget::flipHorizontally);
        contextMenu.addAction(&flipHorizontallyAction);
    }
    
    QAction flipVerticallyAction(tr("V Flip"), this);
    if (hasMultipleSelection()) {
        connect(&flipVerticallyAction, &QAction::triggered, this, &SkeletonGraphicsWidget::flipVertically);
        contextMenu.addAction(&flipVerticallyAction);
    }
    
    QAction selectAllAction(tr("Select All"), this);
    if (hasItems()) {
        connect(&selectAllAction, &QAction::triggered, this, &SkeletonGraphicsWidget::selectAll);
        contextMenu.addAction(&selectAllAction);
    }
    
    QAction selectPartAllAction(tr("Select Part"), this);
    if (hasItems()) {
        connect(&selectPartAllAction, &QAction::triggered, this, &SkeletonGraphicsWidget::selectPartAll);
        contextMenu.addAction(&selectPartAllAction);
    }
    
    QAction unselectAllAction(tr("Unselect All"), this);
    if (hasSelection()) {
        connect(&unselectAllAction, &QAction::triggered, this, &SkeletonGraphicsWidget::unselectAll);
        contextMenu.addAction(&unselectAllAction);
    }
    
    contextMenu.addSeparator();
    
    QAction exportResultAction(tr("Export..."), this);
    if (hasItems()) {
        connect(&exportResultAction, &QAction::triggered, this, &SkeletonGraphicsWidget::exportResult);
        contextMenu.addAction(&exportResultAction);
    }
    
    QAction changeTurnaroundAction(tr("Change Turnaround..."), this);
    connect(&changeTurnaroundAction, &QAction::triggered, [=]() {
        emit changeTurnaround();
    });
    contextMenu.addAction(&changeTurnaroundAction);

    contextMenu.exec(mapToGlobal(pos));
}

bool SkeletonGraphicsWidget::hasSelection()
{
    return !m_rangeSelectionSet.empty();
}

bool SkeletonGraphicsWidget::hasItems()
{
    return !nodeItemMap.empty();
}

bool SkeletonGraphicsWidget::hasMultipleSelection()
{
    return !m_rangeSelectionSet.empty() && !isSingleNodeSelected();
}

bool SkeletonGraphicsWidget::hasEdgeSelection()
{
    for (const auto &it: m_rangeSelectionSet) {
        if (it->data(0) == "edge")
            return true;
    }
    return false;
}

bool SkeletonGraphicsWidget::hasTwoDisconnectedNodesSelection()
{
    std::vector<QUuid> nodeIds;
    for (const auto &it: m_rangeSelectionSet) {
        if (it->data(0) == "node") {
            nodeIds.push_back(((SkeletonGraphicsNodeItem *)it)->id());
        }
    }
    if (nodeIds.size() != 2)
        return false;
    if (m_document->findEdgeByNodes(nodeIds[0], nodeIds[1]))
        return false;
    return true;
}

void SkeletonGraphicsWidget::breakSelected()
{
    std::set<QUuid> edgeIds;
    for (const auto &it: m_rangeSelectionSet) {
        if (it->data(0) == "edge")
            edgeIds.insert(((SkeletonGraphicsEdgeItem *)it)->id());
    }
    if (edgeIds.empty())
        return;
    emit batchChangeBegin();
    for (const auto &it: edgeIds) {
        emit breakEdge(it);
    }
    emit batchChangeEnd();
    emit groupOperationAdded();
}

void SkeletonGraphicsWidget::connectSelected()
{
    std::vector<QUuid> nodeIds;
    for (const auto &it: m_rangeSelectionSet) {
        if (it->data(0) == "node") {
            nodeIds.push_back(((SkeletonGraphicsNodeItem *)it)->id());
        }
    }
    if (nodeIds.size() != 2)
        return;
    if (m_document->findEdgeByNodes(nodeIds[0], nodeIds[1]))
        return;
    emit addEdge(nodeIds[0], nodeIds[1]);
    emit groupOperationAdded();
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
            setCursor(QCursor(Theme::awesome()->icon(fa::mousepointer).pixmap(Theme::toolIconFontSize, Theme::toolIconFontSize), Theme::toolIconFontSize / 5, 0));
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
    updateCursor();
    if (SkeletonDocumentEditMode::Add == m_document->editMode) {
        SkeletonGraphicsNodeItem *choosenNodeItem = nullptr;
        if (!m_rangeSelectionSet.empty()) {
            std::set<SkeletonGraphicsNodeItem *> nodeItems;
            readMergedSkeletonNodeSetFromRangeSelection(&nodeItems);
            if (nodeItems.size() == 1)
                choosenNodeItem = *nodeItems.begin();
        }
        m_addFromNodeItem = choosenNodeItem;
    }
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
    m_mouseEventFromSelf = true;
    if (mousePress(event)) {
        m_mouseEventFromSelf = false;
        return;
    }
    m_mouseEventFromSelf = false;
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
    
    if (SkeletonDocumentEditMode::Select == m_document->editMode) {
        if (m_rangeSelectionStarted) {
            QPointF mouseScenePos = mouseEventScenePos(event);
            m_selectionItem->updateRange(m_rangeSelectionStartPos, mouseScenePos);
            if (!m_selectionItem->isVisible())
                m_selectionItem->setVisible(true);
            checkRangeSelection();
            return true;
        }
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
        if (m_addFromNodeItem) {
            m_cursorEdgeItem->setEndpoints(m_addFromNodeItem, m_cursorNodeItem);
            if (!m_cursorEdgeItem->isVisible()) {
                m_cursorEdgeItem->show();
            }
        }
        return true;
    }
    
    if (SkeletonDocumentEditMode::Select == m_document->editMode) {
        if (m_moveStarted && !m_rangeSelectionSet.empty()) {
            QPointF mouseScenePos = mouseEventScenePos(event);
            float byX = mouseScenePos.x() - m_lastScenePos.x();
            float byY = mouseScenePos.y() - m_lastScenePos.y();
            if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier)) {
                rotateSelected(byX);
            } else {
                moveSelected(byX, byY);
            }
            m_lastScenePos = mouseScenePos;
            m_moveHappened = true;
            return true;
        }
    }
    
    return false;
}

void SkeletonGraphicsWidget::rotateSelected(int degree)
{
    if (m_rangeSelectionSet.empty())
        return;
    
    std::set<SkeletonGraphicsNodeItem *> nodeItems;
    readMergedSkeletonNodeSetFromRangeSelection(&nodeItems);
    
    QVector2D center = centerOfNodeItemSet(nodeItems);
    qNormalizeAngle(degree);
    for (const auto &it: nodeItems) {
        SkeletonGraphicsNodeItem *nodeItem = it;
        QMatrix4x4 mat;
        QVector3D centerPoint(center.x(), center.y(), 0);
        QPointF nodeOrigin = nodeItem->origin();
        QVector3D nodeOriginPoint(nodeOrigin.x(), nodeOrigin.y(), 0);
        QVector3D p = nodeOriginPoint - centerPoint;
        mat.rotate(degree, 0, 0, 1);
        p = mat * p;
        QVector3D finalPoint = p + centerPoint;
        float byX = sceneRadiusToUnified(finalPoint.x() - nodeOrigin.x());
        float byY = sceneRadiusToUnified(finalPoint.y() - nodeOrigin.y());
        if (SkeletonProfile::Main == nodeItem->profile()) {
            emit moveNodeBy(nodeItem->id(), byX, byY, 0);
        } else {
            emit moveNodeBy(nodeItem->id(), 0, byY, byX);
        }
    }
}

void SkeletonGraphicsWidget::moveSelected(float byX, float byY)
{
    if (m_rangeSelectionSet.empty())
        return;
    
    byX = sceneRadiusToUnified(byX);
    byY = sceneRadiusToUnified(byY);
    std::set<SkeletonGraphicsNodeItem *> nodeItems;
    readMergedSkeletonNodeSetFromRangeSelection(&nodeItems);
    
    for (const auto &it: nodeItems) {
        SkeletonGraphicsNodeItem *nodeItem = it;
        if (SkeletonProfile::Main == nodeItem->profile()) {
            emit moveNodeBy(nodeItem->id(), byX, byY, 0);
        } else {
            emit moveNodeBy(nodeItem->id(), 0, byY, byX);
        }
    }
}

void SkeletonGraphicsWidget::zoomSelected(float delta)
{
    if (m_rangeSelectionSet.empty())
        return;
    
    std::set<SkeletonGraphicsNodeItem *> nodeItems;
    readMergedSkeletonNodeSetFromRangeSelection(&nodeItems);
    
    float unifiedDelta = sceneRadiusToUnified(delta);
    for (const auto &it: nodeItems) {
        SkeletonGraphicsNodeItem *nodeItem = it;
        emit scaleNodeByAddRadius(nodeItem->id(), unifiedDelta);
    }
}

void SkeletonGraphicsWidget::scaleSelected(float delta)
{
    if (m_rangeSelectionSet.empty())
        return;
    
    std::set<SkeletonGraphicsNodeItem *> nodeItems;
    readMergedSkeletonNodeSetFromRangeSelection(&nodeItems);
    
    QVector2D center = centerOfNodeItemSet(nodeItems);
    for (const auto &nodeItem: nodeItems) {
        QVector2D origin = QVector2D(nodeItem->origin());
        QVector2D ray = (center - origin) * 0.01 * delta;
        float byX = -sceneRadiusToUnified(ray.x());
        float byY = -sceneRadiusToUnified(ray.y());
        if (SkeletonProfile::Main == nodeItem->profile()) {
            emit moveNodeBy(nodeItem->id(), byX, byY, 0);
        } else {
            emit moveNodeBy(nodeItem->id(), 0, byY, byX);
        }
    }
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
        if (!m_rangeSelectionSet.empty()) {
            if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier)) {
                scaleSelected(delta);
            } else {
                zoomSelected(delta);
            }
            emit groupOperationAdded();
            return true;
        } else if (m_hoveredNodeItem) {
            float unifiedDelta = sceneRadiusToUnified(delta);
            emit scaleNodeByAddRadius(m_hoveredNodeItem->id(), unifiedDelta);
            emit groupOperationAdded();
            return true;
        }
    }
    return false;
}

QVector2D SkeletonGraphicsWidget::centerOfNodeItemSet(const std::set<SkeletonGraphicsNodeItem *> &set)
{
    QVector2D center;
    for (const auto &nodeItem: set) {
        center += QVector2D(nodeItem->origin());
    }
    center /= set.size();
    return center;
}

void SkeletonGraphicsWidget::flipHorizontally()
{
    std::set<SkeletonGraphicsNodeItem *> nodeItems;
    readMergedSkeletonNodeSetFromRangeSelection(&nodeItems);
    if (nodeItems.empty())
        return;
    QVector2D center = centerOfNodeItemSet(nodeItems);
    for (const auto &nodeItem: nodeItems) {
        QPointF origin = nodeItem->origin();
        float offset = origin.x() - center.x();
        float unifiedOffset = -sceneRadiusToUnified(offset * 2);
        if (SkeletonProfile::Main == nodeItem->profile()) {
            emit moveNodeBy(nodeItem->id(), unifiedOffset, 0, 0);
        } else {
            emit moveNodeBy(nodeItem->id(), 0, 0, unifiedOffset);
        }
    }
}

void SkeletonGraphicsWidget::flipVertically()
{
    std::set<SkeletonGraphicsNodeItem *> nodeItems;
    readMergedSkeletonNodeSetFromRangeSelection(&nodeItems);
    if (nodeItems.empty())
        return;
    QVector2D center = centerOfNodeItemSet(nodeItems);
    for (const auto &nodeItem: nodeItems) {
        QPointF origin = nodeItem->origin();
        float offset = origin.y() - center.y();
        float unifiedOffset = -sceneRadiusToUnified(offset * 2);
        emit moveNodeBy(nodeItem->id(), 0, unifiedOffset, 0);
    }
}

bool SkeletonGraphicsWidget::mouseRelease(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        bool processed = m_dragStarted || m_moveStarted || m_rangeSelectionStarted;
        if (m_dragStarted) {
            m_dragStarted = false;
            updateCursor();
        }
        if (m_moveStarted) {
            m_moveStarted = false;
            m_lastRot = 0;
            if (m_moveHappened)
                emit groupOperationAdded();
        }
        if (m_rangeSelectionStarted) {
            m_selectionItem->hide();
            m_rangeSelectionStarted = false;
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
            setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
            scale(1.5, 1.5);
            setTransformationAnchor(lastAnchor);
            return true;
        } else if (SkeletonDocumentEditMode::ZoomOut == m_document->editMode) {
            ViewportAnchor lastAnchor = transformationAnchor();
            setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
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
                if (m_addFromNodeItem) {
                    if (m_hoveredNodeItem && m_addFromNodeItem &&
                            m_hoveredNodeItem != m_addFromNodeItem &&
                            m_hoveredNodeItem->profile() == m_addFromNodeItem->profile()) {
                        if (m_document->findEdgeByNodes(m_addFromNodeItem->id(), m_hoveredNodeItem->id()))
                            return true;
                        emit addEdge(m_addFromNodeItem->id(), m_hoveredNodeItem->id());
                        emit groupOperationAdded();
                        return true;
                    }
                }
                QPointF mainProfile = m_cursorNodeItem->origin();
                QPointF sideProfile = mainProfile;
                if (m_addFromNodeItem) {
                    auto itemIt = nodeItemMap.find(m_addFromNodeItem->id());
                    if (SkeletonProfile::Main == m_addFromNodeItem->profile())
                        sideProfile.setX(itemIt->second.second->origin().x());
                    else
                        mainProfile.setX(itemIt->second.first->origin().x());
                } else {
                    if (mainProfile.x() >= scene()->width() / 2) {
                        sideProfile.setX(mainProfile.x() - scene()->width() / 4);
                    } else {
                        sideProfile.setX(mainProfile.x() +scene()->width() / 4);
                    }
                }
                QPointF unifiedMainPos = scenePosToUnified(mainProfile);
                QPointF unifiedSidePos = scenePosToUnified(sideProfile);
                if (isFloatEqual(m_lastAddedX, unifiedMainPos.x()) && isFloatEqual(m_lastAddedY, unifiedMainPos.y()) && isFloatEqual(m_lastAddedZ, unifiedSidePos.x()))
                    return true;
                m_lastAddedX = unifiedMainPos.x();
                m_lastAddedY = unifiedMainPos.y();
                m_lastAddedZ = unifiedSidePos.x();
                qDebug() << "Emit add node " << m_lastAddedX << m_lastAddedY << m_lastAddedZ;
                emit addNode(unifiedMainPos.x(), unifiedMainPos.y(), unifiedSidePos.x(), sceneRadiusToUnified(m_cursorNodeItem->radius()), nullptr == m_addFromNodeItem ? QUuid() : m_addFromNodeItem->id());
                emit groupOperationAdded();
                return true;
            }
        } else if (SkeletonDocumentEditMode::Select == m_document->editMode) {
            //if (m_mouseEventFromSelf) {
                bool processed = false;
                if ((nullptr == m_hoveredNodeItem || m_rangeSelectionSet.find(m_hoveredNodeItem) == m_rangeSelectionSet.end()) &&
                        (nullptr == m_hoveredEdgeItem || m_rangeSelectionSet.find(m_hoveredEdgeItem) == m_rangeSelectionSet.end())) {
                    if (!QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier)) {
                        clearRangeSelection();
                    }
                    if (!QGuiApplication::queryKeyboardModifiers().testFlag(Qt::AltModifier)) {
                        if (m_hoveredNodeItem) {
                            addItemToRangeSelection(m_hoveredNodeItem);
                        } else if (m_hoveredEdgeItem) {
                            addItemToRangeSelection(m_hoveredEdgeItem);
                        }
                    }
                }
                if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::AltModifier)) {
                    if (m_hoveredNodeItem) {
                        removeItemFromRangeSelection(m_hoveredNodeItem);
                    } else if (m_hoveredEdgeItem) {
                        removeItemFromRangeSelection(m_hoveredEdgeItem);
                    }
                }
                if (!m_rangeSelectionSet.empty()) {
                    if (!QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier)) {
                        if (!m_moveStarted) {
                            m_moveStarted = true;
                            m_lastScenePos = mouseEventScenePos(event);
                            m_moveHappened = false;
                            processed = true;
                        }
                    }
                }
                if (processed) {
                    return true;
                }
            //}
        }
    }
    
    if (event->button() == Qt::LeftButton) {
        if (SkeletonDocumentEditMode::Select == m_document->editMode) {
            if (!m_rangeSelectionStarted) {
                m_rangeSelectionStartPos = mouseEventScenePos(event);
                m_rangeSelectionStarted = true;
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
    if (m_hoveredNodeItem || m_hoveredEdgeItem) {
        selectPartAll();
        return true;
    }
    if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier)) {
        emit open();
        return true;
    }
    return false;
}

void SkeletonGraphicsWidget::deleteSelected()
{
    if (!m_rangeSelectionSet.empty()) {
        emit batchChangeBegin();
        std::set<QUuid> nodeIdSet;
        std::set<QUuid> edgeIdSet;
        readSkeletonNodeAndEdgeIdSetFromRangeSelection(&nodeIdSet, &edgeIdSet);
        for (const auto &id: edgeIdSet) {
            emit removeEdge(id);
        }
        for (const auto &id: nodeIdSet) {
            emit removeNode(id);
        }
        emit batchChangeEnd();
    }
}

bool SkeletonGraphicsWidget::keyPress(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete || event->key() ==Qt::Key_Backspace) {
        bool processed = false;
        if (!m_rangeSelectionSet.empty()) {
            deleteSelected();
            processed = true;
        }
        if (processed) {
            emit groupOperationAdded();
            return true;
        }
    } else if (event->key() == Qt::Key_A) {
        if (SkeletonDocumentEditMode::Add == m_document->editMode) {
            emit setEditMode(SkeletonDocumentEditMode::Select);
        } else {
            emit setEditMode(SkeletonDocumentEditMode::Add);
        }
        return true;
    } else if (event->key() == Qt::Key_Z) {
        if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier)) {
            if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier)) {
                emit redo();
            } else {
                emit undo();
            }
        }
    } else if (event->key() == Qt::Key_Y) {
        if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier)) {
            if (!QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier)) {
                emit redo();
            }
        }
    } else if (event->key() == Qt::Key_X) {
        if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier)) {
            cut();
        }
    } else if (event->key() == Qt::Key_C) {
        if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier)) {
            copy();
        }
    } else if (event->key() == Qt::Key_V) {
        if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier)) {
            emit paste();
        }
    } else if (event->key() == Qt::Key_S) {
        if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier)) {
            emit save();
        } else {
            emit setEditMode(SkeletonDocumentEditMode::Select);
        }
    } else if (event->key() == Qt::Key_D) {
        emit setEditMode(SkeletonDocumentEditMode::Drag);
    } else if (event->key() == Qt::Key_Minus) {
        if (SkeletonDocumentEditMode::Select == m_document->editMode && hasSelection()) {
            zoomSelected(-1);
            emit groupOperationAdded();
        }
    } else if (event->key() == Qt::Key_Equal) {
        if (SkeletonDocumentEditMode::Select == m_document->editMode && hasSelection()) {
            zoomSelected(1);
            emit groupOperationAdded();
        }
    } else if (event->key() == Qt::Key_Comma) {
        if (SkeletonDocumentEditMode::Select == m_document->editMode && hasSelection()) {
            rotateSelected(-1);
            emit groupOperationAdded();
        }
    } else if (event->key() == Qt::Key_Period) {
        if (SkeletonDocumentEditMode::Select == m_document->editMode && hasSelection()) {
            rotateSelected(1);
            emit groupOperationAdded();
        }
    } else if (event->key() == Qt::Key_Left) {
        if (SkeletonDocumentEditMode::Select == m_document->editMode && hasSelection()) {
            moveSelected(-1, 0);
            emit groupOperationAdded();
        }
    } else if (event->key() == Qt::Key_Right) {
        if (SkeletonDocumentEditMode::Select == m_document->editMode && hasSelection()) {
            moveSelected(1, 0);
            emit groupOperationAdded();
        }
    } else if (event->key() == Qt::Key_Up) {
        if (SkeletonDocumentEditMode::Select == m_document->editMode && hasSelection()) {
            moveSelected(0, -1);
            emit groupOperationAdded();
        }
    } else if (event->key() == Qt::Key_Down) {
        if (SkeletonDocumentEditMode::Select == m_document->editMode && hasSelection()) {
            moveSelected(0, 1);
            emit groupOperationAdded();
        }
    } else if (event->key() == Qt::Key_BracketLeft) {
        if (SkeletonDocumentEditMode::Select == m_document->editMode && hasSelection()) {
            scaleSelected(-1);
            emit groupOperationAdded();
        }
    } else if (event->key() == Qt::Key_BracketRight) {
        if (SkeletonDocumentEditMode::Select == m_document->editMode && hasSelection()) {
            scaleSelected(1);
            emit groupOperationAdded();
        }
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
    if (nodeItemMap.find(nodeId) != nodeItemMap.end()) {
        qDebug() << "New node added but node item already exist:" << nodeId;
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
    
    if (nullptr == m_addFromNodeItem) {
        m_addFromNodeItem = mainProfileItem;
    } else {
        if (SkeletonProfile::Main == m_addFromNodeItem->profile()) {
            m_addFromNodeItem = mainProfileItem;
        } else {
            m_addFromNodeItem = sideProfileItem;
        }
        m_cursorEdgeItem->setEndpoints(m_addFromNodeItem, m_cursorNodeItem);
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
    if (edgeItemMap.find(edgeId) != edgeItemMap.end()) {
        qDebug() << "New edge added but edge item already exist:" << edgeId;
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

void SkeletonGraphicsWidget::removeItem(QGraphicsItem *item)
{
    if (m_hoveredNodeItem == item)
        m_hoveredNodeItem = nullptr;
    if (m_addFromNodeItem == item)
        m_addFromNodeItem = nullptr;
    if (m_hoveredEdgeItem == item)
        m_hoveredEdgeItem = nullptr;
    m_rangeSelectionSet.erase(item);
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
    removeItem(nodeItemIt->second.first);
    removeItem(nodeItemIt->second.second);
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
    removeItem(edgeItemIt->second.first);
    removeItem(edgeItemIt->second.second);
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

void SkeletonGraphicsWidget::partVisibleStateChanged(QUuid partId)
{
    const SkeletonPart *part = m_document->findPart(partId);
    for (const auto &nodeId: part->nodeIds) {
        const SkeletonNode *node = m_document->findNode(nodeId);
        for (auto edgeIt = node->edgeIds.begin(); edgeIt != node->edgeIds.end(); edgeIt++) {
            auto edgeItemIt = edgeItemMap.find(*edgeIt);
            if (edgeItemIt == edgeItemMap.end()) {
                qDebug() << "Edge item not found:" << *edgeIt;
                continue;
            }
            edgeItemIt->second.first->setVisible(part->visible);
            edgeItemIt->second.second->setVisible(part->visible);
        }
        auto nodeItemIt = nodeItemMap.find(nodeId);
        if (nodeItemIt == nodeItemMap.end()) {
            qDebug() << "Node item not found:" << nodeId;
            continue;
        }
        nodeItemIt->second.first->setVisible(part->visible);
        nodeItemIt->second.second->setVisible(part->visible);
    }
}

bool SkeletonGraphicsWidget::checkSkeletonItem(QGraphicsItem *item, bool checked)
{
    if (checked) {
        if (!item->isVisible())
            return false;
    }
    if (item->data(0) == "node") {
        SkeletonGraphicsNodeItem *nodeItem = (SkeletonGraphicsNodeItem *)item;
        if (checked) {
            if (!m_document->isNodeEditable(nodeItem->id())) {
                return false;
            }
        }
        if (checked != nodeItem->checked())
            nodeItem->setChecked(checked);
        return true;
    } else if (item->data(0) == "edge") {
        SkeletonGraphicsEdgeItem *edgeItem = (SkeletonGraphicsEdgeItem *)item;
        if (checked) {
            if (!m_document->isEdgeEditable(edgeItem->id())) {
                return false;
            }
        }
        if (checked != edgeItem->checked())
            edgeItem->setChecked(checked);
        return true;
    }
    return false;
}

SkeletonProfile SkeletonGraphicsWidget::readSkeletonItemProfile(QGraphicsItem *item)
{
    if (item->data(0) == "node") {
        SkeletonGraphicsNodeItem *nodeItem = (SkeletonGraphicsNodeItem *)item;
        return nodeItem->profile();
    } else if (item->data(0) == "edge") {
        SkeletonGraphicsEdgeItem *edgeItem = (SkeletonGraphicsEdgeItem *)item;
        return edgeItem->profile();
    }
    return SkeletonProfile::Unknown;
}

void SkeletonGraphicsWidget::checkRangeSelection()
{
    std::set<QGraphicsItem *> newSet;
    std::set<QGraphicsItem *> deleteSet;
    std::set<QGraphicsItem *> forceDeleteSet;
    SkeletonProfile choosenProfile = SkeletonProfile::Unknown;
    if (!m_rangeSelectionSet.empty()) {
        auto it = m_rangeSelectionSet.begin();
        choosenProfile = readSkeletonItemProfile(*it);
    }
    if (m_selectionItem->isVisible()) {
        QList<QGraphicsItem *> items = scene()->items(m_selectionItem->sceneBoundingRect());
        for (auto it = items.begin(); it != items.end(); it++) {
            QGraphicsItem *item = *it;
            if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::AltModifier)) {
                checkSkeletonItem(item, false);
                forceDeleteSet.insert(item);
            } else {
                if (SkeletonProfile::Unknown == choosenProfile) {
                    if (checkSkeletonItem(item, true)) {
                        choosenProfile = readSkeletonItemProfile(item);
                        newSet.insert(item);
                    }
                } else if (choosenProfile == readSkeletonItemProfile(item)) {
                    if (checkSkeletonItem(item, true))
                        newSet.insert(item);
                }
            }
        }
    }
    if (!QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier)) {
        for (const auto &item: m_rangeSelectionSet) {
            if (newSet.find(item) == newSet.end()) {
                checkSkeletonItem(item, false);
                deleteSet.insert(item);
            }
        }
    }
    for (const auto &item: newSet) {
        m_rangeSelectionSet.insert(item);
    }
    for (const auto &item: deleteSet) {
        m_rangeSelectionSet.erase(item);
    }
    for (const auto &item: forceDeleteSet) {
        m_rangeSelectionSet.erase(item);
    }
}

void SkeletonGraphicsWidget::clearRangeSelection()
{
    for (const auto &item: m_rangeSelectionSet) {
        checkSkeletonItem(item, false);
    }
    m_rangeSelectionSet.clear();
}

void SkeletonGraphicsWidget::readMergedSkeletonNodeSetFromRangeSelection(std::set<SkeletonGraphicsNodeItem *> *nodeItemSet)
{
    for (const auto &it: m_rangeSelectionSet) {
        QGraphicsItem *item = it;
        if (item->data(0) == "node") {
            nodeItemSet->insert((SkeletonGraphicsNodeItem *)item);
        } else if (item->data(0) == "edge") {
            SkeletonGraphicsEdgeItem *edgeItem = (SkeletonGraphicsEdgeItem *)item;
            if (edgeItem->firstItem() && edgeItem->secondItem()) {
                nodeItemSet->insert(edgeItem->firstItem());
                nodeItemSet->insert(edgeItem->secondItem());
            }
        }
    }
}

void SkeletonGraphicsWidget::readSkeletonNodeAndEdgeIdSetFromRangeSelection(std::set<QUuid> *nodeIdSet, std::set<QUuid> *edgeIdSet)
{
    for (const auto &it: m_rangeSelectionSet) {
        QGraphicsItem *item = it;
        if (item->data(0) == "node") {
            nodeIdSet->insert(((SkeletonGraphicsNodeItem *)item)->id());
        } else if (item->data(0) == "edge") {
            edgeIdSet->insert(((SkeletonGraphicsEdgeItem *)item)->id());
        }
    }
}

bool SkeletonGraphicsWidget::readSkeletonNodeAndAnyEdgeOfNodeFromRangeSelection(SkeletonGraphicsNodeItem **nodeItem, SkeletonGraphicsEdgeItem **edgeItem)
{
    SkeletonGraphicsNodeItem *choosenNodeItem = nullptr;
    SkeletonGraphicsEdgeItem *choosenEdgeItem = nullptr;
    for (const auto &it: m_rangeSelectionSet) {
        QGraphicsItem *item = it;
        if (item->data(0) == "node") {
            choosenNodeItem = (SkeletonGraphicsNodeItem *)item;
        } else if (item->data(0) == "edge") {
            choosenEdgeItem = (SkeletonGraphicsEdgeItem *)item;
        }
        if (choosenNodeItem && choosenEdgeItem)
            break;
    }
    if (!choosenNodeItem || !choosenEdgeItem)
        return false;
    if (choosenNodeItem->profile() != choosenEdgeItem->profile())
        return false;
    if (choosenNodeItem != choosenEdgeItem->firstItem() && choosenNodeItem != choosenEdgeItem->secondItem())
        return false;
    if (nodeItem)
        *nodeItem = choosenNodeItem;
    if (edgeItem)
        *edgeItem = choosenEdgeItem;
    if (m_rangeSelectionSet.size() != 2)
        return false;
    return true;
}

void SkeletonGraphicsWidget::selectPartAll()
{
    unselectAll();
    SkeletonProfile choosenProfile = SkeletonProfile::Main;
    QUuid choosenPartId;
    if (m_hoveredNodeItem) {
        choosenProfile = m_hoveredNodeItem->profile();
        const SkeletonNode *node = m_document->findNode(m_hoveredNodeItem->id());
        if (node)
            choosenPartId = node->partId;
    } else if (m_hoveredEdgeItem) {
        choosenProfile = m_hoveredEdgeItem->profile();
        const SkeletonEdge *edge = m_document->findEdge(m_hoveredEdgeItem->id());
        if (edge)
            choosenPartId = edge->partId;
    }
    for (const auto &it: nodeItemMap) {
        SkeletonGraphicsNodeItem *item = SkeletonProfile::Main == choosenProfile ? it.second.first : it.second.second;
        const SkeletonNode *node = m_document->findNode(item->id());
        if (!node)
            continue;
        if (choosenPartId.isNull()) {
            choosenPartId = node->partId;
        }
        if (node->partId != choosenPartId)
            continue;
        addItemToRangeSelection(item);
    }
    for (const auto &it: edgeItemMap) {
        SkeletonGraphicsEdgeItem *item = SkeletonProfile::Main == choosenProfile ? it.second.first : it.second.second;
        const SkeletonEdge *edge = m_document->findEdge(item->id());
        if (!edge)
            continue;
        if (choosenPartId.isNull()) {
            choosenPartId = edge->partId;
        }
        if (edge->partId != choosenPartId)
            continue;
        addItemToRangeSelection(item);
    }
}

void SkeletonGraphicsWidget::selectAll()
{
    unselectAll();
    SkeletonProfile choosenProfile = SkeletonProfile::Main;
    if (m_hoveredNodeItem) {
        choosenProfile = m_hoveredNodeItem->profile();
    } else if (m_hoveredEdgeItem) {
        choosenProfile = m_hoveredEdgeItem->profile();
    }
    for (const auto &it: nodeItemMap) {
        SkeletonGraphicsNodeItem *item = SkeletonProfile::Main == choosenProfile ? it.second.first : it.second.second;
        addItemToRangeSelection(item);
    }
    for (const auto &it: edgeItemMap) {
        SkeletonGraphicsEdgeItem *item = SkeletonProfile::Main == choosenProfile ? it.second.first : it.second.second;
        addItemToRangeSelection(item);
    }
}

void SkeletonGraphicsWidget::addItemToRangeSelection(QGraphicsItem *item)
{
    if (!item->isVisible())
        return;
    if (checkSkeletonItem(item, true))
        m_rangeSelectionSet.insert(item);
}

void SkeletonGraphicsWidget::removeItemFromRangeSelection(QGraphicsItem *item)
{
    checkSkeletonItem(item, false);
    m_rangeSelectionSet.erase(item);
}

void SkeletonGraphicsWidget::unselectAll()
{
    for (const auto &item: m_rangeSelectionSet) {
        checkSkeletonItem(item, false);
    }
    m_rangeSelectionSet.clear();
}

void SkeletonGraphicsWidget::cut()
{
    copy();
    deleteSelected();
}

void SkeletonGraphicsWidget::copy()
{
    std::set<QUuid> nodeIdSet;
    std::set<QUuid> edgeIdSet;
    readSkeletonNodeAndEdgeIdSetFromRangeSelection(&nodeIdSet, &edgeIdSet);
    if (nodeIdSet.empty())
        return;
    SkeletonSnapshot snapshot;
    m_document->toSnapshot(&snapshot, nodeIdSet);
    QString snapshotXml;
    QXmlStreamWriter xmlStreamWriter(&snapshotXml);
    saveSkeletonToXmlStream(&snapshot, &xmlStreamWriter);
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(snapshotXml);
}

void SkeletonGraphicsWidget::removeAllContent()
{
    nodeItemMap.clear();
    edgeItemMap.clear();
    m_rangeSelectionSet.clear();
    m_hoveredEdgeItem = nullptr;
    m_hoveredNodeItem = nullptr;
    m_addFromNodeItem = nullptr;
    m_cursorEdgeItem->hide();
    m_cursorNodeItem->hide();
    std::set<QGraphicsItem *> contentItems;
    for (const auto &it: items()) {
        QGraphicsItem *item = it;
        if (item->data(0) == "node") {
            contentItems.insert(item);
        } else if (item->data(0) == "edge") {
            contentItems.insert(item);
        }
    }
    for (const auto &it: contentItems) {
        QGraphicsItem *item = it;
        Q_ASSERT(item != m_cursorEdgeItem);
        Q_ASSERT(item != m_cursorNodeItem);
        scene()->removeItem(item);
    }
}

bool SkeletonGraphicsWidget::isSingleNodeSelected()
{
    if (m_rangeSelectionSet.size() != 1)
        return false;
    const auto &it = m_rangeSelectionSet.begin();
    QGraphicsItem *item = *it;
    return item->data(0) == "node";
}

