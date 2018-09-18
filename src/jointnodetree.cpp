#include "jointnodetree.h"

const std::vector<JointNode> &JointNodeTree::nodes() const
{
    return m_boneNodes;
}

void JointNodeTree::updateRotation(int index, QQuaternion rotation)
{
    m_boneNodes[index].rotation = rotation;
}

void JointNodeTree::reset()
{
    for (auto &node: m_boneNodes) {
        node.rotation = QQuaternion();
    }
}

void JointNodeTree::recalculateTransformMatrices()
{
    for (decltype(m_boneNodes.size()) i = 0; i < m_boneNodes.size(); i++) {
        QMatrix4x4 parentTransformMatrix;
        auto &node = m_boneNodes[i];
        if (node.parentIndex != -1) {
            const auto &parent = m_boneNodes[node.parentIndex];
            parentTransformMatrix = parent.transformMatrix;
        }
        QMatrix4x4 translateMatrix;
        translateMatrix.translate(node.translation);
        QMatrix4x4 rotationMatrix;
        rotationMatrix.rotate(node.rotation);
        node.transformMatrix = parentTransformMatrix * translateMatrix * rotationMatrix;
    }
    for (decltype(m_boneNodes.size()) i = 0; i < m_boneNodes.size(); i++) {
        auto &node = m_boneNodes[i];
        node.transformMatrix *= node.inverseBindMatrix;
    }
}

JointNodeTree::JointNodeTree(const std::vector<AutoRiggerBone> *resultRigBones)
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
    
    for (decltype(resultRigBones->size()) i = 0; i < resultRigBones->size(); i++) {
        QMatrix4x4 parentTransformMatrix;
        auto &node = m_boneNodes[i];
        if (node.parentIndex != -1) {
            const auto &parent = m_boneNodes[node.parentIndex];
            parentTransformMatrix = parent.transformMatrix;
            node.translation = node.position - parent.position;
        } else {
            node.translation = node.position;
        }
        QMatrix4x4 translateMatrix;
        translateMatrix.translate(node.translation);
        node.transformMatrix = parentTransformMatrix * translateMatrix;
        node.inverseBindMatrix = node.transformMatrix.inverted();
    }
}
