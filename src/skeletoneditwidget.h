#ifndef SKELETON_EDIT_WIDGET_H
#define SKELETON_EDIT_WIDGET_H
#include <QFrame>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QMouseEvent>
#include <QGraphicsEllipseItem>
#include "skeletoneditgraphicsview.h"

class SkeletonEditWidget : public QFrame
{
    Q_OBJECT
signals:
    void sizeChanged();
public:
    SkeletonEditWidget(QWidget *parent = 0);
    SkeletonEditGraphicsView *graphicsView();
protected:
    void resizeEvent(QResizeEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
private:
    SkeletonEditGraphicsView *m_graphicsView;
};

#endif
