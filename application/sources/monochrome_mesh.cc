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
    m_lineVertices.reserve(halfEdges.size() * 2);
    for (const auto &halfEdge: halfEdges) {
        // For two halfedges shared one edge, only output one line
        if (halfEdge.first > halfEdge.second)
            continue;
        const auto &from = object.vertices[halfEdge.first];
        const auto &to = object.vertices[halfEdge.second];
        m_lineVertices.emplace_back(MonochromeOpenGLVertex {(GLfloat)from.x(), (GLfloat)from.y(), (GLfloat)from.z()});
        m_lineVertices.emplace_back(MonochromeOpenGLVertex {(GLfloat)to.x(), (GLfloat)to.y(), (GLfloat)to.z()});
    }
}

const MonochromeOpenGLVertex *MonochromeMesh::lineVertices()
{
    if (m_lineVertices.empty())
        return nullptr;
    return &m_lineVertices[0];
}

int MonochromeMesh::lineVertexCount()
{
    return (int)m_lineVertices.size();
}
