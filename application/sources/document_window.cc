#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGridLayout>
#include <QToolBar>
#include <QPushButton>
#include <QFileDialog>
#include <QTabWidget>
#include <QtCore/qbuffer.h>
#include <QMessageBox>
#include <QTimer>
#include <QMenuBar>
#include <QPointer>
#include <QApplication>
#include <map>
#include <QDesktopServices>
#include <QDockWidget>
#include <QWidgetAction>
#include <QGraphicsOpacityEffect>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QTextBrowser>
#include <QMimeData>
#include <dust3d/base/ds3_file.h>
#include <dust3d/base/snapshot.h>
#include <dust3d/base/snapshot_xml.h>
#include <dust3d/base/debug.h>
#include "document_window.h"
#include "skeleton_graphics_widget.h"
#include "theme.h"
#include "log_browser.h"
#include "about_widget.h"
#include "version.h"
#include "glb_file.h"
#include "part_tree_widget.h"
#include "material_manage_widget.h"
#include "image_forever.h"
#include "spinnable_toolbar_icon.h"
#include "fbx_file.h"
#include "float_number_widget.h"
#include "updates_check_widget.h"
#include "document_saver.h"
#include "document.h"
#include "preferences.h"
#include "flow_layout.h"
#include "cut_face_preview.h"
#include "horizontal_line_widget.h"
#include "texture_generator.h"

LogBrowser *g_logBrowser = nullptr;
std::map<DocumentWindow *, dust3d::Uuid> g_documentWindows;
QTextBrowser *g_acknowlegementsWidget = nullptr;
AboutWidget *g_aboutWidget = nullptr;
QTextBrowser *g_contributorsWidget = nullptr;
QTextBrowser *g_supportersWidget = nullptr;
UpdatesCheckWidget *g_updatesCheckWidget = nullptr;

void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (g_logBrowser)
        g_logBrowser->outputMessage(type, msg, context.file, context.line);
}

void ensureFileExtension(QString* filename, const QString extension) {
    if (!filename->endsWith(extension)) {
        filename->append(extension);
    }
}

const std::map<DocumentWindow *, dust3d::Uuid> &DocumentWindow::documentWindows()
{
    return g_documentWindows;
}

Document *DocumentWindow::document()
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

