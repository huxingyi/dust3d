#ifndef DUST3D_HERMITE_CURVE_INTERPOLATION_H
#define DUST3D_HERMITE_CURVE_INTERPOLATION_H
#include <cmath>
#include <vector>
#include <unordered_map>
#include <QVector2D>
#include <QVector3D>

class HermiteCurveInterpolation
{
public:
    HermiteCurveInterpolation()
    {
    }
    
    size_t addNode(const QVector2D &node, const QVector3D &originalNode)
    {
        size_t nodeIndex = m_nodes.size();
        m_nodes.push_back(node);
        m_nodesForDistance.push_back(originalNode);
        return nodeIndex;
    }
    
    void addPerpendicularDirection(size_t nodeIndex, const QVector2D &direction)
    {
        m_perpendicularDirectionsPerNode[nodeIndex].push_back(direction);
    }
    
    void update();
    
    const QVector2D &getUpdatedPosition(size_t nodeIndex)
    {
        return m_updatedPositions[nodeIndex];
    }
private:
    std::vector<QVector2D> m_nodes;
    std::vector<QVector3D> m_nodesForDistance;
    std::unordered_map<size_t, std::vector<QVector2D>> m_perpendicularDirectionsPerNode;
    std::vector<QVector2D> m_updatedPositions;
};

#endif
