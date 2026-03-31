#ifndef DUST3D_APPLICATION_COMPONENT_PREVIEW_GRID_WIDGET_H_
#define DUST3D_APPLICATION_COMPONENT_PREVIEW_GRID_WIDGET_H_

#include "component_list_model.h"
#include "document.h"
#include "preview_grid_view.h"
#include <QAbstractListModel>
#include <QDropEvent>
#include <dust3d/base/uuid.h>
#include <memory>

class ComponentPreviewGridWidget : public PreviewGridView {
    Q_OBJECT
signals:
    void unselectAllOnCanvas();
    void selectPartOnCanvas(const dust3d::Uuid& partId);

public:
    ComponentPreviewGridWidget(Document* document, QWidget* parent = nullptr);
    ComponentListModel* componentListModel();
    std::vector<const Document::Component*> getSelectedComponents() const;
    std::vector<dust3d::Uuid> getSelectedComponentIds() const;
    std::vector<dust3d::Uuid> getSelectedPartIds() const;

protected:
    void dropEvent(QDropEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void startDrag(Qt::DropActions supportedActions) override;

private:
    std::unique_ptr<ComponentListModel> m_componentListModel;
    Document* m_document = nullptr;
    int m_dropIndicatorRow = -1;
    bool m_isDragging = false;
};

#endif
