#ifndef SKELETON_EDIT_WIDGET_H
#define SKELETON_EDIT_WIDGET_H
#include <QFrame>
#include <QGraphicsView>
#include <QGraphicsScene>

class SkeletonEditWidget : public QFrame
{
    Q_OBJECT
public:
    SkeletonEditWidget(QFrame *parent = 0);
private:
    QGraphicsView *graphicsView;
    QGraphicsScene *scene;
};

#endif
