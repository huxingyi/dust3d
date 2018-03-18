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
#include <QXmlStreamReader>
#include <QBuffer>
#include <assert.h>
#include "mainwindow.h"
#include "skeletonwidget.h"
#include "theme.h"
#include "ds3file.h"

MainWindow::MainWindow()
{
    m_modelPageButton = new QPushButton("Model");
    m_sharePageButton = new QPushButton("Share");
    
    QWidget *hrWidget = new QWidget;
    hrWidget->setFixedHeight(2);
    hrWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    hrWidget->setStyleSheet(QString("background-color: #fc6621;"));
    hrWidget->setContentsMargins(0, 0, 0, 0);
    
    QButtonGroup *pageButtonGroup = new QButtonGroup;
    pageButtonGroup->addButton(m_modelPageButton);
    pageButtonGroup->addButton(m_sharePageButton);
    
    m_modelPageButton->setCheckable(true);
    m_sharePageButton->setCheckable(true);
    
    pageButtonGroup->setExclusive(true);
    
    m_modelPageButton->setChecked(true);
    
    QHBoxLayout *topButtonsLayout = new QHBoxLayout;
    topButtonsLayout->setContentsMargins(0, 0, 0, 0);
    topButtonsLayout->setSpacing(0);
    topButtonsLayout->addStretch();
    topButtonsLayout->addWidget(m_modelPageButton);
    topButtonsLayout->addWidget(m_sharePageButton);
    topButtonsLayout->addStretch();
    
    QVBoxLayout *topLayout = new QVBoxLayout;
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(0);
    topLayout->addLayout(topButtonsLayout);
    topLayout->addWidget(hrWidget);
    
    QVBoxLayout *modelRightLayout = new QVBoxLayout;
    
    modelRightLayout->addSpacing(20);
    
    QPushButton *changeTurnaroundButton = new QPushButton("    Change Turnaround    ");
    modelRightLayout->addWidget(changeTurnaroundButton);
    
    QPushButton *exportPartsModelButton = new QPushButton("    Export    ");
    modelRightLayout->addWidget(exportPartsModelButton);
    
    QPushButton *newModelButton = new QPushButton("    New    ");
    modelRightLayout->addWidget(newModelButton);
    
    QPushButton *loadModelButton = new QPushButton("    Load    ");
    modelRightLayout->addWidget(loadModelButton);
    
    QPushButton *saveModelButton = new QPushButton("    Save    ");
    modelRightLayout->addWidget(saveModelButton);
    
    QPushButton *saveModelAsButton = new QPushButton("    Save as    ");
    modelRightLayout->addWidget(saveModelAsButton);
    saveModelAsButton->hide();
    
    modelRightLayout->addStretch();
    
    SkeletonWidget *skeletonWidget = new SkeletonWidget(this);
    m_skeletonWidget = skeletonWidget;
    
    QHBoxLayout *modelPageLayout = new QHBoxLayout;
    modelPageLayout->addWidget(skeletonWidget);
    modelPageLayout->addLayout(modelRightLayout);
    
    QWidget *modelPageWidget = new QWidget;
    modelPageWidget->setLayout(modelPageLayout);
    
    QWidget *sharePageWidget = new QWidget;
    
    QStackedWidget *stackedWidget = new QStackedWidget;
    stackedWidget->addWidget(modelPageWidget);
    stackedWidget->addWidget(sharePageWidget);
    
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
    
    connectResult = connect(m_modelPageButton, SIGNAL(clicked()), this, SLOT(updatePageButtons()));
    assert(connectResult);
    
    connectResult = connect(m_sharePageButton, SIGNAL(clicked()), this, SLOT(updatePageButtons()));
    assert(connectResult);
    
    connectResult = connect(saveModelButton, SIGNAL(clicked()), this, SLOT(saveModel()));
    assert(connectResult);
    
    connectResult = connect(loadModelButton, SIGNAL(clicked()), this, SLOT(loadModel()));
    assert(connectResult);
    
    updatePageButtons();
}

void MainWindow::loadModel()
{
    QString filename = QFileDialog::getOpenFileName(this,
        tr("Load Model"), ".",
        tr("Dust 3D Project (*.ds3)"));
    if (filename.isEmpty())
        return;
    Ds3FileReader ds3Reader(filename);
    for (int i = 0; i < ds3Reader.items().size(); ++i) {
        Ds3ReaderItem item = ds3Reader.items().at(i);
        if (item.type == "model") {
            QByteArray data;
            ds3Reader.loadItem(item.name, &data);
            QXmlStreamReader xmlReader(data);
            m_skeletonWidget->graphicsView()->loadFromXmlStream(xmlReader);
        } else if (item.type == "asset") {
            if (item.name == "canvas.png") {
                QByteArray data;
                ds3Reader.loadItem(item.name, &data);
                QImage image = QImage::fromData(data, "PNG");
                m_skeletonWidget->graphicsView()->updateBackgroundImage(image);
            }
        }
    }
    m_skeletonWidget->graphicsView()->turnOffAddNodeMode();
}

void MainWindow::saveModel()
{
    if (m_saveModelAs.isEmpty()) {
        m_saveModelAs = QFileDialog::getSaveFileName(this,
           tr("Save Model"), ".",
           tr("Dust 3D Project (*.ds3)"));
        if (m_saveModelAs.isEmpty()) {
            return;
        }
    }
    
    Ds3FileWriter ds3Writer;
    
    QByteArray modelXml;
    QXmlStreamWriter stream(&modelXml);
    m_skeletonWidget->graphicsView()->saveToXmlStream(&stream);
    if (modelXml.size() > 0)
        ds3Writer.add("model1.xml", "model", &modelXml);
    
    QByteArray imageByteArray;
    QBuffer pngBuffer(&imageByteArray);
    pngBuffer.open(QIODevice::WriteOnly);
    m_skeletonWidget->graphicsView()->backgroundImage().save(&pngBuffer, "PNG");
    if (imageByteArray.size() > 0)
        ds3Writer.add("canvas.png", "asset", &imageByteArray);
    
    ds3Writer.save(m_saveModelAs);
}

void MainWindow::updatePageButtons()
{
    if (m_modelPageButton->isChecked()) {
        m_stackedWidget->setCurrentIndex(0);
    }
    if (m_sharePageButton->isChecked()) {
        m_stackedWidget->setCurrentIndex(1);
    }
    m_modelPageButton->setStyleSheet(m_modelPageButton->isChecked() ? Theme::tabButtonSelectedStylesheet : Theme::tabButtonStylesheet);
    m_sharePageButton->setStyleSheet(m_sharePageButton->isChecked() ? Theme::tabButtonSelectedStylesheet : Theme::tabButtonStylesheet);
}

