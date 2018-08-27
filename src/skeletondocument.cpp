#include <QFileDialog>
#include <QDebug>
#include <QThread>
#include <QGuiApplication>
#include <QClipboard>
#include <QMimeData>
#include <QApplication>
#include <QVector3D>
#include "skeletondocument.h"
#include "dust3dutil.h"
#include "skeletonxml.h"

unsigned long SkeletonDocument::m_maxSnapshot = 1000;

SkeletonDocument::SkeletonDocument() :
    // public
    originX(0),
    originY(0),
    originZ(0),
    editMode(SkeletonDocumentEditMode::Select),
    xlocked(false),
    ylocked(false),
    zlocked(false),
    textureGuideImage(nullptr),
    textureImage(nullptr),
    textureBorderImage(nullptr),
    textureAmbientOcclusionImage(nullptr),
    textureColorImage(nullptr),
    // private
    m_isResultMeshObsolete(false),
    m_meshGenerator(nullptr),
    m_resultMesh(nullptr),
    m_batchChangeRefCount(0),
    m_currentMeshResultContext(nullptr),
    m_isTextureObsolete(false),
    m_textureGenerator(nullptr),
    m_isPostProcessResultObsolete(false),
    m_postProcessor(nullptr),
    m_postProcessedResultContext(new MeshResultContext),
    m_resultTextureMesh(nullptr),
    m_textureImageUpdateVersion(0),
    m_ambientOcclusionBaker(nullptr),
    m_ambientOcclusionBakedImageUpdateVersion(0),
    m_sharedContextWidget(nullptr)
{
}

SkeletonDocument::~SkeletonDocument()
{
    delete m_resultMesh;
    delete m_postProcessedResultContext;
    delete textureGuideImage;
    delete textureImage;
    delete textureBorderImage;
    delete textureAmbientOcclusionImage;
    delete m_resultTextureMesh;
}

void SkeletonDocument::uiReady()
{
    qDebug() << "uiReady";
    emit editModeChanged();
}

void SkeletonDocument::breakEdge(QUuid edgeId)
{
    const SkeletonEdge *edge = findEdge(edgeId);
    if (nullptr == edge) {
        qDebug() << "Find edge failed:" << edgeId;
        return;
    }
    if (edge->nodeIds.size() != 2) {
        return;
    }
    QUuid firstNodeId = edge->nodeIds[0];
    QUuid secondNodeId = edge->nodeIds[1];
    const SkeletonNode *firstNode = findNode(firstNodeId);
    if (nullptr == firstNode) {
        qDebug() << "Find node failed:" << firstNodeId;
        return;
    }
    const SkeletonNode *secondNode = findNode(secondNodeId);
    if (nullptr == secondNode) {
        qDebug() << "Find node failed:" << secondNodeId;
        return;
    }
    QVector3D firstOrigin(firstNode->x, firstNode->y, firstNode->z);
    QVector3D secondOrigin(secondNode->x, secondNode->y, secondNode->z);
    QVector3D middleOrigin = (firstOrigin + secondOrigin) / 2;
    float middleRadius = (firstNode->radius + secondNode->radius) / 2;
    removeEdge(edgeId);
    QUuid middleNodeId = createNode(middleOrigin.x(), middleOrigin.y(), middleOrigin.z(), middleRadius, firstNodeId);
    if (middleNodeId.isNull()) {
        qDebug() << "Add middle node failed";
        return;
    }
    addEdge(middleNodeId, secondNodeId);
}

void SkeletonDocument::removeEdge(QUuid edgeId)
{
    const SkeletonEdge *edge = findEdge(edgeId);
    if (nullptr == edge) {
        qDebug() << "Find edge failed:" << edgeId;
        return;
    }
    if (isPartReadonly(edge->partId))
        return;
    const SkeletonPart *oldPart = findPart(edge->partId);
    if (nullptr == oldPart) {
        qDebug() << "Find part failed:" << edge->partId;
        return;
    }
    QString nextPartName = oldPart->name;
    QUuid oldPartId = oldPart->id;
    std::vector<std::vector<QUuid>> groups;
    splitPartByEdge(&groups, edgeId);
    for (auto groupIt = groups.begin(); groupIt != groups.end(); groupIt++) {
        SkeletonPart part;
        part.copyAttributes(*oldPart);
        part.name = nextPartName;
        for (auto nodeIdIt = (*groupIt).begin(); nodeIdIt != (*groupIt).end(); nodeIdIt++) {
            auto nodeIt = nodeMap.find(*nodeIdIt);
            if (nodeIt == nodeMap.end()) {
                qDebug() << "Find node failed:" << *nodeIdIt;
                continue;
            }
            nodeIt->second.partId = part.id;
            part.nodeIds.push_back(nodeIt->first);
            for (auto edgeIdIt = nodeIt->second.edgeIds.begin(); edgeIdIt != nodeIt->second.edgeIds.end(); edgeIdIt++) {
                auto edgeIt = edgeMap.find(*edgeIdIt);
                if (edgeIt == edgeMap.end()) {
                    qDebug() << "Find edge failed:" << *edgeIdIt;
                    continue;
                }
                edgeIt->second.partId = part.id;
            }
        }
        partMap[part.id] = part;
        addPartToComponent(part.id, findComponentParentId(part.componentId));
        emit partAdded(part.id);
    }
    for (auto nodeIdIt = edge->nodeIds.begin(); nodeIdIt != edge->nodeIds.end(); nodeIdIt++) {
        auto nodeIt = nodeMap.find(*nodeIdIt);
        if (nodeIt == nodeMap.end()) {
            qDebug() << "Find node failed:" << *nodeIdIt;
            continue;
        }
        nodeIt->second.edgeIds.erase(std::remove(nodeIt->second.edgeIds.begin(), nodeIt->second.edgeIds.end(), edgeId), nodeIt->second.edgeIds.end());
        emit nodeOriginChanged(nodeIt->first);
    }
    edgeMap.erase(edgeId);
    emit edgeRemoved(edgeId);
    removePart(oldPartId);
    
    emit skeletonChanged();
}

