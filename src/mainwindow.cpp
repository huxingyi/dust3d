#include <QVBoxLayout>
#include <QPushButton>
#include <QButtonGroup>
#include <QGridLayout>
#include <QToolBar>
#include <QThread>
#include <QFileDialog>
#include <QApplication>
#include <QDesktopWidget>
#include <assert.h>
#include "mainwindow.h"
#include "skeletonwidget.h"
#include "theme.h"

MainWindow::MainWindow()
{
    m_partsPageButton = new QPushButton("Parts");
    m_combinePageButton = new QPushButton("Combine");
    
    QWidget *hrWidget = new QWidget;
    hrWidget->setFixedHeight(2);
    hrWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    hrWidget->setStyleSheet(QString("background-color: #fc6621;"));
    hrWidget->setContentsMargins(0, 0, 0, 0);
    
    QButtonGroup *pageButtonGroup = new QButtonGroup;
    pageButtonGroup->addButton(m_partsPageButton);
    pageButtonGroup->addButton(m_combinePageButton);
    
    m_partsPageButton->setCheckable(true);
    m_combinePageButton->setCheckable(true);
    
    pageButtonGroup->setExclusive(true);
    
    m_partsPageButton->setChecked(true);
    updatePageButtons();
    
    QHBoxLayout *topButtonsLayout = new QHBoxLayout;
    topButtonsLayout->setContentsMargins(0, 0, 0, 0);
    topButtonsLayout->setSpacing(0);
    topButtonsLayout->addStretch();
    topButtonsLayout->addWidget(m_partsPageButton);
    topButtonsLayout->addWidget(m_combinePageButton);
    topButtonsLayout->addStretch();
    
    QVBoxLayout *topLayout = new QVBoxLayout;
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(0);
    topLayout->addLayout(topButtonsLayout);
    topLayout->addWidget(hrWidget);
    
    SkeletonWidget *skeletonWidget = new SkeletonWidget(this);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(skeletonWidget);
    
    QWidget *centralWidget = new QWidget;
    centralWidget->setLayout(mainLayout);
    
    setCentralWidget(centralWidget);
    setWindowTitle(tr("Dust 3D"));
    
    bool connectResult = false;
    
    connectResult = connect(m_partsPageButton, SIGNAL(clicked()), this, SLOT(updatePageButtons()));
    assert(connectResult);
    
    connectResult = connect(m_combinePageButton, SIGNAL(clicked()), this, SLOT(updatePageButtons()));
    assert(connectResult);
}

void MainWindow::updatePageButtons()
{
    m_partsPageButton->setStyleSheet(m_partsPageButton->isChecked() ? Theme::tabButtonSelectedStylesheet : Theme::tabButtonStylesheet);
    m_combinePageButton->setStyleSheet(m_combinePageButton->isChecked() ? Theme::tabButtonSelectedStylesheet : Theme::tabButtonStylesheet);
}

