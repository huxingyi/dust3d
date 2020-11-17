#include "skinnedmeshcreator.h"
#include "theme.h"

SkinnedMeshCreator::SkinnedMeshCreator(const Object &object,
        const std::map<int, RiggerVertexWeights> &resultWeights) :
    m_object(object),
    m_resultWeights(resultWeights)
{
    m_verticesOldIndices.resize(m_object.triangles.size());
    m_verticesBindNormals.resize(m_object.triangles.size());
    m_verticesBindPositions.resize(m_object.triangles.size());
    const std::vector<std::vector<QVector3D>> *triangleVertexNormals = m_object.triangleVertexNormals();
    for (size_t triangleIndex = 0; triangleIndex < m_object.triangles.size(); triangleIndex++) {
        for (int j = 0; j < 3; j++) {
            int oldIndex = m_object.triangles[triangleIndex][j];
            m_verticesOldIndices[triangleIndex].push_back(oldIndex);
            m_verticesBindPositions[triangleIndex].push_back(m_object.vertices[oldIndex]);
            if (nullptr != triangleVertexNormals)
                m_verticesBindNormals[triangleIndex].push_back((*triangleVertexNormals)[triangleIndex][j]);
            else
                m_verticesBindNormals[triangleIndex].push_back(QVector3D());
        }
    }
    
    std::map<std::pair<QUuid, QUuid>, QColor> sourceNodeToColorMap;
    for (const auto &node: object.nodes)
        sourceNodeToColorMap.insert({{node.partId, node.nodeId}, node.color});
    
    m_triangleColors.resize(m_object.triangles.size(), Theme::white);
    const std::vector<std::pair<QUuid, QUuid>> *triangleSourceNodes = object.triangleSourceNodes();
    if (nullptr != triangleSourceNodes) {
        for (size_t triangleIndex = 0; triangleIndex < m_object.triangles.size(); triangleIndex++) {
            const auto &source = (*triangleSourceNodes)[triangleIndex];
            m_triangleColors[triangleIndex] = sourceNodeToColorMap[source];
        }
    }
}

Model *SkinnedMeshCreator::createMeshFromTransform(const std::vector<QMatrix4x4> &matricies)
{
    std::vector<std::vector<QVector3D>> transformedPositions = m_verticesBindPositions;
    std::vector<std::vector<QVector3D>> transformedPoseNormals = m_verticesBindNormals;
    
    if (!matricies.empty()) {
        for (size_t i = 0; i < transformedPositions.size(); ++i) {
            for (size_t j = 0; j < 3; ++j) {
                const auto &weight = m_resultWeights[m_verticesOldIndices[i][j]];
                QMatrix4x4 mixedMatrix;
                transformedPositions[i][j] = QVector3D();
                transformedPoseNormals[i][j] = QVector3D();
                for (int x = 0; x < 4; x++) {
                    float factor = weight.boneWeights[x];
                    if (factor > 0) {
                        transformedPositions[i][j] += matricies[weight.boneIndices[x]] * m_verticesBindPositions[i][j] * factor;
                        transformedPoseNormals[i][j] += matricies[weight.boneIndices[x]] * m_verticesBindNormals[i][j] * factor;
                    }
                }
            }
        }
    }
    
    ShaderVertex *triangleVertices = new ShaderVertex[m_object.triangles.size() * 3];
    int triangleVerticesNum = 0;
    for (size_t triangleIndex = 0; triangleIndex < m_object.triangles.size(); triangleIndex++) {
        for (int i = 0; i < 3; i++) {
            ShaderVertex &currentVertex = triangleVertices[triangleVerticesNum++];
            const auto &sourcePosition = transformedPositions[triangleIndex][i];
            const auto &sourceColor = m_triangleColors[triangleIndex];
            const auto &sourceNormal = transformedPoseNormals[triangleIndex][i];
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
            currentVertex.metalness = Model::m_defaultMetalness;
            currentVertex.roughness = Model::m_defaultRoughness;
        }
    }
    
    return new Model(triangleVertices, triangleVerticesNum);
}
