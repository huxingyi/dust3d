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
#include <qtsingleapplication.h>
#include "documentwindow.h"
#include "skeletongraphicswidget.h"
#include "theme.h"
#include "ds3file.h"
#include "snapshot.h"
#include "snapshotxml.h"
#include "logbrowser.h"
#include "util.h"
#include "aboutwidget.h"
#include "version.h"
#include "glbfile.h"
#include "parttreewidget.h"
#include "rigwidget.h"
#include "markiconcreator.h"
#include "motionmanagewidget.h"
#include "materialmanagewidget.h"
#include "imageforever.h"
#include "spinnableawesomebutton.h"
#include "fbxfile.h"
#include "shortcuts.h"
#include "floatnumberwidget.h"
#include "cutfacelistwidget.h"
#include "scriptwidget.h"
#include "variablesxml.h"
#include "updatescheckwidget.h"
#include "modeloffscreenrender.h"
#include "fileforever.h"
#include "documentsaver.h"

int DocumentWindow::m_modelRenderWidgetInitialX = 16;
int DocumentWindow::m_modelRenderWidgetInitialY = 16;
int DocumentWindow::m_modelRenderWidgetInitialSize = 128;
int DocumentWindow::m_skeletonRenderWidgetInitialX = DocumentWindow::m_modelRenderWidgetInitialX + DocumentWindow::m_modelRenderWidgetInitialSize + 16;
int DocumentWindow::m_skeletonRenderWidgetInitialY = DocumentWindow::m_modelRenderWidgetInitialY;
int DocumentWindow::m_skeletonRenderWidgetInitialSize = DocumentWindow::m_modelRenderWidgetInitialSize;

int DocumentWindow::m_autoRecovered = false;

LogBrowser *g_logBrowser = nullptr;
std::map<DocumentWindow *, QUuid> g_documentWindows;
QTextBrowser *g_acknowlegementsWidget = nullptr;
AboutWidget *g_aboutWidget = nullptr;
QTextBrowser *g_contributorsWidget = nullptr;
UpdatesCheckWidget *g_updatesCheckWidget = nullptr;

void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (g_logBrowser)
        g_logBrowser->outputMessage(type, msg, context.file, context.line);
}

const std::map<DocumentWindow *, QUuid> &DocumentWindow::documentWindows()
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
        g_acknowlegementsWidget->setWindowTitle(unifiedWindowTitle(tr("Acknowlegements")));
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

