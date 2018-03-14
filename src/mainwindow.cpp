#include <QVBoxLayout>
#include <QPushButton>
#include <QButtonGroup>
#include <QGridLayout>
#include <QToolBar>
#include <QThread>
#include <QFileDialog>
#include <QApplication>
#include <QDesktopWidget>
#include <QStackedWidget>
#include <assert.h>
#include "mainwindow.h"
#include "skeletonwidget.h"
#include "combineeditwidget.h"
#include "theme.h"

MainWindow::MainWindow()
{
    m_partsPageButton = new QPushButton("Parts");
    m_combinePageButton = new QPushButton("Combine");
    m_motionPageButton = new QPushButton("Motion");
    
    QWidget *hrWidget = new QWidget;
    hrWidget->setFixedHeight(2);
    hrWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    hrWidget->setStyleSheet(QString("background-color: #fc6621;"));
    hrWidget->setContentsMargins(0, 0, 0, 0);
    
    QButtonGroup *pageButtonGroup = new QButtonGroup;
    pageButtonGroup->addButton(m_partsPageButton);
    pageButtonGroup->addButton(m_combinePageButton);
    pageButtonGroup->addButton(m_motionPageButton);
    
    m_partsPageButton->setCheckable(true);
    m_combinePageButton->setCheckable(true);
    m_motionPageButton->setCheckable(true);
    
    pageButtonGroup->setExclusive(true);
    
    m_combinePageButton->setChecked(true);
    
    QHBoxLayout *topButtonsLayout = new QHBoxLayout;
    topButtonsLayout->setContentsMargins(0, 0, 0, 0);
    topButtonsLayout->setSpacing(0);
    topButtonsLayout->addStretch();
    topButtonsLayout->addWidget(m_partsPageButton);
    topButtonsLayout->addWidget(m_combinePageButton);
    topButtonsLayout->addWidget(m_motionPageButton);
    topButtonsLayout->addStretch();
    
    QVBoxLayout *topLayout = new QVBoxLayout;
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(0);
    topLayout->addLayout(topButtonsLayout);
    topLayout->addWidget(hrWidget);
    
    QVBoxLayout *partsRightLayout = new QVBoxLayout;
    
    partsRightLayout->addSpacing(20);
    
    QPushButton *changeTurnaroundButton = new QPushButton("    Change Turnaround    ");
    partsRightLayout->addWidget(changeTurnaroundButton);
    
    QPushButton *exportPartsModelButton = new QPushButton("    Export Model(.obj)    ");
    partsRightLayout->addWidget(exportPartsModelButton);
    
    QPushButton *newPartsButton = new QPushButton("    New Parts    ");
    partsRightLayout->addWidget(newPartsButton);
    
    QPushButton *loadPartsButton = new QPushButton("    Load Parts    ");
    partsRightLayout->addWidget(loadPartsButton);
    
    QPushButton *savePartsButton = new QPushButton("    Save Parts    ");
    partsRightLayout->addWidget(savePartsButton);
    
    QPushButton *savePartsAsButton = new QPushButton("    Save Parts as    ");
    partsRightLayout->addWidget(savePartsAsButton);
    savePartsAsButton->hide();
    
    partsRightLayout->addStretch();
    
    SkeletonWidget *skeletonWidget = new SkeletonWidget(this);
    m_skeletonWidget = skeletonWidget;
    
    QHBoxLayout *partsPageLayout = new QHBoxLayout;
    partsPageLayout->addWidget(skeletonWidget);
    partsPageLayout->addLayout(partsRightLayout);
    
    QWidget *partsPageWidget = new QWidget;
    partsPageWidget->setLayout(partsPageLayout);
    
    QVBoxLayout *combineRightLayout = new QVBoxLayout;
    
    combineRightLayout->addSpacing(20);
    
    QPushButton *importPartsToCombineButton = new QPushButton("    Import Parts    ");
    combineRightLayout->addWidget(importPartsToCombineButton);
    
    QPushButton *exportCombineModelButton = new QPushButton("    Export Model(.obj)    ");
    combineRightLayout->addWidget(exportCombineModelButton);
    
    QPushButton *newCombineButton = new QPushButton("    New Combine    ");
    combineRightLayout->addWidget(newCombineButton);
    
    QPushButton *loadCombineButton = new QPushButton("    Load Combine    ");
    combineRightLayout->addWidget(loadCombineButton);
    
    QPushButton *saveCombineButton = new QPushButton("    Save Combine    ");
    combineRightLayout->addWidget(saveCombineButton);
    
    QPushButton *saveCombineAsButton = new QPushButton("    Save Combine as    ");
    combineRightLayout->addWidget(saveCombineAsButton);
    saveCombineAsButton->hide();
    
    combineRightLayout->addStretch();
    
    combineRightLayout->setSizeConstraint(QLayout::SetMinimumSize);
    
    CombineEditWidget *combineEditWidget = new CombineEditWidget();
    
    QHBoxLayout *combinePageLayout = new QHBoxLayout;
    combinePageLayout->addSpacing(10);
    combinePageLayout->addWidget(combineEditWidget);
    combinePageLayout->addStretch();
    combinePageLayout->addLayout(combineRightLayout);
    combinePageLayout->addSpacing(10);
    
    QWidget *combinePageWidget = new QWidget;
    combinePageWidget->setLayout(combinePageLayout);
    
    QWidget *motionPageWidget = new QWidget;
    
    QStackedWidget *stackedWidget = new QStackedWidget;
    stackedWidget->addWidget(partsPageWidget);
    stackedWidget->addWidget(combinePageWidget);
    stackedWidget->addWidget(motionPageWidget);
    
    m_stackedWidget = stackedWidget;
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(stackedWidget);
    
    QWidget *centralWidget = new QWidget;
    centralWidget->setLayout(mainLayout);
    
    setCentralWidget(centralWidget);
    setWindowTitle(tr("Dust 3D"));
    
    bool connectResult = false;
    
    connectResult = connect(changeTurnaroundButton, SIGNAL(clicked()), skeletonWidget, SLOT(changeTurnaround()));
    assert(connectResult);
    
    connectResult = connect(m_partsPageButton, SIGNAL(clicked()), this, SLOT(updatePageButtons()));
    assert(connectResult);
    
    connectResult = connect(m_combinePageButton, SIGNAL(clicked()), this, SLOT(updatePageButtons()));
    assert(connectResult);
    
    connectResult = connect(m_motionPageButton, SIGNAL(clicked()), this, SLOT(updatePageButtons()));
    assert(connectResult);
    
    connectResult = connect(savePartsButton, SIGNAL(clicked()), this, SLOT(saveParts()));
    assert(connectResult);
    
    connectResult = connect(loadPartsButton, SIGNAL(clicked()), this, SLOT(loadParts()));
    assert(connectResult);
    
    updatePageButtons();
}

