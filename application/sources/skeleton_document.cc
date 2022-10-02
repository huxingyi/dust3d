#include <QDebug>
#include "skeleton_document.h"

const SkeletonNode *SkeletonDocument::findNode(dust3d::Uuid nodeId) const
{
    auto it = nodeMap.find(nodeId);
    if (it == nodeMap.end())
        return nullptr;
    return &it->second;
}

const SkeletonEdge *SkeletonDocument::findEdge(dust3d::Uuid edgeId) const
{
    auto it = edgeMap.find(edgeId);
    if (it == edgeMap.end())
        return nullptr;
    return &it->second;
}

const SkeletonPart *SkeletonDocument::findPart(dust3d::Uuid partId) const
{
    auto it = partMap.find(partId);
    if (it == partMap.end())
        return nullptr;
    return &it->second;
}

const SkeletonEdge *SkeletonDocument::findEdgeByNodes(dust3d::Uuid firstNodeId, dust3d::Uuid secondNodeId) const
{
    const SkeletonNode *firstNode = nullptr;
    firstNode = findNode(firstNodeId);
    if (nullptr == firstNode) {
        return nullptr;
    }
    for (auto edgeIdIt = firstNode->edgeIds.begin(); edgeIdIt != firstNode->edgeIds.end(); edgeIdIt++) {
        auto edgeIt = edgeMap.find(*edgeIdIt);
        if (edgeIt == edgeMap.end()) {
            continue;
        }
        if (std::find(edgeIt->second.nodeIds.begin(), edgeIt->second.nodeIds.end(), secondNodeId) != edgeIt->second.nodeIds.end())
            return &edgeIt->second;
    }
    return nullptr;
}

void SkeletonDocument::findAllNeighbors(dust3d::Uuid nodeId, std::set<dust3d::Uuid> &neighbors) const
{
    const auto &node = findNode(nodeId);
    if (nullptr == node) {
        return;
    }
    for (const auto &edgeId: node->edgeIds) {
        const auto &edge = findEdge(edgeId);
        if (nullptr == edge) {
            continue;
        }
        const auto &neighborNodeId = edge->neighborOf(nodeId);
        if (neighborNodeId.isNull()) {
            continue;
        }
        if (neighbors.find(neighborNodeId) != neighbors.end()) {
            continue;
        }
        neighbors.insert(neighborNodeId);
        findAllNeighbors(neighborNodeId, neighbors);
    }
}

bool SkeletonDocument::isNodeConnectable(dust3d::Uuid nodeId) const
{
    const auto &node = findNode(nodeId);
    if (nullptr == node)
        return false;
    if (node->edgeIds.size() < 2)
        return true;
    return false;
}

void SkeletonDocument::reduceNode(dust3d::Uuid nodeId)
{
    const SkeletonNode *node = findNode(nodeId);
    if (nullptr == node) {
        return;
    }
    if (node->edgeIds.size() != 2) {
        return;
    }
    dust3d::Uuid firstEdgeId = node->edgeIds[0];
    dust3d::Uuid secondEdgeId = node->edgeIds[1];
    const SkeletonEdge *firstEdge = findEdge(firstEdgeId);
    if (nullptr == firstEdge) {
        return;
    }
    const SkeletonEdge *secondEdge = findEdge(secondEdgeId);
    if (nullptr == secondEdge) {
        return;
    }
    dust3d::Uuid firstNeighborNodeId = firstEdge->neighborOf(nodeId);
    dust3d::Uuid secondNeighborNodeId = secondEdge->neighborOf(nodeId);
    removeNode(nodeId);
    addEdge(firstNeighborNodeId, secondNeighborNodeId);
}

void SkeletonDocument::breakEdge(dust3d::Uuid edgeId)
{
    const SkeletonEdge *edge = findEdge(edgeId);
    if (nullptr == edge) {
        return;
    }
    if (edge->nodeIds.size() != 2) {
        return;
    }
    dust3d::Uuid firstNodeId = edge->nodeIds[0];
    dust3d::Uuid secondNodeId = edge->nodeIds[1];
    const SkeletonNode *firstNode = findNode(firstNodeId);
    if (nullptr == firstNode) {
        return;
    }
    const SkeletonNode *secondNode = findNode(secondNodeId);
    if (nullptr == secondNode) {
        return;
    }
    QVector3D firstOrigin(firstNode->getX(), firstNode->getY(), firstNode->getZ());
    QVector3D secondOrigin(secondNode->getX(), secondNode->getY(), secondNode->getZ());
    QVector3D middleOrigin = (firstOrigin + secondOrigin) / 2;
    float middleRadius = (firstNode->radius + secondNode->radius) / 2;
    removeEdge(edgeId);
    dust3d::Uuid middleNodeId = createNode(dust3d::Uuid::createUuid(), middleOrigin.x(), middleOrigin.y(), middleOrigin.z(), middleRadius, firstNodeId);
    if (middleNodeId.isNull()) {
        return;
    }
    addEdge(middleNodeId, secondNodeId);
}

void SkeletonDocument::reverseEdge(dust3d::Uuid edgeId)
{
    SkeletonEdge *edge = (SkeletonEdge *)findEdge(edgeId);
    if (nullptr == edge) {
        return;
    }
    if (edge->nodeIds.size() != 2) {
        return;
    }
    std::swap(edge->nodeIds[0], edge->nodeIds[1]);
    auto part = partMap.find(edge->partId);
    if (part != partMap.end())
        part->second.dirty = true;
    emit edgeReversed(edgeId);
    emit skeletonChanged();
}

