#include <QQuaternion>
#include "poser.h"

Poser::Poser(const std::vector<AutoRiggerBone> &bones) :
    m_bones(bones),
    m_jointNodeTree(&bones)
{
    for (decltype(m_bones.size()) i = 0; i < m_bones.size(); i++) {
        m_boneNameToIndexMap[m_bones[i].name] = i;
    }
}

Poser::~Poser()
{
}

int Poser::findBoneIndex(const QString &name)
{
    auto findResult = m_boneNameToIndexMap.find(name);
    if (findResult == m_boneNameToIndexMap.end())
        return -1;
    return findResult->second;
}

const AutoRiggerBone *Poser::findBone(const QString &name)
{
    auto findResult = m_boneNameToIndexMap.find(name);
    if (findResult == m_boneNameToIndexMap.end())
        return nullptr;
    return &m_bones[findResult->second];
}

const std::vector<AutoRiggerBone> &Poser::bones() const
{
    return m_bones;
}

const std::vector<JointNode> &Poser::resultNodes() const
{
    return m_jointNodeTree.nodes();
}

const JointNodeTree &Poser::resultJointNodeTree() const
{
    return m_jointNodeTree;
}

void Poser::commit()
{
    m_jointNodeTree.recalculateTransformMatrices();
}

void Poser::reset()
{
    m_jointNodeTree.reset();
}

std::map<QString, std::map<QString, QString>> &Poser::parameters()
{
    return m_parameters;
}