void DocumentWindow::checkForUpdates()
{
    if (!g_updatesCheckWidget) {
        g_updatesCheckWidget = new UpdatesCheckWidget;
    }
    g_updatesCheckWidget->check();
    g_updatesCheckWidget->show();
    g_updatesCheckWidget->activateWindow();
    g_updatesCheckWidget->raise();
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

    g_documentWindows.insert({this, dust3d::Uuid::createUuid()});

    m_document = new Document;
    
    SkeletonGraphicsWidget *canvasGraphicsWidget = new SkeletonGraphicsWidget(m_document);
    m_canvasGraphicsWidget = canvasGraphicsWidget;

    QVBoxLayout *toolButtonLayout = new QVBoxLayout;
    toolButtonLayout->setSpacing(0);
    toolButtonLayout->setContentsMargins(5, 10, 4, 0);

    ToolbarButton *addButton = new ToolbarButton(":/resources/toolbar_add.svg");
    addButton->setToolTip(tr("Add node to canvas"));

    ToolbarButton *selectButton = new ToolbarButton(":/resources/toolbar_pointer.svg");
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
    connect(m_document, &Document::resultPartPreviewsChanged, this, [=]() {
        generatePartPreviewImages();
    });
    connect(m_document, &Document::postProcessing, this, &DocumentWindow::updateInprogressIndicator);
    connect(m_document, &Document::textureGenerating, this, &DocumentWindow::updateInprogressIndicator);
    connect(m_document, &Document::resultTextureChanged, this, &DocumentWindow::updateInprogressIndicator);
    connect(m_document, &Document::postProcessedResultChanged, this, &DocumentWindow::updateInprogressIndicator);

    toolButtonLayout->addWidget(addButton);
    toolButtonLayout->addWidget(selectButton);
    toolButtonLayout->addSpacing(20);
    toolButtonLayout->addWidget(m_xLockButton);
    toolButtonLayout->addWidget(m_yLockButton);
    toolButtonLayout->addWidget(m_zLockButton);
    toolButtonLayout->addWidget(m_radiusLockButton);
    toolButtonLayout->addSpacing(20);
    toolButtonLayout->addWidget(m_inprogressIndicator);
    

    QLabel *verticalLogoLabel = new QLabel;
    QImage verticalLogoImage;
    verticalLogoImage.load(":/resources/dust3d-vertical.png");
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

    GraphicsContainerWidget *containerWidget = new GraphicsContainerWidget;
    containerWidget->setGraphicsWidget(canvasGraphicsWidget);
    QGridLayout *containerLayout = new QGridLayout;
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
    m_modelRenderWidget->enableEnvironmentLight();
    m_modelRenderWidget->toggleWireframe();
    m_modelRenderWidget->disableCullFace();
    m_modelRenderWidget->setEyePosition(QVector3D(0.0, 0.0, -4.0));
    m_modelRenderWidget->setMoveToPosition(QVector3D(-0.5, -0.5, 0.0));
    
    connect(containerWidget, &GraphicsContainerWidget::containerSizeChanged,
        m_modelRenderWidget, &ModelWidget::canvasResized);

    m_canvasGraphicsWidget->setModelWidget(m_modelRenderWidget);
    containerWidget->setModelWidget(m_modelRenderWidget);
    
    setTabPosition(Qt::RightDockWidgetArea, QTabWidget::East);

    QDockWidget *partsDocker = new QDockWidget(tr("Parts"), this);
    partsDocker->setAllowedAreas(Qt::RightDockWidgetArea);
    m_partTreeWidget = new PartTreeWidget(m_document, nullptr);
    partsDocker->setWidget(m_partTreeWidget);
    addDockWidget(Qt::RightDockWidgetArea, partsDocker);

    QDockWidget *materialDocker = new QDockWidget(tr("Materials"), this);
    materialDocker->setAllowedAreas(Qt::RightDockWidgetArea);
    MaterialManageWidget *materialManageWidget = new MaterialManageWidget(m_document, materialDocker);
    materialDocker->setWidget(materialManageWidget);
    connect(materialManageWidget, &MaterialManageWidget::registerDialog, this, &DocumentWindow::registerDialog);
    connect(materialManageWidget, &MaterialManageWidget::unregisterDialog, this, &DocumentWindow::unregisterDialog);
    addDockWidget(Qt::RightDockWidgetArea, materialDocker);
    connect(materialDocker, &QDockWidget::topLevelChanged, [=](bool topLevel) {
        Q_UNUSED(topLevel);
        for (const auto &material: m_document->materialMap)
            emit m_document->materialPreviewChanged(material.first);
    });
    
    tabifyDockWidget(partsDocker, materialDocker);
    
    partsDocker->raise();
    
    QWidget *titleBarWidget = new QWidget;
    titleBarWidget->setFixedHeight(1);
    
    QHBoxLayout *titleBarLayout = new QHBoxLayout;
    titleBarLayout->addStretch();
    titleBarWidget->setLayout(titleBarLayout);

    QVBoxLayout *canvasLayout = new QVBoxLayout;
    canvasLayout->setSpacing(0);
    canvasLayout->setContentsMargins(0, 0, 0, 0);
    canvasLayout->addWidget(titleBarWidget);
    canvasLayout->addWidget(containerWidget);
    canvasLayout->setStretch(1, 1);
    
    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addLayout(mainLeftLayout);
    mainLayout->addLayout(canvasLayout);
    mainLayout->addSpacing(3);

    QWidget *centralWidget = new QWidget;
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
        "Bicycle",
        "Cat",
        "Dog",
        "Giraffe",
        "Meerkat",
        "Mosquito",
        "Screwdriver",
        "Seagull"
    };
    for (const auto &model: exampleModels) {
        QAction *openModelAction = new QAction(model, this);
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

    m_changeTurnaroundAction = new QAction(tr("Change Reference Sheet..."), this);
    connect(m_changeTurnaroundAction, &QAction::triggered, this, &DocumentWindow::changeTurnaround, Qt::QueuedConnection);
    m_fileMenu->addAction(m_changeTurnaroundAction);
    
    m_fileMenu->addSeparator();
    
    for (int i = 0; i < Preferences::instance().maxRecentFiles(); ++i) {
        QAction *action = new QAction(this);
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
    connect(m_toggleColorAction, &QAction::triggered, [&]() {
        m_modelRemoveColor = !m_modelRemoveColor;
        Model *mesh = nullptr;
        if (m_document->isMeshGenerating() ||
                m_document->isPostProcessing() ||
                m_document->isTextureGenerating()) {
            mesh = m_document->takeResultMesh();
        } else {
            mesh = m_document->takeResultTextureMesh();
        }
        if (m_modelRemoveColor && mesh)
            mesh->removeColor();
        m_modelRenderWidget->updateMesh(mesh);
    });
    m_viewMenu->addAction(m_toggleColorAction);
    
    m_windowMenu = menuBar()->addMenu(tr("&Window"));
    
    m_showPartsListAction = new QAction(tr("Parts"), this);
    connect(m_showPartsListAction, &QAction::triggered, [=]() {
        partsDocker->show();
        partsDocker->raise();
    });
    m_windowMenu->addAction(m_showPartsListAction);
    
    m_showMaterialsAction = new QAction(tr("Materials"), this);
    connect(m_showMaterialsAction, &QAction::triggered, [=]() {
        materialDocker->show();
        materialDocker->raise();
    });
    m_windowMenu->addAction(m_showMaterialsAction);
    
    QMenu *dialogsMenu = m_windowMenu->addMenu(tr("Dialogs"));
    connect(dialogsMenu, &QMenu::aboutToShow, [=]() {
        dialogsMenu->clear();
        if (this->m_dialogs.empty()) {
            QAction *action = dialogsMenu->addAction(tr("None"));
            action->setEnabled(false);
            return;
        }
        for (const auto &dialog: this->m_dialogs) {
            QAction *action = dialogsMenu->addAction(dialog->windowTitle());
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
    
    m_checkForUpdatesAction = new QAction(tr("Check for Updates..."), this);
    connect(m_checkForUpdatesAction, &QAction::triggered, this, &DocumentWindow::checkForUpdates);
    m_helpMenu->addAction(m_checkForUpdatesAction);

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
        m_document->setEditMode(SkeletonDocumentEditMode::Add);
    });

    connect(selectButton, &QPushButton::clicked, [=]() {
        m_document->setEditMode(SkeletonDocumentEditMode::Select);
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
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::setNodeOrigin, m_document, &Document::setNodeOrigin);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::clearNodeCutFaceSettings, m_document, &Document::clearNodeCutFaceSettings);
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
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::setPartColorState, m_document, &Document::setPartColorState);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::setPartDisableState, m_document, &Document::setPartDisableState);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::setPartXmirrorState, m_document, &Document::setPartXmirrorState);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::setPartRoundState, m_document, &Document::setPartRoundState);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::setPartWrapState, m_document, &Document::setPartCutRotation);

    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::setXlockState, m_document, &Document::setXlockState);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::setYlockState, m_document, &Document::setYlockState);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::setZlockState, m_document, &Document::setZlockState);
    
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::enableAllPositionRelatedLocks, m_document, &Document::enableAllPositionRelatedLocks);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::disableAllPositionRelatedLocks, m_document, &Document::disableAllPositionRelatedLocks);

    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::changeTurnaround, this, &DocumentWindow::changeTurnaround);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::open, this, &DocumentWindow::open);
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::showCutFaceSettingPopup, this, &DocumentWindow::showCutFaceSettingPopup);
    
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::showOrHideAllComponents, m_document, &Document::showOrHideAllComponents);
    
    connect(m_document, &Document::nodeAdded, canvasGraphicsWidget, &SkeletonGraphicsWidget::nodeAdded);
    connect(m_document, &Document::nodeRemoved, canvasGraphicsWidget, &SkeletonGraphicsWidget::nodeRemoved);
    connect(m_document, &Document::edgeAdded, canvasGraphicsWidget, &SkeletonGraphicsWidget::edgeAdded);
    connect(m_document, &Document::edgeRemoved, canvasGraphicsWidget, &SkeletonGraphicsWidget::edgeRemoved);
    connect(m_document, &Document::nodeRadiusChanged, canvasGraphicsWidget, &SkeletonGraphicsWidget::nodeRadiusChanged);
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
    
    connect(m_partTreeWidget, &PartTreeWidget::currentComponentChanged, m_document, &Document::setCurrentCanvasComponentId);
    connect(m_partTreeWidget, &PartTreeWidget::moveComponentUp, m_document, &Document::moveComponentUp);
    connect(m_partTreeWidget, &PartTreeWidget::moveComponentDown, m_document, &Document::moveComponentDown);
    connect(m_partTreeWidget, &PartTreeWidget::moveComponentToTop, m_document, &Document::moveComponentToTop);
    connect(m_partTreeWidget, &PartTreeWidget::moveComponentToBottom, m_document, &Document::moveComponentToBottom);
    connect(m_partTreeWidget, &PartTreeWidget::checkPart, m_document, &Document::checkPart);
    connect(m_partTreeWidget, &PartTreeWidget::createNewComponentAndMoveThisIn, m_document, &Document::createNewComponentAndMoveThisIn);
    connect(m_partTreeWidget, &PartTreeWidget::createNewChildComponent, m_document, &Document::createNewChildComponent);
    connect(m_partTreeWidget, &PartTreeWidget::renameComponent, m_document, &Document::renameComponent);
    connect(m_partTreeWidget, &PartTreeWidget::setComponentExpandState, m_document, &Document::setComponentExpandState);
    connect(m_partTreeWidget, &PartTreeWidget::moveComponent, m_document, &Document::moveComponent);
    connect(m_partTreeWidget, &PartTreeWidget::removeComponent, m_document, &Document::removeComponent);
    connect(m_partTreeWidget, &PartTreeWidget::hideOtherComponents, m_document, &Document::hideOtherComponents);
    connect(m_partTreeWidget, &PartTreeWidget::lockOtherComponents, m_document, &Document::lockOtherComponents);
    connect(m_partTreeWidget, &PartTreeWidget::hideAllComponents, m_document, &Document::hideAllComponents);
    connect(m_partTreeWidget, &PartTreeWidget::showAllComponents, m_document, &Document::showAllComponents);
    connect(m_partTreeWidget, &PartTreeWidget::collapseAllComponents, m_document, &Document::collapseAllComponents);
    connect(m_partTreeWidget, &PartTreeWidget::expandAllComponents, m_document, &Document::expandAllComponents);
    connect(m_partTreeWidget, &PartTreeWidget::lockAllComponents, m_document, &Document::lockAllComponents);
    connect(m_partTreeWidget, &PartTreeWidget::unlockAllComponents, m_document, &Document::unlockAllComponents);
    connect(m_partTreeWidget, &PartTreeWidget::setPartLockState, m_document, &Document::setPartLockState);
    connect(m_partTreeWidget, &PartTreeWidget::setPartVisibleState, m_document, &Document::setPartVisibleState);
    connect(m_partTreeWidget, &PartTreeWidget::setPartColorState, m_document, &Document::setPartColorState);
    connect(m_partTreeWidget, &PartTreeWidget::setComponentCombineMode, m_document, &Document::setComponentCombineMode);
    connect(m_partTreeWidget, &PartTreeWidget::setPartTarget, m_document, &Document::setPartTarget);
    connect(m_partTreeWidget, &PartTreeWidget::setPartBase, m_document, &Document::setPartBase);
    connect(m_partTreeWidget, &PartTreeWidget::hideDescendantComponents, m_document, &Document::hideDescendantComponents);
    connect(m_partTreeWidget, &PartTreeWidget::showDescendantComponents, m_document, &Document::showDescendantComponents);
    connect(m_partTreeWidget, &PartTreeWidget::lockDescendantComponents, m_document, &Document::lockDescendantComponents);
    connect(m_partTreeWidget, &PartTreeWidget::unlockDescendantComponents, m_document, &Document::unlockDescendantComponents);
    connect(m_partTreeWidget, &PartTreeWidget::groupOperationAdded, m_document, &Document::saveSnapshot);
    
    connect(m_partTreeWidget, &PartTreeWidget::addPartToSelection, canvasGraphicsWidget, &SkeletonGraphicsWidget::addPartToSelection);
    
    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::partComponentChecked, m_partTreeWidget, &PartTreeWidget::partComponentChecked);
    
    connect(m_document, &Document::componentNameChanged, m_partTreeWidget, &PartTreeWidget::componentNameChanged);
    connect(m_document, &Document::componentChildrenChanged, m_partTreeWidget, &PartTreeWidget::componentChildrenChanged);
    connect(m_document, &Document::componentRemoved, m_partTreeWidget, &PartTreeWidget::componentRemoved);
    connect(m_document, &Document::componentAdded, m_partTreeWidget, &PartTreeWidget::componentAdded);
    connect(m_document, &Document::componentExpandStateChanged, m_partTreeWidget, &PartTreeWidget::componentExpandStateChanged);
    connect(m_document, &Document::componentCombineModeChanged, m_partTreeWidget, &PartTreeWidget::componentCombineModeChanged);
    connect(m_document, &Document::partPreviewChanged, m_partTreeWidget, &PartTreeWidget::partPreviewChanged);
    connect(m_document, &Document::partLockStateChanged, m_partTreeWidget, &PartTreeWidget::partLockStateChanged);
    connect(m_document, &Document::partVisibleStateChanged, m_partTreeWidget, &PartTreeWidget::partVisibleStateChanged);
    connect(m_document, &Document::partSubdivStateChanged, m_partTreeWidget, &PartTreeWidget::partSubdivStateChanged);
    connect(m_document, &Document::partDisableStateChanged, m_partTreeWidget, &PartTreeWidget::partDisableStateChanged);
    connect(m_document, &Document::partXmirrorStateChanged, m_partTreeWidget, &PartTreeWidget::partXmirrorStateChanged);
    connect(m_document, &Document::partDeformThicknessChanged, m_partTreeWidget, &PartTreeWidget::partDeformChanged);
    connect(m_document, &Document::partDeformWidthChanged, m_partTreeWidget, &PartTreeWidget::partDeformChanged);
    connect(m_document, &Document::partRoundStateChanged, m_partTreeWidget, &PartTreeWidget::partRoundStateChanged);
    connect(m_document, &Document::partChamferStateChanged, m_partTreeWidget, &PartTreeWidget::partChamferStateChanged);
    connect(m_document, &Document::partColorStateChanged, m_partTreeWidget, &PartTreeWidget::partColorStateChanged);
    connect(m_document, &Document::partCutRotationChanged, m_partTreeWidget, &PartTreeWidget::partCutRotationChanged);
    connect(m_document, &Document::partCutFaceChanged, m_partTreeWidget, &PartTreeWidget::partCutFaceChanged);
    connect(m_document, &Document::partHollowThicknessChanged, m_partTreeWidget, &PartTreeWidget::partHollowThicknessChanged);
    connect(m_document, &Document::partMaterialIdChanged, m_partTreeWidget, &PartTreeWidget::partMaterialIdChanged);
    connect(m_document, &Document::partColorSolubilityChanged, m_partTreeWidget, &PartTreeWidget::partColorSolubilityChanged);
    connect(m_document, &Document::partMetalnessChanged, m_partTreeWidget, &PartTreeWidget::partMetalnessChanged);
    connect(m_document, &Document::partRoughnessChanged, m_partTreeWidget, &PartTreeWidget::partRoughnessChanged);
    connect(m_document, &Document::partCountershadeStateChanged, m_partTreeWidget, &PartTreeWidget::partCountershadeStateChanged);
    connect(m_document, &Document::partSmoothStateChanged, m_partTreeWidget, &PartTreeWidget::partSmoothStateChanged);
    
    connect(m_document, &Document::partTargetChanged, m_partTreeWidget, &PartTreeWidget::partXmirrorStateChanged);
    connect(m_document, &Document::partTargetChanged, m_partTreeWidget, &PartTreeWidget::partColorStateChanged);
    connect(m_document, &Document::partTargetChanged, m_partTreeWidget, &PartTreeWidget::partSubdivStateChanged);
    connect(m_document, &Document::partTargetChanged, m_partTreeWidget, &PartTreeWidget::partRoundStateChanged);
    connect(m_document, &Document::partTargetChanged, m_partTreeWidget, &PartTreeWidget::partChamferStateChanged);
    connect(m_document, &Document::partTargetChanged, m_partTreeWidget, &PartTreeWidget::partCutRotationChanged);
    connect(m_document, &Document::partTargetChanged, m_partTreeWidget, &PartTreeWidget::partDeformChanged);
    
    connect(m_document, &Document::partRemoved, m_partTreeWidget, &PartTreeWidget::partRemoved);
    connect(m_document, &Document::cleanup, m_partTreeWidget, &PartTreeWidget::removeAllContent);
    connect(m_document, &Document::partChecked, m_partTreeWidget, &PartTreeWidget::partChecked);
    connect(m_document, &Document::partUnchecked, m_partTreeWidget, &PartTreeWidget::partUnchecked);

    connect(m_document, &Document::skeletonChanged, m_document, &Document::generateMesh);
    connect(m_document, &Document::textureChanged, m_document, &Document::generateTexture);
    connect(m_document, &Document::resultMeshChanged, m_document, &Document::postProcess);
    connect(m_document, &Document::postProcessedResultChanged, m_document, &Document::generateTexture);
    connect(m_document, &Document::resultTextureChanged, [=]() {
        if (m_document->isMeshGenerating())
            return;
        auto resultTextureMesh = m_document->takeResultTextureMesh();
        if (nullptr != resultTextureMesh) {
            if (resultTextureMesh->meshId() < m_currentUpdatedMeshId) {
                delete resultTextureMesh;
                return;
            }
        }
        if (m_modelRemoveColor && resultTextureMesh)
            resultTextureMesh->removeColor();
        m_modelRenderWidget->updateMesh(resultTextureMesh);
    });
    connect(m_document, &Document::resultColorTextureChanged, [=]() {
        if (nullptr != m_document->textureImage)
            m_modelRenderWidget->updateColorTexture(new QImage(*m_document->textureImage));
    });
    
    connect(m_document, &Document::resultMeshChanged, [=]() {
        auto resultMesh = m_document->takeResultMesh();
        if (nullptr != resultMesh)
            m_currentUpdatedMeshId = resultMesh->meshId();
        if (m_modelRemoveColor && resultMesh)
            resultMesh->removeColor();
        m_modelRenderWidget->updateMesh(resultMesh);
    });

    connect(canvasGraphicsWidget, &SkeletonGraphicsWidget::cursorChanged, [=]() {
        m_modelRenderWidget->setCursor(canvasGraphicsWidget->cursor());
        containerWidget->setCursor(canvasGraphicsWidget->cursor());
    });

    connect(m_document, &Document::skeletonChanged, this, &DocumentWindow::documentChanged);
    connect(m_document, &Document::turnaroundChanged, this, &DocumentWindow::documentChanged);
    connect(m_document, &Document::optionsChanged, this, &DocumentWindow::documentChanged);

    connect(m_modelRenderWidget, &ModelWidget::customContextMenuRequested, [=](const QPoint &pos) {
        canvasGraphicsWidget->showContextMenu(canvasGraphicsWidget->mapFromGlobal(m_modelRenderWidget->mapToGlobal(pos)));
    });

    connect(m_document, &Document::xlockStateChanged, this, &DocumentWindow::updateXlockButtonState);
    connect(m_document, &Document::ylockStateChanged, this, &DocumentWindow::updateYlockButtonState);
    connect(m_document, &Document::zlockStateChanged, this, &DocumentWindow::updateZlockButtonState);
    connect(m_document, &Document::radiusLockStateChanged, this, &DocumentWindow::updateRadiusLockButtonState);
    
    connect(m_document, &Document::materialAdded, this, [=](dust3d::Uuid materialId) {
        Q_UNUSED(materialId);
        m_document->generateMaterialPreviews();
    });
    connect(m_document, &Document::materialLayersChanged, this, [=](dust3d::Uuid materialId) {
        Q_UNUSED(materialId);
        m_document->generateMaterialPreviews();
    });

    initializeShortcuts();

    connect(this, &DocumentWindow::initialized, m_document, &Document::uiReady);
    
    QTimer *timer = new QTimer(this);
    timer->setInterval(250);
    connect(timer, &QTimer::timeout, [=] {
        QWidget *focusedWidget = QApplication::focusWidget();
        if (nullptr == focusedWidget && isActiveWindow())
            canvasGraphicsWidget->setFocus();
    });
    timer->start();
}

