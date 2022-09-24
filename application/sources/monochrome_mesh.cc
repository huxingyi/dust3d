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
    /*
    auto buildSection = [](const dust3d::Vector3 &origin, 
            const dust3d::Vector3 &faceNormal, 
            const dust3d::Vector3 &tagent, 
            double radius)
    {
        auto upDirection = dust3d::Vector3::crossProduct(tagent, faceNormal);
        return std::vector<dust3d::Vector3> {
            origin + tagent * radius,
            origin - upDirection * radius,
            origin - tagent * radius,
            origin + upDirection * radius
        };
    };

    auto calculateTagent = [](const dust3d::Vector3 &direction) {
        const std::vector<dust3d::Vector3> axisList = {
            dust3d::Vector3 {1, 0, 0},
            dust3d::Vector3 {0, 1, 0},
            dust3d::Vector3 {0, 0, 1},
        };
        float maxDot = -1;
        size_t nearAxisIndex = 0;
        bool reversed = false;
        for (size_t i = 0; i < axisList.size(); ++i) {
            const auto axis = axisList[i];
            auto dot = dust3d::Vector3::dotProduct(axis, direction);
            auto positiveDot = std::abs(dot);
            if (positiveDot >= maxDot) {
                reversed = dot < 0;
                maxDot = positiveDot;
                nearAxisIndex = i;
            }
        }
        const auto& choosenAxis = axisList[(nearAxisIndex + 1) % 3];
        auto startDirection = dust3d::Vector3::crossProduct(direction, choosenAxis).normalized();
        return reversed ? -startDirection : startDirection;
    };
    */
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
    //m_triangleVertices.reserve(halfEdges.size() * 24);
    m_lineVertices.reserve(halfEdges.size() * 2);
    for (const auto &halfEdge: halfEdges) {
        // For two halfedges shared one edge, only output one line
        if (halfEdge.first > halfEdge.second)
            continue;
        const auto &from = object.vertices[halfEdge.first];
        const auto &to = object.vertices[halfEdge.second];
        m_lineVertices.emplace_back(MonochromeOpenGLVertex {
            (GLfloat)from.x(), 
            (GLfloat)from.y(), 
            (GLfloat)from.z()
        });
        m_lineVertices.emplace_back(MonochromeOpenGLVertex {
            (GLfloat)to.x(), 
            (GLfloat)to.y(), 
            (GLfloat)to.z()
        });
        /*
        auto direction = to - from;
        auto sectionNormal = direction.normalized();
        auto sectionTagent = calculateTagent(sectionNormal);
        auto sectionTo = buildSection(to, sectionNormal, sectionTagent, 0.002);
        auto sectionFrom = sectionTo;
        for (auto &it: sectionFrom)
            it = it - direction;

        for (size_t i = 0; i < sectionTo.size(); ++i) {
            size_t j = (i + 1) % sectionTo.size();
            m_triangleVertices.emplace_back(MonochromeOpenGLVertex {
                (GLfloat)sectionTo[i].x(), 
                (GLfloat)sectionTo[i].y(), 
                (GLfloat)sectionTo[i].z()
            });
            m_triangleVertices.emplace_back(MonochromeOpenGLVertex {
                (GLfloat)sectionFrom[i].x(), 
                (GLfloat)sectionFrom[i].y(), 
                (GLfloat)sectionFrom[i].z()
            });
            m_triangleVertices.emplace_back(MonochromeOpenGLVertex {
                (GLfloat)sectionTo[j].x(), 
                (GLfloat)sectionTo[j].y(), 
                (GLfloat)sectionTo[j].z()
            });

            m_triangleVertices.emplace_back(MonochromeOpenGLVertex {
                (GLfloat)sectionFrom[i].x(), 
                (GLfloat)sectionFrom[i].y(), 
                (GLfloat)sectionFrom[i].z()
            });
            m_triangleVertices.emplace_back(MonochromeOpenGLVertex {
                (GLfloat)sectionFrom[j].x(), 
                (GLfloat)sectionFrom[j].y(), 
                (GLfloat)sectionFrom[j].z()
            });
            m_triangleVertices.emplace_back(MonochromeOpenGLVertex {
                (GLfloat)sectionTo[j].x(), 
                (GLfloat)sectionTo[j].y(), 
                (GLfloat)sectionTo[j].z()
            });
        }
        */
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
