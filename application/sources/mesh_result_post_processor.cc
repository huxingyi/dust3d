#include "mesh_result_post_processor.h"
#include <QGuiApplication>
#include <dust3d/mesh/resolve_triangle_tangent.h>
#include <dust3d/uv/unwrap_uv.h>

MeshResultPostProcessor::MeshResultPostProcessor(const dust3d::Object& object)
{
    m_object = new dust3d::Object;
    *m_object = object;
}

MeshResultPostProcessor::~MeshResultPostProcessor()
{
    delete m_object;
}

dust3d::Object* MeshResultPostProcessor::takePostProcessedObject()
{
    dust3d::Object* object = m_object;
    m_object = nullptr;
    return object;
}

void MeshResultPostProcessor::poseProcess()
{
    if (!m_object->nodes.empty()) {
        {
            std::vector<std::vector<dust3d::Vector2>> triangleVertexUvs;
            std::set<int> seamVertices;
            std::map<dust3d::Uuid, std::vector<dust3d::Rectangle>> partUvRects;
            dust3d::unwrapUv(*m_object, triangleVertexUvs, seamVertices, partUvRects);
            m_object->setTriangleVertexUvs(triangleVertexUvs);
            m_object->setPartUvRects(partUvRects);
        }

        {
            std::vector<dust3d::Vector3> triangleTangents;
            dust3d::resolveTriangleTangent(*m_object, triangleTangents);
            m_object->setTriangleTangents(triangleTangents);
        }
    }
}

void MeshResultPostProcessor::process()
{
    poseProcess();

    emit finished();
}
