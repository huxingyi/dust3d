#include <QDebug>
#include <QXmlStreamWriter>
#include <QClipboard>
#include <QApplication>
#include <QMimeData>
#include <QRegularExpression>
#include "posedocument.h"
#include "rigger.h"
#include "util.h"
#include "document.h"
#include "snapshot.h"
#include "snapshotxml.h"

const float PoseDocument::m_nodeRadius = 0.01;
const float PoseDocument::m_groundPlaneHalfThickness = 0.01 / 4;
const bool PoseDocument::m_hideRootAndVirtual = true;
const float PoseDocument::m_outcomeScaleFactor = 0.5;

bool PoseDocument::hasPastableNodesInClipboard() const
{
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();
    if (mimeData->hasText()) {
        if (-1 != mimeData->text().indexOf("<pose ") && -1 != mimeData->text().indexOf("<parameter "))
            return true;
    }
    return false;
}

bool PoseDocument::originSettled() const
{
    return false;
}

bool PoseDocument::isNodeEditable(QUuid nodeId) const
{
    Q_UNUSED(nodeId);
    return true;
}

bool PoseDocument::isEdgeEditable(QUuid edgeId) const
{
    Q_UNUSED(edgeId);
    return true;
}

void PoseDocument::copyNodes(std::set<QUuid> nodeIdSet) const
{
    std::map<QString, std::map<QString, QString>> parameters;
    toParameters(parameters, nodeIdSet);
    if (parameters.empty())
        return;
    Document document;
    QUuid poseId = QUuid::createUuid();
    auto &pose = document.poseMap[poseId];
    pose.id = poseId;
    pose.frames.push_back({std::map<QString, QString>(), parameters});
    document.poseIdList.push_back(poseId);
    
    Snapshot snapshot;
    std::set<QUuid> limitPoseIds;
    document.toSnapshot(&snapshot, limitPoseIds, DocumentToSnapshotFor::Poses);
    QString snapshotXml;
    QXmlStreamWriter xmlStreamWriter(&snapshotXml);
    saveSkeletonToXmlStream(&snapshot, &xmlStreamWriter);
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(snapshotXml);
}

void PoseDocument::saveHistoryItem()
{
    PoseHistoryItem item;
    toParameters(item.parameters);
    m_undoItems.push_back(item);
}

bool PoseDocument::undoable() const
{
    return m_undoItems.size() >= 2;
}

bool PoseDocument::redoable() const
{
    return !m_redoItems.empty();
}

void PoseDocument::undo()
{
    if (!undoable())
        return;
    m_redoItems.push_back(m_undoItems.back());
    m_undoItems.pop_back();
    const auto &item = m_undoItems.back();
    fromParameters(&m_riggerBones, item.parameters);
}

void PoseDocument::redo()
{
    if (m_redoItems.empty())
        return;
    m_undoItems.push_back(m_redoItems.back());
    const auto &item = m_redoItems.back();
    fromParameters(&m_riggerBones, item.parameters);
    m_redoItems.pop_back();
}

void PoseDocument::paste()
{
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();
    if (mimeData->hasText()) {
        QXmlStreamReader xmlStreamReader(mimeData->text());
        Snapshot snapshot;
        loadSkeletonFromXmlStream(&snapshot, xmlStreamReader);
        if (snapshot.poses.empty())
            return;
        const auto &firstPose = *snapshot.poses.begin();
        if (firstPose.second.empty())
            return;
        const auto &firstFrame = *firstPose.second.begin();
        fromParameters(&m_riggerBones, firstFrame.second);
        saveHistoryItem();
    }
}

void PoseDocument::updateTurnaround(const QImage &image)
{
    turnaround = image;
    emit turnaroundChanged();
}

void PoseDocument::reset()
{
    nodeMap.clear();
    edgeMap.clear();
    partMap.clear();
    m_boneNameToIdsMap.clear();
    m_bonesPartId = QUuid();
    m_groundPartId = QUuid();
    m_groundEdgeId = QUuid();
    emit cleanup();
    emit parametersChanged();
}

void PoseDocument::clearHistories()
{
    m_undoItems.clear();
    m_redoItems.clear();
}

