#include "image_preview_widget.h"
#include <QPainter>

ImagePreviewWidget::ImagePreviewWidget(QWidget* parent)
    : QWidget(parent)
{
}

void ImagePreviewWidget::updateImage(const QImage& image)
{
    m_image = image;
    update();
}

void ImagePreviewWidget::updateBackgroundColor(const QColor& color)
{
    m_backgroundColor = color;
}

void ImagePreviewWidget::mousePressEvent(QMouseEvent* event)
{
    emit clicked();
}

void ImagePreviewWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    if (!m_image.isNull())
        painter.drawImage(QRect(0, 0, width(), height()), m_image, QRect(0, 0, m_image.width(), m_image.height()));
    else
        painter.fillRect(QRect(0, 0, width(), height()), QBrush(m_backgroundColor));
}