void SkeletonDocument::removeNode(QUuid nodeId)
{
    const SkeletonNode *node = findNode(nodeId);
    if (nullptr == node) {
        qDebug() << "Find node failed:" << nodeId;
        return;
    }
    if (isPartReadonly(node->partId))
        return;
    const SkeletonPart *oldPart = findPart(node->partId);
    if (nullptr == oldPart) {
        qDebug() << "Find part failed:" << node->partId;
        return;
    }
    QString nextPartName = oldPart->name;
    QUuid oldPartId = oldPart->id;
    std::vector<std::vector<QUuid>> groups;
    splitPartByNode(&groups, nodeId);
    for (auto groupIt = groups.begin(); groupIt != groups.end(); groupIt++) {
        SkeletonPart part;
        part.copyAttributes(*oldPart);
        part.name = nextPartName;
        for (auto nodeIdIt = (*groupIt).begin(); nodeIdIt != (*groupIt).end(); nodeIdIt++) {
            auto nodeIt = nodeMap.find(*nodeIdIt);
            if (nodeIt == nodeMap.end()) {
                qDebug() << "Find node failed:" << *nodeIdIt;
                continue;
            }
            nodeIt->second.partId = part.id;
            part.nodeIds.push_back(nodeIt->first);
            for (auto edgeIdIt = nodeIt->second.edgeIds.begin(); edgeIdIt != nodeIt->second.edgeIds.end(); edgeIdIt++) {
                auto edgeIt = edgeMap.find(*edgeIdIt);
                if (edgeIt == edgeMap.end()) {
                    qDebug() << "Find edge failed:" << *edgeIdIt;
                    continue;
                }
                edgeIt->second.partId = part.id;
            }
        }
        partMap[part.id] = part;
        addPartToComponent(part.id, findComponentParentId(part.componentId));
        emit partAdded(part.id);
    }
    for (auto edgeIdIt = node->edgeIds.begin(); edgeIdIt != node->edgeIds.end(); edgeIdIt++) {
        auto edgeIt = edgeMap.find(*edgeIdIt);
        if (edgeIt == edgeMap.end()) {
            qDebug() << "Find edge failed:" << *edgeIdIt;
            continue;
        }
        QUuid neighborId = edgeIt->second.neighborOf(nodeId);
        auto nodeIt = nodeMap.find(neighborId);
        if (nodeIt == nodeMap.end()) {
            qDebug() << "Find node failed:" << neighborId;
            continue;
        }
        nodeIt->second.edgeIds.erase(std::remove(nodeIt->second.edgeIds.begin(), nodeIt->second.edgeIds.end(), *edgeIdIt), nodeIt->second.edgeIds.end());
        edgeMap.erase(*edgeIdIt);
        emit edgeRemoved(*edgeIdIt);
    }
    nodeMap.erase(nodeId);
    emit nodeRemoved(nodeId);
    removePart(oldPartId);
    
    emit skeletonChanged();
}

void SkeletonDocument::addNode(float x, float y, float z, float radius, QUuid fromNodeId)
{
    createNode(x, y, z, radius, fromNodeId);
}

