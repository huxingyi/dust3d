#include "skeletoneditwidget.h"
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGridLayout>
#include <QGraphicsPixmapItem>

// Modifed from http://doc.qt.io/qt-5/qtwidgets-graphicsview-chip-view-cpp.html

SkeletonEditWidget::SkeletonEditWidget(QFrame *parent)
    : QFrame(parent)
{
    //setFrameStyle(Sunken | StyledPanel);
    
    graphicsView = new QGraphicsView(this);
    graphicsView->setRenderHint(QPainter::Antialiasing, false);
    
    scene = new QGraphicsScene();
    graphicsView->setScene(scene);
    
    QImage image("../assets/male_werewolf_turnaround_lineart_by_jennette_brown.png");
    QGraphicsPixmapItem *backgroundItem = new QGraphicsPixmapItem(QPixmap::fromImage(image));
    scene->addItem(backgroundItem);
    
    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(graphicsView, 0, 0, 1, 1);
    
    setLayout(mainLayout);
}
