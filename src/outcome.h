#ifndef DUST3D_OUTCOME_H
#define DUST3D_OUTCOME_H
#include <vector>
#include <set>
#include <QVector3D>
#include <QUuid>
#include <QColor>
#include "positionmap.h"
#include "bonemark.h"
#include "texturetype.h"
#include "material.h"

#define MAX_WEIGHT_NUM  4

struct OutcomeMaterial
{
    QColor color;
    QUuid materialId;
};

struct OutcomeNode
{
    QUuid partId;
    QUuid nodeId;
    QVector3D origin;
    float radius = 0;
    OutcomeMaterial material;
    QUuid mirrorFromPartId;
    BoneMark boneMark;
};

struct OutcomeNodeVertex
{
    QVector3D position;
    QUuid partId;
    QUuid nodeId;
};

struct OutcomeVertex
{
    QVector3D position;
};

struct OutcomeTriangle
{
    int indicies[3] = {0, 0, 0};
    QVector3D normal;
};

struct OutcomeTriangleUv
{
    float uv[3][2] = {{0, 0}, {0, 0}, {0, 0}};
    bool resolved = false;
};

struct OutcomeVertexUv
{
    float uv[2] = {0, 0};
};

struct ResultPart
{
    OutcomeMaterial material;
    std::vector<OutcomeVertex> vertices;
    std::vector<int> verticesOldIndicies;
    std::vector<QVector3D> interpolatedTriangleVertexNormals;
    std::vector<OutcomeTriangle> triangles;
    std::vector<OutcomeTriangleUv> uvs;
    std::vector<OutcomeVertexUv> vertexUvs;
    std::vector<QVector3D> triangleTangents;
};

class Outcome
{
public:
    std::vector<OutcomeNode> nodes;
    std::vector<OutcomeNodeVertex> nodeVertices;
    std::vector<OutcomeVertex> vertices;
    std::vector<OutcomeTriangle> triangles;
public:
    const std::vector<std::pair<QUuid, QUuid>> &triangleSourceNodes();
    const std::vector<OutcomeMaterial> &triangleMaterials();
    const std::map<std::pair<int, int>, std::pair<QUuid, QUuid>> &triangleEdgeSourceMap();
    const std::map<std::pair<QUuid, QUuid>, OutcomeNode *> &bmeshNodeMap();
    const std::map<QUuid, ResultPart> &parts();
    const std::vector<OutcomeTriangleUv> &triangleUvs();
    const std::map<int, std::pair<QUuid, QUuid>> &vertexSourceMap();
    const std::vector<QVector3D> &interpolatedTriangleVertexNormals();
    const std::vector<QVector3D> &triangleTangents();
private:
    bool m_triangleSourceResolved = false;
    bool m_triangleMaterialResolved = false;
    bool m_triangleEdgeSourceMapResolved = false;
    bool m_bmeshNodeMapResolved = false;
    bool m_resultPartsResolved = false;
    bool m_resultTriangleUvsResolved = false;
    bool m_triangleVertexNormalsInterpolated = false;
    bool m_triangleTangentsResolved = false;
private:
    std::vector<std::pair<QUuid, QUuid>> m_triangleSourceNodes;
    std::vector<OutcomeMaterial> m_triangleMaterials;
    std::map<std::pair<int, int>, std::pair<QUuid, QUuid>> m_triangleEdgeSourceMap;
    std::map<std::pair<QUuid, QUuid>, OutcomeNode *> m_bmeshNodeMap;
    std::map<QUuid, ResultPart> m_resultParts;
    std::vector<OutcomeTriangleUv> m_resultTriangleUvs;
    std::set<int> m_seamVertices;
    std::map<int, std::pair<QUuid, QUuid>> m_vertexSourceMap;
    std::map<int, int> m_rearrangedVerticesToOldIndexMap;
    std::vector<QVector3D> m_interpolatedTriangleVertexNormals;
    std::vector<QVector3D> m_triangleTangents;
private:
    void calculateTriangleSourceNodes(std::vector<std::pair<QUuid, QUuid>> &triangleSourceNodes, std::map<int, std::pair<QUuid, QUuid>> &vertexSourceMap);
    void calculateRemainingVertexSourceNodesAfterTriangleSourceNodesSolved(std::map<int, std::pair<QUuid, QUuid>> &vertexSourceMap);
    void calculateTriangleMaterials(std::vector<OutcomeMaterial> &triangleMaterials);
    void calculateTriangleEdgeSourceMap(std::map<std::pair<int, int>, std::pair<QUuid, QUuid>> &triangleEdgeSourceMap);
    void calculateBmeshNodeMap(std::map<std::pair<QUuid, QUuid>, OutcomeNode *> &bmeshNodeMap);
    void calculateResultParts(std::map<QUuid, ResultPart> &parts);
    void calculateResultTriangleUvs(std::vector<OutcomeTriangleUv> &uvs, std::set<int> &seamVertices);
    void interpolateTriangleVertexNormals(std::vector<QVector3D> &resultNormals);
    void calculateTriangleTangents(std::vector<QVector3D> &tangents);
};

#endif
