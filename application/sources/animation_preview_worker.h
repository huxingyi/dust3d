#ifndef DUST3D_APPLICATION_ANIMATION_PREVIEW_WORKER_H_
#define DUST3D_APPLICATION_ANIMATION_PREVIEW_WORKER_H_

#include "bone_structure.h"
#include "model_mesh.h"
#include "rig_skeleton_mesh_generator.h"
#include <QObject>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/sound_generator.h>
#include <dust3d/rig/rig_generator.h>
#include <map>
#include <memory>
#include <vector>

class AnimationPreviewWorker : public QObject {
    Q_OBJECT
public:
    void setParameters(const RigStructure& rigStructure, const std::string& animationType,
        const dust3d::AnimationParams& parameters = dust3d::AnimationParams())
    {
        m_rigStructure = rigStructure;
        m_animationType = animationType;
        m_animationParameters = parameters;
    }

    void setRigObject(std::unique_ptr<dust3d::Object> rigObject)
    {
        m_rigObject = std::move(rigObject);
    }

    void setHideBones(bool hideBones)
    {
        m_hideBones = hideBones;
    }

    void setHideParts(bool hideParts)
    {
        m_hideParts = hideParts;
    }

    void setSelectedBoneName(const QString& boneName)
    {
        m_selectedBoneName = boneName;
    }

    void setSurfaceMaterial(dust3d::SurfaceMaterial material)
    {
        m_surfaceMaterial = material;
    }

    void setSoundEnabled(bool enabled)
    {
        m_soundEnabled = enabled;
    }

    std::vector<ModelMesh> takePreviewMeshes()
    {
        return std::move(m_previewMeshes);
    }

    dust3d::AnimationSoundData takeSoundData()
    {
        return std::move(m_soundData);
    }

    float movementSpeed() const { return m_movementSpeed; }
    float movementDirectionX() const { return m_movementDirectionX; }
    float movementDirectionZ() const { return m_movementDirectionZ; }
    float durationSeconds() const { return m_durationSeconds; }

signals:
    void finished();

public slots:
    void process();

private:
    dust3d::Color calculateBoneWeightColor(float weight);
    RigStructure m_rigStructure;
    std::string m_animationType;
    dust3d::AnimationParams m_animationParameters;
    std::unique_ptr<dust3d::Object> m_rigObject;
    std::vector<ModelMesh> m_previewMeshes;
    bool m_hideBones = false;
    bool m_hideParts = false;
    bool m_soundEnabled = false;
    dust3d::SurfaceMaterial m_surfaceMaterial = dust3d::SurfaceMaterial::Stone;
    QString m_selectedBoneName;
    dust3d::AnimationSoundData m_soundData;
    float m_movementSpeed = 0.0f;
    float m_movementDirectionX = 0.0f;
    float m_movementDirectionZ = 0.0f;
    float m_durationSeconds = 0.0f;
};

#endif
