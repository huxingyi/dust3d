#ifndef GRAPHICS_CONTAINER_WIDGET_H
#define GRAPHICS_CONTAINER_WIDGET_H
#include <QWidget>
#include <QResizeEvent>

class GraphicsContainerWidget : public QWidget
{
    Q_OBJECT
signals:
    void containerSizeChanged(QSize size);
public:
    GraphicsContainerWidget();
    void resizeEvent(QResizeEvent *event) override;
    void setGraphicsWidget(QWidget *graphicsWidget);
private:
    QWidget *m_graphicsWidget = nullptr;
};

#endif
