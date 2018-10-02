#include <QGraphicsScene>
#include <QBrush>
#include <QGraphicsPathItem>
#include <QMenu>
#include <QDebug>
#include <QLabel>
#include "interpolationgraphicswidget.h"
#include "theme.h"

const float InterpolationGraphicsWidget::m_anchorRadius = 5;
const float InterpolationGraphicsWidget::m_handleRadius = 7;
const int InterpolationGraphicsWidget::m_gridColumns = 20;
const int InterpolationGraphicsWidget::m_gridRows = 10;
const int InterpolationGraphicsWidget::m_sceneWidth = 640;
const int InterpolationGraphicsWidget::m_sceneHeight = 480;
const float InterpolationGraphicsWidget::m_tangentMagnitudeScaleFactor = 9;

InterpolationGraphicsWidget::InterpolationGraphicsWidget(QWidget *parent) :
    QGraphicsView(parent)
{
    setRenderHint(QPainter::Antialiasing);
    
    setObjectName("interpolationGraphics");
    setStyleSheet("#interpolationGraphics {background: transparent}");
    
    setScene(new QGraphicsScene());
    scene()->setSceneRect(QRectF(0, 0, InterpolationGraphicsWidget::m_sceneWidth, InterpolationGraphicsWidget::m_sceneHeight));
    
    QColor curveColor = Theme::green;
    QColor gridColor = QColor(0x19, 0x19, 0x19);
    
    QPen curvePen;
    curvePen.setColor(curveColor);
    curvePen.setWidth(3);
    
    QPen gridPen;
    gridPen.setColor(gridColor);
    gridPen.setWidth(0);
    
    m_cursorItem = new InterpolationGraphicsCursorItem;
    m_cursorItem->setFlag(QGraphicsItem::ItemSendsScenePositionChanges, !m_previewOnly);
    m_cursorItem->setRect(0, 0, 0, InterpolationGraphicsWidget::m_sceneHeight);
    m_cursorItem->setFlags(m_cursorItem->flags() & ~(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable));
    m_cursorItem->setZValue(-1);
    scene()->addItem(m_cursorItem);
    
    if (m_gridEnabled) {
        m_gridRowLineItems.resize(m_gridRows + 1);
        float rowHeight = scene()->sceneRect().height() / m_gridRows;
        for (int row = 0; row <= m_gridRows; row++) {
            m_gridRowLineItems[row] = new QGraphicsLineItem();
            m_gridRowLineItems[row]->setFlags(m_gridRowLineItems[row]->flags() & ~(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable));
            m_gridRowLineItems[row]->setPen(gridPen);
            float y = scene()->sceneRect().top() + row * rowHeight;
            m_gridRowLineItems[row]->setLine(scene()->sceneRect().left(), y, scene()->sceneRect().right(), y);
            scene()->addItem(m_gridRowLineItems[row]);
        }
        m_gridColumnLineItems.resize(m_gridColumns + 1);
        float columnWidth = scene()->sceneRect().width() / m_gridColumns;
        for (int column = 0; column <= m_gridColumns; column++) {
            m_gridColumnLineItems[column] = new QGraphicsLineItem();
            m_gridColumnLineItems[column]->setFlags(m_gridColumnLineItems[column]->flags() & ~(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable));
            m_gridColumnLineItems[column]->setPen(gridPen);
            float x = scene()->sceneRect().left() + column * columnWidth;
            m_gridColumnLineItems[column]->setLine(x, scene()->sceneRect().top(), x, scene()->sceneRect().bottom());
            scene()->addItem(m_gridColumnLineItems[column]);
        }
    }
    
    m_pathItem = new QGraphicsPathItem;
    m_pathItem->setPen(curvePen);
    scene()->addItem(m_pathItem);
}