void PoseDocument::fromParameters(const std::vector<RiggerBone> *rigBones,
        const std::map<QString, std::map<QString, QString>> &parameters)
{
    if (nullptr == rigBones || rigBones->empty()) {
        m_riggerBones.clear();
        return;
    }
    
    if (&m_riggerBones != rigBones)
        m_riggerBones = *rigBones;
    
    QVector3D rootTranslation;
    std::vector<RiggerBone> bones = *rigBones;
    for (auto &bone: bones) {
        const auto findParameterResult = parameters.find(bone.name);
        if (findParameterResult == parameters.end())
            continue;
        const auto &map = findParameterResult->second;
        {
            auto findXResult = map.find("fromX");
            auto findYResult = map.find("fromY");
            auto findZResult = map.find("fromZ");
            if (findXResult != map.end() ||
                    findYResult != map.end() ||
                    findZResult != map.end()) {
                bone.headPosition = {
                    valueOfKeyInMapOrEmpty(map, "fromX").toFloat(),
                    valueOfKeyInMapOrEmpty(map, "fromY").toFloat(),
                    valueOfKeyInMapOrEmpty(map, "fromZ").toFloat()
                };
            }
        }
        {
            auto findXResult = map.find("toX");
            auto findYResult = map.find("toY");
            auto findZResult = map.find("toZ");
            if (findXResult != map.end() ||
                    findYResult != map.end() ||
                    findZResult != map.end()) {
                QVector3D toPosition = {
                    valueOfKeyInMapOrEmpty(map, "toX").toFloat(),
                    valueOfKeyInMapOrEmpty(map, "toY").toFloat(),
                    valueOfKeyInMapOrEmpty(map, "toZ").toFloat()
                };
                bone.tailPosition = toPosition;
                for (const auto &child: bone.children) {
                    auto &childBone = bones[child];
                    childBone.headPosition = toPosition;
                }
            }
        }
    }
    
    const auto findRoot = parameters.find(Rigger::rootBoneName);
    if (findRoot != parameters.end()) {
        const auto &map = findRoot->second;
        {
            auto findXResult = map.find("translateX");
            auto findYResult = map.find("translateY");
            auto findZResult = map.find("translateZ");
            if (findXResult != map.end() ||
                    findYResult != map.end() ||
                    findZResult != map.end()) {
                rootTranslation = {
                    valueOfKeyInMapOrEmpty(map, "translateX").toFloat(),
                    valueOfKeyInMapOrEmpty(map, "translateY").toFloat(),
                    valueOfKeyInMapOrEmpty(map, "translateZ").toFloat()
                };
            }
        }
    }
    
    updateRigBones(&bones, rootTranslation);
}

