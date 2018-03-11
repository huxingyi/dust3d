#include "mainwindow.h"
#include "skeletoneditwidget.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QButtonGroup>
#include <QGridLayout>
#include <QToolBar>

MainWindow::MainWindow()
{
    QPushButton *skeletonButton = new QPushButton("Skeleton");
    QPushButton *motionButton = new QPushButton("Motion");
    QPushButton *modelButton = new QPushButton("Model");
    
    QButtonGroup *pageButtonGroup = new QButtonGroup;
    pageButtonGroup->addButton(skeletonButton);
    pageButtonGroup->addButton(motionButton);
    pageButtonGroup->addButton(modelButton);
    
    skeletonButton->setCheckable(true);
    motionButton->setCheckable(true);
    modelButton->setCheckable(true);
    
    pageButtonGroup->setExclusive(true);
    
    skeletonButton->setChecked(true);
    
    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->addStretch();
    topLayout->addWidget(skeletonButton);
    topLayout->addWidget(motionButton);
    topLayout->addWidget(modelButton);
    topLayout->addStretch();
    
    skeletonButton->adjustSize();
    motionButton->adjustSize();
    modelButton->adjustSize();
    
    SkeletonEditWidget *skeletonEditWidget = new SkeletonEditWidget;
    ModelingWidget *modelViewWidget = new ModelingWidget;
    modelViewWidget->setFixedSize(128, 128);
    
    QPushButton *changeTurnaroundButton = new QPushButton("Change turnaround..");
    
    QVBoxLayout *rightLayout = new QVBoxLayout;
    rightLayout->addWidget(modelViewWidget);
    rightLayout->addSpacing(10);
    rightLayout->addWidget(changeTurnaroundButton);
    rightLayout->addStretch();
    
    QToolBar *toolbar = new QToolBar;
    toolbar->setOrientation(Qt::Vertical);
    QAction *addAction = new QAction(tr("Add"), this);
    QAction *selectAction = new QAction(tr("Select"), this);
    toolbar->addAction(addAction);
    toolbar->addAction(selectAction);
    
    QVBoxLayout *leftLayout = new QVBoxLayout;
    leftLayout->addWidget(toolbar);
    leftLayout->addStretch();
    
    QHBoxLayout *middleLayout = new QHBoxLayout;
    middleLayout->addLayout(leftLayout);
    middleLayout->addWidget(skeletonEditWidget);
    middleLayout->addLayout(rightLayout);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(topLayout);
    mainLayout->addSpacing(20);
    mainLayout->addLayout(middleLayout);
    
    QWidget *centralWidget = new QWidget;
    centralWidget->setLayout(mainLayout);
    
    setCentralWidget(centralWidget);
    setWindowTitle(tr("Dust 3D"));
}

