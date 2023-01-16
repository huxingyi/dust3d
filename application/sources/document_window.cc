#include "document_window.h"
#include "about_widget.h"
#include "bone_manage_widget.h"
#include "cut_face_preview.h"
#include "document.h"
#include "document_saver.h"
#include "fbx_file.h"
#include "float_number_widget.h"
#include "flow_layout.h"
#include "glb_file.h"
#include "horizontal_line_widget.h"
#include "image_forever.h"
#include "log_browser.h"
#include "part_manage_widget.h"
#include "preferences.h"
#include "skeleton_graphics_widget.h"
#include "spinnable_toolbar_icon.h"
#include "theme.h"
#include "uv_map_generator.h"
#include "version.h"
#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGraphicsOpacityEffect>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPixmap>
#include <QPointer>
#include <QPushButton>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidgetAction>
#include <QtCore/qbuffer.h>
#include <dust3d/base/debug.h>
#include <dust3d/base/ds3_file.h>
#include <dust3d/base/snapshot.h>
#include <dust3d/base/snapshot_xml.h>
#include <map>

LogBrowser* g_logBrowser = nullptr;
std::map<DocumentWindow*, dust3d::Uuid> g_documentWindows;
QTextBrowser* g_acknowlegementsWidget = nullptr;
AboutWidget* g_aboutWidget = nullptr;
QTextBrowser* g_contributorsWidget = nullptr;
QTextBrowser* g_supportersWidget = nullptr;

void outputMessage(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    if (g_logBrowser)
        g_logBrowser->outputMessage(type, msg, context.file, context.line);
}

void ensureFileExtension(QString* filename, const QString extension)
{
    if (!filename->endsWith(extension)) {
        filename->append(extension);
    }
}

const std::map<DocumentWindow*, dust3d::Uuid>& DocumentWindow::documentWindows()
{
    return g_documentWindows;
}

Document* DocumentWindow::document()
{
    return m_document;
}

void DocumentWindow::showAcknowlegements()
{
    if (!g_acknowlegementsWidget) {
        g_acknowlegementsWidget = new QTextBrowser;
        g_acknowlegementsWidget->setWindowTitle(applicationTitle(tr("Acknowlegements")));
        g_acknowlegementsWidget->setMinimumSize(QSize(600, 300));
        QFile file(":/ACKNOWLEDGEMENTS.html");
        file.open(QFile::ReadOnly | QFile::Text);
        QTextStream stream(&file);
        g_acknowlegementsWidget->setHtml(stream.readAll());
    }
    g_acknowlegementsWidget->show();
    g_acknowlegementsWidget->activateWindow();
    g_acknowlegementsWidget->raise();
}

void DocumentWindow::showContributors()
{
    if (!g_contributorsWidget) {
        g_contributorsWidget = new QTextBrowser;
        g_contributorsWidget->setWindowTitle(applicationTitle(tr("Contributors")));
        g_contributorsWidget->setMinimumSize(QSize(600, 300));
        QFile authors(":/AUTHORS");
        authors.open(QFile::ReadOnly | QFile::Text);
        QFile contributors(":/CONTRIBUTORS");
        contributors.open(QFile::ReadOnly | QFile::Text);
        g_contributorsWidget->setHtml("<h1>AUTHORS</h1><pre>" + authors.readAll() + "</pre><h1>CONTRIBUTORS</h1><pre>" + contributors.readAll() + "</pre>");
    }
    g_contributorsWidget->show();
    g_contributorsWidget->activateWindow();
    g_contributorsWidget->raise();
}

void DocumentWindow::showSupporters()
{
    if (!g_supportersWidget) {
        g_supportersWidget = new QTextBrowser;
        g_supportersWidget->setWindowTitle(applicationTitle(tr("Supporters")));
        g_supportersWidget->setMinimumSize(QSize(600, 300));
        QFile supporters(":/SUPPORTERS");
        supporters.open(QFile::ReadOnly | QFile::Text);
        g_supportersWidget->setHtml("<h1>SUPPORTERS</h1><pre>" + supporters.readAll() + "</pre>");
    }
    g_supportersWidget->show();
    g_supportersWidget->activateWindow();
    g_supportersWidget->raise();
}

void DocumentWindow::showAbout()
{
    if (!g_aboutWidget) {
        g_aboutWidget = new AboutWidget;
    }
    g_aboutWidget->show();
    g_aboutWidget->activateWindow();
    g_aboutWidget->raise();
}

size_t DocumentWindow::total()
{
    return g_documentWindows.size();
}

