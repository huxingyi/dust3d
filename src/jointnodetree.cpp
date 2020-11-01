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
    const QMatrix4x4 &localMatrix = matrix;
    
    updateTranslation(index, 
        QVector3D(localMatrix(0, 3), localMatrix(1, 3), localMatrix(2, 3)));
    
    float scalar = std::sqrt(std::max(0.0f, 1.0f + localMatrix(0, 0) + localMatrix(1, 1) + localMatrix(2, 2))) / 2.0f;
    float x = std::sqrt(std::max(0.0f, 1.0f + localMatrix(0, 0) - localMatrix(1, 1) - localMatrix(2, 2))) / 2.0f;
    float y = std::sqrt(std::max(0.0f, 1.0f - localMatrix(0, 0) + localMatrix(1, 1) - localMatrix(2, 2))) / 2.0f;
    float z = std::sqrt(std::max(0.0f, 1.0f - localMatrix(0, 0) - localMatrix(1, 1) + localMatrix(2, 2))) / 2.0f;
    x *= x * (localMatrix(2, 1) - localMatrix(1, 2)) > 0 ? 1 : -1;
    y *= y * (localMatrix(0, 2) - localMatrix(2, 0)) > 0 ? 1 : -1;
    z *= z * (localMatrix(1, 0) - localMatrix(0, 1)) > 0 ? 1 : -1;
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
        QMatrix4x4 parentMatrix;
        if (-1 == node.parentIndex) {
            node.bindTranslation = node.position;
        } else {
            const auto &parentNode = m_boneNodes[node.parentIndex];
            node.bindTranslation = node.position - parentNode.position;
            parentMatrix = parentNode.bindMatrix;
        }
        QMatrix4x4 translationMatrix;
        translationMatrix.translate(node.bindTranslation);
        node.bindMatrix = parentMatrix * translationMatrix;
        node.inverseBindMatrix = node.bindMatrix.inverted();
        node.children = bone.children;
        for (const auto &childIndex: bone.children)
            m_boneNodes[childIndex].parentIndex = i;
    }
}
