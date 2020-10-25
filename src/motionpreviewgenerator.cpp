#include <QDebug>
#include <QRegularExpression>
#include "motionpreviewgenerator.h"

void MotionPreviewGenerator::process()
{
    generate();
    
    emit finished();
}

void MotionPreviewGenerator::generate()
{
    std::map<QString, std::vector<int>> chains;
    
    QRegularExpression reJoints("^([a-zA-Z]+\\d*)_Joint\\d+$");
    QRegularExpression reSpine("^([a-zA-Z]+)\\d*$");
    for (int index = 0; index < (int)m_bones.size(); ++index) {
        const auto &bone = m_bones[index];
        const auto &item = bone.name;
        QRegularExpressionMatch match = reJoints.match(item);
        if (match.hasMatch()) {
            QString name = match.captured(1);
            chains[name].push_back(index);
        } else {
            match = reSpine.match(item);
            if (match.hasMatch()) {
                QString name = match.captured(1);
                if (item.startsWith(name + "0"))
                    chains[name + "0"].push_back(index);
                else
                    chains[name].push_back(index);
            } else if (item.startsWith("Virtual_")) {
                //qDebug() << "Ignore connector:" << item;
            } else {
                qDebug() << "Unrecognized bone name:" << item;
            }
        }
    }
    
    std::vector<std::pair<int, bool>> spineBones;
    auto findSpine = chains.find("Spine");
    if (findSpine != chains.end()) {
        for (const auto &it: findSpine->second) {
            spineBones.push_back({it, true});
        }
    }
    auto findNeck = chains.find("Neck");
    if (findNeck != chains.end()) {
        for (const auto &it: findNeck->second) {
            spineBones.push_back({it, true});
        }
    }
    std::reverse(spineBones.begin(), spineBones.end());
    auto findSpine0 = chains.find("Spine0");
    if (findSpine0 != chains.end()) {
        for (const auto &it: findSpine0->second) {
            spineBones.push_back({it, false});
        }
    }
    auto findTail = chains.find("Tail");
    if (findTail != chains.end()) {
        for (const auto &it: findTail->second) {
            spineBones.push_back({it, false});
        }
    }
    
    //for (size_t i = 0; i < spineBones.size(); ++i) {
    //    qDebug() << "spineBones[" << i << "]:" << m_bones[spineBones[i].first].name;
    //}
    
    double radiusScale = 0.5;
    
    std::vector<VertebrataMotion::Node> spineNodes;
    if (!spineBones.empty()) {
        const auto &it = spineBones[0];
        if (it.second) {
            spineNodes.push_back({m_bones[it.first].tailPosition,
                m_bones[it.first].tailRadius * radiusScale});
        } else {
            spineNodes.push_back({m_bones[it.first].headPosition,
                m_bones[it.first].headRadius * radiusScale});
        }
    }
    std::map<int, size_t> spineBoneToNodeMap;
    for (size_t i = 0; i < spineBones.size(); ++i) {
        const auto &it = spineBones[i];
        if (it.second) {
            spineBoneToNodeMap[it.first] = i;
            if (m_bones[it.first].name == "Spine1")
                spineBoneToNodeMap[0] = spineNodes.size();
            spineNodes.push_back({m_bones[it.first].headPosition,
                m_bones[it.first].headRadius * radiusScale});
        } else {
            spineNodes.push_back({m_bones[it.first].tailPosition,
                m_bones[it.first].tailRadius * radiusScale});
        }
    }
    m_vertebrataMotion = new VertebrataMotion;
    
    m_vertebrataMotion->setParameters(m_parameters);
    m_vertebrataMotion->setSpineNodes(spineNodes);
    
    double groundY = std::numeric_limits<double>::max();
    for (const auto &it: spineNodes) {
        if (it.position.y() - it.radius < groundY)
            groundY = it.position.y() - it.radius;
    }
    
    for (const auto &chain: chains) {
        std::vector<VertebrataMotion::Node> legNodes;
        VertebrataMotion::Side side;
        if (chain.first.startsWith("LeftLimb")) {
            side = VertebrataMotion::Side::Left;
        } else if (chain.first.startsWith("RightLimb")) {
            side = VertebrataMotion::Side::Right;
        } else {
            continue;
        }
        int virtualBoneIndex = m_bones[chain.second[0]].parent;
        if (-1 == virtualBoneIndex)
            continue;
        int spineBoneIndex = m_bones[virtualBoneIndex].parent;
        auto findSpine = spineBoneToNodeMap.find(spineBoneIndex);
        if (findSpine == spineBoneToNodeMap.end())
            continue;
        legNodes.push_back({m_bones[virtualBoneIndex].headPosition,
            m_bones[virtualBoneIndex].headRadius * radiusScale});
        legNodes.push_back({m_bones[virtualBoneIndex].tailPosition,
            m_bones[virtualBoneIndex].tailRadius * radiusScale});
        for (const auto &it: chain.second) {
            legNodes.push_back({m_bones[it].tailPosition,
                m_bones[it].tailRadius * radiusScale});
        }
        qDebug() << "chain.first:" << chain.first << "findSpine->second:" << findSpine->second << "length:" << legNodes.size();
        m_vertebrataMotion->setLegNodes(findSpine->second, side, legNodes);
        for (const auto &it: legNodes) {
            if (it.position.y() - it.radius < groundY)
                groundY = it.position.y() - it.radius;
        }
    }
    m_vertebrataMotion->setGroundY(groundY);
    m_vertebrataMotion->generate();
    
    qDebug() << "Generate preview done";
}
