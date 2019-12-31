#include <QDebug>
#include <QScrollBar>
#include <QGuiApplication>
#include <cmath>
#include <QtGlobal>
#include <algorithm>
#include <QVector2D>
#include <QMenu>
#include <QApplication>
#include <QMatrix4x4>
#include <queue>
#include <QBitmap>
#include "skeletongraphicswidget.h"
#include "theme.h"
#include "util.h"
#include "snapshotxml.h"
#include "markiconcreator.h"

SkeletonGraphicsWidget::SkeletonGraphicsWidget(const SkeletonDocument *document) :
    m_document(document),
    m_backgroundItem(nullptr),
    m_turnaroundChanged(false),
    m_turnaroundLoader(nullptr),
    m_dragStarted(false),
    m_moveStarted(false),
    m_cursorNodeItem(nullptr),
    m_cursorEdgeItem(nullptr),
    m_addFromNodeItem(nullptr),
    m_hoveredNodeItem(nullptr),
    m_hoveredEdgeItem(nullptr),
    m_lastAddedX(false),
    m_lastAddedY(false),
    m_lastAddedZ(false),
    m_selectionItem(nullptr),
    m_markerItem(nullptr),
    m_rangeSelectionStarted(false),
    m_markerStarted(false),
    m_mouseEventFromSelf(false),
    m_moveHappened(false),
    m_lastRot(0),
    m_mainOriginItem(nullptr),
    m_sideOriginItem(nullptr),
    m_hoveredOriginItem(nullptr),
    m_checkedOriginItem(nullptr),
    m_ikMoveUpdateVersion(0),
    m_ikMover(nullptr),
    m_deferredRemoveTimer(nullptr),
    m_eventForwardingToModelWidget(false),
    m_modelWidget(nullptr),
    m_inTempDragMode(false),
    m_modeBeforeEnterTempDragMode(SkeletonDocumentEditMode::Select),
    m_nodePositionModifyOnly(false),
    m_mainProfileOnly(false),
    m_turnaroundOpacity(0.25),
    m_rotated(false),
    m_backgroundImage(nullptr)
{
    setRenderHint(QPainter::Antialiasing, false);
    setBackgroundBrush(QBrush(QWidget::palette().color(QWidget::backgroundRole()), Qt::SolidPattern));
    setContentsMargins(0, 0, 0, 0);
    setFrameStyle(QFrame::NoFrame);
    
    setAlignment(Qt::AlignCenter);
    
    setScene(new QGraphicsScene());
    
    m_backgroundItem = new QGraphicsPixmapItem();
    enableBackgroundBlur();
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
    
    m_markerItem = new SkeletonGraphicsMarkerItem();
    m_markerItem->hide();
    scene()->addItem(m_markerItem);
    
    m_mainOriginItem = new SkeletonGraphicsOriginItem(SkeletonProfile::Main);
    m_mainOriginItem->setRotated(m_rotated);
    m_mainOriginItem->hide();
    scene()->addItem(m_mainOriginItem);
    
    m_sideOriginItem = new SkeletonGraphicsOriginItem(SkeletonProfile::Side);
    m_sideOriginItem->hide();
    scene()->addItem(m_sideOriginItem);
    
    scene()->setSceneRect(rect());
    
    setMouseTracking(true);
    
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &SkeletonGraphicsWidget::customContextMenuRequested, this, &SkeletonGraphicsWidget::showContextMenu);
}

SkeletonGraphicsWidget::~SkeletonGraphicsWidget()
{
    delete m_backgroundImage;
}

void SkeletonGraphicsWidget::setRotated(bool rotated)
{
    if (m_rotated == rotated)
        return;
    m_rotated = rotated;
    if (nullptr != m_mainOriginItem)
        m_mainOriginItem->setRotated(m_rotated);
    updateItems();
}

void SkeletonGraphicsWidget::setModelWidget(ModelWidget *modelWidget)
{
    m_modelWidget = modelWidget;
}

void SkeletonGraphicsWidget::enableBackgroundBlur()
{
    m_backgroundItem->setOpacity(m_turnaroundOpacity);
}

void SkeletonGraphicsWidget::disableBackgroundBlur()
{
    m_backgroundItem->setOpacity(1);
}

void SkeletonGraphicsWidget::setBackgroundBlur(float turnaroundOpacity)
{
    m_turnaroundOpacity = turnaroundOpacity;
    m_backgroundItem->setOpacity(m_turnaroundOpacity);
}

