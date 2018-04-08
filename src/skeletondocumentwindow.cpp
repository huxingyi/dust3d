#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGridLayout>
#include <QToolBar>
#include <QPushButton>
#include <QFileDialog>
#include <QTabWidget>
#include "skeletondocumentwindow.h"
#include "skeletongraphicswidget.h"
#include "skeletonpartlistwidget.h"
#include "skeletonedgepropertywidget.h"
#include "skeletonnodepropertywidget.h"
#include "theme.h"

SkeletonDocumentWindow::SkeletonDocumentWindow() :
    m_document(nullptr),
    m_firstShow(true)
{
    m_document = new SkeletonDocument;
    
    QVBoxLayout *toolButtonLayout = new QVBoxLayout;
    toolButtonLayout->setSpacing(0);
    toolButtonLayout->setContentsMargins(5, 10, 4, 0);
    
    QPushButton *undoButton = new QPushButton(QChar(fa::undo));
    undoButton->setFont(Theme::awesome()->font(Theme::toolIconFontSize));
    undoButton->setFixedSize(Theme::toolIconSize, Theme::toolIconSize);
    
    QPushButton *addButton = new QPushButton(QChar(fa::plus));
    addButton->setFont(Theme::awesome()->font(Theme::toolIconFontSize));
    addButton->setFixedSize(Theme::toolIconSize, Theme::toolIconSize);
    
    QPushButton *selectButton = new QPushButton(QChar(fa::mousepointer));
    selectButton->setFont(Theme::awesome()->font(Theme::toolIconFontSize));
    selectButton->setFixedSize(Theme::toolIconSize, Theme::toolIconSize);
    
    QPushButton *dragButton = new QPushButton(QChar(fa::handrocko));
    dragButton->setFont(Theme::awesome()->font(Theme::toolIconFontSize));
    dragButton->setFixedSize(Theme::toolIconSize, Theme::toolIconSize);
    
    QPushButton *zoomInButton = new QPushButton(QChar(fa::searchplus));
    zoomInButton->setFont(Theme::awesome()->font(Theme::toolIconFontSize));
    zoomInButton->setFixedSize(Theme::toolIconSize, Theme::toolIconSize);
    
    QPushButton *zoomOutButton = new QPushButton(QChar(fa::searchminus));
    zoomOutButton->setFont(Theme::awesome()->font(Theme::toolIconFontSize));
    zoomOutButton->setFixedSize(Theme::toolIconSize, Theme::toolIconSize);
    
    QPushButton *changeTurnaroundButton = new QPushButton(QChar(fa::image));
    changeTurnaroundButton->setFont(Theme::awesome()->font(Theme::toolIconFontSize));
    changeTurnaroundButton->setFixedSize(Theme::toolIconSize, Theme::toolIconSize);
    
    QPushButton *resetButton = new QPushButton(QChar(fa::trash));
    resetButton->setFont(Theme::awesome()->font(Theme::toolIconFontSize));
    resetButton->setFixedSize(Theme::toolIconSize, Theme::toolIconSize);
    
    toolButtonLayout->addWidget(undoButton);
    toolButtonLayout->addSpacing(10);
    toolButtonLayout->addWidget(addButton);
    toolButtonLayout->addWidget(selectButton);
    toolButtonLayout->addWidget(dragButton);
    toolButtonLayout->addWidget(zoomInButton);
    toolButtonLayout->addWidget(zoomOutButton);
    toolButtonLayout->addSpacing(10);
    toolButtonLayout->addWidget(changeTurnaroundButton);
    toolButtonLayout->addSpacing(30);
    toolButtonLayout->addWidget(resetButton);
    
    QLabel *dust3dJezzasoftLabel = new QLabel;
    QImage dust3dJezzasoftImage;
    dust3dJezzasoftImage.load(":/resources/dust3d_jezzasoft.png");
    dust3dJezzasoftLabel->setPixmap(QPixmap::fromImage(dust3dJezzasoftImage));
    
    QHBoxLayout *logoLayout = new QHBoxLayout;
    logoLayout->addWidget(dust3dJezzasoftLabel);
    logoLayout->setContentsMargins(0, 0, 0, 0);
    
    QVBoxLayout *mainLeftLayout = new QVBoxLayout;
    mainLeftLayout->setSpacing(0);
    mainLeftLayout->setContentsMargins(0, 0, 0, 0);
    mainLeftLayout->addLayout(toolButtonLayout);
    mainLeftLayout->addStretch();
    mainLeftLayout->addLayout(logoLayout);
    mainLeftLayout->addSpacing(10);
    
    SkeletonGraphicsWidget *graphicsWidget = new SkeletonGraphicsWidget(m_document);
    
    SkeletonGraphicsContainerWidget *containerWidget = new SkeletonGraphicsContainerWidget;
    
    containerWidget->setGraphicsWidget(graphicsWidget);
    QGridLayout *containerLayout = new QGridLayout;
    containerLayout->setSpacing(0);
    containerLayout->setContentsMargins(1, 0, 0, 0);
    containerLayout->addWidget(graphicsWidget);
    containerWidget->setLayout(containerLayout);
    containerWidget->setMinimumSize(400, 400);
    
    m_modelWidget = new ModelWidget(containerWidget);
    m_modelWidget->setMinimumSize(256, 256);
    m_modelWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_modelWidget->move(16, 16);
    m_modelWidget->setGraphicsFunctions(graphicsWidget);
    
    SkeletonPartListWidget *partListWidget = new SkeletonPartListWidget(m_document);
    partListWidget->setMaximumWidth(Theme::previewImageSize);
    
    //QTabWidget *firstTabWidget = new QTabWidget;
    //firstTabWidget->setFont(Theme::awesome()->font(Theme::toolIconFontSize * 3 / 4));
    //firstTabWidget->setMaximumWidth(200);
    //firstTabWidget->addTab(partListWidget, QChar(fa::puzzlepiece));
    //firstTabWidget->addTab(new QWidget, QChar(fa::history));
    //firstTabWidget->addTab(new QWidget, QChar(fa::wrench));
    
    //SkeletonEdgePropertyWidget *edgePropertyWidget = new SkeletonEdgePropertyWidget(m_document);
    //SkeletonNodePropertyWidget *nodePropertyWidget = new SkeletonNodePropertyWidget(m_document);
    
    //QVBoxLayout *propertyLayout = new QVBoxLayout;
    //propertyLayout->addWidget(edgePropertyWidget);
    //propertyLayout->addWidget(nodePropertyWidget);
    
    //QWidget *propertyWidget = new QWidget;
    //propertyWidget->setLayout(propertyLayout);
    
    //QTabWidget *secondTabWidget = new QTabWidget;
    //secondTabWidget->setFont(Theme::awesome()->font(Theme::toolIconFontSize * 3 / 4));
    //secondTabWidget->setMaximumWidth(200);
    //secondTabWidget->setMaximumHeight(90);
    //secondTabWidget->addTab(propertyWidget, QChar(fa::adjust));
    
    QVBoxLayout *mainRightLayout = new QVBoxLayout;
    mainRightLayout->setSpacing(0);
    //mainRightLayout->setContentsMargins(5, 5, 5, 5);
    mainRightLayout->setContentsMargins(0, 0, 0, 0);
    mainRightLayout->addWidget(partListWidget);
    //mainRightLayout->addWidget(firstTabWidget);
    //mainRightLayout->addWidget(secondTabWidget);
    
    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addLayout(mainLeftLayout);
    mainLayout->addWidget(containerWidget);
    mainLayout->addLayout(mainRightLayout);
    
    QWidget *centralWidget = new QWidget;
    centralWidget->setLayout(mainLayout);
    
    setCentralWidget(centralWidget);
    setWindowTitle(tr("Dust3D"));
    
    connect(containerWidget, &SkeletonGraphicsContainerWidget::containerSizeChanged,
        graphicsWidget, &SkeletonGraphicsWidget::canvasResized);
    
    connect(m_document, &SkeletonDocument::turnaroundChanged,
        graphicsWidget, &SkeletonGraphicsWidget::turnaroundChanged);
    
    connect(changeTurnaroundButton, &QPushButton::clicked, this, &SkeletonDocumentWindow::changeTurnaround);
    
    connect(addButton, &QPushButton::clicked, [=]() {
        m_document->setEditMode(SkeletonDocumentEditMode::Add);
    });
    
    connect(selectButton, &QPushButton::clicked, [=]() {
        m_document->setEditMode(SkeletonDocumentEditMode::Select);
    });
    
    connect(dragButton, &QPushButton::clicked, [=]() {
        m_document->setEditMode(SkeletonDocumentEditMode::Drag);
    });
    
    connect(zoomInButton, &QPushButton::clicked, [=]() {
        m_document->setEditMode(SkeletonDocumentEditMode::ZoomIn);
    });
    
    connect(zoomOutButton, &QPushButton::clicked, [=]() {
        m_document->setEditMode(SkeletonDocumentEditMode::ZoomOut);
    });
    
    connect(m_document, &SkeletonDocument::editModeChanged, graphicsWidget, &SkeletonGraphicsWidget::editModeChanged);
    
    connect(graphicsWidget, &SkeletonGraphicsWidget::addNode, m_document, &SkeletonDocument::addNode);
    connect(graphicsWidget, &SkeletonGraphicsWidget::scaleNodeByAddRadius, m_document, &SkeletonDocument::scaleNodeByAddRadius);
    connect(graphicsWidget, &SkeletonGraphicsWidget::moveNodeBy, m_document, &SkeletonDocument::moveNodeBy);
    connect(graphicsWidget, &SkeletonGraphicsWidget::removeNode, m_document, &SkeletonDocument::removeNode);
    connect(graphicsWidget, &SkeletonGraphicsWidget::setEditMode, m_document, &SkeletonDocument::setEditMode);
    connect(graphicsWidget, &SkeletonGraphicsWidget::removeEdge, m_document, &SkeletonDocument::removeEdge);
    connect(graphicsWidget, &SkeletonGraphicsWidget::addEdge, m_document, &SkeletonDocument::addEdge);
    
    connect(m_document, &SkeletonDocument::nodeAdded, graphicsWidget, &SkeletonGraphicsWidget::nodeAdded);
    connect(m_document, &SkeletonDocument::nodeRemoved, graphicsWidget, &SkeletonGraphicsWidget::nodeRemoved);
    connect(m_document, &SkeletonDocument::edgeAdded, graphicsWidget, &SkeletonGraphicsWidget::edgeAdded);
    connect(m_document, &SkeletonDocument::edgeRemoved, graphicsWidget, &SkeletonGraphicsWidget::edgeRemoved);
    connect(m_document, &SkeletonDocument::nodeRadiusChanged, graphicsWidget, &SkeletonGraphicsWidget::nodeRadiusChanged);
    connect(m_document, &SkeletonDocument::nodeOriginChanged, graphicsWidget, &SkeletonGraphicsWidget::nodeOriginChanged);
    connect(m_document, &SkeletonDocument::partVisibleStateChanged, graphicsWidget, &SkeletonGraphicsWidget::partVisibleStateChanged);
    
    connect(m_document, &SkeletonDocument::partListChanged, partListWidget, &SkeletonPartListWidget::partListChanged);
    connect(m_document, &SkeletonDocument::partPreviewChanged, partListWidget, &SkeletonPartListWidget::partPreviewChanged);
    connect(m_document, &SkeletonDocument::partLockStateChanged, partListWidget, &SkeletonPartListWidget::partLockStateChanged);
    connect(m_document, &SkeletonDocument::partVisibleStateChanged, partListWidget, &SkeletonPartListWidget::partVisibleStateChanged);
    connect(m_document, &SkeletonDocument::partSubdivStateChanged, partListWidget, &SkeletonPartListWidget::partSubdivStateChanged);
    
    connect(m_document, &SkeletonDocument::skeletonChanged, m_document, &SkeletonDocument::generateMesh);
    
    connect(m_document, &SkeletonDocument::resultMeshChanged, [=]() {
        m_modelWidget->updateMesh(m_document->takeResultMesh());
    });
    
    connect(graphicsWidget, &SkeletonGraphicsWidget::cursorChanged, [=]() {
        m_modelWidget->setCursor(graphicsWidget->cursor());
    });
    
    connect(this, &SkeletonDocumentWindow::initialized, m_document, &SkeletonDocument::uiReady);
}

SkeletonDocumentWindow::~SkeletonDocumentWindow()
{
}

void SkeletonDocumentWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    if (m_firstShow) {
        m_firstShow = false;
        emit initialized();
    }
}

void SkeletonDocumentWindow::changeTurnaround()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Turnaround Reference Image"),
        QString(),
        tr("Image Files (*.png *.jpg *.bmp)")).trimmed();
    if (fileName.isEmpty())
        return;
    QImage image;
    if (!image.load(fileName))
        return;
    m_document->updateTurnaround(image);
}
