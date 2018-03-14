#include "skeletoneditwidget.h"
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGridLayout>
#include <QGraphicsPixmapItem>

// Modifed from http://doc.qt.io/qt-5/qtwidgets-graphicsview-chip-view-cpp.html

SkeletonEditWidget::SkeletonEditWidget(QWidget *parent) :
    QFrame(parent)
{
    m_graphicsView = new SkeletonEditGraphicsView(this);
    m_graphicsView->setRenderHint(QPainter::Antialiasing, false);
    m_graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    m_graphicsView->setBackgroundBrush(QBrush(QWidget::palette().color(QWidget::backgroundRole()), Qt::SolidPattern));
    
    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(m_graphicsView, 0, 0, 1, 1);
    
    setLayout(mainLayout);
}

SkeletonEditGraphicsView *SkeletonEditWidget::graphicsView()
{
    return m_graphicsView;
}

void SkeletonEditWidget::resizeEvent(QResizeEvent *event)
{
    QFrame::resizeEvent(event);
    emit sizeChanged();
}

void SkeletonEditWidget::mouseMoveEvent(QMouseEvent *event)
{
    QFrame::mouseMoveEvent(event);
}
