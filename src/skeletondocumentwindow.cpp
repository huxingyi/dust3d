#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGridLayout>
#include <QToolBar>
#include <QPushButton>
#include <QFileDialog>
#include <QTabWidget>
#include <QBuffer>
#include <QMessageBox>
#include <QTimer>
#include <QMenuBar>
#include <QPointer>
#include <QApplication>
#include <set>
#include <QDesktopServices>
#include "skeletondocumentwindow.h"
#include "skeletongraphicswidget.h"
#include "skeletonpartlistwidget.h"
#include "theme.h"
#include "ds3file.h"
#include "skeletonsnapshot.h"
#include "skeletonxml.h"
#include "logbrowser.h"
#include "dust3dutil.h"
#include "aboutwidget.h"
#include "version.h"
#include "gltffile.h"

int SkeletonDocumentWindow::m_modelRenderWidgetInitialX = 16;
int SkeletonDocumentWindow::m_modelRenderWidgetInitialY = 16;
int SkeletonDocumentWindow::m_modelRenderWidgetInitialSize = 128;
int SkeletonDocumentWindow::m_skeletonRenderWidgetInitialX = SkeletonDocumentWindow::m_modelRenderWidgetInitialX + SkeletonDocumentWindow::m_modelRenderWidgetInitialSize + 16;
int SkeletonDocumentWindow::m_skeletonRenderWidgetInitialY = SkeletonDocumentWindow::m_modelRenderWidgetInitialY;
int SkeletonDocumentWindow::m_skeletonRenderWidgetInitialSize = SkeletonDocumentWindow::m_modelRenderWidgetInitialSize;

QPointer<LogBrowser> g_logBrowser;
std::set<SkeletonDocumentWindow *> g_documentWindows;
QTextBrowser *g_acknowlegementsWidget = nullptr;
AboutWidget *g_aboutWidget = nullptr;

void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (g_logBrowser)
        g_logBrowser->outputMessage(type, msg);
}

void SkeletonDocumentWindow::showAcknowlegements()
{
    if (!g_acknowlegementsWidget) {
        g_acknowlegementsWidget = new QTextBrowser;
        g_acknowlegementsWidget->setWindowTitle(APP_NAME);
        g_acknowlegementsWidget->setMinimumSize(QSize(400, 300));
        QFile file(":/ACKNOWLEDGEMENTS.html");
        file.open(QFile::ReadOnly | QFile::Text);
        QTextStream stream(&file);
        g_acknowlegementsWidget->setHtml(stream.readAll());
    }
    g_acknowlegementsWidget->show();
    g_acknowlegementsWidget->activateWindow();
    g_acknowlegementsWidget->raise();
}

void SkeletonDocumentWindow::showAbout()
{
    if (!g_aboutWidget) {
        g_aboutWidget = new AboutWidget;
    }
    g_aboutWidget->show();
    g_aboutWidget->activateWindow();
    g_aboutWidget->raise();
}

void SkeletonDocumentWindow::showTextureGuidePreview()
{
    if (!m_textureGuideWidget) {
        m_textureGuideWidget = new TextureGuideWidget;
        connect(m_textureGuideWidget, &TextureGuideWidget::regenerate, m_document, &SkeletonDocument::generateMesh);
    }
    if (m_textureGuideWidget->isHidden()) {
        if (m_document->textureGuideImage) {
            m_textureGuideWidget->updateGuideImage(*m_document->textureGuideImage);
        }
    }
    m_textureGuideWidget->show();
    m_textureGuideWidget->activateWindow();
    m_textureGuideWidget->raise();
}