QUuid SkeletonDocument::createNode(float x, float y, float z, float radius, QUuid fromNodeId)
{
    QUuid partId;
    const SkeletonNode *fromNode = nullptr;
    bool newPartAdded = false;
    if (fromNodeId.isNull()) {
        SkeletonPart part;
        partMap[part.id] = part;
        partId = part.id;
        emit partAdded(partId);
        newPartAdded = true;
    } else {
        fromNode = findNode(fromNodeId);
        if (nullptr == fromNode) {
            qDebug() << "Find node failed:" << fromNodeId;
            return QUuid();
        }
        partId = fromNode->partId;
        if (isPartReadonly(partId))
            return QUuid();
    }
    SkeletonNode node;
    node.partId = partId;
    node.setRadius(radius);
    node.x = x;
    node.y = y;
    node.z = z;
    nodeMap[node.id] = node;
    partMap[partId].nodeIds.push_back(node.id);
    
    qDebug() << "Add node " << node.id << x << y << z;
    
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

bool SkeletonDocument::originSettled() const
{
    return !qFuzzyIsNull(originX) && !qFuzzyIsNull(originY) && !qFuzzyIsNull(originZ);
}

void SkeletonDocument::addEdge(QUuid fromNodeId, QUuid toNodeId)
{
    if (findEdgeByNodes(fromNodeId, toNodeId)) {
        qDebug() << "Add edge but edge already existed:" << fromNodeId << toNodeId;
        return;
    }
    
    const SkeletonNode *fromNode = nullptr;
    const SkeletonNode *toNode = nullptr;
    bool toPartRemoved = false;
    
    fromNode = findNode(fromNodeId);
    if (nullptr == fromNode) {
        qDebug() << "Find node failed:" << fromNodeId;
        return;
    }
    
    if (isPartReadonly(fromNode->partId))
        return;
    
    toNode = findNode(toNodeId);
    if (nullptr == toNode) {
        qDebug() << "Find node failed:" << toNodeId;
        return;
    }
    
    if (isPartReadonly(toNode->partId))
        return;
    
    QUuid toPartId = toNode->partId;
    
    auto fromPart = partMap.find(fromNode->partId);
    if (fromPart == partMap.end()) {
        qDebug() << "Find part failed:" << fromNode->partId;
        return;
    }
    
    fromPart->second.dirty = true;
    
    if (fromNode->partId != toNode->partId) {
        toPartRemoved = true;
        std::vector<QUuid> toGroup;
        std::set<QUuid> visitMap;
        joinNodeAndNeiborsToGroup(&toGroup, toNodeId, &visitMap);
        for (auto nodeIdIt = toGroup.begin(); nodeIdIt != toGroup.end(); nodeIdIt++) {
            auto nodeIt = nodeMap.find(*nodeIdIt);
            if (nodeIt == nodeMap.end()) {
                qDebug() << "Find node failed:" << *nodeIdIt;
                continue;
            }
            nodeIt->second.partId = fromNode->partId;
            fromPart->second.nodeIds.push_back(nodeIt->first);
            for (auto edgeIdIt = nodeIt->second.edgeIds.begin(); edgeIdIt != nodeIt->second.edgeIds.end(); edgeIdIt++) {
                auto edgeIt = edgeMap.find(*edgeIdIt);
                if (edgeIt == edgeMap.end()) {
                    qDebug() << "Find edge failed:" << *edgeIdIt;
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
        removePart(toPartId);
    }
    
    emit skeletonChanged();
}

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

const SkeletonComponent *SkeletonDocument::findComponent(QUuid componentId) const
{
    if (componentId.isNull())
        return &rootComponent;
    auto it = componentMap.find(componentId);
    if (it == componentMap.end())
        return nullptr;
    return &it->second;
}

void SkeletonDocument::scaleNodeByAddRadius(QUuid nodeId, float amount)
{
    auto it = nodeMap.find(nodeId);
    if (it == nodeMap.end()) {
        qDebug() << "Find node failed:" << nodeId;
        return;
    }
    if (isPartReadonly(it->second.partId))
        return;
    it->second.setRadius(it->second.radius + amount);
    auto part = partMap.find(it->second.partId);
    if (part != partMap.end())
        part->second.dirty = true;
    emit nodeRadiusChanged(nodeId);
    emit skeletonChanged();
}

bool SkeletonDocument::isPartReadonly(QUuid partId) const
{
    const SkeletonPart *part = findPart(partId);
    if (nullptr == part) {
        qDebug() << "Find part failed:" << partId;
        return true;
    }
    return part->locked || !part->visible;
}

void SkeletonDocument::moveNodeBy(QUuid nodeId, float x, float y, float z)
{
    auto it = nodeMap.find(nodeId);
    if (it == nodeMap.end()) {
        qDebug() << "Find node failed:" << nodeId;
        return;
    }
    if (isPartReadonly(it->second.partId))
        return;
    if (!xlocked)
        it->second.x += x;
    if (!ylocked)
        it->second.y += y;
    if (!zlocked)
        it->second.z += z;
    auto part = partMap.find(it->second.partId);
    if (part != partMap.end())
        part->second.dirty = true;
    emit nodeOriginChanged(nodeId);
    emit skeletonChanged();
}

void SkeletonDocument::moveOriginBy(float x, float y, float z)
{
    if (!xlocked)
        originX += x;
    if (!ylocked)
        originY += y;
    if (!zlocked)
        originZ += z;
    emit originChanged();
    emit skeletonChanged();
}

void SkeletonDocument::setNodeOrigin(QUuid nodeId, float x, float y, float z)
{
    auto it = nodeMap.find(nodeId);
    if (it == nodeMap.end()) {
        qDebug() << "Find node failed:" << nodeId;
        return;
    }
    if (isPartReadonly(it->second.partId))
        return;
    if (!xlocked)
        it->second.x = x;
    if (!ylocked)
        it->second.y = y;
    if (!zlocked)
        it->second.z = z;
    auto part = partMap.find(it->second.partId);
    if (part != partMap.end())
        part->second.dirty = true;
    emit nodeOriginChanged(nodeId);
    emit skeletonChanged();
}

void SkeletonDocument::setNodeRadius(QUuid nodeId, float radius)
{
    auto it = nodeMap.find(nodeId);
    if (it == nodeMap.end()) {
        qDebug() << "Find node failed:" << nodeId;
        return;
    }
    if (isPartReadonly(it->second.partId))
        return;
    it->second.setRadius(radius);
    auto part = partMap.find(it->second.partId);
    if (part != partMap.end())
        part->second.dirty = true;
    emit nodeRadiusChanged(nodeId);
    emit skeletonChanged();
}

void SkeletonDocument::switchNodeXZ(QUuid nodeId)
{
    auto it = nodeMap.find(nodeId);
    if (it == nodeMap.end()) {
        qDebug() << "Find node failed:" << nodeId;
        return;
    }
    if (isPartReadonly(it->second.partId))
        return;
    std::swap(it->second.x, it->second.z);
    auto part = partMap.find(it->second.partId);
    if (part != partMap.end())
        part->second.dirty = true;
    emit nodeOriginChanged(nodeId);
    emit skeletonChanged();
}

void SkeletonDocument::updateTurnaround(const QImage &image)
{
    turnaround = image;
    emit turnaroundChanged();
}

void SkeletonDocument::setEditMode(SkeletonDocumentEditMode mode)
{
    if (editMode == mode)
        return;
    
    editMode = mode;
    emit editModeChanged();
}

void SkeletonDocument::joinNodeAndNeiborsToGroup(std::vector<QUuid> *group, QUuid nodeId, std::set<QUuid> *visitMap, QUuid noUseEdgeId)
{
    if (nodeId.isNull() || visitMap->find(nodeId) != visitMap->end())
        return;
    const SkeletonNode *node = findNode(nodeId);
    if (nullptr == node) {
        qDebug() << "Find node failed:" << nodeId;
        return;
    }
    visitMap->insert(nodeId);
    group->push_back(nodeId);
    for (auto edgeIt = node->edgeIds.begin(); edgeIt != node->edgeIds.end(); edgeIt++) {
        if (noUseEdgeId == *edgeIt)
            continue;
        const SkeletonEdge *edge = findEdge(*edgeIt);
        if (nullptr == edge) {
            qDebug() << "Find edge failed:" << *edgeIt;
            continue;
        }
        for (auto nodeIt = edge->nodeIds.begin(); nodeIt != edge->nodeIds.end(); nodeIt++) {
            joinNodeAndNeiborsToGroup(group, *nodeIt, visitMap, noUseEdgeId);
        }
    }
}

void SkeletonDocument::splitPartByNode(std::vector<std::vector<QUuid>> *groups, QUuid nodeId)
{
    const SkeletonNode *node = findNode(nodeId);
    std::set<QUuid> visitMap;
    for (auto edgeIt = node->edgeIds.begin(); edgeIt != node->edgeIds.end(); edgeIt++) {
        std::vector<QUuid> group;
        const SkeletonEdge *edge = findEdge(*edgeIt);
        if (nullptr == edge) {
            qDebug() << "Find edge failed:" << *edgeIt;
            continue;
        }
        joinNodeAndNeiborsToGroup(&group, edge->neighborOf(nodeId), &visitMap, *edgeIt);
        if (!group.empty())
            groups->push_back(group);
    }
}

void SkeletonDocument::splitPartByEdge(std::vector<std::vector<QUuid>> *groups, QUuid edgeId)
{
    const SkeletonEdge *edge = findEdge(edgeId);
    if (nullptr == edge) {
        qDebug() << "Find edge failed:" << edgeId;
        return;
    }
    std::set<QUuid> visitMap;
    for (auto nodeIt = edge->nodeIds.begin(); nodeIt != edge->nodeIds.end(); nodeIt++) {
        std::vector<QUuid> group;
        joinNodeAndNeiborsToGroup(&group, *nodeIt, &visitMap, edgeId);
        if (!group.empty())
            groups->push_back(group);
    }
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

void SkeletonDocument::toSnapshot(SkeletonSnapshot *snapshot, const std::set<QUuid> &limitNodeIds) const
{
    std::set<QUuid> limitPartIds;
    std::set<QUuid> limitComponentIds;
    for (const auto &nodeId: limitNodeIds) {
        const SkeletonNode *node = findNode(nodeId);
        if (!node)
            continue;
        const SkeletonPart *part = findPart(node->partId);
        if (!part)
            continue;
        limitPartIds.insert(node->partId);
        const SkeletonComponent *component = findComponent(part->componentId);
        while (component) {
            limitComponentIds.insert(component->id);
            if (component->id.isNull())
                break;
            component = findComponent(component->parentId);
        }
    }
    for (const auto &partIt : partMap) {
        if (!limitPartIds.empty() && limitPartIds.find(partIt.first) == limitPartIds.end())
            continue;
        std::map<QString, QString> part;
        part["id"] = partIt.second.id.toString();
        part["visible"] = partIt.second.visible ? "true" : "false";
        part["locked"] = partIt.second.locked ? "true" : "false";
        part["subdived"] = partIt.second.subdived ? "true" : "false";
        part["disabled"] = partIt.second.disabled ? "true" : "false";
        part["xMirrored"] = partIt.second.xMirrored ? "true" : "false";
        part["zMirrored"] = partIt.second.zMirrored ? "true" : "false";
        part["rounded"] = partIt.second.rounded ? "true" : "false";
        part["dirty"] = partIt.second.dirty ? "true" : "false";
        if (partIt.second.hasColor)
            part["color"] = partIt.second.color.name();
        if (partIt.second.deformThicknessAdjusted())
            part["deformThickness"] = QString::number(partIt.second.deformThickness);
        if (partIt.second.deformWidthAdjusted())
            part["deformWidth"] = QString::number(partIt.second.deformWidth);
        if (!partIt.second.name.isEmpty())
            part["name"] = partIt.second.name;
        snapshot->parts[part["id"]] = part;
    }
    for (const auto &nodeIt: nodeMap) {
        if (!limitNodeIds.empty() && limitNodeIds.find(nodeIt.first) == limitNodeIds.end())
            continue;
        std::map<QString, QString> node;
        node["id"] = nodeIt.second.id.toString();
        node["radius"] = QString::number(nodeIt.second.radius);
        node["x"] = QString::number(nodeIt.second.x);
        node["y"] = QString::number(nodeIt.second.y);
        node["z"] = QString::number(nodeIt.second.z);
        node["partId"] = nodeIt.second.partId.toString();
        if (!nodeIt.second.name.isEmpty())
            node["name"] = nodeIt.second.name;
        snapshot->nodes[node["id"]] = node;
    }
    for (const auto &edgeIt: edgeMap) {
        if (edgeIt.second.nodeIds.size() != 2)
            continue;
        if (!limitNodeIds.empty() &&
                (limitNodeIds.find(edgeIt.second.nodeIds[0]) == limitNodeIds.end() ||
                    limitNodeIds.find(edgeIt.second.nodeIds[1]) == limitNodeIds.end()))
            continue;
        std::map<QString, QString> edge;
        edge["id"] = edgeIt.second.id.toString();
        edge["from"] = edgeIt.second.nodeIds[0].toString();
        edge["to"] = edgeIt.second.nodeIds[1].toString();
        edge["partId"] = edgeIt.second.partId.toString();
        if (!edgeIt.second.name.isEmpty())
            edge["name"] = edgeIt.second.name;
        snapshot->edges[edge["id"]] = edge;
    }
    for (const auto &componentIt: componentMap) {
        if (!limitComponentIds.empty() && limitComponentIds.find(componentIt.first) == limitComponentIds.end())
            continue;
        std::map<QString, QString> component;
        component["id"] = componentIt.second.id.toString();
        if (!componentIt.second.name.isEmpty())
            component["name"] = componentIt.second.name;
        component["expanded"] = componentIt.second.expanded ? "true" : "false";
        component["inverse"] = componentIt.second.inverse ? "true" : "false";
        component["dirty"] = componentIt.second.dirty ? "true" : "false";
        QStringList childIdList;
        for (const auto &childId: componentIt.second.childrenIds) {
            childIdList.append(childId.toString());
        }
        QString children = childIdList.join(",");
        if (!children.isEmpty())
            component["children"] = children;
        QString linkData = componentIt.second.linkData();
        if (!linkData.isEmpty()) {
            component["linkData"] = linkData;
            component["linkDataType"] = componentIt.second.linkDataType();
        }
        if (!componentIt.second.name.isEmpty())
            component["name"] = componentIt.second.name;
        snapshot->components[component["id"]] = component;
    }
    if (limitComponentIds.empty() || limitComponentIds.find(QUuid()) != limitComponentIds.end()) {
        QStringList childIdList;
        for (const auto &childId: rootComponent.childrenIds) {
            childIdList.append(childId.toString());
        }
        QString children = childIdList.join(",");
        if (!children.isEmpty())
            snapshot->rootComponent["children"] = children;
    }
    
    std::map<QString, QString> canvas;
    canvas["originX"] = QString::number(originX);
    canvas["originY"] = QString::number(originY);
    canvas["originZ"] = QString::number(originZ);
    snapshot->canvas = canvas;
}

void SkeletonDocument::addFromSnapshot(const SkeletonSnapshot &snapshot)
{
    const auto &originXit = snapshot.canvas.find("originX");
    const auto &originYit = snapshot.canvas.find("originY");
    const auto &originZit = snapshot.canvas.find("originZ");
    if (originXit != snapshot.canvas.end() &&
            originYit != snapshot.canvas.end() &&
            originZit != snapshot.canvas.end()) {
        originX = originXit->second.toFloat();
        originY = originYit->second.toFloat();
        originZ = originZit->second.toFloat();
    }
    
    std::set<QUuid> newAddedNodeIds;
    std::set<QUuid> newAddedEdgeIds;
    std::set<QUuid> newAddedPartIds;
    std::set<QUuid> newAddedComponentIds;
    
    std::set<QUuid> inversePartIds;
    
    std::map<QUuid, QUuid> oldNewIdMap;
    for (const auto &partKv: snapshot.parts) {
        SkeletonPart part;
        oldNewIdMap[QUuid(partKv.first)] = part.id;
        part.name = valueOfKeyInMapOrEmpty(partKv.second, "name");
        part.visible = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "visible"));
        part.locked = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "locked"));
        part.subdived = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "subdived"));
        part.disabled = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "disabled"));
        part.xMirrored = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "xMirrored"));
        part.zMirrored = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "zMirrored"));
        part.rounded = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "rounded"));
        if (isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "inverse")))
            inversePartIds.insert(part.id);
        const auto &colorIt = partKv.second.find("color");
        if (colorIt != partKv.second.end()) {
            part.color = QColor(colorIt->second);
            part.hasColor = true;
        }
        const auto &deformThicknessIt = partKv.second.find("deformThickness");
        if (deformThicknessIt != partKv.second.end())
            part.setDeformThickness(deformThicknessIt->second.toFloat());
        const auto &deformWidthIt = partKv.second.find("deformWidth");
        if (deformWidthIt != partKv.second.end())
            part.setDeformWidth(deformWidthIt->second.toFloat());
        partMap[part.id] = part;
        newAddedPartIds.insert(part.id);
    }
    for (const auto &nodeKv: snapshot.nodes) {
        if (nodeKv.second.find("radius") == nodeKv.second.end() ||
                nodeKv.second.find("x") == nodeKv.second.end() ||
                nodeKv.second.find("y") == nodeKv.second.end() ||
                nodeKv.second.find("z") == nodeKv.second.end() ||
                nodeKv.second.find("partId") == nodeKv.second.end())
            continue;
        SkeletonNode node;
        oldNewIdMap[QUuid(nodeKv.first)] = node.id;
        node.name = valueOfKeyInMapOrEmpty(nodeKv.second, "name");
        node.radius = valueOfKeyInMapOrEmpty(nodeKv.second, "radius").toFloat();
        node.x = valueOfKeyInMapOrEmpty(nodeKv.second, "x").toFloat();
        node.y = valueOfKeyInMapOrEmpty(nodeKv.second, "y").toFloat();
        node.z = valueOfKeyInMapOrEmpty(nodeKv.second, "z").toFloat();
        node.partId = oldNewIdMap[QUuid(valueOfKeyInMapOrEmpty(nodeKv.second, "partId"))];
        nodeMap[node.id] = node;
        newAddedNodeIds.insert(node.id);
    }
    for (const auto &edgeKv: snapshot.edges) {
        if (edgeKv.second.find("from") == edgeKv.second.end() ||
                edgeKv.second.find("to") == edgeKv.second.end() ||
                edgeKv.second.find("partId") == edgeKv.second.end())
            continue;
        SkeletonEdge edge;
        oldNewIdMap[QUuid(edgeKv.first)] = edge.id;
        edge.name = valueOfKeyInMapOrEmpty(edgeKv.second, "name");
        edge.partId = oldNewIdMap[QUuid(valueOfKeyInMapOrEmpty(edgeKv.second, "partId"))];
        QString fromNodeId = valueOfKeyInMapOrEmpty(edgeKv.second, "from");
        if (!fromNodeId.isEmpty()) {
            QUuid fromId = oldNewIdMap[QUuid(fromNodeId)];
            edge.nodeIds.push_back(fromId);
            nodeMap[fromId].edgeIds.push_back(edge.id);
        }
        QString toNodeId = valueOfKeyInMapOrEmpty(edgeKv.second, "to");
        if (!toNodeId.isEmpty()) {
            QUuid toId = oldNewIdMap[QUuid(toNodeId)];
            edge.nodeIds.push_back(toId);
            nodeMap[toId].edgeIds.push_back(edge.id);
        }
        edgeMap[edge.id] = edge;
        newAddedEdgeIds.insert(edge.id);
    }
    for (const auto &nodeIt: nodeMap) {
        if (newAddedNodeIds.find(nodeIt.first) == newAddedNodeIds.end())
            continue;
        partMap[nodeIt.second.partId].nodeIds.push_back(nodeIt.first);
    }
    for (const auto &componentKv: snapshot.components) {
        QString linkData = valueOfKeyInMapOrEmpty(componentKv.second, "linkData");
        QString linkDataType = valueOfKeyInMapOrEmpty(componentKv.second, "linkDataType");
        SkeletonComponent component(QUuid(), linkData, linkDataType);
        oldNewIdMap[QUuid(componentKv.first)] = component.id;
        component.name = valueOfKeyInMapOrEmpty(componentKv.second, "name");
        component.expanded = isTrueValueString(valueOfKeyInMapOrEmpty(componentKv.second, "expanded"));
        component.inverse = isTrueValueString(valueOfKeyInMapOrEmpty(componentKv.second, "inverse"));
        qDebug() << "Add component:" << component.id << " old:" << componentKv.first << "name:" << component.name;
        if ("partId" == linkDataType) {
            QUuid partId = oldNewIdMap[QUuid(linkData)];
            component.linkToPartId = partId;
            qDebug() << "Add part:" << partId << " from component:" << component.id;
            partMap[partId].componentId = component.id;
            if (inversePartIds.find(partId) != inversePartIds.end())
                component.inverse = true;
        }
        componentMap[component.id] = component;
        newAddedComponentIds.insert(component.id);
    }
    const auto &rootComponentChildren = snapshot.rootComponent.find("children");
    if (rootComponentChildren != snapshot.rootComponent.end()) {
        for (const auto &childId: rootComponentChildren->second.split(",")) {
            if (childId.isEmpty())
                continue;
            QUuid componentId = oldNewIdMap[QUuid(childId)];
            if (componentMap.find(componentId) == componentMap.end())
                continue;
            qDebug() << "Add root component:" << componentId;
            rootComponent.addChild(componentId);
        }
    }
    for (const auto &componentKv: snapshot.components) {
        QUuid componentId = oldNewIdMap[QUuid(componentKv.first)];
        if (componentMap.find(componentId) == componentMap.end())
            continue;
        for (const auto &childId: valueOfKeyInMapOrEmpty(componentKv.second, "children").split(",")) {
            if (childId.isEmpty())
                continue;
            QUuid childComponentId = oldNewIdMap[QUuid(childId)];
            if (componentMap.find(childComponentId) == componentMap.end())
                continue;
            qDebug() << "Add child component:" << childComponentId << "to" << componentId;
            componentMap[componentId].addChild(childComponentId);
            componentMap[childComponentId].parentId = componentId;
        }
    }
    
    for (const auto &nodeIt: newAddedNodeIds) {
        emit nodeAdded(nodeIt);
    }
    for (const auto &edgeIt: newAddedEdgeIds) {
        emit edgeAdded(edgeIt);
    }
    for (const auto &partIt : newAddedPartIds) {
        emit partAdded(partIt);
    }
    
    emit componentChildrenChanged(QUuid());
    emit originChanged();
    emit skeletonChanged();
    
    for (const auto &partIt : newAddedPartIds) {
        emit partVisibleStateChanged(partIt);
    }
    
    emit uncheckAll();
    for (const auto &nodeIt: newAddedNodeIds) {
        emit checkNode(nodeIt);
    }
    for (const auto &edgeIt: newAddedEdgeIds) {
        emit checkEdge(edgeIt);
    }
}

