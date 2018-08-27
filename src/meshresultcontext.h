#ifndef MESH_RESULT_CONTEXT_H
#define MESH_RESULT_CONTEXT_H
#include <vector>
#include <set>
#include <QVector3D>
#include <QUuid>
#include <QColor>
#include "positionmap.h"

#define MAX_WEIGHT_NUM  4

struct BmeshNode
{
    QUuid partId;
    QUuid nodeId;
    QVector3D origin;
    float radius = 0;
    QColor color;
};

struct BmeshVertex
{
    QVector3D position;
    QUuid partId;
    QUuid nodeId;
};

struct ResultVertex
{
    QVector3D position;
};

struct ResultTriangle
{
    int indicies[3] = {0, 0, 0};
    QVector3D normal;
};

struct ResultTriangleUv
{
    float uv[3][2] = {{0, 0}, {0, 0}, {0, 0}};
    bool resolved = false;
};

struct ResultVertexUv
{
    float uv[2] = {0, 0};
};

struct ResultPart
{
    QColor color;
    std::vector<ResultVertex> vertices;
    std::vector<QVector3D> interpolatedVertexNormals;
    std::vector<ResultTriangle> triangles;
    std::vector<ResultTriangleUv> uvs;
    std::vector<ResultVertexUv> vertexUvs;
};

struct ResultRearrangedVertex
{
    QVector3D position;
    int originalIndex;
};

struct ResultRearrangedTriangle
{
    int indicies[3];
    QVector3D normal;
    int originalIndex;
};

class MeshResultContext
{
public:
    std::vector<BmeshNode> bmeshNodes;
    std::vector<BmeshVertex> bmeshVertices;
    std::vector<ResultVertex> vertices;
    std::vector<ResultTriangle> triangles;
    MeshResultContext();
public:
    const std::vector<std::pair<QUuid, QUuid>> &triangleSourceNodes();
    const std::vector<QColor> &triangleColors();
    const std::map<std::pair<int, int>, std::pair<QUuid, QUuid>> &triangleEdgeSourceMap();
    const std::map<std::pair<QUuid, QUuid>, BmeshNode *> &bmeshNodeMap();
    const std::map<QUuid, ResultPart> &parts();
    const std::vector<ResultTriangleUv> &triangleUvs();
    const std::vector<ResultRearrangedVertex> &rearrangedVertices();
    const std::vector<ResultRearrangedTriangle> &rearrangedTriangles();
    const std::map<int, std::pair<QUuid, QUuid>> &vertexSourceMap();
private:
    bool m_triangleSourceResolved;
    bool m_triangleColorResolved;
    bool m_triangleEdgeSourceMapResolved;
    bool m_bmeshNodeMapResolved;
    bool m_resultPartsResolved;
    bool m_resultTriangleUvsResolved;
    bool m_resultRearrangedVerticesResolved;
private:
    std::vector<std::pair<QUuid, QUuid>> m_triangleSourceNodes;
    std::vector<QColor> m_triangleColors;
    std::map<std::pair<int, int>, std::pair<QUuid, QUuid>> m_triangleEdgeSourceMap;
    std::map<std::pair<QUuid, QUuid>, BmeshNode *> m_bmeshNodeMap;
    std::map<QUuid, ResultPart> m_resultParts;
    std::vector<ResultTriangleUv> m_resultTriangleUvs;
    std::set<int> m_seamVertices;
    std::vector<ResultRearrangedVertex> m_rearrangedVertices;
    std::vector<ResultRearrangedTriangle> m_rearrangedTriangles;
    std::map<int, std::pair<QUuid, QUuid>> m_vertexSourceMap;
private:
    void calculateTriangleSourceNodes(std::vector<std::pair<QUuid, QUuid>> &triangleSourceNodes, std::map<int, std::pair<QUuid, QUuid>> &vertexSourceMap);
    void calculateRemainingVertexSourceNodesAfterTriangleSourceNodesSolved(std::map<int, std::pair<QUuid, QUuid>> &vertexSourceMap);
    void calculateTriangleColors(std::vector<QColor> &triangleColors);
    void calculateTriangleEdgeSourceMap(std::map<std::pair<int, int>, std::pair<QUuid, QUuid>> &triangleEdgeSourceMap);
    void calculateBmeshNodeMap(std::map<std::pair<QUuid, QUuid>, BmeshNode *> &bmeshNodeMap);
    void calculateResultParts(std::map<QUuid, ResultPart> &parts);
    void calculateResultTriangleUvs(std::vector<ResultTriangleUv> &uvs, std::set<int> &seamVertices);
    void calculateResultRearrangedVertices(std::vector<ResultRearrangedVertex> &rearrangedVertices, std::vector<ResultRearrangedTriangle> &rearrangedTriangles);
};

#endif