SkeletonDocumentWindow::SkeletonDocumentWindow() :
    m_document(nullptr),
    m_firstShow(true),
    m_documentSaved(true),
    m_textureGuideWidget(nullptr),
    m_exportRequired(false)
{
    if (!g_logBrowser) {
        g_logBrowser = new LogBrowser;
        qInstallMessageHandler(&outputMessage);
    }
    
    g_documentWindows.insert(this);
    
    m_document = new SkeletonDocument;
    
    QVBoxLayout *toolButtonLayout = new QVBoxLayout;
    toolButtonLayout->setSpacing(0);
    toolButtonLayout->setContentsMargins(5, 10, 4, 0);
    
    QPushButton *undoButton = new QPushButton(QChar(fa::undo));
    initAwesomeButton(undoButton);
    
    QPushButton *addButton = new QPushButton(QChar(fa::plus));
    initAwesomeButton(addButton);
    
    QPushButton *selectButton = new QPushButton(QChar(fa::mousepointer));
    initAwesomeButton(selectButton);
    
    QPushButton *dragButton = new QPushButton(QChar(fa::handrocko));
    initAwesomeButton(dragButton);
    
    QPushButton *zoomInButton = new QPushButton(QChar(fa::searchplus));
    initAwesomeButton(zoomInButton);
    
    QPushButton *zoomOutButton = new QPushButton(QChar(fa::searchminus));
    initAwesomeButton(zoomOutButton);
    
    m_xlockButton = new QPushButton(QChar('X'));
    initLockButton(m_xlockButton);
    updateXlockButtonState();
    
    m_ylockButton = new QPushButton(QChar('Y'));
    initLockButton(m_ylockButton);
    updateYlockButtonState();
    
    m_zlockButton = new QPushButton(QChar('Z'));
    initLockButton(m_zlockButton);
    updateZlockButtonState();
    
    toolButtonLayout->addWidget(undoButton);
    toolButtonLayout->addSpacing(10);
    toolButtonLayout->addWidget(addButton);
    toolButtonLayout->addWidget(selectButton);
    toolButtonLayout->addWidget(dragButton);
    toolButtonLayout->addWidget(zoomInButton);
    toolButtonLayout->addWidget(zoomOutButton);
    toolButtonLayout->addSpacing(10);
    toolButtonLayout->addWidget(m_xlockButton);
    toolButtonLayout->addWidget(m_ylockButton);
    toolButtonLayout->addWidget(m_zlockButton);
    
    QLabel *verticalLogoLabel = new QLabel;
    QImage verticalLogoImage;
    verticalLogoImage.load(":/resources/dust3d_vertical.png");
    verticalLogoLabel->setPixmap(QPixmap::fromImage(verticalLogoImage));
    
    QHBoxLayout *logoLayout = new QHBoxLayout;
    logoLayout->addWidget(verticalLogoLabel);
    logoLayout->setContentsMargins(0, 0, 0, 0);
    
    QVBoxLayout *mainLeftLayout = new QVBoxLayout;
    mainLeftLayout->setSpacing(0);
    mainLeftLayout->setContentsMargins(0, 0, 0, 0);
    mainLeftLayout->addLayout(toolButtonLayout);
    mainLeftLayout->addStretch();
    mainLeftLayout->addLayout(logoLayout);
    mainLeftLayout->addSpacing(10);
    
    SkeletonGraphicsWidget *graphicsWidget = new SkeletonGraphicsWidget(m_document);
    m_graphicsWidget = graphicsWidget;
    
    SkeletonGraphicsContainerWidget *containerWidget = new SkeletonGraphicsContainerWidget;
    containerWidget->setGraphicsWidget(graphicsWidget);
    QGridLayout *containerLayout = new QGridLayout;
    containerLayout->setSpacing(0);
    containerLayout->setContentsMargins(1, 0, 0, 0);
    containerLayout->addWidget(graphicsWidget);
    containerWidget->setLayout(containerLayout);
    containerWidget->setMinimumSize(400, 400);
    
    m_modelRenderWidget = new ModelWidget(containerWidget);
    m_modelRenderWidget->setMinimumSize(SkeletonDocumentWindow::m_modelRenderWidgetInitialSize, SkeletonDocumentWindow::m_modelRenderWidgetInitialSize);
    m_modelRenderWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_modelRenderWidget->move(SkeletonDocumentWindow::m_modelRenderWidgetInitialX, SkeletonDocumentWindow::m_modelRenderWidgetInitialY);
    m_modelRenderWidget->setGraphicsFunctions(graphicsWidget);
    
    m_skeletonRenderWidget = new ModelWidget(containerWidget);
    m_skeletonRenderWidget->setMinimumSize(SkeletonDocumentWindow::m_skeletonRenderWidgetInitialSize, SkeletonDocumentWindow::m_skeletonRenderWidgetInitialSize);
    m_skeletonRenderWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_skeletonRenderWidget->move(SkeletonDocumentWindow::m_skeletonRenderWidgetInitialX, SkeletonDocumentWindow::m_skeletonRenderWidgetInitialY);
    m_skeletonRenderWidget->setGraphicsFunctions(graphicsWidget);
    m_skeletonRenderWidget->hide();
    
    SkeletonPartListWidget *partListWidget = new SkeletonPartListWidget(m_document, this);
    partListWidget->setWindowFlags(Qt::Tool);
    partListWidget->move(100, 200);
    partListWidget->show();
    
    //QVBoxLayout *mainRightLayout = new QVBoxLayout;
    //mainRightLayout->setSpacing(0);
    //mainRightLayout->setContentsMargins(0, 0, 0, 0);
    //mainRightLayout->addWidget(partListWidget);
    
    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addLayout(mainLeftLayout);
    mainLayout->addWidget(containerWidget);
    mainLayout->addSpacing(5);
    //mainLayout->addLayout(mainRightLayout);
    
    QWidget *centralWidget = new QWidget;
    centralWidget->setLayout(mainLayout);
    
    setCentralWidget(centralWidget);
    setWindowTitle(tr("Dust3D"));
    
    m_fileMenu = menuBar()->addMenu(tr("File"));
    
    m_newWindowAction = new QAction(tr("New Window"), this);
    connect(m_newWindowAction, &QAction::triggered, this, &SkeletonDocumentWindow::newWindow, Qt::QueuedConnection);
    m_fileMenu->addAction(m_newWindowAction);
    
    m_newDocumentAction = new QAction(tr("New"), this);
    connect(m_newDocumentAction, &QAction::triggered, this, &SkeletonDocumentWindow::newDocument);
    m_fileMenu->addAction(m_newDocumentAction);
    
    m_openAction = new QAction(tr("Open..."), this);
    connect(m_openAction, &QAction::triggered, this, &SkeletonDocumentWindow::open, Qt::QueuedConnection);
    m_fileMenu->addAction(m_openAction);
    
    m_saveAction = new QAction(tr("Save"), this);
    connect(m_saveAction, &QAction::triggered, this, &SkeletonDocumentWindow::save, Qt::QueuedConnection);
    m_fileMenu->addAction(m_saveAction);
    
    m_saveAsAction = new QAction(tr("Save As..."), this);
    connect(m_saveAsAction, &QAction::triggered, this, &SkeletonDocumentWindow::saveAs, Qt::QueuedConnection);
    m_fileMenu->addAction(m_saveAsAction);
    
    m_saveAllAction = new QAction(tr("Save All"), this);
    connect(m_saveAllAction, &QAction::triggered, this, &SkeletonDocumentWindow::saveAll, Qt::QueuedConnection);
    m_fileMenu->addAction(m_saveAllAction);
    
    m_fileMenu->addSeparator();
    
    m_exportMenu = m_fileMenu->addMenu(tr("Export"));
    
    m_exportModelAction = new QAction(tr("Wavefront (.obj)..."), this);
    connect(m_exportModelAction, &QAction::triggered, this, &SkeletonDocumentWindow::exportModelResult, Qt::QueuedConnection);
    m_exportMenu->addAction(m_exportModelAction);
    
    m_exportSkeletonAction = new QAction(tr("GL Transmission Format (.gltf)..."), this);
    connect(m_exportSkeletonAction, &QAction::triggered, this, &SkeletonDocumentWindow::exportGltfResult, Qt::QueuedConnection);
    m_exportMenu->addAction(m_exportSkeletonAction);
    
    m_changeTurnaroundAction = new QAction(tr("Change Turnaround..."), this);
    connect(m_changeTurnaroundAction, &QAction::triggered, this, &SkeletonDocumentWindow::changeTurnaround, Qt::QueuedConnection);
    m_fileMenu->addAction(m_changeTurnaroundAction);
    
    connect(m_fileMenu, &QMenu::aboutToShow, [=]() {
        m_exportModelAction->setEnabled(m_graphicsWidget->hasItems());
        m_exportSkeletonAction->setEnabled(m_graphicsWidget->hasItems());
    });
    
    m_editMenu = menuBar()->addMenu(tr("Edit"));
    
    m_addAction = new QAction(tr("Add..."), this);
    connect(m_addAction, &QAction::triggered, [=]() {
        m_document->setEditMode(SkeletonDocumentEditMode::Add);
    });
    m_editMenu->addAction(m_addAction);
    
    m_undoAction = new QAction(tr("Undo"), this);
    connect(m_undoAction, &QAction::triggered, m_document, &SkeletonDocument::undo);
    m_editMenu->addAction(m_undoAction);
    
    m_redoAction = new QAction(tr("Redo"), this);
    connect(m_redoAction, &QAction::triggered, m_document, &SkeletonDocument::redo);
    m_editMenu->addAction(m_redoAction);
    
    m_deleteAction = new QAction(tr("Delete"), this);
    connect(m_deleteAction, &QAction::triggered, m_graphicsWidget, &SkeletonGraphicsWidget::deleteSelected);
    m_editMenu->addAction(m_deleteAction);
    
    m_breakAction = new QAction(tr("Break"), this);
    connect(m_breakAction, &QAction::triggered, m_graphicsWidget, &SkeletonGraphicsWidget::breakSelected);
    m_editMenu->addAction(m_breakAction);
    
    m_connectAction = new QAction(tr("Connect"), this);
    connect(m_connectAction, &QAction::triggered, m_graphicsWidget, &SkeletonGraphicsWidget::connectSelected);
    m_editMenu->addAction(m_connectAction);
    
    m_cutAction = new QAction(tr("Cut"), this);
    connect(m_cutAction, &QAction::triggered, m_graphicsWidget, &SkeletonGraphicsWidget::cut);
    m_editMenu->addAction(m_cutAction);
    
    m_copyAction = new QAction(tr("Copy"), this);
    connect(m_copyAction, &QAction::triggered, m_graphicsWidget, &SkeletonGraphicsWidget::copy);
    m_editMenu->addAction(m_copyAction);
    
    m_pasteAction = new QAction(tr("Paste"), this);
    connect(m_pasteAction, &QAction::triggered, m_document, &SkeletonDocument::paste);
    m_editMenu->addAction(m_pasteAction);
    
    m_flipHorizontallyAction = new QAction(tr("H Flip"), this);
    connect(m_flipHorizontallyAction, &QAction::triggered, m_graphicsWidget, &SkeletonGraphicsWidget::flipHorizontally);
    m_editMenu->addAction(m_flipHorizontallyAction);
    
    m_flipVerticallyAction = new QAction(tr("V Flip"), this);
    connect(m_flipVerticallyAction, &QAction::triggered, m_graphicsWidget, &SkeletonGraphicsWidget::flipVertically);
    m_editMenu->addAction(m_flipVerticallyAction);
    
    m_alignToCenterAction = new QAction(tr("Align to Center"), this);
    connect(m_alignToCenterAction, &QAction::triggered, m_graphicsWidget, &SkeletonGraphicsWidget::alignSelectedToCenter);
    m_editMenu->addAction(m_alignToCenterAction);
    
    m_selectAllAction = new QAction(tr("Select All"), this);
    connect(m_selectAllAction, &QAction::triggered, m_graphicsWidget, &SkeletonGraphicsWidget::selectAll);
    m_editMenu->addAction(m_selectAllAction);
    
    m_selectPartAllAction = new QAction(tr("Select Part"), this);
    connect(m_selectPartAllAction, &QAction::triggered, m_graphicsWidget, &SkeletonGraphicsWidget::selectPartAll);
    m_editMenu->addAction(m_selectPartAllAction);
    
    m_unselectAllAction = new QAction(tr("Unselect All"), this);
    connect(m_unselectAllAction, &QAction::triggered, m_graphicsWidget, &SkeletonGraphicsWidget::unselectAll);
    m_editMenu->addAction(m_unselectAllAction);
    
    connect(m_editMenu, &QMenu::aboutToShow, [=]() {
        m_undoAction->setEnabled(m_document->undoable());
        m_redoAction->setEnabled(m_document->redoable());
        m_deleteAction->setEnabled(m_graphicsWidget->hasSelection());
        m_breakAction->setEnabled(m_graphicsWidget->hasEdgeSelection());
        m_connectAction->setEnabled(m_graphicsWidget->hasTwoDisconnectedNodesSelection());
        m_cutAction->setEnabled(m_graphicsWidget->hasSelection());
        m_copyAction->setEnabled(m_graphicsWidget->hasSelection());
        m_pasteAction->setEnabled(m_document->hasPastableContentInClipboard());
        m_flipHorizontallyAction->setEnabled(m_graphicsWidget->hasMultipleSelection());
        m_flipVerticallyAction->setEnabled(m_graphicsWidget->hasMultipleSelection());
        m_alignToCenterAction->setEnabled(m_graphicsWidget->hasSelection() && m_document->originSettled());
        m_selectAllAction->setEnabled(m_graphicsWidget->hasItems());
        m_selectPartAllAction->setEnabled(m_graphicsWidget->hasItems());
        m_unselectAllAction->setEnabled(m_graphicsWidget->hasSelection());
    });
    
    m_viewMenu = menuBar()->addMenu(tr("View"));
    
    m_resetModelWidgetPosAction = new QAction(tr("Show Model"), this);
    connect(m_resetModelWidgetPosAction, &QAction::triggered, [=]() {
        QRect parentRect = QRect(QPoint(0, 0), m_modelRenderWidget->parentWidget()->size());
        if (!parentRect.contains(m_modelRenderWidget->geometry().center())) {
            m_modelRenderWidget->move(SkeletonDocumentWindow::m_modelRenderWidgetInitialX, SkeletonDocumentWindow::m_modelRenderWidgetInitialY);
        }
    });
    m_viewMenu->addAction(m_resetModelWidgetPosAction);
    
    m_toggleWireframeAction = new QAction(tr("Toggle Wireframe"), this);
    connect(m_toggleWireframeAction, &QAction::triggered, [=]() {
        m_modelRenderWidget->toggleWireframe();
    });
    m_viewMenu->addAction(m_toggleWireframeAction);
    
    m_viewMenu->addSeparator();
    
    m_toggleSkeletonWidgetAction = new QAction(tr("Toggle Bones"), this);
    connect(m_toggleSkeletonWidgetAction, &QAction::triggered, [=]() {
        if (m_skeletonRenderWidget->isVisible()) {
            m_skeletonRenderWidget->hide();
        } else {
            QRect parentRect = QRect(QPoint(0, 0), m_skeletonRenderWidget->parentWidget()->size());
            if (!parentRect.contains(m_skeletonRenderWidget->geometry().center())) {
                m_skeletonRenderWidget->move(SkeletonDocumentWindow::m_skeletonRenderWidgetInitialX, SkeletonDocumentWindow::m_skeletonRenderWidgetInitialY);
            }
            m_skeletonRenderWidget->show();
        }
    });
    m_viewMenu->addAction(m_toggleSkeletonWidgetAction);
    
    m_viewMenu->addSeparator();
    
    m_showTextureGuidePreviewAction = new QAction(tr("Show Texture"), this);
    connect(m_showTextureGuidePreviewAction, &QAction::triggered, this, &SkeletonDocumentWindow::showTextureGuidePreview);
    m_viewMenu->addAction(m_showTextureGuidePreviewAction);
    
    m_viewMenu->addSeparator();
    
    m_showPartsListAction = new QAction(tr("Show Parts List"), this);
    connect(m_showPartsListAction, &QAction::triggered, [=]() {
        partListWidget->show();
    });
    m_viewMenu->addAction(m_showPartsListAction);
    
    m_viewMenu->addSeparator();
    
    m_showDebugDialogAction = new QAction(tr("Show Debug Dialog"), this);
    connect(m_showDebugDialogAction, &QAction::triggered, g_logBrowser, &LogBrowser::showDialog);
    m_viewMenu->addAction(m_showDebugDialogAction);
    
    m_helpMenu = menuBar()->addMenu(tr("Help"));
    
    m_viewSourceAction = new QAction(tr("Fork me on GitHub"), this);
    connect(m_viewSourceAction, &QAction::triggered, this, &SkeletonDocumentWindow::viewSource);
    m_helpMenu->addAction(m_viewSourceAction);
    
    m_helpMenu->addSeparator();
    
    m_seeReferenceGuideAction = new QAction(tr("Reference Guide"), this);
    connect(m_seeReferenceGuideAction, &QAction::triggered, this, &SkeletonDocumentWindow::seeReferenceGuide);
    m_helpMenu->addAction(m_seeReferenceGuideAction);
    
    m_helpMenu->addSeparator();
    
    m_aboutAction = new QAction(tr("About"), this);
    connect(m_aboutAction, &QAction::triggered, this, &SkeletonDocumentWindow::about);
    m_helpMenu->addAction(m_aboutAction);
    
    m_reportIssuesAction = new QAction(tr("Report Issues"), this);
    connect(m_reportIssuesAction, &QAction::triggered, this, &SkeletonDocumentWindow::reportIssues);
    m_helpMenu->addAction(m_reportIssuesAction);
    
    m_seeAcknowlegementsAction = new QAction(tr("Acknowlegements"), this);
    connect(m_seeAcknowlegementsAction, &QAction::triggered, this, &SkeletonDocumentWindow::seeAcknowlegements);
    m_helpMenu->addAction(m_seeAcknowlegementsAction);
    
    connect(containerWidget, &SkeletonGraphicsContainerWidget::containerSizeChanged,
        graphicsWidget, &SkeletonGraphicsWidget::canvasResized);
    
    connect(m_document, &SkeletonDocument::turnaroundChanged,
        graphicsWidget, &SkeletonGraphicsWidget::turnaroundChanged);
    
    connect(undoButton, &QPushButton::clicked, [=]() {
        m_document->undo();
    });
    
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
    
    connect(m_xlockButton, &QPushButton::clicked, [=]() {
        m_document->setXlockState(!m_document->xlocked);
    });
    connect(m_ylockButton, &QPushButton::clicked, [=]() {
        m_document->setYlockState(!m_document->ylocked);
    });
    connect(m_zlockButton, &QPushButton::clicked, [=]() {
        m_document->setZlockState(!m_document->zlocked);
    });
    
    connect(m_document, &SkeletonDocument::resultTextureChanged, [=]() {
        if (m_document->textureGuideImage && m_textureGuideWidget && m_textureGuideWidget->isVisible()) {
            m_textureGuideWidget->updateGuideImage(*m_document->textureGuideImage);
        }
    });
    
    connect(m_document, &SkeletonDocument::editModeChanged, graphicsWidget, &SkeletonGraphicsWidget::editModeChanged);
    
    connect(graphicsWidget, &SkeletonGraphicsWidget::addNode, m_document, &SkeletonDocument::addNode);
    connect(graphicsWidget, &SkeletonGraphicsWidget::scaleNodeByAddRadius, m_document, &SkeletonDocument::scaleNodeByAddRadius);
    connect(graphicsWidget, &SkeletonGraphicsWidget::moveNodeBy, m_document, &SkeletonDocument::moveNodeBy);
    connect(graphicsWidget, &SkeletonGraphicsWidget::removeNode, m_document, &SkeletonDocument::removeNode);
    connect(graphicsWidget, &SkeletonGraphicsWidget::setEditMode, m_document, &SkeletonDocument::setEditMode);
    connect(graphicsWidget, &SkeletonGraphicsWidget::removeEdge, m_document, &SkeletonDocument::removeEdge);
    connect(graphicsWidget, &SkeletonGraphicsWidget::addEdge, m_document, &SkeletonDocument::addEdge);
    connect(graphicsWidget, &SkeletonGraphicsWidget::groupOperationAdded, m_document, &SkeletonDocument::saveSnapshot);
    connect(graphicsWidget, &SkeletonGraphicsWidget::undo, m_document, &SkeletonDocument::undo);
    connect(graphicsWidget, &SkeletonGraphicsWidget::redo, m_document, &SkeletonDocument::redo);
    connect(graphicsWidget, &SkeletonGraphicsWidget::paste, m_document, &SkeletonDocument::paste);
    connect(graphicsWidget, &SkeletonGraphicsWidget::batchChangeBegin, m_document, &SkeletonDocument::batchChangeBegin);
    connect(graphicsWidget, &SkeletonGraphicsWidget::batchChangeEnd, m_document, &SkeletonDocument::batchChangeEnd);
    connect(graphicsWidget, &SkeletonGraphicsWidget::breakEdge, m_document, &SkeletonDocument::breakEdge);
    connect(graphicsWidget, &SkeletonGraphicsWidget::moveOriginBy, m_document, &SkeletonDocument::moveOriginBy);
    connect(graphicsWidget, &SkeletonGraphicsWidget::partChecked, m_document, &SkeletonDocument::partChecked);
    connect(graphicsWidget, &SkeletonGraphicsWidget::partUnchecked, m_document, &SkeletonDocument::partUnchecked);
    
    connect(graphicsWidget, &SkeletonGraphicsWidget::setPartLockState, m_document, &SkeletonDocument::setPartLockState);
    connect(graphicsWidget, &SkeletonGraphicsWidget::setPartVisibleState, m_document, &SkeletonDocument::setPartVisibleState);
    connect(graphicsWidget, &SkeletonGraphicsWidget::setPartSubdivState, m_document, &SkeletonDocument::setPartSubdivState);
    connect(graphicsWidget, &SkeletonGraphicsWidget::setPartDisableState, m_document, &SkeletonDocument::setPartDisableState);
    connect(graphicsWidget, &SkeletonGraphicsWidget::setPartXmirrorState, m_document, &SkeletonDocument::setPartXmirrorState);
    connect(graphicsWidget, &SkeletonGraphicsWidget::setPartRoundState, m_document, &SkeletonDocument::setPartRoundState);
    
    connect(graphicsWidget, &SkeletonGraphicsWidget::setXlockState, m_document, &SkeletonDocument::setXlockState);
    connect(graphicsWidget, &SkeletonGraphicsWidget::setYlockState, m_document, &SkeletonDocument::setYlockState);
    connect(graphicsWidget, &SkeletonGraphicsWidget::setZlockState, m_document, &SkeletonDocument::setZlockState);
    
    connect(graphicsWidget, &SkeletonGraphicsWidget::changeTurnaround, this, &SkeletonDocumentWindow::changeTurnaround);
    connect(graphicsWidget, &SkeletonGraphicsWidget::save, this, &SkeletonDocumentWindow::save);
    connect(graphicsWidget, &SkeletonGraphicsWidget::open, this, &SkeletonDocumentWindow::open);
    
    connect(m_document, &SkeletonDocument::nodeAdded, graphicsWidget, &SkeletonGraphicsWidget::nodeAdded);
    connect(m_document, &SkeletonDocument::nodeRemoved, graphicsWidget, &SkeletonGraphicsWidget::nodeRemoved);
    connect(m_document, &SkeletonDocument::edgeAdded, graphicsWidget, &SkeletonGraphicsWidget::edgeAdded);
    connect(m_document, &SkeletonDocument::edgeRemoved, graphicsWidget, &SkeletonGraphicsWidget::edgeRemoved);
    connect(m_document, &SkeletonDocument::nodeRadiusChanged, graphicsWidget, &SkeletonGraphicsWidget::nodeRadiusChanged);
    connect(m_document, &SkeletonDocument::nodeOriginChanged, graphicsWidget, &SkeletonGraphicsWidget::nodeOriginChanged);
    connect(m_document, &SkeletonDocument::partVisibleStateChanged, graphicsWidget, &SkeletonGraphicsWidget::partVisibleStateChanged);
    connect(m_document, &SkeletonDocument::partDisableStateChanged, graphicsWidget, &SkeletonGraphicsWidget::partVisibleStateChanged);
    connect(m_document, &SkeletonDocument::cleanup, graphicsWidget, &SkeletonGraphicsWidget::removeAllContent);
    connect(m_document, &SkeletonDocument::originChanged, graphicsWidget, &SkeletonGraphicsWidget::originChanged);
    connect(m_document, &SkeletonDocument::checkPart, graphicsWidget, &SkeletonGraphicsWidget::selectPartAllById);
    connect(m_document, &SkeletonDocument::enableBackgroundBlur, graphicsWidget, &SkeletonGraphicsWidget::enableBackgroundBlur);
    connect(m_document, &SkeletonDocument::disableBackgroundBlur, graphicsWidget, &SkeletonGraphicsWidget::disableBackgroundBlur);
    
    connect(m_document, &SkeletonDocument::partListChanged, partListWidget, &SkeletonPartListWidget::partListChanged);
    connect(m_document, &SkeletonDocument::partPreviewChanged, partListWidget, &SkeletonPartListWidget::partPreviewChanged);
    connect(m_document, &SkeletonDocument::partLockStateChanged, partListWidget, &SkeletonPartListWidget::partLockStateChanged);
    connect(m_document, &SkeletonDocument::partVisibleStateChanged, partListWidget, &SkeletonPartListWidget::partVisibleStateChanged);
    connect(m_document, &SkeletonDocument::partSubdivStateChanged, partListWidget, &SkeletonPartListWidget::partSubdivStateChanged);
    connect(m_document, &SkeletonDocument::partDisableStateChanged, partListWidget, &SkeletonPartListWidget::partDisableStateChanged);
    connect(m_document, &SkeletonDocument::partXmirrorStateChanged, partListWidget, &SkeletonPartListWidget::partXmirrorStateChanged);
    connect(m_document, &SkeletonDocument::partDeformThicknessChanged, partListWidget, &SkeletonPartListWidget::partDeformChanged);
    connect(m_document, &SkeletonDocument::partDeformWidthChanged, partListWidget, &SkeletonPartListWidget::partDeformChanged);
    connect(m_document, &SkeletonDocument::partRoundStateChanged, partListWidget, &SkeletonPartListWidget::partRoundStateChanged);
    connect(m_document, &SkeletonDocument::partColorStateChanged, partListWidget, &SkeletonPartListWidget::partColorStateChanged);
    connect(m_document, &SkeletonDocument::cleanup, partListWidget, &SkeletonPartListWidget::partListChanged);
    connect(m_document, &SkeletonDocument::partChecked, partListWidget, &SkeletonPartListWidget::partChecked);
    connect(m_document, &SkeletonDocument::partUnchecked, partListWidget, &SkeletonPartListWidget::partUnchecked);
    
    connect(m_document, &SkeletonDocument::skeletonChanged, m_document, &SkeletonDocument::generateMesh);
    connect(m_document, &SkeletonDocument::resultMeshChanged, m_document, &SkeletonDocument::postProcess);
    connect(m_document, &SkeletonDocument::postProcessedResultChanged, m_document, &SkeletonDocument::generateSkeleton);
    connect(m_document, &SkeletonDocument::postProcessedResultChanged, m_document, &SkeletonDocument::generateTexture);
    
    connect(m_document, &SkeletonDocument::resultMeshChanged, [=]() {
        m_modelRenderWidget->updateMesh(m_document->takeResultMesh());
    });
    connect(m_document, &SkeletonDocument::resultSkeletonChanged, [=]() {
        m_skeletonRenderWidget->updateMesh(m_document->takeResultSkeletonMesh());
    });
    
    connect(graphicsWidget, &SkeletonGraphicsWidget::cursorChanged, [=]() {
        m_modelRenderWidget->setCursor(graphicsWidget->cursor());
        m_skeletonRenderWidget->setCursor(graphicsWidget->cursor());
    });
    
    connect(m_document, &SkeletonDocument::skeletonChanged, this, &SkeletonDocumentWindow::documentChanged);
    connect(m_document, &SkeletonDocument::turnaroundChanged, this, &SkeletonDocumentWindow::documentChanged);
    
    connect(m_modelRenderWidget, &ModelWidget::customContextMenuRequested, [=](const QPoint &pos) {
        graphicsWidget->showContextMenu(graphicsWidget->mapFromGlobal(m_modelRenderWidget->mapToGlobal(pos)));
    });
    
    connect(m_document, &SkeletonDocument::xlockStateChanged, this, &SkeletonDocumentWindow::updateXlockButtonState);
    connect(m_document, &SkeletonDocument::ylockStateChanged, this, &SkeletonDocumentWindow::updateYlockButtonState);
    connect(m_document, &SkeletonDocument::zlockStateChanged, this, &SkeletonDocumentWindow::updateZlockButtonState);
    
    connect(m_document, &SkeletonDocument::exportReady, this, &SkeletonDocumentWindow::checkDelayedExport);

    connect(this, &SkeletonDocumentWindow::initialized, m_document, &SkeletonDocument::uiReady);
}

