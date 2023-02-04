#include "skeleton_graphics_widget.h"
#include "skeleton_graphics_edge_item.h"
#include "skeleton_graphics_node_item.h"
#include "skeleton_graphics_origin_item.h"
#include "skeleton_graphics_selection_item.h"
#include "theme.h"
#include <QApplication>
#include <QBitmap>
#include <QDebug>
#include <QGuiApplication>
#include <QMatrix4x4>
#include <QMenu>
#include <QScrollBar>
#include <QVector2D>
#include <QtGlobal>
#include <algorithm>
#include <cmath>
#include <queue>

SkeletonGraphicsWidget::SkeletonGraphicsWidget(const Document* document)
    : m_document(document)
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

    m_mainOriginItem = new SkeletonGraphicsOriginItem(Document::Profile::Main);
    m_mainOriginItem->setRotated(m_rotated);
    m_mainOriginItem->hide();
    scene()->addItem(m_mainOriginItem);

    m_sideOriginItem = new SkeletonGraphicsOriginItem(Document::Profile::Side);
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

void SkeletonGraphicsWidget::setModelWidget(ModelWidget* modelWidget)
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
    if (!isVisible())
        return;

    if (Document::EditMode::Select != m_document->editMode) {
        emit setEditMode(Document::EditMode::Select);
        return;
    }
}

