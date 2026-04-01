#include "monochrome_mesh.h"
#include <set>

MonochromeMesh::MonochromeMesh(const MonochromeMesh& mesh)
{
    m_lineVertices = mesh.m_lineVertices;
}

MonochromeMesh::MonochromeMesh(MonochromeMesh&& mesh)
{
    m_lineVertices = std::move(mesh.m_lineVertices);
}

MonochromeMesh::MonochromeMesh(const dust3d::Object& object)
{
    std::set<std::pair<size_t, size_t>> halfEdges;
    for (const auto& face : object.triangleAndQuads) {
        if (3 == face.size()) {
            halfEdges.insert({ face[0], face[1] });
            halfEdges.insert({ face[1], face[0] });
            halfEdges.insert({ face[1], face[2] });
            halfEdges.insert({ face[2], face[1] });
            halfEdges.insert({ face[2], face[0] });
            halfEdges.insert({ face[0], face[2] });
            continue;
        }
        halfEdges.insert({ face[0], face[1] });
        halfEdges.insert({ face[1], face[0] });
        halfEdges.insert({ face[1], face[2] });
        halfEdges.insert({ face[2], face[1] });
        halfEdges.insert({ face[2], face[3] });
        halfEdges.insert({ face[3], face[2] });
        halfEdges.insert({ face[3], face[0] });
        halfEdges.insert({ face[0], face[3] });
    }
    m_lineVertices.reserve(halfEdges.size() * 2);
    for (const auto& halfEdge : halfEdges) {
        // For two halfedges shared one edge, only output one line
        if (halfEdge.first > halfEdge.second)
            continue;
        const auto& from = object.vertices[halfEdge.first];
        const auto& to = object.vertices[halfEdge.second];
        m_lineVertices.emplace_back(MonochromeOpenGLVertex {
            (GLfloat)from.x(),
            (GLfloat)from.y(),
            (GLfloat)from.z() });
        m_lineVertices.emplace_back(MonochromeOpenGLVertex {
            (GLfloat)to.x(),
            (GLfloat)to.y(),
            (GLfloat)to.z() });
    }
}

const MonochromeOpenGLVertex* MonochromeMesh::lineVertices()
{
    if (m_lineVertices.empty())
        return nullptr;
    return &m_lineVertices[0];
}

int MonochromeMesh::lineVertexCount()
{
    return (int)m_lineVertices.size();
}

MonochromeMesh::MonochromeMesh(const ModelOpenGLVertex* triangleVertices, int vertexCount,
    float r, float g, float b, float a)
{
    // Each group of 3 vertices forms a triangle; emit 3 edges per triangle
    m_lineVertices.reserve(vertexCount * 2);
    for (int i = 0; i + 2 < vertexCount; i += 3) {
        const auto& v0 = triangleVertices[i];
        const auto& v1 = triangleVertices[i + 1];
        const auto& v2 = triangleVertices[i + 2];
        m_lineVertices.emplace_back(MonochromeOpenGLVertex { v0.posX, v0.posY, v0.posZ, r, g, b, a });
        m_lineVertices.emplace_back(MonochromeOpenGLVertex { v1.posX, v1.posY, v1.posZ, r, g, b, a });
        m_lineVertices.emplace_back(MonochromeOpenGLVertex { v1.posX, v1.posY, v1.posZ, r, g, b, a });
        m_lineVertices.emplace_back(MonochromeOpenGLVertex { v2.posX, v2.posY, v2.posZ, r, g, b, a });
        m_lineVertices.emplace_back(MonochromeOpenGLVertex { v2.posX, v2.posY, v2.posZ, r, g, b, a });
        m_lineVertices.emplace_back(MonochromeOpenGLVertex { v0.posX, v0.posY, v0.posZ, r, g, b, a });
    }
}
