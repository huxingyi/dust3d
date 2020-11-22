#ifndef DUST3D_JOINT_NODE_TREE_H
#define DUST3D_JOINT_NODE_TREE_H
#include <QMatrix4x4>
#include <vector>
#include <QQuaternion>
#include "rig.h"

struct JointNode
{
    int parentIndex;
    QString name;
    QVector3D position;
    QVector3D bindTranslation;
    QVector3D translation;
    QQuaternion rotation;
    QMatrix4x4 bindMatrix;
    QMatrix4x4 inverseBindMatrix;
    std::vector<int> children;
};

class JointNodeTree
{
public:
    const std::vector<JointNode> &nodes() const;
    JointNodeTree(const std::vector<RigBone> *resultRigBones);
    void updateRotation(int index, const QQuaternion &rotation);
    void updateTranslation(int index, const QVector3D &translation);
    void updateMatrix(int index, const QMatrix4x4 &matrix);
private:
    std::vector<JointNode> m_boneNodes;
};

#endif
