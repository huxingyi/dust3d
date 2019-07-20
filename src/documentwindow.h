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
#include "document.h"
#include "modelwidget.h"
#include "exportpreviewwidget.h"
#include "rigwidget.h"
#include "bonemark.h"
#include "posemanagewidget.h"
#include "preferenceswidget.h"

class SkeletonGraphicsWidget;

class DocumentWindow : public QMainWindow
{
    Q_OBJECT
signals:
    void initialized();
    void uninialized();
public:
    DocumentWindow();
    ~DocumentWindow();
    Document *document();
    static DocumentWindow *createDocumentWindow();
    static const std::map<DocumentWindow *, QUuid> &documentWindows();
    static void showAcknowlegements();
    static void showContributors();
    static void showAbout();
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
    void exportObjResult();
    void exportGlbResult();
    void exportFbxResult();
    void showExportPreview();
    void newWindow();
    void newDocument();
    void saveAs();
    void saveAll();
    void gotoHomepage();
    void viewSource();
    void about();
    void reportIssues();
    void seeAcknowlegements();
    void seeContributors();
    void seeReferenceGuide();
    void documentChanged();
    void updateXlockButtonState();
    void updateYlockButtonState();
    void updateZlockButtonState();
    void updateRadiusLockButtonState();
    void updateRigWeightRenderWidget();
    void registerDialog(QWidget *widget);
    void unregisterDialog(QWidget *widget);
    void showPreferences();
    void showCutFaceSettingPopup(const QPoint &globalPos, std::set<QUuid> nodeIds);
private:
    void initLockButton(QPushButton *button);
    void setCurrentFilename(const QString &filename);
    void updateTitle();
private:
    Document *m_document;
    bool m_firstShow;
    bool m_documentSaved;
    ExportPreviewWidget *m_exportPreviewWidget;
    PreferencesWidget *m_preferencesWidget;
    std::vector<QWidget *> m_dialogs;
    bool m_isLastMeshGenerationSucceed;
    quint64 m_currentUpdatedMeshId;
private:
    QString m_currentFilename;
    
    ModelWidget *m_modelRenderWidget;
    SkeletonGraphicsWidget *m_graphicsWidget;
    RigWidget *m_rigWidget;
    
    QMenu *m_fileMenu;
    QAction *m_newWindowAction;
    QAction *m_newDocumentAction;
    QAction *m_openAction;
    QMenu *m_openExampleMenu;
    //QAction *m_openExampleAction;
    QAction *m_saveAction;
    QAction *m_saveAsAction;
    QAction *m_saveAllAction;
    QAction *m_showPreferencesAction;
    QMenu *m_exportMenu;
    QAction *m_changeTurnaroundAction;
    
    QAction *m_exportAsObjAction;
    QAction *m_exportAsObjPlusMaterialsAction;
    QAction *m_exportAction;
    QAction *m_exportRenderedAsImageAction;
    
    QMenu *m_editMenu;
    QAction *m_addAction;
    QAction *m_undoAction;
    QAction *m_redoAction;
    QAction *m_deleteAction;
    QAction *m_breakAction;
    QAction *m_connectAction;
    QAction *m_cutAction;
    QAction *m_copyAction;
    QAction *m_pasteAction;
    QAction *m_flipHorizontallyAction;
    QAction *m_flipVerticallyAction;
    QAction *m_rotateClockwiseAction;
    QAction *m_rotateCounterclockwiseAction;
    QAction *m_switchXzAction;
    QAction *m_setCutFaceAction;
    QAction *m_clearCutFaceAction;
    
    QMenu *m_alignToMenu;
    QAction *m_alignToGlobalCenterAction;
    QAction *m_alignToGlobalVerticalCenterAction;
    QAction *m_alignToGlobalHorizontalCenterAction;
    QAction *m_alignToLocalCenterAction;
    QAction *m_alignToLocalVerticalCenterAction;
    QAction *m_alignToLocalHorizontalCenterAction;
    
    QAction *m_selectAllAction;
    QAction *m_selectPartAllAction;
    QAction *m_unselectAllAction;
    
    QMenu *m_markAsMenu;
    QAction *m_markAsNoneAction;
    QAction *m_markAsActions[(int)BoneMark::Count - 1];
    
    QMenu *m_viewMenu;
    QAction *m_resetModelWidgetPosAction;
    QAction *m_toggleWireframeAction;
    QAction *m_showMotionsListAction;
    
    QMenu *m_windowMenu;
    QAction *m_showPartsListAction;
    QAction *m_showDebugDialogAction;
    QAction *m_showMaterialsAction;
    QAction *m_showRigAction;
    QAction *m_showPosesAction;
    QAction *m_showMotionsAction;
    QAction *m_showScriptAction;
    
    QMenu *m_helpMenu;
    QAction *m_gotoHomepageAction;
    QAction *m_viewSourceAction;
    QAction *m_aboutAction;
    QAction *m_reportIssuesAction;
    QAction *m_seeContributorsAction;
    QAction *m_seeAcknowlegementsAction;
    QAction *m_seeReferenceGuideAction;

    QPushButton *m_xlockButton;
    QPushButton *m_ylockButton;
    QPushButton *m_zlockButton;
    QPushButton *m_radiusLockButton;
    
    QMetaObject::Connection m_partListDockerVisibleSwitchConnection;
public:
    static int m_modelRenderWidgetInitialX;
    static int m_modelRenderWidgetInitialY;
    static int m_modelRenderWidgetInitialSize;
    static int m_skeletonRenderWidgetInitialX;
    static int m_skeletonRenderWidgetInitialY;
    static int m_skeletonRenderWidgetInitialSize;
};

#endif

