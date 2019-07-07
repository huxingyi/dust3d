#ifndef NODEMESH_MODIFIER_H
#define NODEMESH_MODIFIER_H
#include <QVector3D>
#include <vector>

namespace nodemesh 
{

class Modifier
{
public:
    struct Node
    {
        bool isOriginal = false;
        QVector3D position;
        float radius = 0.0;
        std::vector<QVector2D> cutTemplate;
        int nearOriginNodeIndex = -1;
        int farOriginNodeIndex = -1;
        int originNodeIndex = 0;
    };
    
    struct Edge
    {
        size_t firstNodeIndex;
        size_t secondNodeIndex;
    };
    
    size_t addNode(const QVector3D &position, float radius, const std::vector<QVector2D> &cutTemplate);
    size_t addEdge(size_t firstNodeIndex, size_t secondNodeIndex);
    void subdivide();
    void roundEnd();
    const std::vector<Node> &nodes();
    const std::vector<Edge> &edges();
    void finalize();
    
private:
    
    std::vector<Node> m_nodes;
    std::vector<Edge> m_edges;
    
    void createIntermediateNode(const Node &firstNode, const Node &secondNode, float factor, Node *resultNode);
    float averageCutTemplateEdgeLength(const std::vector<QVector2D> &cutTemplate);
};

}

#endif
