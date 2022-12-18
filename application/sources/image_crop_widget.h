#ifndef DUST3D_APPLICATION_IMAGE_CROP_WIDGET_H_
#define DUST3D_APPLICATION_IMAGE_CROP_WIDGET_H_

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QMouseEvent>

class ImageCropWidget : public QGraphicsView {
    Q_OBJECT

public:
    ImageCropWidget(QWidget* parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    QGraphicsScene* m_scene;
    QGraphicsPixmapItem* m_pixmapItem;
    QGraphicsRectItem* m_cropRect;
    QPointF m_startPoint;
    QPointF m_endPoint;
};

#endif
