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
#include <QFormLayout>
#include <QComboBox>
#include <QMenu>
#include <QMenuBar>
#include <QLabel>
#include <assert.h>
#include "mainwindow.h"
#include "skeletonwidget.h"
#include "theme.h"
#include "ds3file.h"
#include "skeletonxml.h"

MainWindow::MainWindow()
{
    m_modelPageButton = new QPushButton("Model");
    m_sharePageButton = new QPushButton("Share");
    
    QWidget *hrWidget = new QWidget;
    hrWidget->setFixedHeight(1);
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
    topButtonsLayout->setSpacing(20);
    topButtonsLayout->addWidget(m_modelPageButton);
    topButtonsLayout->addWidget(m_sharePageButton);
    topButtonsLayout->addStretch();
    
    QVBoxLayout *topLayout = new QVBoxLayout;
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(0);
    topLayout->addLayout(topButtonsLayout);
    topLayout->addWidget(hrWidget);
    
    m_edgePropertyWidget = new QWidget;
    QFormLayout *formLayout = new QFormLayout;
    QComboBox *edgeTypeBox = new QComboBox;
    edgeTypeBox->addItem("Spine", "Spine");
    edgeTypeBox->addItem("Attach", "Attach");
    formLayout->addRow(tr("Edge Type:"), edgeTypeBox);
    m_edgePropertyWidget->setLayout(formLayout);
    
    QVBoxLayout *modelRightLayout = new QVBoxLayout;
    
    modelRightLayout->addSpacing(10);
    
    QPushButton *changeTurnaroundButton = new QPushButton("    Change Turnaround    ");
    modelRightLayout->addWidget(changeTurnaroundButton);
    
    QPushButton *exportModelButton = new QPushButton("    Export    ");
    modelRightLayout->addWidget(exportModelButton);
    
    QPushButton *newModelButton = new QPushButton("    New    ");
    modelRightLayout->addWidget(newModelButton);
    
    QPushButton *loadModelButton = new QPushButton("    Load    ");
    modelRightLayout->addWidget(loadModelButton);
    
    QPushButton *saveModelButton = new QPushButton("    Save    ");
    modelRightLayout->addWidget(saveModelButton);
    
    QPushButton *saveModelAsButton = new QPushButton("    Save as    ");
    modelRightLayout->addWidget(saveModelAsButton);
    saveModelAsButton->hide();
    
    modelRightLayout->addSpacing(20);
    
    modelRightLayout->addWidget(m_edgePropertyWidget);
    
    modelRightLayout->addStretch();
    
    QLabel *dust3dJezzasoftLabel = new QLabel;
    QImage dust3dJezzasoftImage;
    dust3dJezzasoftImage.load(":/resources/dust3d_jezzasoft.png");
    dust3dJezzasoftLabel->setPixmap(QPixmap::fromImage(dust3dJezzasoftImage));
    
    QVBoxLayout *mainLeftLayout = new QVBoxLayout;
    mainLeftLayout->addStretch();
    mainLeftLayout->addWidget(dust3dJezzasoftLabel);
    mainLeftLayout->setSpacing(0);
    mainLeftLayout->setContentsMargins(0, 0, 0, 0);
    
    SkeletonWidget *skeletonWidget = new SkeletonWidget(this);
    m_skeletonWidget = skeletonWidget;
    
    QHBoxLayout *modelPageLayout = new QHBoxLayout;
    modelPageLayout->addLayout(mainLeftLayout);
    modelPageLayout->addWidget(skeletonWidget);
    //modelPageLayout->addLayout(modelRightLayout);
    
    QWidget *modelPageWidget = new QWidget;
    modelPageWidget->setLayout(modelPageLayout);
    
    QWidget *sharePageWidget = new QWidget;
    
    QStackedWidget *stackedWidget = new QStackedWidget;
    stackedWidget->addWidget(modelPageWidget);
    stackedWidget->addWidget(sharePageWidget);
    stackedWidget->setContentsMargins(0, 0, 0, 0);
    
    m_stackedWidget = stackedWidget;
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    //mainLayout->addLayout(topLayout);
    mainLayout->addWidget(stackedWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    QWidget *centralWidget = new QWidget;
    centralWidget->setLayout(mainLayout);
    
    setCentralWidget(centralWidget);
    setWindowTitle(tr("Dust 3D"));
    
    QAction *newAct = new QAction(tr("&New"), this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip(tr("Create a new file"));
    connect(newAct, &QAction::triggered, this, &MainWindow::newFile);
    
    QAction *openAct = new QAction(tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, &QAction::triggered, this, &MainWindow::open);
    
    QAction *saveAct = new QAction(tr("&Save"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save the document to disk"));
    connect(saveAct, &QAction::triggered, this, &MainWindow::save);
    
    QAction *exportAct = new QAction(tr("E&xport.."), this);
    connect(exportAct, &QAction::triggered, this, &MainWindow::exportModel);
    
    QAction *undoAct = new QAction(tr("&Undo"), this);
    undoAct->setShortcuts(QKeySequence::Undo);
    undoAct->setStatusTip(tr("Undo the last operation"));
    connect(undoAct, &QAction::triggered, this, &MainWindow::undo);

    QAction *redoAct = new QAction(tr("&Redo"), this);
    redoAct->setShortcuts(QKeySequence::Redo);
    redoAct->setStatusTip(tr("Redo the last operation"));
    connect(redoAct, &QAction::triggered, this, &MainWindow::redo);

    QAction *cutAct = new QAction(tr("Cu&t"), this);
    cutAct->setShortcuts(QKeySequence::Cut);
    cutAct->setStatusTip(tr("Cut the current selection's contents to the "
                            "clipboard"));
    connect(cutAct, &QAction::triggered, this, &MainWindow::cut);

    QAction *copyAct = new QAction(tr("&Copy"), this);
    copyAct->setShortcuts(QKeySequence::Copy);
    copyAct->setStatusTip(tr("Copy the current selection's contents to the "
                             "clipboard"));
    connect(copyAct, &QAction::triggered, this, &MainWindow::copy);

    QAction *pasteAct = new QAction(tr("&Paste"), this);
    pasteAct->setShortcuts(QKeySequence::Paste);
    pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current "
                              "selection"));
    connect(pasteAct, &QAction::triggered, this, &MainWindow::paste);
    
    QAction *markAsBranchAct = new QAction(tr("Mark as &Branch"), this);
    connect(markAsBranchAct, &QAction::triggered, skeletonWidget->graphicsView(), &SkeletonEditGraphicsView::markAsBranch);
    
    QAction *markAsTrunkAct = new QAction(tr("Mark as &Trunk"), this);
    connect(markAsTrunkAct, &QAction::triggered, skeletonWidget->graphicsView(), &SkeletonEditGraphicsView::markAsTrunk);
    
    QAction *markAsRootAct = new QAction(tr("Mark as &Root"), this);
    connect(markAsRootAct, &QAction::triggered, skeletonWidget->graphicsView(), &SkeletonEditGraphicsView::markAsRoot);
    
    QAction *markAsChildAct = new QAction(tr("Mark as &Child"), this);
    connect(markAsChildAct, &QAction::triggered, skeletonWidget->graphicsView(), &SkeletonEditGraphicsView::markAsChild);
    
    QAction *changeTurnaroundAct = new QAction(tr("&Change Turnaround..."), this);
    connect(changeTurnaroundAct, &QAction::triggered, skeletonWidget, &SkeletonWidget::changeTurnaround);
    
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(newAct);
    fileMenu->addAction(openAct);
    fileMenu->addAction(saveAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exportAct);

    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(undoAct);
    editMenu->addAction(redoAct);
    editMenu->addSeparator();
    editMenu->addAction(cutAct);
    editMenu->addAction(copyAct);
    editMenu->addAction(pasteAct);
    editMenu->addSeparator();
    editMenu->addAction(markAsBranchAct);
    editMenu->addAction(markAsTrunkAct);
    editMenu->addSeparator();
    editMenu->addAction(markAsRootAct);
    editMenu->addAction(markAsChildAct);
    editMenu->addSeparator();
    editMenu->addAction(changeTurnaroundAct);
    
    bool connectResult = false;
    
    connectResult = connect(changeTurnaroundButton, SIGNAL(clicked()), skeletonWidget, SLOT(changeTurnaround()));
    assert(connectResult);
    
    connectResult = connect(m_modelPageButton, SIGNAL(clicked()), this, SLOT(updatePageButtons()));
    assert(connectResult);
    
    connectResult = connect(m_sharePageButton, SIGNAL(clicked()), this, SLOT(updatePageButtons()));
    assert(connectResult);
    
    connectResult = connect(exportModelButton, SIGNAL(clicked()), this, SLOT(exportModel()));
    assert(connectResult);
    
    connectResult = connect(saveModelButton, SIGNAL(clicked()), this, SLOT(saveModel()));
    assert(connectResult);
    
    connectResult = connect(loadModelButton, SIGNAL(clicked()), this, SLOT(loadModel()));
    assert(connectResult);
    
    updatePageButtons();
}

void MainWindow::newFile()
{

}

void MainWindow::open()
{
    loadModel();
}

void MainWindow::save()
{
    saveModel();
}

void MainWindow::undo()
{

}

void MainWindow::redo()
{

}

void MainWindow::cut()
{

}

void MainWindow::copy()
{

}

void MainWindow::paste()
{

}

void MainWindow::exportModel()
{
    QString exportTo = QFileDialog::getSaveFileName(this,
       tr("Export Model"), ".",
       tr("Wavefront OBJ File (*.obj)"));
    if (exportTo.isEmpty()) {
        return;
    }
    m_skeletonWidget->modelWidget()->exportMeshAsObj(exportTo);
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
            SkeletonSnapshot snapshot;
            loadSkeletonFromXmlStream(&snapshot, xmlReader);
            m_skeletonWidget->graphicsView()->loadFromSnapshot(&snapshot);
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
    SkeletonSnapshot snapshot;
    m_skeletonWidget->graphicsView()->saveToSnapshot(&snapshot);
    saveSkeletonToXmlStream(&snapshot, &stream);
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