void SkeletonDocument::removeEdge(dust3d::Uuid edgeId)
{
    const SkeletonEdge *edge = findEdge(edgeId);
    if (nullptr == edge) {
        return;
    }
    if (isPartReadonly(edge->partId))
        return;
    const SkeletonPart *oldPart = findPart(edge->partId);
    if (nullptr == oldPart) {
        return;
    }
    QString nextPartName = oldPart->name;
    dust3d::Uuid oldPartId = oldPart->id;
    std::vector<std::vector<dust3d::Uuid>> groups;
    splitPartByEdge(&groups, edgeId);
    std::vector<std::pair<dust3d::Uuid, size_t>> newPartNodeNumMap;
    std::vector<dust3d::Uuid> newPartIds;
    for (auto groupIt = groups.begin(); groupIt != groups.end(); groupIt++) {
        const auto newUuid = dust3d::Uuid::createUuid();
        SkeletonPart &part = partMap[newUuid];
        part.id = newUuid;
        part.copyAttributes(*oldPart);
        part.name = nextPartName;
        for (auto nodeIdIt = (*groupIt).begin(); nodeIdIt != (*groupIt).end(); nodeIdIt++) {
            auto nodeIt = nodeMap.find(*nodeIdIt);
            if (nodeIt == nodeMap.end()) {
                continue;
            }
            nodeIt->second.partId = part.id;
            part.nodeIds.push_back(nodeIt->first);
            for (auto edgeIdIt = nodeIt->second.edgeIds.begin(); edgeIdIt != nodeIt->second.edgeIds.end(); edgeIdIt++) {
                auto edgeIt = edgeMap.find(*edgeIdIt);
                if (edgeIt == edgeMap.end()) {
                    continue;
                }
                edgeIt->second.partId = part.id;
            }
        }
        addPartToComponent(part.id, findComponentParentId(part.componentId));
        newPartNodeNumMap.push_back({part.id, part.nodeIds.size()});
        newPartIds.push_back(part.id);
        emit partAdded(part.id);
    }
    for (auto nodeIdIt = edge->nodeIds.begin(); nodeIdIt != edge->nodeIds.end(); nodeIdIt++) {
        auto nodeIt = nodeMap.find(*nodeIdIt);
        if (nodeIt == nodeMap.end()) {
            continue;
        }
        nodeIt->second.edgeIds.erase(std::remove(nodeIt->second.edgeIds.begin(), nodeIt->second.edgeIds.end(), edgeId), nodeIt->second.edgeIds.end());
        emit nodeOriginChanged(nodeIt->first);
    }
    edgeMap.erase(edgeId);
    emit edgeRemoved(edgeId);
    removePart(oldPartId);
    
    if (!newPartNodeNumMap.empty()) {
        std::sort(newPartNodeNumMap.begin(), newPartNodeNumMap.end(), [&](
                const std::pair<dust3d::Uuid, size_t> &first, const std::pair<dust3d::Uuid, size_t> &second) {
            return first.second > second.second;
        });
        updateLinkedPart(oldPartId, newPartNodeNumMap[0].first);
    }
    
    emit skeletonChanged();
}

void SkeletonDocument::removeNode(dust3d::Uuid nodeId)
{
    const SkeletonNode *node = findNode(nodeId);
    if (nullptr == node) {
        return;
    }
    if (isPartReadonly(node->partId))
        return;
    const SkeletonPart *oldPart = findPart(node->partId);
    if (nullptr == oldPart) {
        return;
    }
    QString nextPartName = oldPart->name;
    dust3d::Uuid oldPartId = oldPart->id;
    std::vector<std::vector<dust3d::Uuid>> groups;
    splitPartByNode(&groups, nodeId);
    std::vector<std::pair<dust3d::Uuid, size_t>> newPartNodeNumMap;
    std::vector<dust3d::Uuid> newPartIds;
    for (auto groupIt = groups.begin(); groupIt != groups.end(); groupIt++) {
        const auto newUuid = dust3d::Uuid::createUuid();
        SkeletonPart &part = partMap[newUuid];
        part.id = newUuid;
        part.copyAttributes(*oldPart);
        part.name = nextPartName;
        for (auto nodeIdIt = (*groupIt).begin(); nodeIdIt != (*groupIt).end(); nodeIdIt++) {
            auto nodeIt = nodeMap.find(*nodeIdIt);
            if (nodeIt == nodeMap.end()) {
                continue;
            }
            nodeIt->second.partId = part.id;
            part.nodeIds.push_back(nodeIt->first);
            for (auto edgeIdIt = nodeIt->second.edgeIds.begin(); edgeIdIt != nodeIt->second.edgeIds.end(); edgeIdIt++) {
                auto edgeIt = edgeMap.find(*edgeIdIt);
                if (edgeIt == edgeMap.end()) {
                    continue;
                }
                edgeIt->second.partId = part.id;
            }
        }
        addPartToComponent(part.id, findComponentParentId(part.componentId));
        newPartNodeNumMap.push_back({part.id, part.nodeIds.size()});
        newPartIds.push_back(part.id);
        emit partAdded(part.id);
    }
    for (auto edgeIdIt = node->edgeIds.begin(); edgeIdIt != node->edgeIds.end(); edgeIdIt++) {
        auto edgeIt = edgeMap.find(*edgeIdIt);
        if (edgeIt == edgeMap.end()) {
            continue;
        }
        dust3d::Uuid neighborId = edgeIt->second.neighborOf(nodeId);
        auto nodeIt = nodeMap.find(neighborId);
        if (nodeIt == nodeMap.end()) {
            continue;
        }
        nodeIt->second.edgeIds.erase(std::remove(nodeIt->second.edgeIds.begin(), nodeIt->second.edgeIds.end(), *edgeIdIt), nodeIt->second.edgeIds.end());
        edgeMap.erase(*edgeIdIt);
        emit edgeRemoved(*edgeIdIt);
    }
    nodeMap.erase(nodeId);
    emit nodeRemoved(nodeId);
    removePart(oldPartId);
    
    if (!newPartNodeNumMap.empty()) {
        std::sort(newPartNodeNumMap.begin(), newPartNodeNumMap.end(), [&](
                const std::pair<dust3d::Uuid, size_t> &first, const std::pair<dust3d::Uuid, size_t> &second) {
            return first.second > second.second;
        });
        updateLinkedPart(oldPartId, newPartNodeNumMap[0].first);
    }
    
    emit skeletonChanged();
}

