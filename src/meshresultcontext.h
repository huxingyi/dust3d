#ifndef MESH_RESULT_CONTEXT_H
#define MESH_RESULT_CONTEXT_H
#include <vector>
#include <set>
#include <QVector3D>
#include <QUuid>
#include <QColor>
#include "positionmap.h"
#include "skeletonbonemark.h"
#include "intermediateboneremover.h"

#define MAX_WEIGHT_NUM  4

struct BmeshNode
{
    int bmeshId = 0;
    int nodeId = 0;
    QVector3D origin;
    float radius = 0;
    QColor color;
    SkeletonBoneMark boneMark = SkeletonBoneMark::None;
};

struct BmeshVertex
{
    QVector3D position;
    int bmeshId = 0;
    int nodeId = 0;
};

struct BmeshEdge
{
    int fromBmeshId = 0;
    int fromNodeId = 0;
    int toBmeshId = 0;
    int toNodeId = 0;
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

struct ResultVertexWeight
{
    std::pair<int, int> sourceNode = std::make_pair(0, 0);
    int count = 0;
    float weight = 0;
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
    std::vector<std::vector<ResultVertexWeight>> weights;
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

struct BmeshConnectionPossible
{
    BmeshEdge bmeshEdge;
    float rscore;
};

class MeshResultContext
{
public:
    std::vector<BmeshNode> bmeshNodes;
    std::vector<BmeshVertex> bmeshVertices;
    std::vector<BmeshEdge> bmeshEdges;
    std::vector<ResultVertex> vertices;
    std::vector<ResultTriangle> triangles;
    MeshResultContext();
public:
    const std::vector<std::pair<int, int>> &triangleSourceNodes();
    const std::vector<QColor> &triangleColors();
    const std::map<std::pair<int, int>, std::pair<int, int>> &triangleEdgeSourceMap();
    const std::map<std::pair<int, int>, BmeshNode *> &bmeshNodeMap();
    const BmeshNode *centerBmeshNode();
    void resolveBmeshConnectivity();
    void resolveBmeshEdgeDirections();
    const std::map<std::pair<int, int>, std::vector<std::pair<int, int>>> &nodeNeighbors();
    const std::vector<std::vector<ResultVertexWeight>> &vertexWeights();
    const std::map<int, ResultPart> &parts();
    const std::vector<ResultTriangleUv> &triangleUvs();
    const std::vector<ResultRearrangedVertex> &rearrangedVertices();
    const std::vector<ResultRearrangedTriangle> &rearrangedTriangles();
    const std::map<int, std::pair<int, int>> &vertexSourceMap();
    void removeIntermediateBones();
    bool isVerticalSpine();
private:
    bool m_triangleSourceResolved;
    bool m_triangleColorResolved;
    bool m_bmeshConnectivityResolved;
    bool m_triangleEdgeSourceMapResolved;
    bool m_bmeshNodeMapResolved;
    bool m_centerBmeshNodeResolved;
    bool m_bmeshEdgeDirectionsResolved;
    bool m_bmeshNodeNeighborsResolved;
    bool m_vertexWeightsResolved;
    BmeshNode *m_centerBmeshNode;
    bool m_resultPartsResolved;
    bool m_resultTriangleUvsResolved;
    bool m_resultRearrangedVerticesResolved;
    bool m_isVerticalSpine;
    bool m_spineDirectionResolved;
private:
    std::vector<std::pair<int, int>> m_triangleSourceNodes;
    std::vector<QColor> m_triangleColors;
    std::map<std::pair<int, int>, std::pair<int, int>> m_triangleEdgeSourceMap;
    std::map<std::pair<int, int>, BmeshNode *> m_bmeshNodeMap;
    std::map<std::pair<int, int>, std::vector<std::pair<int, int>>> m_nodeNeighbors;
    std::vector<std::vector<ResultVertexWeight>> m_resultVertexWeights;
    std::map<int, ResultPart> m_resultParts;
    std::vector<ResultTriangleUv> m_resultTriangleUvs;
    std::set<int> m_seamVertices;
    std::vector<ResultRearrangedVertex> m_rearrangedVertices;
    std::vector<ResultRearrangedTriangle> m_rearrangedTriangles;
    std::map<int, std::pair<int, int>> m_vertexSourceMap;
private:
    void calculateTriangleSourceNodes(std::vector<std::pair<int, int>> &triangleSourceNodes, std::map<int, std::pair<int, int>> &vertexSourceMap);
    void calculateRemainingVertexSourceNodesAfterTriangleSourceNodesSolved(std::map<int, std::pair<int, int>> &vertexSourceMap);
    void calculateTriangleColors(std::vector<QColor> &triangleColors);
    void calculateTriangleEdgeSourceMap(std::map<std::pair<int, int>, std::pair<int, int>> &triangleEdgeSourceMap);
    void calculateBmeshNodeMap(std::map<std::pair<int, int>, BmeshNode *> &bmeshNodeMap);
    BmeshNode *calculateCenterBmeshNode();
    void calculateBmeshConnectivity();
    void calculateBmeshEdgeDirections();
    void calculateBmeshNodeNeighbors();
    void calculateBmeshEdgeDirectionsFromNode(std::pair<int, int> node, std::set<std::pair<int, int>> &visitedNodes, std::set<std::pair<std::pair<int, int>, std::pair<int, int>>> &connections, std::vector<BmeshEdge> &rearrangedEdges);
    void calculateBmeshNodeNeighbors(std::map<std::pair<int, int>, std::vector<std::pair<int, int>>> &nodeNeighbors);
    void calculateVertexWeights(std::vector<std::vector<ResultVertexWeight>> &vertexWeights, const std::map<std::pair<int, int>, IntermediateBoneNode> *intermediateNodes=nullptr);
    void calculateResultParts(std::map<int, ResultPart> &parts);
    void calculateResultTriangleUvs(std::vector<ResultTriangleUv> &uvs, std::set<int> &seamVertices);
    void calculateResultRearrangedVertices(std::vector<ResultRearrangedVertex> &rearrangedVertices, std::vector<ResultRearrangedTriangle> &rearrangedTriangles);
    void calculateConnectionPossibleRscore(BmeshConnectionPossible &possible);
    void calculateSpineDirection(bool &isVerticalSpine);
};

#endif
