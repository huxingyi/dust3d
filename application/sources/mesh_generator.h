#ifndef DUST3D_APPLICATION_MESH_GENERATOR_H_
#define DUST3D_APPLICATION_MESH_GENERATOR_H_

#include "model_mesh.h"
#include "monochrome_mesh.h"
#include <QByteArray>
#include <QImage>
#include <QObject>
#include <dust3d/mesh/mesh_generator.h>
#include <memory>

class MeshGenerator : public QObject, public dust3d::MeshGenerator {
    Q_OBJECT
public:
    MeshGenerator(dust3d::Snapshot* snapshot);
    ~MeshGenerator();
    ModelMesh* takeResultMesh();
    std::map<dust3d::Uuid, std::unique_ptr<ModelMesh>>* takeComponentPreviewMeshes();
    std::map<dust3d::Uuid, std::unique_ptr<QImage>>* takeComponentPreviewImages();
    MonochromeMesh* takeWireframeMesh();

    struct PendingGlbData {
        QByteArray data;
        std::string componentIdString;
    };
    void addPendingGlbData(const std::string& glbIdString, QByteArray data, const std::string& componentIdString);

public slots:
    void process();
signals:
    void finished();
    void importedModelTextureReady(dust3d::Uuid componentId, dust3d::Uuid textureId);

private:
    void parseImportedModelData();
    std::unique_ptr<ModelMesh> m_resultMesh;
    std::unique_ptr<std::map<dust3d::Uuid, std::unique_ptr<ModelMesh>>> m_componentPreviewMeshes;
    std::unique_ptr<std::map<dust3d::Uuid, std::unique_ptr<QImage>>> m_componentPreviewImages;
    std::unique_ptr<MonochromeMesh> m_wireframeMesh;
    std::map<std::string, PendingGlbData> m_pendingGlbData;
    static std::map<std::string, std::pair<dust3d::MeshGenerator::ImportedModelData, dust3d::Uuid>> s_glbCache;
};

#endif