void SkeletonGraphicsWidget::shortcutEscape()
{
    if (SkeletonDocumentEditMode::Add == m_document->editMode) {
        emit setEditMode(SkeletonDocumentEditMode::Select);
        return;
    }
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
    if (!m_nodePositionModifyOnly) {
        connect(&addAction, &QAction::triggered, [=]() {
            emit setEditMode(SkeletonDocumentEditMode::Add);
        });
        contextMenu.addAction(&addAction);
    }
    
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
    if (!m_nodePositionModifyOnly && hasSelection()) {
        connect(&deleteAction, &QAction::triggered, this, &SkeletonGraphicsWidget::deleteSelected);
        contextMenu.addAction(&deleteAction);
    }
    
    QAction breakAction(tr("Break"), this);
    if (!m_nodePositionModifyOnly && hasEdgeSelection()) {
        connect(&breakAction, &QAction::triggered, this, &SkeletonGraphicsWidget::breakSelected);
        contextMenu.addAction(&breakAction);
    }
    
    QAction connectAction(tr("Connect"), this);
    if (!m_nodePositionModifyOnly && hasTwoDisconnectedNodesSelection()) {
        connect(&connectAction, &QAction::triggered, this, &SkeletonGraphicsWidget::connectSelected);
        contextMenu.addAction(&connectAction);
    }
    
    QAction cutAction(tr("Cut"), this);
    if (!m_nodePositionModifyOnly && hasSelection()) {
        connect(&cutAction, &QAction::triggered, this, &SkeletonGraphicsWidget::cut);
        contextMenu.addAction(&cutAction);
    }
    
    QAction copyAction(tr("Copy"), this);
    if (!m_mainProfileOnly && hasNodeSelection()) {
        connect(&copyAction, &QAction::triggered, this, &SkeletonGraphicsWidget::copy);
        contextMenu.addAction(&copyAction);
    }
    
    QAction pasteAction(tr("Paste"), this);
    if (m_document->hasPastableNodesInClipboard()) {
        connect(&pasteAction, &QAction::triggered, m_document, &SkeletonDocument::paste);
        contextMenu.addAction(&pasteAction);
    }
    
    QAction flipHorizontallyAction(tr("H Flip"), this);
    if (!m_nodePositionModifyOnly && hasMultipleSelection()) {
        connect(&flipHorizontallyAction, &QAction::triggered, this, &SkeletonGraphicsWidget::flipHorizontally);
        contextMenu.addAction(&flipHorizontallyAction);
    }
    
    QAction flipVerticallyAction(tr("V Flip"), this);
    if (!m_nodePositionModifyOnly && hasMultipleSelection()) {
        connect(&flipVerticallyAction, &QAction::triggered, this, &SkeletonGraphicsWidget::flipVertically);
        contextMenu.addAction(&flipVerticallyAction);
    }
    
    QAction rotateClockwiseAction(tr("Rotate 90D CW"), this);
    if (!m_nodePositionModifyOnly && hasMultipleSelection()) {
        connect(&rotateClockwiseAction, &QAction::triggered, this, &SkeletonGraphicsWidget::rotateClockwise90Degree);
        contextMenu.addAction(&rotateClockwiseAction);
    }
    
    QAction rotateCounterclockwiseAction(tr("Rotate 90D CCW"), this);
    if (!m_nodePositionModifyOnly && hasMultipleSelection()) {
        connect(&rotateCounterclockwiseAction, &QAction::triggered, this, &SkeletonGraphicsWidget::rotateCounterclockwise90Degree);
        contextMenu.addAction(&rotateCounterclockwiseAction);
    }
    
    QAction switchXzAction(tr("Switch XZ"), this);
    if (!m_nodePositionModifyOnly && hasSelection()) {
        connect(&switchXzAction, &QAction::triggered, this, &SkeletonGraphicsWidget::switchSelectedXZ);
        contextMenu.addAction(&switchXzAction);
    }
    
    QAction switchChainSideAction(tr("Switch Chain Side"), this);
    if (m_nodePositionModifyOnly && !m_mainProfileOnly && hasNodeSelection()) {
        connect(&switchChainSideAction, &QAction::triggered, this, &SkeletonGraphicsWidget::switchSelectedChainSide);
        contextMenu.addAction(&switchChainSideAction);
    }
    
    QAction setCutFaceAction(tr("Cut Face..."), this);
    if (!m_nodePositionModifyOnly && hasSelection()) {
        connect(&setCutFaceAction, &QAction::triggered, this, [&]() {
            showSelectedCutFaceSettingPopup(mapFromGlobal(QCursor::pos()));
        });
        contextMenu.addAction(&setCutFaceAction);
    }
    
    QAction clearCutFaceAction(tr("Clear Cut Face"), this);
    if (!m_nodePositionModifyOnly && hasCutFaceAdjustedNodesSelection()) {
        connect(&clearCutFaceAction, &QAction::triggered, this, &SkeletonGraphicsWidget::clearSelectedCutFace);
        contextMenu.addAction(&clearCutFaceAction);
    }
    
    QAction createWrapPartsAction(tr("Create Wrap Parts"), this);
    if (!m_nodePositionModifyOnly && hasSelection()) {
        connect(&createWrapPartsAction, &QAction::triggered, this, [&]() {
            createWrapParts();
        });
        contextMenu.addAction(&createWrapPartsAction);
    }
    
    QAction alignToLocalCenterAction(tr("Local Center"), this);
    QAction alignToLocalVerticalCenterAction(tr("Local Vertical Center"), this);
    QAction alignToLocalHorizontalCenterAction(tr("Local Horizontal Center"), this);
    QAction alignToGlobalCenterAction(tr("Global Center"), this);
    QAction alignToGlobalVerticalCenterAction(tr("Global Vertical Center"), this);
    QAction alignToGlobalHorizontalCenterAction(tr("Global Horizontal Center"), this);
    if (!m_nodePositionModifyOnly && ((hasSelection() && m_document->originSettled()) || hasMultipleSelection())) {
        QMenu *subMenu = contextMenu.addMenu(tr("Align To"));
        
        if (hasMultipleSelection()) {
            connect(&alignToLocalCenterAction, &QAction::triggered, this, &SkeletonGraphicsWidget::alignSelectedToLocalCenter);
            subMenu->addAction(&alignToLocalCenterAction);
            
            connect(&alignToLocalVerticalCenterAction, &QAction::triggered, this, &SkeletonGraphicsWidget::alignSelectedToLocalVerticalCenter);
            subMenu->addAction(&alignToLocalVerticalCenterAction);
            
            connect(&alignToLocalHorizontalCenterAction, &QAction::triggered, this, &SkeletonGraphicsWidget::alignSelectedToLocalHorizontalCenter);
            subMenu->addAction(&alignToLocalHorizontalCenterAction);
        }
        
        if (hasSelection() && m_document->originSettled()) {
            connect(&alignToGlobalCenterAction, &QAction::triggered, this, &SkeletonGraphicsWidget::alignSelectedToGlobalCenter);
            subMenu->addAction(&alignToGlobalCenterAction);
            
            connect(&alignToGlobalVerticalCenterAction, &QAction::triggered, this, &SkeletonGraphicsWidget::alignSelectedToGlobalVerticalCenter);
            subMenu->addAction(&alignToGlobalVerticalCenterAction);
            
            connect(&alignToGlobalHorizontalCenterAction, &QAction::triggered, this, &SkeletonGraphicsWidget::alignSelectedToGlobalHorizontalCenter);
            subMenu->addAction(&alignToGlobalHorizontalCenterAction);
        }
    }
    
    QAction markAsNoneAction(tr("None"), this);
    QAction *markAsActions[(int)BoneMark::Count - 1];
    for (int i = 0; i < (int)BoneMark::Count - 1; i++) {
        markAsActions[i] = nullptr;
    }
    if (!m_nodePositionModifyOnly && hasNodeSelection()) {
        QMenu *subMenu = contextMenu.addMenu(tr("Mark As"));
        
        connect(&markAsNoneAction, &QAction::triggered, [=]() {
            setSelectedNodesBoneMark(BoneMark::None);
        });
        subMenu->addAction(&markAsNoneAction);
        
        subMenu->addSeparator();
        
        for (int i = 0; i < (int)BoneMark::Count - 1; i++) {
            BoneMark boneMark = (BoneMark)(i + 1);
            markAsActions[i] = new QAction(MarkIconCreator::createIcon(boneMark), BoneMarkToDispName(boneMark), this);
            connect(markAsActions[i], &QAction::triggered, [=]() {
                setSelectedNodesBoneMark(boneMark);
            });
            subMenu->addAction(markAsActions[i]);
        }
    }
    
    QAction colorizeAsBlankAction(tr("Blank"), this);
    QAction colorizeAsAutoColorAction(tr("Auto Color"), this);
    if (!m_nodePositionModifyOnly && hasNodeSelection()) {
        QMenu *subMenu = contextMenu.addMenu(tr("Colorize"));
        
        connect(&colorizeAsBlankAction, &QAction::triggered, this, &SkeletonGraphicsWidget::fadeSelected);
        subMenu->addAction(&colorizeAsBlankAction);
        
        connect(&colorizeAsAutoColorAction, &QAction::triggered, this, &SkeletonGraphicsWidget::colorizeSelected);
        subMenu->addAction(&colorizeAsAutoColorAction);
    }
    
    QAction selectAllAction(tr("Select All"), this);
    if (hasItems()) {
        connect(&selectAllAction, &QAction::triggered, this, &SkeletonGraphicsWidget::selectAll);
        contextMenu.addAction(&selectAllAction);
    }
    
    QAction selectPartAllAction(tr("Select Part"), this);
    if (!m_nodePositionModifyOnly && hasItems()) {
        connect(&selectPartAllAction, &QAction::triggered, this, &SkeletonGraphicsWidget::selectPartAll);
        contextMenu.addAction(&selectPartAllAction);
    }
    
    QAction unselectAllAction(tr("Unselect All"), this);
    if (hasSelection()) {
        connect(&unselectAllAction, &QAction::triggered, this, &SkeletonGraphicsWidget::unselectAll);
        contextMenu.addAction(&unselectAllAction);
    }

    contextMenu.exec(mapToGlobal(pos));
    
    for (int i = 0; i < (int)BoneMark::Count - 1; i++) {
        delete markAsActions[i];
    }
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

bool SkeletonGraphicsWidget::hasNodeSelection()
{
    for (const auto &it: m_rangeSelectionSet) {
        if (it->data(0) == "node")
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
    if (!m_document->isNodeConnectable(nodeIds[0]))
        return false;
    if (!m_document->isNodeConnectable(nodeIds[1]))
        return false;
    return true;
}

bool SkeletonGraphicsWidget::hasCutFaceAdjustedNodesSelection()
{
    for (const auto &it: m_rangeSelectionSet) {
        if (it->data(0) == "node") {
            const auto &nodeId = ((SkeletonGraphicsNodeItem *)it)->id();
            const SkeletonNode *node = m_document->findNode(nodeId);
            if (nullptr == node) {
                qDebug() << "Find node failed:" << nodeId;
                continue;
            }
            if (node->hasCutFaceSettings)
                return true;
        }
    }
    return false;
}

void SkeletonGraphicsWidget::fadeSelected()
{
    std::set<QUuid> partIds;
    for (const auto &it: m_rangeSelectionSet) {
        if (it->data(0) == "node") {
            SkeletonGraphicsNodeItem *nodeItem = (SkeletonGraphicsNodeItem *)it;
            const SkeletonNode *node = m_document->findNode(nodeItem->id());
            if (nullptr == node)
                continue;
            if (partIds.find(node->partId) != partIds.end())
                continue;
            partIds.insert(node->partId);
        }
    }
    if (partIds.empty())
        return;
    emit batchChangeBegin();
    for (const auto &it: partIds) {
        emit setPartColorState(it, false, Qt::white);
    }
    emit batchChangeEnd();
    emit groupOperationAdded();
}

void SkeletonGraphicsWidget::colorizeSelected()
{
    if (nullptr == m_backgroundImage)
        return;
    std::map<QUuid, std::map<QString, size_t>> sumOfColor;
    for (const auto &it: m_rangeSelectionSet) {
        if (it->data(0) == "node") {
            SkeletonGraphicsNodeItem *nodeItem = (SkeletonGraphicsNodeItem *)it;
            const SkeletonNode *node = m_document->findNode(nodeItem->id());
            if (nullptr == node)
                continue;
            const auto &position = nodeItem->origin();
            sumOfColor[node->partId][m_backgroundImage->pixelColor(position.x(), position.y()).name()]++;
        } else if (it->data(0) == "edge") {
            SkeletonGraphicsEdgeItem *edgeItem = (SkeletonGraphicsEdgeItem *)it;
            const SkeletonEdge *edge = m_document->findEdge(edgeItem->id());
            if (nullptr == edge)
                continue;
            const auto position = (edgeItem->firstItem()->origin() + edgeItem->secondItem()->origin()) / 2;
            sumOfColor[edge->partId][m_backgroundImage->pixelColor(position.x(), position.y()).name()]++;
        }
    }
    if (sumOfColor.empty())
        return;
    emit batchChangeBegin();
    for (const auto &it: sumOfColor) {
        int r = 0;
        int g = 0;
        int b = 0;
        for (const auto &freq: it.second) {
            QColor color(freq.first);
            r += color.red();
            g += color.green();
            b += color.blue();
        }
        QColor color(r / it.second.size(), g / it.second.size(), b / it.second.size());
        emit setPartColorState(it.first, true, color);
    }
    emit batchChangeEnd();
    emit groupOperationAdded();
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
    if (!m_document->isNodeConnectable(nodeIds[0]))
        return;
    if (!m_document->isNodeConnectable(nodeIds[1]))
        return;
    emit addEdge(nodeIds[0], nodeIds[1]);
    emit groupOperationAdded();
}

void SkeletonGraphicsWidget::alignSelectedToLocal(bool alignToVerticalCenter, bool alignToHorizontalCenter)
{
    if (!hasMultipleSelection())
        return;
    std::set<SkeletonGraphicsNodeItem *> nodeItems;
    readMergedSkeletonNodeSetFromRangeSelection(&nodeItems);
    if (nodeItems.empty())
        return;
    if (nodeItems.size() < 2)
        return;
    emit batchChangeBegin();
    QVector2D center = centerOfNodeItemSet(nodeItems);
    for (const auto &it: nodeItems) {
        SkeletonGraphicsNodeItem *nodeItem = it;
        QPointF nodeOrigin = nodeItem->origin();
        float byX = alignToHorizontalCenter ? sceneRadiusToUnified(center.x() - nodeOrigin.x()) : 0;
        float byY = alignToVerticalCenter ? sceneRadiusToUnified(center.y() - nodeOrigin.y()) : 0;
        if (SkeletonProfile::Main == nodeItem->profile()) {
            if (m_rotated)
                emit moveNodeBy(nodeItem->id(), byY, byX, 0);
            else
                emit moveNodeBy(nodeItem->id(), byX, byY, 0);
        } else {
            if (m_rotated)
                emit moveNodeBy(nodeItem->id(), byY, 0, byX);
            else
                emit moveNodeBy(nodeItem->id(), 0, byY, byX);
        }
    }
    emit batchChangeEnd();
    emit groupOperationAdded();
}

void SkeletonGraphicsWidget::alignSelectedToGlobal(bool alignToVerticalCenter, bool alignToHorizontalCenter)
{
    if (!m_document->originSettled())
        return;
    if (!hasSelection())
        return;
    std::set<SkeletonGraphicsNodeItem *> nodeItems;
    readMergedSkeletonNodeSetFromRangeSelection(&nodeItems);
    if (nodeItems.empty())
        return;
    emit batchChangeBegin();
    for (const auto &nodeItem: nodeItems) {
        const SkeletonNode *node = m_document->findNode(nodeItem->id());
        if (!node) {
            qDebug() << "Find node id failed:" << nodeItem->id();
            continue;
        }
        float byX = 0;
        float byY = 0;
        float byZ = 0;
        if (SkeletonProfile::Main == nodeItem->profile()) {
            if (alignToVerticalCenter && alignToHorizontalCenter) {
                byX = m_document->getOriginX(m_rotated) - node->getX(m_rotated);
                byY = m_document->getOriginY(m_rotated) - node->getY(m_rotated);
            } else if (alignToVerticalCenter) {
                byY = m_document->getOriginY(m_rotated) - node->getY(m_rotated);
            } else if (alignToHorizontalCenter) {
                byX = m_document->getOriginX(m_rotated) - node->getX(m_rotated);
            }
        } else {
            if (alignToVerticalCenter && alignToHorizontalCenter) {
                byY = m_document->getOriginY(m_rotated) - node->getY(m_rotated);
                byZ = m_document->getOriginZ(m_rotated) - node->getZ(m_rotated);
            } else if (alignToVerticalCenter) {
                byY = m_document->getOriginY(m_rotated) - node->getY(m_rotated);
            } else if (alignToHorizontalCenter) {
                byZ = m_document->getOriginZ(m_rotated) - node->getZ(m_rotated);
            }
        }
        if (m_rotated)
            emit moveNodeBy(node->id, byY, byX, byZ);
        else
            emit moveNodeBy(node->id, byX, byY, byZ);
    }
    emit batchChangeEnd();
    emit groupOperationAdded();
}

void SkeletonGraphicsWidget::alignSelectedToGlobalVerticalCenter()
{
    alignSelectedToGlobal(true, false);
}

void SkeletonGraphicsWidget::alignSelectedToGlobalHorizontalCenter()
{
    alignSelectedToGlobal(false, true);
}

void SkeletonGraphicsWidget::alignSelectedToGlobalCenter()
{
    alignSelectedToGlobal(true, true);
}

void SkeletonGraphicsWidget::alignSelectedToLocalVerticalCenter()
{
    alignSelectedToLocal(true, false);
}

void SkeletonGraphicsWidget::alignSelectedToLocalHorizontalCenter()
{
    alignSelectedToLocal(false, true);
}

void SkeletonGraphicsWidget::alignSelectedToLocalCenter()
{
    alignSelectedToLocal(true, true);
}

void SkeletonGraphicsWidget::updateItems()
{
    for (auto nodeItemIt = nodeItemMap.begin(); nodeItemIt != nodeItemMap.end(); nodeItemIt++) {
        nodeRadiusChanged(nodeItemIt->first);
        nodeOriginChanged(nodeItemIt->first);
    }
    originChanged();
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
    const QImage *turnaroundImage = &m_document->turnaround;
    QImage onePixel(16, 10, QImage::Format_ARGB32);
    if (turnaroundImage->isNull()) {
        onePixel.fill(Qt::transparent);
        turnaroundImage = &onePixel;
    }
    
    m_turnaroundChanged = true;
    if (m_turnaroundLoader)
        return;
    
    m_turnaroundChanged = false;

    QThread *thread = new QThread;
    m_turnaroundLoader = new TurnaroundLoader(*turnaroundImage,
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
    delete m_backgroundImage;
    m_backgroundImage = m_turnaroundLoader->takeResultImage();
    if (m_backgroundImage && m_backgroundImage->width() > 0 && m_backgroundImage->height() > 0) {
        //qDebug() << "Fit turnaround finished with image size:" << backgroundImage->size();
        setFixedSize(m_backgroundImage->size());
        scene()->setSceneRect(rect());
        m_backgroundItem->setPixmap(QPixmap::fromImage(*m_backgroundImage));
        updateItems();
    } else {
        qDebug() << "Fit turnaround failed";
    }
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
        case SkeletonDocumentEditMode::Mark: {
                auto pixmap = Theme::awesome()->icon(fa::pencil).pixmap(Theme::toolIconFontSize, Theme::toolIconFontSize);
                QPixmap replacedPixmap(pixmap.size());
                replacedPixmap.fill(m_markerItem->color());
                replacedPixmap.setMask(pixmap.createMaskFromColor(Qt::transparent));
                setCursor(QCursor(replacedPixmap, Theme::toolIconFontSize / 5, Theme::toolIconFontSize * 4 / 5));
            } break;
        case SkeletonDocumentEditMode::Paint:
            setCursor(QCursor(Theme::awesome()->icon(fa::paintbrush).pixmap(Theme::toolIconFontSize, Theme::toolIconFontSize)));
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
            if (nodeItems.size() == 1) {
                choosenNodeItem = *nodeItems.begin();
                if (!m_document->isNodeConnectable(choosenNodeItem->id()))
                    choosenNodeItem = nullptr;
            }
        }
        m_addFromNodeItem = choosenNodeItem;
    }
}

void SkeletonGraphicsWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_modelWidget && m_modelWidget->inputMouseMoveEventFromOtherWidget(event))
        return;
    
    QGraphicsView::mouseMoveEvent(event);
    mouseMove(event);
}

