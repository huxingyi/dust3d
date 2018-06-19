#ifndef RIG_CONTROLLER_H
#define RIG_CONTROLLER_H
#include <QVector3D>
#include <vector>
#include <QQuaternion>
#include <QMatrix4x4>
#include <set>
#include "jointnodetree.h"
#include "rigframe.h"

class RigController
{
public:
    RigController(const JointNodeTree &jointNodeTree);
    void saveFrame(RigFrame &frame);
    void frameToMatrices(const RigFrame &frame, std::vector<QMatrix4x4> &matrices);
    void idle(float amount);
    void breathe(float amount);
    void resetFrame();
    float averageLegEndY();
    float minY();
    float averageLegHeight();
    void liftLegEnd(int leg, QVector3D offset);
    void liftLegs(QVector3D offset);
    void liftLeftLegs(QVector3D offset);
    void liftRightLegs(QVector3D offset);
    void lift(QVector3D offset);
    void prepare();
    void applyRigFrameToJointNodeTree(JointNodeTree &jointNodeTree, const RigFrame &frame);
private:
    void collectParts();
    int addLeg(std::pair<int, int> legStart, std::pair<int, int> legEnd);
    void calculateAverageLegHeight();
    void calculateMinY();
    float calculateAverageLegStartY(JointNodeTree &jointNodeTree);
    float calculateAverageLegEndY(JointNodeTree &jointNodeTree);
    void frameToMatricesAtJoint(const RigFrame &frame, std::vector<QMatrix4x4> &matrices, int jointIndex, const QMatrix4x4 &parentWorldMatrix);
private:
    JointNodeTree m_inputJointNodeTree;
    bool m_prepared;
    float m_legHeight;
    float m_averageLegEndY;
    float m_minY;
private:
    std::vector<std::tuple<int, int, int, int>> m_legs;
    std::vector<std::pair<int, int>> m_spine;
    RigFrame m_rigFrame;
};

#endif
