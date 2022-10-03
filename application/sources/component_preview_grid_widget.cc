#include <memory>
#include "component_preview_grid_widget.h"
#include "component_list_model.h"
#include "document.h"

ComponentPreviewGridWidget::ComponentPreviewGridWidget(const Document *document, QWidget *parent):
    PreviewGridView(parent),
    m_document(document)
{
    m_componentListModel = std::make_unique<ComponentListModel>(m_document);
    setModel(m_componentListModel.get());

    connect(this, &ComponentPreviewGridWidget::doubleClicked, [this](const QModelIndex &index) {
        const SkeletonComponent *component = this->componentListModel()->modelIndexToComponent(index);
        if (nullptr != component && !component->childrenIds.empty()) {
            this->componentListModel()->setListingComponentId(component->id);
        }
    });
}

ComponentListModel *ComponentPreviewGridWidget::componentListModel()
{
    return m_componentListModel.get();
}