void MainWindow::loadParts()
{
    QString filename = QFileDialog::getOpenFileName(this,
        tr("Load Parts"), ".",
        tr("Xml files (*.xml)"));
    if (filename.isEmpty())
        return;
    m_skeletonWidget->graphicsView()->loadFromXml(filename);
}

void MainWindow::saveParts()
{
    if (m_savePartsAs.isEmpty()) {
        m_savePartsAs = QFileDialog::getSaveFileName(this,
           tr("Save Parts"), ".",
           tr("Xml files (*.xml)"));
        if (m_savePartsAs.isEmpty()) {
            return;
        }
    }
    m_skeletonWidget->graphicsView()->saveToXml(m_savePartsAs);
}

void MainWindow::updatePageButtons()
{
    if (m_partsPageButton->isChecked()) {
        m_stackedWidget->setCurrentIndex(0);
    }
    if (m_combinePageButton->isChecked()) {
        m_stackedWidget->setCurrentIndex(1);
    }
    if (m_motionPageButton->isChecked()) {
        m_stackedWidget->setCurrentIndex(2);
    }
    m_partsPageButton->setStyleSheet(m_partsPageButton->isChecked() ? Theme::tabButtonSelectedStylesheet : Theme::tabButtonStylesheet);
    m_combinePageButton->setStyleSheet(m_combinePageButton->isChecked() ? Theme::tabButtonSelectedStylesheet : Theme::tabButtonStylesheet);
    m_motionPageButton->setStyleSheet(m_motionPageButton->isChecked() ? Theme::tabButtonSelectedStylesheet : Theme::tabButtonStylesheet);
}