DocumentWindow::DocumentWindow()
{
    if (!g_logBrowser) {
        g_logBrowser = new LogBrowser;
        qInstallMessageHandler(&outputMessage);
    }

    g_documentWindows.insert({ this, dust3d::Uuid::createUuid() });

    m_document = new Document;

    SkeletonGraphicsWidget* canvasGraphicsWidget = new SkeletonGraphicsWidget(m_document);
    m_canvasGraphicsWidget = canvasGraphicsWidget;

    QVBoxLayout* toolButtonLayout = new QVBoxLayout;
    toolButtonLayout->setSpacing(0);
    toolButtonLayout->setContentsMargins(5, 10, 4, 0);

    ToolbarButton* addButton = new ToolbarButton(":/resources/toolbar_add.svg");
    addButton->setToolTip(tr("Add node to canvas"));

    ToolbarButton* selectButton = new ToolbarButton(":/resources/toolbar_pointer.svg");
    selectButton->setToolTip(tr("Select node on canvas"));

    m_xLockButton = new ToolbarButton();
    m_xLockButton->setToolTip(tr("X axis locker"));
    updateXlockButtonState();

    m_yLockButton = new ToolbarButton();
    m_yLockButton->setToolTip(tr("Y axis locker"));
    updateYlockButtonState();

    m_zLockButton = new ToolbarButton();
    m_zLockButton->setToolTip(tr("Z axis locker"));
    updateZlockButtonState();

    m_radiusLockButton = new ToolbarButton();
    m_radiusLockButton->setToolTip(tr("Node radius locker"));
    updateRadiusLockButtonState();

    m_inprogressIndicator = new SpinnableToolbarIcon();
    updateInprogressIndicator();

    connect(m_document, &Document::meshGenerating, this, &DocumentWindow::updateInprogressIndicator);
    connect(m_document, &Document::resultMeshChanged, this, [=]() {
        m_isLastMeshGenerationSucceed = m_document->isMeshGenerationSucceed();
        updateInprogressIndicator();
    });
    connect(m_document, &Document::resultBodyBonePreviewMeshChanged, this, &DocumentWindow::updateInprogressIndicator);
    connect(m_document, &Document::resultComponentPreviewMeshesChanged, this, &DocumentWindow::generateComponentPreviewImages);
    connect(m_document, &Document::textureChanged, this, &DocumentWindow::generateComponentPreviewImages);
    connect(m_document, &Document::textureGenerating, this, &DocumentWindow::updateInprogressIndicator);
    connect(m_document, &Document::resultTextureChanged, this, &DocumentWindow::updateInprogressIndicator);
    connect(m_document, &Document::boneGenerating, this, &DocumentWindow::updateInprogressIndicator);
    connect(m_document, &Document::resultBoneChanged, this, &DocumentWindow::updateInprogressIndicator);
    connect(m_document, &Document::resultBonePreviewMeshesChanged, this, &DocumentWindow::generateBonePreviewImages);

    toolButtonLayout->addWidget(addButton);
    toolButtonLayout->addWidget(selectButton);
    toolButtonLayout->addSpacing(20);
    toolButtonLayout->addWidget(m_xLockButton);
    toolButtonLayout->addWidget(m_yLockButton);
    toolButtonLayout->addWidget(m_zLockButton);
    toolButtonLayout->addWidget(m_radiusLockButton);
    toolButtonLayout->addSpacing(20);
    toolButtonLayout->addWidget(m_inprogressIndicator);

    QLabel* verticalLogoLabel = new QLabel;
    QImage verticalLogoImage;
    verticalLogoImage.load(":/resources/dust3d-vertical.png");
    verticalLogoLabel->setPixmap(QPixmap::fromImage(verticalLogoImage));

    QHBoxLayout* logoLayout = new QHBoxLayout;
    logoLayout->addWidget(verticalLogoLabel);
    logoLayout->setContentsMargins(0, 0, 0, 0);

    QVBoxLayout* mainLeftLayout = new QVBoxLayout;
    mainLeftLayout->setSpacing(0);
    mainLeftLayout->setContentsMargins(0, 0, 0, 0);
    mainLeftLayout->addLayout(toolButtonLayout);
    mainLeftLayout->addStretch();
    mainLeftLayout->addLayout(logoLayout);
    mainLeftLayout->addSpacing(10);

    GraphicsContainerWidget* containerWidget = new GraphicsContainerWidget;
    containerWidget->setGraphicsWidget(canvasGraphicsWidget);
    QGridLayout* containerLayout = new QGridLayout;
    containerLayout->setSpacing(0);
    containerLayout->setContentsMargins(1, 0, 0, 0);
    containerLayout->addWidget(canvasGraphicsWidget);
    containerWidget->setLayout(containerLayout);
    containerWidget->setMinimumSize(400, 400);

    m_graphicsContainerWidget = containerWidget;

    m_modelRenderWidget = new ModelWidget(containerWidget);
    m_modelRenderWidget->setMoveAndZoomByWindow(false);
    m_modelRenderWidget->move(0, 0);
    m_modelRenderWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_modelRenderWidget->toggleWireframe();
    m_modelRenderWidget->disableCullFace();
    m_modelRenderWidget->setEyePosition(QVector3D(0.0, 0.0, -4.0));
    m_modelRenderWidget->setMoveToPosition(QVector3D(-0.5, -0.5, 0.0));

    connect(containerWidget, &GraphicsContainerWidget::containerSizeChanged,
        m_modelRenderWidget, &ModelWidget::canvasResized);

    m_canvasGraphicsWidget->setModelWidget(m_modelRenderWidget);
    containerWidget->setModelWidget(m_modelRenderWidget);

    setTabPosition(Qt::RightDockWidgetArea, QTabWidget::East);

    QDockWidget* partsDocker = new QDockWidget(tr("Parts"), this);
    partsDocker->setAllowedAreas(Qt::RightDockWidgetArea);
    m_partManageWidget = new PartManageWidget(m_document);
    partsDocker->setWidget(m_partManageWidget);
    addDockWidget(Qt::RightDockWidgetArea, partsDocker);

    QDockWidget* bonesDocker = new QDockWidget(tr("Bones"), this);
    m_boneManageWidget = new BoneManageWidget(m_document);
    bonesDocker->setWidget(m_boneManageWidget);
    bonesDocker->setAllowedAreas(Qt::RightDockWidgetArea);

    connect(bonesDocker, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        if (this->m_showBodyBonePreview == visible)
            return;
        this->m_showBodyBonePreview = visible;
        forceUpdateRenderModel();
    });

    tabifyDockWidget(partsDocker, bonesDocker);

    partsDocker->raise();

    QWidget* titleBarWidget = new QWidget;
    titleBarWidget->setFixedHeight(1);

    QHBoxLayout* titleBarLayout = new QHBoxLayout;
    titleBarLayout->addStretch();
    titleBarWidget->setLayout(titleBarLayout);

    QVBoxLayout* canvasLayout = new QVBoxLayout;
    canvasLayout->setSpacing(0);
    canvasLayout->setContentsMargins(0, 0, 0, 0);
    canvasLayout->addWidget(titleBarWidget);
    canvasLayout->addWidget(containerWidget);
    canvasLayout->setStretch(1, 1);

    QHBoxLayout* mainLayout = new QHBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addLayout(mainLeftLayout);
    mainLayout->addLayout(canvasLayout);
    mainLayout->addSpacing(3);

    QWidget* centralWidget = new QWidget;
    centralWidget->setLayout(mainLayout);

    setCentralWidget(centralWidget);
    setWindowTitle(APP_NAME);

    m_fileMenu = menuBar()->addMenu(tr("&File"));

    m_newWindowAction = new QAction(tr("New Window"), this);
    connect(m_newWindowAction, &QAction::triggered, this, &DocumentWindow::newWindow, Qt::QueuedConnection);
    m_fileMenu->addAction(m_newWindowAction);

    m_newDocumentAction = m_fileMenu->addAction(tr("&New"),
        this, &DocumentWindow::newDocument,
        QKeySequence::New);

    m_openAction = m_fileMenu->addAction(tr("&Open..."),
        this, &DocumentWindow::open,
        QKeySequence::Open);

    m_openExampleMenu = new QMenu(tr("Open Example"));
    std::vector<QString> exampleModels = {
        "Addax",
        "Banana",
        "Bicycle",
        "Dog",
        "Meerkat",
        "Mosquito",
        "Screwdriver",
        "Seagull"
    };
    for (const auto& model : exampleModels) {
        QAction* openModelAction = new QAction(model, this);
        connect(openModelAction, &QAction::triggered, this, [this, model]() {
            openExample("model-" + model.toLower().replace(QChar(' '), QChar('-')) + ".ds3");
        });
        m_openExampleMenu->addAction(openModelAction);
    }

    m_fileMenu->addMenu(m_openExampleMenu);

    m_saveAction = m_fileMenu->addAction(tr("&Save"),
        this, &DocumentWindow::save,
        QKeySequence::Save);

    m_saveAsAction = new QAction(tr("Save As..."), this);
    connect(m_saveAsAction, &QAction::triggered, this, &DocumentWindow::saveAs, Qt::QueuedConnection);
    m_fileMenu->addAction(m_saveAsAction);

    m_saveAllAction = new QAction(tr("Save All"), this);
    connect(m_saveAllAction, &QAction::triggered, this, &DocumentWindow::saveAll, Qt::QueuedConnection);
    m_fileMenu->addAction(m_saveAllAction);

    m_fileMenu->addSeparator();

    m_exportAsObjAction = new QAction(tr("Export as OBJ..."), this);
    connect(m_exportAsObjAction, &QAction::triggered, this, &DocumentWindow::exportObjResult, Qt::QueuedConnection);
    m_fileMenu->addAction(m_exportAsObjAction);

    m_exportAsGlbAction = new QAction(tr("Export as GLB..."), this);
    connect(m_exportAsGlbAction, &QAction::triggered, this, &DocumentWindow::exportGlbResult, Qt::QueuedConnection);
    m_fileMenu->addAction(m_exportAsGlbAction);

    m_exportAsFbxAction = new QAction(tr("Export as FBX..."), this);
    connect(m_exportAsFbxAction, &QAction::triggered, this, &DocumentWindow::exportFbxResult, Qt::QueuedConnection);
    m_fileMenu->addAction(m_exportAsFbxAction);

    m_fileMenu->addSeparator();

    m_changeTurnaroundAction = new QAction(tr("Change Background Image..."), this);
    connect(m_changeTurnaroundAction, &QAction::triggered, this, &DocumentWindow::changeTurnaround, Qt::QueuedConnection);
    m_fileMenu->addAction(m_changeTurnaroundAction);

    m_eraseTurnaroundAction = new QAction(tr("Erase Background Image"), this);
    connect(m_eraseTurnaroundAction, &QAction::triggered, this, &DocumentWindow::eraseTurnaround, Qt::QueuedConnection);
    m_fileMenu->addAction(m_eraseTurnaroundAction);

    m_fileMenu->addSeparator();

    for (int i = 0; i < Preferences::instance().maxRecentFiles(); ++i) {
        QAction* action = new QAction(this);
        action->setVisible(false);
        connect(action, &QAction::triggered, this, &DocumentWindow::openRecentFile, Qt::QueuedConnection);
        m_recentFileActions.push_back(action);
        m_fileMenu->addAction(action);
    }
    m_recentFileSeparatorAction = m_fileMenu->addSeparator();
    m_recentFileSeparatorAction->setVisible(false);
    updateRecentFileActions();

    m_quitAction = m_fileMenu->addAction(tr("&Quit"),
        this, &DocumentWindow::close,
        QKeySequence::Quit);

    connect(m_fileMenu, &QMenu::aboutToShow, [=]() {
        m_exportAsObjAction->setEnabled(m_canvasGraphicsWidget->hasItems());
        m_exportAsGlbAction->setEnabled(m_canvasGraphicsWidget->hasItems() && m_document->isExportReady());
        m_exportAsFbxAction->setEnabled(m_canvasGraphicsWidget->hasItems() && m_document->isExportReady());
    });

    m_viewMenu = menuBar()->addMenu(tr("&View"));

    m_toggleWireframeAction = new QAction(tr("Toggle Wireframe"), this);
    connect(m_toggleWireframeAction, &QAction::triggered, [=]() {
        m_modelRenderWidget->toggleWireframe();
    });
    m_viewMenu->addAction(m_toggleWireframeAction);

    m_toggleRotationAction = new QAction(tr("Toggle Rotation"), this);
    connect(m_toggleRotationAction, &QAction::triggered, [=]() {
        m_modelRenderWidget->toggleRotation();
    });
    m_viewMenu->addAction(m_toggleRotationAction);

    m_toggleColorAction = new QAction(tr("Toggle Color"), this);
    connect(m_toggleColorAction, &QAction::triggered, this, &DocumentWindow::toggleRenderColor);
    m_viewMenu->addAction(m_toggleColorAction);

    m_windowMenu = menuBar()->addMenu(tr("&Window"));

    m_showPartsListAction = new QAction(tr("Parts"), this);
    connect(m_showPartsListAction, &QAction::triggered, [=]() {
        partsDocker->show();
        partsDocker->raise();
    });
    m_windowMenu->addAction(m_showPartsListAction);

    m_showBonesListAction = new QAction(tr("Bones"), this);
    connect(m_showBonesListAction, &QAction::triggered, [=]() {
        bonesDocker->show();
        bonesDocker->raise();
    });
    m_windowMenu->addAction(m_showBonesListAction);

    QMenu* dialogsMenu = m_windowMenu->addMenu(tr("Dialogs"));
    connect(dialogsMenu, &QMenu::aboutToShow, [=]() {
        dialogsMenu->clear();
        if (this->m_dialogs.empty()) {
            QAction* action = dialogsMenu->addAction(tr("None"));
            action->setEnabled(false);
            return;
        }
        for (const auto& dialog : this->m_dialogs) {
            QAction* action = dialogsMenu->addAction(dialog->windowTitle());
            connect(action, &QAction::triggered, [=]() {
                dialog->show();
                dialog->raise();
            });
        }
    });

    m_showDebugDialogAction = new QAction(tr("Debug"), this);
    connect(m_showDebugDialogAction, &QAction::triggered, g_logBrowser, &LogBrowser::showDialog);
    m_windowMenu->addAction(m_showDebugDialogAction);

    m_helpMenu = menuBar()->addMenu(tr("&Help"));

    m_gotoHomepageAction = new QAction(tr("Dust3D Homepage"), this);
    connect(m_gotoHomepageAction, &QAction::triggered, this, &DocumentWindow::gotoHomepage);
    m_helpMenu->addAction(m_gotoHomepageAction);

    m_viewSourceAction = new QAction(tr("Source Code"), this);
    connect(m_viewSourceAction, &QAction::triggered, this, &DocumentWindow::viewSource);
    m_helpMenu->addAction(m_viewSourceAction);

    m_helpMenu->addSeparator();

    m_seeReferenceGuideAction = new QAction(tr("Reference Guide"), this);
    connect(m_seeReferenceGuideAction, &QAction::triggered, this, &DocumentWindow::seeReferenceGuide);
    m_helpMenu->addAction(m_seeReferenceGuideAction);

    m_helpMenu->addSeparator();

    m_aboutAction = new QAction(tr("About"), this);
    connect(m_aboutAction, &QAction::triggered, this, &DocumentWindow::about);
    m_helpMenu->addAction(m_aboutAction);

    m_reportIssuesAction = new QAction(tr("Report Issues"), this);
    connect(m_reportIssuesAction, &QAction::triggered, this, &DocumentWindow::reportIssues);
    m_helpMenu->addAction(m_reportIssuesAction);

    m_helpMenu->addSeparator();

    m_seeContributorsAction = new QAction(tr("Contributors"), this);
    connect(m_seeContributorsAction, &QAction::triggered, this, &DocumentWindow::seeContributors);
    m_helpMenu->addAction(m_seeContributorsAction);

    m_seeSupportersAction = new QAction(tr("Supporters"), this);
    connect(m_seeSupportersAction, &QAction::triggered, this, &DocumentWindow::seeSupporters);
    m_helpMenu->addAction(m_seeSupportersAction);

    m_seeAcknowlegementsAction = new QAction(tr("Acknowlegements"), this);
    connect(m_seeAcknowlegementsAction, &QAction::triggered, this, &DocumentWindow::seeAcknowlegements);
    m_helpMenu->addAction(m_seeAcknowlegementsAction);

    connect(containerWidget, &GraphicsContainerWidget::containerSizeChanged,
        canvasGraphicsWidget, &SkeletonGraphicsWidget::canvasResized);

    connect(m_document, &Document::turnaroundChanged,
        canvasGraphicsWidget, &SkeletonGraphicsWidget::turnaroundChanged);

    connect(addButton, &QPushButton::clicked, [=]() {
        m_document->setEditMode(Document::EditMode::Add);
    });

    connect(selectButton, &QPushButton::clicked, [=]() {
        m_document->setEditMode(Document::EditMode::Select);
    });

    connect(m_xLockButton, &QPushButton::clicked, [=]() {
        m_document->setXlockState(!m_document->xlocked);
    });
    connect(m_yLockButton, &QPushButton::clicked, [=]() {
        m_document->setYlockState(!m_document->ylocked);
    });
    connect(m_zLockButton, &QPushButton::clicked, [=]() {
        m_document->setZlockState(!m_document->zlocked);
    });
    connect(m_radiusLockButton, &QPushButton::clicked, [=]() {
        m_document->setRadiusLockState(!m_document->radiusLocked);
    });

    m_partListDockerVisibleSwitchConnection = connect(m_document, &Document::skeletonChanged, [=]() {
        if (m_canvasGraphicsWidget->hasItems()) {
            if (partsDocker->isHidden())
                partsDocker->show();
            disconnect(m_partListDockerVisibleSwitchConnection);
        }
    });

    connect(m_document, &Document::editModeChanged, canvasGraphicsWidget, &SkeletonGraphicsWidget::editModeChanged);

    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::shortcutToggleWireframe, [=]() {
        m_modelRenderWidget->toggleWireframe();
    });

    //connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::shortcutToggleFlatShading, [=]() {
    //    Preferences::instance().setFlatShading(!Preferences::instance().flatShading());
    //});

    //connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::shortcutToggleRotation, this, &DocumentWindow::toggleRotation);

    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::zoomRenderedModelBy, m_modelRenderWidget, &ModelWidget::zoom);

    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::addNode, m_document, &Document::addNode);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::scaleNodeByAddRadius, m_document, &Document::scaleNodeByAddRadius);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::moveNodeBy, m_document, &Document::moveNodeBy);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::setNodeBoneJointState, m_document, &Document::setNodeBoneJointState);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::setNodeOrigin, m_document, &Document::setNodeOrigin);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::removeNode, m_document, &Document::removeNode);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::removePart, m_document, &Document::removePart);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::setEditMode, m_document, &Document::setEditMode);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::removeEdge, m_document, &Document::removeEdge);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::addEdge, m_document, &Document::addEdge);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::groupOperationAdded, m_document, &Document::saveSnapshot);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::undo, m_document, &Document::undo);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::redo, m_document, &Document::redo);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::paste, m_document, &Document::paste);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::batchChangeBegin, m_document, &Document::batchChangeBegin);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::batchChangeEnd, m_document, &Document::batchChangeEnd);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::breakEdge, m_document, &Document::breakEdge);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::reduceNode, m_document, &Document::reduceNode);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::reverseEdge, m_document, &Document::reverseEdge);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::moveOriginBy, m_document, &Document::moveOriginBy);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::partChecked, m_document, &Document::partChecked);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::partUnchecked, m_document, &Document::partUnchecked);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::switchNodeXZ, m_document, &Document::switchNodeXZ);

    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::setPartLockState, m_document, &Document::setPartLockState);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::setPartVisibleState, m_document, &Document::setPartVisibleState);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::setPartSubdivState, m_document, &Document::setPartSubdivState);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::setPartChamferState, m_document, &Document::setPartChamferState);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::setPartColorState, m_document, [this](const dust3d::Uuid& partId, bool hasColor, QColor color) {
        const Document::Part* part = this->m_document->findPart(partId);
        if (nullptr == part)
            return;
        this->m_document->setComponentColorState(part->componentId, hasColor, color);
    });
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::setPartDisableState, m_document, &Document::setPartDisableState);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::setPartXmirrorState, m_document, &Document::setPartXmirrorState);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::setPartRoundState, m_document, &Document::setPartRoundState);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::setPartWrapState, m_document, &Document::setPartCutRotation);

    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::addNodesToBone, m_document, &Document::addNodesToBone);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::removeNodesFromBone, m_document, &Document::removeNodesFromBone);

    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::nodePicked, m_document, &Document::pickBoneNode);

    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::setXlockState, m_document, &Document::setXlockState);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::setYlockState, m_document, &Document::setYlockState);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::setZlockState, m_document, &Document::setZlockState);

    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::enableAllPositionRelatedLocks, m_document, &Document::enableAllPositionRelatedLocks);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::disableAllPositionRelatedLocks, m_document, &Document::disableAllPositionRelatedLocks);

    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::changeTurnaround, this, &DocumentWindow::changeTurnaround);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::open, this, &DocumentWindow::open);

    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::showOrHideAllComponents, m_document, &Document::showOrHideAllComponents);

    connect(m_document, &Document::nodeAdded, canvasGraphicsWidget, &SkeletonGraphicsWidget::nodeAdded);
    connect(m_document, &Document::nodeRemoved, canvasGraphicsWidget, &SkeletonGraphicsWidget::nodeRemoved);
    connect(m_document, &Document::edgeAdded, canvasGraphicsWidget, &SkeletonGraphicsWidget::edgeAdded);
    connect(m_document, &Document::edgeRemoved, canvasGraphicsWidget, &SkeletonGraphicsWidget::edgeRemoved);
    connect(m_document, &Document::nodeRadiusChanged, canvasGraphicsWidget, &SkeletonGraphicsWidget::nodeRadiusChanged);
    connect(m_document, &Document::nodeBoneJointStateChanged, canvasGraphicsWidget, &SkeletonGraphicsWidget::nodeBoneJointStateChanged);
    connect(m_document, &Document::nodeOriginChanged, canvasGraphicsWidget, &SkeletonGraphicsWidget::nodeOriginChanged);
    connect(m_document, &Document::edgeReversed, canvasGraphicsWidget, &SkeletonGraphicsWidget::edgeReversed);
    connect(m_document, &Document::partVisibleStateChanged, canvasGraphicsWidget, &SkeletonGraphicsWidget::partVisibleStateChanged);
    connect(m_document, &Document::partDisableStateChanged, canvasGraphicsWidget, &SkeletonGraphicsWidget::partVisibleStateChanged);
    connect(m_document, &Document::cleanup, canvasGraphicsWidget, &SkeletonGraphicsWidget::removeAllContent);
    connect(m_document, &Document::originChanged, canvasGraphicsWidget, &SkeletonGraphicsWidget::originChanged);
    connect(m_document, &Document::checkPart, canvasGraphicsWidget, &SkeletonGraphicsWidget::selectPartAllById);
    connect(m_document, &Document::enableBackgroundBlur, canvasGraphicsWidget, &SkeletonGraphicsWidget::enableBackgroundBlur);
    connect(m_document, &Document::disableBackgroundBlur, canvasGraphicsWidget, &SkeletonGraphicsWidget::disableBackgroundBlur);
    connect(m_document, &Document::uncheckAll, canvasGraphicsWidget, &SkeletonGraphicsWidget::unselectAll);
    connect(m_document, &Document::checkNode, canvasGraphicsWidget, &SkeletonGraphicsWidget::addSelectNode);
    connect(m_document, &Document::checkEdge, canvasGraphicsWidget, &SkeletonGraphicsWidget::addSelectEdge);

    connect(m_partManageWidget, &PartManageWidget::unselectAllOnCanvas, canvasGraphicsWidget, &SkeletonGraphicsWidget::unselectAll);
    connect(m_partManageWidget, &PartManageWidget::selectPartOnCanvas, canvasGraphicsWidget, &SkeletonGraphicsWidget::addPartToSelection);

    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::partComponentChecked, m_partManageWidget, &PartManageWidget::selectComponentByPartId);

    connect(m_boneManageWidget, &BoneManageWidget::selectNodeOnCanvas, canvasGraphicsWidget, &SkeletonGraphicsWidget::addNodeToSelection);

    connect(m_document, &Document::skeletonChanged, m_document, &Document::generateMesh);
    connect(m_document, &Document::textureChanged, m_document, &Document::generateTexture);
    connect(m_document, &Document::resultMeshChanged, m_document, &Document::generateTexture);
    connect(m_document, &Document::rigChanged, m_document, &Document::generateBone);
    connect(m_document, &Document::resultMeshChanged, m_document, &Document::generateBone);
    connect(m_document, &Document::resultTextureChanged, this, &DocumentWindow::updateRenderModel);
    connect(m_document, &Document::resultBodyBonePreviewMeshChanged, this, &DocumentWindow::updateRenderModel);

    connect(m_document, &Document::resultMeshChanged, this, &DocumentWindow::updateRenderModel);
    connect(m_document, &Document::resultMeshChanged, this, &DocumentWindow::updateRenderWireframe);

    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::cursorChanged, [=]() {
        m_modelRenderWidget->setCursor(canvasGraphicsWidget->cursor());
        containerWidget->setCursor(canvasGraphicsWidget->cursor());
    });

    connect(m_document, &Document::skeletonChanged, this, &DocumentWindow::documentChanged);
    connect(m_document, &Document::textureChanged, this, &DocumentWindow::documentChanged);
    connect(m_document, &Document::rigChanged, this, &DocumentWindow::documentChanged);
    connect(m_document, &Document::turnaroundChanged, this, &DocumentWindow::documentChanged);
    connect(m_document, &Document::optionsChanged, this, &DocumentWindow::documentChanged);

    connect(m_modelRenderWidget, &ModelWidget::customContextMenuRequested, [=](const QPoint& pos) {
        canvasGraphicsWidget->showContextMenu(canvasGraphicsWidget->mapFromGlobal(m_modelRenderWidget->mapToGlobal(pos)));
    });

    connect(m_document, &Document::xlockStateChanged, this, &DocumentWindow::updateXlockButtonState);
    connect(m_document, &Document::ylockStateChanged, this, &DocumentWindow::updateYlockButtonState);
    connect(m_document, &Document::zlockStateChanged, this, &DocumentWindow::updateZlockButtonState);
    connect(m_document, &Document::radiusLockStateChanged, this, &DocumentWindow::updateRadiusLockButtonState);

    initializeShortcuts();

    connect(this, &DocumentWindow::initialized, m_document, &Document::uiReady);

    QTimer* timer = new QTimer(this);
    timer->setInterval(250);
    connect(timer, &QTimer::timeout, [=] {
        QWidget* focusedWidget = QApplication::focusWidget();
        if (nullptr == focusedWidget && isActiveWindow())
            canvasGraphicsWidget->setFocus();
    });
    timer->start();
}

