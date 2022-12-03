#include "bone_generator.h"
#include <QDebug>
#include <QElapsedTimer>
#include <dust3d/mesh/smooth_normal.h>
#include <dust3d/mesh/trim_vertices.h>

BoneGenerator::BoneGenerator(std::unique_ptr<dust3d::Object> object, std::unique_ptr<dust3d::Snapshot> snapshot)
    : m_object(std::move(object))
    , m_snapshot(std::move(snapshot))
{
}

std::map<dust3d::Uuid, std::unique_ptr<ModelMesh>>* BoneGenerator::takeBonePreviewMeshes()
{
    return m_bonePreviewMeshes.release();
}

void BoneGenerator::process()
{
    QElapsedTimer countTimeConsumed;
    countTimeConsumed.start();

    setVertices(m_object->vertices);
    setTriangles(m_object->triangles);
    setPositionToNodeMap(m_object->positionToNodeIdMap);

    for (const auto& it : m_object->nodeMap) {
        Node node;
        node.position = it.second.origin;
        addNode(it.first, node);
    }

    for (const auto& it : m_snapshot->boneIdList) {
        Bone bone;
        addBone(dust3d::Uuid(it), bone);
    }

    for (const auto& it : m_snapshot->nodes) {
        NodeBinding nodeBinding;
        for (const auto& boneIdString : dust3d::String::split(dust3d::String::valueOrEmpty(it.second, "boneIdList"), ',')) {
            if (boneIdString.empty())
                continue;
            nodeBinding.boneIds.insert(dust3d::Uuid(boneIdString));
        }
        addNodeBinding(dust3d::Uuid(it.first), nodeBinding);
    }

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

    qDebug() << "The bone generation took" << countTimeConsumed.elapsed() << "milliseconds";

    emit finished();
}