bool SkeletonGraphicsWidget::rotated(void)
{
    return m_rotated;
}

bool SkeletonGraphicsWidget::inputWheelEventFromOtherWidget(QWheelEvent *event)
{
    if (!wheel(event)) {
        if (m_modelWidget && m_modelWidget->inputWheelEventFromOtherWidget(event))
            return true;
    }
    return true;
}

void SkeletonGraphicsWidget::wheelEvent(QWheelEvent *event)
{
    if (!wheel(event)) {
        if (m_modelWidget && m_modelWidget->inputWheelEventFromOtherWidget(event))
            return;
    }
}

void SkeletonGraphicsWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_modelWidget && m_modelWidget->inputMouseReleaseEventFromOtherWidget(event))
        return;
    
    QGraphicsView::mouseReleaseEvent(event);
    mouseRelease(event);
}

void SkeletonGraphicsWidget::mousePressEvent(QMouseEvent *event)
{
    if (m_modelWidget && m_modelWidget->inputMousePressEventFromOtherWidget(event))
        return;
    
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
    if (keyPress(event))
        return;
    QGraphicsView::keyPressEvent(event);
}

void SkeletonGraphicsWidget::keyReleaseEvent(QKeyEvent *event)
{
    if (keyRelease(event))
        return;
    QGraphicsView::keyReleaseEvent(event);
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
    
    if (SkeletonDocumentEditMode::Mark == m_document->editMode) {
        if (m_markerStarted) {
            QPointF mouseScenePos = mouseEventScenePos(event);
            m_markerItem->addPoint(mouseScenePos);
            if (!m_markerItem->isVisible())
                m_markerItem->setVisible(true);
            return true;
        }
    }
    
    if (SkeletonDocumentEditMode::Select == m_document->editMode ||
            SkeletonDocumentEditMode::Add == m_document->editMode) {
        
        //
        // > For overlapping nodes, you can make it a bit better by selecting the node center nearest the mouse, rather than simply checking against the wider circle.
        // > https://www.reddit.com/user/hdu
        //
        
        SkeletonGraphicsNodeItem *newHoverNodeItem = nullptr;
        SkeletonGraphicsEdgeItem *newHoverEdgeItem = nullptr;
        SkeletonGraphicsOriginItem *newHoverOriginItem = nullptr;
        QPointF scenePos = mouseEventScenePos(event);
        QList<QGraphicsItem *> items = scene()->items(scenePos);
        std::vector<std::pair<QGraphicsItem *, float>> itemDistance2MapWithMouse;
        for (auto it = items.begin(); it != items.end(); it++) {
            QGraphicsItem *item = *it;
            if (item->data(0) == "node") {
                SkeletonGraphicsNodeItem *nodeItem = (SkeletonGraphicsNodeItem *)item;
                if (nodeItem->deactivated())
                    continue;
                QPointF origin = nodeItem->origin();
                float distance2 = pow(origin.x() - scenePos.x(), 2) + pow(origin.y() - scenePos.y(), 2);
                itemDistance2MapWithMouse.push_back(std::make_pair(item, distance2));
            } else if (item->data(0) == "edge") {
                SkeletonGraphicsEdgeItem *edgeItem = (SkeletonGraphicsEdgeItem *)item;
                if (edgeItem->deactivated())
                    continue;
                if (edgeItem->firstItem() && edgeItem->secondItem()) {
                    QPointF firstOrigin = edgeItem->firstItem()->origin();
                    QPointF secondOrigin = edgeItem->secondItem()->origin();
                    QPointF origin = (firstOrigin + secondOrigin) / 2;
                    float distance2 = pow(origin.x() - scenePos.x(), 2) + pow(origin.y() - scenePos.y(), 2);
                    itemDistance2MapWithMouse.push_back(std::make_pair(item, distance2));
                }
            } else if (item->data(0) == "origin") {
                newHoverOriginItem = (SkeletonGraphicsOriginItem *)item;
            }
        }
        if (!itemDistance2MapWithMouse.empty()) {
            std::sort(itemDistance2MapWithMouse.begin(), itemDistance2MapWithMouse.end(),
                    [](const std::pair<QGraphicsItem *, float> &a, const std::pair<QGraphicsItem *, float> &b) {
                return a.second < b.second;
            });
            QGraphicsItem *pickedNearestItem = itemDistance2MapWithMouse[0].first;
            if (pickedNearestItem->data(0) == "node") {
                newHoverNodeItem = (SkeletonGraphicsNodeItem *)pickedNearestItem;
            } else if (pickedNearestItem->data(0) == "edge") {
                newHoverEdgeItem = (SkeletonGraphicsEdgeItem *)pickedNearestItem;
            }
        }
        if (newHoverNodeItem) {
            newHoverEdgeItem = nullptr;
        }
        if (newHoverOriginItem != m_hoveredOriginItem) {
            if (nullptr != m_hoveredOriginItem) {
                m_hoveredOriginItem->setHovered(false);
            }
            m_hoveredOriginItem = newHoverOriginItem;
            if (nullptr != m_hoveredOriginItem) {
                m_hoveredOriginItem->setHovered(true);
            }
        }
        QUuid hoveredPartId;
        if (newHoverNodeItem != m_hoveredNodeItem) {
            if (nullptr != m_hoveredNodeItem) {
                setItemHoveredOnAllProfiles(m_hoveredNodeItem, false);
            }
            m_hoveredNodeItem = newHoverNodeItem;
            if (nullptr != m_hoveredNodeItem) {
                hoveredPartId = querySkeletonItemPartId(m_hoveredNodeItem);
                setItemHoveredOnAllProfiles(m_hoveredNodeItem, true);
            }
        }
        if (newHoverEdgeItem != m_hoveredEdgeItem) {
            if (nullptr != m_hoveredEdgeItem) {
                setItemHoveredOnAllProfiles(m_hoveredEdgeItem, false);
            }
            m_hoveredEdgeItem = newHoverEdgeItem;
            if (nullptr != m_hoveredEdgeItem) {
                hoveredPartId = querySkeletonItemPartId(m_hoveredEdgeItem);
                setItemHoveredOnAllProfiles(m_hoveredEdgeItem, true);
            }
        }
        if (m_hoveredNodeItem) {
            hoveredPartId = querySkeletonItemPartId(m_hoveredNodeItem);
        } else if (m_hoveredEdgeItem) {
            hoveredPartId = querySkeletonItemPartId(m_hoveredEdgeItem);
        }
        hoverPart(hoveredPartId);
    }
    
    if (SkeletonDocumentEditMode::Add == m_document->editMode) {
        QPointF mouseScenePos = mouseEventScenePos(event);
        m_cursorNodeItem->setOrigin(mouseScenePos);
        if (m_addFromNodeItem) {
            m_cursorEdgeItem->setEndpoints(m_addFromNodeItem, m_cursorNodeItem);
            if (!m_cursorNodeItem->isVisible())
                m_cursorNodeItem->setRadius(m_addFromNodeItem->radius());
            if (!m_cursorEdgeItem->isVisible()) {
                m_cursorEdgeItem->show();
            }
        }
        if (!m_cursorNodeItem->isVisible()) {
            m_cursorNodeItem->show();
        }
        return true;
    }
    
    if (SkeletonDocumentEditMode::Select == m_document->editMode) {
        if (m_moveStarted) {
            if (m_checkedOriginItem) {
                QPointF mouseScenePos = mouseEventScenePos(event);
                float byX = mouseScenePos.x() - m_lastScenePos.x();
                float byY = mouseScenePos.y() - m_lastScenePos.y();
                moveCheckedOrigin(byX, byY);
                m_lastScenePos = mouseScenePos;
                m_moveHappened = true;
                return true;
            } else if (!m_rangeSelectionSet.empty()) {
                QPointF mouseScenePos = mouseEventScenePos(event);
                float byX = mouseScenePos.x() - m_lastScenePos.x();
                float byY = mouseScenePos.y() - m_lastScenePos.y();
                if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier)) {
                    std::set<SkeletonGraphicsNodeItem *> nodeItems;
                    readMergedSkeletonNodeSetFromRangeSelection(&nodeItems);
                    bool ikMoved = false;
                    if (nodeItems.size() == 1) {
                        auto &nodeItem = *nodeItems.begin();
                        const SkeletonNode *node = m_document->findNode(nodeItem->id());
                        if (node->edgeIds.size() == 1) {
                            const auto origin = nodeItem->origin();
                            byX = mouseScenePos.x() - origin.x();
                            byY = mouseScenePos.y() - origin.y();
                            byX = sceneRadiusToUnified(byX);
                            byY = sceneRadiusToUnified(byY);
                            QVector3D target = QVector3D(node->getX(m_rotated), node->getY(m_rotated), node->getZ(m_rotated));
                            if (SkeletonProfile::Main == nodeItem->profile()) {
                                target.setX(target.x() + byX);
                                target.setY(target.y() + byY);
                            } else {
                                target.setY(target.y() + byY);
                                target.setZ(target.z() + byX);
                            }
                            emit ikMove(nodeItem->id(), target);
                            ikMoved = true;
                        }
                    }
                    if (!ikMoved)
                        rotateSelected(byX);
                } else {
                    moveSelected(byX, byY);
                }
                m_lastScenePos = mouseScenePos;
                m_moveHappened = true;
                return true;
            }
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
    
    QVector2D center;

    if (1 == nodeItems.size() && m_document->originSettled()) {
        // Rotate all children nodes around this node
        // 1. Pick who is the parent from neighbors
        // 2. Rotate all the remaining neighbor nodes and their children exlucde the picked parent
        QVector3D worldOrigin = QVector3D(m_document->getOriginX(m_rotated), m_document->getOriginY(m_rotated), m_document->getOriginZ(m_rotated));
        SkeletonGraphicsNodeItem *centerNodeItem = *nodeItems.begin();
        const auto &centerNode = m_document->findNode(centerNodeItem->id());
        if (nullptr == centerNode) {
            qDebug() << "findNode:" << centerNodeItem->id() << " failed while rotate";
            return;
        }
        std::vector<std::pair<QUuid, float>> directNeighborDists;
        for (const auto &neighborId: centerNode->edgeIds) {
            const auto &neighborEdge = m_document->findEdge(neighborId);
            if (nullptr == neighborEdge) {
                qDebug() << "findEdge:" << neighborId << " failed while rotate";
                continue;
            }
            const auto &neighborNodeId = neighborEdge->neighborOf(centerNodeItem->id());
            const auto &neighborNode = m_document->findNode(neighborNodeId);
            if (nullptr == centerNode) {
                qDebug() << "findNode:" << neighborNodeId << " failed while rotate";
                continue;
            }
            QVector3D nodeOrigin(neighborNode->getX(m_rotated), neighborNode->getY(m_rotated), neighborNode->getZ(m_rotated));
            directNeighborDists.push_back(std::make_pair(neighborNodeId, (nodeOrigin - worldOrigin).lengthSquared()));
        }
        std::sort(directNeighborDists.begin(), directNeighborDists.end(), [](const std::pair<QUuid, float> &a, const std::pair<QUuid, float> &b) {
            return a.second < b.second;
        });
        int skippedNum = 1;
        if (directNeighborDists.size() == 1) {
            skippedNum = 0;
        }
        std::set<QUuid> neighborIds;
        // Adding self to force the neighbor finding not reverse
        neighborIds.insert(centerNodeItem->id());
        for (int i = skippedNum; i < (int)directNeighborDists.size(); i++) {
            neighborIds.insert(directNeighborDists[i].first);
            m_document->findAllNeighbors(directNeighborDists[i].first, neighborIds);
        }
        center = QVector2D(centerNodeItem->origin().x(), centerNodeItem->origin().y());
        nodeItems.clear();
        for (const auto &neighborId: neighborIds) {
            const auto &findItemResult = nodeItemMap.find(neighborId);
            if (findItemResult == nodeItemMap.end()) {
                qDebug() << "find node item:" << neighborId << "failed";
                continue;
            }
            if (SkeletonProfile::Main == centerNodeItem->profile()) {
                nodeItems.insert(findItemResult->second.first);
            } else {
                nodeItems.insert(findItemResult->second.second);
            }
        }
    } else {
        center = centerOfNodeItemSet(nodeItems);
    }

    rotateItems(nodeItems, degree, center);
}