void InterpolationGraphicsWidget::toNormalizedControlNodes(std::vector<HermiteControlNode> &snapshot)
{
    snapshot = m_controlNodes;
    for (auto &item: snapshot) {
        item.position.setX(item.position.x() / scene()->sceneRect().width());
        item.position.setY(item.position.y() / scene()->sceneRect().height());
        item.inTangent.setX(item.inTangent.x() / scene()->sceneRect().width());
        item.inTangent.setY(item.inTangent.y() / scene()->sceneRect().height());
        item.outTangent.setX(item.outTangent.x() / scene()->sceneRect().width());
        item.outTangent.setY(item.outTangent.y() / scene()->sceneRect().height());
        item.inTangent *= InterpolationGraphicsWidget::m_tangentMagnitudeScaleFactor;
        item.outTangent *= InterpolationGraphicsWidget::m_tangentMagnitudeScaleFactor;
    }
}

void InterpolationGraphicsWidget::fromNormalizedControlNodes(const std::vector<HermiteControlNode> &snapshot)
{
    m_controlNodes = snapshot;
    for (auto &item: m_controlNodes) {
        item.position.setX(item.position.x() * scene()->sceneRect().width());
        item.position.setY(item.position.y() * scene()->sceneRect().height());
        item.inTangent.setX(item.inTangent.x() * scene()->sceneRect().width());
        item.inTangent.setY(item.inTangent.y() * scene()->sceneRect().height());
        item.outTangent.setX(item.outTangent.x() * scene()->sceneRect().width());
        item.outTangent.setY(item.outTangent.y() * scene()->sceneRect().height());
        item.inTangent /= InterpolationGraphicsWidget::m_tangentMagnitudeScaleFactor;
        item.outTangent /= InterpolationGraphicsWidget::m_tangentMagnitudeScaleFactor;
    }
    refresh();
}

void InterpolationGraphicsWidget::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    if (scene())
        fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
}

void InterpolationGraphicsWidget::setControlNodes(const std::vector<HermiteControlNode> &nodes)
{
    invalidateControlSelection();
    fromNormalizedControlNodes(nodes);
    refresh();
}

void InterpolationGraphicsWidget::setKeyframes(const std::vector<std::pair<float, QString>> &keyframes)
{
    invalidateKeyframeSelection();
    m_keyframes = keyframes;
    refresh();
}

void InterpolationGraphicsWidget::refreshControlAnchor(int index)
{
    if (index >= (int)m_controlNodes.size())
        return;
    
    if (nullptr == m_anchorItems[index]) {
        m_anchorItems[index] = new InterpolationGraphicsCircleItem(m_anchorRadius);
        m_anchorItems[index]->setFlag(QGraphicsItem::ItemIsMovable, !m_previewOnly);
        m_anchorItems[index]->setFlag(QGraphicsItem::ItemSendsScenePositionChanges, !m_previewOnly);
        connect(m_anchorItems[index]->proxy(), &InterpolationGraphicsProxyObject::itemMoved, this, [=](QPointF point) {
            QVector2D newControlPosition = QVector2D(point.x(), point.y());
            if (index - 1 >= 0 && newControlPosition.x() < m_controlNodes[index - 1].position.x())
                newControlPosition.setX(m_controlNodes[index - 1].position.x());
            else if (index + 1 <= (int)m_controlNodes.size() - 1 && newControlPosition.x() > m_controlNodes[index + 1].position.x())
                newControlPosition.setX(m_controlNodes[index + 1].position.x());
            m_controlNodes[index].position = newControlPosition;
            refreshCurve();
            refreshControlNode(index);
            refreshKeyframes();
            emit controlNodesChanged();
        });
        connect(m_anchorItems[index]->proxy(), &InterpolationGraphicsProxyObject::itemHoverEnter, this, [=]() {
            hoverControlNode(index);
        });
        connect(m_anchorItems[index]->proxy(), &InterpolationGraphicsProxyObject::itemHoverLeave, this, [=]() {
            unhoverControlNode(index);
        });
        scene()->addItem(m_anchorItems[index]);
    }
    
    const auto &controlNode = m_controlNodes[index];
    m_anchorItems[index]->setOrigin(QPointF(controlNode.position.x(), controlNode.position.y()));
}

