#ifndef DUST3D_VERTEBRATA_MOTION_H
#define DUST3D_VERTEBRATA_MOTION_H
#include <QVector3D>
#include <vector>
#include <map>
#include <unordered_map>
   
class VertebrataMotion
{
public:
    struct Parameters
    {
        double stanceTime = 0.35;
        double swingTime = 0.23;
        double preferredHeight = 0.39;
        int legSideIntval = 12;
        int legBalanceIntval = 12;
        double spineStability = 0.5;
        size_t cycles = 5;
        double groundOffset = 0.4;
        double tailLiftForce = 0.0;
        double tailDragForce = 4.0;
    };
    
    struct Node
    {
        QVector3D position;
        double radius;
        int boneIndex = -1;
        bool isTail = false;
    };
    
    enum class Side
    {
        Middle = 0,
        Left,
        Right,
    };
    
    struct Leg
    {
        std::vector<std::vector<Node>> nodes;
        std::vector<std::vector<Node>> updatedNodes;
        size_t heightIndices[3] = {0, 0, 0};
        size_t spineNodeIndex = 0;
    };

    VertebrataMotion()
    {
    }
    
    void setSpineNodes(const std::vector<Node> &nodes)
    {
        m_spineNodes = nodes;
    }
    
    void setLegNodes(size_t spineNodeIndex, Side side, const std::vector<Node> &nodes)
    {
        m_legNodes[{spineNodeIndex, side}] = nodes;
    }
    
    const std::vector<std::vector<Node>> &frames()
    {
        return m_frames;
    }
    
    void setParameters(const Parameters &parameters)
    {
        m_parameters = parameters;
    }
    
    void setGroundY(double groundY)
    {
        m_groundY = groundY;
    }
    
    void generate();
private:
    std::vector<std::vector<Node>> m_frames;
    Parameters m_parameters;
    std::vector<Node> m_spineNodes;
    std::vector<Node> m_updatedSpineNodes;
    std::map<std::pair<size_t, Side>, std::vector<Node>> m_legNodes;
    std::vector<double> m_legHeights;
    std::vector<double> m_legMoveOffsets;
    std::vector<Leg> m_legs;
    double m_groundY = 0.0;
    
    void prepareLegHeights();
    void prepareLegs();
    void prepareLegHeightIndices();
    void calculateLegMoves(size_t heightIndex);
};

#endif
