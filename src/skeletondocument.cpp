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
#include "materialpreviewsgenerator.h"

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
    radiusLocked(false),
    textureGuideImage(nullptr),
    textureImage(nullptr),
    textureBorderImage(nullptr),
    textureAmbientOcclusionImage(nullptr),
    textureColorImage(nullptr),
    rigType(RigType::None),
    weldEnabled(true),
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
    m_sharedContextWidget(nullptr),
    m_allPositionRelatedLocksEnabled(true),
    m_smoothNormal(true),
    m_rigGenerator(nullptr),
    m_resultRigWeightMesh(nullptr),
    m_resultRigBones(nullptr),
    m_resultRigWeights(nullptr),
    m_isRigObsolete(false),
    m_riggedResultContext(new MeshResultContext),
    m_posePreviewsGenerator(nullptr),
    m_currentRigSucceed(false),
    m_materialPreviewsGenerator(nullptr)
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
    delete m_resultRigWeightMesh;
}

void SkeletonDocument::uiReady()
{
    qDebug() << "uiReady";
    emit editModeChanged();
}

void SkeletonDocument::SkeletonDocument::enableAllPositionRelatedLocks()
{
    m_allPositionRelatedLocksEnabled = true;
}

void SkeletonDocument::SkeletonDocument::disableAllPositionRelatedLocks()
{
    m_allPositionRelatedLocksEnabled = false;
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
        const auto newUuid = QUuid::createUuid();
        SkeletonPart &part = partMap[newUuid];
        part.id = newUuid;
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
        const auto newUuid = QUuid::createUuid();
        SkeletonPart &part = partMap[newUuid];
        part.id = newUuid;
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
        const auto newUuid = QUuid::createUuid();
        SkeletonPart &part = partMap[newUuid];
        part.id = newUuid;
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
        auto part = partMap.find(partId);
        if (part != partMap.end())
            part->second.dirty = true;
    }
    SkeletonNode node;
    node.partId = partId;
    node.setRadius(radius);
    node.x = x;
    node.y = y;
    node.z = z;
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

void SkeletonDocument::addPose(QString name, std::map<QString, std::map<QString, QString>> parameters)
{
    QUuid newPoseId = QUuid::createUuid();
    auto &pose = poseMap[newPoseId];
    pose.id = newPoseId;
    
    pose.name = name;
    pose.parameters = parameters;
    pose.dirty = true;
    
    poseIdList.push_back(newPoseId);
    
    emit poseAdded(newPoseId);
    emit poseListChanged();
    emit optionsChanged();
}

void SkeletonDocument::addMotion(QString name, std::vector<HermiteControlNode> controlNodes, std::vector<std::pair<float, QUuid>> keyframes)
{
    QUuid newMotionId = QUuid::createUuid();
    auto &motion = motionMap[newMotionId];
    motion.id = newMotionId;
    
    motion.name = name;
    motion.controlNodes = controlNodes;
    motion.keyframes = keyframes;
    motion.dirty = true;
    
    motionIdList.push_back(newMotionId);
    
    emit motionAdded(newMotionId);
    emit motionListChanged();
    emit optionsChanged();
}

void SkeletonDocument::removeMotion(QUuid motionId)
{
    auto findMotionResult = motionMap.find(motionId);
    if (findMotionResult == motionMap.end()) {
        qDebug() << "Remove a none exist motion:" << motionId;
        return;
    }
    motionIdList.erase(std::remove(motionIdList.begin(), motionIdList.end(), motionId), motionIdList.end());
    motionMap.erase(findMotionResult);
    
    emit motionListChanged();
    emit motionRemoved(motionId);
    emit optionsChanged();
}

void SkeletonDocument::setMotionControlNodes(QUuid motionId, std::vector<HermiteControlNode> controlNodes)
{
    auto findMotionResult = motionMap.find(motionId);
    if (findMotionResult == motionMap.end()) {
        qDebug() << "Find motion failed:" << motionId;
        return;
    }
    findMotionResult->second.controlNodes = controlNodes;
    findMotionResult->second.dirty = true;
    emit motionControlNodesChanged(motionId);
    emit optionsChanged();
}

void SkeletonDocument::setMotionKeyframes(QUuid motionId, std::vector<std::pair<float, QUuid>> keyframes)
{
    auto findMotionResult = motionMap.find(motionId);
    if (findMotionResult == motionMap.end()) {
        qDebug() << "Find motion failed:" << motionId;
        return;
    }
    findMotionResult->second.keyframes = keyframes;
    findMotionResult->second.dirty = true;
    emit motionKeyframesChanged(motionId);
    emit optionsChanged();
}

void SkeletonDocument::renameMotion(QUuid motionId, QString name)
{
    auto findMotionResult = motionMap.find(motionId);
    if (findMotionResult == motionMap.end()) {
        qDebug() << "Find motion failed:" << motionId;
        return;
    }
    if (findMotionResult->second.name == name)
        return;
    
    findMotionResult->second.name = name;
    emit motionNameChanged(motionId);
    emit optionsChanged();
}

void SkeletonDocument::removePose(QUuid poseId)
{
    auto findPoseResult = poseMap.find(poseId);
    if (findPoseResult == poseMap.end()) {
        qDebug() << "Remove a none exist pose:" << poseId;
        return;
    }
    poseIdList.erase(std::remove(poseIdList.begin(), poseIdList.end(), poseId), poseIdList.end());
    poseMap.erase(findPoseResult);
    
    emit poseListChanged();
    emit poseRemoved(poseId);
    emit optionsChanged();
}

