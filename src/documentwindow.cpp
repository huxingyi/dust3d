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
#include <QFormLayout>
#include <QStackedWidget>
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
#include "scriptwidget.h"
#include "variablesxml.h"
#include "updatescheckwidget.h"
#include "modeloffscreenrender.h"
#include "fileforever.h"
#include "documentsaver.h"
#include "objectxml.h"
#include "rigxml.h"
#include "statusbarlabel.h"
#include "silhouetteimagegenerator.h"
#include "flowlayout.h"
#include "bonedocument.h"
#include "document.h"

int DocumentWindow::m_autoRecovered = false;

LogBrowser *g_logBrowser = nullptr;
std::map<DocumentWindow *, QUuid> g_documentWindows;
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

const std::map<DocumentWindow *, QUuid> &DocumentWindow::documentWindows()
{
    return g_documentWindows;
}

DocumentWindow::GraphicsViewEditTarget DocumentWindow::graphicsViewEditTarget()
{
    return m_graphicsViewEditTarget;
}

void DocumentWindow::updateGraphicsViewEditTarget(GraphicsViewEditTarget target)
{
    if (m_graphicsViewEditTarget == target)
        return;
    
    m_graphicsViewEditTarget = target;
    if (GraphicsViewEditTarget::Bone == m_graphicsViewEditTarget)
        generateSilhouetteImage();
    emit graphicsViewEditTargetChanged();
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

void DocumentWindow::showSupporters()
{
    if (!g_supportersWidget) {
        g_supportersWidget = new QTextBrowser;
        g_supportersWidget->setWindowTitle(unifiedWindowTitle(tr("Supporters")));
        g_supportersWidget->setMinimumSize(QSize(400, 300));
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
    
    SkeletonGraphicsWidget *shapeGraphicsWidget = new SkeletonGraphicsWidget(m_document);
    m_shapeGraphicsWidget = shapeGraphicsWidget;
    
    m_boneDocument = new BoneDocument;
    
    SkeletonGraphicsWidget *boneGraphicsWidget = new SkeletonGraphicsWidget(m_boneDocument);
    m_boneGraphicsWidget = boneGraphicsWidget;

    QVBoxLayout *toolButtonLayout = new QVBoxLayout;
    toolButtonLayout->setSpacing(0);
    toolButtonLayout->setContentsMargins(5, 10, 4, 0);

    QPushButton *addButton = new QPushButton(QChar(fa::plus));
    addButton->setToolTip(tr("Add node to canvas"));
    Theme::initAwesomeButton(addButton);

    QPushButton *selectButton = new QPushButton(QChar(fa::mousepointer));
    selectButton->setToolTip(tr("Select node on canvas"));
    Theme::initAwesomeButton(selectButton);
    
    QPushButton *paintButton = new QPushButton(QChar(fa::paintbrush));
    paintButton->setToolTip(tr("Paint brush"));
    paintButton->setVisible(m_document->objectLocked);
    Theme::initAwesomeButton(paintButton);
    connect(m_document, &Document::objectLockStateChanged, this, [=]() {
        paintButton->setVisible(m_document->objectLocked);
    });

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
    initializeLockButton(m_xlockButton);
    updateXlockButtonState();

    m_ylockButton = new QPushButton(QChar('Y'));
    m_ylockButton->setToolTip(tr("Y axis locker"));
    initializeLockButton(m_ylockButton);
    updateYlockButtonState();

    m_zlockButton = new QPushButton(QChar('Z'));
    m_zlockButton->setToolTip(tr("Z axis locker"));
    initializeLockButton(m_zlockButton);
    updateZlockButtonState();
    
    m_radiusLockButton = new QPushButton(QChar(fa::bullseye));
    m_radiusLockButton->setToolTip(tr("Node radius locker"));
    Theme::initAwesomeButton(m_radiusLockButton);
    updateRadiusLockButtonState();
    
    m_regenerateButton = new SpinnableAwesomeButton();
    updateRegenerateIcon();
    connect(m_regenerateButton->button(), &QPushButton::clicked, m_document, &Document::regenerateMesh);
    
    connect(m_document, &Document::meshGenerating, this, &DocumentWindow::updateRegenerateIcon);
    connect(m_document, &Document::resultMeshChanged, this, [=]() {
        m_isLastMeshGenerationSucceed = m_document->isMeshGenerationSucceed();
        updateRegenerateIcon();
    });
    connect(m_document, &Document::resultPartPreviewsChanged, this, [=]() {
        generatePartPreviewImages();
    });
    connect(m_document, &Document::paintedMeshChanged, [=]() {
        auto paintedMesh = m_document->takePaintedMesh();
        m_modelRenderWidget->updateMesh(paintedMesh);
    });
    connect(m_document, &Document::postProcessing, this, &DocumentWindow::updateRegenerateIcon);
    connect(m_document, &Document::textureGenerating, this, &DocumentWindow::updateRegenerateIcon);
    connect(m_document, &Document::resultTextureChanged, this, &DocumentWindow::updateRegenerateIcon);
    connect(m_document, &Document::postProcessedResultChanged, this, &DocumentWindow::updateRegenerateIcon);
    connect(m_document, &Document::objectLockStateChanged, this, &DocumentWindow::updateRegenerateIcon);

    toolButtonLayout->addWidget(addButton);
    toolButtonLayout->addWidget(selectButton);
    toolButtonLayout->addWidget(paintButton);
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
    toolButtonLayout->addWidget(m_regenerateButton);
    

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
    
    QWidget *shapeWidget = new QWidget;
    
    QGridLayout *shapeGraphicsLayout = new QGridLayout;
    shapeGraphicsLayout->setSpacing(0);
    shapeGraphicsLayout->setContentsMargins(0, 0, 0, 0);
    shapeGraphicsLayout->addWidget(shapeGraphicsWidget);
    shapeWidget->setLayout(shapeGraphicsLayout);
    
    QWidget *boneWidget = new QWidget;
    
    QGridLayout *boneGraphicsLayout = new QGridLayout;
    boneGraphicsLayout->setSpacing(0);
    boneGraphicsLayout->setContentsMargins(0, 0, 0, 0);
    boneGraphicsLayout->addWidget(boneGraphicsWidget);
    boneWidget->setLayout(boneGraphicsLayout);
    
    QStackedWidget *canvasStackedWidget = new QStackedWidget;
    canvasStackedWidget->addWidget(boneWidget);
    canvasStackedWidget->addWidget(shapeWidget);
    canvasStackedWidget->setCurrentIndex(1);

    GraphicsContainerWidget *containerWidget = new GraphicsContainerWidget;
    containerWidget->setGraphicsWidget(shapeGraphicsWidget);
    QGridLayout *containerLayout = new QGridLayout;
    containerLayout->setSpacing(0);
    containerLayout->setContentsMargins(1, 0, 0, 0);
    containerLayout->addWidget(canvasStackedWidget);
    containerWidget->setLayout(containerLayout);
    containerWidget->setMinimumSize(400, 400);
    
    containerWidget->setAutoFillBackground(true);
    containerWidget->setPalette(Theme::statusBarActivePalette);
    
    auto setGraphicsIndex = [=](int index) {
        canvasStackedWidget->setCurrentIndex(index);
        if (0 == index) {
            containerWidget->setGraphicsWidget(boneGraphicsWidget);
            boneGraphicsWidget->canvasResized();
        } else if (1 == index) {
            containerWidget->setGraphicsWidget(shapeGraphicsWidget);
            shapeGraphicsWidget->canvasResized();
        }
    };
    
    m_graphicsContainerWidget = containerWidget;

    m_modelRenderWidget = new ModelWidget(containerWidget);
    m_modelRenderWidget->setMoveAndZoomByWindow(false);
    m_modelRenderWidget->move(0, 0);
    m_modelRenderWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_modelRenderWidget->setMousePickRadius(m_document->mousePickRadius());
    if (!Preferences::instance().toonShading())
		m_modelRenderWidget->toggleWireframe();
    m_modelRenderWidget->enableEnvironmentLight();
    m_modelRenderWidget->disableCullFace();
    m_modelRenderWidget->setEyePosition(QVector3D(0.0, 0.0, -4.0));
    m_modelRenderWidget->setMoveToPosition(QVector3D(-0.5, -0.5, 0.0));
    
    connect(containerWidget, &GraphicsContainerWidget::containerSizeChanged,
        m_modelRenderWidget, &ModelWidget::canvasResized);
    
    connect(m_modelRenderWidget, &ModelWidget::mouseRayChanged, m_document,
            [=](const QVector3D &nearPosition, const QVector3D &farPosition) {
        std::set<QUuid> nodeIdSet;
        shapeGraphicsWidget->readSkeletonNodeAndEdgeIdSetFromRangeSelection(&nodeIdSet);
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
    
    m_shapeGraphicsWidget->setModelWidget(m_modelRenderWidget);
    m_boneGraphicsWidget->setModelWidget(m_modelRenderWidget);
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
    
    QDockWidget *rigDocker = new QDockWidget(tr("Rig"), this);
    rigDocker->setAllowedAreas(Qt::RightDockWidgetArea);
    m_rigWidget = new RigWidget(m_document, rigDocker);
    rigDocker->setWidget(m_rigWidget);
    addDockWidget(Qt::RightDockWidgetArea, rigDocker);
    connect(rigDocker, &QDockWidget::topLevelChanged, [=](bool topLevel) {
        Q_UNUSED(topLevel);
        updateRigWeightRenderWidget();
    });
    
    QDockWidget *motionDocker = new QDockWidget(tr("Motions"), this);
    motionDocker->setAllowedAreas(Qt::RightDockWidgetArea);
    MotionManageWidget *motionManageWidget = new MotionManageWidget(m_document, motionDocker);
    motionDocker->setWidget(motionManageWidget);
    connect(motionManageWidget, &MotionManageWidget::registerDialog, this, &DocumentWindow::registerDialog);
    connect(motionManageWidget, &MotionManageWidget::unregisterDialog, this, &DocumentWindow::unregisterDialog);
    addDockWidget(Qt::RightDockWidgetArea, motionDocker);
    
    QDockWidget *paintDocker = new QDockWidget(tr("Paint"), this);
    paintDocker->setMinimumWidth(Theme::sidebarPreferredWidth);
    paintDocker->setAllowedAreas(Qt::RightDockWidgetArea);
    QPushButton *lockMeshButton = new QPushButton(Theme::awesome()->icon(fa::lock), tr("Lock Object for Painting"));
    QPushButton *unlockMeshButton = new QPushButton(Theme::awesome()->icon(fa::unlock), tr("Remove Painting"));
    connect(lockMeshButton, &QPushButton::clicked, this, [=]() {
        m_document->setMeshLockState(true);
    });
    connect(unlockMeshButton, &QPushButton::clicked, this, [this]() {
        QMessageBox::StandardButton answer = QMessageBox::question(this,
            APP_NAME,
            tr("Do you really want to remove painting?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (answer == QMessageBox::No) {
            return;
        }
        this->m_document->setMeshLockState(false);
        this->m_document->regenerateMesh();
    });
    m_colorWheelWidget = new color_widgets::ColorWheel(nullptr);
    m_colorWheelWidget->setContentsMargins(0, 5, 0, 5);
    m_colorWheelWidget->setColor(m_document->brushColor);
    m_paintWidget = new QWidget(paintDocker);
    QVBoxLayout *paintLayout = new QVBoxLayout;
    paintLayout->setContentsMargins(5, 5, 5, 5);
    paintLayout->addWidget(lockMeshButton);
    paintLayout->addWidget(unlockMeshButton);
    paintLayout->addWidget(m_colorWheelWidget);
    paintLayout->addStretch();
    m_paintWidget->setLayout(paintLayout);
    paintDocker->setWidget(m_paintWidget);
    connect(m_colorWheelWidget, &color_widgets::ColorWheel::colorChanged, this, [=](QColor color) {
        m_document->brushColor = color;
    });
    connect(m_document, &Document::editModeChanged, this, [=]() {
        if (SkeletonDocumentEditMode::Paint == m_document->editMode) {
            paintDocker->show();
            paintDocker->raise();
        }
    });
    auto updatePaintWidgets = [=]() {
        m_colorWheelWidget->setVisible(m_document->objectLocked);
        lockMeshButton->setVisible(!m_document->objectLocked);
        unlockMeshButton->setVisible(m_document->objectLocked);
    };
    updatePaintWidgets();
    connect(m_document, &Document::objectLockStateChanged, this, [=]() {
        updatePaintWidgets();
    });
    addDockWidget(Qt::RightDockWidgetArea, paintDocker);
    
    QDockWidget *scriptDocker = new QDockWidget(tr("Script"), this);
    scriptDocker->setAllowedAreas(Qt::RightDockWidgetArea);
    ScriptWidget *scriptWidget = new ScriptWidget(m_document, scriptDocker);
    scriptDocker->setVisible(Preferences::instance().scriptEnabled());
    connect(&Preferences::instance(), &Preferences::scriptEnabledChanged, this, [=]() {
        scriptDocker->setVisible(Preferences::instance().scriptEnabled());
    });
    scriptDocker->setWidget(scriptWidget);
    addDockWidget(Qt::RightDockWidgetArea, scriptDocker);
    
    tabifyDockWidget(partsDocker, materialDocker);
    tabifyDockWidget(materialDocker, rigDocker);
    tabifyDockWidget(rigDocker, motionDocker);
    tabifyDockWidget(motionDocker, paintDocker);
    tabifyDockWidget(paintDocker, scriptDocker);
    
    partsDocker->raise();
    
    QWidget *titleBarWidget = new QWidget;
    titleBarWidget->setFixedHeight(1);
    
    QHBoxLayout *titleBarLayout = new QHBoxLayout;
    titleBarLayout->addStretch();
    titleBarWidget->setLayout(titleBarLayout);

    /////////////////////// Status Bar Begin ////////////////////////////
    
    QWidget *statusBarWidget = new QWidget;
    statusBarWidget->setContentsMargins(0, 0, 0, 0);
    
    StatusBarLabel *boneLabel = new StatusBarLabel;
    boneLabel->updateText(tr("Bone"));
    
    StatusBarLabel *shapeLabel = new StatusBarLabel;
    shapeLabel->setSelected(true);
    shapeLabel->updateText(tr("Shape"));
    
    connect(boneLabel, &StatusBarLabel::clicked, this, [=]() {
        boneLabel->setSelected(true);
        shapeLabel->setSelected(false);
        setGraphicsIndex(0);
        updateGraphicsViewEditTarget(GraphicsViewEditTarget::Bone);
    });
    connect(shapeLabel, &StatusBarLabel::clicked, this, [=]() {
        shapeLabel->setSelected(true);
        boneLabel->setSelected(false);
        setGraphicsIndex(1);
        updateGraphicsViewEditTarget(GraphicsViewEditTarget::Shape);
    });
    
    /////////////////////// Status Bar End ////////////////////////////
    
    QHBoxLayout *statusBarLayout = new QHBoxLayout;
    statusBarLayout->setSpacing(0);
    statusBarLayout->setContentsMargins(0, 0, 0, 0);
    statusBarLayout->addStretch();
    statusBarLayout->addWidget(boneLabel);
    statusBarLayout->addWidget(shapeLabel);
    statusBarWidget->setLayout(statusBarLayout);

    QVBoxLayout *canvasLayout = new QVBoxLayout;
    canvasLayout->setSpacing(0);
    canvasLayout->setContentsMargins(0, 0, 0, 0);
    canvasLayout->addWidget(titleBarWidget);
    canvasLayout->addWidget(containerWidget);
    canvasLayout->addWidget(statusBarWidget);
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

    m_exportAsObjAction = new QAction(tr("Export as OBJ..."), this);
    connect(m_exportAsObjAction, &QAction::triggered, this, &DocumentWindow::exportObjResult, Qt::QueuedConnection);
    m_fileMenu->addAction(m_exportAsObjAction);
    
    m_exportAsGlbAction = new QAction(tr("Export as GLB..."), this);
    connect(m_exportAsGlbAction, &QAction::triggered, this, &DocumentWindow::exportGlbResult, Qt::QueuedConnection);
    m_fileMenu->addAction(m_exportAsGlbAction);
    
    m_exportAsFbxAction = new QAction(tr("Export as FBX..."), this);
    connect(m_exportAsFbxAction, &QAction::triggered, this, &DocumentWindow::exportFbxResult, Qt::QueuedConnection);
    m_fileMenu->addAction(m_exportAsFbxAction);
    
    m_exportTexturesAction = new QAction(tr("Export Textures..."), this);
    connect(m_exportTexturesAction, &QAction::triggered, this, &DocumentWindow::exportTextures, Qt::QueuedConnection);
    m_fileMenu->addAction(m_exportTexturesAction);
    
    m_exportDs3objAction = new QAction(tr("Export DS3OBJ..."), this);
    connect(m_exportDs3objAction, &QAction::triggered, this, &DocumentWindow::exportDs3objResult, Qt::QueuedConnection);
    m_fileMenu->addAction(m_exportDs3objAction);
    
    m_exportRenderedAsImageAction = new QAction(tr("Export as Image..."), this);
    connect(m_exportRenderedAsImageAction, &QAction::triggered, this, &DocumentWindow::exportRenderedResult, Qt::QueuedConnection);
    m_fileMenu->addAction(m_exportRenderedAsImageAction);
    
    m_fileMenu->addSeparator();

    m_changeTurnaroundAction = new QAction(tr("Change Reference Sheet..."), this);
    connect(m_changeTurnaroundAction, &QAction::triggered, this, &DocumentWindow::changeTurnaround, Qt::QueuedConnection);
    m_fileMenu->addAction(m_changeTurnaroundAction);
    
    m_fileMenu->addSeparator();

    m_quitAction = m_fileMenu->addAction(tr("&Quit"),
                                         this, &DocumentWindow::close,
                                         QKeySequence::Quit);

    connect(m_fileMenu, &QMenu::aboutToShow, [=]() {
        m_exportAsObjAction->setEnabled(m_shapeGraphicsWidget->hasItems());
        m_exportAsGlbAction->setEnabled(m_shapeGraphicsWidget->hasItems() && m_document->isExportReady());
        m_exportAsFbxAction->setEnabled(m_shapeGraphicsWidget->hasItems() && m_document->isExportReady());
        m_exportTexturesAction->setEnabled(m_shapeGraphicsWidget->hasItems() && m_document->isExportReady());
        m_exportDs3objAction->setEnabled(m_shapeGraphicsWidget->hasItems() && m_document->isExportReady());
        m_exportRenderedAsImageAction->setEnabled(m_shapeGraphicsWidget->hasItems());
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
    
    m_showRigAction = new QAction(tr("Rig"), this);
    connect(m_showRigAction, &QAction::triggered, [=]() {
        rigDocker->show();
        rigDocker->raise();
    });
    m_windowMenu->addAction(m_showRigAction);
    
    m_showMotionsAction = new QAction(tr("Motions"), this);
    connect(m_showMotionsAction, &QAction::triggered, [=]() {
        motionDocker->show();
        motionDocker->raise();
    });
    m_windowMenu->addAction(m_showMotionsAction);
    
    m_showPaintAction = new QAction(tr("Paint"), this);
    connect(m_showPaintAction, &QAction::triggered, [=]() {
        paintDocker->show();
        paintDocker->raise();
    });
    m_windowMenu->addAction(m_showPaintAction);
    
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
    
    m_seeSupportersAction = new QAction(tr("Supporters"), this);
    connect(m_seeSupportersAction, &QAction::triggered, this, &DocumentWindow::seeSupporters);
    m_helpMenu->addAction(m_seeSupportersAction);

    m_seeAcknowlegementsAction = new QAction(tr("Acknowlegements"), this);
    connect(m_seeAcknowlegementsAction, &QAction::triggered, this, &DocumentWindow::seeAcknowlegements);
    m_helpMenu->addAction(m_seeAcknowlegementsAction);

    connect(containerWidget, &GraphicsContainerWidget::containerSizeChanged,
        shapeGraphicsWidget, &SkeletonGraphicsWidget::canvasResized);
    connect(containerWidget, &GraphicsContainerWidget::containerSizeChanged,
        boneGraphicsWidget, &SkeletonGraphicsWidget::canvasResized);

    connect(m_document, &Document::turnaroundChanged,
        shapeGraphicsWidget, &SkeletonGraphicsWidget::turnaroundChanged);
    connect(m_boneDocument, &BoneDocument::turnaroundChanged,
        boneGraphicsWidget, &SkeletonGraphicsWidget::turnaroundChanged);

    connect(addButton, &QPushButton::clicked, [=]() {
        m_document->setEditMode(SkeletonDocumentEditMode::Add);
    });

    connect(selectButton, &QPushButton::clicked, [=]() {
        m_document->setEditMode(SkeletonDocumentEditMode::Select);
    });
    
    connect(paintButton, &QPushButton::clicked, [=]() {
        m_document->setEditMode(SkeletonDocumentEditMode::Paint);
    });

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
    connect(m_document, &Document::editModeChanged, this, [=]() {
        m_boneDocument->setEditMode(m_document->editMode);
    });
    
    connect(m_document, &Document::xlockStateChanged, this, [=]() {
        m_boneDocument->setXlockState(m_document->xlocked);
    });
    connect(m_document, &Document::ylockStateChanged, this, [=]() {
        m_boneDocument->setYlockState(m_document->ylocked);
    });
    connect(m_document, &Document::zlockStateChanged, this, [=]() {
        m_boneDocument->setZlockState(m_document->zlocked);
    });
    connect(m_document, &Document::radiusLockStateChanged, this, [=]() {
        m_boneDocument->setRadiusLockState(m_document->radiusLocked);
    });

    m_partListDockerVisibleSwitchConnection = connect(m_document, &Document::skeletonChanged, [=]() {
        if (m_shapeGraphicsWidget->hasItems()) {
            if (partsDocker->isHidden())
                partsDocker->show();
            disconnect(m_partListDockerVisibleSwitchConnection);
        }
    });

    connect(m_document, &Document::editModeChanged, shapeGraphicsWidget, &SkeletonGraphicsWidget::editModeChanged);
    connect(m_boneDocument, &BoneDocument::editModeChanged, boneGraphicsWidget, &SkeletonGraphicsWidget::editModeChanged);
    
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::shortcutToggleWireframe, [=]() {
        m_modelRenderWidget->toggleWireframe();
    });
    
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::shortcutToggleFlatShading, [=]() {
        Preferences::instance().setFlatShading(!Preferences::instance().flatShading());
    });
    
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::shortcutToggleRotation, this, &DocumentWindow::toggleRotation);

    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::zoomRenderedModelBy, m_modelRenderWidget, &ModelWidget::zoom);

    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::addNode, m_document, &Document::addNode);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::scaleNodeByAddRadius, m_document, &Document::scaleNodeByAddRadius);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::moveNodeBy, m_document, &Document::moveNodeBy);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::setNodeOrigin, m_document, &Document::setNodeOrigin);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::setNodeBoneMark, m_document, &Document::setNodeBoneMark);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::clearNodeCutFaceSettings, m_document, &Document::clearNodeCutFaceSettings);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::removeNode, m_document, &Document::removeNode);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::removePart, m_document, &Document::removePart);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::setEditMode, m_document, &Document::setEditMode);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::removeEdge, m_document, &Document::removeEdge);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::addEdge, m_document, &Document::addEdge);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::groupOperationAdded, m_document, &Document::saveSnapshot);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::undo, m_document, &Document::undo);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::redo, m_document, &Document::redo);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::paste, m_document, &Document::paste);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::batchChangeBegin, m_document, &Document::batchChangeBegin);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::batchChangeEnd, m_document, &Document::batchChangeEnd);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::breakEdge, m_document, &Document::breakEdge);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::reduceNode, m_document, &Document::reduceNode);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::reverseEdge, m_document, &Document::reverseEdge);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::moveOriginBy, m_document, &Document::moveOriginBy);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::partChecked, m_document, &Document::partChecked);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::partUnchecked, m_document, &Document::partUnchecked);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::switchNodeXZ, m_document, &Document::switchNodeXZ);

    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::setPartLockState, m_document, &Document::setPartLockState);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::setPartVisibleState, m_document, &Document::setPartVisibleState);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::setPartSubdivState, m_document, &Document::setPartSubdivState);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::setPartChamferState, m_document, &Document::setPartChamferState);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::setPartColorState, m_document, &Document::setPartColorState);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::setPartDisableState, m_document, &Document::setPartDisableState);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::setPartXmirrorState, m_document, &Document::setPartXmirrorState);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::setPartRoundState, m_document, &Document::setPartRoundState);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::setPartWrapState, m_document, &Document::setPartCutRotation);

    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::setXlockState, m_document, &Document::setXlockState);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::setYlockState, m_document, &Document::setYlockState);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::setZlockState, m_document, &Document::setZlockState);
    
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::enableAllPositionRelatedLocks, m_document, &Document::enableAllPositionRelatedLocks);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::disableAllPositionRelatedLocks, m_document, &Document::disableAllPositionRelatedLocks);

    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::changeTurnaround, this, &DocumentWindow::changeTurnaround);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::open, this, &DocumentWindow::open);
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::showCutFaceSettingPopup, this, &DocumentWindow::showCutFaceSettingPopup);
    
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::showOrHideAllComponents, m_document, &Document::showOrHideAllComponents);
    
    connect(m_document, &Document::nodeAdded, shapeGraphicsWidget, &SkeletonGraphicsWidget::nodeAdded);
    connect(m_document, &Document::nodeRemoved, shapeGraphicsWidget, &SkeletonGraphicsWidget::nodeRemoved);
    connect(m_document, &Document::edgeAdded, shapeGraphicsWidget, &SkeletonGraphicsWidget::edgeAdded);
    connect(m_document, &Document::edgeRemoved, shapeGraphicsWidget, &SkeletonGraphicsWidget::edgeRemoved);
    connect(m_document, &Document::nodeRadiusChanged, shapeGraphicsWidget, &SkeletonGraphicsWidget::nodeRadiusChanged);
    connect(m_document, &Document::nodeBoneMarkChanged, shapeGraphicsWidget, &SkeletonGraphicsWidget::nodeBoneMarkChanged);
    connect(m_document, &Document::nodeOriginChanged, shapeGraphicsWidget, &SkeletonGraphicsWidget::nodeOriginChanged);
    connect(m_document, &Document::edgeReversed, shapeGraphicsWidget, &SkeletonGraphicsWidget::edgeReversed);
    connect(m_document, &Document::partVisibleStateChanged, shapeGraphicsWidget, &SkeletonGraphicsWidget::partVisibleStateChanged);
    connect(m_document, &Document::partDisableStateChanged, shapeGraphicsWidget, &SkeletonGraphicsWidget::partVisibleStateChanged);
    connect(m_document, &Document::cleanup, shapeGraphicsWidget, &SkeletonGraphicsWidget::removeAllContent);
    connect(m_document, &Document::originChanged, shapeGraphicsWidget, &SkeletonGraphicsWidget::originChanged);
    connect(m_document, &Document::checkPart, shapeGraphicsWidget, &SkeletonGraphicsWidget::selectPartAllById);
    connect(m_document, &Document::enableBackgroundBlur, shapeGraphicsWidget, &SkeletonGraphicsWidget::enableBackgroundBlur);
    connect(m_document, &Document::disableBackgroundBlur, shapeGraphicsWidget, &SkeletonGraphicsWidget::disableBackgroundBlur);
    connect(m_document, &Document::uncheckAll, shapeGraphicsWidget, &SkeletonGraphicsWidget::unselectAll);
    connect(m_document, &Document::checkNode, shapeGraphicsWidget, &SkeletonGraphicsWidget::addSelectNode);
    connect(m_document, &Document::checkEdge, shapeGraphicsWidget, &SkeletonGraphicsWidget::addSelectEdge);
    
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
    
    connect(m_partTreeWidget, &PartTreeWidget::addPartToSelection, shapeGraphicsWidget, &SkeletonGraphicsWidget::addPartToSelection);
    
    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::partComponentChecked, m_partTreeWidget, &PartTreeWidget::partComponentChecked);
    
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
    connect(m_document, &Document::partDeformMapImageIdChanged, m_partTreeWidget, &PartTreeWidget::partDeformChanged);
    connect(m_document, &Document::partDeformMapScaleChanged, m_partTreeWidget, &PartTreeWidget::partDeformChanged);
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
    connect(m_document, &Document::postProcessedResultChanged, m_document, &Document::generateRig);
    connect(m_document, &Document::rigChanged, m_document, &Document::generateRig);
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
    
    connect(m_document, &Document::motionsChanged, m_document, &Document::generateMotions);

    connect(shapeGraphicsWidget, &SkeletonGraphicsWidget::cursorChanged, [=]() {
        m_modelRenderWidget->setCursor(shapeGraphicsWidget->cursor());
        containerWidget->setCursor(shapeGraphicsWidget->cursor());
    });

    connect(m_document, &Document::skeletonChanged, this, &DocumentWindow::documentChanged);
    connect(m_document, &Document::turnaroundChanged, this, &DocumentWindow::documentChanged);
    connect(m_document, &Document::optionsChanged, this, &DocumentWindow::documentChanged);
    connect(m_document, &Document::rigChanged, this, &DocumentWindow::documentChanged);
    connect(m_document, &Document::scriptChanged, this, &DocumentWindow::documentChanged);

    connect(m_modelRenderWidget, &ModelWidget::customContextMenuRequested, [=](const QPoint &pos) {
        shapeGraphicsWidget->showContextMenu(shapeGraphicsWidget->mapFromGlobal(m_modelRenderWidget->mapToGlobal(pos)));
    });

    connect(m_document, &Document::xlockStateChanged, this, &DocumentWindow::updateXlockButtonState);
    connect(m_document, &Document::ylockStateChanged, this, &DocumentWindow::updateYlockButtonState);
    connect(m_document, &Document::zlockStateChanged, this, &DocumentWindow::updateZlockButtonState);
    connect(m_document, &Document::radiusLockStateChanged, this, &DocumentWindow::updateRadiusLockButtonState);
    
    connect(m_rigWidget, &RigWidget::setRigType, m_document, &Document::setRigType);
    
    connect(m_document, &Document::rigTypeChanged, m_rigWidget, &RigWidget::rigTypeChanged);
    connect(m_document, &Document::resultRigChanged, m_rigWidget, &RigWidget::updateResultInfo);
    connect(m_document, &Document::resultRigChanged, this, &DocumentWindow::updateRigWeightRenderWidget);
    
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
    
    connect(m_document, &Document::skeletonChanged, this, &DocumentWindow::generateSilhouetteImage);
    connect(m_shapeGraphicsWidget, &SkeletonGraphicsWidget::loadedTurnaroundImageChanged, this, &DocumentWindow::generateSilhouetteImage);
    
    initShortCuts(this, m_shapeGraphicsWidget);
    //initShortCuts(this, m_boneGraphicsWidget);

    connect(this, &DocumentWindow::initialized, m_document, &Document::uiReady);
    connect(this, &DocumentWindow::initialized, this, &DocumentWindow::autoRecover);
    
    m_autoSaver = new AutoSaver(m_document);
    
    QTimer *timer = new QTimer(this);
    timer->setInterval(250);
    connect(timer, &QTimer::timeout, [=] {
        QWidget *focusedWidget = QApplication::focusWidget();
        if (nullptr == focusedWidget && isActiveWindow())
            shapeGraphicsWidget->setFocus();
    });
    timer->start();
}

