#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H
#include <QMainWindow>
#include "modelingwidget.h"
#include "skeletoneditwidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow();
public slots:
    void skeletonChanged();
    void meshReady();
private:
    ModelingWidget *m_modelingWidget;
    SkeletonEditWidget *m_skeletonEditWidget;
};

#endif
