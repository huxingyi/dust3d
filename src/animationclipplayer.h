#ifndef ANIMATION_PLAYER_H
#define ANIMATION_PLAYER_H
#include <QObject>
#include <QTimer>
#include <QTime>
#include "meshloader.h"

class AnimationClipPlayer : public QObject
{
    Q_OBJECT
signals:
    void frameReadyToShow();
public:
    ~AnimationClipPlayer();
    MeshLoader *takeFrameMesh();
    void updateFrameMeshes(std::vector<std::pair<float, MeshLoader *>> &frameMeshes);
    void clear();
private:
    void freeFrames();
private:
    MeshLoader *m_lastFrameMesh = nullptr;
    int m_currentPlayIndex = 0;
private:
    std::vector<std::pair<float, MeshLoader *>> m_frameMeshes;
    QTime m_countForFrame;
    QTimer m_timerForFrame;
};

#endif