void DocumentWindow::updateRegenerateIcon()
{
    if (m_document->isMeshGenerating() ||
            m_document->isPostProcessing() ||
            m_document->isTextureGenerating()) {
        m_regenerateButton->showSpinner(true);
        if (nullptr != m_paintWidget)
            m_paintWidget->hide();
    } else {
        m_regenerateButton->showSpinner(false);
        if (m_document->objectLocked) {
            m_regenerateButton->setToolTip(tr("Object locked for painting"));
            m_regenerateButton->setAwesomeIcon(QChar(fa::lock));
        } else {
            m_regenerateButton->setToolTip(m_isLastMeshGenerationSucceed ? tr("Regenerate") : tr("Mesh generation failed, please undo or adjust recent changed nodes\nTips:\n  - Don't let generated mesh self-intersect\n  - Make multiple parts instead of one single part for whole model"));
            m_regenerateButton->setAwesomeIcon(m_isLastMeshGenerationSucceed ? QChar(fa::recycle) : QChar(fa::warning));
        }
        if (nullptr != m_paintWidget)
            m_paintWidget->show();
    }
}

void DocumentWindow::toggleRotation()
{
    if (nullptr == m_shapeGraphicsWidget)
        return;
    m_shapeGraphicsWidget->setRotated(!m_shapeGraphicsWidget->rotated());
    m_boneGraphicsWidget->setRotated(m_shapeGraphicsWidget->rotated());
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

void DocumentWindow::initializeLockButton(QPushButton *button)
{
    QFont font;
    font.setWeight(QFont::Light);
    font.setPixelSize(Theme::toolIconFontSize);
    font.setBold(false);

    button->setFont(font);
    button->setFixedSize(Theme::toolIconSize, Theme::toolIconSize);
    button->setStyleSheet("QPushButton {color: " + Theme::white.name() + "}");
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
        m_shapeGraphicsWidget->setFocus();
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
        ensureFileExtension(&filename, ".ds3");
    }
    
    QApplication::setOverrideCursor(Qt::WaitCursor);
    Snapshot snapshot;
    m_document->toSnapshot(&snapshot);
    bool saveObject = m_document->objectLocked;
    DocumentSaver::Textures textures;
    if (saveObject) {
        textures.textureImage = m_document->textureImage;
        textures.textureImageByteArray = m_document->textureImageByteArray;
        textures.textureNormalImage = m_document->textureNormalImage;
        textures.textureNormalImageByteArray = m_document->textureNormalImageByteArray;
        textures.textureMetalnessImage = m_document->textureMetalnessImage;
        textures.textureMetalnessImageByteArray = m_document->textureMetalnessImageByteArray;
        textures.textureRoughnessImage = m_document->textureRoughnessImage;
        textures.textureRoughnessImageByteArray = m_document->textureRoughnessImageByteArray;
        textures.textureAmbientOcclusionImage = m_document->textureAmbientOcclusionImage;
        textures.textureAmbientOcclusionImageByteArray = m_document->textureAmbientOcclusionImageByteArray;
    }
    if (DocumentSaver::save(&filename, 
            &snapshot, 
            saveObject ? &m_document->currentPostProcessedObject() : nullptr,
            saveObject ? &textures : nullptr,
            (!m_document->turnaround.isNull() && m_document->turnaroundPngByteArray.size() > 0) ? 
                &m_document->turnaroundPngByteArray : nullptr,
            (!m_document->script().isEmpty()) ? &m_document->script() : nullptr,
            (!m_document->variables().empty()) ? &m_document->variables() : nullptr)) {
        setCurrentFilename(filename);
    }
    if (saveObject) {
        textures.textureImage = nullptr;
        textures.textureNormalImage = nullptr;
        textures.textureMetalnessImage = nullptr;
        textures.textureRoughnessImage = nullptr;
        textures.textureAmbientOcclusionImage = nullptr;
        
        if (textures.textureImageByteArray != m_document->textureImageByteArray)
            std::swap(textures.textureImageByteArray, m_document->textureImageByteArray);
        else
            textures.textureImageByteArray = nullptr;
        
        if (textures.textureNormalImageByteArray != m_document->textureNormalImageByteArray)
            std::swap(textures.textureNormalImageByteArray, m_document->textureNormalImageByteArray);
        else
            textures.textureNormalImageByteArray = nullptr;
        
        if (textures.textureMetalnessImageByteArray != m_document->textureMetalnessImageByteArray)
            std::swap(textures.textureMetalnessImageByteArray, m_document->textureMetalnessImageByteArray);
        else
            textures.textureMetalnessImageByteArray = nullptr;
        
        if (textures.textureRoughnessImageByteArray != m_document->textureRoughnessImageByteArray)
            std::swap(textures.textureRoughnessImageByteArray, m_document->textureRoughnessImageByteArray);
        else
            textures.textureRoughnessImageByteArray = nullptr;
        
        if (textures.textureAmbientOcclusionImageByteArray != m_document->textureAmbientOcclusionImageByteArray)
            std::swap(textures.textureAmbientOcclusionImageByteArray, m_document->textureAmbientOcclusionImageByteArray);
        else
            textures.textureAmbientOcclusionImageByteArray = nullptr;
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
    snapshotComponent["combineMode"] = CombineModeToString(Preferences::instance().componentCombineMode());
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

    m_document->clearHistories();
    m_document->resetScript();
    m_document->reset();
    m_document->clearTurnaround();
    m_document->saveSnapshot();
    
    if (path.endsWith(".xml")) {
        QFile file(path);
        file.open(QIODevice::ReadOnly);
        QXmlStreamReader stream(&file);
        
        Snapshot snapshot;
        loadSkeletonFromXmlStream(&snapshot, stream);
        m_document->fromSnapshot(snapshot);
        m_document->saveSnapshot();
    } else {
        Ds3FileReader ds3Reader(path);
        
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
                    m_document->updateTurnaround(QImage::fromData(data, "PNG"));
                } else if (item.name == "object_color.png") {
                    QByteArray data;
                    ds3Reader.loadItem(item.name, &data);
                    m_document->updateTextureImage(new QImage(QImage::fromData(data, "PNG")));
                } else if (item.name == "object_normal.png") {
                    QByteArray data;
                    ds3Reader.loadItem(item.name, &data);
                    m_document->updateTextureNormalImage(new QImage(QImage::fromData(data, "PNG")));
                } else if (item.name == "object_metallic.png") {
                    QByteArray data;
                    ds3Reader.loadItem(item.name, &data);
                    m_document->updateTextureMetalnessImage(new QImage(QImage::fromData(data, "PNG")));
                } else if (item.name == "object_roughness.png") {
                    QByteArray data;
                    ds3Reader.loadItem(item.name, &data);
                    m_document->updateTextureRoughnessImage(new QImage(QImage::fromData(data, "PNG")));
                } else if (item.name == "object_ao.png") {
                    QByteArray data;
                    ds3Reader.loadItem(item.name, &data);
                    m_document->updateTextureAmbientOcclusionImage(new QImage(QImage::fromData(data, "PNG")));
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
        
        for (int i = 0; i < ds3Reader.items().size(); ++i) {
            Ds3ReaderItem item = ds3Reader.items().at(i);
            if (item.type == "object") {
                if (item.name == "object.xml") {
                    QByteArray data;
                    ds3Reader.loadItem(item.name, &data);
                    QXmlStreamReader stream(data);
                    Object *object = new Object;
                    loadObjectFromXmlStream(object, stream);
                    m_document->updateObject(object);
                }
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
    ensureFileExtension(&filename, ".png");
    exportImageToFilename(filename);
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

void DocumentWindow::exportTextures()
{
    QString directory = QFileDialog::getExistingDirectory(this, QString(), QString(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (directory.isEmpty()) {
        return;
    }
    exportTexturesToDirectory(directory);
}

void DocumentWindow::exportTexturesToDirectory(const QString &directory)
{
    if (!m_document->isExportReady()) {
        qDebug() << "Export but document is not export ready";
        return;
    }
    
    QDir dir(directory);
    
    auto exportTextureImage = [&](const QString &filename, const QImage *image) {
        if (nullptr == image)
            return;
        QString fullFilename = dir.absoluteFilePath(filename);
        image->save(fullFilename);
    };
    
    QApplication::setOverrideCursor(Qt::WaitCursor);
    exportTextureImage("color.png", m_document->textureImage);
    exportTextureImage("normal.png", m_document->textureNormalImage);
    exportTextureImage("metallic.png", m_document->textureMetalnessImage);
    exportTextureImage("roughness.png", m_document->textureRoughnessImage);
    exportTextureImage("ao.png", m_document->textureAmbientOcclusionImage);
    QApplication::restoreOverrideCursor();
}

void DocumentWindow::exportFbxToFilename(const QString &filename)
{
    if (!m_document->isExportReady()) {
        qDebug() << "Export but document is not export ready";
        return;
    }
    QApplication::setOverrideCursor(Qt::WaitCursor);
    Object skeletonResult = m_document->currentPostProcessedObject();
    std::vector<std::pair<QString, std::vector<std::pair<float, JointNodeTree>>>> exportMotions;
    for (const auto &motionIt: m_document->motionMap) {
        exportMotions.push_back({motionIt.second.name, motionIt.second.jointNodeTrees});
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
       tr("glTF Binary Format (*.glb)"));
    if (filename.isEmpty()) {
        return;
    }
    ensureFileExtension(&filename, ".glb");
    exportGlbToFilename(filename);
}

void DocumentWindow::exportDs3objResult()
{
    QString filename = QFileDialog::getSaveFileName(this, QString(), QString(),
       tr("Dust3D Object (*.ds3obj)"));
    if (filename.isEmpty()) {
        return;
    }
    ensureFileExtension(&filename, ".ds3obj");
    exportDs3objToFilename(filename);
}

void DocumentWindow::exportGlbToFilename(const QString &filename)
{
    if (!m_document->isExportReady()) {
        qDebug() << "Export but document is not export ready";
        return;
    }
    QApplication::setOverrideCursor(Qt::WaitCursor);
    Object skeletonResult = m_document->currentPostProcessedObject();
    std::vector<std::pair<QString, std::vector<std::pair<float, JointNodeTree>>>> exportMotions;
    for (const auto &motionIt: m_document->motionMap) {
        exportMotions.push_back({motionIt.second.name, motionIt.second.jointNodeTrees});
    }
    QImage *textureMetalnessRoughnessAmbientOcclusionImage = 
        TextureGenerator::combineMetalnessRoughnessAmbientOcclusionImages(m_document->textureMetalnessImage,
            m_document->textureRoughnessImage,
            m_document->textureAmbientOcclusionImage);
    GlbFileWriter glbFileWriter(skeletonResult, m_document->resultRigBones(), m_document->resultRigWeights(), filename,
        m_document->textureImage, m_document->textureNormalImage, textureMetalnessRoughnessAmbientOcclusionImage, exportMotions.empty() ? nullptr : &exportMotions);
    glbFileWriter.save();
    delete textureMetalnessRoughnessAmbientOcclusionImage;
    QApplication::restoreOverrideCursor();
}

void DocumentWindow::exportDs3objToFilename(const QString &filename)
{
    if (!m_document->isExportReady()) {
        qDebug() << "Export but document is not export ready";
        return;
    }
    QApplication::setOverrideCursor(Qt::WaitCursor);
    Ds3FileWriter ds3Writer;

    {
        QByteArray objectXml;
        QXmlStreamWriter stream(&objectXml);
        saveObjectToXmlStream(&m_document->currentPostProcessedObject(), &stream);
        if (objectXml.size() > 0)
            ds3Writer.add("object.xml", "object", &objectXml);
    }
    
    const std::vector<RigBone> *rigBones = m_document->resultRigBones();
    const std::map<int, RigVertexWeights> *rigWeights = m_document->resultRigWeights();
    if (nullptr != rigBones && nullptr != rigWeights) {
        QByteArray rigXml;
        QXmlStreamWriter stream(&rigXml);
        saveRigToXmlStream(&m_document->currentPostProcessedObject(), rigBones, rigWeights, &stream);
        if (rigXml.size() > 0)
            ds3Writer.add("rig.xml", "rig", &rigXml);
    }
    
    {
        QByteArray motionsXml;
        {
            QXmlStreamWriter stream(&motionsXml);
            QXmlStreamWriter *writer = &stream;
            writer->setAutoFormatting(true);
            writer->writeStartDocument();
            writer->writeStartElement("motions");
            for (const auto &motionIt: m_document->motionMap) {
                writer->writeStartElement("motion");
                    writer->writeAttribute("name", motionIt.second.name);
                    writer->writeStartElement("frames");
                        for (const auto &it: motionIt.second.jointNodeTrees) {
                            writer->writeStartElement("frame");
                                writer->writeAttribute("duration", QString::number(it.first));
                                writer->writeStartElement("bones");
                                    const auto &nodes = it.second.nodes();
                                    for (size_t boneIndex = 0; boneIndex < nodes.size(); ++boneIndex) {
                                        const auto &node = nodes[boneIndex];
                                        writer->writeStartElement("bone");
                                        writer->writeAttribute("index", QString::number(boneIndex));
                                        {
                                            QMatrix4x4 translationMatrix;
                                            translationMatrix.translate(node.translation);
                                            QMatrix4x4 rotationMatrix;
                                            rotationMatrix.rotate(node.rotation);
                                            QMatrix4x4 matrix = translationMatrix * rotationMatrix;
                                            const float *floatArray = matrix.constData();
                                            QStringList matrixItemList;
                                            for (auto j = 0u; j < 16; j++) {
                                                matrixItemList += QString::number(floatArray[j]);
                                            }
                                            writer->writeAttribute("matrix", matrixItemList.join(","));
                                        }
                                        {
                                            writer->writeAttribute("translation", 
                                                QString::number(node.translation.x()) + "," +
                                                QString::number(node.translation.y()) + "," +
                                                QString::number(node.translation.z()));
                                        }
                                        {
                                            writer->writeAttribute("rotation", 
                                                QString::number(node.rotation.x()) + "," +
                                                QString::number(node.rotation.y()) + "," +
                                                QString::number(node.rotation.z()) + "," +
                                                QString::number(node.rotation.scalar()));
                                        }
                                        writer->writeEndElement();
                                    }
                                writer->writeEndElement();
                            writer->writeEndElement();
                        }
                    writer->writeEndElement();
                writer->writeEndElement();
            }
            writer->writeEndElement();
            writer->writeEndDocument();
        }
        if (motionsXml.size() > 0)
            ds3Writer.add("motions.xml", "motions", &motionsXml);
    }
    
    auto saveTexture = [&](const QString &filename, const QImage *image) {
        if (nullptr == image)
            return;
        QByteArray byteArray;
        QBuffer pngBuffer(&byteArray);
        pngBuffer.open(QIODevice::WriteOnly);
        image->save(&pngBuffer, "PNG");
        ds3Writer.add(filename, "asset", &byteArray);
    };
    
    saveTexture("object_color.png", m_document->textureImage);
    saveTexture("object_normal.png", m_document->textureNormalImage);
    saveTexture("object_metallic.png", m_document->textureMetalnessImage);
    saveTexture("object_roughness.png", m_document->textureRoughnessImage);
    saveTexture("object_ao.png", m_document->textureAmbientOcclusionImage);
    
    ds3Writer.save(filename);
    
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
    if (nullptr == m_shapeGraphicsWidget)
        return;
    if (m_shapeGraphicsWidget->rotated()) {
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
        CutFace cutFace;
        QUuid cutFacePartId(cutFaceString);
        QPushButton *button = new QPushButton;
        button->setIconSize(QSize(Theme::toolIconSize * 0.75, Theme::toolIconSize * 0.75));
        if (cutFacePartId.isNull()) {
            cutFace = CutFaceFromString(cutFaceString.toUtf8().constData());
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
            if (CutFace::UserDefined == node->cutFace) {
                if (node->cutFaceLinkedId.toString() == cutFaceList[i]) {
                    updateCutFaceButtonState(i);
                    break;
                }
            } else if (i < (int)CutFace::UserDefined) {
                if ((size_t)node->cutFace == i) {
                    updateCutFaceButtonState(i);
                    break;
                }
            }
        }
    }
    
    QVBoxLayout *popupLayout = new QVBoxLayout;
    popupLayout->addLayout(rotationLayout);
    popupLayout->addWidget(Theme::createHorizontalLineWidget());
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
        } else if (filename.endsWith(".png")) {
            exportImageToFilename(filename);
            emit waitingExportFinished(filename, isSuccessful);
        } else {
            emit waitingExportFinished(filename, false);
        }
    }
}

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
    if (ToonLine::WithoutLine == Preferences::instance().toonLine())
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
        offlineRender->setEyePosition(m_modelRenderWidget->eyePosition());
        offlineRender->setMoveToPosition(m_modelRenderWidget->moveToPosition());
        if (m_modelRenderWidget->isWireframeVisible())
            offlineRender->enableWireframe();
        if (m_modelRenderWidget->isEnvironmentLightEnabled())
            offlineRender->enableEnvironmentLight();
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
        m_partPreviewImagesGenerator->addPart(part.first, part.second.takePreviewMesh(), PartTarget::CutFace == part.second.target);
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
    std::map<QUuid, QImage> *partImages = m_partPreviewImagesGenerator->takePartImages();
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

void DocumentWindow::generateSilhouetteImage()
{
    if (GraphicsViewEditTarget::Bone != m_graphicsViewEditTarget ||
            nullptr != m_silhouetteImageGenerator) {
        m_isSilhouetteImageObsolete = true;
        return;
    }
    
    m_isSilhouetteImageObsolete = false;
    
    const QImage *loadedTurnaroundImage = m_shapeGraphicsWidget->loadedTurnaroundImage();
    if (nullptr == loadedTurnaroundImage) {
        return;
    }
    
    QThread *thread = new QThread;
    
    Snapshot *snapshot = new Snapshot;
    m_document->toSnapshot(snapshot);
    
    m_silhouetteImageGenerator = new SilhouetteImageGenerator(loadedTurnaroundImage->width(), 
        loadedTurnaroundImage->height(), snapshot);
    m_silhouetteImageGenerator->moveToThread(thread);
    connect(thread, &QThread::started, m_silhouetteImageGenerator, &SilhouetteImageGenerator::process);
    connect(m_silhouetteImageGenerator, &SilhouetteImageGenerator::finished, this, &DocumentWindow::silhouetteImageReady);
    connect(m_silhouetteImageGenerator, &SilhouetteImageGenerator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void DocumentWindow::silhouetteImageReady()
{
    QImage *image = m_silhouetteImageGenerator->takeResultImage();
    if (nullptr != image)
        m_boneDocument->updateTurnaround(*image);
    delete image;
    
    delete m_silhouetteImageGenerator;
    m_silhouetteImageGenerator = nullptr;
    
    if (m_isSilhouetteImageObsolete)
        generateSilhouetteImage();
}

void DocumentWindow::initializeShortcuts()
{
    
}