void InterpolationGraphicsWidget::refreshControlEdges(int index)
{
    if (index >= (int)m_controlNodes.size())
        return;
    
    if (nullptr == m_lineItems[index].first) {
        Q_ASSERT(nullptr == m_lineItems[index].second);
        m_lineItems[index] = std::make_pair(new InterpolationGraphicsEdgeItem(),
            new InterpolationGraphicsEdgeItem());
        scene()->addItem(m_lineItems[index].first);
        scene()->addItem(m_lineItems[index].second);
    }
    
    const auto &controlNode = m_controlNodes[index];
    
    QVector2D inHandlePosition = controlNode.position + controlNode.inTangent;
    m_lineItems[index].first->setLine(controlNode.position.x(), controlNode.position.y(),
        inHandlePosition.x(), inHandlePosition.y());
    
    QVector2D outHandlePosition = controlNode.position - controlNode.outTangent;
    m_lineItems[index].second->setLine(controlNode.position.x(), controlNode.position.y(),
        outHandlePosition.x(), outHandlePosition.y());
}

void InterpolationGraphicsWidget::refreshControlHandles(int index)
{
    if (index >= (int)m_controlNodes.size())
        return;
    
    if (nullptr == m_handleItems[index].first) {
        Q_ASSERT(nullptr == m_handleItems[index].second);
        m_handleItems[index] = std::make_pair(new InterpolationGraphicsCircleItem(m_handleRadius, false),
            new InterpolationGraphicsCircleItem(m_handleRadius, false));
        m_handleItems[index].first->setFlag(QGraphicsItem::ItemIsMovable, !m_previewOnly);
        m_handleItems[index].first->setFlag(QGraphicsItem::ItemSendsScenePositionChanges, !m_previewOnly);
        m_handleItems[index].second->setFlag(QGraphicsItem::ItemIsMovable, !m_previewOnly);
        m_handleItems[index].second->setFlag(QGraphicsItem::ItemSendsScenePositionChanges, !m_previewOnly);
        connect(m_handleItems[index].first->proxy(), &InterpolationGraphicsProxyObject::itemMoved, this, [=](QPointF point) {
            auto newHandlePosition = QVector2D(point.x(), point.y());
            m_controlNodes[index].inTangent = newHandlePosition - m_controlNodes[index].position;
            refreshCurve();
            refreshControlEdges(index);
            refreshKeyframes();
            emit controlNodesChanged();
        });
        connect(m_handleItems[index].second->proxy(), &InterpolationGraphicsProxyObject::itemMoved, this, [=](QPointF point) {
            auto newHandlePosition = QVector2D(point.x(), point.y());
            m_controlNodes[index].outTangent = m_controlNodes[index].position - newHandlePosition;
            refreshCurve();
            refreshControlEdges(index);
            refreshKeyframes();
            emit controlNodesChanged();
        });
        connect(m_handleItems[index].first->proxy(), &InterpolationGraphicsProxyObject::itemHoverEnter, this, [=]() {
            hoverControlNode(index);
        });
        connect(m_handleItems[index].second->proxy(), &InterpolationGraphicsProxyObject::itemHoverEnter, this, [=]() {
            hoverControlNode(index);
        });
        connect(m_handleItems[index].first->proxy(), &InterpolationGraphicsProxyObject::itemHoverLeave, this, [=]() {
            unhoverControlNode(index);
        });
        connect(m_handleItems[index].second->proxy(), &InterpolationGraphicsProxyObject::itemHoverLeave, this, [=]() {
            unhoverControlNode(index);
        });
        scene()->addItem(m_handleItems[index].first);
        scene()->addItem(m_handleItems[index].second);
    }
    
    const auto &controlNode = m_controlNodes[index];
    
    QVector2D inHandlePosition = controlNode.position + controlNode.inTangent;
    m_handleItems[index].first->setOrigin(QPointF(inHandlePosition.x(), inHandlePosition.y()));
    
    QVector2D outHandlePosition = controlNode.position - controlNode.outTangent;
    m_handleItems[index].second->setOrigin(QPointF(outHandlePosition.x(), outHandlePosition.y()));
}

void InterpolationGraphicsWidget::refreshControlNode(int index)
{
    if (index >= (int)m_controlNodes.size())
        return;
    
    refreshControlAnchor(index);
    refreshControlEdges(index);
    refreshControlHandles(index);
}