void DocumentWindow::showContributors()
{
    if (!g_contributorsWidget) {
        g_contributorsWidget = new QTextBrowser;
        g_contributorsWidget->setWindowTitle(unifiedWindowTitle(tr("Contributors")));
        g_contributorsWidget->setMinimumSize(QSize(400, 300));
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

DocumentWindow::DocumentWindow() :
    m_document(nullptr),
    m_firstShow(true),
    m_documentSaved(true),
    m_exportPreviewWidget(nullptr),
    m_preferencesWidget(nullptr),
    m_isLastMeshGenerationSucceed(true),
    m_currentUpdatedMeshId(0)
{
    QObject::connect((QtSingleApplication *)QGuiApplication::instance(), 
            &QtSingleApplication::messageReceived, this, [this](const QString &message) {
        if ("activateFromAnotherInstance" == message) {
            show();
            activateWindow();
            raise();
        }
    });
             
    if (!g_logBrowser) {
        g_logBrowser = new LogBrowser;
        qInstallMessageHandler(&outputMessage);
    }

    g_documentWindows.insert({this, QUuid::createUuid()});

    m_document = new Document;
    
    SkeletonGraphicsWidget *graphicsWidget = new SkeletonGraphicsWidget(m_document);
    m_graphicsWidget = graphicsWidget;

    QVBoxLayout *toolButtonLayout = new QVBoxLayout;
    toolButtonLayout->setSpacing(0);
    toolButtonLayout->setContentsMargins(5, 10, 4, 0);

    QPushButton *addButton = new QPushButton(QChar(fa::plus));
    addButton->setToolTip(tr("Add node to canvas"));
    Theme::initAwesomeButton(addButton);

    QPushButton *selectButton = new QPushButton(QChar(fa::mousepointer));
    selectButton->setToolTip(tr("Select node on canvas"));
    Theme::initAwesomeButton(selectButton);
    
    QPushButton *markerButton = new QPushButton(QChar(fa::edit));
    markerButton->setToolTip(tr("Marker pen"));
    Theme::initAwesomeButton(markerButton);
    
    QPushButton *paintButton = new QPushButton(QChar(fa::paintbrush));
    paintButton->setToolTip(tr("Paint brush"));
    Theme::initAwesomeButton(paintButton);

    //QPushButton *dragButton = new QPushButton(QChar(fa::handrocko));
    //dragButton->setToolTip(tr("Enter drag mode"));
    //Theme::initAwesomeButton(dragButton);

    QPushButton *zoomInButton = new QPushButton(QChar(fa::searchplus));
    zoomInButton->setToolTip(tr("Enter zoom in mode"));
    Theme::initAwesomeButton(zoomInButton);

    QPushButton *zoomOutButton = new QPushButton(QChar(fa::searchminus));
    zoomOutButton->setToolTip(tr("Enter zoom out mode"));
    Theme::initAwesomeButton(zoomOutButton);
    
    m_rotationButton = new QPushButton(QChar(fa::caretsquareoup));
    m_rotationButton->setToolTip(tr("Toggle viewport"));
    Theme::initAwesomeButton(m_rotationButton);
    updateRotationButtonState();

    m_xlockButton = new QPushButton(QChar('X'));
    m_xlockButton->setToolTip(tr("X axis locker"));
    initLockButton(m_xlockButton);
    updateXlockButtonState();

    m_ylockButton = new QPushButton(QChar('Y'));
    m_ylockButton->setToolTip(tr("Y axis locker"));
    initLockButton(m_ylockButton);
    updateYlockButtonState();

    m_zlockButton = new QPushButton(QChar('Z'));
    m_zlockButton->setToolTip(tr("Z axis locker"));
    initLockButton(m_zlockButton);
    updateZlockButtonState();
    
    m_radiusLockButton = new QPushButton(QChar(fa::bullseye));
    m_radiusLockButton->setToolTip(tr("Node radius locker"));
    Theme::initAwesomeButton(m_radiusLockButton);
    updateRadiusLockButtonState();
    
    //QPushButton *rotateCounterclockwiseButton = new QPushButton(QChar(fa::rotateleft));
    //rotateCounterclockwiseButton->setToolTip(tr("Rotate whole model (CCW)"));
    //Theme::initAwesomeButton(rotateCounterclockwiseButton);
    
    //QPushButton *rotateClockwiseButton = new QPushButton(QChar(fa::rotateright));
    //rotateClockwiseButton->setToolTip(tr("Rotate whole model"));
    //Theme::initAwesomeButton(rotateClockwiseButton);
    
    auto updateRegenerateIconAndTips = [&](SpinnableAwesomeButton *regenerateButton, bool isSuccessful, bool forceUpdate=false) {
        if (!forceUpdate) {
            if (m_isLastMeshGenerationSucceed == isSuccessful)
                return;
        }
        m_isLastMeshGenerationSucceed = isSuccessful;
        regenerateButton->setToolTip(m_isLastMeshGenerationSucceed ? tr("Regenerate") : tr("Mesh generation failed, please undo or adjust recent changed nodes\nTips:\n  - Don't let generated mesh self-intersect\n  - Make multiple parts instead of one single part for whole model"));
        regenerateButton->setAwesomeIcon(m_isLastMeshGenerationSucceed ? QChar(fa::recycle) : QChar(fa::warning));
    };
    
    SpinnableAwesomeButton *regenerateButton = new SpinnableAwesomeButton();
    updateRegenerateIconAndTips(regenerateButton, m_isLastMeshGenerationSucceed, true);
    connect(m_document, &Document::meshGenerating, this, [=]() {
        regenerateButton->showSpinner(true);
    });
    connect(m_document, &Document::resultMeshChanged, this, [=]() {
        updateRegenerateIconAndTips(regenerateButton, m_document->isMeshGenerationSucceed());
    });
    connect(m_document, &Document::postProcessing, this, [=]() {
        regenerateButton->showSpinner(true);
    });
    connect(m_document, &Document::textureGenerating, this, [=]() {
        regenerateButton->showSpinner(true);
    });
    connect(m_document, &Document::resultTextureChanged, this, [=]() {
        if (!m_document->isMeshGenerating() &&
                !m_document->isPostProcessing() &&
                !m_document->isTextureGenerating()) {
            regenerateButton->showSpinner(false);
        }
    });
    connect(regenerateButton->button(), &QPushButton::clicked, m_document, &Document::regenerateMesh);

    toolButtonLayout->addWidget(addButton);
    toolButtonLayout->addWidget(selectButton);
    toolButtonLayout->addWidget(markerButton);
    toolButtonLayout->addWidget(paintButton);
    //toolButtonLayout->addWidget(dragButton);
    toolButtonLayout->addWidget(zoomInButton);
    toolButtonLayout->addWidget(zoomOutButton);
    toolButtonLayout->addSpacing(10);
    toolButtonLayout->addWidget(m_rotationButton);
    toolButtonLayout->addSpacing(10);
    toolButtonLayout->addWidget(m_xlockButton);
    toolButtonLayout->addWidget(m_ylockButton);
    toolButtonLayout->addWidget(m_zlockButton);
    toolButtonLayout->addWidget(m_radiusLockButton);
    toolButtonLayout->addSpacing(10);
    //toolButtonLayout->addWidget(rotateCounterclockwiseButton);
    //toolButtonLayout->addWidget(rotateClockwiseButton);
    //toolButtonLayout->addSpacing(10);
    toolButtonLayout->addWidget(regenerateButton);
    

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
    containerWidget->setGraphicsWidget(graphicsWidget);
    QGridLayout *containerLayout = new QGridLayout;
    containerLayout->setSpacing(0);
    containerLayout->setContentsMargins(1, 0, 0, 0);
    containerLayout->addWidget(graphicsWidget);
    containerWidget->setLayout(containerLayout);
    containerWidget->setMinimumSize(400, 400);
    
    m_graphicsContainerWidget = containerWidget;
    
    //m_infoWidget = new QLabel(containerWidget);
    //m_infoWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
    //QGraphicsOpacityEffect *graphicsOpacityEffect = new QGraphicsOpacityEffect(m_infoWidget);
    //graphicsOpacityEffect->setOpacity(0.5);
    //m_infoWidget->setGraphicsEffect(graphicsOpacityEffect);
    //updateInfoWidgetPosition();
    
    //connect(containerWidget, &GraphicsContainerWidget::containerSizeChanged, this, &DocumentWindow::updateInfoWidgetPosition);

    m_modelRenderWidget = new ModelWidget(containerWidget);
    m_modelRenderWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_modelRenderWidget->setMinimumSize(DocumentWindow::m_modelRenderWidgetInitialSize, DocumentWindow::m_modelRenderWidgetInitialSize);
    m_modelRenderWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_modelRenderWidget->move(DocumentWindow::m_modelRenderWidgetInitialX, DocumentWindow::m_modelRenderWidgetInitialY);
    m_modelRenderWidget->setMousePickRadius(m_document->mousePickRadius());
    if (!Preferences::instance().toonShading())
		m_modelRenderWidget->toggleWireframe();
    m_modelRenderWidget->enableEnvironmentLight();
    
    connect(m_modelRenderWidget, &ModelWidget::mouseRayChanged, m_document,
            [=](const QVector3D &nearPosition, const QVector3D &farPosition) {
        std::set<QUuid> nodeIdSet;
        graphicsWidget->readSkeletonNodeAndEdgeIdSetFromRangeSelection(&nodeIdSet);
        m_document->setMousePickMaskNodeIds(nodeIdSet);
        m_document->pickMouseTarget(nearPosition, farPosition);
    });
    connect(m_modelRenderWidget, &ModelWidget::mousePressed, m_document, [=]() {
        m_document->startPaint();
        if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier))
            m_document->setPaintMode(PaintMode::Push);
        else
            m_document->setPaintMode(PaintMode::Pull);
    });
    connect(m_modelRenderWidget, &ModelWidget::mouseReleased, m_document, [=]() {
        m_document->setPaintMode(PaintMode::None);
        m_document->stopPaint();
    });
    connect(m_modelRenderWidget, &ModelWidget::addMouseRadius, m_document, [=](float radius) {
        m_document->setMousePickRadius(m_document->mousePickRadius() + radius);
    });
    connect(m_document, &Document::mousePickRadiusChanged, this, [=]() {
        m_modelRenderWidget->setMousePickRadius(m_document->mousePickRadius());
    });
    connect(m_document, &Document::mouseTargetChanged, this, [=]() {
        m_modelRenderWidget->setMousePickTargetPositionInModelSpace(m_document->mouseTargetPosition());
    });
    
    connect(m_modelRenderWidget, &ModelWidget::renderParametersChanged, this, &DocumentWindow::delayedGenerateNormalAndDepthMaps);
    
    m_graphicsWidget->setModelWidget(m_modelRenderWidget);
    containerWidget->setModelWidget(m_modelRenderWidget);
    
    setTabPosition(Qt::RightDockWidgetArea, QTabWidget::East);

    QDockWidget *partTreeDocker = new QDockWidget(tr("Parts"), this);
    partTreeDocker->setAllowedAreas(Qt::RightDockWidgetArea);
    PartTreeWidget *partTreeWidget = new PartTreeWidget(m_document, partTreeDocker);
    partTreeDocker->setWidget(partTreeWidget);
    addDockWidget(Qt::RightDockWidgetArea, partTreeDocker);
    connect(partTreeDocker, &QDockWidget::topLevelChanged, [=](bool topLevel) {
        Q_UNUSED(topLevel);
        for (const auto &part: m_document->partMap)
            partTreeWidget->partPreviewChanged(part.first);
    });
    
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
    
    QDockWidget *rigDocker = new QDockWidget(tr("Rig"), this);
    rigDocker->setAllowedAreas(Qt::RightDockWidgetArea);
    m_rigWidget = new RigWidget(m_document, rigDocker);
    rigDocker->setWidget(m_rigWidget);
    addDockWidget(Qt::RightDockWidgetArea, rigDocker);
    connect(rigDocker, &QDockWidget::topLevelChanged, [=](bool topLevel) {
        Q_UNUSED(topLevel);
        updateRigWeightRenderWidget();
    });
    
    QDockWidget *poseDocker = new QDockWidget(tr("Poses"), this);
    poseDocker->setAllowedAreas(Qt::RightDockWidgetArea);
    PoseManageWidget *poseManageWidget = new PoseManageWidget(m_document, poseDocker);
    poseDocker->setWidget(poseManageWidget);
    connect(poseManageWidget, &PoseManageWidget::registerDialog, this, &DocumentWindow::registerDialog);
    connect(poseManageWidget, &PoseManageWidget::unregisterDialog, this, &DocumentWindow::unregisterDialog);
    addDockWidget(Qt::RightDockWidgetArea, poseDocker);
    connect(poseDocker, &QDockWidget::topLevelChanged, [=](bool topLevel) {
        Q_UNUSED(topLevel);
        for (const auto &pose: m_document->poseMap)
            emit m_document->posePreviewChanged(pose.first);
    });
    
    QDockWidget *motionDocker = new QDockWidget(tr("Motions"), this);
    motionDocker->setAllowedAreas(Qt::RightDockWidgetArea);
    MotionManageWidget *motionManageWidget = new MotionManageWidget(m_document, motionDocker);
    motionDocker->setWidget(motionManageWidget);
    connect(motionManageWidget, &MotionManageWidget::registerDialog, this, &DocumentWindow::registerDialog);
    connect(motionManageWidget, &MotionManageWidget::unregisterDialog, this, &DocumentWindow::unregisterDialog);
    addDockWidget(Qt::RightDockWidgetArea, motionDocker);
    
    QDockWidget *scriptDocker = new QDockWidget(tr("Script"), this);
    scriptDocker->setAllowedAreas(Qt::RightDockWidgetArea);
    ScriptWidget *scriptWidget = new ScriptWidget(m_document, scriptDocker);
    scriptDocker->setWidget(scriptWidget);
    addDockWidget(Qt::RightDockWidgetArea, scriptDocker);
    
    tabifyDockWidget(partTreeDocker, materialDocker);
    tabifyDockWidget(materialDocker, rigDocker);
    tabifyDockWidget(rigDocker, poseDocker);
    tabifyDockWidget(poseDocker, motionDocker);
    tabifyDockWidget(motionDocker, scriptDocker);
    
    partTreeDocker->raise();

    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addLayout(mainLeftLayout);
    mainLayout->addWidget(containerWidget);
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
        "Backpacker",
        "Bicycle",
        "Cat",
        "Dog",
        "Giraffe",
        "Meerkat",
        "Mosquito",
        "Seagull",
        "Procedural Tree"
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
    
    m_showPreferencesAction = new QAction(tr("Preferences..."), this);
    connect(m_showPreferencesAction, &QAction::triggered, this, &DocumentWindow::showPreferences);
    m_fileMenu->addAction(m_showPreferencesAction);
    
    m_fileMenu->addSeparator();
    
    m_importAction = new QAction(tr("Import..."), this);
    connect(m_importAction, &QAction::triggered, this, &DocumentWindow::import, Qt::QueuedConnection);
    m_fileMenu->addAction(m_importAction);
    
    m_fileMenu->addSeparator();

    //m_exportMenu = m_fileMenu->addMenu(tr("Export"));

    m_exportAction = new QAction(tr("Export..."), this);
    connect(m_exportAction, &QAction::triggered, this, &DocumentWindow::showExportPreview, Qt::QueuedConnection);
    m_fileMenu->addAction(m_exportAction);
    
    m_exportAsObjAction = new QAction(tr("Export as OBJ..."), this);
    connect(m_exportAsObjAction, &QAction::triggered, this, &DocumentWindow::exportObjResult, Qt::QueuedConnection);
    m_fileMenu->addAction(m_exportAsObjAction);
    
    m_exportRenderedAsImageAction = new QAction(tr("Export as Image..."), this);
    connect(m_exportRenderedAsImageAction, &QAction::triggered, this, &DocumentWindow::exportRenderedResult, Qt::QueuedConnection);
    m_fileMenu->addAction(m_exportRenderedAsImageAction);
    
    //m_exportAsObjPlusMaterialsAction = new QAction(tr("Wavefront (.obj + .mtl)..."), this);
    //connect(m_exportAsObjPlusMaterialsAction, &QAction::triggered, this, &SkeletonDocumentWindow::exportObjPlusMaterialsResult, Qt::QueuedConnection);
    //m_exportMenu->addAction(m_exportAsObjPlusMaterialsAction);
    
    m_fileMenu->addSeparator();

    m_changeTurnaroundAction = new QAction(tr("Change Reference Sheet..."), this);
    connect(m_changeTurnaroundAction, &QAction::triggered, this, &DocumentWindow::changeTurnaround, Qt::QueuedConnection);
    m_fileMenu->addAction(m_changeTurnaroundAction);
    
    m_fileMenu->addSeparator();

    m_quitAction = m_fileMenu->addAction(tr("&Quit"),
                                         this, &DocumentWindow::close,
                                         QKeySequence::Quit);

    connect(m_fileMenu, &QMenu::aboutToShow, [=]() {
        m_exportAsObjAction->setEnabled(m_graphicsWidget->hasItems());
        //m_exportAsObjPlusMaterialsAction->setEnabled(m_graphicsWidget->hasItems());
        m_exportAction->setEnabled(m_graphicsWidget->hasItems());
        m_exportRenderedAsImageAction->setEnabled(m_graphicsWidget->hasItems());
    });

    m_editMenu = menuBar()->addMenu(tr("&Edit"));

    m_addAction = new QAction(tr("Add..."), this);
    connect(m_addAction, &QAction::triggered, [=]() {
        m_document->setEditMode(SkeletonDocumentEditMode::Add);
    });
    m_editMenu->addAction(m_addAction);

    m_undoAction = new QAction(tr("Undo"), this);
    connect(m_undoAction, &QAction::triggered, m_document, &Document::undo);
    m_editMenu->addAction(m_undoAction);

    m_redoAction = new QAction(tr("Redo"), this);
    connect(m_redoAction, &QAction::triggered, m_document, &Document::redo);
    m_editMenu->addAction(m_redoAction);

    m_deleteAction = new QAction(tr("Delete"), this);
    connect(m_deleteAction, &QAction::triggered, m_graphicsWidget, &SkeletonGraphicsWidget::deleteSelected);
    m_editMenu->addAction(m_deleteAction);

    m_breakAction = new QAction(tr("Break"), this);
    connect(m_breakAction, &QAction::triggered, m_graphicsWidget, &SkeletonGraphicsWidget::breakSelected);
    m_editMenu->addAction(m_breakAction);

    m_reverseAction = new QAction(tr("Reverse"), this);
    connect(m_reverseAction, &QAction::triggered, m_graphicsWidget, &SkeletonGraphicsWidget::reverseSelectedEdges);
    m_editMenu->addAction(m_reverseAction);

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
    connect(m_pasteAction, &QAction::triggered, m_document, &Document::paste);
    m_editMenu->addAction(m_pasteAction);

    m_flipHorizontallyAction = new QAction(tr("H Flip"), this);
    connect(m_flipHorizontallyAction, &QAction::triggered, m_graphicsWidget, &SkeletonGraphicsWidget::flipHorizontally);
    m_editMenu->addAction(m_flipHorizontallyAction);

    m_flipVerticallyAction = new QAction(tr("V Flip"), this);
    connect(m_flipVerticallyAction, &QAction::triggered, m_graphicsWidget, &SkeletonGraphicsWidget::flipVertically);
    m_editMenu->addAction(m_flipVerticallyAction);

    m_rotateClockwiseAction = new QAction(tr("Rotate 90D CW"), this);
    connect(m_rotateClockwiseAction, &QAction::triggered, [=] {
        m_graphicsWidget->rotateClockwise90Degree();
    });
    m_editMenu->addAction(m_rotateClockwiseAction);

    m_rotateCounterclockwiseAction = new QAction(tr("Rotate 90D CCW"), this);
    connect(m_rotateCounterclockwiseAction, &QAction::triggered, [=] {
        m_graphicsWidget->rotateCounterclockwise90Degree();
    });
    m_editMenu->addAction(m_rotateCounterclockwiseAction);

    m_switchXzAction = new QAction(tr("Switch XZ"), this);
    connect(m_switchXzAction, &QAction::triggered, [=] {
        m_graphicsWidget->switchSelectedXZ();
    });
    m_editMenu->addAction(m_switchXzAction);
    
    m_setCutFaceAction = new QAction(tr("Cut Face..."), this);
    connect(m_setCutFaceAction, &QAction::triggered, [=] {
        m_graphicsWidget->showSelectedCutFaceSettingPopup(m_graphicsWidget->mapFromGlobal(QCursor::pos()));
    });
    m_editMenu->addAction(m_setCutFaceAction);
    
    m_clearCutFaceAction = new QAction(tr("Clear Cut Face"), this);
    connect(m_clearCutFaceAction, &QAction::triggered, [=] {
        m_graphicsWidget->clearSelectedCutFace();
    });
    m_editMenu->addAction(m_clearCutFaceAction);
    
    //m_createWrapPartsAction = new QAction(tr("Create Wrap Parts"), this);
    //connect(m_createWrapPartsAction, &QAction::triggered, [=] {
    //    m_graphicsWidget->createWrapParts();
    //});
    //m_editMenu->addAction(m_createWrapPartsAction);

    m_alignToMenu = new QMenu(tr("Align To"));

    m_alignToLocalCenterAction = new QAction(tr("Local Center"), this);
    connect(m_alignToLocalCenterAction, &QAction::triggered, m_graphicsWidget, &SkeletonGraphicsWidget::alignSelectedToLocalCenter);
    m_alignToMenu->addAction(m_alignToLocalCenterAction);

    m_alignToLocalVerticalCenterAction = new QAction(tr("Local Vertical Center"), this);
    connect(m_alignToLocalVerticalCenterAction, &QAction::triggered, m_graphicsWidget, &SkeletonGraphicsWidget::alignSelectedToLocalVerticalCenter);
    m_alignToMenu->addAction(m_alignToLocalVerticalCenterAction);

    m_alignToLocalHorizontalCenterAction = new QAction(tr("Local Horizontal Center"), this);
    connect(m_alignToLocalHorizontalCenterAction, &QAction::triggered, m_graphicsWidget, &SkeletonGraphicsWidget::alignSelectedToLocalHorizontalCenter);
    m_alignToMenu->addAction(m_alignToLocalHorizontalCenterAction);
    
    m_alignToGlobalCenterAction = new QAction(tr("Global Center"), this);
    connect(m_alignToGlobalCenterAction, &QAction::triggered, m_graphicsWidget, &SkeletonGraphicsWidget::alignSelectedToGlobalCenter);
    m_alignToMenu->addAction(m_alignToGlobalCenterAction);

    m_alignToGlobalVerticalCenterAction = new QAction(tr("Global Vertical Center"), this);
    connect(m_alignToGlobalVerticalCenterAction, &QAction::triggered, m_graphicsWidget, &SkeletonGraphicsWidget::alignSelectedToGlobalVerticalCenter);
    m_alignToMenu->addAction(m_alignToGlobalVerticalCenterAction);

    m_alignToGlobalHorizontalCenterAction = new QAction(tr("Global Horizontal Center"), this);
    connect(m_alignToGlobalHorizontalCenterAction, &QAction::triggered, m_graphicsWidget, &SkeletonGraphicsWidget::alignSelectedToGlobalHorizontalCenter);
    m_alignToMenu->addAction(m_alignToGlobalHorizontalCenterAction);

    m_editMenu->addMenu(m_alignToMenu);
    
    m_markAsMenu = new QMenu(tr("Mark As"));

    m_markAsNoneAction = new QAction(tr("None"), this);
    connect(m_markAsNoneAction, &QAction::triggered, [=]() {
        m_graphicsWidget->setSelectedNodesBoneMark(BoneMark::None);
    });
    m_markAsMenu->addAction(m_markAsNoneAction);

    m_markAsMenu->addSeparator();

    for (int i = 0; i < (int)BoneMark::Count - 1; i++) {
        BoneMark boneMark = (BoneMark)(i + 1);
        m_markAsActions[i] = new QAction(MarkIconCreator::createIcon(boneMark), BoneMarkToDispName(boneMark), this);
        connect(m_markAsActions[i], &QAction::triggered, [=]() {
            m_graphicsWidget->setSelectedNodesBoneMark(boneMark);
        });
        m_markAsMenu->addAction(m_markAsActions[i]);
    }

    m_editMenu->addMenu(m_markAsMenu);
    
    m_colorizeAsMenu = new QMenu(tr("Colorize"));
    
    m_colorizeAsBlankAction = new QAction(tr("Blank"), this);
    connect(m_colorizeAsBlankAction, &QAction::triggered, [=]() {
        m_graphicsWidget->fadeSelected();
    });
    m_colorizeAsMenu->addAction(m_colorizeAsBlankAction);
    
    m_colorizeAsAutoAction = new QAction(tr("Auto Color"), this);
    connect(m_colorizeAsAutoAction, &QAction::triggered, [=]() {
        m_graphicsWidget->colorizeSelected();
    });
    m_colorizeAsMenu->addAction(m_colorizeAsAutoAction);
    
    m_editMenu->addMenu(m_colorizeAsMenu);

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
        m_reverseAction->setEnabled(m_graphicsWidget->hasEdgeSelection());
        m_connectAction->setEnabled(m_graphicsWidget->hasTwoDisconnectedNodesSelection());
        m_cutAction->setEnabled(m_graphicsWidget->hasSelection());
        m_copyAction->setEnabled(m_graphicsWidget->hasSelection());
        m_pasteAction->setEnabled(m_document->hasPastableNodesInClipboard());
        m_flipHorizontallyAction->setEnabled(m_graphicsWidget->hasMultipleSelection());
        m_flipVerticallyAction->setEnabled(m_graphicsWidget->hasMultipleSelection());
        m_rotateClockwiseAction->setEnabled(m_graphicsWidget->hasMultipleSelection());
        m_rotateCounterclockwiseAction->setEnabled(m_graphicsWidget->hasMultipleSelection());
        m_switchXzAction->setEnabled(m_graphicsWidget->hasSelection());
        m_setCutFaceAction->setEnabled(m_graphicsWidget->hasSelection());
        m_clearCutFaceAction->setEnabled(m_graphicsWidget->hasCutFaceAdjustedNodesSelection());
        //m_createWrapPartsAction->setEnabled(m_graphicsWidget->hasSelection());
        m_colorizeAsBlankAction->setEnabled(m_graphicsWidget->hasSelection());
        m_colorizeAsAutoAction->setEnabled(m_graphicsWidget->hasSelection());
        m_alignToGlobalCenterAction->setEnabled(m_graphicsWidget->hasSelection() && m_document->originSettled());
        m_alignToGlobalVerticalCenterAction->setEnabled(m_graphicsWidget->hasSelection() && m_document->originSettled());
        m_alignToGlobalHorizontalCenterAction->setEnabled(m_graphicsWidget->hasSelection() && m_document->originSettled());
        m_alignToLocalCenterAction->setEnabled(m_graphicsWidget->hasMultipleSelection());
        m_alignToLocalVerticalCenterAction->setEnabled(m_graphicsWidget->hasMultipleSelection());
        m_alignToLocalHorizontalCenterAction->setEnabled(m_graphicsWidget->hasMultipleSelection());
        m_alignToMenu->setEnabled((m_graphicsWidget->hasSelection() && m_document->originSettled()) || m_graphicsWidget->hasMultipleSelection());
        m_selectAllAction->setEnabled(m_graphicsWidget->hasItems());
        m_selectPartAllAction->setEnabled(m_graphicsWidget->hasItems());
        m_unselectAllAction->setEnabled(m_graphicsWidget->hasSelection());
    });

    m_viewMenu = menuBar()->addMenu(tr("&View"));

    auto isModelSitInVisibleArea = [](ModelWidget *modelWidget) {
        QRect parentRect = QRect(QPoint(0, 0), modelWidget->parentWidget()->size());
        return parentRect.contains(modelWidget->geometry().center());
    };

    m_resetModelWidgetPosAction = new QAction(tr("Show Model"), this);
    connect(m_resetModelWidgetPosAction, &QAction::triggered, [=]() {
        if (!isModelSitInVisibleArea(m_modelRenderWidget)) {
            m_modelRenderWidget->move(DocumentWindow::m_modelRenderWidgetInitialX, DocumentWindow::m_modelRenderWidgetInitialY);
        }
    });
    m_viewMenu->addAction(m_resetModelWidgetPosAction);

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
        if (m_document->isMeshGenerating() &&
                m_document->isPostProcessing() &&
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
    
    m_toggleUvCheckAction = new QAction(tr("Toggle UV Check"), this);
    connect(m_toggleUvCheckAction, &QAction::triggered, [=]() {
        m_modelRenderWidget->toggleUvCheck();
    });
    m_viewMenu->addAction(m_toggleUvCheckAction);

    connect(m_viewMenu, &QMenu::aboutToShow, [=]() {
        m_resetModelWidgetPosAction->setEnabled(!isModelSitInVisibleArea(m_modelRenderWidget));
    });
    
    m_windowMenu = menuBar()->addMenu(tr("&Window"));
    
    m_showPartsListAction = new QAction(tr("Parts"), this);
    connect(m_showPartsListAction, &QAction::triggered, [=]() {
        partTreeDocker->show();
        partTreeDocker->raise();
    });
    m_windowMenu->addAction(m_showPartsListAction);
    
    m_showMaterialsAction = new QAction(tr("Materials"), this);
    connect(m_showMaterialsAction, &QAction::triggered, [=]() {
        materialDocker->show();
        materialDocker->raise();
    });
    m_windowMenu->addAction(m_showMaterialsAction);
    
    m_showRigAction = new QAction(tr("Rig"), this);
    connect(m_showRigAction, &QAction::triggered, [=]() {
        rigDocker->show();
        rigDocker->raise();
    });
    m_windowMenu->addAction(m_showRigAction);
    
    m_showPosesAction = new QAction(tr("Poses"), this);
    connect(m_showPosesAction, &QAction::triggered, [=]() {
        poseDocker->show();
        poseDocker->raise();
    });
    m_windowMenu->addAction(m_showPosesAction);
    
    m_showMotionsAction = new QAction(tr("Motions"), this);
    connect(m_showMotionsAction, &QAction::triggered, [=]() {
        motionDocker->show();
        motionDocker->raise();
    });
    m_windowMenu->addAction(m_showMotionsAction);
    
    m_showScriptAction = new QAction(tr("Script"), this);
    connect(m_showScriptAction, &QAction::triggered, [=]() {
        scriptDocker->show();
        scriptDocker->raise();
    });
    m_windowMenu->addAction(m_showScriptAction);
    
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

    m_seeAcknowlegementsAction = new QAction(tr("Acknowlegements"), this);
    connect(m_seeAcknowlegementsAction, &QAction::triggered, this, &DocumentWindow::seeAcknowlegements);
    m_helpMenu->addAction(m_seeAcknowlegementsAction);

    connect(containerWidget, &GraphicsContainerWidget::containerSizeChanged,
        graphicsWidget, &SkeletonGraphicsWidget::canvasResized);

    connect(m_document, &Document::turnaroundChanged,
        graphicsWidget, &SkeletonGraphicsWidget::turnaroundChanged);

    //connect(rotateCounterclockwiseButton, &QPushButton::clicked, graphicsWidget, &SkeletonGraphicsWidget::rotateAllMainProfileCounterclockwise90DegreeAlongOrigin);
    //connect(rotateClockwiseButton, &QPushButton::clicked, graphicsWidget, &SkeletonGraphicsWidget::rotateAllMainProfileClockwise90DegreeAlongOrigin);

    connect(addButton, &QPushButton::clicked, [=]() {
        m_document->setEditMode(SkeletonDocumentEditMode::Add);
    });

    connect(selectButton, &QPushButton::clicked, [=]() {
        m_document->setEditMode(SkeletonDocumentEditMode::Select);
    });
    
    connect(markerButton, &QPushButton::clicked, [=]() {
        m_document->setEditMode(SkeletonDocumentEditMode::Mark);
    });
    
    connect(paintButton, &QPushButton::clicked, [=]() {
        m_document->setEditMode(SkeletonDocumentEditMode::Paint);
    });

    //connect(dragButton, &QPushButton::clicked, [=]() {
    //    m_document->setEditMode(SkeletonDocumentEditMode::Drag);
    //});

    connect(zoomInButton, &QPushButton::clicked, [=]() {
        m_document->setEditMode(SkeletonDocumentEditMode::ZoomIn);
    });

    connect(zoomOutButton, &QPushButton::clicked, [=]() {
        m_document->setEditMode(SkeletonDocumentEditMode::ZoomOut);
    });
    
    connect(m_rotationButton, &QPushButton::clicked, this, &DocumentWindow::toggleRotation);

    connect(m_xlockButton, &QPushButton::clicked, [=]() {
        m_document->setXlockState(!m_document->xlocked);
    });
    connect(m_ylockButton, &QPushButton::clicked, [=]() {
        m_document->setYlockState(!m_document->ylocked);
    });
    connect(m_zlockButton, &QPushButton::clicked, [=]() {
        m_document->setZlockState(!m_document->zlocked);
    });
    connect(m_radiusLockButton, &QPushButton::clicked, [=]() {
        m_document->setRadiusLockState(!m_document->radiusLocked);
    });
    
    connect(m_document, &Document::editModeChanged, this, [=]() {
        m_modelRenderWidget->enableMousePicking(SkeletonDocumentEditMode::Paint == m_document->editMode);
    });

    m_partListDockerVisibleSwitchConnection = connect(m_document, &Document::skeletonChanged, [=]() {
        if (m_graphicsWidget->hasItems()) {
            if (partTreeDocker->isHidden())
                partTreeDocker->show();
            disconnect(m_partListDockerVisibleSwitchConnection);
        }
    });

    connect(m_document, &Document::editModeChanged, graphicsWidget, &SkeletonGraphicsWidget::editModeChanged);
    
    connect(graphicsWidget, &SkeletonGraphicsWidget::shortcutToggleWireframe, [=]() {
        m_modelRenderWidget->toggleWireframe();
    });
    
    connect(graphicsWidget, &SkeletonGraphicsWidget::shortcutToggleFlatShading, [=]() {
        Preferences::instance().setFlatShading(!Preferences::instance().flatShading());
    });
    
    connect(graphicsWidget, &SkeletonGraphicsWidget::shortcutToggleRotation, this, &DocumentWindow::toggleRotation);

    connect(graphicsWidget, &SkeletonGraphicsWidget::zoomRenderedModelBy, m_modelRenderWidget, &ModelWidget::zoom);

    connect(graphicsWidget, &SkeletonGraphicsWidget::addNode, m_document, &Document::addNode);
    connect(graphicsWidget, &SkeletonGraphicsWidget::scaleNodeByAddRadius, m_document, &Document::scaleNodeByAddRadius);
    connect(graphicsWidget, &SkeletonGraphicsWidget::moveNodeBy, m_document, &Document::moveNodeBy);
    connect(graphicsWidget, &SkeletonGraphicsWidget::setNodeOrigin, m_document, &Document::setNodeOrigin);
    connect(graphicsWidget, &SkeletonGraphicsWidget::setNodeBoneMark, m_document, &Document::setNodeBoneMark);
    connect(graphicsWidget, &SkeletonGraphicsWidget::clearNodeCutFaceSettings, m_document, &Document::clearNodeCutFaceSettings);
    connect(graphicsWidget, &SkeletonGraphicsWidget::removeNode, m_document, &Document::removeNode);
    connect(graphicsWidget, &SkeletonGraphicsWidget::setEditMode, m_document, &Document::setEditMode);
    connect(graphicsWidget, &SkeletonGraphicsWidget::removeEdge, m_document, &Document::removeEdge);
    connect(graphicsWidget, &SkeletonGraphicsWidget::addEdge, m_document, &Document::addEdge);
    connect(graphicsWidget, &SkeletonGraphicsWidget::groupOperationAdded, m_document, &Document::saveSnapshot);
    connect(graphicsWidget, &SkeletonGraphicsWidget::undo, m_document, &Document::undo);
    connect(graphicsWidget, &SkeletonGraphicsWidget::redo, m_document, &Document::redo);
    connect(graphicsWidget, &SkeletonGraphicsWidget::paste, m_document, &Document::paste);
    connect(graphicsWidget, &SkeletonGraphicsWidget::batchChangeBegin, m_document, &Document::batchChangeBegin);
    connect(graphicsWidget, &SkeletonGraphicsWidget::batchChangeEnd, m_document, &Document::batchChangeEnd);
    connect(graphicsWidget, &SkeletonGraphicsWidget::breakEdge, m_document, &Document::breakEdge);
    connect(graphicsWidget, &SkeletonGraphicsWidget::reverseEdge, m_document, &Document::reverseEdge);
    connect(graphicsWidget, &SkeletonGraphicsWidget::moveOriginBy, m_document, &Document::moveOriginBy);
    connect(graphicsWidget, &SkeletonGraphicsWidget::partChecked, m_document, &Document::partChecked);
    connect(graphicsWidget, &SkeletonGraphicsWidget::partUnchecked, m_document, &Document::partUnchecked);
    connect(graphicsWidget, &SkeletonGraphicsWidget::switchNodeXZ, m_document, &Document::switchNodeXZ);

    connect(graphicsWidget, &SkeletonGraphicsWidget::setPartLockState, m_document, &Document::setPartLockState);
    connect(graphicsWidget, &SkeletonGraphicsWidget::setPartVisibleState, m_document, &Document::setPartVisibleState);
    connect(graphicsWidget, &SkeletonGraphicsWidget::setPartSubdivState, m_document, &Document::setPartSubdivState);
    connect(graphicsWidget, &SkeletonGraphicsWidget::setPartChamferState, m_document, &Document::setPartChamferState);
    connect(graphicsWidget, &SkeletonGraphicsWidget::setPartColorState, m_document, &Document::setPartColorState);
    connect(graphicsWidget, &SkeletonGraphicsWidget::setPartDisableState, m_document, &Document::setPartDisableState);
    connect(graphicsWidget, &SkeletonGraphicsWidget::setPartXmirrorState, m_document, &Document::setPartXmirrorState);
    connect(graphicsWidget, &SkeletonGraphicsWidget::setPartRoundState, m_document, &Document::setPartRoundState);
    connect(graphicsWidget, &SkeletonGraphicsWidget::setPartWrapState, m_document, &Document::setPartCutRotation);
    connect(graphicsWidget, &SkeletonGraphicsWidget::addPartByPolygons, m_document, &Document::addPartByPolygons);

    connect(graphicsWidget, &SkeletonGraphicsWidget::setXlockState, m_document, &Document::setXlockState);
    connect(graphicsWidget, &SkeletonGraphicsWidget::setYlockState, m_document, &Document::setYlockState);
    connect(graphicsWidget, &SkeletonGraphicsWidget::setZlockState, m_document, &Document::setZlockState);
    
    connect(graphicsWidget, &SkeletonGraphicsWidget::enableAllPositionRelatedLocks, m_document, &Document::enableAllPositionRelatedLocks);
    connect(graphicsWidget, &SkeletonGraphicsWidget::disableAllPositionRelatedLocks, m_document, &Document::disableAllPositionRelatedLocks);

    connect(graphicsWidget, &SkeletonGraphicsWidget::changeTurnaround, this, &DocumentWindow::changeTurnaround);
    connect(graphicsWidget, &SkeletonGraphicsWidget::open, this, &DocumentWindow::open);
    connect(graphicsWidget, &SkeletonGraphicsWidget::showCutFaceSettingPopup, this, &DocumentWindow::showCutFaceSettingPopup);
    
    connect(graphicsWidget, &SkeletonGraphicsWidget::showOrHideAllComponents, m_document, &Document::showOrHideAllComponents);
    
    connect(m_document, &Document::nodeAdded, graphicsWidget, &SkeletonGraphicsWidget::nodeAdded);
    connect(m_document, &Document::nodeRemoved, graphicsWidget, &SkeletonGraphicsWidget::nodeRemoved);
    connect(m_document, &Document::edgeAdded, graphicsWidget, &SkeletonGraphicsWidget::edgeAdded);
    connect(m_document, &Document::edgeRemoved, graphicsWidget, &SkeletonGraphicsWidget::edgeRemoved);
    connect(m_document, &Document::nodeRadiusChanged, graphicsWidget, &SkeletonGraphicsWidget::nodeRadiusChanged);
    connect(m_document, &Document::nodeBoneMarkChanged, graphicsWidget, &SkeletonGraphicsWidget::nodeBoneMarkChanged);
    connect(m_document, &Document::nodeOriginChanged, graphicsWidget, &SkeletonGraphicsWidget::nodeOriginChanged);
    connect(m_document, &Document::edgeReversed, graphicsWidget, &SkeletonGraphicsWidget::edgeReversed);
    connect(m_document, &Document::partVisibleStateChanged, graphicsWidget, &SkeletonGraphicsWidget::partVisibleStateChanged);
    connect(m_document, &Document::partDisableStateChanged, graphicsWidget, &SkeletonGraphicsWidget::partVisibleStateChanged);
    connect(m_document, &Document::cleanup, graphicsWidget, &SkeletonGraphicsWidget::removeAllContent);
    connect(m_document, &Document::originChanged, graphicsWidget, &SkeletonGraphicsWidget::originChanged);
    connect(m_document, &Document::checkPart, graphicsWidget, &SkeletonGraphicsWidget::selectPartAllById);
    connect(m_document, &Document::enableBackgroundBlur, graphicsWidget, &SkeletonGraphicsWidget::enableBackgroundBlur);
    connect(m_document, &Document::disableBackgroundBlur, graphicsWidget, &SkeletonGraphicsWidget::disableBackgroundBlur);
    connect(m_document, &Document::uncheckAll, graphicsWidget, &SkeletonGraphicsWidget::unselectAll);
    connect(m_document, &Document::checkNode, graphicsWidget, &SkeletonGraphicsWidget::addSelectNode);
    connect(m_document, &Document::checkEdge, graphicsWidget, &SkeletonGraphicsWidget::addSelectEdge);
    
    connect(partTreeWidget, &PartTreeWidget::currentComponentChanged, m_document, &Document::setCurrentCanvasComponentId);
    connect(partTreeWidget, &PartTreeWidget::moveComponentUp, m_document, &Document::moveComponentUp);
    connect(partTreeWidget, &PartTreeWidget::moveComponentDown, m_document, &Document::moveComponentDown);
    connect(partTreeWidget, &PartTreeWidget::moveComponentToTop, m_document, &Document::moveComponentToTop);
    connect(partTreeWidget, &PartTreeWidget::moveComponentToBottom, m_document, &Document::moveComponentToBottom);
    connect(partTreeWidget, &PartTreeWidget::checkPart, m_document, &Document::checkPart);
    connect(partTreeWidget, &PartTreeWidget::createNewComponentAndMoveThisIn, m_document, &Document::createNewComponentAndMoveThisIn);
    connect(partTreeWidget, &PartTreeWidget::createNewChildComponent, m_document, &Document::createNewChildComponent);
    connect(partTreeWidget, &PartTreeWidget::renameComponent, m_document, &Document::renameComponent);
    connect(partTreeWidget, &PartTreeWidget::setComponentExpandState, m_document, &Document::setComponentExpandState);
    connect(partTreeWidget, &PartTreeWidget::setComponentSmoothAll, m_document, &Document::setComponentSmoothAll);
    connect(partTreeWidget, &PartTreeWidget::setComponentSmoothSeam, m_document, &Document::setComponentSmoothSeam);
    connect(partTreeWidget, &PartTreeWidget::setComponentPolyCount, m_document, &Document::setComponentPolyCount);
    connect(partTreeWidget, &PartTreeWidget::setComponentLayer, m_document, &Document::setComponentLayer);
    connect(partTreeWidget, &PartTreeWidget::moveComponent, m_document, &Document::moveComponent);
    connect(partTreeWidget, &PartTreeWidget::removeComponent, m_document, &Document::removeComponent);
    connect(partTreeWidget, &PartTreeWidget::hideOtherComponents, m_document, &Document::hideOtherComponents);
    connect(partTreeWidget, &PartTreeWidget::lockOtherComponents, m_document, &Document::lockOtherComponents);
    connect(partTreeWidget, &PartTreeWidget::hideAllComponents, m_document, &Document::hideAllComponents);
    connect(partTreeWidget, &PartTreeWidget::showAllComponents, m_document, &Document::showAllComponents);
    connect(partTreeWidget, &PartTreeWidget::collapseAllComponents, m_document, &Document::collapseAllComponents);
    connect(partTreeWidget, &PartTreeWidget::expandAllComponents, m_document, &Document::expandAllComponents);
    connect(partTreeWidget, &PartTreeWidget::lockAllComponents, m_document, &Document::lockAllComponents);
    connect(partTreeWidget, &PartTreeWidget::unlockAllComponents, m_document, &Document::unlockAllComponents);
    connect(partTreeWidget, &PartTreeWidget::setPartLockState, m_document, &Document::setPartLockState);
    connect(partTreeWidget, &PartTreeWidget::setPartVisibleState, m_document, &Document::setPartVisibleState);
    connect(partTreeWidget, &PartTreeWidget::setPartColorState, m_document, &Document::setPartColorState);
    connect(partTreeWidget, &PartTreeWidget::setComponentCombineMode, m_document, &Document::setComponentCombineMode);
    connect(partTreeWidget, &PartTreeWidget::setComponentClothStiffness, m_document, &Document::setComponentClothStiffness);
    connect(partTreeWidget, &PartTreeWidget::setComponentClothIteration, m_document, &Document::setComponentClothIteration);
    connect(partTreeWidget, &PartTreeWidget::setComponentClothForce, m_document, &Document::setComponentClothForce);
    connect(partTreeWidget, &PartTreeWidget::setComponentClothOffset, m_document, &Document::setComponentClothOffset);
    connect(partTreeWidget, &PartTreeWidget::setPartTarget, m_document, &Document::setPartTarget);
    connect(partTreeWidget, &PartTreeWidget::setPartBase, m_document, &Document::setPartBase);
    connect(partTreeWidget, &PartTreeWidget::hideDescendantComponents, m_document, &Document::hideDescendantComponents);
    connect(partTreeWidget, &PartTreeWidget::showDescendantComponents, m_document, &Document::showDescendantComponents);
    connect(partTreeWidget, &PartTreeWidget::lockDescendantComponents, m_document, &Document::lockDescendantComponents);
    connect(partTreeWidget, &PartTreeWidget::unlockDescendantComponents, m_document, &Document::unlockDescendantComponents);
    connect(partTreeWidget, &PartTreeWidget::groupOperationAdded, m_document, &Document::saveSnapshot);
    
    connect(partTreeWidget, &PartTreeWidget::addPartToSelection, graphicsWidget, &SkeletonGraphicsWidget::addPartToSelection);
    
    connect(graphicsWidget, &SkeletonGraphicsWidget::partComponentChecked, partTreeWidget, &PartTreeWidget::partComponentChecked);
    
    connect(m_document, &Document::componentNameChanged, partTreeWidget, &PartTreeWidget::componentNameChanged);
    connect(m_document, &Document::componentChildrenChanged, partTreeWidget, &PartTreeWidget::componentChildrenChanged);
    connect(m_document, &Document::componentRemoved, partTreeWidget, &PartTreeWidget::componentRemoved);
    connect(m_document, &Document::componentAdded, partTreeWidget, &PartTreeWidget::componentAdded);
    connect(m_document, &Document::componentExpandStateChanged, partTreeWidget, &PartTreeWidget::componentExpandStateChanged);
    connect(m_document, &Document::componentCombineModeChanged, partTreeWidget, &PartTreeWidget::componentCombineModeChanged);
    connect(m_document, &Document::partPreviewChanged, partTreeWidget, &PartTreeWidget::partPreviewChanged);
    connect(m_document, &Document::partLockStateChanged, partTreeWidget, &PartTreeWidget::partLockStateChanged);
    connect(m_document, &Document::partVisibleStateChanged, partTreeWidget, &PartTreeWidget::partVisibleStateChanged);
    connect(m_document, &Document::partSubdivStateChanged, partTreeWidget, &PartTreeWidget::partSubdivStateChanged);
    connect(m_document, &Document::partDisableStateChanged, partTreeWidget, &PartTreeWidget::partDisableStateChanged);
    connect(m_document, &Document::partXmirrorStateChanged, partTreeWidget, &PartTreeWidget::partXmirrorStateChanged);
    connect(m_document, &Document::partDeformThicknessChanged, partTreeWidget, &PartTreeWidget::partDeformChanged);
    connect(m_document, &Document::partDeformWidthChanged, partTreeWidget, &PartTreeWidget::partDeformChanged);
    connect(m_document, &Document::partDeformMapImageIdChanged, partTreeWidget, &PartTreeWidget::partDeformChanged);
    connect(m_document, &Document::partDeformMapScaleChanged, partTreeWidget, &PartTreeWidget::partDeformChanged);
    connect(m_document, &Document::partRoundStateChanged, partTreeWidget, &PartTreeWidget::partRoundStateChanged);
    connect(m_document, &Document::partChamferStateChanged, partTreeWidget, &PartTreeWidget::partChamferStateChanged);
    connect(m_document, &Document::partColorStateChanged, partTreeWidget, &PartTreeWidget::partColorStateChanged);
    connect(m_document, &Document::partCutRotationChanged, partTreeWidget, &PartTreeWidget::partCutRotationChanged);
    connect(m_document, &Document::partCutFaceChanged, partTreeWidget, &PartTreeWidget::partCutFaceChanged);
    connect(m_document, &Document::partHollowThicknessChanged, partTreeWidget, &PartTreeWidget::partHollowThicknessChanged);
    connect(m_document, &Document::partMaterialIdChanged, partTreeWidget, &PartTreeWidget::partMaterialIdChanged);
    connect(m_document, &Document::partColorSolubilityChanged, partTreeWidget, &PartTreeWidget::partColorSolubilityChanged);
    connect(m_document, &Document::partCountershadeStateChanged, partTreeWidget, &PartTreeWidget::partCountershadeStateChanged);
    connect(m_document, &Document::partRemoved, partTreeWidget, &PartTreeWidget::partRemoved);
    connect(m_document, &Document::cleanup, partTreeWidget, &PartTreeWidget::removeAllContent);
    connect(m_document, &Document::partChecked, partTreeWidget, &PartTreeWidget::partChecked);
    connect(m_document, &Document::partUnchecked, partTreeWidget, &PartTreeWidget::partUnchecked);

    connect(m_document, &Document::skeletonChanged, m_document, &Document::generateMesh);
    //connect(m_document, &SkeletonDocument::resultMeshChanged, [=]() {
    //    if ((m_exportPreviewWidget && m_exportPreviewWidget->isVisible())) {
    //        m_document->postProcess();
    //    }
    //});
    //connect(m_document, &SkeletonDocument::textureChanged, [=]() {
    //    if ((m_exportPreviewWidget && m_exportPreviewWidget->isVisible())) {
    //        m_document->generateTexture();
    //    }
    //});
    connect(m_document, &Document::textureChanged, m_document, &Document::generateTexture);
    connect(m_document, &Document::resultMeshChanged, m_document, &Document::postProcess);
    connect(m_document, &Document::postProcessedResultChanged, m_document, &Document::generateRig);
    connect(m_document, &Document::rigChanged, m_document, &Document::generateRig);
    connect(m_document, &Document::postProcessedResultChanged, m_document, &Document::generateTexture);
    //connect(m_document, &SkeletonDocument::resultTextureChanged, m_document, &SkeletonDocument::bakeAmbientOcclusionTexture);
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
    
    connect(m_document, &Document::resultMeshChanged, [=]() {
        auto resultMesh = m_document->takeResultMesh();
        if (nullptr != resultMesh)
            m_currentUpdatedMeshId = resultMesh->meshId();
        if (m_modelRemoveColor && resultMesh)
            resultMesh->removeColor();
        m_modelRenderWidget->updateMesh(resultMesh);
    });
    
    connect(m_document, &Document::posesChanged, m_document, &Document::generateMotions);
    connect(m_document, &Document::motionsChanged, m_document, &Document::generateMotions);

    connect(graphicsWidget, &SkeletonGraphicsWidget::cursorChanged, [=]() {
        m_modelRenderWidget->setCursor(graphicsWidget->cursor());
        containerWidget->setCursor(graphicsWidget->cursor());
        //m_skeletonRenderWidget->setCursor(graphicsWidget->cursor());
    });

    connect(m_document, &Document::skeletonChanged, this, &DocumentWindow::documentChanged);
    connect(m_document, &Document::turnaroundChanged, this, &DocumentWindow::documentChanged);
    connect(m_document, &Document::optionsChanged, this, &DocumentWindow::documentChanged);
    connect(m_document, &Document::rigChanged, this, &DocumentWindow::documentChanged);
    connect(m_document, &Document::scriptChanged, this, &DocumentWindow::documentChanged);

    connect(m_modelRenderWidget, &ModelWidget::customContextMenuRequested, [=](const QPoint &pos) {
        graphicsWidget->showContextMenu(graphicsWidget->mapFromGlobal(m_modelRenderWidget->mapToGlobal(pos)));
    });

    connect(m_document, &Document::xlockStateChanged, this, &DocumentWindow::updateXlockButtonState);
    connect(m_document, &Document::ylockStateChanged, this, &DocumentWindow::updateYlockButtonState);
    connect(m_document, &Document::zlockStateChanged, this, &DocumentWindow::updateZlockButtonState);
    connect(m_document, &Document::radiusLockStateChanged, this, &DocumentWindow::updateRadiusLockButtonState);
    
    connect(m_rigWidget, &RigWidget::setRigType, m_document, &Document::setRigType);
    
    connect(m_document, &Document::rigTypeChanged, m_rigWidget, &RigWidget::rigTypeChanged);
    connect(m_document, &Document::resultRigChanged, m_rigWidget, &RigWidget::updateResultInfo);
    connect(m_document, &Document::resultRigChanged, this, &DocumentWindow::updateRigWeightRenderWidget);
    
    //connect(m_document, &SkeletonDocument::resultRigChanged, tetrapodPoseEditWidget, &TetrapodPoseEditWidget::updatePreview);
    
    connect(m_document, &Document::poseAdded, this, [=](QUuid poseId) {
        Q_UNUSED(poseId);
        m_document->generatePosePreviews();
    });
    connect(m_document, &Document::poseFramesChanged, this, [=](QUuid poseId) {
        Q_UNUSED(poseId);
        m_document->generatePosePreviews();
    });
    connect(m_document, &Document::resultRigChanged, m_document, &Document::generatePosePreviews);
    
    connect(m_document, &Document::resultRigChanged, m_document, &Document::generateMotions);
    
    connect(m_document, &Document::materialAdded, this, [=](QUuid materialId) {
        Q_UNUSED(materialId);
        m_document->generateMaterialPreviews();
    });
    connect(m_document, &Document::materialLayersChanged, this, [=](QUuid materialId) {
        Q_UNUSED(materialId);
        m_document->generateMaterialPreviews();
    });
    
    connect(m_document, &Document::scriptChanged, m_document, &Document::runScript);
    connect(m_document, &Document::scriptModifiedFromExternal, m_document, &Document::runScript);
    
    initShortCuts(this, m_graphicsWidget);

    connect(this, &DocumentWindow::initialized, m_document, &Document::uiReady);
    connect(this, &DocumentWindow::initialized, this, &DocumentWindow::autoRecover);
    
    m_autoSaver = new AutoSaver(m_document);
    
    QTimer *timer = new QTimer(this);
    timer->setInterval(250);
    connect(timer, &QTimer::timeout, [=] {
        QWidget *focusedWidget = QApplication::focusWidget();
        //qDebug() << (focusedWidget ? ("Focused on:" + QString(focusedWidget->metaObject()->className()) + " title:" + focusedWidget->windowTitle()) : "No Focus") << " isActive:" << isActiveWindow();
        if (nullptr == focusedWidget && isActiveWindow())
            graphicsWidget->setFocus();
    });
    timer->start();
}

