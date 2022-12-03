#include "bone_generator.h"
#include <dust3d/mesh/smooth_normal.h>
#include <dust3d/mesh/trim_vertices.h>

void BoneGenerator::process()
{
    generate();

    m_bonePreviewMeshes = std::make_unique<std::map<dust3d::Uuid, std::unique_ptr<ModelMesh>>>();
    for (auto& it : bonePreviews()) {
        dust3d::trimVertices(&it.second.vertices, true);
        for (auto& it : it.second.vertices) {
            it *= 2.0;
        }
        std::vector<dust3d::Vector3> previewTriangleNormals;
        previewTriangleNormals.reserve(it.second.triangles.size());
        for (const auto& face : it.second.triangles) {
            previewTriangleNormals.emplace_back(dust3d::Vector3::normal(
                it.second.vertices[face[0]],
                it.second.vertices[face[1]],
                it.second.vertices[face[2]]));
        }
        std::vector<std::vector<dust3d::Vector3>> previewTriangleVertexNormals;
        dust3d::smoothNormal(it.second.vertices,
            it.second.triangles,
            previewTriangleNormals,
            0,
            &previewTriangleVertexNormals);
        (*m_bonePreviewMeshes)[it.first] = std::make_unique<ModelMesh>(it.second.vertices,
            it.second.triangles,
            previewTriangleVertexNormals);
    }

    emit finished();
}
