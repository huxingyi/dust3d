#include "skeletonsnapshot.h"
#include <assert.h>

SkeletonSnapshot::SkeletonSnapshot() :
    m_boundingBoxResolved(false),
    m_rootNodeResolved(false)
{
}

void joinNodeAndNeighborsToGroup(std::map<QString, std::vector<QString>> &nodeLinkMap,
    std::map<QString, int> &nodeGroupMap,
    const QString &nodeId,
    int groupId,
    std::vector<QString> &group)
{
    group.push_back(nodeId);
    nodeGroupMap[nodeId] = groupId;
    std::vector<QString> *neighbors = &nodeLinkMap[nodeId];
    for (size_t i = 0; i < neighbors->size(); i++) {
        QString neighborNodeId = (*neighbors)[i];
        if (nodeGroupMap.find(neighborNodeId) != nodeGroupMap.end())
            continue;
        nodeGroupMap[neighborNodeId] = groupId;
        joinNodeAndNeighborsToGroup(nodeLinkMap,
            nodeGroupMap,
            neighborNodeId,
            groupId,
            group);
    }
}

void SkeletonSnapshot::splitByConnectivity(std::vector<SkeletonSnapshot> *groups)
{
    std::map<QString, std::vector<QString>> nodeLinkMap;
    std::map<QString, std::map<QString, QString>>::iterator edgeIterator;
    std::map<QString, int> neighborCountMap;
    for (edgeIterator = edges.begin(); edgeIterator != edges.end(); edgeIterator++) {
        neighborCountMap[edgeIterator->second["from"]]++;
        neighborCountMap[edgeIterator->second["to"]]++;
    }
    int nextNewXnodeId = 1;
    std::vector<std::pair<QString, QString>> newPendingEdges;
    for (edgeIterator = edges.begin(); edgeIterator != edges.end(); ) {
        std::map<QString, QString> *oneNode = &nodes[edgeIterator->second["from"]];
        std::map<QString, QString> *linkNode = &nodes[edgeIterator->second["to"]];
        if ("true" != (*oneNode)["isBranch"]) {
            oneNode = &nodes[edgeIterator->second["to"]];
            linkNode = &nodes[edgeIterator->second["from"]];
            if ("true" != (*oneNode)["isBranch"]) {
                edgeIterator++;
                continue;
            } else {
                if (neighborCountMap[(*linkNode)["id"]] < 3) {
                    edgeIterator++;
                    continue;
                }
            }
        } else {
            if (neighborCountMap[(*linkNode)["id"]] < 3) {
                edgeIterator++;
                continue;
            }
        }
        
        if ("red" == (*oneNode)["sideColorName"]) {
            QString nodeId = QString("nodex%1").arg(nextNewXnodeId++);
            std::map<QString, QString> *newNode = &nodes[nodeId];
            *newNode = *linkNode;
            (*newNode)["id"] = nodeId;
            (*newNode)["radius"] = QString("%1").arg((*newNode)["radius"].toFloat() / 5);
            
            std::map<QString, QString> *pairNode = &nodes[(*oneNode)["nextSidePair"]];
            QString pairNodeId = QString("nodex%1").arg(nextNewXnodeId++);
            std::map<QString, QString> *newPairNode = &nodes[pairNodeId];
            *newPairNode = *pairNode;
            (*newPairNode)["id"] = pairNodeId;
            (*newPairNode)["radius"] = QString("%1").arg((*newPairNode)["radius"].toFloat() / 5);
            
            (*newNode)["nextSidePair"] = pairNodeId;
            (*newPairNode)["nextSidePair"] = nodeId;
            
            newPendingEdges.push_back(std::make_pair((*oneNode)["id"], nodeId));
        }
        
        edgeIterator = edges.erase(edgeIterator);
    }
    int nextNewXedgeId = 1;
    for (size_t i = 0; i < newPendingEdges.size(); i++) {
        QString edgeId = QString("edgex%1").arg(nextNewXedgeId++);
        std::map<QString, QString> *newEdge = &edges[edgeId];
        (*newEdge)["id"] = edgeId;
        (*newEdge)["from"] = newPendingEdges[i].first;
        (*newEdge)["to"] = newPendingEdges[i].second;
        
        edgeId = QString("edgex%1").arg(nextNewXedgeId++);
        newEdge = &edges[edgeId];
        (*newEdge)["id"] = edgeId;
        (*newEdge)["from"] = nodes[newPendingEdges[i].first]["nextSidePair"];
        (*newEdge)["to"] = nodes[newPendingEdges[i].second]["nextSidePair"];
    }
    for (edgeIterator = edges.begin(); edgeIterator != edges.end(); edgeIterator++) {
        nodeLinkMap[edgeIterator->second["from"]].push_back(edgeIterator->second["to"]);
        nodeLinkMap[edgeIterator->second["to"]].push_back(edgeIterator->second["from"]);
    }
    std::map<QString, std::map<QString, QString>>::iterator nodeIterator;
    for (nodeIterator = nodes.begin(); nodeIterator != nodes.end(); nodeIterator++) {
        nodeLinkMap[nodeIterator->first].push_back(nodeIterator->second["nextSidePair"]);
    }
    std::map<QString, int> nodeGroupMap;
    std::vector<std::vector<QString>> nameGroups;
    for (nodeIterator = nodes.begin(); nodeIterator != nodes.end(); nodeIterator++) {
        if (nodeGroupMap.find(nodeIterator->first) != nodeGroupMap.end())
            continue;
        std::vector<QString> nameGroup;
        joinNodeAndNeighborsToGroup(nodeLinkMap,
            nodeGroupMap,
            nodeIterator->first,
            nameGroups.size(),
            nameGroup);
        for (size_t i = 0; i < nameGroup.size(); i++) {
            nodeGroupMap[nameGroup[i]] = nameGroups.size();
        }
        nameGroups.push_back(nameGroup);
    }
    groups->resize(nameGroups.size());
    for (edgeIterator = edges.begin(); edgeIterator != edges.end(); edgeIterator++) {
        int groupIndex = nodeGroupMap[edgeIterator->second["from"]];
        (*groups)[groupIndex].edges[edgeIterator->first] = edgeIterator->second;
    }
    for (nodeIterator = nodes.begin(); nodeIterator != nodes.end(); nodeIterator++) {
        int groupIndex = nodeGroupMap[nodeIterator->first];
        (*groups)[groupIndex].nodes[nodeIterator->first] = nodeIterator->second;
    }
    for (size_t i = 0; i < groups->size(); i++) {
        (*groups)[i].canvas = canvas;
    }
}