void SkeletonDocument::reset()
{
    originX = 0.0;
    originY = 0.0;
    originZ = 0.0;
    nodeMap.clear();
    edgeMap.clear();
    partMap.clear();
    componentMap.clear();
    rootComponent = SkeletonComponent();
    emit cleanup();
    emit skeletonChanged();
}

void SkeletonDocument::fromSnapshot(const SkeletonSnapshot &snapshot)
{
    reset();
    addFromSnapshot(snapshot);
    emit uncheckAll();
}

MeshLoader *SkeletonDocument::takeResultMesh()
{
    if (nullptr == m_resultMesh)
        return nullptr;
    MeshLoader *resultMesh = new MeshLoader(*m_resultMesh);
    return resultMesh;
}

MeshLoader *SkeletonDocument::takeResultTextureMesh()
{
    MeshLoader *resultTextureMesh = m_resultTextureMesh;
    m_resultTextureMesh = nullptr;
    return resultTextureMesh;
}

void SkeletonDocument::meshReady()
{
    MeshLoader *resultMesh = m_meshGenerator->takeResultMesh();
    MeshResultContext *meshResultContext = m_meshGenerator->takeMeshResultContext();
    
    QImage *resultPreview = m_meshGenerator->takePreview();
    if (resultPreview) {
        preview = *resultPreview;
        delete resultPreview;
    }
    
    for (auto &part: partMap) {
        QImage *resultPartPreview = m_meshGenerator->takePartPreview(part.first.toString());
        if (resultPartPreview) {
            part.second.preview = *resultPartPreview;
            emit partPreviewChanged(part.first);
            delete resultPartPreview;
        }
    }
    
    delete m_resultMesh;
    m_resultMesh = resultMesh;
    
    delete m_currentMeshResultContext;
    m_currentMeshResultContext = meshResultContext;
    
    if (nullptr == m_resultMesh) {
        qDebug() << "Result mesh is null";
    }
    
    delete m_meshGenerator;
    m_meshGenerator = nullptr;
    
    qDebug() << "MeshLoader generation done";
    
    m_isPostProcessResultObsolete = true;
    
    emit resultMeshChanged();
    
    if (m_isResultMeshObsolete) {
        generateMesh();
    }
}

