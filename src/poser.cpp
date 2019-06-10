#include <QQuaternion>
#include <QRegularExpression>
#include "poser.h"

Poser::Poser(const std::vector<RiggerBone> &bones) :
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

const RiggerBone *Poser::findBone(const QString &name)
{
    auto findResult = m_boneNameToIndexMap.find(name);
    if (findResult == m_boneNameToIndexMap.end())
        return nullptr;
    return &m_bones[findResult->second];
}

const std::vector<RiggerBone> &Poser::bones() const
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

void Poser::fetchChains(const std::vector<QString> &boneNames, std::map<QString, std::vector<QString>> &chains)
{
    QRegularExpression reJoints("^([a-zA-Z]+\\d*)_Joint\\d+$");
    QRegularExpression reSpine("^([a-zA-Z]+)\\d*$");
    for (const auto &item: boneNames) {
        QRegularExpressionMatch match = reJoints.match(item);
        if (match.hasMatch()) {
            QString name = match.captured(1);
            chains[name].push_back(item);
        } else {
            match = reSpine.match(item);
            if (match.hasMatch()) {
                QString name = match.captured(1);
                chains[name].push_back(item);
            } else if (item.startsWith("Virtual_")) {
                //qDebug() << "Ignore connector:" << item;
            } else {
                qDebug() << "Unrecognized bone name:" << item;
            }
        }
    }
    for (auto &chain: chains) {
        std::sort(chain.second.begin(), chain.second.end(), [](const QString &first, const QString &second) {
            return first < second;
        });
    }
}
