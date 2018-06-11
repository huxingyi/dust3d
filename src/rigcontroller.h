#ifndef RIG_CONTROLLER_H
#define RIG_CONTROLLER_H
#include <QVector3D>
#include <vector>
#include <QQuaternion>
#include <QMatrix4x4>
#include "jointnodetree.h"

class RigFrame
{
public:
    RigFrame()
    {
    }
    RigFrame(int jointNum)
    {
        rotations.clear();
        rotations.resize(jointNum);
        
        translations.clear();
        translations.resize(jointNum);
    }
    std::vector<QQuaternion> rotations;
    std::vector<QVector3D> translations;
};

class RigController
{
public:
    RigController(const JointNodeTree &jointNodeTree);
    void saveFrame(RigFrame &frame);
    void frameToMatrices(const RigFrame &frame, std::vector<QMatrix4x4> &matrices);
    void squat(float amount);
private:
    void prepare();
    void collectLegs();
    int addLeg(std::pair<int, int> legStart, std::pair<int, int> legEnd);
    void liftLegEnd(int leg, QVector3D offset, QVector3D &effectedOffset);
    void liftLegs(QVector3D offset, QVector3D &effectedOffset);
    void lift(QVector3D offset);
    void calculateAverageLegHeight();
    void frameToMatricesAtJoint(const RigFrame &frame, std::vector<QMatrix4x4> &matrices, int jointIndex, const QMatrix4x4 &parentWorldMatrix);
private:
    JointNodeTree m_inputJointNodeTree;
    bool m_prepared;
    float m_legHeight;
private:
    std::vector<std::tuple<int, int, int, int>> m_legs;
    RigFrame m_rigFrame;
};

#endif
