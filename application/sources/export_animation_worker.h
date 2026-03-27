#ifndef DUST3D_APPLICATION_EXPORT_ANIMATION_WORKER_H_
#define DUST3D_APPLICATION_EXPORT_ANIMATION_WORKER_H_

#include "bone_structure.h"
#include "document.h"
#include <QObject>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/rig/rig_generator.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

class ExportAnimationWorker : public QObject {
    Q_OBJECT
public:
    void setParameters(const RigStructure& rigStructure,
        const std::vector<Document::Animation>& animations,
        int frameCount = 30,
        float durationSeconds = 1.0f)
    {
        m_rigStructure = rigStructure;
        m_animations = animations;
        m_frameCount = frameCount;
        m_durationSeconds = durationSeconds;
    }

    const std::map<std::string, dust3d::Matrix4x4>& inverseBindMatrices() const
    {
        return m_inverseBindMatrices;
    }

    std::vector<dust3d::RigAnimationClip> takeAnimationClips()
    {
        return std::move(m_animationClips);
    }

    bool isSuccessful() const { return m_successful; }

signals:
    void progress(int current, int total);
    void finished();

public slots:
    void process();

private:
    RigStructure m_rigStructure;
    std::vector<Document::Animation> m_animations;
    int m_frameCount = 30;
    float m_durationSeconds = 1.0f;
    std::map<std::string, dust3d::Matrix4x4> m_inverseBindMatrices;
    std::vector<dust3d::RigAnimationClip> m_animationClips;
    bool m_successful = false;
};

#endif
