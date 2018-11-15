#include "graphicscontainerwidget.h"

GraphicsContainerWidget::GraphicsContainerWidget()
{
}

void GraphicsContainerWidget::resizeEvent(QResizeEvent *event)
{
    if (m_graphicsWidget && m_graphicsWidget->size() != event->size())
        emit containerSizeChanged(event->size());
}

void GraphicsContainerWidget::mousePressEvent(QMouseEvent *event)
{
    if (m_modelWidget)
        m_modelWidget->inputMousePressEventFromOtherWidget(event);
}

void GraphicsContainerWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_modelWidget)
        m_modelWidget->inputMouseMoveEventFromOtherWidget(event);
}

void GraphicsContainerWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_modelWidget)
        m_modelWidget->inputMouseReleaseEventFromOtherWidget(event);
}

void GraphicsContainerWidget::wheelEvent(QWheelEvent *event)
{
    if (m_graphicsWidget) {
        m_graphicsWidget->inputWheelEventFromOtherWidget(event);
        return;
    }
        
    if (m_modelWidget)
        m_modelWidget->inputWheelEventFromOtherWidget(event);
}

void GraphicsContainerWidget::setGraphicsWidget(SkeletonGraphicsWidget *graphicsWidget)
{
    m_graphicsWidget = graphicsWidget;
}

void GraphicsContainerWidget::setModelWidget(ModelWidget *modelWidget)
{
    m_modelWidget = modelWidget;
}
