#ifndef DUST3D_APPLICATION_MESH_GENERATOR_H_
#define DUST3D_APPLICATION_MESH_GENERATOR_H_

#include <memory>
#include <QObject>
#include <QImage>
#include <dust3d/mesh/mesh_generator.h>
#include "model_mesh.h"
#include "monochrome_mesh.h"

class MeshGenerator : public QObject, public dust3d::MeshGenerator
{
    Q_OBJECT
public:
    MeshGenerator(dust3d::Snapshot *snapshot);
    ~MeshGenerator();
    ModelMesh *takeResultMesh();
    std::map<dust3d::Uuid, std::unique_ptr<ModelMesh>> *takeComponentPreviewMeshes();
    MonochromeMesh *takeWireframeMesh();
public slots:
    void process();
signals:
    void finished();
private:
    std::unique_ptr<ModelMesh> m_resultMesh;
    std::unique_ptr<std::map<dust3d::Uuid, std::unique_ptr<ModelMesh>>> m_componentPreviewMeshes;
    std::unique_ptr<MonochromeMesh> m_wireframeMesh;
};

#endif
