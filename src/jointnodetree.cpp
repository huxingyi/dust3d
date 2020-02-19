#include "jointnodetree.h"
#include "util.h"

const std::vector<JointNode> &JointNodeTree::nodes() const
{
    return m_boneNodes;
}

void JointNodeTree::updateRotation(int index, QQuaternion rotation)
{
    m_boneNodes[index].rotation = rotation;
}

void JointNodeTree::updateTranslation(int index, QVector3D translation)
{
    m_boneNodes[index].translation = translation;
}

void JointNodeTree::addTranslation(int index, QVector3D translation)
{
    m_boneNodes[index].translation += translation;
}

void JointNodeTree::reset()
{
    for (auto &node: m_boneNodes) {
        node.rotation = QQuaternion();
        node.translation = node.bindTranslation;
    }
}

void JointNodeTree::calculateBonePositions(std::vector<std::pair<QVector3D, QVector3D>> *bonePositions,
    const JointNodeTree *jointNodeTree,
    const std::vector<RiggerBone> *rigBones) const
{
    if (nullptr == bonePositions || nullptr == jointNodeTree || nullptr == rigBones)
        return;
    
    (*bonePositions).resize(jointNodeTree->nodes().size());
    for (int i = 0; i < (int)jointNodeTree->nodes().size(); i++) {
        const auto &node = jointNodeTree->nodes()[i];
        (*bonePositions)[i] = std::make_pair(node.transformMatrix * node.position,
            node.transformMatrix * (node.position + ((*rigBones)[i].tailPosition - (*rigBones)[i].headPosition)));
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
        node.bindTranslation = node.translation;
        QMatrix4x4 translateMatrix;
        translateMatrix.translate(node.translation);
        node.transformMatrix = parentTransformMatrix * translateMatrix;
        node.inverseBindMatrix = node.transformMatrix.inverted();
    }
}

JointNodeTree JointNodeTree::slerp(const JointNodeTree &first, const JointNodeTree &second, float t)
{
    JointNodeTree slerpResult = first;
    for (decltype(first.nodes().size()) i = 0; i < first.nodes().size() && i < second.nodes().size(); i++) {
        slerpResult.updateRotation(i,
            quaternionOvershootSlerp(first.nodes()[i].rotation, second.nodes()[i].rotation, t));
        slerpResult.updateTranslation(i, (first.nodes()[i].translation * (1.0 - t) + second.nodes()[i].translation * t));
    }
    slerpResult.recalculateTransformMatrices();
    return slerpResult;
}

