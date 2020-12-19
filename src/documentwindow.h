#ifndef DUST3D_DOCUMENT_WINDOW_H
#define DUST3D_DOCUMENT_WINDOW_H
#include <QMainWindow>
#include <QShowEvent>
#include <QPushButton>
#include <QString>
#include <QMenu>
#include <QAction>
#include <QTextBrowser>
#include <map>
#include <QStringList>
#include <QLabel>
#include <QShortcut>
#include "modelwidget.h"
#include "rigwidget.h"
#include "bonemark.h"
#include "preferenceswidget.h"
#include "graphicscontainerwidget.h"
#include "normalanddepthmapsgenerator.h"
#include "autosaver.h"
#include "partpreviewimagesgenerator.h"
#include "QtColorWidgets/ColorWheel"

class Document;
class SkeletonGraphicsWidget;
class PartTreeWidget;
class SpinnableAwesomeButton;
class SilhouetteImageGenerator;
class BoneDocument;
class Snapshot;

class DocumentWindow : public QMainWindow
{
    Q_OBJECT
signals:
    void initialized();
    void uninialized();
    void waitingExportFinished(const QString &filename, bool isSuccessful);
    void mouseTargetVertexPositionChanged(const QVector3D &position);
    void graphicsViewEditTargetChanged();
public:
    enum class GraphicsViewEditTarget
    {
        Shape,
        Bone
    };

    DocumentWindow();
    ~DocumentWindow();
    Document *document();
    ModelWidget *modelWidget();
    GraphicsViewEditTarget graphicsViewEditTarget();
    static DocumentWindow *createDocumentWindow();
    static const std::map<DocumentWindow *, QUuid> &documentWindows();
    static void showAcknowlegements();
    static void showContributors();
    static void showSupporters();
    static void showAbout();
    static size_t total();
protected:
    void showEvent(QShowEvent *event);
    void closeEvent(QCloseEvent *event);
    void mousePressEvent(QMouseEvent *event);
public slots:
    void changeTurnaround();
    void save();
    void saveTo(const QString &saveAsFilename);
    void open();
    void openExample(const QString &modelName);
    void openPathAs(const QString &path, const QString &asName);
    void exportRenderedResult();
    void exportObjResult();
    void exportGlbResult();
    void exportFbxResult();
    void exportTextures();
    void exportDs3objResult();
    void newWindow();
    void newDocument();
    void saveAs();
    void saveAll();
    void gotoHomepage();
    void viewSource();
    void about();
    void checkForUpdates();
    void reportIssues();
    void seeAcknowlegements();
    void seeContributors();
    void seeSupporters();
    void seeReferenceGuide();
    void documentChanged();
    void updateRotationButtonState();
    void updateXlockButtonState();
    void updateYlockButtonState();
    void updateZlockButtonState();
    void updateRadiusLockButtonState();
    void updateRigWeightRenderWidget();
    void registerDialog(QWidget *widget);
    void unregisterDialog(QWidget *widget);
    void showPreferences();
    void showCutFaceSettingPopup(const QPoint &globalPos, std::set<QUuid> nodeIds);
    void setExportWaitingList(const QStringList &filenames);
    void checkExportWaitingList();
    void exportImageToFilename(const QString &filename);
    void exportObjToFilename(const QString &filename);
    void exportFbxToFilename(const QString &filename);
    void exportGlbToFilename(const QString &filename);
    void exportTexturesToDirectory(const QString &directory);
    void exportDs3objToFilename(const QString &filename);
    void toggleRotation();
    void generateNormalAndDepthMaps();
    void delayedGenerateNormalAndDepthMaps();
    void normalAndDepthMapsReady();
    void autoRecover();
    void import();
    void importPath(const QString &filename);
    void generatePartPreviewImages();
    void partPreviewImagesReady();
    void updateRegenerateIcon();
    void updateGraphicsViewEditTarget(GraphicsViewEditTarget target);
    void generateSilhouetteImage();
    void silhouetteImageReady();
private:
    void initializeLockButton(QPushButton *button);
    void setCurrentFilename(const QString &filename);
    void updateTitle();
    void createPartSnapshotForFillMesh(const QUuid &fillMeshFileId, Snapshot *snapshot);
    void initializeShortcuts();
    void initializeToolShortcuts(SkeletonGraphicsWidget *graphicsWidget);
    void initializeCanvasShortcuts(SkeletonGraphicsWidget *graphicsWidget);
    QShortcut *createShortcut(QKeySequence key);
private:
    Document *m_document = nullptr;
    BoneDocument *m_boneDocument = nullptr;
    bool m_firstShow = true;
    bool m_documentSaved = true;
    PreferencesWidget *m_preferencesWidget = nullptr;
    std::vector<QWidget *> m_dialogs;
    bool m_isLastMeshGenerationSucceed = true;
    quint64 m_currentUpdatedMeshId = 0;
    QStringList m_waitingForExportToFilenames;
    color_widgets::ColorWheel *m_colorWheelWidget = nullptr;
private:
    QString m_currentFilename;
    
