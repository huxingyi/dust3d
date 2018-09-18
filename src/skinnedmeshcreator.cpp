#include "skinnedmeshcreator.h"

SkinnedMeshCreator::SkinnedMeshCreator(const MeshResultContext &meshResultContext,
        const std::map<int, AutoRiggerVertexWeights> &resultWeights) :
    m_meshResultContext(meshResultContext),
    m_resultWeights(resultWeights)
{
    for (const auto &vertex: m_meshResultContext.vertices) {
        m_verticesBindPositions.push_back(vertex.position);
    }
    
    m_verticesBindNormals.resize(m_meshResultContext.vertices.size());
    for (size_t triangleIndex = 0; triangleIndex < m_meshResultContext.triangles.size(); triangleIndex++) {
        const auto &sourceTriangle = m_meshResultContext.triangles[triangleIndex];
        for (int i = 0; i < 3; i++)
            m_verticesBindNormals[sourceTriangle.indicies[i]] += sourceTriangle.normal;
    }
    for (auto &item: m_verticesBindNormals)
        item.normalize();
}

MeshLoader *SkinnedMeshCreator::createMeshFromTransform(const std::vector<QMatrix4x4> &matricies)
{
    std::vector<QVector3D> transformedPositions = m_verticesBindPositions;
    std::vector<QVector3D> transformedPoseNormals = m_verticesBindNormals;
    
    if (!matricies.empty()) {
        for (const auto &weightItem: m_resultWeights) {
            size_t vertexIndex = weightItem.first;
            const auto &weight = weightItem.second;
            QMatrix4x4 mixedMatrix;
            transformedPositions[vertexIndex] = QVector3D();
            transformedPoseNormals[vertexIndex] = QVector3D();
            for (int i = 0; i < 4; i++) {
                float factor = weight.boneWeights[i];
                if (factor > 0) {
                    transformedPositions[vertexIndex] += matricies[weight.boneIndicies[i]] * m_verticesBindPositions[vertexIndex] * factor;
                    transformedPoseNormals[vertexIndex] += matricies[weight.boneIndicies[i]] * m_verticesBindNormals[vertexIndex] * factor;
                }
            }
        }
    }
    
    Vertex *triangleVertices = new Vertex[m_meshResultContext.triangles.size() * 3];
    int triangleVerticesNum = 0;
    for (size_t triangleIndex = 0; triangleIndex < m_meshResultContext.triangles.size(); triangleIndex++) {
        const auto &sourceTriangle = m_meshResultContext.triangles[triangleIndex];
        QColor triangleColor = m_meshResultContext.triangleColors()[triangleIndex];
        for (int i = 0; i < 3; i++) {
            Vertex &currentVertex = triangleVertices[triangleVerticesNum++];
            const auto &sourcePosition = transformedPositions[sourceTriangle.indicies[i]];
            const auto &sourceColor = triangleColor;
            const auto &sourceNormal = transformedPoseNormals[sourceTriangle.indicies[i]];
            currentVertex.posX = sourcePosition.x();
            currentVertex.posY = sourcePosition.y();
            currentVertex.posZ = sourcePosition.z();
            currentVertex.texU = 0;
            currentVertex.texV = 0;
            currentVertex.colorR = sourceColor.redF();
            currentVertex.colorG = sourceColor.greenF();
            currentVertex.colorB = sourceColor.blueF();
            currentVertex.normX = sourceNormal.x();
            currentVertex.normY = sourceNormal.y();
            currentVertex.normZ = sourceNormal.z();
        }
    }
    
    return new MeshLoader(triangleVertices, triangleVerticesNum);
}