void SkeletonDocument::addNode(float x, float y, float z, float radius, dust3d::Uuid fromNodeId)
{
    createNode(dust3d::Uuid::createUuid(), x, y, z, radius, fromNodeId);
}

void SkeletonDocument::addNodeWithId(dust3d::Uuid nodeId, float x, float y, float z, float radius, dust3d::Uuid fromNodeId)
{
    createNode(nodeId, x, y, z, radius, fromNodeId);
}

dust3d::Uuid SkeletonDocument::createNode(dust3d::Uuid nodeId, float x, float y, float z, float radius, dust3d::Uuid fromNodeId)
{
    dust3d::Uuid partId;
    const SkeletonNode *fromNode = nullptr;
    bool newPartAdded = false;
    if (fromNodeId.isNull()) {
        const auto newUuid = dust3d::Uuid::createUuid();
        SkeletonPart &part = partMap[newUuid];
        part.id = newUuid;
        partId = part.id;
        emit partAdded(partId);
        newPartAdded = true;
    } else {
        fromNode = findNode(fromNodeId);
        if (nullptr == fromNode) {
            return dust3d::Uuid();
        }
        partId = fromNode->partId;
        if (isPartReadonly(partId))
            return dust3d::Uuid();
        auto part = partMap.find(partId);
        if (part != partMap.end())
            part->second.dirty = true;
    }
    SkeletonNode node(nodeId);
    node.partId = partId;
    node.setRadius(radius);
    node.setX(x);
    node.setY(y);
    node.setZ(z);
    nodeMap[node.id] = node;
    partMap[partId].nodeIds.push_back(node.id);
    
    emit nodeAdded(node.id);
    
    if (nullptr != fromNode) {
        SkeletonEdge edge;
        edge.partId = partId;
        edge.nodeIds.push_back(fromNode->id);
        edge.nodeIds.push_back(node.id);
        edgeMap[edge.id] = edge;
        
        nodeMap[node.id].edgeIds.push_back(edge.id);
        nodeMap[fromNode->id].edgeIds.push_back(edge.id);
        
        emit edgeAdded(edge.id);
    }
    
    if (newPartAdded)
        addPartToComponent(partId, m_currentCanvasComponentId);
    
    emit skeletonChanged();
    
    return node.id;
}

void SkeletonDocument::joinNodeAndNeiborsToGroup(std::vector<dust3d::Uuid> *group, dust3d::Uuid nodeId, std::set<dust3d::Uuid> *visitMap, dust3d::Uuid noUseEdgeId)
{
    if (nodeId.isNull() || visitMap->find(nodeId) != visitMap->end())
        return;
    const SkeletonNode *node = findNode(nodeId);
    if (nullptr == node) {
        return;
    }
    visitMap->insert(nodeId);
    group->push_back(nodeId);
    for (auto edgeIt = node->edgeIds.begin(); edgeIt != node->edgeIds.end(); edgeIt++) {
        if (noUseEdgeId == *edgeIt)
            continue;
        const SkeletonEdge *edge = findEdge(*edgeIt);
        if (nullptr == edge) {
            continue;
        }
        for (auto nodeIt = edge->nodeIds.begin(); nodeIt != edge->nodeIds.end(); nodeIt++) {
            joinNodeAndNeiborsToGroup(group, *nodeIt, visitMap, noUseEdgeId);
        }
    }
}

void SkeletonDocument::splitPartByNode(std::vector<std::vector<dust3d::Uuid>> *groups, dust3d::Uuid nodeId)
{
    const SkeletonNode *node = findNode(nodeId);
    std::set<dust3d::Uuid> visitMap;
    visitMap.insert(nodeId);
    for (auto edgeIt = node->edgeIds.begin(); edgeIt != node->edgeIds.end(); edgeIt++) {
        std::vector<dust3d::Uuid> group;
        const SkeletonEdge *edge = findEdge(*edgeIt);
        if (nullptr == edge) {
            continue;
        }
        joinNodeAndNeiborsToGroup(&group, edge->neighborOf(nodeId), &visitMap, *edgeIt);
        if (!group.empty())
            groups->push_back(group);
    }
}