void PoseDocument::updateRigBones(const std::vector<RiggerBone> *rigBones, const QVector3D &rootTranslation)
{
    reset();
    
    if (nullptr == rigBones || rigBones->empty()) {
        return;
    }
    
    std::set<QUuid> newAddedNodeIds;
    std::set<QUuid> newAddedEdgeIds;
    
    m_bonesPartId = QUuid::createUuid();
    auto &bonesPart = partMap[m_bonesPartId];
    bonesPart.id = m_bonesPartId;
    
    //qDebug() << "rigBones size:" << rigBones->size();
    
    std::vector<std::pair<int, int>> edgePairs;
    for (size_t i = m_hideRootAndVirtual ? 1 : 0; i < rigBones->size(); ++i) {
        const auto &bone = (*rigBones)[i];
        for (const auto &child: bone.children) {
            //qDebug() << "Add pair:" << bone.name << "->" << (*rigBones)[child].name;
            edgePairs.push_back({i, child});
        }
    }
    std::map<int, QUuid> boneIndexToHeadNodeIdMap;
    for (const auto &edgePair: edgePairs) {
        QUuid firstNodeId, secondNodeId;
        auto findFirst = boneIndexToHeadNodeIdMap.find(edgePair.first);
        if (findFirst == boneIndexToHeadNodeIdMap.end()) {
            const auto &bone = (*rigBones)[edgePair.first];
            if (!bone.name.startsWith("Virtual_") || !m_hideRootAndVirtual) {
                SkeletonNode node;
                node.partId = m_bonesPartId;
                node.id = QUuid::createUuid();
                node.setRadius(m_nodeRadius);
                node.x = fromOutcomeX(bone.headPosition.x());
                node.y = fromOutcomeY(bone.headPosition.y());
                node.z = fromOutcomeZ(bone.headPosition.z());
                nodeMap[node.id] = node;
                newAddedNodeIds.insert(node.id);
                boneIndexToHeadNodeIdMap[edgePair.first] = node.id;
                firstNodeId = node.id;
            }
        } else {
            firstNodeId = findFirst->second;
        }
        auto findSecond = boneIndexToHeadNodeIdMap.find(edgePair.second);
        if (findSecond == boneIndexToHeadNodeIdMap.end()) {
            const auto &bone = (*rigBones)[edgePair.second];
            if (!bone.name.startsWith("Virtual_") || !m_hideRootAndVirtual) {
                SkeletonNode node;
                node.partId = m_bonesPartId;
                node.id = QUuid::createUuid();
                node.setRadius(m_nodeRadius);
                node.x = fromOutcomeX(bone.headPosition.x());
                node.y = fromOutcomeY(bone.headPosition.y());
                node.z = fromOutcomeZ(bone.headPosition.z());
                nodeMap[node.id] = node;
                newAddedNodeIds.insert(node.id);
                boneIndexToHeadNodeIdMap[edgePair.second] = node.id;
                secondNodeId = node.id;
            }
        } else {
            secondNodeId = findSecond->second;
        }
        
        if (firstNodeId.isNull() || secondNodeId.isNull())
            continue;
        
        SkeletonEdge edge;
        edge.partId = m_bonesPartId;
        edge.id = QUuid::createUuid();
        edge.nodeIds.push_back(firstNodeId);
        edge.nodeIds.push_back(secondNodeId);
        edgeMap[edge.id] = edge;
        newAddedEdgeIds.insert(edge.id);
        nodeMap[firstNodeId].edgeIds.push_back(edge.id);
        nodeMap[secondNodeId].edgeIds.push_back(edge.id);
    }
    
    for (size_t i = m_hideRootAndVirtual ? 1 : 0; i < rigBones->size(); ++i) {
        const auto &bone = (*rigBones)[i];
        if (m_hideRootAndVirtual && bone.name.startsWith("Virtual_"))
            continue;
        if (bone.children.empty()) {
            const QUuid &firstNodeId = boneIndexToHeadNodeIdMap[i];
            
            SkeletonNode node;
            node.partId = m_bonesPartId;
            node.id = QUuid::createUuid();
            node.setRadius(m_nodeRadius / 2);
            node.x = fromOutcomeX(bone.tailPosition.x());
            node.y = fromOutcomeY(bone.tailPosition.y());
            node.z = fromOutcomeZ(bone.tailPosition.z());
            nodeMap[node.id] = node;
            newAddedNodeIds.insert(node.id);
            m_boneNameToIdsMap[bone.name] = {firstNodeId, node.id};
            
            SkeletonEdge edge;
            edge.partId = m_bonesPartId;
            edge.id = QUuid::createUuid();
            edge.nodeIds.push_back(firstNodeId);
            edge.nodeIds.push_back(node.id);
            edgeMap[edge.id] = edge;
            newAddedEdgeIds.insert(edge.id);
            nodeMap[firstNodeId].edgeIds.push_back(edge.id);
            nodeMap[node.id].edgeIds.push_back(edge.id);
            
            //qDebug() << "Add pair:" << bone.name << "->" << "~";
            continue;
        }
        for (const auto &child: bone.children) {
            m_boneNameToIdsMap[bone.name] = {boneIndexToHeadNodeIdMap[i], boneIndexToHeadNodeIdMap[child]};
        }
    }
    
    auto findRootNodeId = boneIndexToHeadNodeIdMap.find(0);
    if (findRootNodeId != boneIndexToHeadNodeIdMap.end()) {
        nodeMap[findRootNodeId->second].setRadius(m_nodeRadius * 2);
    }
    
    m_groundPartId = QUuid::createUuid();
    auto &groundPart = partMap[m_groundPartId];
    groundPart.id = m_groundPartId;
    
    float groundY = findGroundY() + rootTranslation.y();
    
    std::pair<QUuid, QUuid> groundNodesPair;
    {
        SkeletonNode node;
        node.partId = m_groundPartId;
        node.id = QUuid::createUuid();
        node.setRadius(m_groundPlaneHalfThickness);
        node.x = -100;
        node.y = groundY + m_groundPlaneHalfThickness;
        node.z = -100;
        nodeMap[node.id] = node;
        newAddedNodeIds.insert(node.id);
        groundNodesPair.first = node.id;
    }
    
    {
        SkeletonNode node;
        node.partId = m_groundPartId;
        node.id = QUuid::createUuid();
        node.setRadius(m_groundPlaneHalfThickness);
        node.x = 100;
        node.y = groundY + m_groundPlaneHalfThickness;
        node.z = 100;
        nodeMap[node.id] = node;
        newAddedNodeIds.insert(node.id);
        groundNodesPair.second = node.id;
    }
    
    {
        SkeletonEdge edge;
        edge.partId = m_groundPartId;
        edge.id = QUuid::createUuid();
        edge.nodeIds.push_back(groundNodesPair.first);
        edge.nodeIds.push_back(groundNodesPair.second);
        edgeMap[edge.id] = edge;
        m_groundEdgeId = edge.id;
        newAddedEdgeIds.insert(edge.id);
        nodeMap[groundNodesPair.first].edgeIds.push_back(edge.id);
        nodeMap[groundNodesPair.second].edgeIds.push_back(edge.id);
    }
    
    for (const auto &nodeIt: newAddedNodeIds) {
        emit nodeAdded(nodeIt);
    }
    for (const auto &edgeIt: newAddedEdgeIds) {
        emit edgeAdded(edgeIt);
    }
    
    emit parametersChanged();
}