SkeletonDocumentWindow *SkeletonDocumentWindow::createDocumentWindow()
{
    SkeletonDocumentWindow *documentWindow = new SkeletonDocumentWindow();
    documentWindow->setAttribute(Qt::WA_DeleteOnClose);
    documentWindow->showMaximized();
    return documentWindow;
}

void SkeletonDocumentWindow::closeEvent(QCloseEvent *event)
{
    if (m_documentSaved) {
        event->accept();
        return;
    }
    
    QMessageBox::StandardButton answer = QMessageBox::question(this,
        APP_NAME,
        tr("Do you really want to close while there are unsaved changes?"),
        QMessageBox::Yes | QMessageBox::No);
    if (answer == QMessageBox::Yes)
        event->accept();
    else
        event->ignore();
}

void SkeletonDocumentWindow::setCurrentFilename(const QString &filename)
{
    m_currentFilename = filename;
    m_documentSaved = true;
    updateTitle();
}

void SkeletonDocumentWindow::updateTitle()
{
    QString appName = APP_NAME;
    QString appVer = APP_HUMAN_VER;
    setWindowTitle(QString("%1 %2 %3%4").arg(appName).arg(appVer).arg(m_currentFilename).arg(m_documentSaved ? "" : "*"));
}

void SkeletonDocumentWindow::documentChanged()
{
    if (m_documentSaved) {
        m_documentSaved = false;
        updateTitle();
    }
}

