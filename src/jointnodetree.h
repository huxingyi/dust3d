#ifndef DUST3D_JOINT_NODE_TREE_H
#define DUST3D_JOINT_NODE_TREE_H
#include <QMatrix4x4>
#include <vector>
#include <QQuaternion>
#include "rigger.h"

struct JointNode
{
    int parentIndex;
    QString name;
    QVector3D position;
    QVector3D bindTranslation;
    QVector3D translation;
    QQuaternion rotation;
    QMatrix4x4 transformMatrix;
    QMatrix4x4 inverseBindMatrix;
    std::vector<int> children;
};

class JointNodeTree
{
public:
    const std::vector<JointNode> &nodes() const;
    JointNodeTree(const std::vector<RiggerBone> *resultRigBones);
    void updateRotation(int index, QQuaternion rotation);
    void updateTranslation(int index, QVector3D translation);
    void addTranslation(int index, QVector3D translation);
    void reset();
    void recalculateTransformMatrices();
    void calculateBonePositions(std::vector<std::pair<QVector3D, QVector3D>> *bonePositions,
        const JointNodeTree *jointNodeTree,
        const std::vector<RiggerBone> *rigBones) const;
    static JointNodeTree slerp(const JointNodeTree &first, const JointNodeTree &second, float t);
private:
    std::vector<JointNode> m_boneNodes;
};

#endif