bool SkeletonDocument::isPostProcessResultObsolete() const
{
    return m_isPostProcessResultObsolete;
}

void SkeletonDocument::batchChangeBegin()
{
    m_batchChangeRefCount++;
}

void SkeletonDocument::batchChangeEnd()
{
    m_batchChangeRefCount--;
    if (0 == m_batchChangeRefCount) {
        if (m_isResultMeshObsolete) {
            generateMesh();
        }
    }
}

void SkeletonDocument::regenerateMesh()
{
    markAllDirty();
    generateMesh();
}

void SkeletonDocument::generateMesh()
{
    if (nullptr != m_meshGenerator || m_batchChangeRefCount > 0) {
        m_isResultMeshObsolete = true;
        return;
    }
    
    qDebug() << "MeshLoader generating..";
    
    settleOrigin();
    
    m_isResultMeshObsolete = false;
    
    QThread *thread = new QThread;
    
    SkeletonSnapshot *snapshot = new SkeletonSnapshot;
    toSnapshot(snapshot);
    resetDirtyFlags();
    m_meshGenerator = new MeshGenerator(snapshot, thread);
    m_meshGenerator->setGeneratedCacheContext(&m_generatedCacheContext);
    if (nullptr != m_sharedContextWidget)
        m_meshGenerator->setSharedContextWidget(m_sharedContextWidget);
    m_meshGenerator->moveToThread(thread);
    for (auto &part: partMap) {
        m_meshGenerator->addPartPreviewRequirement(part.first.toString());
    }
    connect(thread, &QThread::started, m_meshGenerator, &MeshGenerator::process);
    connect(m_meshGenerator, &MeshGenerator::finished, this, &SkeletonDocument::meshReady);
    connect(m_meshGenerator, &MeshGenerator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void SkeletonDocument::generateTexture()
{
    if (nullptr != m_textureGenerator) {
        m_isTextureObsolete = true;
        return;
    }
    
    qDebug() << "Texture guide generating..";
    
    m_isTextureObsolete = false;
    
    QThread *thread = new QThread;
    m_textureGenerator = new TextureGenerator(*m_postProcessedResultContext, thread);
    m_textureGenerator->moveToThread(thread);
    connect(thread, &QThread::started, m_textureGenerator, &TextureGenerator::process);
    connect(m_textureGenerator, &TextureGenerator::finished, this, &SkeletonDocument::textureReady);
    connect(m_textureGenerator, &TextureGenerator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void SkeletonDocument::textureReady()
{
    delete textureGuideImage;
    textureGuideImage = m_textureGenerator->takeResultTextureGuideImage();
    
    delete textureImage;
    textureImage = m_textureGenerator->takeResultTextureImage();
    
    delete textureBorderImage;
    textureBorderImage = m_textureGenerator->takeResultTextureBorderImage();
    
    delete textureAmbientOcclusionImage;
    textureAmbientOcclusionImage = nullptr;
    
    delete textureColorImage;
    textureColorImage = m_textureGenerator->takeResultTextureColorImage();
    
    delete m_resultTextureMesh;
    m_resultTextureMesh = m_textureGenerator->takeResultMesh();
    
    m_textureImageUpdateVersion++;
    
    delete m_textureGenerator;
    m_textureGenerator = nullptr;
    
    qDebug() << "Texture guide generation done";
    
    emit resultTextureChanged();
    
    if (m_isTextureObsolete) {
        generateTexture();
    } else {
        checkExportReadyState();
    }
}

void SkeletonDocument::bakeAmbientOcclusionTexture()
{
    if (nullptr != m_ambientOcclusionBaker) {
        return;
    }

    if (nullptr == textureColorImage || m_isTextureObsolete)
        return;

    qDebug() << "Ambient occlusion texture baking..";

    QThread *thread = new QThread;
    m_ambientOcclusionBaker = new AmbientOcclusionBaker();
    m_ambientOcclusionBaker->setInputMesh(*m_postProcessedResultContext);
    m_ambientOcclusionBaker->setBakeSize(TextureGenerator::m_textureWidth, TextureGenerator::m_textureHeight);
    if (textureBorderImage)
        m_ambientOcclusionBaker->setBorderImage(*textureBorderImage);
    if (textureColorImage)
        m_ambientOcclusionBaker->setColorImage(*textureColorImage);
    m_ambientOcclusionBaker->setImageUpdateVersion(m_textureImageUpdateVersion);
    m_ambientOcclusionBaker->setRenderThread(thread);
    m_ambientOcclusionBaker->moveToThread(thread);
    connect(thread, &QThread::started, m_ambientOcclusionBaker, &AmbientOcclusionBaker::process);
    connect(m_ambientOcclusionBaker, &AmbientOcclusionBaker::finished, this, &SkeletonDocument::ambientOcclusionTextureReady);
    connect(m_ambientOcclusionBaker, &AmbientOcclusionBaker::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void SkeletonDocument::ambientOcclusionTextureReady()
{
    m_ambientOcclusionBakedImageUpdateVersion = m_ambientOcclusionBaker->getImageUpdateVersion();

    if (m_textureImageUpdateVersion == m_ambientOcclusionBakedImageUpdateVersion) {
        QImage *bakedGuideImage = m_ambientOcclusionBaker->takeGuideImage();
        if (nullptr != bakedGuideImage) {
            delete textureGuideImage;
            textureGuideImage = bakedGuideImage;
        }

        QImage *bakedTextureImage = m_ambientOcclusionBaker->takeTextureImage();
        if (nullptr != bakedTextureImage) {
            delete textureImage;
            textureImage = bakedTextureImage;
        }

        delete textureAmbientOcclusionImage;
        textureAmbientOcclusionImage = m_ambientOcclusionBaker->takeAmbientOcclusionImage();
        
        MeshLoader *resultMesh = m_ambientOcclusionBaker->takeResultMesh();
        if (resultMesh) {
            delete m_resultTextureMesh;
            m_resultTextureMesh = resultMesh;
        }
    }

    delete m_ambientOcclusionBaker;
    m_ambientOcclusionBaker = nullptr;

    qDebug() << "Ambient occlusion texture baking done";

    if (m_textureImageUpdateVersion == m_ambientOcclusionBakedImageUpdateVersion) {
        emit resultBakedTextureChanged();
    } else {
        bakeAmbientOcclusionTexture();
    }
}

void SkeletonDocument::postProcess()
{
    if (nullptr != m_postProcessor) {
        m_isPostProcessResultObsolete = true;
        return;
    }

    qDebug() << "Post processing..";

    m_isPostProcessResultObsolete = false;

    if (!m_currentMeshResultContext) {
        qDebug() << "MeshLoader is null";
        return;
    }

    QThread *thread = new QThread;
    m_postProcessor = new MeshResultPostProcessor(*m_currentMeshResultContext);
    m_postProcessor->moveToThread(thread);
    connect(thread, &QThread::started, m_postProcessor, &MeshResultPostProcessor::process);
    connect(m_postProcessor, &MeshResultPostProcessor::finished, this, &SkeletonDocument::postProcessedMeshResultReady);
    connect(m_postProcessor, &MeshResultPostProcessor::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void SkeletonDocument::postProcessedMeshResultReady()
{
    delete m_postProcessedResultContext;
    m_postProcessedResultContext = m_postProcessor->takePostProcessedResultContext();

    delete m_postProcessor;
    m_postProcessor = nullptr;

    qDebug() << "Post process done";

    emit postProcessedResultChanged();

    if (m_isPostProcessResultObsolete) {
        postProcess();
    }
}

const MeshResultContext &SkeletonDocument::currentPostProcessedResultContext() const
{
    return *m_postProcessedResultContext;
}

void SkeletonDocument::setPartLockState(QUuid partId, bool locked)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        qDebug() << "Part not found:" << partId;
        return;
    }
    if (part->second.locked == locked)
        return;
    part->second.locked = locked;
    emit partLockStateChanged(partId);
    emit optionsChanged();
}

void SkeletonDocument::setPartVisibleState(QUuid partId, bool visible)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        qDebug() << "Part not found:" << partId;
        return;
    }
    if (part->second.visible == visible)
        return;
    part->second.visible = visible;
    emit partVisibleStateChanged(partId);
    emit optionsChanged();
}

void SkeletonDocument::setComponentInverseState(QUuid componentId, bool inverse)
{
    auto component = componentMap.find(componentId);
    if (component == componentMap.end()) {
        qDebug() << "Component not found:" << componentId;
        return;
    }
    if (component->second.inverse == inverse)
        return;
    component->second.inverse = inverse;
    component->second.dirty = true;
    emit componentInverseStateChanged(componentId);
    emit skeletonChanged();
}

void SkeletonDocument::setPartSubdivState(QUuid partId, bool subdived)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        qDebug() << "Part not found:" << partId;
        return;
    }
    if (part->second.subdived == subdived)
        return;
    part->second.subdived = subdived;
    part->second.dirty = true;
    emit partSubdivStateChanged(partId);
    emit skeletonChanged();
}

void SkeletonDocument::setPartDisableState(QUuid partId, bool disabled)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        qDebug() << "Part not found:" << partId;
        return;
    }
    if (part->second.disabled == disabled)
        return;
    part->second.disabled = disabled;
    part->second.dirty = true;
    emit partDisableStateChanged(partId);
    emit skeletonChanged();
}