void DocumentWindow::toggleRotation()
{
    if (nullptr == m_graphicsWidget)
        return;
    m_graphicsWidget->setRotated(!m_graphicsWidget->rotated());
    updateRotationButtonState();
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
    
    m_autoSaver->stop();

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
    
    m_autoSaver->documentChanged();
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
    m_document->resetScript();
    m_document->reset();
    m_document->saveSnapshot();
}

void DocumentWindow::saveAs()
{
    QString filename = QFileDialog::getSaveFileName(this, QString(), QString(),
       tr("Dust3D Document (*.ds3)"));
    if (filename.isEmpty()) {
        return;
    }
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

void DocumentWindow::initLockButton(QPushButton *button)
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
        m_graphicsWidget->setFocus();
        emit initialized();
    }
}

void DocumentWindow::autoRecover()
{
    if (m_autoRecovered)
        return;
    
    m_autoRecovered = true;
    
    QString dir = AutoSaver::autoSavedDir();
    if (dir.isEmpty())
        return;
    
    QDir recoverFromDir(dir, "*.d3b");
    recoverFromDir.setSorting(QDir::Name);
    auto autoSavedFiles = recoverFromDir.entryList();
    if (autoSavedFiles.isEmpty())
        return;
    
    auto filename = dir + QDir::separator() + autoSavedFiles.last();
    openPathAs(filename, "");
    m_documentSaved = false;
    updateTitle();
    QFile::remove(filename);
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
    }
    
    QApplication::setOverrideCursor(Qt::WaitCursor);
    Snapshot snapshot;
    m_document->toSnapshot(&snapshot);
    if (DocumentSaver::save(&filename, 
            &snapshot, 
            (!m_document->turnaround.isNull() && m_document->turnaroundPngByteArray.size() > 0) ? 
                &m_document->turnaroundPngByteArray : nullptr,
            (!m_document->script().isEmpty()) ? &m_document->script() : nullptr,
            (!m_document->variables().empty()) ? &m_document->variables() : nullptr)) {
        setCurrentFilename(filename);
    }
    QApplication::restoreOverrideCursor();
}

