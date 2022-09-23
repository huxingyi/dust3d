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
    ModelMesh *takePartPreviewMesh(const dust3d::Uuid &partId);
    QImage *takePartPreviewImage(const dust3d::Uuid &partId);
    MonochromeMesh *takeWireframeMesh();
public slots:
    void process();
signals:
    void finished();
private:
    ModelMesh *m_resultMesh = nullptr;
    std::map<dust3d::Uuid, ModelMesh *> m_partPreviewMeshes;
    std::map<dust3d::Uuid, QImage *> m_partPreviewImages;
    std::unique_ptr<MonochromeMesh> m_wireframeMesh;
};

#endif
