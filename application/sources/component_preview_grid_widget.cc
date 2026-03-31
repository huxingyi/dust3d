#include "component_preview_grid_widget.h"
#include "component_list_model.h"
#include "theme.h"
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QPainter>
#include <algorithm>
#include <memory>

ComponentPreviewGridWidget::ComponentPreviewGridWidget(Document* document, QWidget* parent)
    : PreviewGridView(parent)
    , m_document(document)
{
    m_componentListModel = std::make_unique<ComponentListModel>(m_document);
    setModel(m_componentListModel.get());

    // Enable drag and drop support
    setDragEnabled(true);
    setAcceptDrops(true);
    setDefaultDropAction(Qt::DropAction::MoveAction);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDropIndicatorShown(true);

    connect(this, &ComponentPreviewGridWidget::doubleClicked, [this](const QModelIndex& index) {
        const Document::Component* component = this->componentListModel()->modelIndexToComponent(index);
        if (nullptr == component)
            return;
        if (component->childrenIds.empty()) {
            std::vector<dust3d::Uuid> partIds;
            this->m_document->collectComponentDescendantParts(component->id, partIds);
            for (const auto& partId : partIds)
                emit this->selectPartOnCanvas(partId);
        } else {
            this->componentListModel()->setListingComponentId(component->id);
        }
    });
}

ComponentListModel* ComponentPreviewGridWidget::componentListModel()
{
    return m_componentListModel.get();
}

std::vector<const Document::Component*> ComponentPreviewGridWidget::getSelectedComponents() const
{
    std::vector<const Document::Component*> components;
    QModelIndexList selected = selectionModel()->selectedIndexes();
    for (const auto& it : selected) {
        const auto& component = m_componentListModel->modelIndexToComponent(it);
        if (nullptr == component)
            continue;
        components.push_back(component);
    }
    return components;
}

std::vector<dust3d::Uuid> ComponentPreviewGridWidget::getSelectedComponentIds() const
{
    std::vector<dust3d::Uuid> componentIds;
    QModelIndexList selected = selectionModel()->selectedIndexes();
    for (const auto& it : selected) {
        const auto& componentId = m_componentListModel->modelIndexToComponentId(it);
        if (componentId.isNull())
            continue;
        componentIds.push_back(componentId);
    }
    return componentIds;
}

std::vector<dust3d::Uuid> ComponentPreviewGridWidget::getSelectedPartIds() const
{
    auto selectedComponents = getSelectedComponents();
    std::vector<dust3d::Uuid> partIds;
    for (const auto& component : selectedComponents) {
        if (component->linkToPartId.isNull())
            continue;
        partIds.push_back(component->linkToPartId);
    }
    return partIds;
}

void ComponentPreviewGridWidget::startDrag(Qt::DropActions supportedActions)
{
    // Get selected indexes
    QModelIndexList selectedIndexes = selectionModel()->selectedIndexes();
    if (selectedIndexes.isEmpty())
        return;

    // Create MIME data
    QMimeData* mimeData = model()->mimeData(selectedIndexes);
    if (!mimeData)
        return;

    // Start the drag operation
    Qt::DropAction dropAction = Qt::MoveAction;
    if (supportedActions & Qt::MoveAction && defaultDropAction() != Qt::IgnoreAction)
        dropAction = defaultDropAction();

    QDrag* drag = new QDrag(this);
    drag->setMimeData(mimeData);

    // Set a pixmap as visual feedback (use the first selected item's icon)
    QVariant decoration = model()->data(selectedIndexes.first(), Qt::DecorationRole);
    if (decoration.type() == QVariant::Pixmap) {
        drag->setPixmap(qvariant_cast<QPixmap>(decoration).scaled(QSize(Theme::partPreviewImageSize / 2, Theme::partPreviewImageSize / 2), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    drag->exec(supportedActions, dropAction);
}

void ComponentPreviewGridWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")) {
        event->acceptProposedAction();
        m_isDragging = true;
    }
    QListView::dragEnterEvent(event);
}

void ComponentPreviewGridWidget::dragMoveEvent(QDragMoveEvent* event)
{
    if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")) {
        event->acceptProposedAction();

        // Calculate the drop index
        QModelIndex dropIndex = indexAt(event->pos());
        if (!dropIndex.isValid() && m_componentListModel->rowCount() > 0) {
            m_dropIndicatorRow = m_componentListModel->rowCount();
        } else if (dropIndex.isValid()) {
            m_dropIndicatorRow = dropIndex.row();
        } else {
            m_dropIndicatorRow = -1;
        }

        // Trigger repaint to show drop hint
        viewport()->update();
    }
    QListView::dragMoveEvent(event);
}