void SkeletonDocumentWindow::newWindow()
{
    SkeletonDocumentWindow::createDocumentWindow();
}

void SkeletonDocumentWindow::newDocument()
{
    if (!m_documentSaved) {
        QMessageBox::StandardButton answer = QMessageBox::question(this,
            APP_NAME,
            tr("Do you really want to create new document and lose the unsaved changes?"),
            QMessageBox::Yes | QMessageBox::No);
        if (answer != QMessageBox::Yes)
            return;
    }
    m_document->reset();
}

void SkeletonDocumentWindow::saveAs()
{
    QString filename = QFileDialog::getSaveFileName(this, QString(), QString(),
       tr("Dust3D Document (*.ds3)"));
    if (filename.isEmpty()) {
        return;
    }
    saveTo(filename);
}

void SkeletonDocumentWindow::saveAll()
{
    for (auto &window: g_documentWindows) {
        window->save();
    }
}

void SkeletonDocumentWindow::viewSource()
{
    QString url = APP_REPOSITORY_URL;
    qDebug() << "viewSource:" << url;
    QDesktopServices::openUrl(QUrl(url));
}

void SkeletonDocumentWindow::about()
{
    SkeletonDocumentWindow::showAbout();
}

void SkeletonDocumentWindow::reportIssues()
{
    QString url = APP_ISSUES_URL;
    qDebug() << "reportIssues:" << url;
    QDesktopServices::openUrl(QUrl(url));
}

