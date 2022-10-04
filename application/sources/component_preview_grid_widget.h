#ifndef DUST3D_APPLICATION_COMPONENT_PREVIEW_GRID_WIDGET_H_
#define DUST3D_APPLICATION_COMPONENT_PREVIEW_GRID_WIDGET_H_

#include <memory>
#include <dust3d/base/uuid.h>
#include <QAbstractListModel>
#include "preview_grid_view.h"

class ComponentListModel;
class Document;
class SkeletonComponent;

class ComponentPreviewGridWidget: public PreviewGridView
{
public:
    ComponentPreviewGridWidget(Document *document, QWidget *parent=nullptr);
    ComponentListModel *componentListModel();
    std::vector<const SkeletonComponent *> getSelectedComponents() const;
    std::vector<dust3d::Uuid> getSelectedComponentIds() const;
    std::vector<dust3d::Uuid> getSelectedPartIds() const;
private:
    std::unique_ptr<ComponentListModel> m_componentListModel;
    Document *m_document = nullptr;
};

#endif