void SkeletonDocument::splitPartByEdge(std::vector<std::vector<dust3d::Uuid>> *groups, dust3d::Uuid edgeId)
{
    const SkeletonEdge *edge = findEdge(edgeId);
    if (nullptr == edge) {
        return;
    }
    std::set<dust3d::Uuid> visitMap;
    for (auto nodeIt = edge->nodeIds.begin(); nodeIt != edge->nodeIds.end(); nodeIt++) {
        std::vector<dust3d::Uuid> group;
        joinNodeAndNeiborsToGroup(&group, *nodeIt, &visitMap, edgeId);
        if (!group.empty())
            groups->push_back(group);
    }
}

const SkeletonComponent *SkeletonDocument::findComponentParent(dust3d::Uuid componentId) const
{
    auto component = componentMap.find(componentId);
    if (component == componentMap.end()) {
        return nullptr;
    }
    
    if (component->second.parentId.isNull())
        return &rootComponent;
    
    return (SkeletonComponent *)findComponent(component->second.parentId);
}

dust3d::Uuid SkeletonDocument::findComponentParentId(dust3d::Uuid componentId) const
{
    auto component = componentMap.find(componentId);
    if (component == componentMap.end()) {
        return dust3d::Uuid();
    }
    
    return component->second.parentId;
}

void SkeletonDocument::moveComponentUp(dust3d::Uuid componentId)
{
    SkeletonComponent *parent = (SkeletonComponent *)findComponentParent(componentId);
    if (nullptr == parent)
        return;
    
    dust3d::Uuid parentId = findComponentParentId(componentId);
    
    parent->moveChildUp(componentId);
    parent->dirty = true;
    emit componentChildrenChanged(parentId);
    emit skeletonChanged();
}

void SkeletonDocument::moveComponentDown(dust3d::Uuid componentId)
{
    SkeletonComponent *parent = (SkeletonComponent *)findComponentParent(componentId);
    if (nullptr == parent)
        return;
    
    dust3d::Uuid parentId = findComponentParentId(componentId);
    
    parent->moveChildDown(componentId);
    parent->dirty = true;
    emit componentChildrenChanged(parentId);
    emit skeletonChanged();
}

void SkeletonDocument::moveComponentToTop(dust3d::Uuid componentId)
{
    SkeletonComponent *parent = (SkeletonComponent *)findComponentParent(componentId);
    if (nullptr == parent)
        return;
    
    dust3d::Uuid parentId = findComponentParentId(componentId);
    
    parent->moveChildToTop(componentId);
    parent->dirty = true;
    emit componentChildrenChanged(parentId);
    emit skeletonChanged();
}

void SkeletonDocument::moveComponentToBottom(dust3d::Uuid componentId)
{
    SkeletonComponent *parent = (SkeletonComponent *)findComponentParent(componentId);
    if (nullptr == parent)
        return;
    
    dust3d::Uuid parentId = findComponentParentId(componentId);
    
    parent->moveChildToBottom(componentId);
    parent->dirty = true;
    emit componentChildrenChanged(parentId);
    emit skeletonChanged();
}

void SkeletonDocument::renameComponent(dust3d::Uuid componentId, QString name)
{
    auto component = componentMap.find(componentId);
    if (component == componentMap.end()) {
        return;
    }
    
    if (component->second.name == name)
        return;
    
    if (!name.trimmed().isEmpty())
        component->second.name = name;
    emit componentNameChanged(componentId);
    emit optionsChanged();
}

void SkeletonDocument::setComponentExpandState(dust3d::Uuid componentId, bool expanded)
{
    auto component = componentMap.find(componentId);
    if (component == componentMap.end()) {
        return;
    }
    
    if (component->second.expanded == expanded)
        return;
    
    component->second.expanded = expanded;
    emit componentExpandStateChanged(componentId);
    emit optionsChanged();
}

void SkeletonDocument::createNewComponentAndMoveThisIn(dust3d::Uuid componentId)
{
    auto component = componentMap.find(componentId);
    if (component == componentMap.end()) {
        return;
    }
    
    SkeletonComponent *oldParent = (SkeletonComponent *)findComponentParent(componentId);
    
    SkeletonComponent newParent(dust3d::Uuid::createUuid());
    newParent.name = tr("Group") + " " + QString::number(componentMap.size() - partMap.size() + 1);
    
    oldParent->replaceChild(componentId, newParent.id);
    newParent.parentId = oldParent->id;
    newParent.addChild(componentId);
    auto newParentId = newParent.id;
    componentMap.emplace(newParentId, std::move(newParent));
    
    component->second.parentId = newParentId;
    
    emit componentChildrenChanged(oldParent->id);
    emit componentAdded(newParentId);
    emit optionsChanged();
}

void SkeletonDocument::createNewChildComponent(dust3d::Uuid parentComponentId)
{
    SkeletonComponent *parentComponent = (SkeletonComponent *)findComponent(parentComponentId);
    if (!parentComponent->linkToPartId.isNull()) {
        parentComponentId = parentComponent->parentId;
        parentComponent = (SkeletonComponent *)findComponent(parentComponentId);
    }
    
    SkeletonComponent newComponent(dust3d::Uuid::createUuid());
    newComponent.name = tr("Group") + " " + QString::number(componentMap.size() - partMap.size() + 1);
    
    parentComponent->addChild(newComponent.id);
    newComponent.parentId = parentComponentId;
    
    auto newComponentId = newComponent.id;
    componentMap.emplace(newComponentId, std::move(newComponent));
    
    emit componentChildrenChanged(parentComponentId);
    emit componentAdded(newComponentId);
    emit optionsChanged();
}

