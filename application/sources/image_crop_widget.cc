#include "image_crop_widget.h"

ImageCropWidget::ImageCropWidget(QWidget* parent)
    : QGraphicsView(parent)
{
    m_scene = new QGraphicsScene;
    // Load the image and add it to the scene
    //QPixmap pixmap("image.jpg");
    //m_pixmapItem = m_scene->addPixmap(pixmap);

    // Create the crop rectangle and add it to the scene
    m_cropRect = new QGraphicsRectItem();
    m_scene->addItem(m_cropRect);
    m_cropRect->setVisible(true);

    // Set up the view
    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setBackgroundBrush(QBrush(QColor(230, 230, 230)));
    setFrameShape(QFrame::NoFrame);
}

void ImageCropWidget::mousePressEvent(QMouseEvent* event)
{
    // Start drawing the crop rectangle from the mouse press point
    m_startPoint = event->pos();
    m_endPoint = event->pos();
    m_cropRect->setRect(QRectF(m_startPoint, m_endPoint));
    m_cropRect->setVisible(true);
}

void ImageCropWidget::mouseMoveEvent(QMouseEvent* event)
{
    // Update the end point of the crop rectangle as the mouse is moved
    m_endPoint = event->pos();
    m_cropRect->setRect(QRectF(m_startPoint, m_endPoint));
}

void ImageCropWidget::mouseReleaseEvent(QMouseEvent* event)
{
    // Crop the image and hide the crop rectangle when the mouse is released
    /*
    QRectF rect(m_startPoint, m_endPoint);
    QPixmap cropped = m_pixmapItem->pixmap().copy(rect.toRect());
    m_pixmapItem->setPixmap(cropped);
    m_cropRect->setVisible(false);
    */
}