void SkeletonGraphicsWidget::showContextMenu(const QPoint& pos)
{
    if (Document::EditMode::Add == m_document->editMode) {
        emit setEditMode(Document::EditMode::Select);
        return;
    }

    if (Document::EditMode::Select != m_document->editMode) {
        return;
    }

    QMenu contextMenu(this);

    QAction addAction(tr("Add..."), this);
    connect(&addAction, &QAction::triggered, [=]() {
        emit setEditMode(Document::EditMode::Add);
    });
    contextMenu.addAction(&addAction);

    QAction undoAction(tr("Undo"), this);
    if (m_document->undoable()) {
        connect(&undoAction, &QAction::triggered, m_document, &Document::undo);
        contextMenu.addAction(&undoAction);
    }

    QAction redoAction(tr("Redo"), this);
    if (m_document->redoable()) {
        connect(&redoAction, &QAction::triggered, m_document, &Document::redo);
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

    QAction reduceAction(tr("Reduce"), this);
    if (hasSelection()) {
        connect(&reduceAction, &QAction::triggered, this, &SkeletonGraphicsWidget::reduceSelected);
        contextMenu.addAction(&reduceAction);
    }

    QAction reverseAction(tr("Reverse"), this);
    if (hasEdgeSelection()) {
        connect(&reverseAction, &QAction::triggered, this, &SkeletonGraphicsWidget::reverseSelectedEdges);
        contextMenu.addAction(&reverseAction);
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
    if (hasNodeSelection()) {
        connect(&copyAction, &QAction::triggered, this, &SkeletonGraphicsWidget::copy);
        contextMenu.addAction(&copyAction);
    }

    QAction pasteAction(tr("Paste"), this);
    if (m_document->hasPastableNodesInClipboard()) {
        connect(&pasteAction, &QAction::triggered, m_document, &Document::paste);
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

    QAction rotateClockwiseAction(tr("Rotate 90D CW"), this);
    if (hasMultipleSelection()) {
        connect(&rotateClockwiseAction, &QAction::triggered, this, &SkeletonGraphicsWidget::rotateClockwise90Degree);
        contextMenu.addAction(&rotateClockwiseAction);
    }

    QAction rotateCounterclockwiseAction(tr("Rotate 90D CCW"), this);
    if (hasMultipleSelection()) {
        connect(&rotateCounterclockwiseAction, &QAction::triggered, this, &SkeletonGraphicsWidget::rotateCounterclockwise90Degree);
        contextMenu.addAction(&rotateCounterclockwiseAction);
    }

    QAction switchXzAction(tr("Switch XZ"), this);
    if (hasSelection()) {
        connect(&switchXzAction, &QAction::triggered, this, &SkeletonGraphicsWidget::switchSelectedXZ);
        contextMenu.addAction(&switchXzAction);
    }

    QAction alignToLocalCenterAction(tr("Local Center"), this);
    QAction alignToLocalVerticalCenterAction(tr("Local Vertical Center"), this);
    QAction alignToLocalHorizontalCenterAction(tr("Local Horizontal Center"), this);
    QAction alignToGlobalCenterAction(tr("Global Center"), this);
    QAction alignToGlobalVerticalCenterAction(tr("Global Vertical Center"), this);
    QAction alignToGlobalHorizontalCenterAction(tr("Global Horizontal Center"), this);
    if (((hasSelection() && m_document->originSettled()) || hasMultipleSelection())) {
        QMenu* subMenu = contextMenu.addMenu(tr("Align To"));

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

    QAction colorizeAsBlankAction(tr("Blank"), this);
    QAction colorizeAsAutoColorAction(tr("Auto Color"), this);
    if (hasNodeSelection()) {
        QMenu* subMenu = contextMenu.addMenu(tr("Colorize"));

        connect(&colorizeAsBlankAction, &QAction::triggered, this, &SkeletonGraphicsWidget::fadeSelected);
        subMenu->addAction(&colorizeAsBlankAction);

        connect(&colorizeAsAutoColorAction, &QAction::triggered, this, &SkeletonGraphicsWidget::colorizeSelected);
        subMenu->addAction(&colorizeAsAutoColorAction);
    }

    QAction markAsBoneJointAction(tr("Bone Joint"), this);
    QAction markAsNotBoneJointAction(tr("Not Bone Joint"), this);
    if (hasNodeSelection()) {
        QMenu* subMenu = contextMenu.addMenu(tr("Mark As"));

        connect(&markAsBoneJointAction, &QAction::triggered, this, &SkeletonGraphicsWidget::markSelectedAsBoneJoint);
        subMenu->addAction(&markAsBoneJointAction);

        connect(&markAsNotBoneJointAction, &QAction::triggered, this, &SkeletonGraphicsWidget::markSelectedAsNotBoneJoint);
        subMenu->addAction(&markAsNotBoneJointAction);
    }

    if (hasNodeSelection() && !m_document->boneIdList.empty()) {
        QMenu* addToBoneMenu = contextMenu.addMenu(tr("Add To Bone"));
        QMenu* removeFromBoneMenu = contextMenu.addMenu(tr("Remove From Bone"));

        for (const auto& boneId : m_document->boneIdList) {
            const Document::Bone* bone = m_document->findBone(boneId);
            if (nullptr == bone)
                continue;
            QAction* addAction = new QAction(bone->previewPixmap, bone->name, this);
            connect(addAction, &QAction::triggered, this, [=]() {
                this->addSelectedToBone(boneId);
            });
            QAction* removeAction = new QAction(bone->previewPixmap, bone->name, this);
            connect(removeAction, &QAction::triggered, this, [=]() {
                this->removeSelectedFromBone(boneId);
            });
            addToBoneMenu->addAction(addAction);
            removeFromBoneMenu->addAction(removeAction);
        }
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
    for (const auto& it : m_rangeSelectionSet) {
        if (it->data(0) == "edge")
            return true;
    }
    return false;
}

bool SkeletonGraphicsWidget::hasNodeSelection()
{
    for (const auto& it : m_rangeSelectionSet) {
        if (it->data(0) == "node")
            return true;
    }
    return false;
}

bool SkeletonGraphicsWidget::hasTwoDisconnectedNodesSelection()
{
    std::vector<dust3d::Uuid> nodeIds;
    for (const auto& it : m_rangeSelectionSet) {
        if (it->data(0) == "node") {
            nodeIds.push_back(((SkeletonGraphicsNodeItem*)it)->id());
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

void SkeletonGraphicsWidget::fadeSelected()
{
    std::set<dust3d::Uuid> partIds;
    for (const auto& it : m_rangeSelectionSet) {
        if (it->data(0) == "node") {
            SkeletonGraphicsNodeItem* nodeItem = (SkeletonGraphicsNodeItem*)it;
            const Document::Node* node = m_document->findNode(nodeItem->id());
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
    for (const auto& it : partIds) {
        emit setPartColorState(it, false, Qt::white);
    }
    emit batchChangeEnd();
    emit groupOperationAdded();
}

void SkeletonGraphicsWidget::colorizeSelected()
{
    if (nullptr == m_backgroundImage)
        return;
    std::map<dust3d::Uuid, std::map<QString, size_t>> sumOfColor;
    for (const auto& it : m_rangeSelectionSet) {
        if (it->data(0) == "node") {
            SkeletonGraphicsNodeItem* nodeItem = (SkeletonGraphicsNodeItem*)it;
            const Document::Node* node = m_document->findNode(nodeItem->id());
            if (nullptr == node)
                continue;
            const auto& position = nodeItem->origin();
            sumOfColor[node->partId][m_backgroundImage->pixelColor(position.x(), position.y()).name()]++;
        } else if (it->data(0) == "edge") {
            SkeletonGraphicsEdgeItem* edgeItem = (SkeletonGraphicsEdgeItem*)it;
            const Document::Edge* edge = m_document->findEdge(edgeItem->id());
            if (nullptr == edge)
                continue;
            const auto position = (edgeItem->firstItem()->origin() + edgeItem->secondItem()->origin()) / 2;
            sumOfColor[edge->partId][m_backgroundImage->pixelColor(position.x(), position.y()).name()]++;
        }
    }
    if (sumOfColor.empty())
        return;
    emit batchChangeBegin();
    for (const auto& it : sumOfColor) {
        int r = 0;
        int g = 0;
        int b = 0;
        for (const auto& freq : it.second) {
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
    std::set<dust3d::Uuid> edgeIds;
    for (const auto& it : m_rangeSelectionSet) {
        if (it->data(0) == "edge")
            edgeIds.insert(((SkeletonGraphicsEdgeItem*)it)->id());
    }
    if (edgeIds.empty())
        return;
    emit batchChangeBegin();
    for (const auto& it : edgeIds) {
        emit breakEdge(it);
    }
    emit batchChangeEnd();
    emit groupOperationAdded();
}

void SkeletonGraphicsWidget::reduceSelected()
{
    std::set<dust3d::Uuid> nodeIds;
    for (const auto& it : m_rangeSelectionSet) {
        if (it->data(0) == "node")
            nodeIds.insert(((SkeletonGraphicsNodeItem*)it)->id());
    }
    if (nodeIds.empty())
        return;
    emit batchChangeBegin();
    for (const auto& it : nodeIds) {
        emit reduceNode(it);
    }
    emit batchChangeEnd();
    emit groupOperationAdded();
}

void SkeletonGraphicsWidget::reverseSelectedEdges()
{
    std::set<dust3d::Uuid> edgeIds;
    for (const auto& it : m_rangeSelectionSet) {
        if (it->data(0) == "edge")
            edgeIds.insert(((SkeletonGraphicsEdgeItem*)it)->id());
    }
    if (edgeIds.empty())
        return;
    emit batchChangeBegin();
    for (const auto& it : edgeIds) {
        emit reverseEdge(it);
    }
    emit batchChangeEnd();
    emit groupOperationAdded();
}

void SkeletonGraphicsWidget::connectSelected()
{
    std::vector<dust3d::Uuid> nodeIds;
    for (const auto& it : m_rangeSelectionSet) {
        if (it->data(0) == "node") {
            nodeIds.push_back(((SkeletonGraphicsNodeItem*)it)->id());
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
    std::set<SkeletonGraphicsNodeItem*> nodeItems;
    readMergedSkeletonNodeSetFromRangeSelection(&nodeItems);
    if (nodeItems.empty())
        return;
    if (nodeItems.size() < 2)
        return;
    emit batchChangeBegin();
    QVector2D center = centerOfNodeItemSet(nodeItems);
    for (const auto& it : nodeItems) {
        SkeletonGraphicsNodeItem* nodeItem = it;
        QPointF nodeOrigin = nodeItem->origin();
        float byX = alignToHorizontalCenter ? sceneRadiusToUnified(center.x() - nodeOrigin.x()) : 0;
        float byY = alignToVerticalCenter ? sceneRadiusToUnified(center.y() - nodeOrigin.y()) : 0;
        if (Document::Profile::Main == nodeItem->profile()) {
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
    std::set<SkeletonGraphicsNodeItem*> nodeItems;
    readMergedSkeletonNodeSetFromRangeSelection(&nodeItems);
    if (nodeItems.empty())
        return;
    emit batchChangeBegin();
    for (const auto& nodeItem : nodeItems) {
        const Document::Node* node = m_document->findNode(nodeItem->id());
        if (!node) {
            qDebug() << "Find node id failed:" << nodeItem->id();
            continue;
        }
        float byX = 0;
        float byY = 0;
        float byZ = 0;
        if (Document::Profile::Main == nodeItem->profile()) {
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
    const QImage* turnaroundImage = &m_document->turnaround;
    QImage onePixel(2, 1, QImage::Format_ARGB32);
    if (turnaroundImage->isNull()) {
        onePixel.fill(Qt::white);
        turnaroundImage = &onePixel;
    }

    m_turnaroundChanged = true;
    if (m_turnaroundLoader)
        return;

    m_turnaroundChanged = false;

    QThread* thread = new QThread;
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

    emit loadedTurnaroundImageChanged();

    if (m_turnaroundChanged) {
        updateTurnaround();
    }
}

const QImage* SkeletonGraphicsWidget::loadedTurnaroundImage() const
{
    return m_backgroundImage;
}

void SkeletonGraphicsWidget::updateCursor()
{
    if (Document::EditMode::Add != m_document->editMode) {
        m_cursorEdgeItem->hide();
        m_cursorNodeItem->hide();
    }

    // Set cursor here if needed
    switch (m_document->editMode) {
    case Document::EditMode::Pick:
        setCursor(Qt::CrossCursor);
        break;
    default:
        setCursor(Qt::ArrowCursor);
    }

    emit cursorChanged();
}

void SkeletonGraphicsWidget::editModeChanged()
{
    updateCursor();
    if (Document::EditMode::Add == m_document->editMode) {
        SkeletonGraphicsNodeItem* choosenNodeItem = nullptr;
        if (!m_rangeSelectionSet.empty()) {
            std::set<SkeletonGraphicsNodeItem*> nodeItems;
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

void SkeletonGraphicsWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_modelWidget && m_modelWidget->inputMouseMoveEventFromOtherWidget(event))
        return;

    QGraphicsView::mouseMoveEvent(event);
    mouseMove(event);
}

bool SkeletonGraphicsWidget::rotated()
{
    return m_rotated;
}

bool SkeletonGraphicsWidget::inputWheelEventFromOtherWidget(QWheelEvent* event)
{
    if (!wheel(event)) {
        if (m_modelWidget && m_modelWidget->inputWheelEventFromOtherWidget(event))
            return true;
    }
    return true;
}

void SkeletonGraphicsWidget::wheelEvent(QWheelEvent* event)
{
    if (!wheel(event)) {
        if (m_modelWidget && m_modelWidget->inputWheelEventFromOtherWidget(event))
            return;
    }
}

void SkeletonGraphicsWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_modelWidget && m_modelWidget->inputMouseReleaseEventFromOtherWidget(event))
        return;

    QGraphicsView::mouseReleaseEvent(event);
    mouseRelease(event);
}

void SkeletonGraphicsWidget::mousePressEvent(QMouseEvent* event)
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

void SkeletonGraphicsWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    QGraphicsView::mouseDoubleClickEvent(event);
    mouseDoubleClick(event);
}

void SkeletonGraphicsWidget::keyPressEvent(QKeyEvent* event)
{
    if (keyPress(event))
        return;
    QGraphicsView::keyPressEvent(event);
}

void SkeletonGraphicsWidget::keyReleaseEvent(QKeyEvent* event)
{
    if (keyRelease(event))
        return;
    QGraphicsView::keyReleaseEvent(event);
}

bool SkeletonGraphicsWidget::mouseMove(QMouseEvent* event)
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

    if (Document::EditMode::Select == m_document->editMode) {
        if (m_rangeSelectionStarted) {
            QPointF mouseScenePos = mouseEventScenePos(event);
            m_selectionItem->updateRange(m_rangeSelectionStartPos, mouseScenePos);
            if (!m_selectionItem->isVisible())
                m_selectionItem->setVisible(true);
            checkRangeSelection();
            return true;
        }
    }

    if (Document::EditMode::Select == m_document->editMode || Document::EditMode::Add == m_document->editMode || Document::EditMode::Pick == m_document->editMode) {

        //
        // > For overlapping nodes, you can make it a bit better by selecting the node center nearest the mouse, rather than simply checking against the wider circle.
        // > https://www.reddit.com/user/hdu
        //

        SkeletonGraphicsNodeItem* newHoverNodeItem = nullptr;
        SkeletonGraphicsEdgeItem* newHoverEdgeItem = nullptr;
        SkeletonGraphicsOriginItem* newHoverOriginItem = nullptr;
        QPointF scenePos = mouseEventScenePos(event);
        QList<QGraphicsItem*> items = scene()->items(scenePos);
        std::vector<std::pair<QGraphicsItem*, float>> itemDistance2MapWithMouse;
        for (auto it = items.begin(); it != items.end(); it++) {
            QGraphicsItem* item = *it;
            if (item->data(0) == "node") {
                SkeletonGraphicsNodeItem* nodeItem = (SkeletonGraphicsNodeItem*)item;
                if (nodeItem->deactivated())
                    continue;
                QPointF origin = nodeItem->origin();
                float distance2 = pow(origin.x() - scenePos.x(), 2) + pow(origin.y() - scenePos.y(), 2);
                itemDistance2MapWithMouse.push_back(std::make_pair(item, distance2));
            } else if (item->data(0) == "edge") {
                SkeletonGraphicsEdgeItem* edgeItem = (SkeletonGraphicsEdgeItem*)item;
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
                newHoverOriginItem = (SkeletonGraphicsOriginItem*)item;
            }
        }
        if (!itemDistance2MapWithMouse.empty()) {
            std::sort(itemDistance2MapWithMouse.begin(), itemDistance2MapWithMouse.end(),
                [](const std::pair<QGraphicsItem*, float>& a, const std::pair<QGraphicsItem*, float>& b) {
                    return a.second < b.second;
                });
            QGraphicsItem* pickedNearestItem = itemDistance2MapWithMouse[0].first;
            if (pickedNearestItem->data(0) == "node") {
                newHoverNodeItem = (SkeletonGraphicsNodeItem*)pickedNearestItem;
            } else if (pickedNearestItem->data(0) == "edge") {
                newHoverEdgeItem = (SkeletonGraphicsEdgeItem*)pickedNearestItem;
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
        dust3d::Uuid hoveredPartId;
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

    if (Document::EditMode::Add == m_document->editMode) {
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

    if (Document::EditMode::Select == m_document->editMode) {
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
                    std::set<SkeletonGraphicsNodeItem*> nodeItems;
                    readMergedSkeletonNodeSetFromRangeSelection(&nodeItems);
                    bool ikMoved = false;
                    if (nodeItems.size() == 1) {
                        auto& nodeItem = *nodeItems.begin();
                        const Document::Node* node = m_document->findNode(nodeItem->id());
                        if (node->edgeIds.size() == 1) {
                            const auto origin = nodeItem->origin();
                            byX = mouseScenePos.x() - origin.x();
                            byY = mouseScenePos.y() - origin.y();
                            byX = sceneRadiusToUnified(byX);
                            byY = sceneRadiusToUnified(byY);
                            QVector3D target = QVector3D(node->getX(m_rotated), node->getY(m_rotated), node->getZ(m_rotated));
                            if (Document::Profile::Main == nodeItem->profile()) {
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

    std::set<SkeletonGraphicsNodeItem*> nodeItems;
    readMergedSkeletonNodeSetFromRangeSelection(&nodeItems);

    QVector2D center;

    if (1 == nodeItems.size() && m_document->originSettled()) {
        // Rotate all children nodes around this node
        // 1. Pick who is the parent from neighbors
        // 2. Rotate all the remaining neighbor nodes and their children exlucde the picked parent
        QVector3D worldOrigin = QVector3D(m_document->getOriginX(m_rotated), m_document->getOriginY(m_rotated), m_document->getOriginZ(m_rotated));
        SkeletonGraphicsNodeItem* centerNodeItem = *nodeItems.begin();
        const auto& centerNode = m_document->findNode(centerNodeItem->id());
        if (nullptr == centerNode) {
            qDebug() << "findNode:" << centerNodeItem->id() << " failed while rotate";
            return;
        }
        std::vector<std::pair<dust3d::Uuid, float>> directNeighborDists;
        for (const auto& neighborId : centerNode->edgeIds) {
            const auto& neighborEdge = m_document->findEdge(neighborId);
            if (nullptr == neighborEdge) {
                qDebug() << "findEdge:" << neighborId << " failed while rotate";
                continue;
            }
            const auto& neighborNodeId = neighborEdge->neighborOf(centerNodeItem->id());
            const auto& neighborNode = m_document->findNode(neighborNodeId);
            if (nullptr == centerNode) {
                qDebug() << "findNode:" << neighborNodeId << " failed while rotate";
                continue;
            }
            QVector3D nodeOrigin(neighborNode->getX(m_rotated), neighborNode->getY(m_rotated), neighborNode->getZ(m_rotated));
            directNeighborDists.push_back(std::make_pair(neighborNodeId, (nodeOrigin - worldOrigin).lengthSquared()));
        }
        std::sort(directNeighborDists.begin(), directNeighborDists.end(), [](const std::pair<dust3d::Uuid, float>& a, const std::pair<dust3d::Uuid, float>& b) {
            return a.second < b.second;
        });
        int skippedNum = 1;
        if (directNeighborDists.size() == 1) {
            skippedNum = 0;
        }
        std::set<dust3d::Uuid> neighborIds;
        // Adding self to force the neighbor finding not reverse
        neighborIds.insert(centerNodeItem->id());
        for (int i = skippedNum; i < (int)directNeighborDists.size(); i++) {
            neighborIds.insert(directNeighborDists[i].first);
            m_document->findAllNeighbors(directNeighborDists[i].first, neighborIds);
        }
        center = QVector2D(centerNodeItem->origin().x(), centerNodeItem->origin().y());
        nodeItems.clear();
        for (const auto& neighborId : neighborIds) {
            const auto& findItemResult = nodeItemMap.find(neighborId);
            if (findItemResult == nodeItemMap.end()) {
                qDebug() << "find node item:" << neighborId << "failed";
                continue;
            }
            if (Document::Profile::Main == centerNodeItem->profile()) {
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

void SkeletonGraphicsWidget::rotateItems(const std::set<SkeletonGraphicsNodeItem*>& nodeItems, int degree, QVector2D center)
{
    emit disableAllPositionRelatedLocks();
    QVector3D centerPoint(center.x(), center.y(), 0);
    for (const auto& it : nodeItems) {
        SkeletonGraphicsNodeItem* nodeItem = it;
        QMatrix4x4 mat;
        QPointF nodeOrigin = nodeItem->origin();
        QVector3D nodeOriginPoint(nodeOrigin.x(), nodeOrigin.y(), 0);
        QVector3D p = nodeOriginPoint - centerPoint;
        mat.rotate(degree, 0, 0, 1);
        p = mat * p;
        QVector3D finalPoint = p + centerPoint;
        float byX = sceneRadiusToUnified(finalPoint.x() - nodeOrigin.x());
        float byY = sceneRadiusToUnified(finalPoint.y() - nodeOrigin.y());
        if (Document::Profile::Main == nodeItem->profile()) {
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
    std::set<SkeletonGraphicsNodeItem*> items;
    for (const auto& item : nodeItemMap) {
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
    if (Document::Profile::Main == m_checkedOriginItem->profile()) {
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

    m_ikMoveEndEffectorId = dust3d::Uuid();

    byX = sceneRadiusToUnified(byX);
    byY = sceneRadiusToUnified(byY);
    std::set<SkeletonGraphicsNodeItem*> nodeItems;
    readMergedSkeletonNodeSetFromRangeSelection(&nodeItems);

    for (const auto& it : nodeItems) {
        SkeletonGraphicsNodeItem* nodeItem = it;
        if (Document::Profile::Main == nodeItem->profile()) {
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

    std::set<dust3d::Uuid> nodeIdSet;
    readSkeletonNodeAndEdgeIdSetFromRangeSelection(&nodeIdSet);
    if (nodeIdSet.empty())
        return;

    emit batchChangeBegin();
    for (const auto nodeId : nodeIdSet) {
        emit switchNodeXZ(nodeId);
    }
    emit batchChangeEnd();
    emit groupOperationAdded();
}

void SkeletonGraphicsWidget::zoomSelected(float delta)
{
    if (m_rangeSelectionSet.empty())
        return;

    std::set<SkeletonGraphicsNodeItem*> nodeItems;
    readMergedSkeletonNodeSetFromRangeSelection(&nodeItems);

    float unifiedDelta = sceneRadiusToUnified(delta);
    for (const auto& it : nodeItems) {
        SkeletonGraphicsNodeItem* nodeItem = it;
        emit scaleNodeByAddRadius(nodeItem->id(), unifiedDelta);
    }
}

void SkeletonGraphicsWidget::getOtherProfileNodeItems(const std::set<SkeletonGraphicsNodeItem*>& nodeItemSet,
    std::set<SkeletonGraphicsNodeItem*>* otherProfileNodeItemSet)
{
    for (const auto& nodeItem : nodeItemSet) {
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

    std::set<SkeletonGraphicsNodeItem*> nodeItems;
    readMergedSkeletonNodeSetFromRangeSelection(&nodeItems);

    std::set<SkeletonGraphicsNodeItem*> otherProfileNodeItems;
    getOtherProfileNodeItems(nodeItems, &otherProfileNodeItems);

    QVector2D center = centerOfNodeItemSet(nodeItems);
    QVector2D otherCenter = centerOfNodeItemSet(otherProfileNodeItems);
    std::map<dust3d::Uuid, std::tuple<float, float, float>> moveByMap;
    float scale = sceneRadiusToUnified(delta);
    for (const auto& nodeItem : nodeItems) {
        QVector2D origin = QVector2D(nodeItem->origin());
        QVector2D ray = (center - origin) * scale;
        float byX = -sceneRadiusToUnified(ray.x());
        float byY = -sceneRadiusToUnified(ray.y());
        if (Document::Profile::Main == nodeItem->profile()) {
            moveByMap[nodeItem->id()] = std::make_tuple(byX, byY, 0.0);
        } else {
            moveByMap[nodeItem->id()] = std::make_tuple(0.0, byY, byX);
        }
    }
    for (const auto& nodeItem : otherProfileNodeItems) {
        QVector2D origin = QVector2D(nodeItem->origin());
        QVector2D ray = (otherCenter - origin) * scale;
        float byX = -sceneRadiusToUnified(ray.x());
        if (Document::Profile::Main == nodeItem->profile()) {
            std::get<0>(moveByMap[nodeItem->id()]) = byX;
        } else {
            std::get<2>(moveByMap[nodeItem->id()]) = byX;
        }
    }
    for (const auto& it : moveByMap) {
        if (m_rotated)
            emit moveNodeBy(it.first, std::get<1>(it.second), std::get<0>(it.second), std::get<2>(it.second));
        else
            emit moveNodeBy(it.first, std::get<0>(it.second), std::get<1>(it.second), std::get<2>(it.second));
    }
}

bool SkeletonGraphicsWidget::wheel(QWheelEvent* event)
{
    float delta = 0;
    if (event->delta() > 0)
        delta = 1;
    else
        delta = -1;
    if (Document::EditMode::Add == m_document->editMode) {
        if (m_cursorNodeItem->isVisible()) {
            m_cursorNodeItem->setRadius(m_cursorNodeItem->radius() + delta);
            return true;
        }
    } else if (Document::EditMode::Select == m_document->editMode) {
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

QVector2D SkeletonGraphicsWidget::centerOfNodeItemSet(const std::set<SkeletonGraphicsNodeItem*>& set)
{
    QVector2D center;
    for (const auto& nodeItem : set) {
        center += QVector2D(nodeItem->origin());
    }
    center /= set.size();
    return center;
}

void SkeletonGraphicsWidget::flipHorizontally()
{
    std::set<SkeletonGraphicsNodeItem*> nodeItems;
    readMergedSkeletonNodeSetFromRangeSelection(&nodeItems);
    if (nodeItems.empty())
        return;
    QVector2D center = centerOfNodeItemSet(nodeItems);
    emit batchChangeBegin();
    for (const auto& nodeItem : nodeItems) {
        QPointF origin = nodeItem->origin();
        float offset = origin.x() - center.x();
        float unifiedOffset = -sceneRadiusToUnified(offset * 2);
        if (Document::Profile::Main == nodeItem->profile()) {
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
    std::set<SkeletonGraphicsNodeItem*> nodeItems;
    readMergedSkeletonNodeSetFromRangeSelection(&nodeItems);
    if (nodeItems.empty())
        return;
    QVector2D center = centerOfNodeItemSet(nodeItems);
    emit batchChangeBegin();
    for (const auto& nodeItem : nodeItems) {
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

bool SkeletonGraphicsWidget::mouseRelease(QMouseEvent* event)
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

QPointF SkeletonGraphicsWidget::mouseEventScenePos(QMouseEvent* event)
{
    return mapToScene(mapFromGlobal(event->globalPos()));
}

bool SkeletonGraphicsWidget::mousePress(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (Document::EditMode::Add == m_document->editMode) {
            if (m_cursorNodeItem->isVisible()) {
                if (m_addFromNodeItem) {
                    if (m_hoveredNodeItem && m_addFromNodeItem && m_hoveredNodeItem != m_addFromNodeItem && m_hoveredNodeItem->profile() == m_addFromNodeItem->profile() && !m_document->findEdgeByNodes(m_addFromNodeItem->id(), m_hoveredNodeItem->id()) && m_document->isNodeEditable(m_hoveredNodeItem->id())) {
                        if (m_document->isNodeConnectable(m_hoveredNodeItem->id())) {
                            emit addEdge(m_addFromNodeItem->id(), m_hoveredNodeItem->id());
                            emit groupOperationAdded();
                            emit setEditMode(Document::EditMode::Select);
                            return true;
                        }
                    }
                }
                QPointF mainProfile = m_cursorNodeItem->origin();
                QPointF sideProfile = mainProfile;
                if (m_addFromNodeItem) {
                    auto itemIt = nodeItemMap.find(m_addFromNodeItem->id());
                    if (Document::Profile::Main == m_addFromNodeItem->profile())
                        sideProfile.setX(itemIt->second.second->origin().x());
                    else
                        mainProfile.setX(itemIt->second.first->origin().x());
                } else {
                    if (mainProfile.x() >= scene()->width() / 2) {
                        sideProfile.setX(mainProfile.x() - scene()->width() / 4);
                    } else {
                        sideProfile.setX(mainProfile.x() + scene()->width() / 4);
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
                    emit addNode(unifiedMainPos.y(), unifiedMainPos.x(), unifiedSidePos.x(), sceneRadiusToUnified(m_cursorNodeItem->radius()), nullptr == m_addFromNodeItem ? dust3d::Uuid() : m_addFromNodeItem->id());
                else
                    emit addNode(unifiedMainPos.x(), unifiedMainPos.y(), unifiedSidePos.x(), sceneRadiusToUnified(m_cursorNodeItem->radius()), nullptr == m_addFromNodeItem ? dust3d::Uuid() : m_addFromNodeItem->id());
                emit groupOperationAdded();
                return true;
            }
        } else if (Document::EditMode::Select == m_document->editMode) {
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
                if ((nullptr == m_hoveredNodeItem || m_rangeSelectionSet.find(m_hoveredNodeItem) == m_rangeSelectionSet.end()) && (nullptr == m_hoveredEdgeItem || m_rangeSelectionSet.find(m_hoveredEdgeItem) == m_rangeSelectionSet.end())) {
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
        } else if (Document::EditMode::Pick == m_document->editMode) {
            if (m_hoveredNodeItem) {
                emit nodePicked(m_hoveredNodeItem->id());
            }
        }
    }

    if (event->button() == Qt::LeftButton) {
        if (Document::EditMode::Select == m_document->editMode) {
            if (!m_rangeSelectionStarted) {
                m_rangeSelectionStartPos = mouseEventScenePos(event);
                m_rangeSelectionStarted = true;
            }
        }
    }
    return false;
}

bool SkeletonGraphicsWidget::isFloatEqual(float a, float b)
{
    return fabs(a - b) <= 0.000001;
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

bool SkeletonGraphicsWidget::mouseDoubleClick(QMouseEvent* event)
{
    if (m_hoveredNodeItem || m_hoveredEdgeItem) {
        if (Document::EditMode::Select == m_document->editMode)
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
        const auto& it = m_deferredRemoveEdgeIds.begin();
        const auto edgeId = *it;
        m_deferredRemoveEdgeIds.erase(it);
        emit removeEdge(edgeId);
        committed = true;
    }

    if (!committed && !m_deferredRemoveNodeIds.empty()) {
        const auto& it = m_deferredRemoveNodeIds.begin();
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

    std::set<dust3d::Uuid> nodeIdSet;
    std::set<dust3d::Uuid> edgeIdSet;
    readSkeletonNodeAndEdgeIdSetFromRangeSelection(&nodeIdSet, &edgeIdSet);

    std::set<dust3d::Uuid> partIds;
    for (const auto& it : nodeIdSet) {
        const Document::Node* node = m_document->findNode(it);
        if (nullptr == node)
            continue;
        partIds.insert(node->partId);
    }
    for (const auto& it : edgeIdSet) {
        const Document::Edge* edge = m_document->findEdge(it);
        if (nullptr == edge)
            continue;
        partIds.insert(edge->partId);
    }
    std::set<dust3d::Uuid> cleanupPartIds;
    for (const auto& partId : partIds) {
        const Document::Part* part = m_document->findPart(partId);
        if (nullptr != part) {
            bool allNodesSelected = true;
            for (const auto& it : part->nodeIds) {
                if (nodeIdSet.find(it) == nodeIdSet.end()) {
                    allNodesSelected = false;
                    break;
                }
            }
            if (allNodesSelected) {
                cleanupPartIds.insert(partId);
            }
        }
    }
    for (const auto& partId : cleanupPartIds) {
        for (auto it = nodeIdSet.begin(); it != nodeIdSet.end();) {
            const Document::Node* node = m_document->findNode(*it);
            if (nullptr != node && node->partId == partId) {
                it = nodeIdSet.erase(it);
                continue;
            }
            ++it;
        }
        for (auto it = edgeIdSet.begin(); it != edgeIdSet.end();) {
            const Document::Edge* edge = m_document->findEdge(*it);
            if (nullptr != edge && edge->partId == partId) {
                it = edgeIdSet.erase(it);
                continue;
            }
            ++it;
        }
    }

    for (const auto& partId : cleanupPartIds) {
        emit removePart(partId);
    }

    for (const auto& it : nodeIdSet) {
        m_deferredRemoveNodeIds.insert(it);
    }
    for (const auto& it : edgeIdSet) {
        m_deferredRemoveEdgeIds.insert(it);
    }

    if (!nodeIdSet.empty() || !edgeIdSet.empty()) {
        if (nullptr == m_deferredRemoveTimer) {
            timeToRemoveDeferredNodesAndEdges();
        }
    }
}

void SkeletonGraphicsWidget::shortcutDelete()
{
    if (!isVisible())
        return;

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
    if (Document::EditMode::Add == m_document->editMode) {
        emit setEditMode(Document::EditMode::Select);
    } else {
        emit setEditMode(Document::EditMode::Add);
    }
}

void SkeletonGraphicsWidget::shortcutUndo()
{
    if (!isVisible())
        return;

    emit undo();
}

void SkeletonGraphicsWidget::shortcutRedo()
{
    if (!isVisible())
        return;

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
    if (!isVisible())
        return;

    cut();
}

void SkeletonGraphicsWidget::shortcutCopy()
{
    if (!isVisible())
        return;

    copy();
}

void SkeletonGraphicsWidget::shortcutPaste()
{
    if (!isVisible())
        return;

    emit paste();
}

void SkeletonGraphicsWidget::shortcutSelectMode()
{
    emit setEditMode(Document::EditMode::Select);
}

void SkeletonGraphicsWidget::shortcutZoomRenderedModelByMinus10()
{
    if (!isVisible())
        return;

    emit zoomRenderedModelBy(-10);
}

void SkeletonGraphicsWidget::shortcutZoomSelectedByMinus1()
{
    if (!isVisible())
        return;

    if (Document::EditMode::Select == m_document->editMode && hasSelection()) {
        zoomSelected(-1);
        emit groupOperationAdded();
    } else if (Document::EditMode::Add == m_document->editMode) {
        if (m_cursorNodeItem->isVisible()) {
            m_cursorNodeItem->setRadius(m_cursorNodeItem->radius() + -1);
        }
    }
}

void SkeletonGraphicsWidget::shortcutZoomRenderedModelBy10()
{
    if (!isVisible())
        return;

    emit zoomRenderedModelBy(10);
}

void SkeletonGraphicsWidget::shortcutZoomSelectedBy1()
{
    if (!isVisible())
        return;

    if (Document::EditMode::Select == m_document->editMode && hasSelection()) {
        zoomSelected(1);
        emit groupOperationAdded();
    } else if (Document::EditMode::Add == m_document->editMode) {
        if (m_cursorNodeItem->isVisible()) {
            m_cursorNodeItem->setRadius(m_cursorNodeItem->radius() + 1);
        }
    }
}

void SkeletonGraphicsWidget::shortcutRotateSelectedByMinus1()
{
    if (!isVisible())
        return;

    if ((Document::EditMode::Select == m_document->editMode) && hasSelection()) {
        rotateSelected(-1);
        emit groupOperationAdded();
    }
}

void SkeletonGraphicsWidget::shortcutRotateSelectedBy1()
{
    if (!isVisible())
        return;

    if ((Document::EditMode::Select == m_document->editMode) && hasSelection()) {
        rotateSelected(1);
        emit groupOperationAdded();
    }
}

void SkeletonGraphicsWidget::shortcutMoveSelectedToLeft()
{
    if (!isVisible())
        return;

    if (Document::EditMode::Select == m_document->editMode) {
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
    if (!isVisible())
        return;

    if (Document::EditMode::Select == m_document->editMode) {
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
    if (!isVisible())
        return;

    if (Document::EditMode::Select == m_document->editMode) {
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
    if (!isVisible())
        return;

    if (Document::EditMode::Select == m_document->editMode) {
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
    if (!isVisible())
        return;

    if ((Document::EditMode::Select == m_document->editMode) && hasSelection()) {
        scaleSelected(-1);
        emit groupOperationAdded();
    }
}

void SkeletonGraphicsWidget::shortcutScaleSelectedBy1()
{
    if (!isVisible())
        return;

    if ((Document::EditMode::Select == m_document->editMode) && hasSelection()) {
        scaleSelected(1);
        emit groupOperationAdded();
    }
}

void SkeletonGraphicsWidget::shortcutSwitchProfileOnSelected()
{
    if (!isVisible())
        return;

    if ((Document::EditMode::Select == m_document->editMode) && hasSelection()) {
        switchProfileOnRangeSelection();
    }
}

void SkeletonGraphicsWidget::shortcutShowOrHideSelectedPart()
{
    if (!isVisible())
        return;

    if (Document::EditMode::Select == m_document->editMode) {
        if (!m_lastCheckedPart.isNull()) {
            const Document::Part* part = m_document->findPart(m_lastCheckedPart);
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
    if (!isVisible())
        return;

    if ((Document::EditMode::Select == m_document->editMode) && !m_lastCheckedPart.isNull()) {
        const Document::Part* part = m_document->findPart(m_lastCheckedPart);
        bool partDisabled = part && part->disabled;
        emit setPartDisableState(m_lastCheckedPart, !partDisabled);
        emit groupOperationAdded();
    }
}

void SkeletonGraphicsWidget::shortcutLockOrUnlockSelectedPart()
{
    if (!isVisible())
        return;

    if ((Document::EditMode::Select == m_document->editMode) && !m_lastCheckedPart.isNull()) {
        const Document::Part* part = m_document->findPart(m_lastCheckedPart);
        bool partLocked = part && part->locked;
        emit setPartLockState(m_lastCheckedPart, !partLocked);
        emit groupOperationAdded();
    }
}

void SkeletonGraphicsWidget::shortcutXmirrorOnOrOffSelectedPart()
{
    if (!isVisible())
        return;

    if ((Document::EditMode::Select == m_document->editMode) && !m_lastCheckedPart.isNull()) {
        const Document::Part* part = m_document->findPart(m_lastCheckedPart);
        bool partXmirrored = part && part->xMirrored;
        emit setPartXmirrorState(m_lastCheckedPart, !partXmirrored);
        emit groupOperationAdded();
    }
}

void SkeletonGraphicsWidget::shortcutSubdivedOrNotSelectedPart()
{
    if (!isVisible())
        return;

    if ((Document::EditMode::Select == m_document->editMode) && !m_lastCheckedPart.isNull()) {
        const Document::Part* part = m_document->findPart(m_lastCheckedPart);
        bool partSubdived = part && part->subdived;
        emit setPartSubdivState(m_lastCheckedPart, !partSubdived);
        emit groupOperationAdded();
    }
}

void SkeletonGraphicsWidget::shortcutChamferedOrNotSelectedPart()
{
    if (!isVisible())
        return;

    if ((Document::EditMode::Select == m_document->editMode) && !m_lastCheckedPart.isNull()) {
        const Document::Part* part = m_document->findPart(m_lastCheckedPart);
        bool partChamfered = part && part->chamfered;
        emit setPartChamferState(m_lastCheckedPart, !partChamfered);
        emit groupOperationAdded();
    }
}

void SkeletonGraphicsWidget::shortcutSelectAll()
{
    if (!isVisible())
        return;

    selectAll();
}

void SkeletonGraphicsWidget::shortcutRoundEndOrNotSelectedPart()
{
    if (!isVisible())
        return;

    if ((Document::EditMode::Select == m_document->editMode) && !m_lastCheckedPart.isNull()) {
        const Document::Part* part = m_document->findPart(m_lastCheckedPart);
        bool partRounded = part && part->rounded;
        emit setPartRoundState(m_lastCheckedPart, !partRounded);
        emit groupOperationAdded();
    }
}

bool SkeletonGraphicsWidget::keyPress(QKeyEvent* event)
{
    return false;
}

bool SkeletonGraphicsWidget::keyRelease(QKeyEvent* event)
{
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

void SkeletonGraphicsWidget::nodeBoneJointStateChanged(const dust3d::Uuid& nodeId)
{
    const Document::Node* node = m_document->findNode(nodeId);
    if (nullptr == node) {
        qDebug() << "Node changed but node id not exist:" << nodeId;
        return;
    }
    auto it = nodeItemMap.find(nodeId);
    if (it == nodeItemMap.end()) {
        qDebug() << "Node not found:" << nodeId;
        return;
    }
    it->second.first->setMarkColor(node->boneJoint ? Theme::blue : Qt::transparent);
    it->second.second->setMarkColor(node->boneJoint ? Theme::blue : Qt::transparent);
}

void SkeletonGraphicsWidget::nodeAdded(dust3d::Uuid nodeId)
{
    const Document::Node* node = m_document->findNode(nodeId);
    if (nullptr == node) {
        qDebug() << "New node added but node id not exist:" << nodeId;
        return;
    }
    if (nodeItemMap.find(nodeId) != nodeItemMap.end()) {
        qDebug() << "New node added but node item already exist:" << nodeId;
        return;
    }
    SkeletonGraphicsNodeItem* mainProfileItem = new SkeletonGraphicsNodeItem(Document::Profile::Main);
    mainProfileItem->setRotated(m_rotated);
    SkeletonGraphicsNodeItem* sideProfileItem = new SkeletonGraphicsNodeItem(Document::Profile::Side);
    sideProfileItem->setRotated(m_rotated);
    mainProfileItem->setOrigin(scenePosFromUnified(QPointF(node->getX(m_rotated), node->getY(m_rotated))));
    sideProfileItem->setOrigin(scenePosFromUnified(QPointF(node->getZ(m_rotated), node->getY(m_rotated))));
    mainProfileItem->setRadius(sceneRadiusFromUnified(node->radius));
    sideProfileItem->setRadius(sceneRadiusFromUnified(node->radius));
    mainProfileItem->setMarkColor(node->boneJoint ? Theme::blue : Qt::transparent);
    sideProfileItem->setMarkColor(node->boneJoint ? Theme::blue : Qt::transparent);
    mainProfileItem->setId(nodeId);
    sideProfileItem->setId(nodeId);
    scene()->addItem(mainProfileItem);
    scene()->addItem(sideProfileItem);
    nodeItemMap[nodeId] = std::make_pair(mainProfileItem, sideProfileItem);

    if (nullptr == m_addFromNodeItem) {
        m_addFromNodeItem = mainProfileItem;
    } else {
        if (Document::Profile::Main == m_addFromNodeItem->profile()) {
            m_addFromNodeItem = mainProfileItem;
        } else {
            m_addFromNodeItem = sideProfileItem;
        }
        m_cursorEdgeItem->setEndpoints(m_addFromNodeItem, m_cursorNodeItem);
    }
}

void SkeletonGraphicsWidget::edgeReversed(dust3d::Uuid edgeId)
{
    const Document::Edge* edge = m_document->findEdge(edgeId);
    if (nullptr == edge) {
        qDebug() << "Edge changed but edge id not exist:" << edgeId;
        return;
    }
    auto edgeItemIt = edgeItemMap.find(edgeId);
    if (edgeItemIt == edgeItemMap.end()) {
        qDebug() << "Edge removed but edge id not exist:" << edgeId;
        return;
    }
    edgeItemIt->second.first->reverse();
    edgeItemIt->second.second->reverse();
}

void SkeletonGraphicsWidget::edgeNodeChanged(const dust3d::Uuid& edgeId)
{
    const Document::Edge* edge = m_document->findEdge(edgeId);
    if (nullptr == edge) {
        qDebug() << "New edge added but edge id not exist:" << edgeId;
        return;
    }
    if (edge->nodeIds.size() != 2) {
        qDebug() << "Invalid node count of edge:" << edgeId;
        return;
    }
    dust3d::Uuid fromNodeId = edge->nodeIds[0];
    dust3d::Uuid toNodeId = edge->nodeIds[1];
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
    auto edgeIt = edgeItemMap.find(edgeId);
    if (edgeIt == edgeItemMap.end()) {
        qDebug() << "Edge not found:" << toNodeId;
        return;
    }
    edgeIt->second.first->setEndpoints(fromIt->second.first, toIt->second.first);
    edgeIt->second.second->setEndpoints(fromIt->second.second, toIt->second.second);
}

void SkeletonGraphicsWidget::edgeAdded(dust3d::Uuid edgeId)
{
    const Document::Edge* edge = m_document->findEdge(edgeId);
    if (nullptr == edge) {
        qDebug() << "New edge added but edge id not exist:" << edgeId;
        return;
    }
    if (edge->nodeIds.size() != 2) {
        qDebug() << "Invalid node count of edge:" << edgeId;
        return;
    }
    dust3d::Uuid fromNodeId = edge->nodeIds[0];
    dust3d::Uuid toNodeId = edge->nodeIds[1];
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
    SkeletonGraphicsEdgeItem* mainProfileEdgeItem = new SkeletonGraphicsEdgeItem();
    mainProfileEdgeItem->setRotated(m_rotated);
    SkeletonGraphicsEdgeItem* sideProfileEdgeItem = new SkeletonGraphicsEdgeItem();
    sideProfileEdgeItem->setRotated(m_rotated);
    mainProfileEdgeItem->setId(edgeId);
    sideProfileEdgeItem->setId(edgeId);
    mainProfileEdgeItem->setEndpoints(fromIt->second.first, toIt->second.first);
    sideProfileEdgeItem->setEndpoints(fromIt->second.second, toIt->second.second);
    scene()->addItem(mainProfileEdgeItem);
    scene()->addItem(sideProfileEdgeItem);
    edgeItemMap[edgeId] = std::make_pair(mainProfileEdgeItem, sideProfileEdgeItem);
}

void SkeletonGraphicsWidget::removeItem(QGraphicsItem* item)
{
    if (m_hoveredNodeItem == item)
        m_hoveredNodeItem = nullptr;
    if (m_addFromNodeItem == item)
        m_addFromNodeItem = nullptr;
    if (m_hoveredEdgeItem == item)
        m_hoveredEdgeItem = nullptr;
    m_rangeSelectionSet.erase(item);
    scene()->removeItem(item);
}

void SkeletonGraphicsWidget::nodeRemoved(dust3d::Uuid nodeId)
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

void SkeletonGraphicsWidget::edgeRemoved(dust3d::Uuid edgeId)
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

void SkeletonGraphicsWidget::nodeRadiusChanged(dust3d::Uuid nodeId)
{
    const Document::Node* node = m_document->findNode(nodeId);
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

void SkeletonGraphicsWidget::nodeOriginChanged(dust3d::Uuid nodeId)
{
    const Document::Node* node = m_document->findNode(nodeId);
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

void SkeletonGraphicsWidget::partVisibleStateChanged(dust3d::Uuid partId)
{
    const Document::Part* part = m_document->findPart(partId);
    for (const auto& nodeId : part->nodeIds) {
        const Document::Node* node = m_document->findNode(nodeId);
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

bool SkeletonGraphicsWidget::checkSkeletonItem(QGraphicsItem* item, bool checked)
{
    if (checked) {
        if (!item->isVisible())
            return false;
    }
    if (item->data(0) == "node") {
        SkeletonGraphicsNodeItem* nodeItem = (SkeletonGraphicsNodeItem*)item;
        if (checked) {
            if (!m_document->isNodeEditable(nodeItem->id())) {
                return false;
            }
        }
        if (checked != nodeItem->checked())
            nodeItem->setChecked(checked);
        return true;
    } else if (item->data(0) == "edge") {
        SkeletonGraphicsEdgeItem* edgeItem = (SkeletonGraphicsEdgeItem*)item;
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

dust3d::Uuid SkeletonGraphicsWidget::querySkeletonItemPartId(QGraphicsItem* item)
{
    if (item->data(0) == "node") {
        SkeletonGraphicsNodeItem* nodeItem = (SkeletonGraphicsNodeItem*)item;
        const Document::Node* node = m_document->findNode(nodeItem->id());
        if (!node) {
            qDebug() << "Find node id failed:" << nodeItem->id();
            return dust3d::Uuid();
        }
        return node->partId;
    } else if (item->data(0) == "edge") {
        SkeletonGraphicsEdgeItem* edgeItem = (SkeletonGraphicsEdgeItem*)item;
        const Document::Edge* edge = m_document->findEdge(edgeItem->id());
        if (!edge) {
            qDebug() << "Find edge id failed:" << edgeItem->id();
            return dust3d::Uuid();
        }
        return edge->partId;
    }
    return dust3d::Uuid();
}

Document::Profile SkeletonGraphicsWidget::readSkeletonItemProfile(QGraphicsItem* item)
{
    if (item->data(0) == "node") {
        SkeletonGraphicsNodeItem* nodeItem = (SkeletonGraphicsNodeItem*)item;
        return nodeItem->profile();
    } else if (item->data(0) == "edge") {
        SkeletonGraphicsEdgeItem* edgeItem = (SkeletonGraphicsEdgeItem*)item;
        return edgeItem->profile();
    }
    return Document::Profile::Unknown;
}

void SkeletonGraphicsWidget::checkRangeSelection()
{
    std::set<QGraphicsItem*> newSet;
    std::set<QGraphicsItem*> deleteSet;
    std::set<QGraphicsItem*> forceDeleteSet;
    Document::Profile choosenProfile = Document::Profile::Unknown;
    if (!m_rangeSelectionSet.empty()) {
        auto it = m_rangeSelectionSet.begin();
        choosenProfile = readSkeletonItemProfile(*it);
    }
    if (m_selectionItem->isVisible()) {
        QList<QGraphicsItem*> items = scene()->items(m_selectionItem->sceneBoundingRect());
        for (auto it = items.begin(); it != items.end(); it++) {
            QGraphicsItem* item = *it;
            if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::AltModifier)) {
                checkSkeletonItem(item, false);
                forceDeleteSet.insert(item);
            } else {
                if (Document::Profile::Unknown == choosenProfile) {
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
        for (const auto& item : m_rangeSelectionSet) {
            if (newSet.find(item) == newSet.end()) {
                checkSkeletonItem(item, false);
                deleteSet.insert(item);
            }
        }
    }
    for (const auto& item : newSet) {
        m_rangeSelectionSet.insert(item);
    }
    for (const auto& item : deleteSet) {
        m_rangeSelectionSet.erase(item);
    }
    for (const auto& item : forceDeleteSet) {
        m_rangeSelectionSet.erase(item);
    }
}

void SkeletonGraphicsWidget::clearRangeSelection()
{
    for (const auto& item : m_rangeSelectionSet) {
        checkSkeletonItem(item, false);
    }
    m_rangeSelectionSet.clear();
}

void SkeletonGraphicsWidget::readMergedSkeletonNodeSetFromRangeSelection(std::set<SkeletonGraphicsNodeItem*>* nodeItemSet)
{
    for (const auto& it : m_rangeSelectionSet) {
        QGraphicsItem* item = it;
        if (item->data(0) == "node") {
            nodeItemSet->insert((SkeletonGraphicsNodeItem*)item);
        } else if (item->data(0) == "edge") {
            SkeletonGraphicsEdgeItem* edgeItem = (SkeletonGraphicsEdgeItem*)item;
            if (edgeItem->firstItem() && edgeItem->secondItem()) {
                nodeItemSet->insert(edgeItem->firstItem());
                nodeItemSet->insert(edgeItem->secondItem());
            }
        }
    }
}

void SkeletonGraphicsWidget::readSkeletonNodeAndEdgeIdSetFromRangeSelection(std::set<dust3d::Uuid>* nodeIdSet, std::set<dust3d::Uuid>* edgeIdSet)
{
    for (const auto& it : m_rangeSelectionSet) {
        QGraphicsItem* item = it;
        if (item->data(0) == "node") {
            nodeIdSet->insert(((SkeletonGraphicsNodeItem*)item)->id());
        } else if (item->data(0) == "edge") {
            if (nullptr != edgeIdSet)
                edgeIdSet->insert(((SkeletonGraphicsEdgeItem*)item)->id());
        }
    }
}

bool SkeletonGraphicsWidget::readSkeletonNodeAndAnyEdgeOfNodeFromRangeSelection(SkeletonGraphicsNodeItem** nodeItem, SkeletonGraphicsEdgeItem** edgeItem)
{
    SkeletonGraphicsNodeItem* choosenNodeItem = nullptr;
    SkeletonGraphicsEdgeItem* choosenEdgeItem = nullptr;
    for (const auto& it : m_rangeSelectionSet) {
        QGraphicsItem* item = it;
        if (item->data(0) == "node") {
            choosenNodeItem = (SkeletonGraphicsNodeItem*)item;
        } else if (item->data(0) == "edge") {
            choosenEdgeItem = (SkeletonGraphicsEdgeItem*)item;
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

void SkeletonGraphicsWidget::selectPartAllById(dust3d::Uuid partId)
{
    unselectAll();
    for (const auto& it : nodeItemMap) {
        SkeletonGraphicsNodeItem* item = it.second.first;
        const Document::Node* node = m_document->findNode(item->id());
        if (!node)
            continue;
        if (node->partId != partId)
            continue;
        addItemToRangeSelection(item);
    }
    for (const auto& it : edgeItemMap) {
        SkeletonGraphicsEdgeItem* item = it.second.first;
        const Document::Edge* edge = m_document->findEdge(item->id());
        if (!edge)
            continue;
        if (edge->partId != partId)
            continue;
        addItemToRangeSelection(item);
    }
    hoverPart(dust3d::Uuid());
}

void SkeletonGraphicsWidget::addSelectNode(dust3d::Uuid nodeId)
{
    const auto& findResult = nodeItemMap.find(nodeId);
    if (findResult == nodeItemMap.end()) {
        qDebug() << "addSelectNode failed because of node id not exists:<<" << nodeId;
        return;
    }
    addItemToRangeSelection(findResult->second.first);
    hoverPart(dust3d::Uuid());
}

void SkeletonGraphicsWidget::addSelectEdge(dust3d::Uuid edgeId)
{
    const auto& findResult = edgeItemMap.find(edgeId);
    if (findResult == edgeItemMap.end()) {
        qDebug() << "addSelectEdge failed because of edge id not exists:<<" << edgeId;
        return;
    }
    addItemToRangeSelection(findResult->second.first);
    hoverPart(dust3d::Uuid());
}

void SkeletonGraphicsWidget::addPartToSelection(dust3d::Uuid partId)
{
    Document::Profile choosenProfile = Document::Profile::Main;
    if (m_hoveredNodeItem) {
        choosenProfile = m_hoveredNodeItem->profile();
    } else if (m_hoveredEdgeItem) {
        choosenProfile = m_hoveredEdgeItem->profile();
    }
    dust3d::Uuid choosenPartId = partId;
    for (const auto& it : nodeItemMap) {
        SkeletonGraphicsNodeItem* item = Document::Profile::Main == choosenProfile ? it.second.first : it.second.second;
        const Document::Node* node = m_document->findNode(item->id());
        if (!node)
            continue;
        if (choosenPartId.isNull()) {
            choosenPartId = node->partId;
        }
        if (node->partId != choosenPartId)
            continue;
        addItemToRangeSelection(item);
    }
    for (const auto& it : edgeItemMap) {
        SkeletonGraphicsEdgeItem* item = Document::Profile::Main == choosenProfile ? it.second.first : it.second.second;
        const Document::Edge* edge = m_document->findEdge(item->id());
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

void SkeletonGraphicsWidget::addNodeToSelection(const dust3d::Uuid& nodeId)
{
    Document::Profile choosenProfile = Document::Profile::Main;
    if (m_hoveredNodeItem) {
        choosenProfile = m_hoveredNodeItem->profile();
    } else if (m_hoveredEdgeItem) {
        choosenProfile = m_hoveredEdgeItem->profile();
    }
    auto findNodeItem = nodeItemMap.find(nodeId);
    if (findNodeItem == nodeItemMap.end())
        return;
    SkeletonGraphicsNodeItem* item = Document::Profile::Main == choosenProfile ? findNodeItem->second.first : findNodeItem->second.second;
    addItemToRangeSelection(item);
}

void SkeletonGraphicsWidget::selectConnectedAll()
{
    unselectAll();
    Document::Profile choosenProfile = Document::Profile::Main;
    dust3d::Uuid startNodeId;
    if (m_hoveredNodeItem) {
        choosenProfile = m_hoveredNodeItem->profile();
        const Document::Node* node = m_document->findNode(m_hoveredNodeItem->id());
        if (node)
            startNodeId = node->id;
    } else if (m_hoveredEdgeItem) {
        choosenProfile = m_hoveredEdgeItem->profile();
        const Document::Edge* edge = m_document->findEdge(m_hoveredEdgeItem->id());
        if (edge && !edge->nodeIds.empty())
            startNodeId = *edge->nodeIds.begin();
    }
    if (startNodeId.isNull())
        return;
    std::set<dust3d::Uuid> visitedNodes;
    std::set<dust3d::Uuid> visitedEdges;
    std::queue<dust3d::Uuid> nodeIds;
    nodeIds.push(startNodeId);
    while (!nodeIds.empty()) {
        dust3d::Uuid nodeId = nodeIds.front();
        nodeIds.pop();
        if (visitedNodes.find(nodeId) != visitedNodes.end())
            continue;
        const Document::Node* node = m_document->findNode(nodeId);
        if (nullptr == node)
            continue;
        visitedNodes.insert(nodeId);
        for (const auto& edgeId : node->edgeIds) {
            const Document::Edge* edge = m_document->findEdge(edgeId);
            if (nullptr == edge)
                continue;
            visitedEdges.insert(edgeId);
            for (const auto& nodeIdOfEdge : edge->nodeIds) {
                if (visitedNodes.find(nodeIdOfEdge) != visitedNodes.end())
                    continue;
                nodeIds.push(nodeIdOfEdge);
            }
        }
    }
    for (const auto& it : nodeItemMap) {
        SkeletonGraphicsNodeItem* item = Document::Profile::Main == choosenProfile ? it.second.first : it.second.second;
        if (visitedNodes.find(item->id()) == visitedNodes.end())
            continue;
        addItemToRangeSelection(item);
    }
    for (const auto& it : edgeItemMap) {
        SkeletonGraphicsEdgeItem* item = Document::Profile::Main == choosenProfile ? it.second.first : it.second.second;
        if (visitedEdges.find(item->id()) == visitedEdges.end())
            continue;
        addItemToRangeSelection(item);
    }
}

void SkeletonGraphicsWidget::selectPartAll()
{
    unselectAll();
    Document::Profile choosenProfile = Document::Profile::Main;
    dust3d::Uuid choosenPartId;
    if (m_hoveredNodeItem) {
        choosenProfile = m_hoveredNodeItem->profile();
        const Document::Node* node = m_document->findNode(m_hoveredNodeItem->id());
        if (node)
            choosenPartId = node->partId;
    } else if (m_hoveredEdgeItem) {
        choosenProfile = m_hoveredEdgeItem->profile();
        const Document::Edge* edge = m_document->findEdge(m_hoveredEdgeItem->id());
        if (edge)
            choosenPartId = edge->partId;
    }
    for (const auto& it : nodeItemMap) {
        SkeletonGraphicsNodeItem* item = Document::Profile::Main == choosenProfile ? it.second.first : it.second.second;
        const Document::Node* node = m_document->findNode(item->id());
        if (!node)
            continue;
        if (choosenPartId.isNull()) {
            choosenPartId = node->partId;
        }
        if (node->partId != choosenPartId)
            continue;
        addItemToRangeSelection(item);
    }
    for (const auto& it : edgeItemMap) {
        SkeletonGraphicsEdgeItem* item = Document::Profile::Main == choosenProfile ? it.second.first : it.second.second;
        const Document::Edge* edge = m_document->findEdge(item->id());
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
    if (!isVisible())
        return;

    dust3d::Uuid choosenPartId;
    if (m_hoveredNodeItem) {
        const Document::Node* node = m_document->findNode(m_hoveredNodeItem->id());
        if (node)
            choosenPartId = node->partId;
    } else if (m_hoveredEdgeItem) {
        const Document::Edge* edge = m_document->findEdge(m_hoveredEdgeItem->id());
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
    Document::Profile choosenProfile = Document::Profile::Main;
    if (m_hoveredNodeItem) {
        choosenProfile = m_hoveredNodeItem->profile();
    } else if (m_hoveredEdgeItem) {
        choosenProfile = m_hoveredEdgeItem->profile();
    }
    for (const auto& it : nodeItemMap) {
        SkeletonGraphicsNodeItem* item = Document::Profile::Main == choosenProfile ? it.second.first : it.second.second;
        addItemToRangeSelection(item);
    }
    for (const auto& it : edgeItemMap) {
        SkeletonGraphicsEdgeItem* item = Document::Profile::Main == choosenProfile ? it.second.first : it.second.second;
        addItemToRangeSelection(item);
    }
}

void SkeletonGraphicsWidget::addItemToRangeSelection(QGraphicsItem* item)
{
    if (!item->isVisible())
        return;
    if (checkSkeletonItem(item, true))
        m_rangeSelectionSet.insert(item);
}

void SkeletonGraphicsWidget::removeItemFromRangeSelection(QGraphicsItem* item)
{
    checkSkeletonItem(item, false);
    m_rangeSelectionSet.erase(item);
}

void SkeletonGraphicsWidget::unselectAll()
{
    for (const auto& item : m_rangeSelectionSet) {
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
    std::set<dust3d::Uuid> nodeIdSet;
    std::set<dust3d::Uuid> edgeIdSet;
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
    std::vector<QGraphicsItem*> contentItems;
    for (const auto& it : items()) {
        QGraphicsItem* item = it;
        if (item->data(0) == "node") {
            contentItems.push_back(item);
        } else if (item->data(0) == "edge") {
            contentItems.push_back(item);
        }
    }
    for (size_t i = 0; i < contentItems.size(); i++) {
        QGraphicsItem* item = contentItems[i];
        Q_ASSERT(item != m_cursorEdgeItem);
        Q_ASSERT(item != m_cursorNodeItem);
        delete item;
    }
}

bool SkeletonGraphicsWidget::isSingleNodeSelected()
{
    if (m_rangeSelectionSet.size() != 1)
        return false;
    const auto& it = m_rangeSelectionSet.begin();
    QGraphicsItem* item = *it;
    return item->data(0) == "node";
}

void SkeletonGraphicsWidget::hoverPart(dust3d::Uuid partId)
{
    if (partId.isNull()) {
        if (!m_rangeSelectionSet.empty()) {
            const auto& it = m_rangeSelectionSet.begin();
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
    for (const auto& item : copiedSet) {
        if (item->data(0) == "node") {
            SkeletonGraphicsNodeItem* nodeItem = (SkeletonGraphicsNodeItem*)item;
            const auto& find = nodeItemMap.find(nodeItem->id());
            if (find == nodeItemMap.end()) {
                qDebug() << "Node item map key not found:" << nodeItem->id();
                return;
            }
            checkSkeletonItem(nodeItem, false);
            m_rangeSelectionSet.erase(nodeItem);
            SkeletonGraphicsNodeItem* altNodeItem = nodeItem == find->second.first ? find->second.second : find->second.first;
            if (checkSkeletonItem(altNodeItem, true))
                m_rangeSelectionSet.insert(altNodeItem);
        } else if (item->data(0) == "edge") {
            SkeletonGraphicsEdgeItem* edgeItem = (SkeletonGraphicsEdgeItem*)item;
            const auto& find = edgeItemMap.find(edgeItem->id());
            if (find == edgeItemMap.end()) {
                qDebug() << "Edge item map key not found:" << edgeItem->id();
                return;
            }
            checkSkeletonItem(edgeItem, false);
            m_rangeSelectionSet.erase(edgeItem);
            SkeletonGraphicsEdgeItem* altEdgeItem = edgeItem == find->second.first ? find->second.second : find->second.first;
            if (checkSkeletonItem(altEdgeItem, true))
                m_rangeSelectionSet.insert(altEdgeItem);
        }
    }
}

void SkeletonGraphicsWidget::setItemHoveredOnAllProfiles(QGraphicsItem* item, bool hovered)
{
    if (item->data(0) == "node") {
        SkeletonGraphicsNodeItem* nodeItem = (SkeletonGraphicsNodeItem*)item;
        const auto& find = nodeItemMap.find(nodeItem->id());
        if (find == nodeItemMap.end()) {
            qDebug() << "Node item map key not found:" << nodeItem->id();
            return;
        }
        find->second.first->setHovered(hovered);
        find->second.second->setHovered(hovered);
    } else if (item->data(0) == "edge") {
        SkeletonGraphicsEdgeItem* edgeItem = (SkeletonGraphicsEdgeItem*)item;
        const auto& find = edgeItemMap.find(edgeItem->id());
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

    if (movedUpdateVersion == m_ikMoveUpdateVersion && !m_ikMoveEndEffectorId.isNull()) {
        emit batchChangeBegin();
        for (const auto& it : m_ikMover->ikNodes()) {
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

    if (movedUpdateVersion != m_ikMoveUpdateVersion && !m_ikMoveEndEffectorId.isNull()) {
        ikMove(m_ikMoveEndEffectorId, m_ikMoveTarget);
    }
}

void SkeletonGraphicsWidget::ikMove(dust3d::Uuid endEffectorId, QVector3D target)
{
    m_ikMoveEndEffectorId = endEffectorId;
    m_ikMoveTarget = target;
    m_ikMoveUpdateVersion++;
    if (nullptr != m_ikMover) {
        return;
    }

    QThread* thread = new QThread;

    m_ikMover = new SkeletonIkMover();
    m_ikMover->setUpdateVersion(m_ikMoveUpdateVersion);
    m_ikMover->setTarget(m_ikMoveTarget);
    dust3d::Uuid nodeId = endEffectorId;
    std::set<dust3d::Uuid> historyNodeIds;
    std::vector<std::pair<dust3d::Uuid, QVector3D>> appendNodes;
    for (;;) {
        historyNodeIds.insert(nodeId);
        const auto node = m_document->findNode(nodeId);
        if (nullptr == node)
            break;
        appendNodes.push_back(std::make_pair(nodeId, QVector3D(node->getX(m_rotated), node->getY(m_rotated), node->getZ(m_rotated))));
        if (node->edgeIds.size() < 1 || node->edgeIds.size() > 2)
            break;
        dust3d::Uuid choosenNodeId;
        for (const auto& edgeId : node->edgeIds) {
            const auto edge = m_document->findEdge(edgeId);
            if (nullptr == edge)
                break;
            dust3d::Uuid neighborNodeId = edge->neighborOf(nodeId);
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

void SkeletonGraphicsWidget::setNodeBoneJointStates(const std::vector<dust3d::Uuid>& nodeIds, bool boneJoint)
{
    if (nodeIds.empty())
        return;

    emit batchChangeBegin();
    for (const auto& it : nodeIds) {
        emit setNodeBoneJointState(it, boneJoint);
    }
    emit batchChangeEnd();
    emit groupOperationAdded();
}

void SkeletonGraphicsWidget::markSelectedAsBoneJoint()
{
    std::set<SkeletonGraphicsNodeItem*> nodeItems;
    readMergedSkeletonNodeSetFromRangeSelection(&nodeItems);
    if (nodeItems.empty())
        return;

    std::vector<dust3d::Uuid> nodeIds;
    for (const auto& it : nodeItems) {
        nodeIds.push_back(it->id());
    }

    setNodeBoneJointStates(nodeIds, true);
}

void SkeletonGraphicsWidget::markSelectedAsNotBoneJoint()
{
    std::set<SkeletonGraphicsNodeItem*> nodeItems;
    readMergedSkeletonNodeSetFromRangeSelection(&nodeItems);
    if (nodeItems.empty())
        return;

    std::vector<dust3d::Uuid> nodeIds;
    for (const auto& it : nodeItems) {
        nodeIds.push_back(it->id());
    }

    setNodeBoneJointStates(nodeIds, false);
}

void SkeletonGraphicsWidget::addSelectedToBone(const dust3d::Uuid& boneId)
{
    std::set<SkeletonGraphicsNodeItem*> nodeItems;
    readMergedSkeletonNodeSetFromRangeSelection(&nodeItems);
    if (nodeItems.empty())
        return;

    std::vector<dust3d::Uuid> nodeIds;
    for (const auto& it : nodeItems) {
        nodeIds.push_back(it->id());
    }

    emit addNodesToBone(boneId, nodeIds);
}

void SkeletonGraphicsWidget::removeSelectedFromBone(const dust3d::Uuid& boneId)
{
    std::set<SkeletonGraphicsNodeItem*> nodeItems;
    readMergedSkeletonNodeSetFromRangeSelection(&nodeItems);
    if (nodeItems.empty())
        return;

    std::vector<dust3d::Uuid> nodeIds;
    for (const auto& it : nodeItems) {
        nodeIds.push_back(it->id());
    }

    emit removeNodesFromBone(boneId, nodeIds);
}
