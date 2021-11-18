#ifndef DUST3D_APPLICATION_MESH_GENERATOR_H_
#define DUST3D_APPLICATION_MESH_GENERATOR_H_

#include <QObject>
#include <QImage>
#include <dust3d/mesh/mesh_generator.h>
#include "model.h"

class MeshGenerator : public QObject, public dust3d::MeshGenerator
{
    Q_OBJECT
public:
    MeshGenerator(dust3d::Snapshot *snapshot);
    ~MeshGenerator();
    Model *takeResultMesh();
    Model *takePartPreviewMesh(const dust3d::Uuid &partId);
    QImage *takePartPreviewImage(const dust3d::Uuid &partId);
public slots:
    void process();
signals:
    void finished();
private:
    Model *m_resultMesh = nullptr;
    std::map<dust3d::Uuid, Model *> m_partPreviewMeshes;
    std::map<dust3d::Uuid, QImage *> m_partPreviewImages;
};

#endif
