#include <memory>
#include "component_preview_grid_widget.h"
#include "component_list_model.h"
#include "document.h"

ComponentPreviewGridWidget::ComponentPreviewGridWidget(Document *document, QWidget *parent):
    PreviewGridView(parent),
    m_document(document)
{
    m_componentListModel = std::make_unique<ComponentListModel>(m_document);
    setModel(m_componentListModel.get());

    connect(this, &ComponentPreviewGridWidget::doubleClicked, [this](const QModelIndex &index) {
        const SkeletonComponent *component = this->componentListModel()->modelIndexToComponent(index);
        if (nullptr == component)
            return;
        if (component->childrenIds.empty()) {
            std::vector<dust3d::Uuid> partIds;
            this->m_document->collectComponentDescendantParts(component->id, partIds);
            for (const auto &partId: partIds)
                emit this->selectPartOnCanvas(partId);
        } else {
            this->componentListModel()->setListingComponentId(component->id);
        }
    });
}

ComponentListModel *ComponentPreviewGridWidget::componentListModel()
{
    return m_componentListModel.get();
}

std::vector<const SkeletonComponent *> ComponentPreviewGridWidget::getSelectedComponents() const
{
    std::vector<const SkeletonComponent *> components;
    QModelIndexList selected = selectionModel()->selectedIndexes();
    for (const auto &it: selected) {
        const auto &component = m_componentListModel->modelIndexToComponent(it);
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
    for (const auto &it: selected) {
        const auto &componentId = m_componentListModel->modelIndexToComponentId(it);
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
    for (const auto &component: selectedComponents) {
        if (component->linkToPartId.isNull())
            continue;
        partIds.push_back(component->linkToPartId);
    }
    return partIds;
}