void InterpolationGraphicsWidget::refreshCurve()
{
    QPainterPath path;
    std::vector<HermiteControlNode> controlNodes;
    toNormalizedControlNodes(controlNodes);
    for (auto &item: controlNodes) {
        item.position.setX(item.position.x() * scene()->sceneRect().width());
        item.position.setY(item.position.y() * scene()->sceneRect().height());
        item.inTangent.setX(item.inTangent.x() * scene()->sceneRect().width());
        item.inTangent.setY(item.inTangent.y() * scene()->sceneRect().height());
        item.outTangent.setX(item.outTangent.x() * scene()->sceneRect().width());
        item.outTangent.setY(item.outTangent.y() * scene()->sceneRect().height());
    }
    hermiteCurveToPainterPath(controlNodes, path);
    m_pathItem->setPath(path);
}

void InterpolationGraphicsWidget::refreshKeyframeKnot(int index)
{
    if (index >= (int)m_keyframes.size())
        return;
    
    std::vector<HermiteControlNode> controlNodes;
    toNormalizedControlNodes(controlNodes);
    if (controlNodes.size() < 2)
        return;
    
    if (scene()->sceneRect().width() <= 0)
        return;
    
    const auto &knot = m_keyframes[index].first;
    QVector2D interpolatedPosition = calculateHermiteInterpolation(controlNodes, knot);
    
    if (nullptr == m_keyframeKnotItems[index]) {
        m_keyframeKnotItems[index] = new InterpolationGraphicsKeyframeItem;
        scene()->addItem(m_keyframeKnotItems[index]);
    }
    
    QPointF knotPos = QPointF(interpolatedPosition.x() * scene()->sceneRect().width(),
        interpolatedPosition.y() * scene()->sceneRect().height());
    m_keyframeKnotItems[index]->setPos(knotPos);
}

void InterpolationGraphicsWidget::refreshKeyframeLabel(int index)
{
    if (index >= (int)m_keyframes.size())
        return;
    
    std::vector<HermiteControlNode> controlNodes;
    toNormalizedControlNodes(controlNodes);
    if (controlNodes.size() < 2)
        return;
    
    if (scene()->sceneRect().width() <= 0)
        return;
    
    const auto &knot = m_keyframes[index].first;
    QVector2D interpolatedPosition = calculateHermiteInterpolation(controlNodes, knot);
    
    if (nullptr == m_keyframeNameItems[index]) {
        InterpolationGraphicsLabelParentWidget *parentWidget = new InterpolationGraphicsLabelParentWidget;
        parentWidget->setFlag(QGraphicsItem::ItemIsMovable, !m_previewOnly);
        parentWidget->setFlag(QGraphicsItem::ItemSendsScenePositionChanges, !m_previewOnly);
        parentWidget->setAutoFillBackground(true);
        parentWidget->setZValue(1);
        scene()->addItem(parentWidget);
  
        QLabel *nameLabel = new QLabel;
        nameLabel->setFocusPolicy(Qt::NoFocus);
        m_keyframeNameItems[index] = scene()->addWidget(nameLabel);
        m_keyframeNameItems[index]->setParentItem(parentWidget);
        connect(parentWidget->proxy(), &InterpolationGraphicsProxyObject::itemMoved, this, [=](QPointF point) {
            QVector2D newKnotPosition = QVector2D(point.x(), point.y());
            float knot = point.x() / scene()->sceneRect().width();
            if (index - 1 >= 0 && knot < m_keyframes[index - 1].first)
                newKnotPosition.setX(m_keyframes[index - 1].first * scene()->sceneRect().width());
            else if (index + 1 <= (int)m_keyframes.size() - 1 && knot > m_keyframes[index + 1].first)
                newKnotPosition.setX(m_keyframes[index + 1].first * scene()->sceneRect().width());
            knot = newKnotPosition.x() / scene()->sceneRect().width();
            m_keyframes[index].first = knot;
            refreshKeyframe(index);
            emit keyframeKnotChanged(index, knot);
        });
        connect(parentWidget->proxy(), &InterpolationGraphicsProxyObject::itemHoverEnter, this, [=]() {
            hoverKeyframe(index);
        });
        connect(parentWidget->proxy(), &InterpolationGraphicsProxyObject::itemHoverLeave, this, [=]() {
            unhoverKeyframe(index);
        });
    }
    
    QPointF knotPos = QPointF(interpolatedPosition.x() * scene()->sceneRect().width(),
        interpolatedPosition.y() * scene()->sceneRect().height());
    
    QLabel *nameLabel = (QLabel *)m_keyframeNameItems[index]->widget();
    nameLabel->setText(m_keyframes[index].second);
    nameLabel->adjustSize();
    QPointF textPos(knotPos.x(), knotPos.y());
    InterpolationGraphicsLabelParentWidget *parentWidget = (InterpolationGraphicsLabelParentWidget *)(m_keyframeNameItems[index]->parentItem());
    QSize sizeHint = nameLabel->sizeHint();
    nameLabel->resize(sizeHint);
    parentWidget->resize(sizeHint);
    parentWidget->setPosWithoutEmitSignal(textPos);
}