void PoseDocument::moveNodeBy(QUuid nodeId, float x, float y, float z)
{
    auto it = nodeMap.find(nodeId);
    if (it == nodeMap.end()) {
        qDebug() << "Find node failed:" << nodeId;
        return;
    }
    it->second.x += x;
    it->second.y += y;
    it->second.z += z;
    emit nodeOriginChanged(it->first);
    emit parametersChanged();
}

void PoseDocument::setNodeOrigin(QUuid nodeId, float x, float y, float z)
{
    auto it = nodeMap.find(nodeId);
    if (it == nodeMap.end()) {
        qDebug() << "Find node failed:" << nodeId;
        return;
    }
    it->second.x = x;
    it->second.y = y;
    it->second.z = z;
    auto part = partMap.find(it->second.partId);
    if (part != partMap.end())
        part->second.dirty = true;
    emit nodeOriginChanged(nodeId);
    emit parametersChanged();
}

float PoseDocument::findGroundY() const
{
    auto maxY = std::numeric_limits<float>::lowest();
    for (const auto &nodeIt: nodeMap) {
        if (nodeIt.second.partId != m_bonesPartId)
            continue;
        auto y = nodeIt.second.y + nodeIt.second.radius;
        if (y > maxY)
            maxY = y;
    }
    return maxY;
}

void PoseDocument::toParameters(std::map<QString, std::map<QString, QString>> &parameters, const std::set<QUuid> &limitNodeIds) const
{
    float translateY = 0;
    auto findGroundEdge = edgeMap.find(m_groundEdgeId);
    if (findGroundEdge != edgeMap.end()) {
        const auto &nodeIds = findGroundEdge->second.nodeIds;
        if (nodeIds.size() == 2) {
            auto findFirstNode = nodeMap.find(nodeIds[0]);
            auto findSecondNode = nodeMap.find(nodeIds[1]);
            if (findFirstNode != nodeMap.end() && findSecondNode != nodeMap.end()) {
                if (limitNodeIds.empty() || limitNodeIds.find(findFirstNode->first) != limitNodeIds.end() ||
                        limitNodeIds.find(findSecondNode->first) != limitNodeIds.end())
                translateY = (findFirstNode->second.y + findSecondNode->second.y) / 2 -
                    (findGroundY() + m_groundPlaneHalfThickness);
            }
        }
    }
    if (!qFuzzyIsNull(translateY)) {
        auto &boneParameter = parameters[Rigger::rootBoneName];
        boneParameter["translateY"] = QString::number(translateY);
    }
    for (const auto &item: m_boneNameToIdsMap) {
        const auto &boneNodeIdPair = item.second;
        auto findFirstNode = nodeMap.find(boneNodeIdPair.first);
        if (findFirstNode == nodeMap.end())
            continue;
        auto findSecondNode = nodeMap.find(boneNodeIdPair.second);
        if (findSecondNode == nodeMap.end())
            continue;
        if (limitNodeIds.empty() || limitNodeIds.find(boneNodeIdPair.first) != limitNodeIds.end() ||
                limitNodeIds.find(boneNodeIdPair.second) != limitNodeIds.end()) {
            auto &boneParameter = parameters[item.first];
            boneParameter["fromX"] = QString::number(toOutcomeX(findFirstNode->second.x));
            boneParameter["fromY"] = QString::number(toOutcomeY(findFirstNode->second.y));
            boneParameter["fromZ"] = QString::number(toOutcomeZ(findFirstNode->second.z));
            boneParameter["toX"] = QString::number(toOutcomeX(findSecondNode->second.x));
            boneParameter["toY"] = QString::number(toOutcomeY(findSecondNode->second.y));
            boneParameter["toZ"] = QString::number(toOutcomeZ(findSecondNode->second.z));
        }
    }
}

