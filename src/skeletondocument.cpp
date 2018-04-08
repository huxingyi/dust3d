#include <QFileDialog>
#include <QDebug>
#include <QThread>
#include <QGuiApplication>
#include "skeletondocument.h"
#include "util.h"

SkeletonDocument::SkeletonDocument() :
    m_resultMeshIsObsolete(false),
    m_resultMesh(nullptr),
    m_meshGenerator(nullptr)
{
}

SkeletonDocument::~SkeletonDocument()
{
    delete m_resultMesh;
}

void SkeletonDocument::uiReady()
{
    emit editModeChanged();
}

void SkeletonDocument::removePart(QUuid partId)
{
}

void SkeletonDocument::removeEdge(QUuid edgeId)
{
    const SkeletonEdge *edge = findEdge(edgeId);
    if (nullptr == edge) {
        qDebug() << "Find edge failed:" << edgeId;
        return;
    }
    if (isPartLocked(edge->partId))
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
    if (isPartLocked(node->partId))
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
            return;
        }
        partId = fromNode->partId;
        if (isPartLocked(partId))
            return;
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

void SkeletonDocument::addEdge(QUuid fromNodeId, QUuid toNodeId)
{
    const SkeletonNode *fromNode = nullptr;
    const SkeletonNode *toNode = nullptr;
    bool toPartRemoved = false;
    
    fromNode = findNode(fromNodeId);
    if (nullptr == fromNode) {
        qDebug() << "Find node failed:" << fromNodeId;
        return;
    }
    
    if (isPartLocked(fromNode->partId))
        return;
    
    toNode = findNode(toNodeId);
    if (nullptr == toNode) {
        qDebug() << "Find node failed:" << toNodeId;
        return;
    }
    
    if (isPartLocked(toNode->partId))
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
    if (isPartLocked(it->second.partId))
        return;
    it->second.radius += amount;
    emit nodeRadiusChanged(nodeId);
    emit skeletonChanged();
}

bool SkeletonDocument::isPartLocked(QUuid partId)
{
    const SkeletonPart *part = findPart(partId);
    if (nullptr == part) {
        qDebug() << "Find part failed:" << partId;
        return true;
    }
    return part->locked;
}

void SkeletonDocument::moveNodeBy(QUuid nodeId, float x, float y, float z)
{
    auto it = nodeMap.find(nodeId);
    if (it == nodeMap.end()) {
        qDebug() << "Find node failed:" << nodeId;
        return;
    }
    if (isPartLocked(it->second.partId))
        return;
    it->second.x += x;
    it->second.y += y;
    it->second.z += z;
    emit nodeOriginChanged(nodeId);
    emit skeletonChanged();
}