void DocumentWindow::importPath(const QString &path)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    Ds3FileReader ds3Reader(path);
    bool documentChanged = false;
    
    for (int i = 0; i < ds3Reader.items().size(); ++i) {
        Ds3ReaderItem item = ds3Reader.items().at(i);
        if (item.type == "asset") {
            if (item.name.startsWith("images/")) {
                QString filename = item.name.split("/")[1];
                QString imageIdString = filename.split(".")[0];
                QUuid imageId = QUuid(imageIdString);
                if (!imageId.isNull()) {
                    QByteArray data;
                    ds3Reader.loadItem(item.name, &data);
                    QImage image = QImage::fromData(data, "PNG");
                    (void)ImageForever::add(&image, imageId);
                }
            }
        }
    }
    
    for (int i = 0; i < ds3Reader.items().size(); ++i) {
        Ds3ReaderItem item = ds3Reader.items().at(i);
        if (item.type == "model") {
            {
                QByteArray data;
                ds3Reader.loadItem(item.name, &data);
                QXmlStreamReader stream(data);
            
                Snapshot snapshot;
                loadSkeletonFromXmlStream(&snapshot, stream, SNAPSHOT_ITEM_MATERIAL);
                m_document->addFromSnapshot(snapshot, Document::SnapshotSource::Import);
                documentChanged = true;
            }
            {
                QByteArray data;
                ds3Reader.loadItem(item.name, &data);
                QXmlStreamReader stream(data);
                
                Snapshot snapshot;
                loadSkeletonFromXmlStream(&snapshot, stream, SNAPSHOT_ITEM_CANVAS | SNAPSHOT_ITEM_COMPONENT);

                QByteArray modelXml;
                QXmlStreamWriter modelStream(&modelXml);
                saveSkeletonToXmlStream(&snapshot, &modelStream);
                if (modelXml.size() > 0) {
                    QUuid fillMeshFileId = FileForever::add(item.name, modelXml);
                    if (!fillMeshFileId.isNull()) {
                        Snapshot partSnapshot;
                        createPartSnapshotForFillMesh(fillMeshFileId, &partSnapshot);
                        m_document->addFromSnapshot(partSnapshot, Document::SnapshotSource::Paste);
                        documentChanged = true;
                    }
                }
            }
        }
    }
    
    if (documentChanged)
        m_document->saveSnapshot();
    
    QApplication::restoreOverrideCursor();
}