void SkeletonGraphicsWidget::rotateItems(const std::set<SkeletonGraphicsNodeItem *> &nodeItems, int degree, QVector2D center)
{
    emit disableAllPositionRelatedLocks();
    QVector3D centerPoint(center.x(), center.y(), 0);
    qNormalizeAngle(degree);
    for (const auto &it: nodeItems) {
        SkeletonGraphicsNodeItem *nodeItem = it;
        QMatrix4x4 mat;
        QPointF nodeOrigin = nodeItem->origin();
        QVector3D nodeOriginPoint(nodeOrigin.x(), nodeOrigin.y(), 0);
        QVector3D p = nodeOriginPoint - centerPoint;
        mat.rotate(degree, 0, 0, 1);
        p = mat * p;
        QVector3D finalPoint = p + centerPoint;
        float byX = sceneRadiusToUnified(finalPoint.x() - nodeOrigin.x());
        float byY = sceneRadiusToUnified(finalPoint.y() - nodeOrigin.y());
        if (SkeletonProfile::Main == nodeItem->profile()) {
            if (m_rotated)
                emit moveNodeBy(nodeItem->id(), byY, byX, 0);
            else
                emit moveNodeBy(nodeItem->id(), byX, byY, 0);
        } else {
            if (m_rotated)
                emit moveNodeBy(nodeItem->id(), byY, 0, byX);
            else
                emit moveNodeBy(nodeItem->id(), 0, byY, byX);
        }
    }
    emit enableAllPositionRelatedLocks();
}

void SkeletonGraphicsWidget::rotateAllSideProfile(int degree)
{
    std::set<SkeletonGraphicsNodeItem *> items;
    for (const auto &item: nodeItemMap) {
        items.insert(item.second.second);
    }
    QVector2D center(scenePosFromUnified(QPointF(m_document->getOriginZ(m_rotated), m_document->getOriginY(m_rotated))));
    rotateItems(items, degree, center);
}

void SkeletonGraphicsWidget::moveCheckedOrigin(float byX, float byY)
{
    if (!m_checkedOriginItem)
        return;
    byX = sceneRadiusToUnified(byX);
    byY = sceneRadiusToUnified(byY);
    if (SkeletonProfile::Main == m_checkedOriginItem->profile()) {
        if (m_rotated)
            emit moveOriginBy(byY, byX, 0);
        else
            emit moveOriginBy(byX, byY, 0);
    } else {
        if (m_rotated)
            emit moveOriginBy(byY, 0, byX);
        else
            emit moveOriginBy(0, byY, byX);
    }
}

void SkeletonGraphicsWidget::moveSelected(float byX, float byY)
{
    if (m_rangeSelectionSet.empty())
        return;
    
    m_ikMoveEndEffectorId = QUuid();
    
    byX = sceneRadiusToUnified(byX);
    byY = sceneRadiusToUnified(byY);
    std::set<SkeletonGraphicsNodeItem *> nodeItems;
    readMergedSkeletonNodeSetFromRangeSelection(&nodeItems);
    
    for (const auto &it: nodeItems) {
        SkeletonGraphicsNodeItem *nodeItem = it;
        if (SkeletonProfile::Main == nodeItem->profile()) {
            if (m_rotated)
                emit moveNodeBy(nodeItem->id(), byY, byX, 0);
            else
                emit moveNodeBy(nodeItem->id(), byX, byY, 0);
        } else {
            if (m_rotated)
                emit moveNodeBy(nodeItem->id(), byY, 0, byX);
            else
                emit moveNodeBy(nodeItem->id(), 0, byY, byX);
        }
    }
}

void SkeletonGraphicsWidget::switchSelectedXZ()
{
    if (m_rangeSelectionSet.empty())
        return;
    
    std::set<QUuid> nodeIdSet;
    readSkeletonNodeAndEdgeIdSetFromRangeSelection(&nodeIdSet);
    if (nodeIdSet.empty())
        return;
    
    emit batchChangeBegin();
    for (const auto nodeId: nodeIdSet) {
        emit switchNodeXZ(nodeId);
    }
    emit batchChangeEnd();
    emit groupOperationAdded();
}