void DocumentWindow::updateInprogressIndicator()
{
    if (m_document->isMeshGenerating() ||
            m_document->isPostProcessing() ||
            m_document->isTextureGenerating()) {
        m_inprogressIndicator->showSpinner(true);
    } else {
        m_inprogressIndicator->showSpinner(false);
    }
}

void DocumentWindow::toggleRotation()
{
    if (nullptr == m_canvasGraphicsWidget)
        return;
    m_canvasGraphicsWidget->setRotated(!m_canvasGraphicsWidget->rotated());
}

DocumentWindow *DocumentWindow::createDocumentWindow()
{
    DocumentWindow *documentWindow = new DocumentWindow();
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

void DocumentWindow::closeEvent(QCloseEvent *event)
{
    if (! m_documentSaved) {
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

void DocumentWindow::setCurrentFilename(const QString &filename)
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
    m_document->clearHistories();
    m_document->reset();
    m_document->clearTurnaround();
    m_document->saveSnapshot();
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
    for (auto &it: g_documentWindows) {
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

void DocumentWindow::showEvent(QShowEvent *event)
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

void DocumentWindow::mousePressEvent(QMouseEvent *event)
{
    QMainWindow::mousePressEvent(event);
}

void DocumentWindow::changeTurnaround()
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

void DocumentWindow::save()
{
    saveTo(m_currentFilename);
}

void DocumentWindow::saveTo(const QString &saveAsFilename)
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
            (!m_document->turnaround.isNull() && m_document->turnaroundPngByteArray.size() > 0) ? 
                &m_document->turnaroundPngByteArray : nullptr)) {
        setCurrentFilename(filename);
    }
    QApplication::restoreOverrideCursor();
}

void DocumentWindow::openPathAs(const QString &path, const QString &asName)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);

    m_document->clearHistories();
    m_document->reset();
    m_document->clearTurnaround();
    m_document->saveSnapshot();
    
    QFile file(path);
    file.open(QFile::ReadOnly);
    QByteArray fileData = file.readAll();
    
    dust3d::Ds3FileReader ds3Reader((const std::uint8_t *)fileData.data(), fileData.size());
    for (int i = 0; i < ds3Reader.items().size(); ++i) {
        const dust3d::Ds3ReaderItem &item = ds3Reader.items()[i];
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
    
    for (int i = 0; i < ds3Reader.items().size(); ++i) {
        const dust3d::Ds3ReaderItem &item = ds3Reader.items()[i];
        if (item.type == "model") {
            std::vector<std::uint8_t> data;
            ds3Reader.loadItem(item.name, &data);
            data.push_back('\0');
            dust3d::Snapshot snapshot;
            loadSnapshotFromXmlString(&snapshot, (char *)data.data());
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
        for (auto &it: g_documentWindows) {
            it.first->updateRecentFileActions();
        }
    }
    setCurrentFilename(asName);
}

void DocumentWindow::openExample(const QString &modelName)
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

void DocumentWindow::exportObjToFilename(const QString &filename)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    Model *resultMesh = m_document->takeResultMesh();
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

void DocumentWindow::exportFbxToFilename(const QString &filename)
{
    if (!m_document->isExportReady()) {
        qDebug() << "Export but document is not export ready";
        return;
    }
    QApplication::setOverrideCursor(Qt::WaitCursor);
    dust3d::Object skeletonResult = m_document->currentPostProcessedObject();
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

void DocumentWindow::exportGlbToFilename(const QString &filename)
{
    if (!m_document->isExportReady()) {
        qDebug() << "Export but document is not export ready";
        return;
    }
    QApplication::setOverrideCursor(Qt::WaitCursor);
    dust3d::Object skeletonResult = m_document->currentPostProcessedObject();
    QImage *textureMetalnessRoughnessAmbientOcclusionImage = 
        TextureGenerator::combineMetalnessRoughnessAmbientOcclusionImages(m_document->textureMetalnessImage,
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

void DocumentWindow::registerDialog(QWidget *widget)
{
    m_dialogs.push_back(widget);
}

void DocumentWindow::unregisterDialog(QWidget *widget)
{
    m_dialogs.erase(std::remove(m_dialogs.begin(), m_dialogs.end(), widget), m_dialogs.end());
}

void DocumentWindow::showCutFaceSettingPopup(const QPoint &globalPos, std::set<dust3d::Uuid> nodeIds)
{
    QMenu popupMenu;
    
    const SkeletonNode *node = nullptr;
    if (1 == nodeIds.size()) {
        node = m_document->findNode(*nodeIds.begin());
    }
    
    QWidget *popup = new QWidget;
    
    FloatNumberWidget *rotationWidget = new FloatNumberWidget;
    rotationWidget->setItemName(tr("Rotation"));
    rotationWidget->setRange(-1, 1);
    rotationWidget->setValue(0);
    if (nullptr != node) {
        rotationWidget->setValue(node->cutRotation);
    }
    
    connect(rotationWidget, &FloatNumberWidget::valueChanged, [=](float value) {
        m_document->batchChangeBegin();
        for (const auto &id: nodeIds) {
            m_document->setNodeCutRotation(id, value);
        }
        m_document->batchChangeEnd();
        m_document->saveSnapshot();
    });
    
    QPushButton *rotationEraser = new QPushButton(QChar(fa::eraser));
    Theme::initAwesomeToolButton(rotationEraser);
    
    connect(rotationEraser, &QPushButton::clicked, [=]() {
        rotationWidget->setValue(0.0);
        m_document->saveSnapshot();
    });
    
    QHBoxLayout *rotationLayout = new QHBoxLayout;
    rotationLayout->addWidget(rotationEraser);
    rotationLayout->addWidget(rotationWidget);
    
    FlowLayout *cutFaceLayout = nullptr;
    
    std::vector<QPushButton *> buttons;
    std::vector<QString> cutFaceList;
    
    cutFaceLayout = new FlowLayout(nullptr, 0, 0);

    auto updateCutFaceButtonState = [&](size_t index) {
        for (size_t i = 0; i < cutFaceList.size(); ++i) {
            auto button = buttons[i];
            if (i == index) {
                button->setFlat(true);
                button->setEnabled(false);
            } else {
                button->setFlat(false);
                button->setEnabled(true);
            }
        }
    };
    m_document->collectCutFaceList(cutFaceList);
    buttons.resize(cutFaceList.size());
    for (size_t i = 0; i < cutFaceList.size(); ++i) {
        QString cutFaceString = cutFaceList[i];
        dust3d::CutFace cutFace;
        dust3d::Uuid cutFacePartId(cutFaceString.toUtf8().constData());
        QPushButton *button = new QPushButton;
        button->setIconSize(QSize(Theme::toolIconSize * 0.75, Theme::toolIconSize * 0.75));
        if (cutFacePartId.isNull()) {
            cutFace = dust3d::CutFaceFromString(cutFaceString.toUtf8().constData());
            button->setIcon(QIcon(QPixmap::fromImage(*cutFacePreviewImage(cutFace))));
        } else {
            const SkeletonPart *part = m_document->findPart(cutFacePartId);
            if (nullptr != part) {
                button->setIcon(QIcon(part->previewPixmap));
            }
        }
        connect(button, &QPushButton::clicked, [=]() {
            updateCutFaceButtonState(i);
            m_document->batchChangeBegin();
            if (cutFacePartId.isNull()) {
                for (const auto &id: nodeIds) {
                    m_document->setNodeCutFace(id, cutFace);
                }
            } else {
                for (const auto &id: nodeIds) {
                    m_document->setNodeCutFaceLinkedId(id, cutFacePartId);
                }
            }
            m_document->batchChangeEnd();
            m_document->saveSnapshot();
        });
        cutFaceLayout->addWidget(button);
        buttons[i] = button;
    }
    if (nullptr != node) {
        for (size_t i = 0; i < cutFaceList.size(); ++i) {
            if (dust3d::CutFace::UserDefined == node->cutFace) {
                if (QString(node->cutFaceLinkedId.toString().c_str()) == cutFaceList[i]) {
                    updateCutFaceButtonState(i);
                    break;
                }
            } else if (i < (int)dust3d::CutFace::UserDefined) {
                if ((size_t)node->cutFace == i) {
                    updateCutFaceButtonState(i);
                    break;
                }
            }
        }
    }
    
    QVBoxLayout *popupLayout = new QVBoxLayout;
    popupLayout->addLayout(rotationLayout);
    popupLayout->addWidget(new HorizontalLineWidget());
    popupLayout->addLayout(cutFaceLayout);
    
    popup->setLayout(popupLayout);
    
    QWidgetAction action(this);
    action.setDefaultWidget(popup);
    
    popupMenu.addAction(&action);
    
    popupMenu.exec(globalPos);
}

void DocumentWindow::setExportWaitingList(const QStringList &filenames)
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
    for (const auto &filename: list) {
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

void DocumentWindow::generatePartPreviewImages()
{
    if (nullptr != m_partPreviewImagesGenerator) {
        m_isPartPreviewImagesObsolete = true;
        return;
    }
    
    m_isPartPreviewImagesObsolete = false;
     
    QThread *thread = new QThread;
    
    m_partPreviewImagesGenerator = new PartPreviewImagesGenerator(new ModelOffscreenRender(m_modelRenderWidget->format()));
    for (const auto &part: m_document->partMap) {
        if (!part.second.isPreviewMeshObsolete)
            continue;
        m_partPreviewImagesGenerator->addPart(part.first, part.second.takePreviewMesh(), dust3d::PartTarget::CutFace == part.second.target);
    }
    m_partPreviewImagesGenerator->moveToThread(thread);
    connect(thread, &QThread::started, m_partPreviewImagesGenerator, &PartPreviewImagesGenerator::process);
    connect(m_partPreviewImagesGenerator, &PartPreviewImagesGenerator::finished, this, &DocumentWindow::partPreviewImagesReady);
    connect(m_partPreviewImagesGenerator, &PartPreviewImagesGenerator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void DocumentWindow::partPreviewImagesReady()
{
    std::map<dust3d::Uuid, QImage> *partImages = m_partPreviewImagesGenerator->takePartImages();
    if (nullptr != partImages) {
        for (const auto &it: *partImages) {
            SkeletonPart *part = (SkeletonPart *)m_document->findPart(it.first);
            if (nullptr != part) {
                part->isPreviewMeshObsolete = false;
                part->previewPixmap = QPixmap::fromImage(it.second);
            }
        }
        for (const auto &it: *partImages)
            m_partTreeWidget->partPreviewChanged(it.first);
    }
    delete partImages;
    
    delete m_partPreviewImagesGenerator;
    m_partPreviewImagesGenerator = nullptr;
    
    if (m_isPartPreviewImagesObsolete)
        generatePartPreviewImages();
}

ModelWidget *DocumentWindow::modelWidget()
{
    return m_modelRenderWidget;
}

QShortcut *DocumentWindow::createShortcut(QKeySequence key)
{
    auto shortcutIt = m_shortcutMap.find(key);
    if (shortcutIt != m_shortcutMap.end())
        return shortcutIt->second;
    QShortcut *shortcut = new QShortcut(this);
    shortcut->setKey(key);
    m_shortcutMap.insert({key, shortcut});
    return shortcut;
}

#define defineShortcut(keyVal, widget, funcName)                                     \
    QObject::connect(createShortcut(keyVal), &QShortcut::activated, widget, funcName)
        
void DocumentWindow::initializeToolShortcuts(SkeletonGraphicsWidget *graphicsWidget)
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
    defineShortcut(Qt::Key_D, graphicsWidget, &SkeletonGraphicsWidget::shortcutPaintMode);
    defineShortcut(Qt::Key_R, graphicsWidget, &SkeletonGraphicsWidget::shortcutToggleRotation);
    defineShortcut(Qt::Key_O, graphicsWidget, &SkeletonGraphicsWidget::shortcutToggleFlatShading);
    defineShortcut(Qt::Key_W, graphicsWidget, &SkeletonGraphicsWidget::shortcutToggleWireframe);
    defineShortcut(Qt::Key_Escape, graphicsWidget, &SkeletonGraphicsWidget::shortcutEscape);
}

void DocumentWindow::initializeCanvasShortcuts(SkeletonGraphicsWidget *graphicsWidget)
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

    QAction *action = qobject_cast<QAction *>(sender());
    if (action) {
        QString fileName = action->data().toString();
        openPathAs(fileName, fileName);
    }
}

void DocumentWindow::updateRecentFileActions()
{
    QStringList files = Preferences::instance().recentFileList();
    
    for (int i = 0; i < files.size() && i < m_recentFileActions.size(); ++i) {
        QString text = tr("&%1 %2").arg(i + 1).arg(strippedName(files[i]));
        m_recentFileActions[i]->setText(text);
        m_recentFileActions[i]->setData(files[i]);
        m_recentFileActions[i]->setVisible(true);
    }
    for (int j = files.size(); j < m_recentFileActions.size(); ++j)
        m_recentFileActions[j]->setVisible(false);

    m_recentFileSeparatorAction->setVisible(files.size() > 0);
}

QString DocumentWindow::strippedName(const QString &fullFileName)
{
    return QFileInfo(fullFileName).fileName();
}