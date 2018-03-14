#ifndef SKELETON_WIDGET_H
#define SKELETON_WIDGET_H
#include <QWidget>
#include <QString>
#include <QVBoxLayout>
#include <QPushButton>
#include "modelingwidget.h"
#include "skeletoneditwidget.h"
#include "skeletontomesh.h"
#include "turnaroundloader.h"

class SkeletonWidget : public QWidget
{
    Q_OBJECT
public:
    SkeletonWidget(QWidget *parent=0);
public slots:
    void skeletonChanged();
    void meshReady();
    void turnaroundChanged();
    void turnaroundImageReady();
    void changeTurnaround();
    void saveClip();
    void showModelingWidgetAtCorner();
private:
    ModelingWidget *m_modelingWidget;
    SkeletonEditWidget *m_skeletonEditWidget;
    SkeletonToMesh *m_skeletonToMesh;
    bool m_skeletonDirty;
    TurnaroundLoader *m_turnaroundLoader;
    bool m_turnaroundDirty;
    QString m_turnaroundFilename;
};

#endif
