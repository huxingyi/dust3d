#include "component_preview_grid_widget.h"
#include "component_list_model.h"
#include "theme.h"
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
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
    }
    QListView::dragEnterEvent(event);
}

void ComponentPreviewGridWidget::dragMoveEvent(QDragMoveEvent* event)
{
    if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")) {
        event->acceptProposedAction();
    }
    QListView::dragMoveEvent(event);
}

void ComponentPreviewGridWidget::dropEvent(QDropEvent* event)
{
    if (!event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")) {
        event->ignore();
        return;
    }

    // Get the drop position
    QModelIndex dropIndex = indexAt(event->pos());
    int dropRow = dropIndex.isValid() ? dropIndex.row() : m_componentListModel->rowCount();

    // Get the source items (selected rows)
    QModelIndexList selectedIndexes = selectionModel()->selectedIndexes();
    if (selectedIndexes.isEmpty()) {
        event->ignore();
        return;
    }

    // Sort selected indexes in descending order to remove them safely
    std::sort(selectedIndexes.begin(), selectedIndexes.end(), [](const QModelIndex& a, const QModelIndex& b) {
        return a.row() > b.row();
    });

    // Collect all selected component IDs
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

    // Get the parent component
    const dust3d::Uuid listingComponentId = m_componentListModel->listingComponentId();
    const Document::Component* listingComponent = m_document->findComponent(listingComponentId);
    if (nullptr == listingComponent) {
        event->ignore();
        return;
    }

    // Calculate the target position accounting for removed items
    int targetRow = dropRow;
    for (const auto& selectedIndex : selectedIndexes) {
        if (selectedIndex.row() < dropRow) {
            targetRow--;
        }
    }

    // Clamp target row to valid range
    if (targetRow < 0)
        targetRow = 0;
    if (targetRow > (int)listingComponent->childrenIds.size())
        targetRow = listingComponent->childrenIds.size();

    // Move each selected component to the new position
    int insertOffset = 0;
    for (const auto& componentId : selectedComponentIds) {
        int targetIndex = targetRow + insertOffset;
        m_document->moveComponentToIndex(componentId, targetIndex);
        insertOffset++;
    }

    event->accept();
}
