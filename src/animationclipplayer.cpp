#include "animationclipplayer.h"

AnimationClipPlayer::~AnimationClipPlayer()
{
    clear();
}

void AnimationClipPlayer::setSpeedMode(SpeedMode speedMode)
{
    m_speedMode = speedMode;
}

void AnimationClipPlayer::updateFrameMeshes(std::vector<std::pair<float, Model *>> &frameMeshes)
{
    clear();
    
    m_frameMeshes = frameMeshes;
    frameMeshes.clear();
    
    m_currentPlayIndex = 0;
    m_countForFrame.restart();
    
    if (!m_frameMeshes.empty())
        m_timerForFrame.singleShot(0, this, &AnimationClipPlayer::frameReadyToShow);
}

void AnimationClipPlayer::clear()
{
    freeFrames();
    delete m_lastFrameMesh;
    m_lastFrameMesh = nullptr;
}

void AnimationClipPlayer::freeFrames()
{
    for (auto &it: m_frameMeshes) {
        delete it.second;
    }
    m_frameMeshes.clear();
}

int AnimationClipPlayer::getFrameDurationMillis(int frame)
{
    int millis = m_frameMeshes[frame].first * 1000;
    if (SpeedMode::Slow == m_speedMode) {
        millis *= 2;
    } else if (SpeedMode::Fast == m_speedMode) {
        millis /= 2;
    }
    return millis;
}

Model *AnimationClipPlayer::takeFrameMesh()
{
    if (m_currentPlayIndex >= (int)m_frameMeshes.size()) {
        if (nullptr != m_lastFrameMesh)
            return new Model(*m_lastFrameMesh);
        return nullptr;
    }
    int millis = getFrameDurationMillis(m_currentPlayIndex) - m_countForFrame.elapsed();
    if (millis > 0) {
        m_timerForFrame.singleShot(millis, this, &AnimationClipPlayer::frameReadyToShow);
        if (nullptr != m_lastFrameMesh)
            return new Model(*m_lastFrameMesh);
        return nullptr;
    }
    m_currentPlayIndex = (m_currentPlayIndex + 1) % m_frameMeshes.size();
    m_countForFrame.restart();

    Model *mesh = new Model(*m_frameMeshes[m_currentPlayIndex].second);
    m_timerForFrame.singleShot(getFrameDurationMillis(m_currentPlayIndex), this, &AnimationClipPlayer::frameReadyToShow);
    delete m_lastFrameMesh;
    m_lastFrameMesh = new Model(*mesh);
    return mesh;
}