void SkeletonDocument::removePart(dust3d::Uuid partId)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        return;
    }
    
    if (!part->second.componentId.isNull()) {
        removeComponent(part->second.componentId);
        return;
    }
    
    removePartDontCareComponent(partId);
}

void SkeletonDocument::removePartDontCareComponent(dust3d::Uuid partId)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        return;
    }
    
    std::vector<dust3d::Uuid> removedNodeIds;
    std::vector<dust3d::Uuid> removedEdgeIds;
    
    for (auto nodeIt = nodeMap.begin(); nodeIt != nodeMap.end();) {
        if (nodeIt->second.partId != partId) {
            nodeIt++;
            continue;
        }
        removedNodeIds.push_back(nodeIt->second.id);
        nodeIt = nodeMap.erase(nodeIt);
    }
    
    for (auto edgeIt = edgeMap.begin(); edgeIt != edgeMap.end();) {
        if (edgeIt->second.partId != partId) {
            edgeIt++;
            continue;
        }
        removedEdgeIds.push_back(edgeIt->second.id);
        edgeIt = edgeMap.erase(edgeIt);
    }
    
    partMap.erase(part);
    
    for (const auto &nodeId: removedNodeIds) {
        emit nodeRemoved(nodeId);
    }
    for (const auto &edgeId: removedEdgeIds) {
        emit edgeRemoved(edgeId);
    }
    emit partRemoved(partId);
}

void SkeletonDocument::addPartToComponent(dust3d::Uuid partId, dust3d::Uuid componentId)
{
    SkeletonComponent child(dust3d::Uuid::createUuid());
    
    if (!componentId.isNull()) {
        auto parentComponent = componentMap.find(componentId);
        if (parentComponent == componentMap.end()) {
            componentId = dust3d::Uuid();
            rootComponent.addChild(child.id);
        } else {
            parentComponent->second.addChild(child.id);
        }
    } else {
        rootComponent.addChild(child.id);
    }
    
    partMap[partId].componentId = child.id;
    child.linkToPartId = partId;
    child.parentId = componentId;
    auto childId = child.id;
    componentMap.emplace(childId, std::move(child));
    
    emit componentChildrenChanged(componentId);
    emit componentAdded(childId);
}

void SkeletonDocument::removeComponent(dust3d::Uuid componentId)
{
    removeComponentRecursively(componentId);
    emit skeletonChanged();
}

void SkeletonDocument::removeComponentRecursively(dust3d::Uuid componentId)
{
    auto component = componentMap.find(componentId);
    if (component == componentMap.end()) {
        return;
    }
    
    if (!component->second.linkToPartId.isNull()) {
        removePartDontCareComponent(component->second.linkToPartId);
    }
    
    auto childrenIds = component->second.childrenIds;
    for (const auto &childId: childrenIds) {
        removeComponentRecursively(childId);
    }
    
    dust3d::Uuid parentId = component->second.parentId;
    if (!parentId.isNull()) {
        auto parentComponent = componentMap.find(parentId);
        if (parentComponent != componentMap.end()) {
            parentComponent->second.dirty = true;
            parentComponent->second.removeChild(componentId);
        }
    } else {
        rootComponent.removeChild(componentId);
    }
    
    componentMap.erase(component);
    emit componentRemoved(componentId);
    emit componentChildrenChanged(parentId);
}

void SkeletonDocument::setCurrentCanvasComponentId(dust3d::Uuid componentId)
{
    m_currentCanvasComponentId = componentId;
    const SkeletonComponent *component = findComponent(m_currentCanvasComponentId);
    if (nullptr == component) {
        m_currentCanvasComponentId = dust3d::Uuid();
    } else {
        if (!component->linkToPartId.isNull()) {
            m_currentCanvasComponentId = component->parentId;
            component = findComponent(m_currentCanvasComponentId);
        }
    }
}

void SkeletonDocument::addComponent(dust3d::Uuid parentId)
{
    SkeletonComponent component(dust3d::Uuid::createUuid());
    
    if (!parentId.isNull()) {
        auto parentComponent = componentMap.find(parentId);
        if (parentComponent == componentMap.end()) {
            return;
        }
        parentComponent->second.addChild(component.id);
    } else {
        rootComponent.addChild(component.id);
    }
    
    component.parentId = parentId;
    auto componentId = component.id;
    componentMap.emplace(componentId, std::move(component));
    
    emit componentChildrenChanged(parentId);
    emit componentAdded(componentId);
}

bool SkeletonDocument::isDescendantComponent(dust3d::Uuid componentId, dust3d::Uuid suspiciousId)
{
    const SkeletonComponent *loopComponent = findComponentParent(suspiciousId);
    while (nullptr != loopComponent) {
        if (loopComponent->id == componentId)
            return true;
        loopComponent = findComponentParent(loopComponent->parentId);
    }
    return false;
}