float PoseDocument::fromOutcomeX(float x)
{
    return x * m_outcomeScaleFactor + 0.5;
}

float PoseDocument::toOutcomeX(float x)
{
    return (x - 0.5) / m_outcomeScaleFactor;
}

float PoseDocument::fromOutcomeY(float y)
{
    return -y * m_outcomeScaleFactor + 0.5;
}

float PoseDocument::toOutcomeY(float y)
{
    return (0.5 - y) / m_outcomeScaleFactor;
}

float PoseDocument::fromOutcomeZ(float z)
{
    return -z * m_outcomeScaleFactor + 1;
}

float PoseDocument::toOutcomeZ(float z)
{
    return (1.0 - z) / m_outcomeScaleFactor;
}

QString PoseDocument::findBoneNameByNodeId(const QUuid &nodeId)
{
    for (const auto &item: m_boneNameToIdsMap) {
        if (nodeId == item.second.first || nodeId == item.second.second)
            return item.first;
    }
    return QString();
}

void PoseDocument::switchChainSide(const std::set<QUuid> nodeIds)
{
    QRegularExpression reJoints("^(Left|Right)([a-zA-Z]+\\d*)_(Joint\\d+)$");
    
    std::set<QString> baseNames;
    for (const auto &nodeId: nodeIds) {
        QString boneName = findBoneNameByNodeId(nodeId);
        if (boneName.isEmpty()) {
            //qDebug() << "Find bone name for node failed:" << nodeId;
            continue;
        }
        
        QRegularExpressionMatch match = reJoints.match(boneName);
        if (!match.hasMatch()) {
            //qDebug() << "Match bone name for side failed:" << boneName;
            continue;
        }
        
        QString baseName = match.captured(2);
        baseNames.insert(baseName);
    }
    
    auto switchYZ = [=](const QUuid &first, const QUuid &second) {
        auto findFirstNode = nodeMap.find(first);
        if (findFirstNode == nodeMap.end())
            return;
        auto findSecondNode = nodeMap.find(second);
        if (findSecondNode == nodeMap.end())
            return;
        std::swap(findFirstNode->second.y, findSecondNode->second.y);
        std::swap(findFirstNode->second.z, findSecondNode->second.z);
        emit nodeOriginChanged(first);
        emit nodeOriginChanged(second);
    };
    
    std::set<std::pair<QUuid, QUuid>> switchPairs;
    for (const auto &baseName: baseNames) {
        for (const auto &item: m_boneNameToIdsMap) {
            QRegularExpressionMatch match = reJoints.match(item.first);
            if (!match.hasMatch())
                continue;
            QString itemSide = match.captured(1);
            QString itemBaseName = match.captured(2);
            QString itemJointName = match.captured(3);
            //qDebug() << "itemSide:" << itemSide << "itemBaseName:" << itemBaseName << "itemJointName:" << itemJointName;
            if (itemBaseName == baseName && "Left" == itemSide) {
                QString otherSide = "Right";
                QString pairedName = otherSide + itemBaseName + "_" + itemJointName;
                const auto findPaired = m_boneNameToIdsMap.find(pairedName);
                if (findPaired == m_boneNameToIdsMap.end()) {
                    qDebug() << "Couldn't find paired name:" << pairedName;
                    continue;
                }
                //qDebug() << "Switched:" << pairedName;
                switchPairs.insert({item.second.first, findPaired->second.first});
                switchPairs.insert({item.second.second, findPaired->second.second});
            }
        }
    }
    
    for (const auto &pair: switchPairs) {
        switchYZ(pair.first, pair.second);
    }
    
    //qDebug() << "switchedPairNum:" << switchPairs.size();
    if (!switchPairs.empty())
        emit parametersChanged();
}
