#include <set>
#include "monochrome_mesh.h"

MonochromeMesh::MonochromeMesh(const MonochromeMesh &mesh)
{
    m_vertices = mesh.m_vertices;
    m_lineVertexCount = mesh.m_lineVertexCount;
    m_triangleVertexCount = mesh.m_triangleVertexCount;
}

MonochromeMesh::MonochromeMesh(MonochromeMesh &&mesh)
{
    std::swap(m_vertices, mesh.m_vertices);
    std::swap(m_lineVertexCount, mesh.m_lineVertexCount);
    std::swap(m_triangleVertexCount, mesh.m_triangleVertexCount);
}

MonochromeMesh::MonochromeMesh(std::vector<float> &&vertices, int lineVertexCount, int triangleVertexCount)
{
    m_vertices = std::move(vertices);
    m_lineVertexCount = lineVertexCount;
    m_triangleVertexCount = triangleVertexCount;
}

MonochromeMesh::MonochromeMesh(const dust3d::Object &object)
{
    std::set<std::pair<size_t, size_t>> halfEdges;
    for (const auto &face: object.triangleAndQuads) {
        if (3 == face.size()) {
            halfEdges.insert({face[0], face[1]});
            halfEdges.insert({face[1], face[0]});
            halfEdges.insert({face[1], face[2]});
            halfEdges.insert({face[2], face[1]});
            halfEdges.insert({face[2], face[0]});
            halfEdges.insert({face[0], face[2]});
            continue;
        }
        halfEdges.insert({face[0], face[1]});
        halfEdges.insert({face[1], face[0]});
        halfEdges.insert({face[1], face[2]});
        halfEdges.insert({face[2], face[1]});
        halfEdges.insert({face[2], face[3]});
        halfEdges.insert({face[3], face[2]});
        halfEdges.insert({face[3], face[0]});
        halfEdges.insert({face[0], face[3]});
    }
    m_vertices.reserve(halfEdges.size() * 2 * sizeof(MonochromeOpenGLVertex));
    for (const auto &halfEdge: halfEdges) {
        // For two halfedges shared one edge, only output one line
        if (halfEdge.first > halfEdge.second)
            continue;
        const auto &from = object.vertices[halfEdge.first];
        const auto &to = object.vertices[halfEdge.second];
        m_vertices.push_back(from.x());
        m_vertices.push_back(from.y());
        m_vertices.push_back(from.z());
        m_vertices.push_back(0.0);
        m_vertices.push_back(0.0);
        m_vertices.push_back(0.0);
        m_vertices.push_back(1.0);
        m_vertices.push_back(to.x());
        m_vertices.push_back(to.y());
        m_vertices.push_back(to.z());
        m_vertices.push_back(0.0);
        m_vertices.push_back(0.0);
        m_vertices.push_back(0.0);
        m_vertices.push_back(1.0);
    }
    m_lineVertexCount = (int)(m_vertices.size() * sizeof(float) / sizeof(MonochromeOpenGLVertex));
}

const float *MonochromeMesh::vertices()
{
    if (m_vertices.empty())
        return nullptr;
    return &m_vertices[0];
}

int MonochromeMesh::triangleVertexCount()
{
    return m_triangleVertexCount;
}

int MonochromeMesh::lineVertexCount()
{
    return m_lineVertexCount;
}
