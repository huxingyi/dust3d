#include "toolmesh.h"

void ToolMesh::addNode(const QVector3D &position, float radius, const QMatrix4x4 &transform)
{
    m_nodes.push_back({position, radius, transform});
}

void ToolMesh::generate()
{
    // TODO:
}

ShaderVertex *ToolMesh::takeShaderVertices(int *shaderVertexCount)
{
    // TODO:
    return nullptr;
}