    ModelWidget *m_modelRenderWidget = nullptr;
    SkeletonGraphicsWidget *m_shapeGraphicsWidget = nullptr;
    SkeletonGraphicsWidget *m_boneGraphicsWidget = nullptr;
    RigWidget *m_rigWidget = nullptr;
    GraphicsContainerWidget *m_graphicsContainerWidget = nullptr;
    
    QMenu *m_fileMenu = nullptr;
    QAction *m_newWindowAction = nullptr;
    QAction *m_newDocumentAction = nullptr;
    QAction *m_openAction = nullptr;
    QMenu *m_openExampleMenu = nullptr;
    QAction *m_saveAction = nullptr;
    QAction *m_saveAsAction = nullptr;
    QAction *m_saveAllAction = nullptr;
    QAction *m_showPreferencesAction = nullptr;
    QAction *m_changeTurnaroundAction = nullptr;
    QAction *m_quitAction = nullptr;
    
    QAction *m_importAction = nullptr;
    
    QAction *m_exportAsObjAction = nullptr;
    QAction *m_exportAsGlbAction = nullptr;
    QAction *m_exportAsFbxAction = nullptr;
    QAction *m_exportTexturesAction = nullptr;
    QAction *m_exportDs3objAction = nullptr;
    QAction *m_exportRenderedAsImageAction = nullptr;
    
    QMenu *m_viewMenu = nullptr;
    QAction *m_toggleWireframeAction = nullptr;
    QAction *m_toggleUvCheckAction = nullptr;
    QAction *m_toggleRotationAction = nullptr;
    QAction *m_toggleColorAction = nullptr;
    bool m_modelRemoveColor = false;
    
    QMenu *m_windowMenu = nullptr;
    QAction *m_showPartsListAction = nullptr;
    QAction *m_showDebugDialogAction = nullptr;
    QAction *m_showMaterialsAction = nullptr;
    QAction *m_showRigAction = nullptr;
    QAction *m_showMotionsAction = nullptr;
    QAction *m_showPaintAction = nullptr;
    QAction *m_showScriptAction = nullptr;
    
    QMenu *m_helpMenu = nullptr;
    QAction *m_gotoHomepageAction = nullptr;
    QAction *m_viewSourceAction = nullptr;
    QAction *m_aboutAction = nullptr;
    QAction *m_checkForUpdatesAction = nullptr;
    QAction *m_reportIssuesAction = nullptr;
    QAction *m_seeContributorsAction = nullptr;
    QAction *m_seeSupportersAction = nullptr;
    QAction *m_seeAcknowlegementsAction = nullptr;
    QAction *m_seeReferenceGuideAction = nullptr;
    
    QPushButton *m_rotationButton = nullptr;

    QPushButton *m_xlockButton = nullptr;
    QPushButton *m_ylockButton = nullptr;
    QPushButton *m_zlockButton = nullptr;
    QPushButton *m_radiusLockButton = nullptr;
    
    QMetaObject::Connection m_partListDockerVisibleSwitchConnection;
    
    NormalAndDepthMapsGenerator *m_normalAndDepthMapsGenerator = nullptr;
    bool m_isNormalAndDepthMapsObsolete = false;
    
    AutoSaver *m_autoSaver = nullptr;
    
    PartPreviewImagesGenerator *m_partPreviewImagesGenerator = nullptr;
    bool m_isPartPreviewImagesObsolete = false;
    
    PartTreeWidget *m_partTreeWidget = nullptr;
    
    SpinnableAwesomeButton *m_regenerateButton = nullptr;
    
    QWidget *m_paintWidget = nullptr;
    
    GraphicsViewEditTarget m_graphicsViewEditTarget = GraphicsViewEditTarget::Shape;
    bool m_isSilhouetteImageObsolete = false;
    SilhouetteImageGenerator *m_silhouetteImageGenerator = nullptr;
    
    std::map<QKeySequence, QShortcut *> m_shortcutMap;
public:
    static int m_autoRecovered;
};

#endif