const QRectF &SkeletonSnapshot::boundingBoxFront()
{
    if (!m_boundingBoxResolved)
        resolveBoundingBox();
    return m_boundingBoxFront;
}

const QRectF &SkeletonSnapshot::boundingBoxSide()
{
    if (!m_boundingBoxResolved)
        resolveBoundingBox();
    return m_boundingBoxSide;
}

void SkeletonSnapshot::resolveBoundingBox()
{
    if (m_boundingBoxResolved)
        return;
    m_boundingBoxResolved = true;
    
    float left = -1;
    float right = -1;
    float top = -1;
    float bottom = -1;
    float zLeft = -1;
    float zRight = -1;
    std::map<QString, std::map<QString, QString>>::iterator nodeIterator;
    for (nodeIterator = nodes.begin(); nodeIterator != nodes.end(); nodeIterator++) {
        printf("loop node:%s color: %s\n", nodeIterator->first.toUtf8().constData(), nodeIterator->second["sideColorName"].toUtf8().constData());
    }
    for (nodeIterator = nodes.begin(); nodeIterator != nodes.end(); nodeIterator++) {
        if ("red" != nodeIterator->second["sideColorName"])
            continue;
        float originX = nodeIterator->second["x"].toFloat();
        float originY = nodeIterator->second["y"].toFloat();
        float originZ = 0;
        QString nextSidePairId = nodeIterator->second["nextSidePair"];
        printf("nextSidePair: %s\n", nextSidePairId.toUtf8().constData());
        std::map<QString, std::map<QString, QString>>::iterator findNextSidePair = nodes.find(nextSidePairId);
        if (findNextSidePair != nodes.end()) {
            originZ = findNextSidePair->second["x"].toFloat();
        }
        if (left < 0 || originX < left) {
            left = originX;
        }
        if (top < 0 || originY < top) {
            top = originY;
        }
        if (originX > right) {
            right = originX;
        }
        if (originY > bottom) {
            bottom = originY;
        }
        if (zLeft < 0 || originZ < zLeft) {
            zLeft = originZ;
        }
        if (originZ > zRight) {
            zRight = originZ;
        }
    }
    
    m_boundingBoxFront = QRectF(left, top, right - left, bottom - top);
    m_boundingBoxSide = QRectF(zLeft, top, zRight - zLeft, bottom - top);
}

