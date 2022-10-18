#ifndef DUST3D_APPLICATION_DOCUMENT_WINDOW_H_
#define DUST3D_APPLICATION_DOCUMENT_WINDOW_H_

#include "component_preview_images_decorator.h"
#include "graphics_container_widget.h"
#include "mesh_preview_images_generator.h"
#include "model_widget.h"
#include <QAction>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QPushButton>
#include <QShortcut>
#include <QShowEvent>
#include <QString>
#include <QStringList>
#include <map>
#include <memory>

class Document;
class SkeletonGraphicsWidget;
class PartManageWidget;
class ToolbarButton;
class SpinnableToolbarIcon;

class DocumentWindow : public QMainWindow {
    Q_OBJECT
signals:
    void initialized();
    void uninialized();
    void waitingExportFinished(const QString& filename, bool isSuccessful);
    void workingStatusChanged(bool isWorking);

public:
    DocumentWindow();
    ~DocumentWindow();
    Document* document();
    ModelWidget* modelWidget();
    bool isWorking();
    static DocumentWindow* createDocumentWindow();
    static const std::map<DocumentWindow*, dust3d::Uuid>& documentWindows();
    static void showAcknowlegements();
    static void showContributors();
    static void showSupporters();
    static void showAbout();
    static size_t total();

protected:
    void showEvent(QShowEvent* event);
    void closeEvent(QCloseEvent* event);
    void mousePressEvent(QMouseEvent* event);
public slots:
    void changeTurnaround();
    void save();
    void saveTo(const QString& saveAsFilename);
    void open();
    void openExample(const QString& modelName);
    void openPathAs(const QString& path, const QString& asName);
    void exportObjResult();
    void exportGlbResult();
    void exportFbxResult();
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
    void updateXlockButtonState();
    void updateYlockButtonState();
    void updateZlockButtonState();
    void updateRadiusLockButtonState();
    void registerDialog(QWidget* widget);
    void unregisterDialog(QWidget* widget);
    void setExportWaitingList(const QStringList& filenames);
    void checkExportWaitingList();
    void exportObjToFilename(const QString& filename);
    void exportFbxToFilename(const QString& filename);
    void exportGlbToFilename(const QString& filename);
    void toggleRotation();
    void generateComponentPreviewImages();
    void componentPreviewImagesReady();
    void decorateComponentPreviewImages();
    void componentPreviewImageDecorationsReady();
    void updateInprogressIndicator();
    void openRecentFile();
    void updateRecentFileActions();
    void toggleRenderColor();

private:
    void setCurrentFilename(const QString& filename);
    void updateTitle();
    void initializeShortcuts();
    void initializeToolShortcuts(SkeletonGraphicsWidget* graphicsWidget);
    void initializeCanvasShortcuts(SkeletonGraphicsWidget* graphicsWidget);
    QShortcut* createShortcut(QKeySequence key);
    QString strippedName(const QString& fullFileName);
    bool openFiles(const QStringList& pathList);

    Document* m_document = nullptr;
    bool m_firstShow = true;
    bool m_documentSaved = true;
    std::vector<QWidget*> m_dialogs;
    bool m_isLastMeshGenerationSucceed = true;
    quint64 m_currentUpdatedMeshId = 0;
    QStringList m_waitingForExportToFilenames;

    QString m_currentFilename;

    ModelWidget* m_modelRenderWidget = nullptr;
    SkeletonGraphicsWidget* m_canvasGraphicsWidget = nullptr;
    GraphicsContainerWidget* m_graphicsContainerWidget = nullptr;

    QMenu* m_fileMenu = nullptr;
    QAction* m_newWindowAction = nullptr;
    QAction* m_newDocumentAction = nullptr;
    QAction* m_openAction = nullptr;
    QMenu* m_openExampleMenu = nullptr;
    QAction* m_saveAction = nullptr;
    QAction* m_saveAsAction = nullptr;
    QAction* m_saveAllAction = nullptr;
    QAction* m_changeTurnaroundAction = nullptr;
    std::vector<QAction*> m_recentFileActions;
    QAction* m_recentFileSeparatorAction = nullptr;
    QAction* m_quitAction = nullptr;

    QAction* m_exportAsObjAction = nullptr;
    QAction* m_exportAsGlbAction = nullptr;
    QAction* m_exportAsFbxAction = nullptr;

    QMenu* m_viewMenu = nullptr;
    QAction* m_toggleWireframeAction = nullptr;
    QAction* m_toggleRotationAction = nullptr;
    QAction* m_toggleColorAction = nullptr;
    bool m_modelRemoveColor = false;

    QMenu* m_windowMenu = nullptr;
    QAction* m_showPartsListAction = nullptr;
    QAction* m_showDebugDialogAction = nullptr;
    QAction* m_showMaterialsAction = nullptr;

    QMenu* m_helpMenu = nullptr;
    QAction* m_gotoHomepageAction = nullptr;
    QAction* m_viewSourceAction = nullptr;
    QAction* m_aboutAction = nullptr;
    QAction* m_checkForUpdatesAction = nullptr;
    QAction* m_reportIssuesAction = nullptr;
    QAction* m_seeContributorsAction = nullptr;
    QAction* m_seeSupportersAction = nullptr;
    QAction* m_seeAcknowlegementsAction = nullptr;
    QAction* m_seeReferenceGuideAction = nullptr;

    ToolbarButton* m_xLockButton = nullptr;
    ToolbarButton* m_yLockButton = nullptr;
    ToolbarButton* m_zLockButton = nullptr;
    ToolbarButton* m_radiusLockButton = nullptr;

    QMetaObject::Connection m_partListDockerVisibleSwitchConnection;

    MeshPreviewImagesGenerator* m_componentPreviewImagesGenerator = nullptr;
    bool m_isComponentPreviewImagesObsolete = false;

    std::unique_ptr<ComponentPreviewImagesDecorator> m_componentPreviewImagesDecorator;
    bool m_isComponentPreviewImageDecorationsObsolete = false;

    PartManageWidget* m_partManageWidget = nullptr;

    SpinnableToolbarIcon* m_inprogressIndicator = nullptr;

    std::map<QKeySequence, QShortcut*> m_shortcutMap;
};

#endif
