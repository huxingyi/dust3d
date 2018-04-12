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
    static void showAbout();
protected:
    void showEvent(QShowEvent *event);
    void closeEvent(QCloseEvent *event);
public slots:
    void changeTurnaround();
    void save();
    void saveTo(const QString &saveAsFilename);
    void open();
    void exportResult();
    void newWindow();
    void newDocument();
    void saveAs();
    void saveAll();
    void viewSource();
    void about();
    void reportIssues();
    void seeAcknowlegements();
    void documentChanged();
private:
    void initButton(QPushButton *button);
    void setCurrentFilename(const QString &filename);
    void updateTitle();
private:
    SkeletonDocument *m_document;
    bool m_firstShow;
    ModelWidget *m_modelWidget;
    SkeletonGraphicsWidget *m_graphicsWidget;
    QString m_currentFilename;
    bool m_documentSaved;
    
    QMenu *m_fileMenu;
    QAction *m_newWindowAction;
    QAction *m_newDocumentAction;
    QAction *m_openAction;
    QAction *m_saveAction;
    QAction *m_saveAsAction;
    QAction *m_saveAllAction;
    QAction *m_exportAction;
    QAction *m_changeTurnaroundAction;
    
    QMenu *m_editMenu;
    QAction *m_addAction;
    QAction *m_undoAction;
    QAction *m_redoAction;
    QAction *m_deleteAction;
    QAction *m_breakAction;
    QAction *m_cutAction;
    QAction *m_copyAction;
    QAction *m_pasteAction;
    QAction *m_flipHorizontallyAction;
    QAction *m_flipVerticallyAction;
    QAction *m_selectAllAction;
    QAction *m_selectPartAllAction;
    QAction *m_unselectAllAction;
    
    QMenu *m_viewMenu;
    QAction *m_resetModelWidgetPosAction;
    QAction *m_showDebugDialogAction;
    
    QMenu *m_helpMenu;
    QAction *m_viewSourceAction;
    QAction *m_aboutAction;
    QAction *m_reportIssuesAction;
    QAction *m_seeAcknowlegementsAction;
};

#endif