void SkeletonDocument::setPoseParameters(QUuid poseId, std::map<QString, std::map<QString, QString>> parameters)
{
    auto findPoseResult = poseMap.find(poseId);
    if (findPoseResult == poseMap.end()) {
        qDebug() << "Find pose failed:" << poseId;
        return;
    }
    findPoseResult->second.parameters = parameters;
    findPoseResult->second.dirty = true;
    emit poseParametersChanged(poseId);
    emit optionsChanged();
}

void SkeletonDocument::renamePose(QUuid poseId, QString name)
{
    auto findPoseResult = poseMap.find(poseId);
    if (findPoseResult == poseMap.end()) {
        qDebug() << "Find pose failed:" << poseId;
        return;
    }
    if (findPoseResult->second.name == name)
        return;
    
    findPoseResult->second.name = name;
    emit poseNameChanged(poseId);
    emit optionsChanged();
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

const SkeletonPose *SkeletonDocument::findPose(QUuid poseId) const
{
    auto it = poseMap.find(poseId);
    if (it == poseMap.end())
        return nullptr;
    return &it->second;
}

const SkeletonMaterial *SkeletonDocument::findMaterial(QUuid materialId) const
{
    auto it = materialMap.find(materialId);
    if (it == materialMap.end())
        return nullptr;
    return &it->second;
}

const SkeletonMotion *SkeletonDocument::findMotion(QUuid motionId) const
{
    auto it = motionMap.find(motionId);
    if (it == motionMap.end())
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
    if (!radiusLocked)
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
    if (m_allPositionRelatedLocksEnabled && isPartReadonly(it->second.partId))
        return;
    if (!(m_allPositionRelatedLocksEnabled && xlocked))
        it->second.x += x;
    if (!(m_allPositionRelatedLocksEnabled && ylocked))
        it->second.y += y;
    if (!(m_allPositionRelatedLocksEnabled && zlocked))
        it->second.z += z;
    auto part = partMap.find(it->second.partId);
    if (part != partMap.end())
        part->second.dirty = true;
    emit nodeOriginChanged(nodeId);
    emit skeletonChanged();
}

void SkeletonDocument::moveOriginBy(float x, float y, float z)
{
    if (!(m_allPositionRelatedLocksEnabled && xlocked))
        originX += x;
    if (!(m_allPositionRelatedLocksEnabled && ylocked))
        originY += y;
    if (!(m_allPositionRelatedLocksEnabled && zlocked))
        originZ += z;
    markAllDirty();
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
    if ((m_allPositionRelatedLocksEnabled && isPartReadonly(it->second.partId)))
        return;
    if (!(m_allPositionRelatedLocksEnabled && xlocked))
        it->second.x = x;
    if (!(m_allPositionRelatedLocksEnabled && ylocked))
        it->second.y = y;
    if (!(m_allPositionRelatedLocksEnabled && zlocked))
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
    if (!radiusLocked)
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

void SkeletonDocument::setNodeBoneMark(QUuid nodeId, SkeletonBoneMark mark)
{
    auto it = nodeMap.find(nodeId);
    if (it == nodeMap.end()) {
        qDebug() << "Find node failed:" << nodeId;
        return;
    }
    if (isPartReadonly(it->second.partId))
        return;
    if (it->second.boneMark == mark)
        return;
    it->second.boneMark = mark;
    auto part = partMap.find(it->second.partId);
    if (part != partMap.end())
        part->second.dirty = true;
    emit nodeBoneMarkChanged(nodeId);
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

void SkeletonDocument::toSnapshot(SkeletonSnapshot *snapshot, const std::set<QUuid> &limitNodeIds,
    SkeletonDocumentToSnapshotFor forWhat,
    const std::set<QUuid> &limitPoseIds,
    const std::set<QUuid> &limitMotionIds,
    const std::set<QUuid> &limitMaterialIds) const
{
    if (SkeletonDocumentToSnapshotFor::Document == forWhat ||
            SkeletonDocumentToSnapshotFor::Nodes == forWhat) {
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
            part["wrapped"] = partIt.second.wrapped ? "true" : "false";
            part["dirty"] = partIt.second.dirty ? "true" : "false";
            if (partIt.second.hasColor)
                part["color"] = partIt.second.color.name();
            if (partIt.second.deformThicknessAdjusted())
                part["deformThickness"] = QString::number(partIt.second.deformThickness);
            if (partIt.second.deformWidthAdjusted())
                part["deformWidth"] = QString::number(partIt.second.deformWidth);
            if (!partIt.second.name.isEmpty())
                part["name"] = partIt.second.name;
            if (partIt.second.materialAdjusted())
                part["materialId"] = partIt.second.materialId.toString();
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
            if (nodeIt.second.boneMark != SkeletonBoneMark::None)
                node["boneMark"] = SkeletonBoneMarkToString(nodeIt.second.boneMark);
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
            if (componentIt.second.smoothAllAdjusted())
                component["smoothAll"] = QString::number(componentIt.second.smoothAll);
            if (componentIt.second.smoothSeamAdjusted())
                component["smoothSeam"] = QString::number(componentIt.second.smoothSeam);
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
    }
    if (SkeletonDocumentToSnapshotFor::Document == forWhat ||
            SkeletonDocumentToSnapshotFor::Materials == forWhat) {
        for (const auto &materialId: materialIdList) {
            if (!limitMaterialIds.empty() && limitMaterialIds.find(materialId) == limitMaterialIds.end())
                continue;
            auto findMaterialResult = materialMap.find(materialId);
            if (findMaterialResult == materialMap.end()) {
                qDebug() << "Find material failed:" << materialId;
                continue;
            }
            auto &materialIt = *findMaterialResult;
            std::map<QString, QString> material;
            material["id"] = materialIt.second.id.toString();
            material["type"] = "MetalRoughness";
            if (!materialIt.second.name.isEmpty())
                material["name"] = materialIt.second.name;
            std::vector<std::pair<std::map<QString, QString>, std::vector<std::map<QString, QString>>>> layers;
            for (const auto &layer: materialIt.second.layers) {
                std::vector<std::map<QString, QString>> maps;
                for (const auto &mapItem: layer.maps) {
                    std::map<QString, QString> textureMap;
                    textureMap["for"] = TextureTypeToString(mapItem.forWhat);
                    textureMap["linkDataType"] = "imageId";
                    textureMap["linkData"] = mapItem.imageId.toString();
                    maps.push_back(textureMap);
                }
                std::map<QString, QString> layerAttributes;
                layers.push_back({layerAttributes, maps});
            }
            snapshot->materials.push_back(std::make_pair(material, layers));
        }
    }
    if (SkeletonDocumentToSnapshotFor::Document == forWhat ||
            SkeletonDocumentToSnapshotFor::Poses == forWhat) {
        for (const auto &poseId: poseIdList) {
            if (!limitPoseIds.empty() && limitPoseIds.find(poseId) == limitPoseIds.end())
                continue;
            auto findPoseResult = poseMap.find(poseId);
            if (findPoseResult == poseMap.end()) {
                qDebug() << "Find pose failed:" << poseId;
                continue;
            }
            auto &poseIt = *findPoseResult;
            std::map<QString, QString> pose;
            pose["id"] = poseIt.second.id.toString();
            if (!poseIt.second.name.isEmpty())
                pose["name"] = poseIt.second.name;
            snapshot->poses.push_back(std::make_pair(pose, poseIt.second.parameters));
        }
    }
    if (SkeletonDocumentToSnapshotFor::Document == forWhat ||
            SkeletonDocumentToSnapshotFor::Motions == forWhat) {
        for (const auto &motionId: motionIdList) {
            if (!limitMotionIds.empty() && limitMotionIds.find(motionId) == limitMotionIds.end())
                continue;
            auto findMotionResult = motionMap.find(motionId);
            if (findMotionResult == motionMap.end()) {
                qDebug() << "Find motion failed:" << motionId;
                continue;
            }
            auto &motionIt = *findMotionResult;
            std::map<QString, QString> motion;
            motion["id"] = motionIt.second.id.toString();
            if (!motionIt.second.name.isEmpty())
                motion["name"] = motionIt.second.name;
            std::vector<std::map<QString, QString>> controlNodesAttributes;
            std::vector<std::map<QString, QString>> keyframesAttributes;
            for (const auto &controlNode: motionIt.second.controlNodes) {
                std::map<QString, QString> attributes;
                attributes["x"] = QString::number(controlNode.position.x());
                attributes["y"] = QString::number(controlNode.position.y());
                attributes["inTangentX"] = QString::number(controlNode.inTangent.x());
                attributes["inTangentY"] = QString::number(controlNode.inTangent.y());
                attributes["outTangentX"] = QString::number(controlNode.outTangent.x());
                attributes["outTangentY"] = QString::number(controlNode.outTangent.y());
                controlNodesAttributes.push_back(attributes);
            }
            for (const auto &keyframe: motionIt.second.keyframes) {
                std::map<QString, QString> attributes;
                attributes["knot"] = QString::number(keyframe.first);
                attributes["linkDataType"] = "poseId";
                attributes["linkData"] = keyframe.second.toString();
                keyframesAttributes.push_back(attributes);
            }
            snapshot->motions.push_back(std::make_tuple(motion, controlNodesAttributes, keyframesAttributes));
        }
    }
    if (SkeletonDocumentToSnapshotFor::Document == forWhat) {
        std::map<QString, QString> canvas;
        canvas["originX"] = QString::number(originX);
        canvas["originY"] = QString::number(originY);
        canvas["originZ"] = QString::number(originZ);
        canvas["rigType"] = RigTypeToString(rigType);
        snapshot->canvas = canvas;
    }
}

void SkeletonDocument::addFromSnapshot(const SkeletonSnapshot &snapshot, bool fromPaste)
{
    bool isOriginChanged = false;
    bool isRigTypeChanged = false;
    if (!fromPaste) {
        const auto &originXit = snapshot.canvas.find("originX");
        const auto &originYit = snapshot.canvas.find("originY");
        const auto &originZit = snapshot.canvas.find("originZ");
        if (originXit != snapshot.canvas.end() &&
                originYit != snapshot.canvas.end() &&
                originZit != snapshot.canvas.end()) {
            originX = originXit->second.toFloat();
            originY = originYit->second.toFloat();
            originZ = originZit->second.toFloat();
            isOriginChanged = true;
        }
        const auto &rigTypeIt = snapshot.canvas.find("rigType");
        if (rigTypeIt != snapshot.canvas.end()) {
            rigType = RigTypeFromString(rigTypeIt->second.toUtf8().constData());
        }
        isRigTypeChanged = true;
    }
    
    std::set<QUuid> newAddedNodeIds;
    std::set<QUuid> newAddedEdgeIds;
    std::set<QUuid> newAddedPartIds;
    std::set<QUuid> newAddedComponentIds;
    
    std::set<QUuid> inversePartIds;
    
    std::map<QUuid, QUuid> oldNewIdMap;
    for (const auto &materialIt: snapshot.materials) {
        const auto &materialAttributes = materialIt.first;
        auto materialType = valueOfKeyInMapOrEmpty(materialAttributes, "type");
        if ("MetalRoughness" != materialType) {
            qDebug() << "Unsupported material type:" << materialType;
            continue;
        }
        QUuid newMaterialId = QUuid::createUuid();
        auto &newMaterial = materialMap[newMaterialId];
        newMaterial.id = newMaterialId;
        newMaterial.name = valueOfKeyInMapOrEmpty(materialAttributes, "name");
        oldNewIdMap[QUuid(valueOfKeyInMapOrEmpty(materialAttributes, "id"))] = newMaterialId;
        for (const auto &layerIt: materialIt.second) {
            SkeletonMaterialLayer layer;
            for (const auto &mapItem: layerIt.second) {
                auto textureTypeString = valueOfKeyInMapOrEmpty(mapItem, "for");
                auto textureType = TextureTypeFromString(textureTypeString.toUtf8().constData());
                if (TextureType::None == textureType) {
                    qDebug() << "Unsupported texture type:" << textureTypeString;
                    continue;
                }
                auto linkTypeString = valueOfKeyInMapOrEmpty(mapItem, "linkDataType");
                if ("imageId" != linkTypeString) {
                    qDebug() << "Unsupported link data type:" << linkTypeString;
                    continue;
                }
                auto imageId = QUuid(valueOfKeyInMapOrEmpty(mapItem, "linkData"));
                SkeletonMaterialMap materialMap;
                materialMap.imageId = imageId;
                materialMap.forWhat = textureType;
                layer.maps.push_back(materialMap);
            }
            newMaterial.layers.push_back(layer);
        }
        materialIdList.push_back(newMaterialId);
        emit materialAdded(newMaterialId);
    }
    for (const auto &partKv: snapshot.parts) {
        const auto newUuid = QUuid::createUuid();
        SkeletonPart &part = partMap[newUuid];
        part.id = newUuid;
        oldNewIdMap[QUuid(partKv.first)] = part.id;
        part.name = valueOfKeyInMapOrEmpty(partKv.second, "name");
        part.visible = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "visible"));
        part.locked = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "locked"));
        part.subdived = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "subdived"));
        part.disabled = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "disabled"));
        part.xMirrored = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "xMirrored"));
        part.zMirrored = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "zMirrored"));
        part.rounded = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "rounded"));
        part.wrapped = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "wrapped"));
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
        const auto &materialIdIt = partKv.second.find("materialId");
        if (materialIdIt != partKv.second.end())
            part.materialId = oldNewIdMap[QUuid(materialIdIt->second)];
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
        node.boneMark = SkeletonBoneMarkFromString(valueOfKeyInMapOrEmpty(nodeKv.second, "boneMark").toUtf8().constData());
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
        const auto &smoothAllIt = componentKv.second.find("smoothAll");
        if (smoothAllIt != componentKv.second.end())
            component.setSmoothAll(smoothAllIt->second.toFloat());
        const auto &smoothSeamIt = componentKv.second.find("smoothSeam");
        if (smoothSeamIt != componentKv.second.end())
            component.setSmoothSeam(smoothSeamIt->second.toFloat());
        //qDebug() << "Add component:" << component.id << " old:" << componentKv.first << "name:" << component.name;
        if ("partId" == linkDataType) {
            QUuid partId = oldNewIdMap[QUuid(linkData)];
            component.linkToPartId = partId;
            //qDebug() << "Add part:" << partId << " from component:" << component.id;
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
            //qDebug() << "Add root component:" << componentId;
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
            //qDebug() << "Add child component:" << childComponentId << "to" << componentId;
            componentMap[componentId].addChild(childComponentId);
            componentMap[childComponentId].parentId = componentId;
        }
    }
    for (const auto &poseIt: snapshot.poses) {
        QUuid newPoseId = QUuid::createUuid();
        auto &newPose = poseMap[newPoseId];
        newPose.id = newPoseId;
        const auto &poseAttributes = poseIt.first;
        newPose.name = valueOfKeyInMapOrEmpty(poseAttributes, "name");
        newPose.parameters = poseIt.second;
        oldNewIdMap[QUuid(valueOfKeyInMapOrEmpty(poseAttributes, "id"))] = newPoseId;
        poseIdList.push_back(newPoseId);
        emit poseAdded(newPoseId);
    }
    for (const auto &motionIt: snapshot.motions) {
        QUuid newMotionId = QUuid::createUuid();
        auto &newMotion = motionMap[newMotionId];
        newMotion.id = newMotionId;
        const auto &motionAttributes = std::get<0>(motionIt);
        newMotion.name = valueOfKeyInMapOrEmpty(motionAttributes, "name");
        for (const auto &attributes: std::get<1>(motionIt)) {
            float x = valueOfKeyInMapOrEmpty(attributes, "x").toFloat();
            float y = valueOfKeyInMapOrEmpty(attributes, "y").toFloat();
            float inTangentX = valueOfKeyInMapOrEmpty(attributes, "inTangentX").toFloat();
            float inTangentY = valueOfKeyInMapOrEmpty(attributes, "inTangentY").toFloat();
            float outTangentX = valueOfKeyInMapOrEmpty(attributes, "outTangentX").toFloat();
            float outTangentY = valueOfKeyInMapOrEmpty(attributes, "outTangentY").toFloat();
            HermiteControlNode hermite(QVector2D(x, y),
                QVector2D(inTangentX, inTangentY), QVector2D(outTangentX, outTangentY));
            newMotion.controlNodes.push_back(hermite);
        }
        for (const auto &attributes: std::get<2>(motionIt)) {
            float knot = valueOfKeyInMapOrEmpty(attributes, "knot").toFloat();
            QString linkDataType = valueOfKeyInMapOrEmpty(attributes, "linkDataType");
            if ("poseId" != linkDataType) {
                qDebug() << "Encounter unknown linkDataType:" << linkDataType;
                continue;
            }
            QUuid linkToPoseId = QUuid(valueOfKeyInMapOrEmpty(attributes, "linkData"));
            auto findPoseResult = poseMap.find(linkToPoseId);
            if (findPoseResult != poseMap.end()) {
                newMotion.keyframes.push_back({knot, findPoseResult->first});
            } else {
                auto findInOldNewIdMapResult = oldNewIdMap.find(linkToPoseId);
                if (findInOldNewIdMapResult != oldNewIdMap.end())
                    newMotion.keyframes.push_back({knot, findInOldNewIdMapResult->second});
            }
        }
        oldNewIdMap[QUuid(valueOfKeyInMapOrEmpty(motionAttributes, "id"))] = newMotionId;
        motionIdList.push_back(newMotionId);
        emit motionAdded(newMotionId);
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
    if (isOriginChanged)
        emit originChanged();
    if (isRigTypeChanged)
        emit rigTypeChanged();
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
    
    if (!snapshot.materials.empty())
        emit materialListChanged();
    if (!snapshot.poses.empty())
        emit poseListChanged();
    if (!snapshot.motions.empty())
        emit motionListChanged();
}

