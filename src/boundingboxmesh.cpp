#include <QVector2D>
#include "boundingboxmesh.h"
#include "meshloader.h"

ShaderVertex *buildBoundingBoxMeshEdges(const std::vector<std::tuple<QVector3D, QVector3D, float>> &boxes,
        int *edgeVerticesNum)
{
    int numPerItem = 12 * 2;
    *edgeVerticesNum = boxes.size() * numPerItem;
    
    auto generateForBox = [&](const std::tuple<QVector3D, QVector3D, float> &box, ShaderVertex *vertices) {
        const auto &headPosition = std::get<0>(box);
        const auto &tailPosition = std::get<1>(box);
        float radius = std::get<2>(box);
        QVector3D direction = tailPosition - headPosition;
        QVector3D cutNormal = direction.normalized();
        QVector3D baseNormal = QVector3D(0, 0, 1);
        QVector3D u = QVector3D::crossProduct(cutNormal, baseNormal).normalized();
        QVector3D v = QVector3D::crossProduct(u, cutNormal).normalized();
        auto uFactor = u * radius;
        auto vFactor = v * radius;
        const std::vector<QVector2D> cutFaceTemplate = {QVector2D((float)-1.0, (float)-1.0),
            QVector2D((float)1.0, (float)-1.0),
            QVector2D((float)1.0,  (float)1.0),
            QVector2D((float)-1.0,  (float)1.0)
        };
        std::vector<QVector3D> resultCut;
        for (const auto &t: cutFaceTemplate) {
            resultCut.push_back(uFactor * t.x() + vFactor * t.y());
        }
        std::vector<QVector3D> headRing;
        std::vector<QVector3D> tailRing;
        std::vector<QVector3D> finalizedPoints;
        for (const auto &it: resultCut) {
            headRing.push_back(it + headPosition);
        }
        for (const auto &it: resultCut) {
            tailRing.push_back(it + tailPosition);
        }
        for (size_t i = 0; i < headRing.size(); ++i) {
            finalizedPoints.push_back(headRing[i]);
            finalizedPoints.push_back(headRing[(i + 1) % headRing.size()]);
        }
        for (size_t i = 0; i < tailRing.size(); ++i) {
            finalizedPoints.push_back(tailRing[i]);
            finalizedPoints.push_back(tailRing[(i + 1) % tailRing.size()]);
        }
        for (size_t i = 0; i < headRing.size(); ++i) {
            finalizedPoints.push_back(headRing[i]);
            finalizedPoints.push_back(tailRing[i]);
        }
        for (size_t i = 0; i < finalizedPoints.size(); ++i) {
            const auto &sourcePosition = finalizedPoints[i];
            ShaderVertex &currentVertex = vertices[i];
            currentVertex.posX = sourcePosition.x();
            currentVertex.posY = sourcePosition.y();
            currentVertex.posZ = sourcePosition.z();
            currentVertex.texU = 0;
            currentVertex.texV = 0;
            currentVertex.colorR = 0.0;
            currentVertex.colorG = 0.0;
            currentVertex.colorB = 0.0;
            currentVertex.normX = 0;
            currentVertex.normY = 0;
            currentVertex.normZ = 0;
            currentVertex.metalness = MeshLoader::m_defaultMetalness;
            currentVertex.roughness = MeshLoader::m_defaultRoughness;
            currentVertex.tangentX = 0;
            currentVertex.tangentY = 0;
            currentVertex.tangentZ = 0;
        }
    };
    
    ShaderVertex *edgeVertices = new ShaderVertex[*edgeVerticesNum];
    int vertexIndex = 0;
    for (const auto &box: boxes) {
        generateForBox(box, &edgeVertices[vertexIndex]);
        vertexIndex += numPerItem;
    }
    
    *edgeVerticesNum = vertexIndex;
    
    return edgeVertices;
}

MeshLoader *buildBoundingBoxMesh(const std::vector<std::tuple<QVector3D, QVector3D, float>> &boxes)
{
    int edgeVerticesNum = 0;
    ShaderVertex *edgeVertices = buildBoundingBoxMeshEdges(boxes, &edgeVerticesNum);
    return new MeshLoader(nullptr, 0, edgeVertices, edgeVerticesNum);
}
