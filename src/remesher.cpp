#include <instant-meshes-api.h>
#include <cmath>
#include <QElapsedTimer>
#include "remesher.h"
#include "util.h"
#include "projectfacestonodes.h"

Remesher::Remesher()
{
}

Remesher::~Remesher()
{
}

void Remesher::setMesh(const std::vector<QVector3D> &vertices,
        const std::vector<std::vector<size_t>> &triangles)
{
    m_vertices = vertices;
    m_triangles = triangles;
}

const std::vector<QVector3D> &Remesher::getRemeshedVertices() const
{
    return m_remeshedVertices;
}

const std::vector<std::vector<size_t>> &Remesher::getRemeshedFaces() const
{
    return m_remeshedFaces;
}

const std::vector<std::pair<QUuid, QUuid>> &Remesher::getRemeshedVertexSources() const
{
    return m_remeshedVertexSources;
}

void Remesher::remesh(float targetVertexMultiplyFactor)
{
    std::vector<Dust3D_InstantMeshesVertex> inputVertices(m_vertices.size());
    for (size_t i = 0; i < m_vertices.size(); ++i) {
        const auto &vertex = m_vertices[i];
        inputVertices[i] = Dust3D_InstantMeshesVertex {
            vertex.x(), vertex.y(), vertex.z()
        };
    }
    std::vector<Dust3D_InstantMeshesTriangle> inputTriangles;
    inputTriangles.reserve(m_triangles.size());
    float totalArea = 0.0f;
    for (size_t i = 0; i < m_triangles.size(); ++i) {
        const auto &triangle = m_triangles[i];
        if (triangle.size() != 3)
            continue;
        inputTriangles.push_back(Dust3D_InstantMeshesTriangle {{
            triangle[0],
            triangle[1],
            triangle[2]
        }});
        totalArea += areaOfTriangle(m_vertices[triangle[0]], m_vertices[triangle[1]], m_vertices[triangle[2]]);
    }
    const Dust3D_InstantMeshesVertex *resultVertices = nullptr;
    size_t nResultVertices = 0;
    const Dust3D_InstantMeshesTriangle *resultTriangles = nullptr;
    size_t nResultTriangles = 0;
    const Dust3D_InstantMeshesQuad *resultQuads = nullptr;
    size_t nResultQuads = 0;
    Dust3D_instantMeshesRemesh(inputVertices.data(), inputVertices.size(),
        inputTriangles.data(), inputTriangles.size(),
        (size_t)(targetVertexMultiplyFactor * 30 * std::sqrt(totalArea) / 0.02f),
        &resultVertices,
        &nResultVertices,
        &resultTriangles,
        &nResultTriangles,
        &resultQuads,
        &nResultQuads);
    m_remeshedVertices.resize(nResultVertices);
    memcpy(m_remeshedVertices.data(), resultVertices, sizeof(Dust3D_InstantMeshesVertex) * nResultVertices);
    m_remeshedFaces.reserve(nResultTriangles + nResultQuads);
    for (size_t i = 0; i < nResultTriangles; ++i) {
        const auto &source = resultTriangles[i];
        m_remeshedFaces.push_back(std::vector<size_t> {
            source.indices[0],
            source.indices[1],
            source.indices[2]
        });
    }
    for (size_t i = 0; i < nResultQuads; ++i) {
        const auto &source = resultQuads[i];
        m_remeshedFaces.push_back(std::vector<size_t> {
            source.indices[0],
            source.indices[1],
            source.indices[2],
            source.indices[3]
        });
    }
    resolveSources();
}

void Remesher::setNodes(const std::vector<std::pair<QVector3D, float>> &nodes,
        const std::vector<std::pair<QUuid, QUuid>> &sourceIds)
{
    m_nodes = nodes;
    m_sourceIds = sourceIds;
}

void Remesher::resolveSources()
{
    QElapsedTimer timer;
    timer.start();
    std::vector<size_t> faceSources;
    auto projectFacesToNodesStartTime = timer.elapsed();
    projectFacesToNodes(m_remeshedVertices, m_remeshedFaces, m_nodes, &faceSources);
    auto projectFacesToNodesStopTime = timer.elapsed();
    qDebug() << "Project faces to nodes took" << (projectFacesToNodesStopTime - projectFacesToNodesStartTime) << "milliseconds";
    std::map<size_t, std::map<size_t, size_t>> vertexToNodeVotes;
    for (size_t i = 0; i < m_remeshedFaces.size(); ++i) {
        const auto &face = m_remeshedFaces[i];
        const auto &source = faceSources[i];
        for (const auto &vertexIndex: face)
            vertexToNodeVotes[vertexIndex][source]++;
    }
    m_remeshedVertexSources.resize(m_remeshedVertices.size());
    for (const auto &it: vertexToNodeVotes) {
        auto sourceIndex = std::max_element(it.second.begin(), it.second.end(), [](const std::pair<size_t, size_t> &first, const std::pair<size_t, size_t> &second) {
            return first.second < second.second;
        })->first;
        m_remeshedVertexSources[it.first] = m_sourceIds[sourceIndex];
    }
}
