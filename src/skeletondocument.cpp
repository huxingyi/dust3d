#include <QFileDialog>
#include <QDebug>
#include <QThread>
#include <QGuiApplication>
#include <QClipboard>
#include <QMimeData>
#include <QApplication>
#include <QVector3D>
#include "skeletondocument.h"
#include "util.h"
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
    // private
    m_resultMeshIsObsolete(false),
    m_meshGenerator(nullptr),
    m_resultMesh(nullptr),
    m_batchChangeRefCount(0)
{
}

SkeletonDocument::~SkeletonDocument()
{
    delete m_resultMesh;
}

void SkeletonDocument::uiReady()
{
    qDebug() << "uiReady";
    emit editModeChanged();
}

void SkeletonDocument::removePart(QUuid partId)
{
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
        partIds.push_back(part.id);
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
    partIds.erase(std::remove(partIds.begin(), partIds.end(), oldPartId), partIds.end());
    partMap.erase(oldPartId);
    emit partRemoved(oldPartId);
    emit partListChanged();
    
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
        partIds.push_back(part.id);
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
    partIds.erase(std::remove(partIds.begin(), partIds.end(), oldPartId), partIds.end());
    partMap.erase(oldPartId);
    emit partRemoved(oldPartId);
    emit partListChanged();
    
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
        partIds.push_back(part.id);
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
    node.radius = radius;
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
    
    emit partChanged(partId);
    
    if (newPartAdded)
        emit partListChanged();
    
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
    return originX > 0 && originY > 0 && originZ > 0;
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
        partIds.erase(std::remove(partIds.begin(), partIds.end(), toPartId), partIds.end());
        partMap.erase(toPartId);
        emit partRemoved(toPartId);
        emit partListChanged();
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
    emit nodeRadiusChanged(nodeId);
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

void SkeletonDocument::toSnapshot(SkeletonSnapshot *snapshot, const std::set<QUuid> &limitNodeIds) const
{
    std::set<QUuid> limitPartIds;
    for (const auto &nodeId: limitNodeIds) {
        const SkeletonNode *node = findNode(nodeId);
        if (!node)
            continue;
        limitPartIds.insert(node->partId);
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
    for (const auto &partIdIt: partIds) {
        if (!limitPartIds.empty() && limitPartIds.find(partIdIt) == limitPartIds.end())
            continue;
        snapshot->partIdList.push_back(partIdIt.toString());
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
    
    std::map<QUuid, QUuid> oldNewIdMap;
    for (const auto &partKv : snapshot.parts) {
        SkeletonPart part;
        oldNewIdMap[QUuid(partKv.first)] = part.id;
        part.name = valueOfKeyInMapOrEmpty(partKv.second, "name");
        part.visible = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "visible"));
        part.locked = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "locked"));
        part.subdived = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "subdived"));
        part.disabled = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "disabled"));
        part.xMirrored = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "xMirrored"));
        part.zMirrored = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "zMirrored"));
        const auto &deformThicknessIt = partKv.second.find("deformThickness");
        if (deformThicknessIt != partKv.second.end())
            part.setDeformThickness(deformThicknessIt->second.toFloat());
        const auto &deformWidthIt = partKv.second.find("deformWidth");
        if (deformWidthIt != partKv.second.end())
            part.setDeformWidth(deformWidthIt->second.toFloat());
        partMap[part.id] = part;
    }
    for (const auto &nodeKv : snapshot.nodes) {
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
    }
    for (const auto &edgeKv : snapshot.edges) {
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
    }
    for (const auto &nodeIt: nodeMap) {
        partMap[nodeIt.second.partId].nodeIds.push_back(nodeIt.first);
    }
    for (const auto &partIdIt: snapshot.partIdList) {
        partIds.push_back(oldNewIdMap[QUuid(partIdIt)]);
    }
    
    for (const auto &nodeIt: nodeMap) {
        emit nodeAdded(nodeIt.first);
    }
    for (const auto &edgeIt: edgeMap) {
        emit edgeAdded(edgeIt.first);
    }
    for (const auto &partIt : partMap) {
        emit partAdded(partIt.first);
    }
    
    emit partListChanged();
    emit originChanged();
    emit skeletonChanged();
}

void SkeletonDocument::reset()
{
    originX = 0;
    originY = 0;
    originZ = 0;
    nodeMap.clear();
    edgeMap.clear();
    partMap.clear();
    partIds.clear();
    emit cleanup();
    emit skeletonChanged();
}

void SkeletonDocument::fromSnapshot(const SkeletonSnapshot &snapshot)
{
    reset();
    addFromSnapshot(snapshot);
}

Mesh *SkeletonDocument::takeResultMesh()
{
    Mesh *resultMesh = m_resultMesh;
    m_resultMesh = nullptr;
    return resultMesh;
}

void SkeletonDocument::meshReady()
{
    Mesh *resultMesh = m_meshGenerator->takeResultMesh();
    
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
    
    if (nullptr == m_resultMesh) {
        qDebug() << "Result mesh is null";
    }
    
    delete m_meshGenerator;
    m_meshGenerator = nullptr;
    
    qDebug() << "Mesh generation done";
    
    emit resultMeshChanged();
    
    if (m_resultMeshIsObsolete) {
        generateMesh();
    }
}

void SkeletonDocument::batchChangeBegin()
{
    m_batchChangeRefCount++;
}

void SkeletonDocument::batchChangeEnd()
{
    m_batchChangeRefCount--;
    if (0 == m_batchChangeRefCount) {
        if (m_resultMeshIsObsolete) {
            generateMesh();
        }
    }
}

void SkeletonDocument::generateMesh()
{
    if (nullptr != m_meshGenerator || m_batchChangeRefCount > 0) {
        m_resultMeshIsObsolete = true;
        return;
    }
    
    qDebug() << "Mesh generating..";
    
    m_resultMeshIsObsolete = false;
    
    QThread *thread = new QThread;
    
    SkeletonSnapshot *snapshot = new SkeletonSnapshot;
    toSnapshot(snapshot);
    m_meshGenerator = new MeshGenerator(snapshot, thread);
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
    emit partDisableStateChanged(partId);
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
    emit partDeformWidthChanged(partId);
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
