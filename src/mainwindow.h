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
    void saveParts();
    void loadParts();
private:
    QPushButton *m_partsPageButton;
    QPushButton *m_combinePageButton;
    QPushButton *m_motionPageButton;
    QStackedWidget *m_stackedWidget;
    QString m_savePartsAs;
    SkeletonWidget *m_skeletonWidget;
};

#endif