void SkeletonDocumentWindow::seeReferenceGuide()
{
    QString url = APP_REFERENCE_GUIDE_URL;
    qDebug() << "referenceGuide:" << url;
    QDesktopServices::openUrl(QUrl(url));
}

void SkeletonDocumentWindow::seeAcknowlegements()
{
    SkeletonDocumentWindow::showAcknowlegements();
}

void SkeletonDocumentWindow::initAwesomeButton(QPushButton *button)
{
    button->setFont(Theme::awesome()->font(Theme::toolIconFontSize));
    button->setFixedSize(Theme::toolIconSize, Theme::toolIconSize);
    button->setStyleSheet("QPushButton {color: #f7d9c8}");
    button->setFocusPolicy(Qt::NoFocus);
}

void SkeletonDocumentWindow::initLockButton(QPushButton *button)
{
    QFont font;
    font.setWeight(QFont::Light);
    font.setPixelSize(Theme::toolIconFontSize);
    font.setBold(false);
    
    button->setFont(font);
    button->setFixedSize(Theme::toolIconSize, Theme::toolIconSize);
    button->setStyleSheet("QPushButton {color: #f7d9c8}");
    button->setFocusPolicy(Qt::NoFocus);
}

SkeletonDocumentWindow::~SkeletonDocumentWindow()
{
    if (m_textureGuideWidget) {
        delete m_textureGuideWidget;
        m_textureGuideWidget = nullptr;
    }
    g_documentWindows.erase(this);
}

void SkeletonDocumentWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    if (m_firstShow) {
        m_firstShow = false;
        updateTitle();
        m_document->saveSnapshot();
        m_graphicsWidget->setFocus();
        emit initialized();
    }
}

void SkeletonDocumentWindow::mousePressEvent(QMouseEvent *event)
{
    QMainWindow::mousePressEvent(event);
}

void SkeletonDocumentWindow::changeTurnaround()
{
    QString fileName = QFileDialog::getOpenFileName(this, QString(), QString(),
        tr("Image Files (*.png *.jpg *.bmp)")).trimmed();
    if (fileName.isEmpty())
        return;
    QImage image;
    if (!image.load(fileName))
        return;
    m_document->updateTurnaround(image);
}

void SkeletonDocumentWindow::save()
{
    saveTo(m_currentFilename);
}

void SkeletonDocumentWindow::saveTo(const QString &saveAsFilename)
{
    QString filename = saveAsFilename;
    
    if (filename.isEmpty()) {
        filename = QFileDialog::getSaveFileName(this, QString(), QString(),
           tr("Dust3D Document (*.ds3)"));
        if (filename.isEmpty()) {
            return;
        }
    }
    
    QApplication::setOverrideCursor(Qt::WaitCursor);
    
    Ds3FileWriter ds3Writer;
    
    QByteArray modelXml;
    QXmlStreamWriter stream(&modelXml);
    SkeletonSnapshot snapshot;
    m_document->toSnapshot(&snapshot);
    saveSkeletonToXmlStream(&snapshot, &stream);
    if (modelXml.size() > 0)
        ds3Writer.add("model.xml", "model", &modelXml);
    
    QByteArray imageByteArray;
    QBuffer pngBuffer(&imageByteArray);
    if (!m_document->turnaround.isNull()) {
        pngBuffer.open(QIODevice::WriteOnly);
        m_document->turnaround.save(&pngBuffer, "PNG");
        if (imageByteArray.size() > 0)
            ds3Writer.add("canvas.png", "asset", &imageByteArray);
    }
    
    if (ds3Writer.save(filename)) {
        setCurrentFilename(filename);
    }
    
    QApplication::restoreOverrideCursor();
}

