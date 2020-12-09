#ifndef DUST3D_VERTEBRATA_MOVE_MOTION_H
#define DUST3D_VERTEBRATA_MOVE_MOTION_H
#include <QVector3D>
#include <vector>
#include <map>
#include <unordered_map>
#include "motionbuilder.h"
   
class VertebrataMoveMotionBuilder : public MotionBuilder
{
public:
    struct Parameters
    {
        double stanceTime = 0.35;
        double swingTime = 0.23;
        double hipHeight = 0.39;
        double armLength = 0.29;
        double legSideIntval = 0.5;
        double legBalanceIntval = 0.5;
        double spineStability = 0.5;
        size_t cycles = 5;
        double groundOffset = 0.4;
        double tailLiftForce = 2.0;
        double tailDragForce = 4.0;
        bool biped = false;
    };

    struct Leg
    {
        std::vector<std::vector<Node>> nodes;
        std::vector<std::vector<Node>> updatedNodes;
        size_t heightIndices[3] = {0, 0, 0};
        size_t spineNodeIndex = 0;
        double top = 0.0;
        double move[3] = {0, 0, 0};
    };

    VertebrataMoveMotionBuilder() :
        MotionBuilder()
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
    void calculateSpineJoints();
};

#endif