QString SkeletonSnapshot::rootNode()
{
    if (!m_rootNodeResolved)
        resolveRootNode();
    return m_rootNode;
}

void SkeletonSnapshot::setRootNode(const QString &nodeName)
{
    assert(!nodeName.isEmpty());
    m_rootNode = nodeName;
    m_rootNodeResolved = true;
}

QString SkeletonSnapshot::findMaxNeighborNumberNode()
{
    std::map<QString, std::map<QString, QString>>::iterator edgeIterator;
    std::map<QString, int> nodeNeighborCountMap;
    for (edgeIterator = edges.begin(); edgeIterator != edges.end(); edgeIterator++) {
        if ("red" != nodes[edgeIterator->second["from"]]["sideColorName"])
            continue;
        nodeNeighborCountMap[edgeIterator->second["from"]]++;
        nodeNeighborCountMap[edgeIterator->second["to"]]++;
    }
    if (nodeNeighborCountMap.size() == 0)
        return "";
    auto x = std::max_element(nodeNeighborCountMap.begin(), nodeNeighborCountMap.end(),
        [](const std::pair<QString, int>& p1, const std::pair<QString, int>& p2) {
            return p1.second < p2.second;
        });
    return x->first;
}

void SkeletonSnapshot::resolveRootNode()
{
    if (m_rootNodeResolved)
        return;
    m_rootNodeResolved = true;
    
    std::map<QString, std::map<QString, QString>>::iterator edgeIterator;
    std::map<QString, int> nodeNeighborCountMap;
    for (edgeIterator = edges.begin(); edgeIterator != edges.end(); edgeIterator++) {
        if ("red" != nodes[edgeIterator->second["from"]]["sideColorName"])
            continue;
        nodeNeighborCountMap[edgeIterator->second["from"]]++;
        nodeNeighborCountMap[edgeIterator->second["to"]]++;
    }
    std::map<QString, std::map<QString, QString>>::iterator nodeIterator;
    
    // First try to select the node with more than 2 neighbors and have the largest radius.
    float maxRadius = 0;
    m_rootNode = "";
    for (nodeIterator = nodes.begin(); nodeIterator != nodes.end(); nodeIterator++) {
        if ("red" != nodeIterator->second["sideColorName"])
            continue;
        if ("true" == nodeIterator->second["isRoot"]) {
            m_rootNode = nodeIterator->first;
            break;
        }
        if (nodeNeighborCountMap[nodeIterator->first] < 3)
            continue;
        float radius = nodeIterator->second["radius"].toFloat();
        if (radius > maxRadius) {
            maxRadius = radius;
            m_rootNode = nodeIterator->first;
        }
    }
    
    if (m_rootNode.isEmpty())
        m_rootNode = findMaxNeighborNumberNode();
}
