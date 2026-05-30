#ifndef DUST3D_APPLICATION_COMPONENT_BREADCRUMB_WIDGET_H_
#define DUST3D_APPLICATION_COMPONENT_BREADCRUMB_WIDGET_H_

#include <QWidget>
#include <dust3d/base/uuid.h>
#include <vector>

class Document;

class ComponentBreadcrumbWidget : public QWidget {
    Q_OBJECT
signals:
    void navigateToComponent(const dust3d::Uuid& componentId);

public:
    ComponentBreadcrumbWidget(Document* document, QWidget* parent = nullptr);
    void setCurrentComponentId(const dust3d::Uuid& componentId);
    void updateThumbnails();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    struct BreadcrumbItem {
        dust3d::Uuid componentId;
        QRect rect;
        QPixmap thumbnail;
    };

    Document* m_document = nullptr;
    dust3d::Uuid m_currentComponentId;
    std::vector<BreadcrumbItem> m_items;
    int m_hoveredDropIndex = -1;

    void rebuildPath();
    int thumbnailSize() const;
    int itemAt(const QPoint& pos) const;
};

#endif