void SkeletonGraphicsWidget::switchSelectedChainSide()
{
    if (m_rangeSelectionSet.empty())
        return;
    
    std::set<QUuid> nodeIdSet;
    readSkeletonNodeAndEdgeIdSetFromRangeSelection(&nodeIdSet);
    if (nodeIdSet.empty())
        return;
    
    emit switchChainSide(nodeIdSet);
    emit groupOperationAdded();
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

void SkeletonGraphicsWidget::getOtherProfileNodeItems(const std::set<SkeletonGraphicsNodeItem *> &nodeItemSet,
        std::set<SkeletonGraphicsNodeItem *> *otherProfileNodeItemSet)
{
    for (const auto &nodeItem: nodeItemSet) {
        auto findNodeItem = nodeItemMap.find(nodeItem->id());
        if (findNodeItem == nodeItemMap.end())
            continue;
        if (nodeItem == findNodeItem->second.first)
            otherProfileNodeItemSet->insert(findNodeItem->second.second);
        else
            otherProfileNodeItemSet->insert(findNodeItem->second.first);
    }
}

void SkeletonGraphicsWidget::scaleSelected(float delta)
{
    if (m_rangeSelectionSet.empty())
        return;
    
    std::set<SkeletonGraphicsNodeItem *> nodeItems;
    readMergedSkeletonNodeSetFromRangeSelection(&nodeItems);
    
    std::set<SkeletonGraphicsNodeItem *> otherProfileNodeItems;
    getOtherProfileNodeItems(nodeItems, &otherProfileNodeItems);
    
    QVector2D center = centerOfNodeItemSet(nodeItems);
    QVector2D otherCenter = centerOfNodeItemSet(otherProfileNodeItems);
    std::map<QUuid, std::tuple<float, float, float>> moveByMap;
    float scale = sceneRadiusToUnified(delta);
    for (const auto &nodeItem: nodeItems) {
        QVector2D origin = QVector2D(nodeItem->origin());
        QVector2D ray = (center - origin) * scale;
        float byX = -sceneRadiusToUnified(ray.x());
        float byY = -sceneRadiusToUnified(ray.y());
        if (SkeletonProfile::Main == nodeItem->profile()) {
            moveByMap[nodeItem->id()] = std::make_tuple(byX, byY, 0.0);
        } else {
            moveByMap[nodeItem->id()] = std::make_tuple(0.0, byY, byX);
        }
    }
    for (const auto &nodeItem: otherProfileNodeItems) {
        QVector2D origin = QVector2D(nodeItem->origin());
        QVector2D ray = (otherCenter - origin) * scale;
        float byX = -sceneRadiusToUnified(ray.x());
        if (SkeletonProfile::Main == nodeItem->profile()) {
            std::get<0>(moveByMap[nodeItem->id()]) = byX;
        } else {
            std::get<2>(moveByMap[nodeItem->id()]) = byX;
        }
    }
    for (const auto &it: moveByMap) {
        if (m_rotated)
            emit moveNodeBy(it.first, std::get<1>(it.second), std::get<0>(it.second), std::get<2>(it.second));
        else
            emit moveNodeBy(it.first, std::get<0>(it.second), std::get<1>(it.second), std::get<2>(it.second));
    }
}

bool SkeletonGraphicsWidget::wheel(QWheelEvent *event)
{
    float delta = 0;
    if (event->delta() > 0)
        delta = 1;
    else
        delta = -1;
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
    emit batchChangeBegin();
    for (const auto &nodeItem: nodeItems) {
        QPointF origin = nodeItem->origin();
        float offset = origin.x() - center.x();
        float unifiedOffset = -sceneRadiusToUnified(offset * 2);
        if (SkeletonProfile::Main == nodeItem->profile()) {
            if (m_rotated)
                emit moveNodeBy(nodeItem->id(), 0, unifiedOffset, 0);
            else
                emit moveNodeBy(nodeItem->id(), unifiedOffset, 0, 0);
        } else {
            emit moveNodeBy(nodeItem->id(), 0, 0, unifiedOffset);
        }
    }
    emit batchChangeEnd();
    emit groupOperationAdded();
}

void SkeletonGraphicsWidget::flipVertically()
{
    std::set<SkeletonGraphicsNodeItem *> nodeItems;
    readMergedSkeletonNodeSetFromRangeSelection(&nodeItems);
    if (nodeItems.empty())
        return;
    QVector2D center = centerOfNodeItemSet(nodeItems);
    emit batchChangeBegin();
    for (const auto &nodeItem: nodeItems) {
        QPointF origin = nodeItem->origin();
        float offset = origin.y() - center.y();
        float unifiedOffset = -sceneRadiusToUnified(offset * 2);
        if (m_rotated)
            emit moveNodeBy(nodeItem->id(), unifiedOffset, 0, 0);
        else
            emit moveNodeBy(nodeItem->id(), 0, unifiedOffset, 0);
    }
    emit batchChangeEnd();
    emit groupOperationAdded();
}

void SkeletonGraphicsWidget::rotateClockwise90Degree()
{
    emit batchChangeBegin();
    emit rotateSelected(90);
    emit batchChangeEnd();
    emit groupOperationAdded();
}

void SkeletonGraphicsWidget::rotateCounterclockwise90Degree()
{
    emit batchChangeBegin();
    emit rotateSelected(360 - 90);
    emit batchChangeEnd();
    emit groupOperationAdded();
}

void SkeletonGraphicsWidget::rotateAllMainProfileClockwise90DegreeAlongOrigin()
{
    if (!m_document->originSettled())
        return;
    emit batchChangeBegin();
    emit rotateAllSideProfile(90);
    emit batchChangeEnd();
    emit groupOperationAdded();
}

void SkeletonGraphicsWidget::rotateAllMainProfileCounterclockwise90DegreeAlongOrigin()
{
    if (!m_document->originSettled())
        return;
    emit batchChangeBegin();
    emit rotateAllSideProfile(360 - 90);
    emit batchChangeEnd();
    emit groupOperationAdded();
}

bool SkeletonGraphicsWidget::mouseRelease(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        bool processed = m_dragStarted || m_moveStarted || m_rangeSelectionStarted || m_markerStarted;
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
        if (m_markerStarted) {
            auto boundingBox = m_markerItem->polygon().boundingRect();
            if (boundingBox.width() * boundingBox.height() > 4) {
                const QPolygonF &previousPolygon = m_markerItem->previousPolygon();
                if (previousPolygon.empty()) {
                    m_markerItem->save();
                } else {
                    if (m_markerItem->isMainProfile()) {
                        emit addPartByPolygons(m_markerItem->polygon(), previousPolygon, sceneRect().size());
                    } else {
                        emit addPartByPolygons(previousPolygon, m_markerItem->polygon(), sceneRect().size());
                    }
                    m_markerItem->reset();
                }
                m_markerItem->hide();
                m_markerItem->toggleProfile();
                updateCursor();
            } else {
                m_markerItem->clear();
            }
            m_markerStarted = false;
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
                            m_hoveredNodeItem->profile() == m_addFromNodeItem->profile() &&
                            !m_document->findEdgeByNodes(m_addFromNodeItem->id(), m_hoveredNodeItem->id()) &&
                            m_document->isNodeEditable(m_hoveredNodeItem->id())) {
                        if (m_document->isNodeConnectable(m_hoveredNodeItem->id())) {
                            emit addEdge(m_addFromNodeItem->id(), m_hoveredNodeItem->id());
                            emit groupOperationAdded();
                            return true;
                        }
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
                if (m_rotated)
                    emit addNode(unifiedMainPos.y(), unifiedMainPos.x(), unifiedSidePos.x(), sceneRadiusToUnified(m_cursorNodeItem->radius()), nullptr == m_addFromNodeItem ? QUuid() : m_addFromNodeItem->id());
                else
                    emit addNode(unifiedMainPos.x(), unifiedMainPos.y(), unifiedSidePos.x(), sceneRadiusToUnified(m_cursorNodeItem->radius()), nullptr == m_addFromNodeItem ? QUuid() : m_addFromNodeItem->id());
                emit groupOperationAdded();
                return true;
            }
        } else if (SkeletonDocumentEditMode::Select == m_document->editMode) {
            bool processed = false;
            if (m_hoveredOriginItem != m_checkedOriginItem) {
                if (m_checkedOriginItem) {
                    m_checkedOriginItem->setChecked(false);
                    m_checkedOriginItem->setHovered(false);
                }
                m_checkedOriginItem = m_hoveredOriginItem;
                if (m_checkedOriginItem) {
                    m_checkedOriginItem->setChecked(true);
                }
            }
            if (m_checkedOriginItem) {
                if (!m_moveStarted) {
                    m_moveStarted = true;
                    m_lastScenePos = mouseEventScenePos(event);
                    m_moveHappened = false;
                    processed = true;
                }
            } else {
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
            }
            if (processed) {
                return true;
            }
        }
    }
    
    if (event->button() == Qt::LeftButton) {
        if (SkeletonDocumentEditMode::Select == m_document->editMode) {
            if (!m_rangeSelectionStarted) {
                m_rangeSelectionStartPos = mouseEventScenePos(event);
                m_rangeSelectionStarted = true;
            }
        } else if (SkeletonDocumentEditMode::Mark == m_document->editMode) {
            if (!m_markerStarted) {
                m_markerItem->addPoint(mouseEventScenePos(event));
                m_markerStarted = true;
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
        if (m_nodePositionModifyOnly)
            selectConnectedAll();
        else
            selectPartAll();
        return true;
    }
    if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier)) {
        emit open();
        return true;
    }
    return false;
}

void SkeletonGraphicsWidget::timeToRemoveDeferredNodesAndEdges()
{
    delete m_deferredRemoveTimer;
    m_deferredRemoveTimer = nullptr;
    
    bool committed = false;
    
    if (!committed && !m_deferredRemoveEdgeIds.empty()) {
        const auto &it = m_deferredRemoveEdgeIds.begin();
        const auto edgeId = *it;
        m_deferredRemoveEdgeIds.erase(it);
        emit removeEdge(edgeId);
        committed = true;
    }
    
    if (!committed && !m_deferredRemoveNodeIds.empty()) {
        const auto &it = m_deferredRemoveNodeIds.begin();
        const auto nodeId = *it;
        m_deferredRemoveNodeIds.erase(it);
        emit removeNode(nodeId);
        committed = true;
    }
    
    if (committed) {
        m_deferredRemoveTimer = new QTimer(this);
        connect(m_deferredRemoveTimer, &QTimer::timeout, this, &SkeletonGraphicsWidget::timeToRemoveDeferredNodesAndEdges);
        m_deferredRemoveTimer->start(0);
    } else {
        emit groupOperationAdded();
    }
}

