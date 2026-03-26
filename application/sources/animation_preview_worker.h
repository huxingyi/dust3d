#ifndef DUST3D_APPLICATION_ANIMATION_PREVIEW_WORKER_H_
#define DUST3D_APPLICATION_ANIMATION_PREVIEW_WORKER_H_

#include "bone_structure.h"
#include "model_mesh.h"
#include "rig_skeleton_mesh_generator.h"
#include <QObject>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/rig/rig_generator.h>
#include <map>
#include <vector>

class AnimationPreviewWorker : public QObject {
    Q_OBJECT
public:
    void setParameters(const RigStructure& rigStructure, int frameCount = 30, float durationSeconds = 1.0f,
        const dust3d::AnimationParams& parameters = dust3d::AnimationParams())
    {
        m_rigStructure = rigStructure;
        m_frameCount = frameCount;
        m_durationSeconds = durationSeconds;
        m_animationParameters = parameters;
    }

    std::vector<ModelMesh> takePreviewMeshes()
    {
        return std::move(m_previewMeshes);
    }

signals:
    void finished();

public slots:
    void process();

private:
    RigStructure m_rigStructure;
    int m_frameCount = 30;
    float m_durationSeconds = 1.0f;
    dust3d::AnimationParams m_animationParameters;
    std::vector<ModelMesh> m_previewMeshes;
};

#endif
