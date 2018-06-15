#ifndef RIG_CONTROLLER_H
#define RIG_CONTROLLER_H
#include <QVector3D>
#include <vector>
#include <QQuaternion>
#include <QMatrix4x4>
#include <set>
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
        
        scales.clear();
        scales.resize(jointNum, QVector3D(1.0, 1.0, 1.0));
    }
    void updateRotation(int index, const QQuaternion &rotation)
    {
        rotations[index] = rotation;
        rotatedIndicies.insert(index);
    }
    void updateTranslation(int index, const QVector3D &translation)
    {
        translations[index] = translation;
        translatedIndicies.insert(index);
    }
    void updateScale(int index, const QVector3D &scale)
    {
        scales[index] = scale;
        scaledIndicies.insert(index);
    }
    std::vector<QQuaternion> rotations;
    std::vector<QVector3D> translations;
    std::vector<QVector3D> scales;
    std::set<int> rotatedIndicies;
    std::set<int> translatedIndicies;
    std::set<int> scaledIndicies;
};

class RigController
{
public:
    RigController(const JointNodeTree &jointNodeTree);
    void saveFrame(RigFrame &frame);
    void frameToMatrices(const RigFrame &frame, std::vector<QMatrix4x4> &matrices);
    void idle(float amount);
    void breathe(float amount);
    void resetFrame();
private:
    void prepare();
    void collectParts();
    int addLeg(std::pair<int, int> legStart, std::pair<int, int> legEnd);
    void liftLegEnd(int leg, QVector3D offset, QVector3D &effectedOffset);
    void liftLegs(QVector3D offset, QVector3D &effectedOffset);
    void lift(QVector3D offset);
    void calculateAverageLegHeight();
    QVector3D calculateAverageLegStartPosition(JointNodeTree &jointNodeTree);
    QVector3D calculateAverageLegEndPosition(JointNodeTree &jointNodeTree);
    void frameToMatricesAtJoint(const RigFrame &frame, std::vector<QMatrix4x4> &matrices, int jointIndex, const QMatrix4x4 &parentWorldMatrix);
    void applyRigFrameToJointNodeTree(JointNodeTree &jointNodeTree, const RigFrame &frame);
private:
    JointNodeTree m_inputJointNodeTree;
    bool m_prepared;
    float m_legHeight;
    float m_averageLegEndY;
private:
    std::vector<std::tuple<int, int, int, int>> m_legs;
    std::vector<std::pair<int, int>> m_spine;
    RigFrame m_rigFrame;
};

#endif
