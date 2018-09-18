#ifndef JOINT_NODE_TREE_H
#define JOINT_NODE_TREE_H
#include <QMatrix4x4>
#include <vector>
#include <QQuaternion>
#include "autorigger.h"

struct JointNode
{
    int parentIndex;
    QString name;
    QVector3D position;
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
    JointNodeTree(const std::vector<AutoRiggerBone> *resultRigBones);
    void updateRotation(int index, QQuaternion rotation);
    void reset();
    void recalculateTransformMatrices();
private:
    std::vector<JointNode> m_boneNodes;
};

#endif