const SkeletonComponent *SkeletonDocument::findComponentParent(QUuid componentId) const
{
    auto component = componentMap.find(componentId);
    if (component == componentMap.end()) {
        qDebug() << "Component not found:" << componentId;
        return nullptr;
    }
    
    if (component->second.parentId.isNull())
        return &rootComponent;
    
    return (SkeletonComponent *)findComponent(component->second.parentId);
}

QUuid SkeletonDocument::findComponentParentId(QUuid componentId) const
{
    auto component = componentMap.find(componentId);
    if (component == componentMap.end()) {
        qDebug() << "Component not found:" << componentId;
        return QUuid();
    }
    
    return component->second.parentId;
}

void SkeletonDocument::moveComponentUp(QUuid componentId)
{
    SkeletonComponent *parent = (SkeletonComponent *)findComponentParent(componentId);
    if (nullptr == parent)
        return;
    
    QUuid parentId = findComponentParentId(componentId);
    
    parent->moveChildUp(componentId);
    parent->dirty = true;
    emit componentChildrenChanged(parentId);
    emit skeletonChanged();
}

void SkeletonDocument::moveComponentDown(QUuid componentId)
{
    SkeletonComponent *parent = (SkeletonComponent *)findComponentParent(componentId);
    if (nullptr == parent)
        return;
    
    QUuid parentId = findComponentParentId(componentId);
    
    parent->moveChildDown(componentId);
    parent->dirty = true;
    emit componentChildrenChanged(parentId);
    emit skeletonChanged();
}

