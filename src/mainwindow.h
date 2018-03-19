#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H
#include <QMainWindow>
#include <QString>
#include <QVBoxLayout>
#include <QPushButton>
#include <QStackedWidget>
#include "skeletonwidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow();
public slots:
    void updatePageButtons();
    void saveModel();
    void loadModel();
private:
    QPushButton *m_modelPageButton;
    QPushButton *m_sharePageButton;
    QStackedWidget *m_stackedWidget;
    QString m_saveModelAs;
    SkeletonWidget *m_skeletonWidget;
    QWidget *m_edgePropertyWidget;
};

#endif
