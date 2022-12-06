#ifndef DUST3D_APPLICATION_BONE_GENERATOR_H_
#define DUST3D_APPLICATION_BONE_GENERATOR_H_

#include "model_mesh.h"
#include <QObject>
#include <dust3d/base/snapshot.h>
#include <dust3d/rig/bone_generator.h>
#include <map>
#include <memory>

class BoneGenerator : public QObject, public dust3d::BoneGenerator {
    Q_OBJECT
public:
    BoneGenerator(std::unique_ptr<dust3d::Object> object, std::unique_ptr<dust3d::Snapshot> snapshot);
    std::map<dust3d::Uuid, std::unique_ptr<ModelMesh>>* takeBonePreviewMeshes();
    std::unique_ptr<ModelMesh> takeBodyPreviewMesh();
public slots:
    void process();
signals:
    void finished();

private:
    std::unique_ptr<std::map<dust3d::Uuid, std::unique_ptr<ModelMesh>>> m_bonePreviewMeshes;
    std::unique_ptr<dust3d::Object> m_object;
    std::unique_ptr<dust3d::Snapshot> m_snapshot;
    std::unique_ptr<ModelMesh> m_bodyPreviewMesh;
};

#endif
