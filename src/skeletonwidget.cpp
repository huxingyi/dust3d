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
#include "skeletonwidget.h"
#include "skeletoneditwidget.h"
#include "meshlite.h"
#include "skeletontomesh.h"
#include "turnaroundloader.h"

SkeletonWidget::SkeletonWidget(QWidget *parent) :
    QWidget(parent),
    m_skeletonToMesh(NULL),
    m_skeletonDirty(false),
    m_turnaroundLoader(NULL),
    m_turnaroundDirty(false)
{
    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->addStretch();
    
    m_skeletonEditWidget = new SkeletonEditWidget;
    
    m_modelingWidget = new ModelingWidget(this);
    m_modelingWidget->setMinimumSize(128, 128);
    m_modelingWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_modelingWidget->setWindowFlags(Qt::Tool | Qt::Window);
    m_modelingWidget->setWindowTitle("3D Model");

    QVBoxLayout *rightLayout = new QVBoxLayout;
    rightLayout->addSpacing(10);
    rightLayout->addStretch();
    
    QToolBar *toolbar = new QToolBar;
    toolbar->setIconSize(QSize(16, 16));
    toolbar->setOrientation(Qt::Vertical);
    
    QAction *addAction = new QAction(tr("Add"), this);
    addAction->setIcon(QIcon(":/resources/add.png"));
    toolbar->addAction(addAction);
    
    QAction *selectAction = new QAction(tr("Select"), this);
    selectAction->setIcon(QIcon(":/resources/select.png"));
    toolbar->addAction(selectAction);
    
    QAction *rangeSelectAction = new QAction(tr("Range Select"), this);
    rangeSelectAction->setIcon(QIcon(":/resources/rangeselect.png"));
    toolbar->addAction(rangeSelectAction);
    
    QVBoxLayout *leftLayout = new QVBoxLayout;
    leftLayout->addWidget(toolbar);
    leftLayout->addStretch();
    leftLayout->addSpacing(10);
    
    QHBoxLayout *middleLayout = new QHBoxLayout;
    middleLayout->addLayout(leftLayout);
    middleLayout->addWidget(m_skeletonEditWidget);
    middleLayout->addLayout(rightLayout);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(topLayout);
    //mainLayout->addSpacing(10);
    mainLayout->addLayout(middleLayout);
    
    setLayout(mainLayout);
    
    setWindowTitle(tr("Dust 3D"));
    
    bool connectResult;
    
    connectResult = connect(addAction, SIGNAL(triggered(bool)), m_skeletonEditWidget->graphicsView(), SLOT(turnOnAddNodeMode()));
    assert(connectResult);
    
    connectResult = connectResult = connect(selectAction, SIGNAL(triggered(bool)), m_skeletonEditWidget->graphicsView(), SLOT(turnOffAddNodeMode()));
    assert(connectResult);
    
    connectResult = connect(m_skeletonEditWidget->graphicsView(), SIGNAL(nodesChanged()), this, SLOT(skeletonChanged()));
    assert(connectResult);
    
    connectResult = connect(m_skeletonEditWidget, SIGNAL(sizeChanged()), this, SLOT(turnaroundChanged()));
    assert(connectResult);
    
    connectResult = connect(m_skeletonEditWidget->graphicsView(), SIGNAL(changeTurnaroundTriggered()), this, SLOT(changeTurnaround()));
    assert(connectResult);
    
    //connectResult = connect(clipButton, SIGNAL(clicked()), this, SLOT(saveClip()));
    //assert(connectResult);
}

void SkeletonWidget::showModelingWidgetAtCorner()
{
    if (!m_modelingWidget->isVisible()) {
        QPoint pos = QPoint(QApplication::desktop()->width(),
            QApplication::desktop()->height());
        m_modelingWidget->move(pos.x() - m_modelingWidget->width(),
            pos.y() - m_modelingWidget->height());
        m_modelingWidget->show();
    }
}

void SkeletonWidget::meshReady()
{
    Mesh *resultMesh = m_skeletonToMesh->takeResultMesh();
    showModelingWidgetAtCorner();
    m_modelingWidget->updateMesh(resultMesh);
    delete m_skeletonToMesh;
    m_skeletonToMesh = NULL;
    if (m_skeletonDirty) {
        skeletonChanged();
    }
}

void SkeletonWidget::skeletonChanged()
{
    if (m_skeletonToMesh) {
        m_skeletonDirty = true;
        return;
    }
    
    m_skeletonDirty = false;
    
    QThread *thread = new QThread;
    m_skeletonToMesh = new SkeletonToMesh(m_skeletonEditWidget->graphicsView());
    m_skeletonToMesh->moveToThread(thread);
    connect(thread, SIGNAL(started()), m_skeletonToMesh, SLOT(process()));
    connect(m_skeletonToMesh, SIGNAL(finished()), this, SLOT(meshReady()));
    connect(m_skeletonToMesh, SIGNAL(finished()), thread, SLOT(quit()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();
}

void SkeletonWidget::turnaroundChanged()
{
    if (m_turnaroundFilename.isEmpty())
        return;
    
    if (m_turnaroundLoader) {
        m_turnaroundDirty = true;
        return;
    }
    
    m_turnaroundDirty = false;
    
    QThread *thread = new QThread;
    m_turnaroundLoader = new TurnaroundLoader(m_turnaroundFilename,
        m_skeletonEditWidget->rect().size());
    m_turnaroundLoader->moveToThread(thread);
    connect(thread, SIGNAL(started()), m_turnaroundLoader, SLOT(process()));
    connect(m_turnaroundLoader, SIGNAL(finished()), this, SLOT(turnaroundImageReady()));
    connect(m_turnaroundLoader, SIGNAL(finished()), thread, SLOT(quit()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();
}

void SkeletonWidget::turnaroundImageReady()
{
    QImage *backgroundImage = m_turnaroundLoader->takeResultImage();
    if (backgroundImage && backgroundImage->width() > 0 && backgroundImage->height() > 0) {
        m_skeletonEditWidget->graphicsView()->updateBackgroundImage(*backgroundImage);
    }
    delete backgroundImage;
    delete m_turnaroundLoader;
    m_turnaroundLoader = NULL;
    if (m_turnaroundDirty) {
        turnaroundChanged();
    }
}

void SkeletonWidget::changeTurnaround()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Turnaround Reference Image"),
        QString(),
        tr("Image Files (*.png *.jpg *.bmp)")).trimmed();
    if (fileName.isEmpty())
        return;
    m_turnaroundFilename = fileName;
    turnaroundChanged();
}

void SkeletonWidget::saveClip()
{
    QImage image = m_modelingWidget->grabFramebuffer();
    //QTableWidgetItem *item = new QTableWidgetItem;
    //item->setSizeHint(QSize(32, 32));
    //item->setIcon(QPixmap::fromImage(image.scaled(32, 32)));
    //m_clipTableWidget->insertRow(m_clipTableWidget->rowCount());
    //m_clipTableWidget->setItem(m_clipTableWidget->rowCount() - 1, 0, item);
    //image.save("/Users/jeremy/Repositories/dust3d/gl.png");
}
