#ifndef DUST3D_APPLICATION_BONE_GENERATOR_H_
#define DUST3D_APPLICATION_BONE_GENERATOR_H_

#include "model_mesh.h"
#include <QObject>
#include <dust3d/rig/bone_generator.h>
#include <memory>

class BoneGenerator : public QObject, public dust3d::BoneGenerator {
    Q_OBJECT
public slots:
    void process();
signals:
    void finished();

private:
    std::unique_ptr<std::map<dust3d::Uuid, std::unique_ptr<ModelMesh>>> m_bonePreviewMeshes;
};

#endif
