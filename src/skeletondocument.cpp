#include <QDebug>
#include "skeletondocument.h"

const SkeletonNode *SkeletonDocument::findNode(QUuid nodeId) const
{
    auto it = nodeMap.find(nodeId);
    if (it == nodeMap.end())
        return nullptr;
    return &it->second;
}

const SkeletonEdge *SkeletonDocument::findEdge(QUuid edgeId) const
{
    auto it = edgeMap.find(edgeId);
    if (it == edgeMap.end())
        return nullptr;
    return &it->second;
}

const SkeletonPart *SkeletonDocument::findPart(QUuid partId) const
{
    auto it = partMap.find(partId);
    if (it == partMap.end())
        return nullptr;
    return &it->second;
}

const SkeletonEdge *SkeletonDocument::findEdgeByNodes(QUuid firstNodeId, QUuid secondNodeId) const
{
    const SkeletonNode *firstNode = nullptr;
    firstNode = findNode(firstNodeId);
    if (nullptr == firstNode) {
        qDebug() << "Find node failed:" << firstNodeId;
        return nullptr;
    }
    for (auto edgeIdIt = firstNode->edgeIds.begin(); edgeIdIt != firstNode->edgeIds.end(); edgeIdIt++) {
        auto edgeIt = edgeMap.find(*edgeIdIt);
        if (edgeIt == edgeMap.end()) {
            qDebug() << "Find edge failed:" << *edgeIdIt;
            continue;
        }
        if (std::find(edgeIt->second.nodeIds.begin(), edgeIt->second.nodeIds.end(), secondNodeId) != edgeIt->second.nodeIds.end())
            return &edgeIt->second;
    }
    return nullptr;
}

void SkeletonDocument::findAllNeighbors(QUuid nodeId, std::set<QUuid> &neighbors) const
{
    const auto &node = findNode(nodeId);
    if (nullptr == node) {
        qDebug() << "findNode:" << nodeId << "failed";
        return;
    }
    for (const auto &edgeId: node->edgeIds) {
        const auto &edge = findEdge(edgeId);
        if (nullptr == edge) {
            qDebug() << "findEdge:" << edgeId << "failed";
            continue;
        }
        const auto &neighborNodeId = edge->neighborOf(nodeId);
        if (neighborNodeId.isNull()) {
            qDebug() << "neighborOf:" << nodeId << "is null from edge:" << edgeId;
            continue;
        }
        if (neighbors.find(neighborNodeId) != neighbors.end()) {
            continue;
        }
        neighbors.insert(neighborNodeId);
        findAllNeighbors(neighborNodeId, neighbors);
    }
}

bool SkeletonDocument::isNodeConnectable(QUuid nodeId) const
{
    return true;
}