void InterpolationGraphicsWidget::refreshKeyframe(int index)
{
    refreshKeyframeKnot(index);
    refreshKeyframeLabel(index);
}

void InterpolationGraphicsWidget::refresh()
{
    refreshCurve();

    for (size_t i = m_controlNodes.size(); i < m_anchorItems.size(); i++)
        delete m_anchorItems[i];
    m_anchorItems.resize(m_controlNodes.size());
    
    for (size_t i = m_controlNodes.size(); i < m_lineItems.size(); i++) {
        delete m_lineItems[i].first;
        delete m_lineItems[i].second;
    }
    m_lineItems.resize(m_controlNodes.size());
    
    for (size_t i = m_controlNodes.size(); i < m_handleItems.size(); i++) {
        delete m_handleItems[i].first;
        delete m_handleItems[i].second;
    }
    m_handleItems.resize(m_controlNodes.size());
    
    for (size_t i = 0; i < m_controlNodes.size(); i++)
        refreshControlNode(i);
    
    for (size_t i = m_keyframes.size(); i < m_keyframeKnotItems.size(); i++) {
        delete m_keyframeKnotItems[i];
        delete m_keyframeNameItems[i]->parentItem();
    }
    m_keyframeKnotItems.resize(m_keyframes.size());
    m_keyframeNameItems.resize(m_keyframes.size());
    
    refreshKeyframes();
    
    fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
}

float InterpolationGraphicsWidget::cursorKnot() const
{
    return m_cursorItem->pos().x() / scene()->sceneRect().width();
}

void InterpolationGraphicsWidget::refreshKeyframes()
{
    for (size_t i = 0; i < m_keyframes.size(); i++)
        refreshKeyframe(i);
}

void InterpolationGraphicsWidget::mouseMoveEvent(QMouseEvent *event)
{
    QGraphicsView::mouseMoveEvent(event);
}

void InterpolationGraphicsWidget::mouseReleaseEvent(QMouseEvent *event)
{
    QGraphicsView::mouseReleaseEvent(event);
}

void InterpolationGraphicsWidget::mousePressEvent(QMouseEvent *event)
{
    QGraphicsView::mousePressEvent(event);
    
    if (m_previewOnly)
        return;
    
    if (event->button() == Qt::LeftButton) {
        auto cursorPos = mapToScene(event->pos());
        cursorPos.setY(0);
        m_cursorItem->updateCursorPosition(cursorPos);
    }
    
    if (event->button() == Qt::LeftButton ||
            event->button() == Qt::RightButton) {
        selectControlNode(m_currentHoveringControlIndex);
        selectKeyframe(m_currentHoveringKeyframeIndex);
    }
    
    if (event->button() == Qt::RightButton) {
        showContextMenu(mapFromGlobal(event->globalPos()));
    }
}

void InterpolationGraphicsWidget::deleteControlNode(int index)
{
    if (index >= (int)m_controlNodes.size())
        return;
    
    invalidateControlSelection();
    
    m_controlNodes.erase(m_controlNodes.begin() + index);
    refresh();
    
    emit controlNodesChanged();
}

void InterpolationGraphicsWidget::deleteKeyframe(int index)
{
    if (index >= (int)m_keyframes.size())
        return;
    
    emit removeKeyframe(index);
}

