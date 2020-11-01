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
    QVector3D translation;
    QMatrix4x4 transformMatrix;
    QQuaternion rotation;
    std::vector<int> children;
};

class JointNodeTree
{
public:
    const std::vector<JointNode> &nodes() const;
    JointNodeTree(const std::vector<RiggerBone> *resultRigBones);
    void updateRotation(int index, const QQuaternion &rotation);
    void updateTranslation(int index, const QVector3D &translation);
    void updateMatrix(int index, const QMatrix4x4 &matrix);
private:
    std::vector<JointNode> m_boneNodes;
};

#endif