void SkeletonDocument::moveComponentToTop(QUuid componentId)
{
    SkeletonComponent *parent = (SkeletonComponent *)findComponentParent(componentId);
    if (nullptr == parent)
        return;
    
    QUuid parentId = findComponentParentId(componentId);
    
    parent->moveChildToTop(componentId);
    parent->dirty = true;
    emit componentChildrenChanged(parentId);
    emit skeletonChanged();
}

void SkeletonDocument::moveComponentToBottom(QUuid componentId)
{
    SkeletonComponent *parent = (SkeletonComponent *)findComponentParent(componentId);
    if (nullptr == parent)
        return;
    
    QUuid parentId = findComponentParentId(componentId);
    
    parent->moveChildToBottom(componentId);
    parent->dirty = true;
    emit componentChildrenChanged(parentId);
    emit skeletonChanged();
}

void SkeletonDocument::renameComponent(QUuid componentId, QString name)
{
    auto component = componentMap.find(componentId);
    if (component == componentMap.end()) {
        qDebug() << "Component not found:" << componentId;
        return;
    }
    
    if (component->second.name == name)
        return;
    
    if (!name.trimmed().isEmpty())
        component->second.name = name;
    emit componentNameChanged(componentId);
    emit optionsChanged();
}

void SkeletonDocument::setComponentExpandState(QUuid componentId, bool expanded)
{
    auto component = componentMap.find(componentId);
    if (component == componentMap.end()) {
        qDebug() << "Component not found:" << componentId;
        return;
    }
    
    if (component->second.expanded == expanded)
        return;
    
    component->second.expanded = expanded;
    emit componentExpandStateChanged(componentId);
    emit optionsChanged();
}

void SkeletonDocument::createNewComponentAndMoveThisIn(QUuid componentId)
{
    auto component = componentMap.find(componentId);
    if (component == componentMap.end()) {
        qDebug() << "Component not found:" << componentId;
        return;
    }
    
    SkeletonComponent *oldParent = (SkeletonComponent *)findComponentParent(componentId);
    
    SkeletonComponent newParent(QUuid::createUuid());
    newParent.name = tr("Group") + " " + QString::number(componentMap.size() - partMap.size() + 1);
    
    oldParent->replaceChild(componentId, newParent.id);
    newParent.parentId = oldParent->id;
    newParent.addChild(componentId);
    componentMap[newParent.id] = newParent;
    
    component->second.parentId = newParent.id;
    
    emit componentChildrenChanged(oldParent->id);
    emit componentAdded(newParent.id);
    emit optionsChanged();
}

void SkeletonDocument::removePart(QUuid partId)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        qDebug() << "Part not found:" << partId;
        return;
    }
    
    if (!part->second.componentId.isNull()) {
        removeComponent(part->second.componentId);
        return;
    }
    
    removePartDontCareComponent(partId);
}

void SkeletonDocument::removePartDontCareComponent(QUuid partId)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        qDebug() << "Part not found:" << partId;
        return;
    }
    
    std::vector<QUuid> removedNodeIds;
    std::vector<QUuid> removedEdgeIds;
    
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

void SkeletonDocument::addPartToComponent(QUuid partId, QUuid componentId)
{
    SkeletonComponent child(QUuid::createUuid());
    
    if (!componentId.isNull()) {
        auto parentComponent = componentMap.find(componentId);
        if (parentComponent == componentMap.end()) {
            componentId = QUuid();
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
    componentMap[child.id] = child;
    
    emit componentChildrenChanged(componentId);
    emit componentAdded(child.id);
}

void SkeletonDocument::removeComponent(QUuid componentId)
{
    removeComponentRecursively(componentId);
    emit skeletonChanged();
}

void SkeletonDocument::removeComponentRecursively(QUuid componentId)
{
    auto component = componentMap.find(componentId);
    if (component == componentMap.end()) {
        qDebug() << "Component not found:" << componentId;
        return;
    }
    
    for (const auto &childId: component->second.childrenIds) {
        removeComponent(childId);
    }
    
    if (!component->second.linkToPartId.isNull()) {
        removePartDontCareComponent(component->second.linkToPartId);
    }
    
    QUuid parentId = component->second.parentId;
    if (!parentId.isNull()) {
        auto parentComponent = componentMap.find(parentId);
        if (parentComponent == componentMap.end()) {
            qDebug() << "Component not found:" << parentId;
        }
        parentComponent->second.dirty = true;
        parentComponent->second.removeChild(componentId);
    } else {
        rootComponent.removeChild(componentId);
    }
    
    componentMap.erase(component);
    emit componentRemoved(componentId);
    emit componentChildrenChanged(parentId);
}

void SkeletonDocument::setCurrentCanvasComponentId(QUuid componentId)
{
    m_currentCanvasComponentId = componentId;
}

void SkeletonDocument::addComponent(QUuid parentId)
{
    SkeletonComponent component(QUuid::createUuid());
    
    if (!parentId.isNull()) {
        auto parentComponent = componentMap.find(parentId);
        if (parentComponent == componentMap.end()) {
            qDebug() << "Component not found:" << parentId;
            return;
        }
        parentComponent->second.addChild(component.id);
    } else {
        rootComponent.addChild(component.id);
    }
    
    component.parentId = parentId;
    componentMap[component.id] = component;
    
    emit componentChildrenChanged(parentId);
    emit componentAdded(component.id);
}

bool SkeletonDocument::isDescendantComponent(QUuid componentId, QUuid suspiciousId)
{
    const SkeletonComponent *loopComponent = findComponentParent(suspiciousId);
    while (nullptr != loopComponent) {
        if (loopComponent->id == componentId)
            return true;
        loopComponent = findComponentParent(loopComponent->parentId);
    }
    return false;
}

void SkeletonDocument::moveComponent(QUuid componentId, QUuid toParentId)
{
    if (componentId == toParentId)
        return;

    auto component = componentMap.find(componentId);
    if (component == componentMap.end()) {
        qDebug() << "Component not found:" << componentId;
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

void SkeletonDocument::settleOrigin()
{
    if (originSettled())
        return;
    SkeletonSnapshot snapshot;
    toSnapshot(&snapshot);
    QRectF mainProfile;
    QRectF sideProfile;
    snapshot.resolveBoundingBox(&mainProfile, &sideProfile);
    originX = mainProfile.x() + mainProfile.width() / 2;
    originY = mainProfile.y() + mainProfile.height() / 2;
    originZ = sideProfile.x() + sideProfile.width() / 2;
    markAllDirty();
    emit originChanged();
}

void SkeletonDocument::setPartXmirrorState(QUuid partId, bool mirrored)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        qDebug() << "Part not found:" << partId;
        return;
    }
    if (part->second.xMirrored == mirrored)
        return;
    part->second.xMirrored = mirrored;
    part->second.dirty = true;
    settleOrigin();
    emit partXmirrorStateChanged(partId);
    emit skeletonChanged();
}

void SkeletonDocument::setPartZmirrorState(QUuid partId, bool mirrored)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        qDebug() << "Part not found:" << partId;
        return;
    }
    if (part->second.zMirrored == mirrored)
        return;
    part->second.zMirrored = mirrored;
    part->second.dirty = true;
    settleOrigin();
    emit partZmirrorStateChanged(partId);
    emit skeletonChanged();
}