void SkeletonDocument::setNodeOrigin(QUuid nodeId, float x, float y, float z)
{
    auto it = nodeMap.find(nodeId);
    if (it == nodeMap.end()) {
        qDebug() << "Find node failed:" << nodeId;
        return;
    }
    if (isPartLocked(it->second.partId))
        return;
    it->second.x = x;
    it->second.y = y;
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
    if (isPartLocked(it->second.partId))
        return;
    it->second.radius = radius;
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

void SkeletonDocument::setEdgeBranchMode(QUuid edgeId, SkeletonEdgeBranchMode mode)
{
    auto edgeIt = edgeMap.find(edgeId);
    if (edgeIt == edgeMap.end()) {
        qDebug() << "Find edge failed:" << edgeId;
        return;
    }
    edgeIt->second.branchMode = mode;
}

void SkeletonDocument::setNodeRootMarkMode(QUuid nodeId, SkeletonNodeRootMarkMode mode)
{
    auto nodeIt = nodeMap.find(nodeId);
    if (nodeIt == nodeMap.end()) {
        qDebug() << "Find node failed:" << nodeId;
        return;
    }
    nodeIt->second.rootMarkMode = mode;
}

void SkeletonDocument::toSnapshot(SkeletonSnapshot *snapshot)
{
    for (const auto &partIt : partMap) {
        std::map<QString, QString> part;
        part["id"] = partIt.second.id.toString();
        part["visible"] = partIt.second.visible ? "true" : "false";
        part["locked"] = partIt.second.locked ? "true" : "false";
        part["subdived"] = partIt.second.subdived ? "true" : "false";
        if (!partIt.second.name.isEmpty())
            part["name"] = partIt.second.name;
        snapshot->parts[part["id"]] = part;
    }
    for (const auto &nodeIt: nodeMap) {
        std::map<QString, QString> node;
        node["id"] = nodeIt.second.id.toString();
        node["radius"] = QString::number(nodeIt.second.radius);
        node["x"] = QString::number(nodeIt.second.x);
        node["y"] = QString::number(nodeIt.second.y);
        node["z"] = QString::number(nodeIt.second.z);
        node["rootMarkMode"] = SkeletonNodeRootMarkModeToString(nodeIt.second.rootMarkMode);
        node["partId"] = nodeIt.second.partId.toString();
        if (!nodeIt.second.name.isEmpty())
            node["name"] = nodeIt.second.name;
        snapshot->nodes[node["id"]] = node;
        qDebug() << "Export node to snapshot " << node["id"] << node["x"] << node["y"] << node["z"];
    }
    for (const auto &edgeIt: edgeMap) {
        if (edgeIt.second.nodeIds.size() != 2)
            continue;
        std::map<QString, QString> edge;
        edge["id"] = edgeIt.second.id.toString();
        edge["from"] = edgeIt.second.nodeIds[0].toString();
        edge["to"] = edgeIt.second.nodeIds[1].toString();
        edge["branchMode"] = SkeletonEdgeBranchModeToString(edgeIt.second.branchMode);
        edge["partId"] = edgeIt.second.partId.toString();
        if (!edgeIt.second.name.isEmpty())
            edge["name"] = edgeIt.second.name;
        snapshot->edges[edge["id"]] = edge;
        qDebug() << "Export edge to snapshot " << edge["from"] << "<=>" << edge["to"];
    }
    for (const auto &partIdIt: partIds) {
        snapshot->partIdList.push_back(partIdIt.toString());
    }
}

void SkeletonDocument::fromSnapshot(const SkeletonSnapshot &snapshot)
{
    for (const auto &nodeIt: nodeMap) {
        emit nodeRemoved(nodeIt.first);
    }
    for (const auto &edgeIt: edgeMap) {
        emit edgeRemoved(edgeIt.first);
    }
    for (const auto &partIt : partMap) {
        emit partRemoved(partIt.first);
    }
    
    nodeMap.clear();
    edgeMap.clear();
    partMap.clear();
    
    for (const auto &nodeKv : snapshot.nodes) {
        SkeletonNode node(QUuid(nodeKv.first));
        node.name = valueOfKeyInMapOrEmpty(nodeKv.second, "name");
        node.radius = valueOfKeyInMapOrEmpty(nodeKv.second, "radius").toFloat();
        node.x = valueOfKeyInMapOrEmpty(nodeKv.second, "x").toFloat();
        node.y = valueOfKeyInMapOrEmpty(nodeKv.second, "y").toFloat();
        node.z = valueOfKeyInMapOrEmpty(nodeKv.second, "z").toFloat();
        node.partId = QUuid(valueOfKeyInMapOrEmpty(nodeKv.second, "partId"));
        node.rootMarkMode = SkeletonNodeRootMarkModeFromString(valueOfKeyInMapOrEmpty(nodeKv.second, "rootMarkMode"));
        nodeMap[node.id] = node;
    }
    for (const auto &edgeKv : snapshot.edges) {
        SkeletonEdge edge(QUuid(edgeKv.first));
        edge.name = valueOfKeyInMapOrEmpty(edgeKv.second, "name");
        edge.partId = QUuid(valueOfKeyInMapOrEmpty(edgeKv.second, "partId"));
        edge.branchMode = SkeletonEdgeBranchModeFromString(valueOfKeyInMapOrEmpty(edgeKv.second, "branchMode"));
        QString fromNodeId = valueOfKeyInMapOrEmpty(edgeKv.second, "from");
        if (!fromNodeId.isEmpty())
            edge.nodeIds.push_back(QUuid(fromNodeId));
        QString toNodeId = valueOfKeyInMapOrEmpty(edgeKv.second, "to");
        if (!toNodeId.isEmpty())
            edge.nodeIds.push_back(QUuid(toNodeId));
        edgeMap[edge.id] = edge;
    }
    for (const auto &partKv : snapshot.parts) {
        SkeletonPart part(QUuid(partKv.first));
        part.name = valueOfKeyInMapOrEmpty(partKv.second, "name");
        part.visible = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "visible"));
        part.locked = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "locked"));
        part.subdived = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "subdived"));
        partMap[part.id] = part;
    }
    for (const auto &partIdIt: snapshot.partIdList) {
        partIds.push_back(QUuid(partIdIt));
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
}

const char *SkeletonNodeRootMarkModeToString(SkeletonNodeRootMarkMode mode)
{
    switch (mode) {
        case SkeletonNodeRootMarkMode::Auto:
            return "auto";
        case SkeletonNodeRootMarkMode::MarkAsRoot:
            return "markAsRoot";
        case SkeletonNodeRootMarkMode::MarkAsNotRoot:
            return "markAsNotRoot";
        default:
            return "";
    }
}

SkeletonNodeRootMarkMode SkeletonNodeRootMarkModeFromString(const QString &mode)
{
    if (mode == "auto")
        return SkeletonNodeRootMarkMode::Auto;
    if (mode == "markAsRoot")
        return SkeletonNodeRootMarkMode::MarkAsRoot;
    if (mode == "markAsNotRoot")
        return SkeletonNodeRootMarkMode::MarkAsNotRoot;
    return SkeletonNodeRootMarkMode::Auto;
}

const char *SkeletonEdgeBranchModeToString(SkeletonEdgeBranchMode mode)
{
    switch (mode) {
        case SkeletonEdgeBranchMode::Auto:
            return "auto";
        case SkeletonEdgeBranchMode::NoTrivial:
            return "noTrivial";
        case SkeletonEdgeBranchMode::Trivial:
            return "trivial";
        default:
            return "";
    }
}

SkeletonEdgeBranchMode SkeletonEdgeBranchModeFromString(const QString &mode)
{
    if (mode == "auto")
        return SkeletonEdgeBranchMode::Auto;
    if (mode == "noTrivial")
        return SkeletonEdgeBranchMode::NoTrivial;
    if (mode == "trivial")
        return SkeletonEdgeBranchMode::Trivial;
    return SkeletonEdgeBranchMode::Auto;
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

void SkeletonDocument::generateMesh()
{
    if (nullptr != m_meshGenerator) {
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
    m_meshGenerator->addPreviewRequirement();
    for (auto &part: partMap) {
        //if (part.second.previewIsObsolete) {
        //    part.second.previewIsObsolete = false;
            m_meshGenerator->addPartPreviewRequirement(part.first.toString());
        //}
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
    emit partSubdivStateChanged(partId);
    emit skeletonChanged();
}

