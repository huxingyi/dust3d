#include <QMatrix3x3>
#include "jointnodetree.h"
#include "util.h"

const std::vector<JointNode> &JointNodeTree::nodes() const
{
    return m_boneNodes;
}

void JointNodeTree::updateRotation(int index, const QQuaternion &rotation)
{
    m_boneNodes[index].rotation = rotation;
}

void JointNodeTree::updateTranslation(int index, const QVector3D &translation)
{
    m_boneNodes[index].translation = translation;
}

void JointNodeTree::updateMatrix(int index, const QMatrix4x4 &matrix)
{
    updateTranslation(index, 
        QVector3D(matrix(0, 3), matrix(1, 3), matrix(2, 3)));
    
    float scalar = std::sqrt(std::max(0.0f, 1.0f + matrix(0, 0) + matrix(1, 1) + matrix(2, 2))) / 2.0f;
    float x = std::sqrt(std::max(0.0f, 1.0f + matrix(0, 0) - matrix(1, 1) - matrix(2, 2))) / 2.0f;
    float y = std::sqrt(std::max(0.0f, 1.0f - matrix(0, 0) + matrix(1, 1) - matrix(2, 2))) / 2.0f;
    float z = std::sqrt(std::max(0.0f, 1.0f - matrix(0, 0) - matrix(1, 1) + matrix(2, 2))) / 2.0f;
    x *= x * (matrix(2, 1) - matrix(1, 2)) > 0 ? 1 : -1;
    y *= y * (matrix(0, 2) - matrix(2, 0)) > 0 ? 1 : -1;
    z *= z * (matrix(1, 0) - matrix(0, 1)) > 0 ? 1 : -1;
    float length = std::sqrt(scalar * scalar + x * x + y * y + z * z);
    updateRotation(index, 
        QQuaternion(scalar / length, x / length, y / length, z / length));
}

JointNodeTree::JointNodeTree(const std::vector<RiggerBone> *resultRigBones)
{
    if (nullptr == resultRigBones || resultRigBones->empty())
        return;
    
    m_boneNodes.resize(resultRigBones->size());
    
    m_boneNodes[0].parentIndex = -1;
    for (decltype(resultRigBones->size()) i = 0; i < resultRigBones->size(); i++) {
        const auto &bone = (*resultRigBones)[i];
        auto &node = m_boneNodes[i];
        node.name = bone.name;
        node.position = bone.headPosition;
        node.children = bone.children;
        for (const auto &childIndex: bone.children)
            m_boneNodes[childIndex].parentIndex = i;
    }
}
