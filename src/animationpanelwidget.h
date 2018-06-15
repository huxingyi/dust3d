#ifndef ANIMATION_PANEL_WIDGET_H
#define ANIMATION_PANEL_WIDGET_H
#include <QWidget>
#include <QTime>
#include <QTimer>
#include "skeletondocument.h"
#include "animationclipgenerator.h"
#include "animationclipplayer.h"

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
    void reset();
private:
    SkeletonDocument *m_document;
    AnimationClipGenerator *m_animationClipGenerator;
private:
    AnimationClipPlayer m_clipPlayer;
    QString m_nextMotionName;
    QString m_lastMotionName;
};

#endif
