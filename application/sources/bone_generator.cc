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

std::unique_ptr<ModelMesh> BoneGenerator::takeBodyPreviewMesh()
{
    return std::move(m_bodyPreviewMesh);
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
        std::vector<std::tuple<dust3d::Color, float /*metalness*/, float /*roughness*/>> vertexProperties(it.second.vertexColors.size());
        for (size_t i = 0; i < vertexProperties.size(); ++i) {
            vertexProperties[i] = std::make_tuple(it.second.vertexColors[i],
                (float)0.0, (float)1.0);
        }
        (*m_bonePreviewMeshes)[it.first] = std::make_unique<ModelMesh>(it.second.vertices,
            it.second.triangles,
            previewTriangleVertexNormals,
            dust3d::Color::createWhite(),
            (float)0.0,
            (float)1.0,
            &vertexProperties);
    }

    {
        const auto& preview = bodyPreview();
        std::vector<dust3d::Vector3> previewTriangleNormals;
        previewTriangleNormals.reserve(preview.triangles.size());
        for (const auto& face : preview.triangles) {
            previewTriangleNormals.emplace_back(dust3d::Vector3::normal(
                preview.vertices[face[0]],
                preview.vertices[face[1]],
                preview.vertices[face[2]]));
        }
        std::vector<std::vector<dust3d::Vector3>> previewTriangleVertexNormals;
        dust3d::smoothNormal(preview.vertices,
            preview.triangles,
            previewTriangleNormals,
            0,
            &previewTriangleVertexNormals);
        std::vector<std::tuple<dust3d::Color, float /*metalness*/, float /*roughness*/>> vertexProperties(preview.vertexColors.size());
        for (size_t i = 0; i < vertexProperties.size(); ++i) {
            vertexProperties[i] = std::make_tuple(preview.vertexColors[i],
                (float)0.0, (float)1.0);
        }
        m_bodyPreviewMesh = std::make_unique<ModelMesh>(preview.vertices,
            preview.triangles,
            previewTriangleVertexNormals,
            dust3d::Color::createWhite(),
            (float)0.0,
            (float)1.0,
            &vertexProperties);
        m_bodyPreviewMesh->setMeshId(m_object->meshId);
    }

    qDebug() << "The bone generation took" << countTimeConsumed.elapsed() << "milliseconds";

    emit finished();
}