void SkeletonDocument::setPartDeformThickness(QUuid partId, float thickness)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        qDebug() << "Part not found:" << partId;
        return;
    }
    part->second.setDeformThickness(thickness);
    part->second.dirty = true;
    emit partDeformThicknessChanged(partId);
    emit skeletonChanged();
}

void SkeletonDocument::setPartDeformWidth(QUuid partId, float width)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        qDebug() << "Part not found:" << partId;
        return;
    }
    part->second.setDeformWidth(width);
    part->second.dirty = true;
    emit partDeformWidthChanged(partId);
    emit skeletonChanged();
}

void SkeletonDocument::setPartRoundState(QUuid partId, bool rounded)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        qDebug() << "Part not found:" << partId;
        return;
    }
    if (part->second.rounded == rounded)
        return;
    part->second.rounded = rounded;
    part->second.dirty = true;
    emit partRoundStateChanged(partId);
    emit skeletonChanged();
}

void SkeletonDocument::setPartColorState(QUuid partId, bool hasColor, QColor color)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        qDebug() << "Part not found:" << partId;
        return;
    }
    if (part->second.hasColor == hasColor && part->second.color == color)
        return;
    part->second.hasColor = hasColor;
    part->second.color = color;
    part->second.dirty = true;
    emit partColorStateChanged(partId);
    emit skeletonChanged();
}

void SkeletonDocument::saveSnapshot()
{
    if (m_undoItems.size() + 1 > m_maxSnapshot)
        m_undoItems.pop_front();
    SkeletonHistoryItem item;
    toSnapshot(&item.snapshot);
    m_undoItems.push_back(item);
    qDebug() << "Undo items:" << m_undoItems.size();
}

void SkeletonDocument::undo()
{
    if (m_undoItems.empty())
        return;
    m_redoItems.push_back(m_undoItems.back());
    fromSnapshot(m_undoItems.back().snapshot);
    m_undoItems.pop_back();
    qDebug() << "Undo/Redo items:" << m_undoItems.size() << m_redoItems.size();
}

void SkeletonDocument::redo()
{
    if (m_redoItems.empty())
        return;
    m_undoItems.push_back(m_redoItems.back());
    fromSnapshot(m_redoItems.back().snapshot);
    m_redoItems.pop_back();
    qDebug() << "Undo/Redo items:" << m_undoItems.size() << m_redoItems.size();
}

void SkeletonDocument::paste()
{
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();
    if (mimeData->hasText()) {
        QXmlStreamReader xmlStreamReader(mimeData->text());
        SkeletonSnapshot snapshot;
        loadSkeletonFromXmlStream(&snapshot, xmlStreamReader);
        addFromSnapshot(snapshot);
    }
}

bool SkeletonDocument::hasPastableContentInClipboard() const
{
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();
    if (mimeData->hasText()) {
        if (-1 != mimeData->text().left(1000).indexOf("partIdList"))
            return true;
    }
    return false;
}

bool SkeletonDocument::undoable() const
{
    return !m_undoItems.empty();
}

bool SkeletonDocument::redoable() const
{
    return !m_redoItems.empty();
}

bool SkeletonDocument::isNodeEditable(QUuid nodeId) const
{
    const SkeletonNode *node = findNode(nodeId);
    if (!node) {
        qDebug() << "Node id not found:" << nodeId;
        return false;
    }
    return !isPartReadonly(node->partId);
}

bool SkeletonDocument::isEdgeEditable(QUuid edgeId) const
{
    const SkeletonEdge *edge = findEdge(edgeId);
    if (!edge) {
        qDebug() << "Edge id not found:" << edgeId;
        return false;
    }
    return !isPartReadonly(edge->partId);
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

bool SkeletonDocument::isExportReady() const
{
    if (m_isResultMeshObsolete ||
            m_isTextureObsolete ||
            m_isPostProcessResultObsolete ||
            m_meshGenerator ||
            m_textureGenerator ||
            m_postProcessor)
        return false;
    return true;
}

void SkeletonDocument::checkExportReadyState()
{
    if (isExportReady())
        emit exportReady();
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

void SkeletonDocument::setSharedContextWidget(QOpenGLWidget *sharedContextWidget)
{
    m_sharedContextWidget = sharedContextWidget;
}

void SkeletonDocument::collectComponentDescendantParts(QUuid componentId, std::vector<QUuid> &partIds) const
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

void SkeletonDocument::collectComponentDescendantComponents(QUuid componentId, std::vector<QUuid> &componentIds)
{
    const SkeletonComponent *component = findComponent(componentId);
    if (nullptr == component)
        return;
    
    if (!component->linkToPartId.isNull()) {
        return;
    }
    
    componentIds.push_back(component->id);

    for (const auto &childId: component->childrenIds) {
        collectComponentDescendantComponents(childId, componentIds);
    }
}

void SkeletonDocument::hideOtherComponents(QUuid componentId)
{
    std::vector<QUuid> partIds;
    collectComponentDescendantParts(componentId, partIds);
    std::set<QUuid> partIdSet;
    for (const auto &partId: partIds) {
        partIdSet.insert(partId);
    }
    for (const auto &part: partMap) {
        if (partIdSet.find(part.first) != partIdSet.end())
            continue;
        setPartVisibleState(part.first, false);
    }
}

void SkeletonDocument::lockOtherComponents(QUuid componentId)
{
    std::vector<QUuid> partIds;
    collectComponentDescendantParts(componentId, partIds);
    std::set<QUuid> partIdSet;
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

void SkeletonDocument::hideDescendantComponents(QUuid componentId)
{
    std::vector<QUuid> partIds;
    collectComponentDescendantParts(componentId, partIds);
    for (const auto &partId: partIds) {
        setPartVisibleState(partId, false);
    }
}

void SkeletonDocument::showDescendantComponents(QUuid componentId)
{
    std::vector<QUuid> partIds;
    collectComponentDescendantParts(componentId, partIds);
    for (const auto &partId: partIds) {
        setPartVisibleState(partId, true);
    }
}

void SkeletonDocument::lockDescendantComponents(QUuid componentId)
{
    std::vector<QUuid> partIds;
    collectComponentDescendantParts(componentId, partIds);
    for (const auto &partId: partIds) {
        setPartLockState(partId, true);
    }
}

void SkeletonDocument::unlockDescendantComponents(QUuid componentId)
{
    std::vector<QUuid> partIds;
    collectComponentDescendantParts(componentId, partIds);
    for (const auto &partId: partIds) {
        setPartLockState(partId, false);
    }
}