void ComponentPreviewGridWidget::dropEvent(QDropEvent* event)
{
    if (!event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")) {
        event->ignore();
        m_isDragging = false;
        m_dropIndicatorRow = -1;
        viewport()->update();
        return;
    }

    // Get the drop position
    QModelIndex dropIndex = indexAt(event->pos());
    if (!dropIndex.isValid() && m_componentListModel->rowCount() > 0) {
        // If no item under cursor but there are items, drop at the end
        dropIndex = m_componentListModel->index(m_componentListModel->rowCount() - 1, 0);
    }
    int dropRow = dropIndex.isValid() ? dropIndex.row() : m_componentListModel->rowCount();

    // Get the source items (selected rows)
    QModelIndexList selectedIndexes = selectionModel()->selectedIndexes();
    if (selectedIndexes.isEmpty()) {
        event->ignore();
        m_isDragging = false;
        m_dropIndicatorRow = -1;
        viewport()->update();
        return;
    }

    // Sort selected indexes in ascending order
    std::sort(selectedIndexes.begin(), selectedIndexes.end(), [](const QModelIndex& a, const QModelIndex& b) {
        return a.row() < b.row();
    });

    // Get first selected row
    int firstSelectedRow = selectedIndexes.first().row();
    int lastSelectedRow = selectedIndexes.last().row();

    // Build a new order by:
    // 1. Starting with all component IDs
    // 2. Removing selected items
    // 3. Inserting them at the drop position

    const dust3d::Uuid listingComponentId = m_componentListModel->listingComponentId();
    const Document::Component* listingComponent = m_document->findComponent(listingComponentId);
    if (nullptr == listingComponent) {
        event->ignore();
        return;
    }

    // Create a vector of all component IDs in current order
    std::vector<dust3d::Uuid> allComponentIds = listingComponent->childrenIds;

    // Collect selected component IDs
    std::vector<dust3d::Uuid> selectedComponentIds;
    for (const auto& index : selectedIndexes) {
        const dust3d::Uuid componentId = m_componentListModel->modelIndexToComponentId(index);
        if (!componentId.isNull()) {
            selectedComponentIds.push_back(componentId);
        }
    }

    if (selectedComponentIds.empty()) {
        event->ignore();
        return;
    }

    // Remove selected items from the all list (in reverse to maintain indices)
    for (int i = (int)allComponentIds.size() - 1; i >= 0; --i) {
        for (const auto& selectedId : selectedComponentIds) {
            if (allComponentIds[i] == selectedId) {
                allComponentIds.erase(allComponentIds.begin() + i);
                break;
            }
        }
    }

    // Calculate insert position
    // Count how many selected items are before the drop row
    int itemsBeforeDrop = 0;
    for (const auto& index : selectedIndexes) {
        if (index.row() < dropRow) {
            itemsBeforeDrop++;
        }
    }

    // Calculate insert position based on direction
    int insertPos;
    if (dropRow > lastSelectedRow) {
        // Dropping after all selected items: account for all removed items
        insertPos = dropRow - selectedComponentIds.size() + 1;
    } else {
        // Dropping before/within: account only for items removed before dropRow
        insertPos = dropRow - itemsBeforeDrop;
    }

    // Clamp to valid range
    if (insertPos < 0)
        insertPos = 0;
    if (insertPos > (int)allComponentIds.size())
        insertPos = (int)allComponentIds.size();

    // Insert selected items at the new position
    for (int i = 0; i < (int)selectedComponentIds.size(); ++i) {
        allComponentIds.insert(allComponentIds.begin() + insertPos + i, selectedComponentIds[i]);
    }

    // Now apply this new order to the document
    for (int i = 0; i < (int)allComponentIds.size(); ++i) {
        m_document->moveComponentToIndex(allComponentIds[i], i);
    }

    m_isDragging = false;
    m_dropIndicatorRow = -1;
    viewport()->update();
    event->accept();
}

void ComponentPreviewGridWidget::dragLeaveEvent(QDragLeaveEvent* event)
{
    m_isDragging = false;
    m_dropIndicatorRow = -1;
    viewport()->update();
    QListView::dragLeaveEvent(event);
}

void ComponentPreviewGridWidget::paintEvent(QPaintEvent* event)
{
    // Call the parent paint event first
    QListView::paintEvent(event);

    // Draw drop hint indicator if dragging
    if (m_isDragging && m_dropIndicatorRow >= 0) {
        QPainter painter(viewport());

        // Get the target item rectangle
        QRect targetRect;

        if (m_dropIndicatorRow < m_componentListModel->rowCount()) {
            QModelIndex index = m_componentListModel->index(m_dropIndicatorRow, 0);
            targetRect = visualRect(index);
        } else {
            // If dropping at the end, try to estimate the position from the last item
            if (m_componentListModel->rowCount() > 0) {
                QModelIndex lastIndex = m_componentListModel->index(m_componentListModel->rowCount() - 1, 0);
                QRect lastItemRect = visualRect(lastIndex);
                targetRect = lastItemRect.translated(0, lastItemRect.height() + 5);
            }
        }

        if (targetRect.isValid()) {
            // Draw a dashed border around the target item place
            painter.setPen(QPen(Theme::red, 2, Qt::DashLine));
            painter.drawRect(targetRect);
        }
    }
}
