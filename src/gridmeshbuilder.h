#ifndef DUST3D_GRID_MESH_BUILDER_H
#define DUST3D_GRID_MESH_BUILDER_H
#include <QVector3D>
#include <vector>
#include <map>
#include "regionfiller.h"

class GridMeshBuilder
{
public:
    struct Node
    {
        QVector3D position;
        float radius;
        size_t source;
        std::vector<size_t> neighborIndices;
    };
    
    struct Edge
    {
        size_t firstNodeIndex;
        size_t secondNodeIndex;
    };

    size_t addNode(const QVector3D &position, float radius);
    size_t addEdge(size_t firstNodeIndex, size_t secondNodeIndex);
    void setSubdived(bool subdived);
    void build();
    const std::vector<QVector3D> &getGeneratedPositions();
    const std::vector<size_t> &getGeneratedSources();
    const std::vector<std::vector<size_t>> &getGeneratedFaces();
    
private:
    std::vector<Node> m_nodes;
    std::vector<Edge> m_edges;
    std::vector<RegionFiller::Node> m_generatedVertices;
    std::vector<QVector3D> m_generatedPositions;
    std::vector<size_t> m_generatedSources;
    std::vector<std::vector<size_t>> m_generatedFaces;
    std::vector<size_t> m_generatedFaceSourceCycles;
    std::vector<RegionFiller::Node> m_nodeVertices;
    std::vector<QVector3D> m_nodePositions;
    std::vector<std::vector<size_t>> m_cycles;
    std::map<std::pair<size_t, size_t>, size_t> m_halfEdgeMap;
    std::vector<QVector3D> m_nodeNormals;
    float m_polylineAngleChangeThreshold = 35;
    float m_meshTargetEdgeSize = 0.04;
    size_t m_maxBigRingSize = 8;
    bool m_subdived = false;
    void applyModifiers();
    void prepareNodeVertices();
    void findCycles();
    void splitCycleToPolylines(const std::vector<size_t> &cycle,
        std::vector<std::vector<size_t>> *polylines);
    void generateFaces();
    void extrude();
    void calculateNormals();
    void removeBigRingFaces();
};

#endif

