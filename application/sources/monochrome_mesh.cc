#include <set>
#include "monochrome_mesh.h"

MonochromeMesh::MonochromeMesh(const MonochromeMesh &mesh)
{
    m_lineVertices = mesh.m_lineVertices;
}

MonochromeMesh::MonochromeMesh(MonochromeMesh &&mesh)
{
    m_lineVertices = std::move(mesh.m_lineVertices);
}

MonochromeMesh::MonochromeMesh(std::vector<float> &&lineVertices)
{
    m_lineVertices = std::move(lineVertices);
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
    m_lineVertices.reserve(halfEdges.size() * 2 * sizeof(MonochromeOpenGLVertex));
    for (const auto &halfEdge: halfEdges) {
        // For two halfedges shared one edge, only output one line
        if (halfEdge.first > halfEdge.second)
            continue;
        const auto &from = object.vertices[halfEdge.first];
        const auto &to = object.vertices[halfEdge.second];
        m_lineVertices.push_back(from.x());
        m_lineVertices.push_back(from.y());
        m_lineVertices.push_back(from.z());
        m_lineVertices.push_back(0.0);
        m_lineVertices.push_back(0.0);
        m_lineVertices.push_back(0.0);
        m_lineVertices.push_back(1.0);
        m_lineVertices.push_back(to.x());
        m_lineVertices.push_back(to.y());
        m_lineVertices.push_back(to.z());
        m_lineVertices.push_back(0.0);
        m_lineVertices.push_back(0.0);
        m_lineVertices.push_back(0.0);
        m_lineVertices.push_back(1.0);
    }
}

const float *MonochromeMesh::lineVertices()
{
    if (m_lineVertices.empty())
        return nullptr;
    return &m_lineVertices[0];
}

int MonochromeMesh::lineVertexCount()
{
    return (int)(m_lineVertices.size() * sizeof(float) / sizeof(MonochromeOpenGLVertex));
}
