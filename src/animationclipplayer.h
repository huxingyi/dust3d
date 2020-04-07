#ifndef DUST3D_ANIMATION_PLAYER_H
#define DUST3D_ANIMATION_PLAYER_H
#include <QObject>
#include <QTimer>
#include <QTime>
#include "model.h"

class AnimationClipPlayer : public QObject
{
    Q_OBJECT
 
signals:
    void frameReadyToShow();
    
public:
    enum class SpeedMode
    {
        Slow,
        Normal,
        Fast
    };
    
    ~AnimationClipPlayer();
    Model *takeFrameMesh();
    void updateFrameMeshes(std::vector<std::pair<float, Model *>> &frameMeshes);
    void clear();
    
public slots:
    void setSpeedMode(SpeedMode speedMode);
    
private:
    void freeFrames();
    int getFrameDurationMillis(int frame);

    Model *m_lastFrameMesh = nullptr;
    int m_currentPlayIndex = 0;
    std::vector<std::pair<float, Model *>> m_frameMeshes;
    QTime m_countForFrame;
    QTimer m_timerForFrame;
    SpeedMode m_speedMode = SpeedMode::Normal;
};

#endif
