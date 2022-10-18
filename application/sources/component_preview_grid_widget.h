#ifndef DUST3D_APPLICATION_COMPONENT_PREVIEW_GRID_WIDGET_H_
#define DUST3D_APPLICATION_COMPONENT_PREVIEW_GRID_WIDGET_H_

#include "component_list_model.h"
#include "preview_grid_view.h"
#include <QAbstractListModel>
#include <dust3d/base/uuid.h>
#include <memory>

class Document;
class SkeletonComponent;

class ComponentPreviewGridWidget : public PreviewGridView {
    Q_OBJECT
signals:
    void unselectAllOnCanvas();
    void selectPartOnCanvas(const dust3d::Uuid& partId);

public:
    ComponentPreviewGridWidget(Document* document, QWidget* parent = nullptr);
    ComponentListModel* componentListModel();
    std::vector<const SkeletonComponent*> getSelectedComponents() const;
    std::vector<dust3d::Uuid> getSelectedComponentIds() const;
    std::vector<dust3d::Uuid> getSelectedPartIds() const;

private:
    std::unique_ptr<ComponentListModel> m_componentListModel;
    Document* m_document = nullptr;
};

#endif