void SkeletonDocument::moveComponent(dust3d::Uuid componentId, dust3d::Uuid toParentId)
{
    if (componentId == toParentId)
        return;

    auto component = componentMap.find(componentId);
    if (component == componentMap.end()) {
        return;
    }
    
    if (component->second.parentId == toParentId)
        return;
    
    if (isDescendantComponent(componentId, toParentId))
        return;
    
    if (component->second.parentId.isNull()) {
        rootComponent.removeChild(componentId);
        emit componentChildrenChanged(rootComponent.id);
    } else {
        auto oldParent = componentMap.find(component->second.parentId);
        if (oldParent != componentMap.end()) {
            oldParent->second.dirty = true;
            oldParent->second.removeChild(componentId);
            emit componentChildrenChanged(oldParent->second.id);
        }
    }
    
    component->second.parentId = toParentId;
    
    if (toParentId.isNull()) {
        rootComponent.addChild(componentId);
        emit componentChildrenChanged(rootComponent.id);
    } else {
        auto newParent = componentMap.find(toParentId);
        if (newParent != componentMap.end()) {
            newParent->second.dirty = true;
            newParent->second.addChild(componentId);
            emit componentChildrenChanged(newParent->second.id);
        }
    }
    
    emit skeletonChanged();
}

void SkeletonDocument::setPartLockState(dust3d::Uuid partId, bool locked)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        return;
    }
    if (part->second.locked == locked)
        return;
    part->second.locked = locked;
    emit partLockStateChanged(partId);
    emit optionsChanged();
}

void SkeletonDocument::setPartVisibleState(dust3d::Uuid partId, bool visible)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        return;
    }
    if (part->second.visible == visible)
        return;
    part->second.visible = visible;
    emit partVisibleStateChanged(partId);
    emit optionsChanged();
}

void SkeletonDocument::setPartDisableState(dust3d::Uuid partId, bool disabled)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        return;
    }
    if (part->second.disabled == disabled)
        return;
    part->second.disabled = disabled;
    part->second.dirty = true;
    emit partDisableStateChanged(partId);
    emit skeletonChanged();
}

void SkeletonDocument::collectComponentDescendantParts(dust3d::Uuid componentId, std::vector<dust3d::Uuid> &partIds) const
{
    const SkeletonComponent *component = findComponent(componentId);
    if (nullptr == component)
        return;
    
    if (!component->linkToPartId.isNull()) {
        partIds.push_back(component->linkToPartId);
        return;
    }
    
    for (const auto &childId: component->childrenIds) {
        collectComponentDescendantParts(childId, partIds);
    }
}

void SkeletonDocument::collectComponentDescendantComponents(dust3d::Uuid componentId, std::vector<dust3d::Uuid> &componentIds) const
{
    const SkeletonComponent *component = findComponent(componentId);
    if (nullptr == component)
        return;

    if (!component->linkToPartId.isNull()) {
        return;
    }

    for (const auto &childId: component->childrenIds) {
        componentIds.push_back(childId);
        collectComponentDescendantComponents(childId, componentIds);
    }
}

void SkeletonDocument::hideOtherComponents(dust3d::Uuid componentId)
{
    std::vector<dust3d::Uuid> partIds;
    collectComponentDescendantParts(componentId, partIds);
    std::set<dust3d::Uuid> partIdSet;
    for (const auto &partId: partIds) {
        partIdSet.insert(partId);
    }
    for (const auto &part: partMap) {
        if (partIdSet.find(part.first) != partIdSet.end())
            continue;
        setPartVisibleState(part.first, false);
    }
}

void SkeletonDocument::lockOtherComponents(dust3d::Uuid componentId)
{
    std::vector<dust3d::Uuid> partIds;
    collectComponentDescendantParts(componentId, partIds);
    std::set<dust3d::Uuid> partIdSet;
    for (const auto &partId: partIds) {
        partIdSet.insert(partId);
    }
    for (const auto &part: partMap) {
        if (partIdSet.find(part.first) != partIdSet.end())
            continue;
        setPartLockState(part.first, true);
    }
}

void SkeletonDocument::hideAllComponents()
{
    for (const auto &part: partMap) {
        setPartVisibleState(part.first, false);
    }
}

void SkeletonDocument::showAllComponents()
{
    for (const auto &part: partMap) {
        setPartVisibleState(part.first, true);
    }
}

void SkeletonDocument::showOrHideAllComponents()
{
    bool foundVisiblePart = false;
    for (const auto &part: partMap) {
        if (part.second.visible) {
            foundVisiblePart = true;
        }
    }
    if (foundVisiblePart)
        hideAllComponents();
    else
        showAllComponents();
}

void SkeletonDocument::collapseAllComponents()
{
    for (const auto &component: componentMap) {
        if (!component.second.linkToPartId.isNull())
            continue;
        setComponentExpandState(component.first, false);
    }
}

void SkeletonDocument::expandAllComponents()
{
    for (const auto &component: componentMap) {
        if (!component.second.linkToPartId.isNull())
            continue;
        setComponentExpandState(component.first, true);
    }
}

void SkeletonDocument::lockAllComponents()
{
    for (const auto &part: partMap) {
        setPartLockState(part.first, true);
    }
}

void SkeletonDocument::unlockAllComponents()
{
    for (const auto &part: partMap) {
        setPartLockState(part.first, false);
    }
}

void SkeletonDocument::hideDescendantComponents(dust3d::Uuid componentId)
{
    std::vector<dust3d::Uuid> partIds;
    collectComponentDescendantParts(componentId, partIds);
    for (const auto &partId: partIds) {
        setPartVisibleState(partId, false);
    }
}

