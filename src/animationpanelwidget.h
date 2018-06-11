#ifndef ANIMATION_PANEL_WIDGET_H
#define ANIMATION_PANEL_WIDGET_H
#include <QWidget>
#include <QTime>
#include <QTimer>
#include "skeletondocument.h"
#include "animationclipgenerator.h"

class AnimationPanelWidget : public QWidget
{
    Q_OBJECT
signals:
    void frameReadyToShow();
    void panelClosed();
public:
    AnimationPanelWidget(SkeletonDocument *document, QWidget *parent=nullptr);
    ~AnimationPanelWidget();
    MeshLoader *takeFrameMesh();
protected:
    void hideEvent(QHideEvent *event);
public slots:
    void generateClip(QString motionName);
    void clipReady();
    void sourceMeshChanged();
private:
    SkeletonDocument *m_document;
    AnimationClipGenerator *m_animationClipGenerator;
    MeshLoader *m_lastFrameMesh;
    bool m_sourceMeshReady;
private:
    std::vector<std::pair<int, MeshLoader *>> m_frameMeshes;
    QTime m_countForFrame;
    QString m_nextMotionName;
    QString m_lastMotionName;
};

#endif