void InterpolationGraphicsWidget::addControlNodeAfter(int index)
{
    if (index >= (int)m_controlNodes.size())
        return;
    
    invalidateControlSelection();
    
    const auto &current = m_controlNodes[index];
    int newX = current.position.x();
    auto inTangent = current.outTangent;
    if (index + 1 >= (int)m_controlNodes.size()) {
        newX = (current.position.x() + scene()->sceneRect().right()) / 2;
    } else {
        const auto &next = m_controlNodes[index + 1];
        newX = (current.position.x() + next.position.x()) / 2;
        inTangent = (current.outTangent + next.inTangent) / 2;
    }
    auto outTangent = inTangent;
    
    HermiteControlNode newNode(QVector2D(newX, current.position.y()), inTangent, outTangent);
    m_controlNodes.insert(m_controlNodes.begin() + index + 1, newNode);
    refresh();
    
    emit controlNodesChanged();
}

void InterpolationGraphicsWidget::addControlNodeBefore(int index)
{
    if (index >= (int)m_controlNodes.size())
        return;
    
    invalidateControlSelection();
    
    const auto &current = m_controlNodes[index];
    int newX = current.position.x();
    auto inTangent = current.outTangent;
    if (index - 1 < 0) {
        newX = (current.position.x() + scene()->sceneRect().left()) / 2;
    } else {
        const auto &previous= m_controlNodes[index - 1];
        newX = (current.position.x() + previous.position.x()) / 2;
        inTangent = (current.inTangent + previous.outTangent) / 2;
    }
    auto outTangent = inTangent;
    
    HermiteControlNode newNode(QVector2D(newX, current.position.y()), inTangent, outTangent);
    m_controlNodes.insert(m_controlNodes.begin() + index, newNode);
    refresh();
    
    emit controlNodesChanged();
}

void InterpolationGraphicsWidget::invalidateControlSelection()
{
    m_currentHoveringControlIndex = -1;
    if (-1 != m_currentSelectedControlIndex)
        selectControlNode(-1);
}

void InterpolationGraphicsWidget::invalidateKeyframeSelection()
{
    m_currentHoveringKeyframeIndex = -1;
    if (-1 != m_currentSelectedKeyframeIndex)
        selectKeyframe(-1);
}

void InterpolationGraphicsWidget::addControlNodeAtPosition(const QPointF &pos)
{
    auto inTangent = QVector2D(0, 50);
    auto outTangent = inTangent;
    HermiteControlNode newNode(QVector2D(pos.x(), pos.y()), inTangent, outTangent);
    
    invalidateControlSelection();
    
    m_controlNodes.insert(m_controlNodes.end(), newNode);
    refresh();
    
    emit controlNodesChanged();
}

void InterpolationGraphicsWidget::hoverControlNode(int index)
{
    if (index >= (int)m_controlNodes.size())
        return;
    
    if (m_currentHoveringControlIndex == index)
        return;
    
    m_currentHoveringControlIndex = index;
}

void InterpolationGraphicsWidget::unhoverControlNode(int index)
{
    if (index >= (int)m_controlNodes.size())
        return;
    
    if (m_currentHoveringControlIndex == index) {
        m_currentHoveringControlIndex = -1;
    }
}

void InterpolationGraphicsWidget::hoverKeyframe(int index)
{
    if (index >= (int)m_keyframes.size())
        return;
    
    if (m_currentHoveringKeyframeIndex == index)
        return;
    
    m_currentHoveringKeyframeIndex = index;
    
    QLabel *nameLabel = (QLabel *)m_keyframeNameItems[m_currentHoveringKeyframeIndex]->widget();
    nameLabel->setStyleSheet("QLabel {color : " + Theme::red.name() + ";}");
}

void InterpolationGraphicsWidget::unhoverKeyframe(int index)
{
    if (index >= (int)m_keyframes.size())
        return;
    
    if (m_currentHoveringKeyframeIndex == index) {
        if (m_currentSelectedKeyframeIndex != m_currentHoveringKeyframeIndex) {
            QLabel *nameLabel = (QLabel *)m_keyframeNameItems[m_currentHoveringKeyframeIndex]->widget();
            nameLabel->setStyleSheet("QLabel {color : " + Theme::white.name() + ";}");
        }
        m_currentHoveringKeyframeIndex = -1;
    }
}

