#ifndef DUST3D_APPLICATION_IMAGE_PREVIEW_WIDGET_H_
#define DUST3D_APPLICATION_IMAGE_PREVIEW_WIDGET_H_

#include <QWidget>
#include <QImage>
#include <QPaintEvent>
#include <QMouseEvent>

class ImagePreviewWidget : public QWidget
{
    Q_OBJECT
signals:
    void clicked();
public:
    ImagePreviewWidget(QWidget *parent=nullptr);
    void updateImage(const QImage &image);
    void updateBackgroundColor(const QColor &color);
    
protected:
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    
private:
    QImage m_image;
    QColor m_backgroundColor;
    
    void resizeImage(QImage *image, const QSize &newSize);
};

#endif