void SkeletonDocumentWindow::open()
{
    if (!m_documentSaved) {
        QMessageBox::StandardButton answer = QMessageBox::question(this,
            APP_NAME,
            tr("Do you really want to open another file and lose the unsaved changes?"),
            QMessageBox::Yes | QMessageBox::No);
        if (answer != QMessageBox::Yes)
            return;
    }
    
    QString filename = QFileDialog::getOpenFileName(this, QString(), QString(),
        tr("Dust3D Document (*.ds3)"));
    if (filename.isEmpty())
        return;
    
    QApplication::setOverrideCursor(Qt::WaitCursor);
    Ds3FileReader ds3Reader(filename);
    for (int i = 0; i < ds3Reader.items().size(); ++i) {
        Ds3ReaderItem item = ds3Reader.items().at(i);
        if (item.type == "model") {
            QByteArray data;
            ds3Reader.loadItem(item.name, &data);
            QXmlStreamReader stream(data);
            SkeletonSnapshot snapshot;
            loadSkeletonFromXmlStream(&snapshot, stream);
            m_document->fromSnapshot(snapshot);
        } else if (item.type == "asset") {
            if (item.name == "canvas.png") {
                QByteArray data;
                ds3Reader.loadItem(item.name, &data);
                QImage image = QImage::fromData(data, "PNG");
                m_document->updateTurnaround(image);
            }
        }
    }
    QApplication::restoreOverrideCursor();
    
    setCurrentFilename(filename);
}