void InterpolationGraphicsWidget::selectKeyframe(int index)
{
    if (m_currentSelectedKeyframeIndex == index)
        return;
    
    if (-1 != m_currentSelectedKeyframeIndex && m_currentSelectedKeyframeIndex < (int)m_keyframes.size()) {
        QLabel *nameLabel = (QLabel *)m_keyframeNameItems[m_currentSelectedKeyframeIndex]->widget();
        nameLabel->setStyleSheet("QLabel {color : " + Theme::white.name() + ";}");
    }
    
    m_currentSelectedKeyframeIndex = index;
    
    if (-1 != m_currentSelectedKeyframeIndex && m_currentSelectedKeyframeIndex < (int)m_keyframes.size()) {
        QLabel *nameLabel = (QLabel *)m_keyframeNameItems[m_currentSelectedKeyframeIndex]->widget();
        nameLabel->setStyleSheet("QLabel {color : " + Theme::red.name() + ";}");
    }
}

void InterpolationGraphicsWidget::selectControlNode(int index)
{
    if (m_currentSelectedControlIndex == index)
        return;
    
    if (-1 != m_currentSelectedControlIndex && m_currentSelectedControlIndex < (int)m_controlNodes.size()) {
        m_anchorItems[m_currentSelectedControlIndex]->setChecked(false);
        m_handleItems[m_currentSelectedControlIndex].first->setChecked(false);
        m_handleItems[m_currentSelectedControlIndex].second->setChecked(false);
    }
    
    m_currentSelectedControlIndex = index;
    
    if (-1 != m_currentSelectedControlIndex && m_currentSelectedControlIndex < (int)m_controlNodes.size()) {
        m_anchorItems[m_currentSelectedControlIndex]->setChecked(true);
        m_handleItems[m_currentSelectedControlIndex].first->setChecked(true);
        m_handleItems[m_currentSelectedControlIndex].second->setChecked(true);
    }
}

void InterpolationGraphicsWidget::showContextMenu(const QPoint &pos)
{
    if (m_previewOnly)
        return;
    
    QMenu contextMenu(this);
    
    QAction addAction(tr("Add Control Node"), this);
    QAction addAfterAction(tr("Add Control Node After"), this);
    QAction addBeforeAction(tr("Add Control Node Before"), this);
    if (-1 != m_currentSelectedControlIndex) {
        connect(&addAfterAction, &QAction::triggered, this, [=]() {
            addControlNodeAfter(m_currentSelectedControlIndex);
        });
        contextMenu.addAction(&addAfterAction);
        
        connect(&addBeforeAction, &QAction::triggered, this, [=]() {
            addControlNodeBefore(m_currentSelectedControlIndex);
        });
        contextMenu.addAction(&addBeforeAction);
    } else if (m_controlNodes.empty()) {
        connect(&addAction, &QAction::triggered, this, [=]() {
            addControlNodeAtPosition(mapToScene(pos));
        });
        contextMenu.addAction(&addAction);
    }
    
    QAction deleteControlNodeAction(tr("Delete Control Node"), this);
    if (-1 != m_currentSelectedControlIndex) {
        connect(&deleteControlNodeAction, &QAction::triggered, this, [=]() {
            deleteControlNode(m_currentSelectedControlIndex);
        });
        contextMenu.addAction(&deleteControlNodeAction);
    }
    
    QAction deleteKeyframeAction(tr("Delete Keyframe"), this);
    if (-1 != m_currentSelectedKeyframeIndex) {
        connect(&deleteKeyframeAction, &QAction::triggered, this, [=]() {
            deleteKeyframe(m_currentSelectedKeyframeIndex);
        });
        contextMenu.addAction(&deleteKeyframeAction);
    }
    
    contextMenu.exec(mapToGlobal(pos));
}

void InterpolationGraphicsWidget::setPreviewOnly(bool previewOnly)
{
    if (m_previewOnly == previewOnly)
        return;
    m_previewOnly = previewOnly;
    m_cursorItem->setVisible(!m_previewOnly);
}
