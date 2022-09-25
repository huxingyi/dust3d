#include <QElapsedTimer>
#include <QDebug>
#include <dust3d/mesh/hud_manager.h>
#include "mesh_generator.h"
#include "cut_face_preview.h"

MeshGenerator::MeshGenerator(dust3d::Snapshot *snapshot) :
    dust3d::MeshGenerator(snapshot)
{
}

MeshGenerator::~MeshGenerator()
{
    for (auto &it: m_partPreviewImages)
        delete it.second;
    for (auto &it: m_partPreviewMeshes)
        delete it.second;
    delete m_resultMesh;
}

ModelMesh *MeshGenerator::takePartPreviewMesh(const dust3d::Uuid &partId)
{
    ModelMesh *resultMesh = m_partPreviewMeshes[partId];
    m_partPreviewMeshes[partId] = nullptr;
    return resultMesh;
}

QImage *MeshGenerator::takePartPreviewImage(const dust3d::Uuid &partId)
{
    QImage *image = m_partPreviewImages[partId];
    m_partPreviewImages[partId] = nullptr;
    return image;
}

MonochromeMesh *MeshGenerator::takeWireframeMesh()
{
    return m_wireframeMesh.release();
}

MonochromeMesh *MeshGenerator::takeHudMesh()
{
    return m_hudMesh.release();
}

void MeshGenerator::process()
{
    QElapsedTimer countTimeConsumed;
    countTimeConsumed.start();
    
    generate();
    
    if (nullptr != m_object)
        m_resultMesh = new ModelMesh(*m_object);
    
    for (const auto &partId: m_generatedPreviewImagePartIds) {
        auto it = m_generatedPartPreviews.find(partId);
        if (it == m_generatedPartPreviews.end())
            continue;
        m_partPreviewImages[partId] = buildCutFaceTemplatePreviewImage(it->second.cutTemplate);
    }
    for (const auto &partId: m_generatedPreviewPartIds) {
        auto it = m_generatedPartPreviews.find(partId);
        if (it == m_generatedPartPreviews.end())
            continue;
        m_partPreviewMeshes[partId] = new ModelMesh(it->second.vertices,
            it->second.triangles,
            it->second.vertexNormals,
            it->second.color,
            it->second.metalness,
            it->second.roughness);
    }

    if (nullptr != m_object)
        m_wireframeMesh = std::make_unique<MonochromeMesh>(*m_object);
    
    if (nullptr != m_object) {
        dust3d::HudManager hudManager;
        hudManager.addFromObject(*m_object);
        hudManager.generate();
        int lineVertexCount = 0;
        int triangleVertexCount = 0;
        auto vertices = hudManager.takeVertices(&lineVertexCount, &triangleVertexCount);
        if (vertices)
            m_hudMesh = std::make_unique<MonochromeMesh>(std::move(*vertices), lineVertexCount, triangleVertexCount);
    }
    
    qDebug() << "The mesh generation took" << countTimeConsumed.elapsed() << "milliseconds";
    
    emit finished();
}

ModelMesh *MeshGenerator::takeResultMesh()
{
    ModelMesh *resultMesh = m_resultMesh;
    m_resultMesh = nullptr;
    return resultMesh;
}

