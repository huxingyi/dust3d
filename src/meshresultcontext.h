#ifndef MESH_RESULT_CONTEXT_H
#define MESH_RESULT_CONTEXT_H
#include <vector>
#include <QVector3D>
#include <QUuid>
#include <QColor>
#include "positionmap.h"

struct BmeshNode
{
    int bmeshId;
    int nodeId;
    QVector3D origin;
    float radius;
    QColor color;
};

struct BmeshVertex
{
    QVector3D position;
    int bmeshId;
    int nodeId;
};

struct BmeshEdge
{
    int bmeshId;
    int fromNodeId;
    int toNodeId;
};

struct ResultVertex
{
    QVector3D position;
};

struct ResultTriangle
{
    int indicies[3];
    QVector3D normal;
};

class MeshResultContext
{
public:
    std::vector<BmeshNode> bmeshNodes;
    std::vector<BmeshVertex> bmeshVertices;
    std::vector<BmeshEdge> bmeshEdges;
    std::vector<ResultVertex> resultVertices;
    std::vector<ResultTriangle> resultTriangles;
public:
    void calculateTriangleColors(std::vector<QColor> &triangleColors);
};

#endif