void SkeletonDocument::reset()
{
    originX = 0.0;
    originY = 0.0;
    originZ = 0.0;
    rigType = RigType::None;
    nodeMap.clear();
    edgeMap.clear();
    partMap.clear();
    componentMap.clear();
    materialMap.clear();
    materialIdList.clear();
    poseMap.clear();
    poseIdList.clear();
    motionMap.clear();
    motionIdList.clear();
    rootComponent = SkeletonComponent();
    emit cleanup();
    emit skeletonChanged();
}

void SkeletonDocument::fromSnapshot(const SkeletonSnapshot &snapshot)
{
    reset();
    addFromSnapshot(snapshot, false);
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
    if (nullptr == m_resultTextureMesh)
        return nullptr;
    MeshLoader *resultTextureMesh = new MeshLoader(*m_resultTextureMesh);
    return resultTextureMesh;
}

MeshLoader *SkeletonDocument::takeResultRigWeightMesh()
{
    if (nullptr == m_resultRigWeightMesh)
        return nullptr;
    MeshLoader *resultMesh = new MeshLoader(*m_resultRigWeightMesh);
    return resultMesh;
}

void SkeletonDocument::meshReady()
{
    MeshLoader *resultMesh = m_meshGenerator->takeResultMesh();
    MeshResultContext *meshResultContext = m_meshGenerator->takeMeshResultContext();
    
    for (auto &partId: m_meshGenerator->generatedPreviewPartIds()) {
        auto part = partMap.find(partId);
        if (part != partMap.end()) {
            MeshLoader *resultPartPreviewMesh = m_meshGenerator->takePartPreviewMesh(partId);
            part->second.updatePreviewMesh(resultPartPreviewMesh);
            emit partPreviewChanged(partId);
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
    m_isRigObsolete = true;
    
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

void SkeletonDocument::toggleSmoothNormal()
{
    m_smoothNormal = !m_smoothNormal;
    regenerateMesh();
}

void SkeletonDocument::enableWeld(bool enabled)
{
    weldEnabled = enabled;
    regenerateMesh();
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
    m_meshGenerator = new MeshGenerator(snapshot);
    m_meshGenerator->setSmoothNormal(m_smoothNormal);
    m_meshGenerator->setWeldEnabled(weldEnabled);
    m_meshGenerator->setGeneratedCacheContext(&m_generatedCacheContext);
    if (nullptr != m_sharedContextWidget)
        m_meshGenerator->setSharedContextWidget(m_sharedContextWidget);
    for (auto &part: partMap) {
        m_meshGenerator->addPartPreviewRequirement(part.first);
    }
    m_meshGenerator->moveToThread(thread);
    connect(thread, &QThread::started, m_meshGenerator, &MeshGenerator::process);
    connect(m_meshGenerator, &MeshGenerator::finished, this, &SkeletonDocument::meshReady);
    connect(m_meshGenerator, &MeshGenerator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    emit meshGenerating();
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
    m_textureGenerator = new TextureGenerator(*m_postProcessedResultContext);
    for (const auto &bmeshNode: m_postProcessedResultContext->bmeshNodes) {
        for (size_t i = 0; i < sizeof(bmeshNode.material.textureImages) / sizeof(bmeshNode.material.textureImages[0]); ++i) {
            TextureType forWhat = (TextureType)(i + 1);
            const QImage *image = bmeshNode.material.textureImages[i];
            if (nullptr != image) {
                if (TextureType::BaseColor == forWhat)
                    m_textureGenerator->addPartColorMap(bmeshNode.partId, image);
                else if (TextureType::Normal == forWhat)
                    m_textureGenerator->addPartNormalMap(bmeshNode.partId, image);
                else if (TextureType::Metalness == forWhat)
                    m_textureGenerator->addPartMetalnessMap(bmeshNode.partId, image);
                else if (TextureType::Roughness == forWhat)
                    m_textureGenerator->addPartRoughnessMap(bmeshNode.partId, image);
                else if (TextureType::AmbientOcclusion == forWhat)
                    m_textureGenerator->addPartAmbientOcclusionMap(bmeshNode.partId, image);
            }
        }
    }
    m_textureGenerator->moveToThread(thread);
    connect(thread, &QThread::started, m_textureGenerator, &TextureGenerator::process);
    connect(m_textureGenerator, &TextureGenerator::finished, this, &SkeletonDocument::textureReady);
    connect(m_textureGenerator, &TextureGenerator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    emit textureGenerating();
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
    m_ambientOcclusionBaker->setBakeSize(TextureGenerator::m_textureSize, TextureGenerator::m_textureSize);
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

    m_isPostProcessResultObsolete = false;

    if (!m_currentMeshResultContext) {
        qDebug() << "MeshLoader is null";
        return;
    }

    qDebug() << "Post processing..";

    QThread *thread = new QThread;
    m_postProcessor = new MeshResultPostProcessor(*m_currentMeshResultContext);
    m_postProcessor->moveToThread(thread);
    connect(thread, &QThread::started, m_postProcessor, &MeshResultPostProcessor::process);
    connect(m_postProcessor, &MeshResultPostProcessor::finished, this, &SkeletonDocument::postProcessedMeshResultReady);
    connect(m_postProcessor, &MeshResultPostProcessor::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    emit postProcessing();
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

void SkeletonDocument::setComponentSmoothAll(QUuid componentId, float toSmoothAll)
{
    auto component = componentMap.find(componentId);
    if (component == componentMap.end()) {
        qDebug() << "Component not found:" << componentId;
        return;
    }
    component->second.setSmoothAll(toSmoothAll);
    component->second.dirty = true;
    emit componentSmoothAllChanged(componentId);
    emit skeletonChanged();
}

void SkeletonDocument::setComponentSmoothSeam(QUuid componentId, float toSmoothSeam)
{
    auto component = componentMap.find(componentId);
    if (component == componentMap.end()) {
        qDebug() << "Component not found:" << componentId;
        return;
    }
    component->second.setSmoothSeam(toSmoothSeam);
    component->second.dirty = true;
    emit componentSmoothSeamChanged(componentId);
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

void SkeletonDocument::createNewChildComponent(QUuid parentComponentId)
{
    SkeletonComponent *parentComponent = (SkeletonComponent *)findComponent(parentComponentId);
    if (!parentComponent->linkToPartId.isNull()) {
        parentComponentId = parentComponent->parentId;
        parentComponent = (SkeletonComponent *)findComponent(parentComponentId);
    }
    
    SkeletonComponent newComponent(QUuid::createUuid());
    newComponent.name = tr("Group") + " " + QString::number(componentMap.size() - partMap.size() + 1);
    
    parentComponent->addChild(newComponent.id);
    newComponent.parentId = parentComponentId;
    
    componentMap[newComponent.id] = newComponent;
    
    emit componentChildrenChanged(parentComponentId);
    emit componentAdded(newComponent.id);
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
    
    if (!component->second.linkToPartId.isNull()) {
        removePartDontCareComponent(component->second.linkToPartId);
    }
    
    auto childrenIds = component->second.childrenIds;
    for (const auto &childId: childrenIds) {
        removeComponentRecursively(childId);
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
    const SkeletonComponent *component = findComponent(m_currentCanvasComponentId);
    if (nullptr == component) {
        //qDebug() << "Current component switch to nullptr componentId:" << componentId;
        m_currentCanvasComponentId = QUuid();
    } else {
        //qDebug() << "Current component switch to " << component->name << "componentId:" << componentId;
        if (!component->linkToPartId.isNull()) {
            m_currentCanvasComponentId = component->parentId;
            component = findComponent(m_currentCanvasComponentId);
            //if (nullptr != component) {
            //    qDebug() << "Then switch to " << component->name << "componentId:" << m_currentCanvasComponentId;
            //}
        }
    }
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

void SkeletonDocument::setPartMaterialId(QUuid partId, QUuid materialId)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        qDebug() << "Part not found:" << partId;
        return;
    }
    if (part->second.materialId == materialId)
        return;
    part->second.materialId = materialId;
    part->second.dirty = true;
    emit partMaterialIdChanged(partId);
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

void SkeletonDocument::setPartWrapState(QUuid partId, bool wrapped)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        qDebug() << "Part not found:" << partId;
        return;
    }
    if (part->second.wrapped == wrapped)
        return;
    part->second.wrapped = wrapped;
    part->second.dirty = true;
    emit partWrapStateChanged(partId);
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
        addFromSnapshot(snapshot, true);
    }
}

bool SkeletonDocument::hasPastableNodesInClipboard() const
{
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();
    if (mimeData->hasText()) {
        if (-1 != mimeData->text().indexOf("<node "))
            return true;
    }
    return false;
}

bool SkeletonDocument::hasPastableMaterialsInClipboard() const
{
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();
    if (mimeData->hasText()) {
        if (-1 != mimeData->text().indexOf("<material "))
            return true;
    }
    return false;
}

bool SkeletonDocument::hasPastablePosesInClipboard() const
{
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();
    if (mimeData->hasText()) {
        if (-1 != mimeData->text().indexOf("<pose "))
            return true;
    }
    return false;
}

bool SkeletonDocument::hasPastableMotionsInClipboard() const
{
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();
    if (mimeData->hasText()) {
        if (-1 != mimeData->text().indexOf("<motion "))
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

void SkeletonDocument::setRadiusLockState(bool locked)
{
    if (radiusLocked == locked)
        return;
    radiusLocked = locked;
    emit radiusLockStateChanged();
}

bool SkeletonDocument::isExportReady() const
{
    if (m_isResultMeshObsolete ||
            m_isTextureObsolete ||
            m_isPostProcessResultObsolete ||
            m_isRigObsolete ||
            m_meshGenerator ||
            m_textureGenerator ||
            m_postProcessor ||
            m_rigGenerator)
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

void SkeletonDocument::collectComponentDescendantComponents(QUuid componentId, std::vector<QUuid> &componentIds) const
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

void SkeletonDocument::generateRig()
{
    if (nullptr != m_rigGenerator) {
        m_isRigObsolete = true;
        return;
    }
    
    m_isRigObsolete = false;
    
    if (RigType::None == rigType || nullptr == m_currentMeshResultContext) {
        removeRigResults();
        return;
    }
    
    qDebug() << "Rig generating..";
    
    QThread *thread = new QThread;
    m_rigGenerator = new RigGenerator(*m_currentMeshResultContext);
    m_rigGenerator->moveToThread(thread);
    connect(thread, &QThread::started, m_rigGenerator, &RigGenerator::process);
    connect(m_rigGenerator, &RigGenerator::finished, this, &SkeletonDocument::rigReady);
    connect(m_rigGenerator, &RigGenerator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void SkeletonDocument::rigReady()
{
    m_currentRigSucceed = m_rigGenerator->isSucceed();

    delete m_resultRigWeightMesh;
    m_resultRigWeightMesh = m_rigGenerator->takeResultMesh();
    
    delete m_resultRigBones;
    m_resultRigBones = m_rigGenerator->takeResultBones();
    
    delete m_resultRigWeights;
    m_resultRigWeights = m_rigGenerator->takeResultWeights();
    
    m_resultRigMissingMarkNames = m_rigGenerator->missingMarkNames();
    m_resultRigErrorMarkNames = m_rigGenerator->errorMarkNames();
    
    delete m_riggedResultContext;
    m_riggedResultContext = m_rigGenerator->takeMeshResultContext();
    if (nullptr == m_riggedResultContext)
        m_riggedResultContext = new MeshResultContext;
    
    delete m_rigGenerator;
    m_rigGenerator = nullptr;
    
    qDebug() << "Rig generation done";
    
    emit resultRigChanged();
    
    if (m_isRigObsolete) {
        generateRig();
    } else {
        checkExportReadyState();
    }
}

const std::vector<AutoRiggerBone> *SkeletonDocument::resultRigBones() const
{
    return m_resultRigBones;
}

const std::map<int, AutoRiggerVertexWeights> *SkeletonDocument::resultRigWeights() const
{
    return m_resultRigWeights;
}

void SkeletonDocument::removeRigResults()
{
    delete m_resultRigBones;
    m_resultRigBones = nullptr;
    
    delete m_resultRigWeights;
    m_resultRigWeights = nullptr;
    
    delete m_resultRigWeightMesh;
    m_resultRigWeightMesh = nullptr;
    
    m_resultRigErrorMarkNames.clear();
    m_resultRigMissingMarkNames.clear();
    
    emit resultRigChanged();
}

void SkeletonDocument::setRigType(RigType toRigType)
{
    if (rigType == toRigType)
        return;
    
    rigType = toRigType;
    
    m_isRigObsolete = true;
    
    removeRigResults();
    
    emit rigTypeChanged();
    emit rigChanged();
}

const std::vector<QString> &SkeletonDocument::resultRigMissingMarkNames() const
{
    return m_resultRigMissingMarkNames;
}

const std::vector<QString> &SkeletonDocument::resultRigErrorMarkNames() const
{
    return m_resultRigErrorMarkNames;
}

const MeshResultContext &SkeletonDocument::currentRiggedResultContext() const
{
    return *m_riggedResultContext;
}

bool SkeletonDocument::currentRigSucceed() const
{
    return m_currentRigSucceed;
}

void SkeletonDocument::generatePosePreviews()
{
    if (nullptr != m_posePreviewsGenerator) {
        return;
    }
    
    const std::vector<AutoRiggerBone> *rigBones = resultRigBones();
    const std::map<int, AutoRiggerVertexWeights> *rigWeights = resultRigWeights();
    
    if (nullptr == rigBones || nullptr == rigWeights) {
        return;
    }

    QThread *thread = new QThread;
    m_posePreviewsGenerator = new PosePreviewsGenerator(rigBones,
        rigWeights, *m_riggedResultContext);
    bool hasDirtyPose = false;
    for (auto &poseIt: poseMap) {
        if (!poseIt.second.dirty)
            continue;
        m_posePreviewsGenerator->addPose(poseIt.first, poseIt.second.parameters);
        poseIt.second.dirty = false;
        hasDirtyPose = true;
    }
    if (!hasDirtyPose) {
        delete m_posePreviewsGenerator;
        m_posePreviewsGenerator = nullptr;
        delete thread;
        return;
    }
    
    qDebug() << "Pose previews generating..";
    
    m_posePreviewsGenerator->moveToThread(thread);
    connect(thread, &QThread::started, m_posePreviewsGenerator, &PosePreviewsGenerator::process);
    connect(m_posePreviewsGenerator, &PosePreviewsGenerator::finished, this, &SkeletonDocument::posePreviewsReady);
    connect(m_posePreviewsGenerator, &PosePreviewsGenerator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void SkeletonDocument::posePreviewsReady()
{
    for (const auto &poseId: m_posePreviewsGenerator->generatedPreviewPoseIds()) {
        auto pose = poseMap.find(poseId);
        if (pose != poseMap.end()) {
            MeshLoader *resultPartPreviewMesh = m_posePreviewsGenerator->takePreview(poseId);
            pose->second.updatePreviewMesh(resultPartPreviewMesh);
            emit posePreviewChanged(poseId);
        }
    }

    delete m_posePreviewsGenerator;
    m_posePreviewsGenerator = nullptr;
    
    qDebug() << "Pose previews generation done";
    
    generatePosePreviews();
}

void SkeletonDocument::addMaterial(QString name, std::vector<SkeletonMaterialLayer> layers)
{
    QUuid newMaterialId = QUuid::createUuid();
    auto &material = materialMap[newMaterialId];
    material.id = newMaterialId;
    
    material.name = name;
    material.layers = layers;
    material.dirty = true;
    
    materialIdList.push_back(newMaterialId);
    
    emit materialAdded(newMaterialId);
    emit materialListChanged();
    emit optionsChanged();
}

void SkeletonDocument::removeMaterial(QUuid materialId)
{
    auto findMaterialResult = materialMap.find(materialId);
    if (findMaterialResult == materialMap.end()) {
        qDebug() << "Remove a none exist material:" << materialId;
        return;
    }
    materialIdList.erase(std::remove(materialIdList.begin(), materialIdList.end(), materialId), materialIdList.end());
    materialMap.erase(findMaterialResult);
    
    emit materialListChanged();
    emit materialRemoved(materialId);
    emit optionsChanged();
}

void SkeletonDocument::setMaterialLayers(QUuid materialId, std::vector<SkeletonMaterialLayer> layers)
{
    auto findMaterialResult = materialMap.find(materialId);
    if (findMaterialResult == materialMap.end()) {
        qDebug() << "Find material failed:" << materialId;
        return;
    }
    findMaterialResult->second.layers = layers;
    findMaterialResult->second.dirty = true;
    emit materialLayersChanged(materialId);
    emit optionsChanged();
}

void SkeletonDocument::renameMaterial(QUuid materialId, QString name)
{
    auto findMaterialResult = materialMap.find(materialId);
    if (findMaterialResult == materialMap.end()) {
        qDebug() << "Find material failed:" << materialId;
        return;
    }
    if (findMaterialResult->second.name == name)
        return;
    
    findMaterialResult->second.name = name;
    emit materialNameChanged(materialId);
    emit optionsChanged();
}

void SkeletonDocument::generateMaterialPreviews()
{
    if (nullptr != m_materialPreviewsGenerator) {
        return;
    }

    QThread *thread = new QThread;
    m_materialPreviewsGenerator = new MaterialPreviewsGenerator();
    bool hasDirtyMaterial = false;
    for (auto &materialIt: materialMap) {
        if (!materialIt.second.dirty)
            continue;
        m_materialPreviewsGenerator->addMaterial(materialIt.first, materialIt.second.layers);
        materialIt.second.dirty = false;
        hasDirtyMaterial = true;
    }
    if (!hasDirtyMaterial) {
        delete m_materialPreviewsGenerator;
        m_materialPreviewsGenerator = nullptr;
        delete thread;
        return;
    }
    
    qDebug() << "Material previews generating..";
    
    m_materialPreviewsGenerator->moveToThread(thread);
    connect(thread, &QThread::started, m_materialPreviewsGenerator, &MaterialPreviewsGenerator::process);
    connect(m_materialPreviewsGenerator, &MaterialPreviewsGenerator::finished, this, &SkeletonDocument::materialPreviewsReady);
    connect(m_materialPreviewsGenerator, &MaterialPreviewsGenerator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void SkeletonDocument::materialPreviewsReady()
{
    for (const auto &materialId: m_materialPreviewsGenerator->generatedPreviewMaterialIds()) {
        auto material = materialMap.find(materialId);
        if (material != materialMap.end()) {
            MeshLoader *resultPartPreviewMesh = m_materialPreviewsGenerator->takePreview(materialId);
            material->second.updatePreviewMesh(resultPartPreviewMesh);
            emit materialPreviewChanged(materialId);
        }
    }

    delete m_materialPreviewsGenerator;
    m_materialPreviewsGenerator = nullptr;
    
    qDebug() << "Material previews generation done";
    
    generateMaterialPreviews();
}

