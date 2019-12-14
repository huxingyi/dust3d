#ifndef DUST3D_MODIFIER_H
#define DUST3D_MODIFIER_H
#include <QVector3D>
#include <vector>

class StrokeModifier
{
public:
    struct Node
    {
        bool isOriginal = false;
        QVector3D position;
        float radius = 0.0;
        std::vector<QVector2D> cutTemplate;
        float cutRotation = 0.0;
        int nearOriginNodeIndex = -1;
        int farOriginNodeIndex = -1;
        int originNodeIndex = 0;
        float averageCutTemplateLength;
    };
    
    struct Edge
    {
        size_t firstNodeIndex;
        size_t secondNodeIndex;
    };
    
    size_t addNode(const QVector3D &position, float radius, const std::vector<QVector2D> &cutTemplate, float cutRotation);
    size_t addEdge(size_t firstNodeIndex, size_t secondNodeIndex);
    void subdivide();
    void roundEnd();
    void enableIntermediateAddition();
    const std::vector<Node> &nodes();
    const std::vector<Edge> &edges();
    void finalize();
    
private:
    
    std::vector<Node> m_nodes;
    std::vector<Edge> m_edges;
    bool m_intermediateAdditionEnabled = false;
    
    void createIntermediateNode(const Node &firstNode, const Node &secondNode, float factor, Node *resultNode);
    float averageCutTemplateEdgeLength(const std::vector<QVector2D> &cutTemplate);
    void createIntermediateCutTemplateEdges(std::vector<QVector2D> &cutTemplate, float averageCutTemplateLength);
};

#endif
