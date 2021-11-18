#ifndef DUST3D_APPLICATION_GRAPHICS_CONTAINER_WIDGET_H_
#define DUST3D_APPLICATION_GRAPHICS_CONTAINER_WIDGET_H_

#include <QWidget>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include "model_widget.h"
#include "skeleton_graphics_widget.h"

class GraphicsContainerWidget : public QWidget
{
    Q_OBJECT
signals:
    void containerSizeChanged(QSize size);
public:
    GraphicsContainerWidget();
    void setGraphicsWidget(SkeletonGraphicsWidget *graphicsWidget);
    void setModelWidget(ModelWidget *modelWidget);
protected:
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
private:
    ModelWidget *m_modelWidget = nullptr;
    SkeletonGraphicsWidget *m_graphicsWidget = nullptr;
};

#endif