void DocumentWindow::createPartSnapshotForFillMesh(const QUuid &fillMeshFileId, Snapshot *snapshot)
{
    if (fillMeshFileId.isNull())
        return;
    
    auto partId = QUuid::createUuid();
    auto partIdString = partId.toString();
    std::map<QString, QString> snapshotPart;
    snapshotPart["id"] = partIdString;
    snapshotPart["fillMesh"] = fillMeshFileId.toString();
    snapshot->parts[partIdString] = snapshotPart;
    
    auto componentId = QUuid::createUuid();
    auto componentIdString = componentId.toString();
    std::map<QString, QString> snapshotComponent;
    snapshotComponent["id"] = componentIdString;
    snapshotComponent["combineMode"] = "Uncombined";
    snapshotComponent["linkDataType"] = "partId";
    snapshotComponent["linkData"] = partIdString;
    snapshot->components[componentIdString] = snapshotComponent;
    
    snapshot->rootComponent["children"] = componentIdString;
    
    auto createNode = [&](const QVector3D &position, float radius) {
        auto nodeId = QUuid::createUuid();
        auto nodeIdString = nodeId.toString();
        std::map<QString, QString> snapshotNode;
        snapshotNode["id"] = nodeIdString;
        snapshotNode["x"] = QString::number(position.x());
        snapshotNode["y"] = QString::number(position.y());
        snapshotNode["z"] = QString::number(position.z());
        snapshotNode["radius"] = QString::number(radius);
        snapshotNode["partId"] = partIdString;
        snapshot->nodes[nodeIdString] = snapshotNode;
        return nodeIdString;
    };
    
    auto createEdge = [&](const QString &fromNode, const QString &toNode) {
        auto edgeId = QUuid::createUuid();
        auto edgeIdString = edgeId.toString();
        std::map<QString, QString> snapshotEdge;
        snapshotEdge["id"] = edgeIdString;
        snapshotEdge["from"] = fromNode;
        snapshotEdge["to"] = toNode;
        snapshotEdge["partId"] = partIdString;
        snapshot->edges[edgeIdString] = snapshotEdge;
    };
    
    createEdge(createNode(QVector3D(0.5, 0.5, 1.0), 0.1), 
        createNode(QVector3D(0.5, 0.3, 1.0), 0.1));
}

