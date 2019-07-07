#include "toolmesh.h"
#include "theme.h"

const std::vector<QVector3D> ToolMesh::m_predefinedPoints = {
    QVector3D(1.000000, 1.000000, -1.000000),
    QVector3D(1.000000, -1.000000, -1.000000),
    QVector3D(1.000000, 1.000000, 1.000000),
    QVector3D(1.000000, -1.000000, 1.000000),
    QVector3D(-1.000000, 1.000000, -1.000000),
    QVector3D(-1.000000, -1.000000, -1.000000),
    QVector3D(-1.000000, 1.000000, 1.000000),
    QVector3D(-1.000000, -1.000000, 1.000000),
};

const std::vector<QVector3D> ToolMesh::m_predefinedNormals = {
    QVector3D(0.0000, 1.0000, 0.0000),
    QVector3D(0.0000, 0.0000, 1.0000),
    QVector3D(-1.0000, 0.0000, 0.0000),
    QVector3D(0.0000, -1.0000, 0.0000),
    QVector3D(1.0000, 0.0000, 0.0000),
    QVector3D(0.0000, 0.0000, -1.0000),
};

ToolMesh::~ToolMesh()
{
}

void ToolMesh::addNode(const QString &nodeId, const QVector3D &position)
{
    m_nodes.insert({nodeId, {position}});
}

void ToolMesh::fillCube(std::vector<ShaderVertex> *vertices, const QVector3D &position)
{
    auto addTriangle = [&](const std::vector<size_t> &indices, int normalIndex) {
        for (const auto &index: indices) {
            ShaderVertex vertex;
            memset(&vertex, 0, sizeof(ShaderVertex));
            
            const auto &transformedPosition = position + m_predefinedPoints[index - 1] * 0.003;
            const auto &transformedNormal = m_predefinedNormals[normalIndex - 1];
            
            vertex.posX = transformedPosition.x();
            vertex.posY = transformedPosition.y();
            vertex.posZ = transformedPosition.z();
            
            vertex.normX = transformedNormal.x();
            vertex.normY = transformedNormal.y();
            vertex.normZ = transformedNormal.z();
            
            vertex.colorR = Theme::green.redF();
            vertex.colorG = Theme::green.greenF();
            vertex.colorB = Theme::green.blueF();
            
            vertices->push_back(vertex);
        }
    };
    
    addTriangle({5, 3, 1}, 1);
    addTriangle({3, 8, 4}, 2);
    addTriangle({7, 6, 8}, 3);
    addTriangle({2, 8, 6}, 4);
    addTriangle({1, 4, 2}, 5);
    addTriangle({5, 2, 6}, 6);
    addTriangle({5, 7, 3}, 1);
    addTriangle({3, 7, 8}, 2);
    addTriangle({7, 5, 6}, 3);
    addTriangle({2, 4, 8}, 4);
    addTriangle({1, 3, 4}, 5);
    addTriangle({5, 1, 2}, 6);
}

void ToolMesh::generate()
{
    for (const auto &node: m_nodes) {
        fillCube(&m_nodesVertices[node.first], node.second.position);
    }
}

ShaderVertex *ToolMesh::takeShaderVertices(int *shaderVertexCount)
{
    int vertexCount = m_nodesVertices.size() * 12 * 3;
    ShaderVertex *shaderVertices = new ShaderVertex[vertexCount];
    if (nullptr != shaderVertexCount)
        *shaderVertexCount = vertexCount;
    int vertexIndex = 0;
    for (const auto &nodeIt: m_nodesVertices) {
        for (const auto &vertexIt: nodeIt.second) {
            Q_ASSERT(vertexIndex < vertexCount);
            ShaderVertex *dest = &shaderVertices[vertexIndex++];
            *dest = vertexIt;
        }
    }
    Q_ASSERT(vertexIndex == vertexCount);
    return shaderVertices;
}
