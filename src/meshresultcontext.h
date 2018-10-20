#ifndef MESH_RESULT_CONTEXT_H
#define MESH_RESULT_CONTEXT_H
#include <vector>
#include <set>
#include <QVector3D>
#include <QUuid>
#include <QColor>
#include "positionmap.h"
#include "skeletonbonemark.h"
#include "texturetype.h"
#include "material.h"

#define MAX_WEIGHT_NUM  4

struct BmeshNode
{
    QUuid partId;
    QUuid nodeId;
    QVector3D origin;
    float radius = 0;
    Material material;
    QUuid mirrorFromPartId;
    SkeletonBoneMark boneMark;
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
    Material material;
    std::vector<ResultVertex> vertices;
    std::vector<int> verticesOldIndicies;
    std::vector<QVector3D> interpolatedTriangleVertexNormals;
    std::vector<ResultTriangle> triangles;
    std::vector<ResultTriangleUv> uvs;
    std::vector<ResultVertexUv> vertexUvs;
    std::vector<QVector3D> triangleTangents;
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
    const std::vector<Material> &triangleMaterials();
    const std::map<std::pair<int, int>, std::pair<QUuid, QUuid>> &triangleEdgeSourceMap();
    const std::map<std::pair<QUuid, QUuid>, BmeshNode *> &bmeshNodeMap();
    const std::map<QUuid, ResultPart> &parts();
    const std::vector<ResultTriangleUv> &triangleUvs();
    const std::map<int, std::pair<QUuid, QUuid>> &vertexSourceMap();
    const std::vector<QVector3D> &interpolatedTriangleVertexNormals();
    const std::vector<QVector3D> &triangleTangents();
private:
    bool m_triangleSourceResolved;
    bool m_triangleMaterialResolved;
    bool m_triangleEdgeSourceMapResolved;
    bool m_bmeshNodeMapResolved;
    bool m_resultPartsResolved;
    bool m_resultTriangleUvsResolved;
    bool m_triangleVertexNormalsInterpolated;
    bool m_triangleTangentsResolved;
private:
    std::vector<std::pair<QUuid, QUuid>> m_triangleSourceNodes;
    std::vector<Material> m_triangleMaterials;
    std::map<std::pair<int, int>, std::pair<QUuid, QUuid>> m_triangleEdgeSourceMap;
    std::map<std::pair<QUuid, QUuid>, BmeshNode *> m_bmeshNodeMap;
    std::map<QUuid, ResultPart> m_resultParts;
    std::vector<ResultTriangleUv> m_resultTriangleUvs;
    std::set<int> m_seamVertices;
    std::map<int, std::pair<QUuid, QUuid>> m_vertexSourceMap;
    std::map<int, int> m_rearrangedVerticesToOldIndexMap;
    std::vector<QVector3D> m_interpolatedTriangleVertexNormals;
    std::vector<QVector3D> m_triangleTangents;
private:
    void calculateTriangleSourceNodes(std::vector<std::pair<QUuid, QUuid>> &triangleSourceNodes, std::map<int, std::pair<QUuid, QUuid>> &vertexSourceMap);
    void calculateRemainingVertexSourceNodesAfterTriangleSourceNodesSolved(std::map<int, std::pair<QUuid, QUuid>> &vertexSourceMap);
    void calculateTriangleMaterials(std::vector<Material> &triangleMaterials);
    void calculateTriangleEdgeSourceMap(std::map<std::pair<int, int>, std::pair<QUuid, QUuid>> &triangleEdgeSourceMap);
    void calculateBmeshNodeMap(std::map<std::pair<QUuid, QUuid>, BmeshNode *> &bmeshNodeMap);
    void calculateResultParts(std::map<QUuid, ResultPart> &parts);
    void calculateResultTriangleUvs(std::vector<ResultTriangleUv> &uvs, std::set<int> &seamVertices);
    void interpolateTriangleVertexNormals(std::vector<QVector3D> &resultNormals);
    void calculateTriangleTangents(std::vector<QVector3D> &tangents);
};

#endif