void DocumentWindow::openPathAs(const QString &path, const QString &asName)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    Ds3FileReader ds3Reader(path);
    
    m_document->clearHistories();
    m_document->resetScript();
    m_document->reset();
    m_document->saveSnapshot();
    
    for (int i = 0; i < ds3Reader.items().size(); ++i) {
        Ds3ReaderItem item = ds3Reader.items().at(i);
        if (item.type == "asset") {
            if (item.name.startsWith("images/")) {
                QString filename = item.name.split("/")[1];
                QString imageIdString = filename.split(".")[0];
                QUuid imageId = QUuid(imageIdString);
                if (!imageId.isNull()) {
                    QByteArray data;
                    ds3Reader.loadItem(item.name, &data);
                    QImage image = QImage::fromData(data, "PNG");
                    (void)ImageForever::add(&image, imageId);
                }
            } else if (item.name.startsWith("files/")) {
                QString filename = item.name.split("/")[1];
                QString fileIdString = filename.split(".")[0];
                QUuid fileId = QUuid(fileIdString);
                if (!fileId.isNull()) {
                    QByteArray data;
                    ds3Reader.loadItem(item.name, &data);
                    (void)FileForever::add(item.name, data, fileId);
                }
            }
        }
    }
    
    for (int i = 0; i < ds3Reader.items().size(); ++i) {
        Ds3ReaderItem item = ds3Reader.items().at(i);
        if (item.type == "model") {
            QByteArray data;
            ds3Reader.loadItem(item.name, &data);
            QXmlStreamReader stream(data);
            Snapshot snapshot;
            loadSkeletonFromXmlStream(&snapshot, stream);
            m_document->fromSnapshot(snapshot);
            m_document->saveSnapshot();
        } else if (item.type == "asset") {
            if (item.name == "canvas.png") {
                QByteArray data;
                ds3Reader.loadItem(item.name, &data);
                QImage image = QImage::fromData(data, "PNG");
                m_document->updateTurnaround(image);
            }
        } else if (item.type == "script") {
            if (item.name == "model.js") {
                QByteArray script;
                ds3Reader.loadItem(item.name, &script);
                m_document->initScript(QString::fromUtf8(script.constData()));
            }
        } else if (item.type == "variable") {
            if (item.name == "variables.xml") {
                QByteArray data;
                ds3Reader.loadItem(item.name, &data);
                QXmlStreamReader stream(data);
                std::map<QString, std::map<QString, QString>> variables;
                loadVariablesFromXmlStream(&variables, stream);
                for (const auto &it: variables)
                    m_document->updateVariable(it.first, it.second);
            }
        }
    }
    QApplication::restoreOverrideCursor();

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