void SkeletonDocument::showDescendantComponents(dust3d::Uuid componentId)
{
    std::vector<dust3d::Uuid> partIds;
    collectComponentDescendantParts(componentId, partIds);
    for (const auto &partId: partIds) {
        setPartVisibleState(partId, true);
    }
}

void SkeletonDocument::lockDescendantComponents(dust3d::Uuid componentId)
{
    std::vector<dust3d::Uuid> partIds;
    collectComponentDescendantParts(componentId, partIds);
    for (const auto &partId: partIds) {
        setPartLockState(partId, true);
    }
}

void SkeletonDocument::unlockDescendantComponents(dust3d::Uuid componentId)
{
    std::vector<dust3d::Uuid> partIds;
    collectComponentDescendantParts(componentId, partIds);
    for (const auto &partId: partIds) {
        setPartLockState(partId, false);
    }
}

void SkeletonDocument::scaleNodeByAddRadius(dust3d::Uuid nodeId, float amount)
{
    auto it = nodeMap.find(nodeId);
    if (it == nodeMap.end()) {
        return;
    }
    if (isPartReadonly(it->second.partId))
        return;
    if (radiusLocked)
        return;
    it->second.setRadius(it->second.radius + amount);
    auto part = partMap.find(it->second.partId);
    if (part != partMap.end())
        part->second.dirty = true;
    emit nodeRadiusChanged(nodeId);
    emit skeletonChanged();
}

bool SkeletonDocument::isPartReadonly(dust3d::Uuid partId) const
{
    const SkeletonPart *part = findPart(partId);
    if (nullptr == part) {
        return true;
    }
    return part->locked || !part->visible;
}

void SkeletonDocument::moveNodeBy(dust3d::Uuid nodeId, float x, float y, float z)
{
    auto it = nodeMap.find(nodeId);
    if (it == nodeMap.end()) {
        return;
    }
    if (m_allPositionRelatedLocksEnabled && isPartReadonly(it->second.partId))
        return;
    bool changed = false;
    if (!(m_allPositionRelatedLocksEnabled && xlocked)) {
        it->second.addX(x);
        changed = true;
    }
    if (!(m_allPositionRelatedLocksEnabled && ylocked)) {
        it->second.addY(y);
        changed = true;
    }
    if (!(m_allPositionRelatedLocksEnabled && zlocked)) {
        it->second.addZ(z);
        changed = true;
    }
    if (!changed)
        return;
    auto part = partMap.find(it->second.partId);
    if (part != partMap.end())
        part->second.dirty = true;
    emit nodeOriginChanged(nodeId);
    emit skeletonChanged();
}

void SkeletonDocument::moveOriginBy(float x, float y, float z)
{
    if (!(m_allPositionRelatedLocksEnabled && xlocked))
        addOriginX(x);
    if (!(m_allPositionRelatedLocksEnabled && ylocked))
        addOriginY(y);
    if (!(m_allPositionRelatedLocksEnabled && zlocked))
        addOriginZ(z);
    markAllDirty();
    emit originChanged();
    emit skeletonChanged();
}

void SkeletonDocument::setNodeOrigin(dust3d::Uuid nodeId, float x, float y, float z)
{
    auto it = nodeMap.find(nodeId);
    if (it == nodeMap.end()) {
        return;
    }
    if ((m_allPositionRelatedLocksEnabled && isPartReadonly(it->second.partId)))
        return;
    if (!(m_allPositionRelatedLocksEnabled && xlocked))
        it->second.setX(x);
    if (!(m_allPositionRelatedLocksEnabled && ylocked))
        it->second.setY(y);
    if (!(m_allPositionRelatedLocksEnabled && zlocked))
        it->second.setZ(z);
    auto part = partMap.find(it->second.partId);
    if (part != partMap.end())
        part->second.dirty = true;
    emit nodeOriginChanged(nodeId);
    emit skeletonChanged();
}

void SkeletonDocument::setNodeRadius(dust3d::Uuid nodeId, float radius)
{
    auto it = nodeMap.find(nodeId);
    if (it == nodeMap.end()) {
        return;
    }
    if (isPartReadonly(it->second.partId))
        return;
    if (!radiusLocked)
        it->second.setRadius(radius);
    auto part = partMap.find(it->second.partId);
    if (part != partMap.end())
        part->second.dirty = true;
    emit nodeRadiusChanged(nodeId);
    emit skeletonChanged();
}

void SkeletonDocument::switchNodeXZ(dust3d::Uuid nodeId)
{
    auto it = nodeMap.find(nodeId);
    if (it == nodeMap.end()) {
        return;
    }
    if (isPartReadonly(it->second.partId))
        return;
    {
        float oldX = it->second.getX();
        float oldZ = it->second.getZ();
        it->second.setX(oldZ);
        it->second.setZ(oldX);
    }
    auto part = partMap.find(it->second.partId);
    if (part != partMap.end())
        part->second.dirty = true;
    emit nodeOriginChanged(nodeId);
    emit skeletonChanged();
}

const SkeletonComponent *SkeletonDocument::findComponent(dust3d::Uuid componentId) const
{
    if (componentId.isNull())
        return &rootComponent;
    auto it = componentMap.find(componentId);
    if (it == componentMap.end())
        return nullptr;
    return &it->second;
}

