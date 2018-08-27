#ifndef SKELETON_DOCUMENT_WINDOW_H
#define SKELETON_DOCUMENT_WINDOW_H
#include <QMainWindow>
#include <QShowEvent>
#include <QPushButton>
#include <QString>
#include <QMenu>
#include <QAction>
#include <QTextBrowser>
#include "skeletondocument.h"
#include "modelwidget.h"
#include "exportpreviewwidget.h"

class SkeletonGraphicsWidget;

class SkeletonDocumentWindow : public QMainWindow
{
    Q_OBJECT
signals:
    void initialized();
public:
    SkeletonDocumentWindow();
    ~SkeletonDocumentWindow();
    static SkeletonDocumentWindow *createDocumentWindow();
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
    void exportModelResult();
    void exportGltfResult();
    void showExportPreview();
    void newWindow();
    void newDocument();
    void saveAs();
    void saveAll();
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
private:
    void initLockButton(QPushButton *button);
    void setCurrentFilename(const QString &filename);
    void updateTitle();
private:
    SkeletonDocument *m_document;
    bool m_firstShow;
    bool m_documentSaved;
    ExportPreviewWidget *m_exportPreviewWidget;
private:
    QString m_currentFilename;
    
    ModelWidget *m_modelRenderWidget;
    //ModelWidget *m_skeletonRenderWidget;
    SkeletonGraphicsWidget *m_graphicsWidget;
    
    QMenu *m_fileMenu;
    QAction *m_newWindowAction;
    QAction *m_newDocumentAction;
    QAction *m_openAction;
    QAction *m_saveAction;
    QAction *m_saveAsAction;
    QAction *m_saveAllAction;
    QMenu *m_exportMenu;
    QAction *m_changeTurnaroundAction;
    
    QAction *m_exportModelAction;
    QAction *m_exportSkeletonAction;
    
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
    
    QMenu *m_viewMenu;
    QAction *m_resetModelWidgetPosAction;
    QAction *m_showPartsListAction;
    QAction *m_showDebugDialogAction;
    QAction *m_toggleWireframeAction;
    QAction *m_showMotionsListAction;
    
    QMenu *m_helpMenu;
    QAction *m_viewSourceAction;
    QAction *m_aboutAction;
    QAction *m_reportIssuesAction;
    QAction *m_seeContributorsAction;
    QAction *m_seeAcknowlegementsAction;
    QAction *m_seeReferenceGuideAction;

    QPushButton *m_xlockButton;
    QPushButton *m_ylockButton;
    QPushButton *m_zlockButton;
    
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