void DocumentWindow::showPreferences()
{
    if (nullptr == m_preferencesWidget) {
        m_preferencesWidget = new PreferencesWidget(m_document, this);
        connect(m_preferencesWidget, &PreferencesWidget::enableBackgroundBlur, m_document, &Document::enableBackgroundBlur);
        connect(m_preferencesWidget, &PreferencesWidget::disableBackgroundBlur, m_document, &Document::disableBackgroundBlur);
    }
    m_preferencesWidget->show();
    m_preferencesWidget->raise();
}

void DocumentWindow::exportRenderedResult()
{
    QString filename = QFileDialog::getSaveFileName(this, QString(), QString(),
       tr("Image (*.png)"));
    if (filename.isEmpty()) {
        return;
    }
    exportImageToFilename(filename);
}

void DocumentWindow::exportObjResult()
{
    QString filename = QFileDialog::getSaveFileName(this, QString(), QString(),
       tr("Wavefront (*.obj)"));
    if (filename.isEmpty()) {
        return;
    }
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

void DocumentWindow::showExportPreview()
{
    if (nullptr == m_exportPreviewWidget) {
        m_exportPreviewWidget = new ExportPreviewWidget(m_document, this);
        connect(m_exportPreviewWidget, &ExportPreviewWidget::regenerate, m_document, &Document::regenerateMesh);
        connect(m_exportPreviewWidget, &ExportPreviewWidget::saveAsGlb, this, &DocumentWindow::exportGlbResult);
        connect(m_exportPreviewWidget, &ExportPreviewWidget::saveAsFbx, this, &DocumentWindow::exportFbxResult);
        connect(m_document, &Document::resultMeshChanged, m_exportPreviewWidget, &ExportPreviewWidget::checkSpinner);
        connect(m_document, &Document::exportReady, m_exportPreviewWidget, &ExportPreviewWidget::checkSpinner);
        connect(m_document, &Document::resultTextureChanged, m_exportPreviewWidget, &ExportPreviewWidget::updateTexturePreview);
        //connect(m_document, &Document::resultBakedTextureChanged, m_exportPreviewWidget, &ExportPreviewWidget::updateTexturePreview);
        registerDialog(m_exportPreviewWidget);
    }
    m_exportPreviewWidget->show();
    m_exportPreviewWidget->raise();
}

void DocumentWindow::exportFbxResult()
{
    QString filename = QFileDialog::getSaveFileName(this, QString(), QString(),
       tr("Autodesk FBX (.fbx)"));
    if (filename.isEmpty()) {
        return;
    }
    exportFbxToFilename(filename);
}

void DocumentWindow::exportFbxToFilename(const QString &filename)
{
    if (!m_document->isExportReady()) {
        qDebug() << "Export but document is not export ready";
        return;
    }
    QApplication::setOverrideCursor(Qt::WaitCursor);
    Outcome skeletonResult = m_document->currentPostProcessedOutcome();
    std::vector<std::pair<QString, std::vector<std::pair<float, JointNodeTree>>>> exportMotions;
    for (const auto &motionId: m_document->motionIdList) {
        const Motion *motion = m_document->findMotion(motionId);
        if (nullptr == motion)
            continue;
        exportMotions.push_back({motion->name, motion->jointNodeTrees});
    }
    FbxFileWriter fbxFileWriter(skeletonResult, m_document->resultRigBones(), m_document->resultRigWeights(), filename,
        m_document->textureImage,
        m_document->textureNormalImage,
        m_document->textureMetalnessImage,
        m_document->textureRoughnessImage,
        m_document->textureAmbientOcclusionImage,
        exportMotions.empty() ? nullptr : &exportMotions);
    fbxFileWriter.save();
    QApplication::restoreOverrideCursor();
}

void DocumentWindow::exportGlbResult()
{
    QString filename = QFileDialog::getSaveFileName(this, QString(), QString(),
       tr("glTF Binary Format (.glb)"));
    if (filename.isEmpty()) {
        return;
    }
    exportGlbToFilename(filename);
}

void DocumentWindow::exportGlbToFilename(const QString &filename)
{
    if (!m_document->isExportReady()) {
        qDebug() << "Export but document is not export ready";
        return;
    }
    QApplication::setOverrideCursor(Qt::WaitCursor);
    Outcome skeletonResult = m_document->currentPostProcessedOutcome();
    std::vector<std::pair<QString, std::vector<std::pair<float, JointNodeTree>>>> exportMotions;
    for (const auto &motionId: m_document->motionIdList) {
        const Motion *motion = m_document->findMotion(motionId);
        if (nullptr == motion)
            continue;
        exportMotions.push_back({motion->name, motion->jointNodeTrees});
    }
    GlbFileWriter glbFileWriter(skeletonResult, m_document->resultRigBones(), m_document->resultRigWeights(), filename,
        m_document->textureHasTransparencySettings,
        m_document->textureImage, m_document->textureNormalImage, m_document->textureMetalnessRoughnessAmbientOcclusionImage, exportMotions.empty() ? nullptr : &exportMotions);
    glbFileWriter.save();
    QApplication::restoreOverrideCursor();
}

void DocumentWindow::updateXlockButtonState()
{
    if (m_document->xlocked)
        m_xlockButton->setStyleSheet("QPushButton {color: #252525}");
    else
        m_xlockButton->setStyleSheet("QPushButton {color: " + Theme::red.name() + "}");
}

void DocumentWindow::updateRotationButtonState()
{
    if (nullptr == m_graphicsWidget)
        return;
    if (m_graphicsWidget->rotated()) {
        m_rotationButton->setText(QChar(fa::caretsquareoleft));
        m_rotationButton->setStyleSheet("QPushButton {color: " + Theme::blue.name() + "}");
    } else {
        m_rotationButton->setText(QChar(fa::caretsquareoup));
        m_rotationButton->setStyleSheet("QPushButton {color: " + Theme::white.name() + "}");
    }
}

void DocumentWindow::updateYlockButtonState()
{
    if (m_document->ylocked)
        m_ylockButton->setStyleSheet("QPushButton {color: #252525}");
    else
        m_ylockButton->setStyleSheet("QPushButton {color: " + Theme::blue.name() + "}");
}

void DocumentWindow::updateZlockButtonState()
{
    if (m_document->zlocked)
        m_zlockButton->setStyleSheet("QPushButton {color: #252525}");
    else
        m_zlockButton->setStyleSheet("QPushButton {color: " + Theme::green.name() + "}");
}

void DocumentWindow::updateRadiusLockButtonState()
{
    if (m_document->radiusLocked)
        m_radiusLockButton->setStyleSheet("QPushButton {color: #252525}");
    else
        m_radiusLockButton->setStyleSheet("QPushButton {color: " + Theme::white.name() + "}");
}

void DocumentWindow::updateRigWeightRenderWidget()
{
    Model *resultRigWeightMesh = m_document->takeResultRigWeightMesh();
    if (nullptr == resultRigWeightMesh) {
        m_rigWidget->rigWeightRenderWidget()->hide();
    } else {
        m_rigWidget->rigWeightRenderWidget()->updateMesh(resultRigWeightMesh);
        m_rigWidget->rigWeightRenderWidget()->show();
        m_rigWidget->rigWeightRenderWidget()->update();
    }
}

void DocumentWindow::registerDialog(QWidget *widget)
{
    m_dialogs.push_back(widget);
}

void DocumentWindow::unregisterDialog(QWidget *widget)
{
    m_dialogs.erase(std::remove(m_dialogs.begin(), m_dialogs.end(), widget), m_dialogs.end());
}

void DocumentWindow::showCutFaceSettingPopup(const QPoint &globalPos, std::set<QUuid> nodeIds)
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
    
    QHBoxLayout *standardFacesLayout = new QHBoxLayout;
    QPushButton *buttons[(int)CutFace::Count] = {0};
    
    CutFaceListWidget *cutFaceListWidget = new CutFaceListWidget(m_document);
    size_t cutFaceTypeCount = (size_t)CutFace::UserDefined;
    
    auto updateCutFaceButtonState = [&](size_t index) {
        if (index != (int)CutFace::UserDefined)
            cutFaceListWidget->selectCutFace(QUuid());
        for (size_t i = 0; i < (size_t)cutFaceTypeCount; ++i) {
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
    
    cutFaceListWidget->enableMultipleSelection(false);
    if (nullptr != node) {
        cutFaceListWidget->selectCutFace(node->cutFaceLinkedId);
    }
    connect(cutFaceListWidget, &CutFaceListWidget::currentSelectedCutFaceChanged, this, [=](QUuid partId) {
        if (partId.isNull()) {
            CutFace cutFace = CutFace::Quad;
            updateCutFaceButtonState((int)cutFace);
            m_document->batchChangeBegin();
            for (const auto &id: nodeIds) {
                m_document->setNodeCutFace(id, cutFace);
            }
            m_document->batchChangeEnd();
            m_document->saveSnapshot();
        } else {
            updateCutFaceButtonState((int)CutFace::UserDefined);
            m_document->batchChangeBegin();
            for (const auto &id: nodeIds) {
                m_document->setNodeCutFaceLinkedId(id, partId);
            }
            m_document->batchChangeEnd();
            m_document->saveSnapshot();
        }
    });
    if (cutFaceListWidget->isEmpty())
        cutFaceListWidget->hide();
    
    for (size_t i = 0; i < (size_t)cutFaceTypeCount; ++i) {
        CutFace cutFace = (CutFace)i;
        QString iconFilename = ":/resources/" + CutFaceToString(cutFace).toLower() + ".png";
        QPixmap pixmap(iconFilename);
        QIcon buttonIcon(pixmap);
        QPushButton *button = new QPushButton;
        button->setIconSize(QSize(Theme::toolIconSize / 2, Theme::toolIconSize / 2));
        button->setIcon(buttonIcon);
        connect(button, &QPushButton::clicked, [=]() {
            updateCutFaceButtonState(i);
            m_document->batchChangeBegin();
            for (const auto &id: nodeIds) {
                m_document->setNodeCutFace(id, cutFace);
            }
            m_document->batchChangeEnd();
            m_document->saveSnapshot();
        });
        standardFacesLayout->addWidget(button);
        buttons[i] = button;
    }
    standardFacesLayout->addStretch();
    if (nullptr != node) {
        updateCutFaceButtonState((size_t)node->cutFace);
    }
    
    QVBoxLayout *popupLayout = new QVBoxLayout;
    popupLayout->addLayout(rotationLayout);
    popupLayout->addWidget(Theme::createHorizontalLineWidget());
    popupLayout->addLayout(standardFacesLayout);
    popupLayout->addWidget(cutFaceListWidget);
    
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

//void DocumentWindow::updateInfoWidgetPosition()
//{
//    m_infoWidget->move(0, m_graphicsContainerWidget->height() - m_infoWidget->height() - 5);
//}

void DocumentWindow::normalAndDepthMapsReady()
{
    QImage *normalMap = m_normalAndDepthMapsGenerator->takeNormalMap();
    QImage *depthMap = m_normalAndDepthMapsGenerator->takeDepthMap();

    m_modelRenderWidget->updateToonNormalAndDepthMaps(normalMap, depthMap);

    delete m_normalAndDepthMapsGenerator;
    m_normalAndDepthMapsGenerator = nullptr;
    
    qDebug() << "Normal and depth maps generation done";
    
    if (m_isNormalAndDepthMapsObsolete) {
        generateNormalAndDepthMaps();
    }
}

void DocumentWindow::generateNormalAndDepthMaps()
{
    if (nullptr != m_normalAndDepthMapsGenerator) {
        m_isNormalAndDepthMapsObsolete = true;
        return;
    }
    
    m_isNormalAndDepthMapsObsolete = false;
    
    auto resultMesh = m_document->takeResultMesh();
    if (nullptr == resultMesh)
		return;
    
    qDebug() << "Normal and depth maps generating...";
    
    QThread *thread = new QThread;
    m_normalAndDepthMapsGenerator = new NormalAndDepthMapsGenerator(m_modelRenderWidget);
    m_normalAndDepthMapsGenerator->updateMesh(resultMesh);
    m_normalAndDepthMapsGenerator->moveToThread(thread);
    m_normalAndDepthMapsGenerator->setRenderThread(thread);
    connect(thread, &QThread::started, m_normalAndDepthMapsGenerator, &NormalAndDepthMapsGenerator::process);
    connect(m_normalAndDepthMapsGenerator, &NormalAndDepthMapsGenerator::finished, this, &DocumentWindow::normalAndDepthMapsReady);
    connect(m_normalAndDepthMapsGenerator, &NormalAndDepthMapsGenerator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void DocumentWindow::delayedGenerateNormalAndDepthMaps()
{
    if (!Preferences::instance().toonShading())
        return;
    
    generateNormalAndDepthMaps();
}

void DocumentWindow::exportImageToFilename(const QString &filename)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    Model *resultMesh = m_modelRenderWidget->fetchCurrentMesh();
    if (nullptr != resultMesh) {
        ModelOffscreenRender *offlineRender = new ModelOffscreenRender(m_modelRenderWidget->format());
        offlineRender->setXRotation(m_modelRenderWidget->xRot());
        offlineRender->setYRotation(m_modelRenderWidget->yRot());
        offlineRender->setZRotation(m_modelRenderWidget->zRot());
        offlineRender->setRenderPurpose(0);
        QImage *normalMap = new QImage();
        QImage *depthMap = new QImage();
        m_modelRenderWidget->fetchCurrentToonNormalAndDepthMaps(normalMap, depthMap);
        if (!normalMap->isNull() && !depthMap->isNull()) {
            offlineRender->updateToonNormalAndDepthMaps(normalMap, depthMap);
        } else {
            delete normalMap;
            delete depthMap;
        }
        offlineRender->updateMesh(resultMesh);
        if (Preferences::instance().toonShading())
            offlineRender->setToonShading(true);
        offlineRender->toImage(QSize(m_modelRenderWidget->widthInPixels(),
            m_modelRenderWidget->heightInPixels())).save(filename);
    }
    QApplication::restoreOverrideCursor();
}

void DocumentWindow::import()
{
    QString fileName = QFileDialog::getOpenFileName(this, QString(), QString(),
        tr("Dust3D Document (*.ds3)")).trimmed();
    if (fileName.isEmpty())
        return;
    importPath(fileName);
}
