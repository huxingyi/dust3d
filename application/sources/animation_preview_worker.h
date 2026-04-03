#ifndef DUST3D_APPLICATION_ANIMATION_PREVIEW_WORKER_H_
#define DUST3D_APPLICATION_ANIMATION_PREVIEW_WORKER_H_

#include "bone_structure.h"
#include "model_mesh.h"
#include "rig_skeleton_mesh_generator.h"
#include <QObject>
#include <dust3d/animation/animation_generator.h>
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

    std::vector<ModelMesh> takePreviewMeshes()
    {
        return std::move(m_previewMeshes);
    }

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
    QString m_selectedBoneName;
};

#endif
