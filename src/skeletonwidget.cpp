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
    topLayout->setContentsMargins(0, 0, 0, 0);
    
    m_graphicsView = new SkeletonEditGraphicsView(this);
    m_graphicsView->setRenderHint(QPainter::Antialiasing, false);
    m_graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_graphicsView->setBackgroundBrush(QBrush(QWidget::palette().color(QWidget::backgroundRole()), Qt::SolidPattern));
    m_graphicsView->setContentsMargins(0, 0, 0, 0);
    m_graphicsView->setFrameStyle(QFrame::NoFrame);
    
    m_modelWidget = new ModelWidget(this);
    m_modelWidget->setMinimumSize(128, 128);
    m_modelWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_modelWidget->setWindowFlags(Qt::Tool | Qt::Window);
    m_modelWidget->setWindowTitle("3D Model");

    QVBoxLayout *rightLayout = new QVBoxLayout;
    rightLayout->addSpacing(0);
    rightLayout->addStretch();
    rightLayout->setContentsMargins(0, 0, 0, 0);
    
    QToolBar *toolbar = new QToolBar;
    toolbar->setIconSize(QSize(18, 18));
    toolbar->setOrientation(Qt::Vertical);
    
    QAction *addAction = new QAction(tr("Add"), this);
    addAction->setIcon(QPixmap(":/resources/add.svg"));
    toolbar->addAction(addAction);
    
    QAction *selectAction = new QAction(tr("Select"), this);
    selectAction->setIcon(QPixmap(":/resources/rotate.svg"));
    toolbar->addAction(selectAction);
    
    QAction *rangeSelectAction = new QAction(tr("Range Select"), this);
    rangeSelectAction->setIcon(QPixmap(":/resources/zoomin.svg"));
    toolbar->addAction(rangeSelectAction);
    
    toolbar->setContentsMargins(0, 0, 0, 0);
    
    QVBoxLayout *leftLayout = new QVBoxLayout;
    leftLayout->addWidget(toolbar);
    leftLayout->addStretch();
    leftLayout->addSpacing(0);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    
    QHBoxLayout *middleLayout = new QHBoxLayout;
    //middleLayout->addLayout(leftLayout);
    middleLayout->addWidget(m_graphicsView);
    //middleLayout->addLayout(rightLayout);
    middleLayout->setContentsMargins(0, 0, 0, 0);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    //mainLayout->addLayout(topLayout);
    //mainLayout->addSpacing(10);
    mainLayout->addLayout(middleLayout);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    setLayout(mainLayout);
    setContentsMargins(0, 0, 0, 0);
    
    setWindowTitle(tr("Dust 3D"));
    
    bool connectResult;
    
    connectResult = connect(addAction, SIGNAL(triggered(bool)), m_graphicsView, SLOT(turnOnAddNodeMode()));
    assert(connectResult);
    
    connectResult = connectResult = connect(selectAction, SIGNAL(triggered(bool)), m_graphicsView, SLOT(turnOffAddNodeMode()));
    assert(connectResult);
    
    connectResult = connect(m_graphicsView, SIGNAL(nodesChanged()), this, SLOT(skeletonChanged()));
    assert(connectResult);
    
    connectResult = connect(m_graphicsView, SIGNAL(sizeChanged()), this, SLOT(turnaroundChanged()));
    assert(connectResult);
    
    connectResult = connect(m_graphicsView, SIGNAL(changeTurnaroundTriggered()), this, SLOT(changeTurnaround()));
    assert(connectResult);
    
    //connectResult = connect(clipButton, SIGNAL(clicked()), this, SLOT(saveClip()));
    //assert(connectResult);
}

SkeletonEditGraphicsView *SkeletonWidget::graphicsView()
{
    return m_graphicsView;
}

ModelWidget *SkeletonWidget::modelWidget()
{
    return m_modelWidget;
}

void SkeletonWidget::showModelingWidgetAtCorner()
{
    if (!m_modelWidget->isVisible()) {
        QPoint pos = QPoint(QApplication::desktop()->width(),
            QApplication::desktop()->height());
        m_modelWidget->move(pos.x() - m_modelWidget->width(),
            pos.y() - m_modelWidget->height());
        m_modelWidget->show();
    }
}

void SkeletonWidget::meshReady()
{
    Mesh *resultMesh = m_skeletonToMesh->takeResultMesh();
    showModelingWidgetAtCorner();
    m_modelWidget->updateMesh(resultMesh);
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
    SkeletonSnapshot snapshot;
    m_graphicsView->saveToSnapshot(&snapshot);
    m_skeletonToMesh = new SkeletonToMesh(&snapshot);
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
        m_graphicsView->rect().size());
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
        m_graphicsView->updateBackgroundImage(*backgroundImage);
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