void SkeletonGraphicsWidget::deleteSelected()
{
    if (m_rangeSelectionSet.empty())
        return;
    
    std::set<QUuid> nodeIdSet;
    std::set<QUuid> edgeIdSet;
    readSkeletonNodeAndEdgeIdSetFromRangeSelection(&nodeIdSet, &edgeIdSet);
    for (const auto &it: nodeIdSet) {
        m_deferredRemoveNodeIds.insert(it);
    }
    for (const auto &it: edgeIdSet) {
        m_deferredRemoveEdgeIds.insert(it);
    }
    
    if (nullptr == m_deferredRemoveTimer) {
        timeToRemoveDeferredNodesAndEdges();
    }
}

void SkeletonGraphicsWidget::shortcutDelete()
{
    bool processed = false;
    if (!m_rangeSelectionSet.empty()) {
        deleteSelected();
        processed = true;
    }
    if (processed) {
        emit groupOperationAdded();
        return;
    }
}

void SkeletonGraphicsWidget::shortcutAddMode()
{
    if (SkeletonDocumentEditMode::Add == m_document->editMode) {
        emit setEditMode(SkeletonDocumentEditMode::Select);
    } else {
        emit setEditMode(SkeletonDocumentEditMode::Add);
    }
}

void SkeletonGraphicsWidget::shortcutMarkMode()
{
    emit setEditMode(SkeletonDocumentEditMode::Mark);
}

void SkeletonGraphicsWidget::shortcutUndo()
{
    emit undo();
}

void SkeletonGraphicsWidget::shortcutRedo()
{
    emit redo();
}

void SkeletonGraphicsWidget::shortcutXlock()
{
    emit setXlockState(!m_document->xlocked);
}

void SkeletonGraphicsWidget::shortcutYlock()
{
    emit setYlockState(!m_document->ylocked);
}

void SkeletonGraphicsWidget::shortcutZlock()
{
    emit setZlockState(!m_document->zlocked);
}

void SkeletonGraphicsWidget::shortcutCut()
{
    cut();
}

void SkeletonGraphicsWidget::shortcutCopy()
{
    copy();
}

void SkeletonGraphicsWidget::shortcutPaste()
{
    emit paste();
}

void SkeletonGraphicsWidget::shortcutSelectMode()
{
    emit setEditMode(SkeletonDocumentEditMode::Select);
}

void SkeletonGraphicsWidget::shortcutPaintMode()
{
    emit setEditMode(SkeletonDocumentEditMode::Paint);
}

void SkeletonGraphicsWidget::shortcutZoomRenderedModelByMinus10()
{
    emit zoomRenderedModelBy(-10);
}

void SkeletonGraphicsWidget::shortcutZoomSelectedByMinus1()
{
    if (SkeletonDocumentEditMode::Select == m_document->editMode && hasSelection()) {
        zoomSelected(-1);
        emit groupOperationAdded();
    } else if (SkeletonDocumentEditMode::Add == m_document->editMode) {
        if (m_cursorNodeItem->isVisible()) {
            m_cursorNodeItem->setRadius(m_cursorNodeItem->radius() + -1);
        }
    }
}

void SkeletonGraphicsWidget::shortcutZoomRenderedModelBy10()
{
    emit zoomRenderedModelBy(10);
}

void SkeletonGraphicsWidget::shortcutZoomSelectedBy1()
{
    if (SkeletonDocumentEditMode::Select == m_document->editMode && hasSelection()) {
        zoomSelected(1);
        emit groupOperationAdded();
    } else if (SkeletonDocumentEditMode::Add == m_document->editMode) {
        if (m_cursorNodeItem->isVisible()) {
            m_cursorNodeItem->setRadius(m_cursorNodeItem->radius() + 1);
        }
    }
}

void SkeletonGraphicsWidget::shortcutRotateSelectedByMinus1()
{
    if ((SkeletonDocumentEditMode::Select == m_document->editMode || SkeletonDocumentEditMode::Mark == m_document->editMode) &&
            hasSelection()) {
        rotateSelected(-1);
        emit groupOperationAdded();
    }
}

void SkeletonGraphicsWidget::shortcutRotateSelectedBy1()
{
    if ((SkeletonDocumentEditMode::Select == m_document->editMode || SkeletonDocumentEditMode::Mark == m_document->editMode) &&
            hasSelection()) {
        rotateSelected(1);
        emit groupOperationAdded();
    }
}

void SkeletonGraphicsWidget::shortcutMoveSelectedToLeft()
{
    if (SkeletonDocumentEditMode::Select == m_document->editMode || SkeletonDocumentEditMode::Mark == m_document->editMode) {
        if (m_checkedOriginItem) {
            moveCheckedOrigin(-1, 0);
            emit groupOperationAdded();
        } else if (hasSelection()) {
            moveSelected(-1, 0);
            emit groupOperationAdded();
        }
    }
}

void SkeletonGraphicsWidget::shortcutMoveSelectedToRight()
{
    if (SkeletonDocumentEditMode::Select == m_document->editMode || SkeletonDocumentEditMode::Mark == m_document->editMode) {
        if (m_checkedOriginItem) {
            moveCheckedOrigin(1, 0);
            emit groupOperationAdded();
        } else if (hasSelection()) {
            moveSelected(1, 0);
            emit groupOperationAdded();
        }
    }
}

void SkeletonGraphicsWidget::shortcutMoveSelectedToUp()
{
    if (SkeletonDocumentEditMode::Select == m_document->editMode || SkeletonDocumentEditMode::Mark == m_document->editMode) {
        if (m_checkedOriginItem) {
            moveCheckedOrigin(0, -1);
            emit groupOperationAdded();
        } else if (hasSelection()) {
            moveSelected(0, -1);
            emit groupOperationAdded();
        }
    }
}

void SkeletonGraphicsWidget::shortcutMoveSelectedToDown()
{
    if (SkeletonDocumentEditMode::Select == m_document->editMode || SkeletonDocumentEditMode::Mark == m_document->editMode) {
        if (m_checkedOriginItem) {
            moveCheckedOrigin(0, 1);
            emit groupOperationAdded();
        } else if (hasSelection()) {
            moveSelected(0, 1);
            emit groupOperationAdded();
        }
    }
}

void SkeletonGraphicsWidget::shortcutScaleSelectedByMinus1()
{
    if ((SkeletonDocumentEditMode::Select == m_document->editMode || SkeletonDocumentEditMode::Mark == m_document->editMode) &&
            hasSelection()) {
        scaleSelected(-1);
        emit groupOperationAdded();
    }
}

void SkeletonGraphicsWidget::shortcutScaleSelectedBy1()
{
    if ((SkeletonDocumentEditMode::Select == m_document->editMode || SkeletonDocumentEditMode::Mark == m_document->editMode) &&
            hasSelection()) {
        scaleSelected(1);
        emit groupOperationAdded();
    }
}

void SkeletonGraphicsWidget::shortcutSwitchProfileOnSelected()
{
    if ((SkeletonDocumentEditMode::Select == m_document->editMode || SkeletonDocumentEditMode::Mark == m_document->editMode) &&
            hasSelection()) {
        switchProfileOnRangeSelection();
    }
}

void SkeletonGraphicsWidget::shortcutShowOrHideSelectedPart()
{
    if (SkeletonDocumentEditMode::Select == m_document->editMode) {
        if (!m_lastCheckedPart.isNull()) {
            const SkeletonPart *part = m_document->findPart(m_lastCheckedPart);
            bool partVisible = part && part->visible;
            emit setPartVisibleState(m_lastCheckedPart, !partVisible);
            emit groupOperationAdded();
        } else {
            emit showOrHideAllComponents();
        }
    }
}

void SkeletonGraphicsWidget::shortcutEnableOrDisableSelectedPart()
{
    if ((SkeletonDocumentEditMode::Select == m_document->editMode || SkeletonDocumentEditMode::Mark == m_document->editMode) &&
            !m_lastCheckedPart.isNull()) {
        const SkeletonPart *part = m_document->findPart(m_lastCheckedPart);
        bool partDisabled = part && part->disabled;
        emit setPartDisableState(m_lastCheckedPart, !partDisabled);
        emit groupOperationAdded();
    }
}

void SkeletonGraphicsWidget::shortcutLockOrUnlockSelectedPart()
{
    if ((SkeletonDocumentEditMode::Select == m_document->editMode || SkeletonDocumentEditMode::Mark == m_document->editMode) &&
            !m_lastCheckedPart.isNull()) {
        const SkeletonPart *part = m_document->findPart(m_lastCheckedPart);
        bool partLocked = part && part->locked;
        emit setPartLockState(m_lastCheckedPart, !partLocked);
        emit groupOperationAdded();
    }
}

void SkeletonGraphicsWidget::shortcutXmirrorOnOrOffSelectedPart()
{
    if ((SkeletonDocumentEditMode::Select == m_document->editMode || SkeletonDocumentEditMode::Mark == m_document->editMode) &&
            !m_lastCheckedPart.isNull()) {
        const SkeletonPart *part = m_document->findPart(m_lastCheckedPart);
        bool partXmirrored = part && part->xMirrored;
        emit setPartXmirrorState(m_lastCheckedPart, !partXmirrored);
        emit groupOperationAdded();
    }
}

void SkeletonGraphicsWidget::shortcutSubdivedOrNotSelectedPart()
{
    if ((SkeletonDocumentEditMode::Select == m_document->editMode || SkeletonDocumentEditMode::Mark == m_document->editMode) &&
            !m_lastCheckedPart.isNull()) {
        const SkeletonPart *part = m_document->findPart(m_lastCheckedPart);
        bool partSubdived = part && part->subdived;
        emit setPartSubdivState(m_lastCheckedPart, !partSubdived);
        emit groupOperationAdded();
    }
}

void SkeletonGraphicsWidget::shortcutChamferedOrNotSelectedPart()
{
    if ((SkeletonDocumentEditMode::Select == m_document->editMode || SkeletonDocumentEditMode::Mark == m_document->editMode) &&
            !m_lastCheckedPart.isNull()) {
        const SkeletonPart *part = m_document->findPart(m_lastCheckedPart);
        bool partChamfered = part && part->chamfered;
        emit setPartChamferState(m_lastCheckedPart, !partChamfered);
        emit groupOperationAdded();
    }
}

void SkeletonGraphicsWidget::shortcutSelectAll()
{
    selectAll();
}

void SkeletonGraphicsWidget::shortcutRoundEndOrNotSelectedPart()
{
    if ((SkeletonDocumentEditMode::Select == m_document->editMode || SkeletonDocumentEditMode::Mark == m_document->editMode) &&
            !m_lastCheckedPart.isNull()) {
        const SkeletonPart *part = m_document->findPart(m_lastCheckedPart);
        bool partRounded = part && part->rounded;
        emit setPartRoundState(m_lastCheckedPart, !partRounded);
        emit groupOperationAdded();
    }
}

