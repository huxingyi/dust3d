#include <QElapsedTimer>
#include <QDebug>
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

Model *MeshGenerator::takePartPreviewMesh(const dust3d::Uuid &partId)
{
    Model *resultMesh = m_partPreviewMeshes[partId];
    m_partPreviewMeshes[partId] = nullptr;
    return resultMesh;
}

QImage *MeshGenerator::takePartPreviewImage(const dust3d::Uuid &partId)
{
    QImage *image = m_partPreviewImages[partId];
    m_partPreviewImages[partId] = nullptr;
    return image;
}

void MeshGenerator::process()
{
    QElapsedTimer countTimeConsumed;
    countTimeConsumed.start();
    
    generate();
    
    if (nullptr != m_object)
        m_resultMesh = new Model(*m_object);
    
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
        m_partPreviewMeshes[partId] = new Model(it->second.vertices,
            it->second.triangles,
            it->second.vertexNormals,
            it->second.color,
            it->second.metalness,
            it->second.roughness);
    }
    
    qDebug() << "The mesh generation took" << countTimeConsumed.elapsed() << "milliseconds";
    
    emit finished();
}

Model *MeshGenerator::takeResultMesh()
{
    Model *resultMesh = m_resultMesh;
    m_resultMesh = nullptr;
    return resultMesh;
}

