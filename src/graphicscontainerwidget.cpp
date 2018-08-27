#include "graphicscontainerwidget.h"

GraphicsContainerWidget::GraphicsContainerWidget() :
    m_graphicsWidget(nullptr)
{
}

void GraphicsContainerWidget::resizeEvent(QResizeEvent *event)
{
    if (m_graphicsWidget && m_graphicsWidget->size() != event->size())
        emit containerSizeChanged(event->size());
}

void GraphicsContainerWidget::setGraphicsWidget(QWidget *graphicsWidget)
{
    m_graphicsWidget = graphicsWidget;
}
