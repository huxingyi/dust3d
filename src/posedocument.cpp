#include <QDebug>
#include "posedocument.h"
#include "rigger.h"
#include "util.h"

const float PoseDocument::m_nodeRadius = 0.01;
const float PoseDocument::m_groundPlaneHalfThickness = 0.01 / 4;
const bool PoseDocument::m_hideRootAndVirtual = true;
const float PoseDocument::m_outcomeScaleFactor = 0.5;

bool PoseDocument::hasPastableNodesInClipboard() const
{
    return false;
}

bool PoseDocument::originSettled() const
{
    return false;
}

bool PoseDocument::isNodeEditable(QUuid nodeId) const
{
    return true;
}

bool PoseDocument::isEdgeEditable(QUuid edgeId) const
{
    return true;
}

void PoseDocument::copyNodes(std::set<QUuid> nodeIdSet) const
{
    // TODO:
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
                bone.tailPosition = {
                    valueOfKeyInMapOrEmpty(map, "toX").toFloat(),
                    valueOfKeyInMapOrEmpty(map, "toY").toFloat(),
                    valueOfKeyInMapOrEmpty(map, "toZ").toFloat()
                };
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

void PoseDocument::toParameters(std::map<QString, std::map<QString, QString>> &parameters) const
{
    float translateY = 0;
    auto findGroundEdge = edgeMap.find(m_groundEdgeId);
    if (findGroundEdge != edgeMap.end()) {
        const auto &nodeIds = findGroundEdge->second.nodeIds;
        if (nodeIds.size() == 2) {
            auto findFirstNode = nodeMap.find(nodeIds[0]);
            auto findSecondNode = nodeMap.find(nodeIds[1]);
            if (findFirstNode != nodeMap.end() && findSecondNode != nodeMap.end()) {
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
        auto &boneParameter = parameters[item.first];
        const auto &boneNodeIdPair = item.second;
        auto findFirstNode = nodeMap.find(boneNodeIdPair.first);
        if (findFirstNode == nodeMap.end())
            continue;
        auto findSecondNode = nodeMap.find(boneNodeIdPair.second);
        if (findSecondNode == nodeMap.end())
            continue;
        boneParameter["fromX"] = QString::number(toOutcomeX(findFirstNode->second.x));
        boneParameter["fromY"] = QString::number(toOutcomeY(findFirstNode->second.y));
        boneParameter["fromZ"] = QString::number(toOutcomeZ(findFirstNode->second.z));
        boneParameter["toX"] = QString::number(toOutcomeX(findSecondNode->second.x));
        boneParameter["toY"] = QString::number(toOutcomeY(findSecondNode->second.y));
        boneParameter["toZ"] = QString::number(toOutcomeZ(findSecondNode->second.z));
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