void SkeletonDocumentWindow::exportModelResult()
{
    QString filename = QFileDialog::getSaveFileName(this, QString(), QString(),
       tr("Wavefront (*.obj)"));
    if (filename.isEmpty()) {
        return;
    }
    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_modelRenderWidget->exportMeshAsObj(filename);
    QApplication::restoreOverrideCursor();
}

void SkeletonDocumentWindow::exportGltfResult()
{
    if (!m_document->isExportReady()) {
        m_exportRequired = true;
        return;
    }
    
    m_exportRequired = false;
    
    QString filename = QFileDialog::getSaveFileName(this, QString(), QString(),
       tr("GL Transmission Format (.gltf)"));
    if (filename.isEmpty()) {
        return;
    }
    QApplication::setOverrideCursor(Qt::WaitCursor);
    MeshResultContext skeletonResult = m_document->currentPostProcessedResultContext();
    GLTFFileWriter gltfFileWriter(skeletonResult, filename);
    gltfFileWriter.save();
    if (m_document->textureImage)
        m_document->textureImage->save(gltfFileWriter.textureFilenameInGltf());
    QFileInfo nameInfo(filename);
    QString guideFilename = nameInfo.path() + QDir::separator() + nameInfo.completeBaseName() + "-BORDER.png";
    if (m_document->textureBorderImage)
        m_document->textureBorderImage->save(guideFilename);
    QApplication::restoreOverrideCursor();
}

void SkeletonDocumentWindow::updateXlockButtonState()
{
    if (m_document->xlocked)
        m_xlockButton->setStyleSheet("QPushButton {color: #252525}");
    else
        m_xlockButton->setStyleSheet("QPushButton {color: #fc6621}");
}

void SkeletonDocumentWindow::updateYlockButtonState()
{
    if (m_document->ylocked)
        m_ylockButton->setStyleSheet("QPushButton {color: #252525}");
    else
        m_ylockButton->setStyleSheet("QPushButton {color: #2a5aac}");
}

void SkeletonDocumentWindow::updateZlockButtonState()
{
    if (m_document->zlocked)
        m_zlockButton->setStyleSheet("QPushButton {color: #252525}");
    else
        m_zlockButton->setStyleSheet("QPushButton {color: #aaebc4}");
}

void SkeletonDocumentWindow::checkDelayedExport()
{
    if (!m_exportRequired)
        return;
    exportGltfResult();
}