bool SkeletonGraphicsWidget::keyPress(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space) {
        if (SkeletonDocumentEditMode::ZoomIn == m_document->editMode ||
                SkeletonDocumentEditMode::ZoomOut == m_document->editMode ||
                SkeletonDocumentEditMode::Select == m_document->editMode ||
                SkeletonDocumentEditMode::Add == m_document->editMode) {
            m_inTempDragMode = true;
            m_modeBeforeEnterTempDragMode = m_document->editMode;
            emit setEditMode(SkeletonDocumentEditMode::Drag);
            return true;
        }
    }
    
    return false;
}

bool SkeletonGraphicsWidget::keyRelease(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space) {
        if (m_inTempDragMode) {
            m_inTempDragMode = false;
            emit setEditMode(m_modeBeforeEnterTempDragMode);
            return true;
        }
    }
    
    return false;
}

void SkeletonGraphicsWidget::originChanged()
{
    if (!m_document->originSettled()) {
        m_mainOriginItem->hide();
        m_sideOriginItem->hide();
        return;
    }
    m_mainOriginItem->setOrigin(scenePosFromUnified(QPointF(m_document->getOriginX(m_rotated), m_document->getOriginY(m_rotated))));
    m_sideOriginItem->setOrigin(scenePosFromUnified(QPointF(m_document->getOriginZ(m_rotated), m_document->getOriginY(m_rotated))));
    m_mainOriginItem->show();
    m_sideOriginItem->show();
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
    QColor markColor = BoneMarkToColor(node->boneMark);
    SkeletonGraphicsNodeItem *mainProfileItem = new SkeletonGraphicsNodeItem(SkeletonProfile::Main);
    mainProfileItem->setRotated(m_rotated);
    SkeletonGraphicsNodeItem *sideProfileItem = new SkeletonGraphicsNodeItem(SkeletonProfile::Side);
    sideProfileItem->setRotated(m_rotated);
    mainProfileItem->setOrigin(scenePosFromUnified(QPointF(node->getX(m_rotated), node->getY(m_rotated))));
    sideProfileItem->setOrigin(scenePosFromUnified(QPointF(node->getZ(m_rotated), node->getY(m_rotated))));
    mainProfileItem->setRadius(sceneRadiusFromUnified(node->radius));
    sideProfileItem->setRadius(sceneRadiusFromUnified(node->radius));
    mainProfileItem->setMarkColor(markColor);
    sideProfileItem->setMarkColor(markColor);
    mainProfileItem->setId(nodeId);
    sideProfileItem->setId(nodeId);
    if (m_document->isNodeDeactivated(nodeId)) {
        mainProfileItem->setDeactivated(true);
        sideProfileItem->setDeactivated(true);
    }
    if (m_mainProfileOnly)
        sideProfileItem->hide();
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
    mainProfileEdgeItem->setRotated(m_rotated);
    SkeletonGraphicsEdgeItem *sideProfileEdgeItem = new SkeletonGraphicsEdgeItem();
    sideProfileEdgeItem->setRotated(m_rotated);
    mainProfileEdgeItem->setId(edgeId);
    sideProfileEdgeItem->setId(edgeId);
    mainProfileEdgeItem->setEndpoints(fromIt->second.first, toIt->second.first);
    sideProfileEdgeItem->setEndpoints(fromIt->second.second, toIt->second.second);
    if (m_document->isNodeDeactivated(edgeId)) {
        mainProfileEdgeItem->setDeactivated(true);
        sideProfileEdgeItem->setDeactivated(true);
    }
    if (m_mainProfileOnly)
        sideProfileEdgeItem->hide();
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

void SkeletonGraphicsWidget::nodeBoneMarkChanged(QUuid nodeId)
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
    QColor markColor = BoneMarkToColor(node->boneMark);
    it->second.first->setMarkColor(markColor);
    it->second.second->setMarkColor(markColor);
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
    QPointF mainPos = scenePosFromUnified(QPointF(node->getX(m_rotated), node->getY(m_rotated)));
    QPointF sidePos = scenePosFromUnified(QPointF(node->getZ(m_rotated), node->getY(m_rotated)));
    it->second.first->setOrigin(mainPos);
    it->second.first->setRotated(m_rotated);
    it->second.first->updateAppearance();
    it->second.second->setOrigin(sidePos);
    it->second.second->setRotated(m_rotated);
    it->second.second->updateAppearance();
    for (auto edgeIt = node->edgeIds.begin(); edgeIt != node->edgeIds.end(); edgeIt++) {
        auto edgeItemIt = edgeItemMap.find(*edgeIt);
        if (edgeItemIt == edgeItemMap.end()) {
            qDebug() << "Edge item not found:" << *edgeIt;
            continue;
        }
        edgeItemIt->second.first->setRotated(m_rotated);
        edgeItemIt->second.first->updateAppearance();
        edgeItemIt->second.second->setRotated(m_rotated);
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
            edgeItemIt->second.first->setVisible(part->isEditVisible());
            edgeItemIt->second.second->setVisible(part->isEditVisible());
        }
        auto nodeItemIt = nodeItemMap.find(nodeId);
        if (nodeItemIt == nodeItemMap.end()) {
            qDebug() << "Node item not found:" << nodeId;
            continue;
        }
        nodeItemIt->second.first->setVisible(part->isEditVisible());
        nodeItemIt->second.second->setVisible(part->isEditVisible());
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

QUuid SkeletonGraphicsWidget::querySkeletonItemPartId(QGraphicsItem *item)
{
    if (item->data(0) == "node") {
        SkeletonGraphicsNodeItem *nodeItem = (SkeletonGraphicsNodeItem *)item;
        const SkeletonNode *node = m_document->findNode(nodeItem->id());
        if (!node) {
            qDebug() << "Find node id failed:" << nodeItem->id();
            return QUuid();
        }
        return node->partId;
    } else if (item->data(0) == "edge") {
        SkeletonGraphicsEdgeItem *edgeItem = (SkeletonGraphicsEdgeItem *)item;
        const SkeletonEdge *edge = m_document->findEdge(edgeItem->id());
        if (!edge) {
            qDebug() << "Find edge id failed:" << edgeItem->id();
            return QUuid();
        }
        return edge->partId;
    }
    return QUuid();
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
            if (nullptr != edgeIdSet)
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

void SkeletonGraphicsWidget::selectPartAllById(QUuid partId)
{
    unselectAll();
    for (const auto &it: nodeItemMap) {
        SkeletonGraphicsNodeItem *item = it.second.first;
        const SkeletonNode *node = m_document->findNode(item->id());
        if (!node)
            continue;
        if (node->partId != partId)
            continue;
        addItemToRangeSelection(item);
    }
    for (const auto &it: edgeItemMap) {
        SkeletonGraphicsEdgeItem *item = it.second.first;
        const SkeletonEdge *edge = m_document->findEdge(item->id());
        if (!edge)
            continue;
        if (edge->partId != partId)
            continue;
        addItemToRangeSelection(item);
    }
    hoverPart(QUuid());
}

void SkeletonGraphicsWidget::addSelectNode(QUuid nodeId)
{
    const auto &findResult = nodeItemMap.find(nodeId);
    if (findResult == nodeItemMap.end()) {
        qDebug() << "addSelectNode failed because of node id not exists:<<" << nodeId;
        return;
    }
    addItemToRangeSelection(findResult->second.first);
    hoverPart(QUuid());
}

void SkeletonGraphicsWidget::addSelectEdge(QUuid edgeId)
{
    const auto &findResult = edgeItemMap.find(edgeId);
    if (findResult == edgeItemMap.end()) {
        qDebug() << "addSelectEdge failed because of edge id not exists:<<" << edgeId;
        return;
    }
    addItemToRangeSelection(findResult->second.first);
    hoverPart(QUuid());
}

void SkeletonGraphicsWidget::addPartToSelection(QUuid partId)
{
    SkeletonProfile choosenProfile = SkeletonProfile::Main;
    if (m_hoveredNodeItem) {
        choosenProfile = m_hoveredNodeItem->profile();
    } else if (m_hoveredEdgeItem) {
        choosenProfile = m_hoveredEdgeItem->profile();
    }
    QUuid choosenPartId = partId;
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

void SkeletonGraphicsWidget::selectConnectedAll()
{
    unselectAll();
    SkeletonProfile choosenProfile = SkeletonProfile::Main;
    QUuid startNodeId;
    if (m_hoveredNodeItem) {
        choosenProfile = m_hoveredNodeItem->profile();
        const SkeletonNode *node = m_document->findNode(m_hoveredNodeItem->id());
        if (node)
            startNodeId = node->id;
    } else if (m_hoveredEdgeItem) {
        choosenProfile = m_hoveredEdgeItem->profile();
        const SkeletonEdge *edge = m_document->findEdge(m_hoveredEdgeItem->id());
        if (edge && !edge->nodeIds.empty())
            startNodeId = *edge->nodeIds.begin();
    }
    if (startNodeId.isNull())
        return;
    std::set<QUuid> visitedNodes;
    std::set<QUuid> visitedEdges;
    std::queue<QUuid> nodeIds;
    nodeIds.push(startNodeId);
    while (!nodeIds.empty()) {
        QUuid nodeId = nodeIds.front();
        nodeIds.pop();
        if (visitedNodes.find(nodeId) != visitedNodes.end())
            continue;
        const SkeletonNode *node = m_document->findNode(nodeId);
        if (nullptr == node)
            continue;
        visitedNodes.insert(nodeId);
        for (const auto &edgeId: node->edgeIds) {
            const SkeletonEdge *edge = m_document->findEdge(edgeId);
            if (nullptr == edge)
                continue;
            visitedEdges.insert(edgeId);
            for (const auto &nodeIdOfEdge: edge->nodeIds) {
                if (visitedNodes.find(nodeIdOfEdge) != visitedNodes.end())
                    continue;
                nodeIds.push(nodeIdOfEdge);
            }
        }
    }
    for (const auto &it: nodeItemMap) {
        SkeletonGraphicsNodeItem *item = SkeletonProfile::Main == choosenProfile ? it.second.first : it.second.second;
        if (visitedNodes.find(item->id()) == visitedNodes.end())
            continue;
        addItemToRangeSelection(item);
    }
    for (const auto &it: edgeItemMap) {
        SkeletonGraphicsEdgeItem *item = SkeletonProfile::Main == choosenProfile ? it.second.first : it.second.second;
        if (visitedEdges.find(item->id()) == visitedEdges.end())
            continue;
        addItemToRangeSelection(item);
    }
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
    if (!choosenPartId.isNull()) {
        emit partComponentChecked(choosenPartId);
    }
}

void SkeletonGraphicsWidget::shortcutCheckPartComponent()
{
    QUuid choosenPartId;
    if (m_hoveredNodeItem) {
        const SkeletonNode *node = m_document->findNode(m_hoveredNodeItem->id());
        if (node)
            choosenPartId = node->partId;
    } else if (m_hoveredEdgeItem) {
        const SkeletonEdge *edge = m_document->findEdge(m_hoveredEdgeItem->id());
        if (edge)
            choosenPartId = edge->partId;
    }
    if (!choosenPartId.isNull()) {
        emit partComponentChecked(choosenPartId);
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
    m_document->copyNodes(nodeIdSet);
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
    std::vector<QGraphicsItem *> contentItems;
    for (const auto &it: items()) {
        QGraphicsItem *item = it;
        if (item->data(0) == "node") {
            contentItems.push_back(item);
        } else if (item->data(0) == "edge") {
            contentItems.push_back(item);
        }
    }
    for (size_t i = 0; i < contentItems.size(); i++) {
        QGraphicsItem *item = contentItems[i];
        Q_ASSERT(item != m_cursorEdgeItem);
        Q_ASSERT(item != m_cursorNodeItem);
        delete item;
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

void SkeletonGraphicsWidget::hoverPart(QUuid partId)
{
    if (partId.isNull()) {
        if (!m_rangeSelectionSet.empty()) {
            const auto &it = m_rangeSelectionSet.begin();
            partId = querySkeletonItemPartId(*it);
        }
    }
    
    if (m_lastCheckedPart == partId)
        return;
    
    if (!m_lastCheckedPart.isNull())
        emit partUnchecked(m_lastCheckedPart);
    m_lastCheckedPart = partId;
    if (!m_lastCheckedPart.isNull())
        emit partChecked(m_lastCheckedPart);
}

void SkeletonGraphicsWidget::switchProfileOnRangeSelection()
{
    auto copiedSet = m_rangeSelectionSet;
    for (const auto &item: copiedSet) {
        if (item->data(0) == "node") {
            SkeletonGraphicsNodeItem *nodeItem = (SkeletonGraphicsNodeItem *)item;
            const auto &find = nodeItemMap.find(nodeItem->id());
            if (find == nodeItemMap.end()) {
                qDebug() << "Node item map key not found:" << nodeItem->id();
                return;
            }
            checkSkeletonItem(nodeItem, false);
            m_rangeSelectionSet.erase(nodeItem);
            SkeletonGraphicsNodeItem *altNodeItem = nodeItem == find->second.first ? find->second.second : find->second.first;
            if (checkSkeletonItem(altNodeItem, true))
                m_rangeSelectionSet.insert(altNodeItem);
        } else if (item->data(0) == "edge") {
            SkeletonGraphicsEdgeItem *edgeItem = (SkeletonGraphicsEdgeItem *)item;
            const auto &find = edgeItemMap.find(edgeItem->id());
            if (find == edgeItemMap.end()) {
                qDebug() << "Edge item map key not found:" << edgeItem->id();
                return;
            }
            checkSkeletonItem(edgeItem, false);
            m_rangeSelectionSet.erase(edgeItem);
            SkeletonGraphicsEdgeItem *altEdgeItem = edgeItem == find->second.first ? find->second.second : find->second.first;
            if (checkSkeletonItem(altEdgeItem, true))
                m_rangeSelectionSet.insert(altEdgeItem);
        }
    }
}

void SkeletonGraphicsWidget::setItemHoveredOnAllProfiles(QGraphicsItem *item, bool hovered)
{
    if (item->data(0) == "node") {
        SkeletonGraphicsNodeItem *nodeItem = (SkeletonGraphicsNodeItem *)item;
        const auto &find = nodeItemMap.find(nodeItem->id());
        if (find == nodeItemMap.end()) {
            qDebug() << "Node item map key not found:" << nodeItem->id();
            return;
        }
        find->second.first->setHovered(hovered);
        find->second.second->setHovered(hovered);
    } else if (item->data(0) == "edge") {
        SkeletonGraphicsEdgeItem *edgeItem = (SkeletonGraphicsEdgeItem *)item;
        const auto &find = edgeItemMap.find(edgeItem->id());
        if (find == edgeItemMap.end()) {
            qDebug() << "Edge item map key not found:" << edgeItem->id();
            return;
        }
        find->second.first->setHovered(hovered);
        find->second.second->setHovered(hovered);
    }
}

void SkeletonGraphicsWidget::ikMoveReady()
{
    unsigned long long movedUpdateVersion = m_ikMover->getUpdateVersion();
    
    if (movedUpdateVersion == m_ikMoveUpdateVersion &&
            !m_ikMoveEndEffectorId.isNull()) {
        emit batchChangeBegin();
        for (const auto &it: m_ikMover->ikNodes()) {
            if (m_rotated)
                emit setNodeOrigin(it.id, it.newPosition.y(), it.newPosition.x(), it.newPosition.z());
            else
                emit setNodeOrigin(it.id, it.newPosition.x(), it.newPosition.y(), it.newPosition.z());
        }
        emit batchChangeEnd();
        emit groupOperationAdded();
    }
    
    delete m_ikMover;
    m_ikMover = nullptr;
    
    if (movedUpdateVersion != m_ikMoveUpdateVersion &&
            !m_ikMoveEndEffectorId.isNull()) {
        ikMove(m_ikMoveEndEffectorId, m_ikMoveTarget);
    }
}

void SkeletonGraphicsWidget::ikMove(QUuid endEffectorId, QVector3D target)
{
    m_ikMoveEndEffectorId = endEffectorId;
    m_ikMoveTarget = target;
    m_ikMoveUpdateVersion++;
    if (nullptr != m_ikMover) {
        return;
    }
    
    QThread *thread = new QThread;
    
    m_ikMover = new SkeletonIkMover();
    m_ikMover->setUpdateVersion(m_ikMoveUpdateVersion);
    m_ikMover->setTarget(m_ikMoveTarget);
    QUuid nodeId = endEffectorId;
    std::set<QUuid> historyNodeIds;
    std::vector<std::pair<QUuid, QVector3D>> appendNodes;
    for (;;) {
        historyNodeIds.insert(nodeId);
        const auto node = m_document->findNode(nodeId);
        if (nullptr == node)
            break;
        appendNodes.push_back(std::make_pair(nodeId, QVector3D(node->getX(m_rotated), node->getY(m_rotated), node->getZ(m_rotated))));
        if (node->edgeIds.size() < 1 || node->edgeIds.size() > 2)
            break;
        QUuid choosenNodeId;
        for (const auto &edgeId: node->edgeIds) {
            const auto edge = m_document->findEdge(edgeId);
            if (nullptr == edge)
                break;
            QUuid neighborNodeId = edge->neighborOf(nodeId);
            if (historyNodeIds.find(neighborNodeId) != historyNodeIds.end())
                continue;
            choosenNodeId = neighborNodeId;
            break;
        }
        if (choosenNodeId.isNull())
            break;
        nodeId = choosenNodeId;
    }
    qDebug() << "ik move nodes:";
    for (int i = appendNodes.size() - 1; i >= 0; i--) {
        qDebug() << i << appendNodes[i].first << appendNodes[i].second;
        m_ikMover->addNode(appendNodes[i].first, appendNodes[i].second);
    }
    qDebug() << "target:" << m_ikMoveTarget;
    m_ikMover->moveToThread(thread);
    connect(thread, &QThread::started, m_ikMover, &SkeletonIkMover::process);
    connect(m_ikMover, &SkeletonIkMover::finished, this, &SkeletonGraphicsWidget::ikMoveReady);
    connect(m_ikMover, &SkeletonIkMover::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void SkeletonGraphicsWidget::setSelectedNodesBoneMark(BoneMark boneMark)
{
    std::set<QUuid> nodeIdSet;
    std::set<QUuid> edgeIdSet;
    readSkeletonNodeAndEdgeIdSetFromRangeSelection(&nodeIdSet, &edgeIdSet);
    if (!nodeIdSet.empty()) {
        emit batchChangeBegin();
        for (const auto &id: nodeIdSet) {
            emit setNodeBoneMark(id, boneMark);
        }
        emit batchChangeEnd();
        emit groupOperationAdded();
    }
}

void SkeletonGraphicsWidget::showSelectedCutFaceSettingPopup(const QPoint &pos)
{
    std::set<QUuid> nodeIdSet;
    std::set<QUuid> edgeIdSet;
    readSkeletonNodeAndEdgeIdSetFromRangeSelection(&nodeIdSet, &edgeIdSet);
    emit showCutFaceSettingPopup(mapToGlobal(pos), nodeIdSet);
}

void SkeletonGraphicsWidget::clearSelectedCutFace()
{
    std::set<QUuid> nodeIdSet;
    for (const auto &it: m_rangeSelectionSet) {
        if (it->data(0) == "node") {
            const auto &nodeId = ((SkeletonGraphicsNodeItem *)it)->id();
            const SkeletonNode *node = m_document->findNode(nodeId);
            if (nullptr == node) {
                qDebug() << "Find node failed:" << nodeId;
                continue;
            }
            if (node->hasCutFaceSettings) {
                nodeIdSet.insert(nodeId);
            }
        }
    }
    if (nodeIdSet.empty())
        return;
    emit batchChangeBegin();
    for (const auto &id: nodeIdSet) {
        emit clearNodeCutFaceSettings(id);
    }
    emit batchChangeEnd();
    emit groupOperationAdded();
}

void SkeletonGraphicsWidget::setNodePositionModifyOnly(bool nodePositionModifyOnly)
{
    m_nodePositionModifyOnly = nodePositionModifyOnly;
}

void SkeletonGraphicsWidget::setMainProfileOnly(bool mainProfileOnly)
{
    m_mainProfileOnly = mainProfileOnly;
}

void SkeletonGraphicsWidget::createWrapParts()
{
    std::set<SkeletonGraphicsNodeItem *> nodeItemSet;
    readMergedSkeletonNodeSetFromRangeSelection(&nodeItemSet);
    std::set<QUuid> nodeIds;
    for (const auto &it: nodeItemSet) {
        nodeIds.insert(it->id());
    }
    emit createGriddedPartsFromNodes(nodeIds);
}