void SkeletonDocument::addEdge(dust3d::Uuid fromNodeId, dust3d::Uuid toNodeId)
{
    if (findEdgeByNodes(fromNodeId, toNodeId)) {
        return;
    }
    
    const SkeletonNode *fromNode = nullptr;
    const SkeletonNode *toNode = nullptr;
    bool toPartRemoved = false;
    
    fromNode = findNode(fromNodeId);
    if (nullptr == fromNode) {
        return;
    }
    
    if (isPartReadonly(fromNode->partId))
        return;
    
    toNode = findNode(toNodeId);
    if (nullptr == toNode) {
        return;
    }
    
    if (isPartReadonly(toNode->partId))
        return;
    
    dust3d::Uuid toPartId = toNode->partId;
    
    auto fromPart = partMap.find(fromNode->partId);
    if (fromPart == partMap.end()) {
        return;
    }
    
    fromPart->second.dirty = true;
    
    if (fromNode->partId != toNode->partId) {
        toPartRemoved = true;
        std::vector<dust3d::Uuid> toGroup;
        std::set<dust3d::Uuid> visitMap;
        joinNodeAndNeiborsToGroup(&toGroup, toNodeId, &visitMap);
        for (auto nodeIdIt = toGroup.begin(); nodeIdIt != toGroup.end(); nodeIdIt++) {
            auto nodeIt = nodeMap.find(*nodeIdIt);
            if (nodeIt == nodeMap.end()) {
                continue;
            }
            nodeIt->second.partId = fromNode->partId;
            fromPart->second.nodeIds.push_back(nodeIt->first);
            for (auto edgeIdIt = nodeIt->second.edgeIds.begin(); edgeIdIt != nodeIt->second.edgeIds.end(); edgeIdIt++) {
                auto edgeIt = edgeMap.find(*edgeIdIt);
                if (edgeIt == edgeMap.end()) {
                    continue;
                }
                edgeIt->second.partId = fromNode->partId;
            }
        }
    }
    
    SkeletonEdge edge;
    edge.partId = fromNode->partId;
    edge.nodeIds.push_back(fromNode->id);
    edge.nodeIds.push_back(toNodeId);
    edgeMap[edge.id] = edge;

    nodeMap[toNodeId].edgeIds.push_back(edge.id);
    nodeMap[fromNode->id].edgeIds.push_back(edge.id);
    
    emit edgeAdded(edge.id);
    
    if (toPartRemoved) {
        updateLinkedPart(toPartId, fromNode->partId);
        removePart(toPartId);
    }
    
    emit skeletonChanged();
}

void SkeletonDocument::updateLinkedPart(dust3d::Uuid oldPartId, dust3d::Uuid newPartId)
{
    for (auto &partIt: partMap) {
        if (partIt.second.cutFaceLinkedId == oldPartId) {
            partIt.second.dirty = true;
            partIt.second.setCutFaceLinkedId(newPartId);
        }
    }
    std::set<dust3d::Uuid> dirtyPartIds;
    for (auto &nodeIt: nodeMap) {
        if (nodeIt.second.cutFaceLinkedId == oldPartId) {
            dirtyPartIds.insert(nodeIt.second.partId);
            nodeIt.second.setCutFaceLinkedId(newPartId);
        }
    }
    for (const auto &partId: dirtyPartIds) {
        SkeletonPart *part = (SkeletonPart *)findPart(partId);
        if (nullptr == part)
            continue;
        part->dirty = true;
    }
}

void SkeletonDocument::enableAllPositionRelatedLocks()
{
    m_allPositionRelatedLocksEnabled = true;
}

void SkeletonDocument::disableAllPositionRelatedLocks()
{
    m_allPositionRelatedLocksEnabled = false;
}

void SkeletonDocument::resetDirtyFlags()
{
    for (auto &part: partMap) {
        part.second.dirty = false;
    }
    for (auto &component: componentMap) {
        component.second.dirty = false;
    }
}

void SkeletonDocument::markAllDirty()
{
    for (auto &part: partMap) {
        part.second.dirty = true;
    }
}

void SkeletonDocument::setXlockState(bool locked)
{
    if (xlocked == locked)
        return;
    xlocked = locked;
    emit xlockStateChanged();
}

void SkeletonDocument::setYlockState(bool locked)
{
    if (ylocked == locked)
        return;
    ylocked = locked;
    emit ylockStateChanged();
}

void SkeletonDocument::setZlockState(bool locked)
{
    if (zlocked == locked)
        return;
    zlocked = locked;
    emit zlockStateChanged();
}

void SkeletonDocument::setRadiusLockState(bool locked)
{
    if (radiusLocked == locked)
        return;
    radiusLocked = locked;
    emit radiusLockStateChanged();
}

void SkeletonDocument::setComponentPreviewImage(const dust3d::Uuid &componentId, const QImage &image)
{
    SkeletonComponent *component = (SkeletonComponent *)findComponent(componentId);
    if (nullptr == component)
        return;
    component->isPreviewMeshObsolete = false;
    component->previewPixmap = QPixmap::fromImage(image);
    emit componentPreviewImageChanged(componentId);
}

void SkeletonDocument::setComponentPreviewMesh(const dust3d::Uuid &componentId, std::unique_ptr<ModelMesh> mesh)
{
    SkeletonComponent *component = (SkeletonComponent *)findComponent(componentId);
    if (nullptr == component)
        return;
    component->updatePreviewMesh(std::move(mesh));
    emit componentPreviewMeshChanged(componentId);
}
