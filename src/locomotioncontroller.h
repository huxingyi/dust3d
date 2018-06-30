#ifndef LOCOMOTION_CONTROLLER_H
#define LOCOMOTION_CONTROLLER_H
#include "jointnodetree.h"
#include "pogostick.h"
#include "jointconstraint.h"

class LocomotionController
{
public:
    LocomotionController(const JointNodeTree &jointNodeTree);
    ~LocomotionController();
    void prepare();
    void release();
    void simulate(float amount);
    const JointNodeTree &outputJointNodeTreeOnlyPositions() const;
private:
    void simulateLeg(PogoStick *pogoStick, const std::vector<int> &childrenOfLegEnd, std::pair<int, int> leg, std::map<int, JointConstraint> *constrants, float amount,
        QVector3D *footDirection, QVector3D *finalLegStartPosition, float *finalLegStartOffsetY);
    void makeInbetweenNodesInHermiteCurve(int firstJointIndex, QVector3D firstPitch, int secondJointIndex, QVector3D secondPitch);
private:
    JointNodeTree m_inputJointNodeTree;
    JointNodeTree m_outputJointNodeTree;
    std::vector<PogoStick> m_leftPogoSticks;
    std::vector<PogoStick> m_rightPogoSticks;
    std::vector<std::vector<int>> m_childrenOfLeftLegEnds;
    std::vector<std::vector<int>> m_childrenOfRightLegEnds;
};

#endif
