#include <QVector2D>
#include <iostream>
#include "boundingboxmesh.h"
#include "model.h"
#include "util.h"

ShaderVertex *buildBoundingBoxMeshEdges(const std::vector<std::tuple<QVector3D, QVector3D, float, float, QColor>> &boxes,
        int *edgeVerticesNum)
{
    int numPerItem = 12 * 2;
    *edgeVerticesNum = boxes.size() * numPerItem;
    
    auto generateForBox = [&](const std::tuple<QVector3D, QVector3D, float, float, QColor> &box, ShaderVertex *vertices) {
        const auto &headPosition = std::get<0>(box);
        const auto &tailPosition = std::get<1>(box);
        float headRadius = std::get<2>(box);
        float tailRadius = std::get<3>(box);
        const auto &color = std::get<4>(box);
        QVector3D direction = tailPosition - headPosition;
        QVector3D cutNormal = direction.normalized();
        QVector3D baseNormal = choosenBaseAxis(cutNormal);
        QVector3D u = QVector3D::crossProduct(cutNormal, baseNormal).normalized();
        QVector3D v = QVector3D::crossProduct(u, cutNormal).normalized();
        auto uHeadFactor = u * headRadius;
        auto vHeadFactor = v * headRadius;
        auto uTailFactor = u * tailRadius;
        auto vTailFactor = v * tailRadius;
        const std::vector<QVector2D> cutFaceTemplate = {QVector2D((float)-1.0, (float)-1.0),
            QVector2D((float)1.0, (float)-1.0),
            QVector2D((float)1.0,  (float)1.0),
            QVector2D((float)-1.0,  (float)1.0)
        };
        std::vector<QVector3D> resultHeadCut;
        for (const auto &t: cutFaceTemplate) {
            resultHeadCut.push_back(uHeadFactor * t.x() + vHeadFactor * t.y());
        }
        std::vector<QVector3D> resultTailCut;
        for (const auto &t: cutFaceTemplate) {
            resultTailCut.push_back(uTailFactor * t.x() + vTailFactor * t.y());
        }
        std::vector<QVector3D> headRing;
        std::vector<QVector3D> tailRing;
        std::vector<QVector3D> finalizedPoints;
        for (const auto &it: resultHeadCut) {
            headRing.push_back(it + headPosition);
        }
        for (const auto &it: resultTailCut) {
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
            currentVertex.colorR = color.redF();
            currentVertex.colorG = color.greenF();
            currentVertex.colorB = color.blueF();
            currentVertex.normX = 0;
            currentVertex.normY = 1;
            currentVertex.normZ = 0;
            currentVertex.metalness = Model::m_defaultMetalness;
            currentVertex.roughness = Model::m_defaultRoughness;
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

Model *buildBoundingBoxMesh(const std::vector<std::tuple<QVector3D, QVector3D, float, float, QColor>> &boxes)
{
    int edgeVerticesNum = 0;
    ShaderVertex *edgeVertices = buildBoundingBoxMeshEdges(boxes, &edgeVerticesNum);
    return new Model(nullptr, 0, edgeVertices, edgeVerticesNum);
}
