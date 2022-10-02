#ifndef DUST3D_APPLICATION_COMPONENT_PREVIEW_GRID_WIDGET_H_
#define DUST3D_APPLICATION_COMPONENT_PREVIEW_GRID_WIDGET_H_

#include <QAbstractListModel>
#include "preview_grid_view.h"

class ComponentListModel;
class Document;

class ComponentPreviewGridWidget: public PreviewGridView
{
public:
    ComponentPreviewGridWidget(const Document *document, QWidget *parent=nullptr);
    ComponentListModel *componentListModel();
private:
    std::unique_ptr<ComponentListModel> m_componentListModel;
    const Document *m_document = nullptr;
};

#endif