void DocumentWindow::updateInprogressIndicator()
{
    bool inprogress = m_document->isMeshGenerating() || m_document->isTextureGenerating() || m_document->isBoneGenerating() || nullptr != m_componentPreviewImagesGenerator || nullptr != m_componentPreviewImagesDecorator;
    if (inprogress == m_inprogressIndicator->isSpinning())
        return;
    m_inprogressIndicator->showSpinner(inprogress);
    emit workingStatusChanged(inprogress);
}

void DocumentWindow::toggleRotation()
{
    if (nullptr == m_canvasGraphicsWidget)
        return;
    m_canvasGraphicsWidget->setRotated(!m_canvasGraphicsWidget->rotated());
}

DocumentWindow* DocumentWindow::createDocumentWindow()
{
    DocumentWindow* documentWindow = new DocumentWindow();
    documentWindow->setAttribute(Qt::WA_DeleteOnClose);

    QSize size = Preferences::instance().documentWindowSize();
    if (size.isValid()) {
        documentWindow->resize(size);
        documentWindow->show();
    } else {
        documentWindow->showMaximized();
    }

    return documentWindow;
}

void DocumentWindow::closeEvent(QCloseEvent* event)
{
    if (!m_documentSaved) {
        QMessageBox::StandardButton answer = QMessageBox::question(this,
            APP_NAME,
            tr("Do you really want to close while there are unsaved changes?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (answer == QMessageBox::No) {
            event->ignore();
            return;
        }
    }

    QSize saveSize;
    if (!isMaximized())
        saveSize = size();
    Preferences::instance().setDocumentWindowSize(saveSize);

    event->accept();
}

void DocumentWindow::setCurrentFilename(const QString& filename)
{
    m_currentFilename = filename;
    m_documentSaved = true;
    updateTitle();
}

void DocumentWindow::updateTitle()
{
    QString appName = APP_NAME;
    QString appVer = APP_HUMAN_VER;
    setWindowTitle(QString("%1 %2 %3%4").arg(appName).arg(appVer).arg(m_currentFilename).arg(m_documentSaved ? "" : "*"));
}

void DocumentWindow::documentChanged()
{
    if (m_documentSaved) {
        m_documentSaved = false;
        updateTitle();
    }
}

void DocumentWindow::newWindow()
{
    DocumentWindow::createDocumentWindow();
}

void DocumentWindow::newDocument()
{
    if (!m_documentSaved) {
        QMessageBox::StandardButton answer = QMessageBox::question(this,
            APP_NAME,
            tr("Do you really want to create new document and lose the unsaved changes?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (answer != QMessageBox::Yes)
            return;
    }
    reset();
}

void DocumentWindow::reset()
{
    m_document->clearHistories();
    m_document->reset();
    m_document->clearTurnaround();
    m_document->saveSnapshot();

    m_currentUpdatedMeshId = 0;
    m_currentUpdatedWireframeId = 0;
}

void DocumentWindow::saveAs()
{
    QString filename = QFileDialog::getSaveFileName(this, QString(), QString(),
        tr("Dust3D Document (*.ds3)"));
    if (filename.isEmpty()) {
        return;
    }
    ensureFileExtension(&filename, ".ds3");
    saveTo(filename);
}

void DocumentWindow::saveAll()
{
    for (auto& it : g_documentWindows) {
        it.first->save();
    }
}

void DocumentWindow::viewSource()
{
    QString url = APP_REPOSITORY_URL;
    qDebug() << "viewSource:" << url;
    QDesktopServices::openUrl(QUrl(url));
}

void DocumentWindow::gotoHomepage()
{
    QString url = APP_HOMEPAGE_URL;
    qDebug() << "gotoHomepage:" << url;
    QDesktopServices::openUrl(QUrl(url));
}

void DocumentWindow::about()
{
    DocumentWindow::showAbout();
}

void DocumentWindow::reportIssues()
{
    QString url = APP_ISSUES_URL;
    qDebug() << "reportIssues:" << url;
    QDesktopServices::openUrl(QUrl(url));
}

void DocumentWindow::seeReferenceGuide()
{
    QString url = APP_REFERENCE_GUIDE_URL;
    qDebug() << "referenceGuide:" << url;
    QDesktopServices::openUrl(QUrl(url));
}

void DocumentWindow::seeAcknowlegements()
{
    DocumentWindow::showAcknowlegements();
}

void DocumentWindow::seeContributors()
{
    DocumentWindow::showContributors();
}

void DocumentWindow::seeSupporters()
{
    DocumentWindow::showSupporters();
}

DocumentWindow::~DocumentWindow()
{
    emit uninialized();
    g_documentWindows.erase(this);
    delete m_document;
}

void DocumentWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);
    if (m_firstShow) {
        m_firstShow = false;
        updateTitle();
        m_document->saveSnapshot();
        m_canvasGraphicsWidget->setFocus();
        emit initialized();
    }
}

void DocumentWindow::mousePressEvent(QMouseEvent* event)
{
    QMainWindow::mousePressEvent(event);
}

void DocumentWindow::changeTurnaround()
{
    QStringList fileNames = QFileDialog::getOpenFileNames(this, QString(), QString(),
        tr("Image Files (*.png *.jpg *.bmp)"));
    if (fileNames.isEmpty())
        return;

    if (2 == fileNames.size()) {
        QImage frontImage;
        if (!frontImage.load(fileNames[0]))
            return;
        QImage sideImage;
        if (!sideImage.load(fileNames[1]))
            return;
        frontImage = frontImage.scaledToHeight(512);
        sideImage = sideImage.scaledToHeight(512);
        QImage combined(frontImage.width() + sideImage.width(), frontImage.height(), QImage::Format_RGB32);
        {
            QPainter painter(&combined);
            painter.drawImage(0, 0, frontImage);
            painter.drawImage(frontImage.width(), 0, sideImage);
        }
        m_document->updateTurnaround(combined);
        return;
    }

    QImage image;
    if (!image.load(fileNames[0]))
        return;
    m_document->updateTurnaround(image);
}

void DocumentWindow::eraseTurnaround()
{
    if (m_document->turnaround.isNull())
        return;

    QMessageBox::StandardButton answer = QMessageBox::question(this,
        APP_NAME,
        tr("Do you really want to erase background image? This can not be undo."),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (answer != QMessageBox::Yes)
        return;

    QImage image(m_document->turnaround.width(), m_document->turnaround.height(), QImage::Format_RGBA8888);
    image.fill(0);
    m_document->updateTurnaround(image);
}

void DocumentWindow::save()
{
    saveTo(m_currentFilename);
}

void DocumentWindow::saveTo(const QString& saveAsFilename)
{
    QString filename = saveAsFilename;

    if (filename.isEmpty()) {
        filename = QFileDialog::getSaveFileName(this, QString(), QString(),
            tr("Dust3D Document (*.ds3)"));
        if (filename.isEmpty()) {
            return;
        }
        ensureFileExtension(&filename, ".ds3");
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    dust3d::Snapshot snapshot;
    m_document->toSnapshot(&snapshot);
    if (DocumentSaver::save(&filename,
            &snapshot,
            (!m_document->turnaround.isNull() && m_document->turnaroundPngByteArray.size() > 0) ? &m_document->turnaroundPngByteArray : nullptr)) {
        setCurrentFilename(filename);
    }
    QApplication::restoreOverrideCursor();
}

void DocumentWindow::openPathAs(const QString& path, const QString& asName)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);

    reset();

    QFile file(path);
    file.open(QFile::ReadOnly);
    QByteArray fileData = file.readAll();

    dust3d::Ds3FileReader ds3Reader((const std::uint8_t*)fileData.data(), fileData.size());
    for (int i = 0; i < (int)ds3Reader.items().size(); ++i) {
        const dust3d::Ds3ReaderItem& item = ds3Reader.items()[i];
        qDebug() << "[" << i << "]item.name:" << item.name << "item.type:" << item.type;
        if (item.type == "asset") {
            if (dust3d::String::startsWith(item.name, "images/")) {
                std::string filename = dust3d::String::split(item.name, '/')[1];
                std::string imageIdString = dust3d::String::split(filename, '.')[0];
                dust3d::Uuid imageId = dust3d::Uuid(imageIdString);
                if (!imageId.isNull()) {
                    std::vector<std::uint8_t> data;
                    ds3Reader.loadItem(item.name, &data);
                    QImage image = QImage::fromData(data.data(), (int)data.size(), "PNG");
                    (void)ImageForever::add(&image, imageId);
                }
            }
        }
    }

    for (int i = 0; i < (int)ds3Reader.items().size(); ++i) {
        const dust3d::Ds3ReaderItem& item = ds3Reader.items()[i];
        if (item.type == "model") {
            std::vector<std::uint8_t> data;
            ds3Reader.loadItem(item.name, &data);
            data.push_back('\0');
            dust3d::Snapshot snapshot;
            loadSnapshotFromXmlString(&snapshot, (char*)data.data());
            m_document->fromSnapshot(snapshot);
            m_document->saveSnapshot();
        } else if (item.type == "asset") {
            if (item.name == "canvas.png") {
                std::vector<std::uint8_t> data;
                ds3Reader.loadItem(item.name, &data);
                m_document->updateTurnaround(QImage::fromData(data.data(), (int)data.size(), "PNG"));
            }
        }
    }

    QApplication::restoreOverrideCursor();

    if (!asName.isEmpty()) {
        Preferences::instance().setCurrentFile(path);
        for (auto& it : g_documentWindows) {
            it.first->updateRecentFileActions();
        }
    }
    setCurrentFilename(asName);
}

void DocumentWindow::openExample(const QString& modelName)
{
    if (!m_documentSaved) {
        QMessageBox::StandardButton answer = QMessageBox::question(this,
            APP_NAME,
            tr("Do you really want to open example and lose the unsaved changes?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (answer != QMessageBox::Yes)
            return;
    }

    openPathAs(":/resources/" + modelName, "");
}

void DocumentWindow::open()
{
    if (!m_documentSaved) {
        QMessageBox::StandardButton answer = QMessageBox::question(this,
            APP_NAME,
            tr("Do you really want to open another file and lose the unsaved changes?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (answer != QMessageBox::Yes)
            return;
    }

    QString filename = QFileDialog::getOpenFileName(this, QString(), QString(),
        tr("Dust3D Document (*.ds3)"));
    if (filename.isEmpty())
        return;

    openPathAs(filename, filename);
}

void DocumentWindow::exportObjResult()
{
    QString filename = QFileDialog::getSaveFileName(this, QString(), QString(),
        tr("Wavefront (*.obj)"));
    if (filename.isEmpty()) {
        return;
    }
    ensureFileExtension(&filename, ".obj");
    exportObjToFilename(filename);
}

void DocumentWindow::exportObjToFilename(const QString& filename)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    ModelMesh* resultMesh = m_document->takeResultMesh();
    if (nullptr != resultMesh) {
        resultMesh->exportAsObj(filename);
        delete resultMesh;
    }
    QApplication::restoreOverrideCursor();
}

void DocumentWindow::exportFbxResult()
{
    QString filename = QFileDialog::getSaveFileName(this, QString(), QString(),
        tr("Autodesk FBX (*.fbx)"));
    if (filename.isEmpty()) {
        return;
    }
    ensureFileExtension(&filename, ".fbx");
    exportFbxToFilename(filename);
}

void DocumentWindow::exportFbxToFilename(const QString& filename)
{
    if (!m_document->isExportReady()) {
        qDebug() << "Export but document is not export ready";
        return;
    }
    QApplication::setOverrideCursor(Qt::WaitCursor);
    dust3d::Object skeletonResult = m_document->currentUvMappedObject();
    FbxFileWriter fbxFileWriter(skeletonResult,
        filename,
        m_document->textureImage,
        m_document->textureNormalImage,
        m_document->textureMetalnessImage,
        m_document->textureRoughnessImage,
        m_document->textureAmbientOcclusionImage);
    fbxFileWriter.save();
    QApplication::restoreOverrideCursor();
}

void DocumentWindow::exportGlbResult()
{
    QString filename = QFileDialog::getSaveFileName(this, QString(), QString(),
        tr("glTF Binary Format (*.glb)"));
    if (filename.isEmpty()) {
        return;
    }
    ensureFileExtension(&filename, ".glb");
    exportGlbToFilename(filename);
}

void DocumentWindow::exportGlbToFilename(const QString& filename)
{
    if (!m_document->isExportReady()) {
        qDebug() << "Export but document is not export ready";
        return;
    }
    QApplication::setOverrideCursor(Qt::WaitCursor);
    dust3d::Object skeletonResult = m_document->currentUvMappedObject();
    QImage* textureMetalnessRoughnessAmbientOcclusionImage = UvMapGenerator::combineMetalnessRoughnessAmbientOcclusionImages(m_document->textureMetalnessImage,
        m_document->textureRoughnessImage,
        m_document->textureAmbientOcclusionImage);
    GlbFileWriter glbFileWriter(skeletonResult, filename,
        m_document->textureImage, m_document->textureNormalImage, textureMetalnessRoughnessAmbientOcclusionImage);
    glbFileWriter.save();
    delete textureMetalnessRoughnessAmbientOcclusionImage;
    QApplication::restoreOverrideCursor();
}

void DocumentWindow::updateXlockButtonState()
{
    if (m_document->xlocked)
        m_xLockButton->setIcon(":/resources/toolbar_x_disabled.svg");
    else
        m_xLockButton->setIcon(":/resources/toolbar_x.svg");
}

void DocumentWindow::updateYlockButtonState()
{
    if (m_document->ylocked)
        m_yLockButton->setIcon(":/resources/toolbar_y_disabled.svg");
    else
        m_yLockButton->setIcon(":/resources/toolbar_y.svg");
}

void DocumentWindow::updateZlockButtonState()
{
    if (m_document->zlocked)
        m_zLockButton->setIcon(":/resources/toolbar_z_disabled.svg");
    else
        m_zLockButton->setIcon(":/resources/toolbar_z.svg");
}

void DocumentWindow::updateRadiusLockButtonState()
{
    if (m_document->radiusLocked)
        m_radiusLockButton->setIcon(":/resources/toolbar_radius_disabled.svg");
    else
        m_radiusLockButton->setIcon(":/resources/toolbar_radius.svg");
}

void DocumentWindow::registerDialog(QWidget* widget)
{
    m_dialogs.push_back(widget);
}

void DocumentWindow::unregisterDialog(QWidget* widget)
{
    m_dialogs.erase(std::remove(m_dialogs.begin(), m_dialogs.end(), widget), m_dialogs.end());
}

void DocumentWindow::setExportWaitingList(const QStringList& filenames)
{
    m_waitingForExportToFilenames = filenames;
}

void DocumentWindow::checkExportWaitingList()
{
    if (m_waitingForExportToFilenames.empty())
        return;

    auto list = m_waitingForExportToFilenames;
    m_waitingForExportToFilenames.clear();

    bool isSuccessful = m_document->isMeshGenerationSucceed();
    for (const auto& filename : list) {
        if (filename.endsWith(".obj")) {
            exportObjToFilename(filename);
            emit waitingExportFinished(filename, isSuccessful);
        } else if (filename.endsWith(".fbx")) {
            exportFbxToFilename(filename);
            emit waitingExportFinished(filename, isSuccessful);
        } else if (filename.endsWith(".glb")) {
            exportGlbToFilename(filename);
            emit waitingExportFinished(filename, isSuccessful);
        } else {
            emit waitingExportFinished(filename, false);
        }
    }
}

void DocumentWindow::generateComponentPreviewImages()
{
    if (nullptr != m_componentPreviewImagesGenerator) {
        m_isComponentPreviewImagesObsolete = true;
        return;
    }

    m_isComponentPreviewImagesObsolete = false;

    QThread* thread = new QThread;

    m_componentPreviewImagesGenerator = new MeshPreviewImagesGenerator(new ModelOffscreenRender(m_modelRenderWidget->format()));
    for (auto& component : m_document->componentMap) {
        if (!component.second.isPreviewMeshObsolete)
            continue;
        component.second.isPreviewMeshObsolete = false;
        auto previewMesh = std::unique_ptr<ModelMesh>(component.second.takePreviewMesh());
        if (nullptr == previewMesh)
            continue;
        bool useFrontView = false;
        if (!component.second.linkToPartId.isNull()) {
            const auto& part = m_document->findPart(component.second.linkToPartId);
            if (nullptr != part) {
                if (dust3d::PartTarget::CutFace == part->target)
                    useFrontView = true;
            }
        }
        if (!component.second.colorImageId.isNull()) {
            const auto& colorImage = ImageForever::get(component.second.colorImageId);
            if (nullptr != colorImage) {
                previewMesh->setTextureImage(new QImage(*colorImage));
            }
        }
        m_componentPreviewImagesGenerator->addInput(component.first, std::move(previewMesh), useFrontView);
    }
    m_componentPreviewImagesGenerator->moveToThread(thread);
    connect(thread, &QThread::started, m_componentPreviewImagesGenerator, &MeshPreviewImagesGenerator::process);
    connect(m_componentPreviewImagesGenerator, &MeshPreviewImagesGenerator::finished, this, &DocumentWindow::componentPreviewImagesReady);
    connect(m_componentPreviewImagesGenerator, &MeshPreviewImagesGenerator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();

    updateInprogressIndicator();
}

void DocumentWindow::componentPreviewImagesReady()
{
    std::unique_ptr<std::map<dust3d::Uuid, QImage>> componentImages;
    componentImages.reset(m_componentPreviewImagesGenerator->takeImages());
    if (nullptr != componentImages) {
        for (const auto& it : *componentImages) {
            m_document->setComponentPreviewImage(it.first, std::make_unique<QImage>(std::move(it.second)));
        }
    }

    decorateComponentPreviewImages();

    delete m_componentPreviewImagesGenerator;
    m_componentPreviewImagesGenerator = nullptr;

    if (m_isComponentPreviewImagesObsolete)
        generateComponentPreviewImages();
    else
        updateInprogressIndicator();
}

void DocumentWindow::decorateComponentPreviewImages()
{
    if (nullptr != m_componentPreviewImagesDecorator) {
        m_isComponentPreviewImageDecorationsObsolete = true;
        return;
    }

    m_isComponentPreviewImageDecorationsObsolete = false;

    QThread* thread = new QThread;

    auto previewInputs = std::make_unique<std::vector<ComponentPreviewImagesDecorator::PreviewInput>>();
    for (auto& component : m_document->componentMap) {
        if (!component.second.isPreviewImageDecorationObsolete)
            continue;
        component.second.isPreviewImageDecorationObsolete = false;
        if (nullptr == component.second.previewImage)
            continue;
        previewInputs->emplace_back(ComponentPreviewImagesDecorator::PreviewInput {
            component.first,
            std::make_unique<QImage>(*component.second.previewImage),
            !component.second.childrenIds.empty() });
    }
    m_componentPreviewImagesDecorator = std::make_unique<ComponentPreviewImagesDecorator>(std::move(previewInputs));
    m_componentPreviewImagesDecorator->moveToThread(thread);
    connect(thread, &QThread::started, m_componentPreviewImagesDecorator.get(), &ComponentPreviewImagesDecorator::process);
    connect(m_componentPreviewImagesDecorator.get(), &ComponentPreviewImagesDecorator::finished, this, &DocumentWindow::componentPreviewImageDecorationsReady);
    connect(m_componentPreviewImagesDecorator.get(), &ComponentPreviewImagesDecorator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();

    updateInprogressIndicator();
}

void DocumentWindow::componentPreviewImageDecorationsReady()
{
    auto resultImages = m_componentPreviewImagesDecorator->takeResultImages();
    if (nullptr != resultImages) {
        for (auto& it : *resultImages) {
            if (nullptr == it.second)
                continue;
            m_document->setComponentPreviewPixmap(it.first, QPixmap::fromImage(*it.second));
        }
    }

    m_componentPreviewImagesDecorator.reset();

    if (m_isComponentPreviewImageDecorationsObsolete)
        decorateComponentPreviewImages();
    else
        updateInprogressIndicator();
}

ModelWidget* DocumentWindow::modelWidget()
{
    return m_modelRenderWidget;
}

QShortcut* DocumentWindow::createShortcut(QKeySequence key)
{
    auto shortcutIt = m_shortcutMap.find(key);
    if (shortcutIt != m_shortcutMap.end())
        return shortcutIt->second;
    QShortcut* shortcut = new QShortcut(this);
    shortcut->setKey(key);
    m_shortcutMap.insert({ key, shortcut });
    return shortcut;
}

#define defineShortcut(keyVal, widget, funcName) \
    QObject::connect(createShortcut(keyVal), &QShortcut::activated, widget, funcName)

void DocumentWindow::initializeToolShortcuts(SkeletonGraphicsWidget* graphicsWidget)
{
    defineShortcut(Qt::Key_A, graphicsWidget, &SkeletonGraphicsWidget::shortcutAddMode);
    defineShortcut(Qt::CTRL + Qt::Key_A, graphicsWidget, &SkeletonGraphicsWidget::shortcutSelectAll);
    defineShortcut(Qt::CTRL + Qt::Key_Z, graphicsWidget, &SkeletonGraphicsWidget::shortcutUndo);
    defineShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_Z, graphicsWidget, &SkeletonGraphicsWidget::shortcutRedo);
    defineShortcut(Qt::CTRL + Qt::Key_Y, graphicsWidget, &SkeletonGraphicsWidget::shortcutRedo);
    defineShortcut(Qt::Key_Z, graphicsWidget, &SkeletonGraphicsWidget::shortcutZlock);
    defineShortcut(Qt::Key_Y, graphicsWidget, &SkeletonGraphicsWidget::shortcutYlock);
    defineShortcut(Qt::Key_X, graphicsWidget, &SkeletonGraphicsWidget::shortcutXlock);
    defineShortcut(Qt::Key_S, graphicsWidget, &SkeletonGraphicsWidget::shortcutSelectMode);
    defineShortcut(Qt::Key_R, graphicsWidget, &SkeletonGraphicsWidget::shortcutToggleRotation);
    defineShortcut(Qt::Key_O, graphicsWidget, &SkeletonGraphicsWidget::shortcutToggleFlatShading);
    defineShortcut(Qt::Key_W, graphicsWidget, &SkeletonGraphicsWidget::shortcutToggleWireframe);
    defineShortcut(Qt::Key_Escape, graphicsWidget, &SkeletonGraphicsWidget::shortcutEscape);
}

void DocumentWindow::initializeCanvasShortcuts(SkeletonGraphicsWidget* graphicsWidget)
{
    defineShortcut(Qt::Key_Delete, graphicsWidget, &SkeletonGraphicsWidget::shortcutDelete);
    defineShortcut(Qt::Key_Backspace, graphicsWidget, &SkeletonGraphicsWidget::shortcutDelete);
    defineShortcut(Qt::CTRL + Qt::Key_X, graphicsWidget, &SkeletonGraphicsWidget::shortcutCut);
    defineShortcut(Qt::CTRL + Qt::Key_C, graphicsWidget, &SkeletonGraphicsWidget::shortcutCopy);
    defineShortcut(Qt::CTRL + Qt::Key_V, graphicsWidget, &SkeletonGraphicsWidget::shortcutPaste);
    defineShortcut(Qt::ALT + Qt::Key_Minus, graphicsWidget, &SkeletonGraphicsWidget::shortcutZoomRenderedModelByMinus10);
    defineShortcut(Qt::Key_Minus, graphicsWidget, &SkeletonGraphicsWidget::shortcutZoomSelectedByMinus1);
    defineShortcut(Qt::ALT + Qt::Key_Equal, graphicsWidget, &SkeletonGraphicsWidget::shortcutZoomRenderedModelBy10);
    defineShortcut(Qt::Key_Equal, graphicsWidget, &SkeletonGraphicsWidget::shortcutZoomSelectedBy1);
    defineShortcut(Qt::Key_Comma, graphicsWidget, &SkeletonGraphicsWidget::shortcutRotateSelectedByMinus1);
    defineShortcut(Qt::Key_Period, graphicsWidget, &SkeletonGraphicsWidget::shortcutRotateSelectedBy1);
    defineShortcut(Qt::Key_Left, graphicsWidget, &SkeletonGraphicsWidget::shortcutMoveSelectedToLeft);
    defineShortcut(Qt::Key_Right, graphicsWidget, &SkeletonGraphicsWidget::shortcutMoveSelectedToRight);
    defineShortcut(Qt::Key_Up, graphicsWidget, &SkeletonGraphicsWidget::shortcutMoveSelectedToUp);
    defineShortcut(Qt::Key_Down, graphicsWidget, &SkeletonGraphicsWidget::shortcutMoveSelectedToDown);
    defineShortcut(Qt::Key_BracketLeft, graphicsWidget, &SkeletonGraphicsWidget::shortcutScaleSelectedByMinus1);
    defineShortcut(Qt::Key_BracketRight, graphicsWidget, &SkeletonGraphicsWidget::shortcutScaleSelectedBy1);
    defineShortcut(Qt::Key_E, graphicsWidget, &SkeletonGraphicsWidget::shortcutSwitchProfileOnSelected);
    defineShortcut(Qt::Key_H, graphicsWidget, &SkeletonGraphicsWidget::shortcutShowOrHideSelectedPart);
    defineShortcut(Qt::Key_J, graphicsWidget, &SkeletonGraphicsWidget::shortcutEnableOrDisableSelectedPart);
    defineShortcut(Qt::Key_L, graphicsWidget, &SkeletonGraphicsWidget::shortcutLockOrUnlockSelectedPart);
    defineShortcut(Qt::Key_F, graphicsWidget, &SkeletonGraphicsWidget::shortcutCheckPartComponent);
}

void DocumentWindow::initializeShortcuts()
{
    initializeToolShortcuts(m_canvasGraphicsWidget);
    initializeCanvasShortcuts(m_canvasGraphicsWidget);

    defineShortcut(Qt::Key_M, m_canvasGraphicsWidget, &SkeletonGraphicsWidget::shortcutXmirrorOnOrOffSelectedPart);
    defineShortcut(Qt::Key_B, m_canvasGraphicsWidget, &SkeletonGraphicsWidget::shortcutSubdivedOrNotSelectedPart);
    defineShortcut(Qt::Key_U, m_canvasGraphicsWidget, &SkeletonGraphicsWidget::shortcutRoundEndOrNotSelectedPart);
    defineShortcut(Qt::Key_C, m_canvasGraphicsWidget, &SkeletonGraphicsWidget::shortcutChamferedOrNotSelectedPart);
}

void DocumentWindow::openRecentFile()
{
    if (!m_documentSaved) {
        QMessageBox::StandardButton answer = QMessageBox::question(this,
            APP_NAME,
            tr("Do you really want to open another file and lose the unsaved changes?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (answer != QMessageBox::Yes)
            return;
    }

    QAction* action = qobject_cast<QAction*>(sender());
    if (action) {
        QString fileName = action->data().toString();
        openPathAs(fileName, fileName);
    }
}

void DocumentWindow::updateRecentFileActions()
{
    QStringList files = Preferences::instance().recentFileList();

    for (int i = 0; i < (int)files.size() && i < (int)m_recentFileActions.size(); ++i) {
        QString text = tr("&%1 %2").arg(i + 1).arg(strippedName(files[i]));
        m_recentFileActions[i]->setText(text);
        m_recentFileActions[i]->setData(files[i]);
        m_recentFileActions[i]->setVisible(true);
    }
    for (int j = files.size(); j < (int)m_recentFileActions.size(); ++j)
        m_recentFileActions[j]->setVisible(false);

    m_recentFileSeparatorAction->setVisible(files.size() > 0);
}

QString DocumentWindow::strippedName(const QString& fullFileName)
{
    return QFileInfo(fullFileName).fileName();
}

bool DocumentWindow::isWorking()
{
    return nullptr == m_inprogressIndicator || m_inprogressIndicator->isSpinning();
}

void DocumentWindow::toggleRenderColor()
{
    m_modelRemoveColor = !m_modelRemoveColor;
    forceUpdateRenderModel();
}

void DocumentWindow::generateBonePreviewImages()
{
    if (nullptr != m_bonePreviewImagesGenerator) {
        m_isBonePreviewImagesObsolete = true;
        return;
    }

    m_isBonePreviewImagesObsolete = false;

    QThread* thread = new QThread;

    m_bonePreviewImagesGenerator = new MeshPreviewImagesGenerator(new ModelOffscreenRender(m_modelRenderWidget->format()));
    for (auto& bone : m_document->boneMap) {
        if (!bone.second.isPreviewMeshObsolete)
            continue;
        bone.second.isPreviewMeshObsolete = false;
        auto previewMesh = std::unique_ptr<ModelMesh>(bone.second.takePreviewMesh());
        const bool useFrontView = false;
        m_bonePreviewImagesGenerator->addInput(bone.first, std::move(previewMesh), useFrontView);
    }
    m_bonePreviewImagesGenerator->moveToThread(thread);
    connect(thread, &QThread::started, m_bonePreviewImagesGenerator, &MeshPreviewImagesGenerator::process);
    connect(m_bonePreviewImagesGenerator, &MeshPreviewImagesGenerator::finished, this, &DocumentWindow::bonePreviewImagesReady);
    connect(m_bonePreviewImagesGenerator, &MeshPreviewImagesGenerator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();

    updateInprogressIndicator();
}

void DocumentWindow::bonePreviewImagesReady()
{
    std::unique_ptr<std::map<dust3d::Uuid, QImage>> boneImages;
    boneImages.reset(m_bonePreviewImagesGenerator->takeImages());
    if (nullptr != boneImages) {
        for (const auto& it : *boneImages) {
            m_document->setBonePreviewPixmap(it.first, QPixmap::fromImage(it.second));
        }
    }

    delete m_bonePreviewImagesGenerator;
    m_bonePreviewImagesGenerator = nullptr;

    if (m_isBonePreviewImagesObsolete)
        generateBonePreviewImages();
    else
        updateInprogressIndicator();
}

void DocumentWindow::forceUpdateRenderModel()
{
    ModelMesh* mesh = nullptr;
    if (m_showBodyBonePreview) {
        mesh = m_document->takeResultBodyBonePreviewMesh();
        m_currentUpdatedMeshId = m_document->resultBodyBonePreviewMeshId();
    } else {
        if (m_document->isMeshGenerating() || m_document->isTextureGenerating()) {
            mesh = m_document->takeResultMesh();
            m_currentUpdatedMeshId = m_document->resultMeshId();
        } else {
            mesh = m_document->takeResultTextureMesh();
            m_currentUpdatedMeshId = m_document->resultTextureMeshId();
            m_currentTextureImageUpdateVersion = m_document->resultTextureImageUpdateVersion();
        }
        if (m_modelRemoveColor && mesh)
            mesh->removeColor();
    }
    m_modelRenderWidget->updateMesh(mesh);
}

void DocumentWindow::updateRenderModel()
{
    qint64 shouldShowId = 0;
    quint64 shouldShowTextureVersion = m_currentTextureImageUpdateVersion;
    if (m_document->isMeshGenerating() || m_document->isTextureGenerating()) {
        shouldShowId = m_document->resultMeshId();
    } else {
        shouldShowId = -(qint64)m_document->resultTextureMeshId();
        shouldShowTextureVersion = m_document->resultTextureImageUpdateVersion();
    }
    if (shouldShowId == m_currentUpdatedMeshId && shouldShowTextureVersion == m_currentTextureImageUpdateVersion) {
        return;
    }
    if (std::abs(shouldShowId) < std::abs(m_currentUpdatedMeshId)) {
        return;
    }
    forceUpdateRenderModel();
}

void DocumentWindow::forceUpdateRenderWireframe()
{
    m_modelRenderWidget->updateWireframeMesh(m_document->takeWireframeMesh());
    m_currentUpdatedWireframeId = m_document->resultMeshId();
}

void DocumentWindow::updateRenderWireframe()
{
    if (m_document->resultMeshId() == m_currentUpdatedWireframeId)
        return;
    forceUpdateRenderWireframe();
}
