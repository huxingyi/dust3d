#include <QFileDialog>
#include <QDebug>
#include <QThread>
#include <QGuiApplication>
#include <QClipboard>
#include <QMimeData>
#include <QApplication>
#include <QVector3D>
#include <functional>
#include <QBuffer>
#include <QElapsedTimer>
#include <queue>
#include "document.h"
#include "util.h"
#include "snapshotxml.h"
#include "materialpreviewsgenerator.h"
#include "motionsgenerator.h"
#include "skeletonside.h"
#include "scriptrunner.h"
#include "mousepicker.h"
#include "imageforever.h"
#include "contourtopartconverter.h"

unsigned long Document::m_maxSnapshot = 1000;

Document::Document() :
    SkeletonDocument(),
    // public
    textureGuideImage(nullptr),
    textureImage(nullptr),
    textureBorderImage(nullptr),
    textureColorImage(nullptr),
    textureNormalImage(nullptr),
    textureMetalnessRoughnessAmbientOcclusionImage(nullptr),
    textureMetalnessImage(nullptr),
    textureRoughnessImage(nullptr),
    textureAmbientOcclusionImage(nullptr),
    rigType(RigType::None),
    weldEnabled(true),
    // private
    m_isResultMeshObsolete(false),
    m_meshGenerator(nullptr),
    m_resultMesh(nullptr),
    m_resultMeshCutFaceTransforms(nullptr),
    m_resultMeshNodesCutFaces(nullptr),
    m_isMeshGenerationSucceed(true),
    m_batchChangeRefCount(0),
    m_currentOutcome(nullptr),
    m_isTextureObsolete(false),
    m_textureGenerator(nullptr),
    m_isPostProcessResultObsolete(false),
    m_postProcessor(nullptr),
    m_postProcessedOutcome(new Outcome),
    m_resultTextureMesh(nullptr),
    m_textureImageUpdateVersion(0),
    m_sharedContextWidget(nullptr),
    m_allPositionRelatedLocksEnabled(true),
    m_smoothNormal(!Preferences::instance().flatShading()),
    m_rigGenerator(nullptr),
    m_resultRigWeightMesh(nullptr),
    m_resultRigBones(nullptr),
    m_resultRigWeights(nullptr),
    m_isRigObsolete(false),
    m_riggedOutcome(new Outcome),
    m_posePreviewsGenerator(nullptr),
    m_currentRigSucceed(false),
    m_materialPreviewsGenerator(nullptr),
    m_motionsGenerator(nullptr),
    m_meshGenerationId(0),
    m_nextMeshGenerationId(1),
    m_scriptRunner(nullptr),
    m_isScriptResultObsolete(false),
    m_mousePicker(nullptr),
    m_isMouseTargetResultObsolete(false),
    m_paintMode(PaintMode::None),
    m_mousePickRadius(0.2),
    m_saveNextPaintSnapshot(false)
{
    connect(&Preferences::instance(), &Preferences::partColorChanged, this, &Document::applyPreferencePartColorChange);
    connect(&Preferences::instance(), &Preferences::flatShadingChanged, this, &Document::applyPreferenceFlatShadingChange);
}

void Document::applyPreferencePartColorChange()
{
    regenerateMesh();
}

void Document::applyPreferenceFlatShadingChange()
{
    m_smoothNormal = !Preferences::instance().flatShading();
    regenerateMesh();
}

Document::~Document()
{
    delete m_resultMesh;
    delete m_resultMeshCutFaceTransforms;
    delete m_resultMeshNodesCutFaces;
    delete m_postProcessedOutcome;
    delete textureGuideImage;
    delete textureImage;
    delete textureColorImage;
    delete textureNormalImage;
    delete textureMetalnessRoughnessAmbientOcclusionImage;
    delete textureMetalnessImage;
    delete textureRoughnessImage;
    delete textureAmbientOcclusionImage;
    delete textureBorderImage;
    delete m_resultTextureMesh;
    delete m_resultRigWeightMesh;
}

void Document::uiReady()
{
    qDebug() << "uiReady";
    emit editModeChanged();
}

void Document::Document::enableAllPositionRelatedLocks()
{
    m_allPositionRelatedLocksEnabled = true;
}

void Document::Document::disableAllPositionRelatedLocks()
{
    m_allPositionRelatedLocksEnabled = false;
}

void Document::breakEdge(QUuid edgeId)
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
    QVector3D firstOrigin(firstNode->getX(), firstNode->getY(), firstNode->getZ());
    QVector3D secondOrigin(secondNode->getX(), secondNode->getY(), secondNode->getZ());
    QVector3D middleOrigin = (firstOrigin + secondOrigin) / 2;
    float middleRadius = (firstNode->radius + secondNode->radius) / 2;
    removeEdge(edgeId);
    QUuid middleNodeId = createNode(QUuid::createUuid(), middleOrigin.x(), middleOrigin.y(), middleOrigin.z(), middleRadius, firstNodeId);
    if (middleNodeId.isNull()) {
        qDebug() << "Add middle node failed";
        return;
    }
    addEdge(middleNodeId, secondNodeId);
}

void Document::removeEdge(QUuid edgeId)
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
    std::vector<std::pair<QUuid, size_t>> newPartNodeNumMap;
    std::vector<QUuid> newPartIds;
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
        newPartNodeNumMap.push_back({part.id, part.nodeIds.size()});
        newPartIds.push_back(part.id);
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
    
    if (!newPartNodeNumMap.empty()) {
        std::sort(newPartNodeNumMap.begin(), newPartNodeNumMap.end(), [&](
                const std::pair<QUuid, size_t> &first, const std::pair<QUuid, size_t> &second) {
            return first.second > second.second;
        });
        updateLinkedPart(oldPartId, newPartNodeNumMap[0].first);
    }
    
    for (const auto &partId: newPartIds) {
        checkPartGrid(partId);
    }
    
    emit skeletonChanged();
}

void Document::removeNode(QUuid nodeId)
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
    std::vector<std::pair<QUuid, size_t>> newPartNodeNumMap;
    std::vector<QUuid> newPartIds;
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
        newPartNodeNumMap.push_back({part.id, part.nodeIds.size()});
        newPartIds.push_back(part.id);
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
    
    if (!newPartNodeNumMap.empty()) {
        std::sort(newPartNodeNumMap.begin(), newPartNodeNumMap.end(), [&](
                const std::pair<QUuid, size_t> &first, const std::pair<QUuid, size_t> &second) {
            return first.second > second.second;
        });
        updateLinkedPart(oldPartId, newPartNodeNumMap[0].first);
    }
    
    for (const auto &partId: newPartIds) {
        checkPartGrid(partId);
    }
    
    emit skeletonChanged();
}

void Document::addNode(float x, float y, float z, float radius, QUuid fromNodeId)
{
    createNode(QUuid::createUuid(), x, y, z, radius, fromNodeId);
}

void Document::addPartByPolygons(const QPolygonF &mainProfile, const QPolygonF &sideProfile, const QSizeF &canvasSize)
{
    if (mainProfile.empty() || sideProfile.empty())
        return;
    
    QThread *thread = new QThread;
    ContourToPartConverter *contourToPartConverter = new ContourToPartConverter(mainProfile, sideProfile, canvasSize);
    contourToPartConverter->moveToThread(thread);
    connect(thread, &QThread::started, contourToPartConverter, &ContourToPartConverter::process);
    connect(contourToPartConverter, &ContourToPartConverter::finished, this, [=]() {
        const auto &snapshot = contourToPartConverter->getSnapshot();
        if (!snapshot.nodes.empty()) {
            addFromSnapshot(snapshot, true);
            saveSnapshot();
        }
        delete contourToPartConverter;
    });
    connect(contourToPartConverter, &ContourToPartConverter::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void Document::addNodeWithId(QUuid nodeId, float x, float y, float z, float radius, QUuid fromNodeId)
{
    createNode(nodeId, x, y, z, radius, fromNodeId);
}

QUuid Document::createNode(QUuid nodeId, float x, float y, float z, float radius, QUuid fromNodeId)
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
    
    checkPartGrid(partId);
    emit skeletonChanged();
    
    return node.id;
}

void Document::addPose(QUuid poseId, QString name, std::vector<std::pair<std::map<QString, QString>, std::map<QString, std::map<QString, QString>>>> frames, QUuid turnaroundImageId, float yTranslationScale)
{
    QUuid newPoseId = poseId;
    auto &pose = poseMap[newPoseId];
    pose.id = newPoseId;
    
    pose.name = name;
    pose.frames = frames;
    pose.turnaroundImageId = turnaroundImageId;
    pose.yTranslationScale = yTranslationScale;
    pose.dirty = true;
    
    poseIdList.push_back(newPoseId);
    
    emit posesChanged();
    emit poseAdded(newPoseId);
    emit poseListChanged();
    emit optionsChanged();
}

void Document::addMotion(QUuid motionId, QString name, std::vector<MotionClip> clips)
{
    QUuid newMotionId = motionId;
    auto &motion = motionMap[newMotionId];
    motion.id = newMotionId;
    
    motion.name = name;
    motion.clips = clips;
    motion.dirty = true;
    
    motionIdList.push_back(newMotionId);
    
    emit motionsChanged();
    emit motionAdded(newMotionId);
    emit motionListChanged();
    emit optionsChanged();
}

void Document::removeMotion(QUuid motionId)
{
    auto findMotionResult = motionMap.find(motionId);
    if (findMotionResult == motionMap.end()) {
        qDebug() << "Remove a none exist motion:" << motionId;
        return;
    }
    motionIdList.erase(std::remove(motionIdList.begin(), motionIdList.end(), motionId), motionIdList.end());
    motionMap.erase(findMotionResult);
    emit motionsChanged();
    emit motionListChanged();
    emit motionRemoved(motionId);
    emit optionsChanged();
}

void Document::setMotionClips(QUuid motionId, std::vector<MotionClip> clips)
{
    auto findMotionResult = motionMap.find(motionId);
    if (findMotionResult == motionMap.end()) {
        qDebug() << "Find motion failed:" << motionId;
        return;
    }
    findMotionResult->second.clips = clips;
    findMotionResult->second.dirty = true;
    emit motionsChanged();
    emit motionClipsChanged(motionId);
    emit optionsChanged();
}

void Document::renameMotion(QUuid motionId, QString name)
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
    emit motionListChanged();
    emit optionsChanged();
}

void Document::removePose(QUuid poseId)
{
    auto findPoseResult = poseMap.find(poseId);
    if (findPoseResult == poseMap.end()) {
        qDebug() << "Remove a none exist pose:" << poseId;
        return;
    }
    poseIdList.erase(std::remove(poseIdList.begin(), poseIdList.end(), poseId), poseIdList.end());
    poseMap.erase(findPoseResult);
    emit posesChanged();
    emit poseListChanged();
    emit poseRemoved(poseId);
    emit optionsChanged();
}

void Document::setPoseFrames(QUuid poseId, std::vector<std::pair<std::map<QString, QString>, std::map<QString, std::map<QString, QString>>>> frames)
{
    auto findPoseResult = poseMap.find(poseId);
    if (findPoseResult == poseMap.end()) {
        qDebug() << "Find pose failed:" << poseId;
        return;
    }
    findPoseResult->second.frames = frames;
    findPoseResult->second.dirty = true;
    emit posesChanged();
    emit poseFramesChanged(poseId);
    emit optionsChanged();
}

void Document::setPoseTurnaroundImageId(QUuid poseId, QUuid imageId)
{
    auto findPoseResult = poseMap.find(poseId);
    if (findPoseResult == poseMap.end()) {
        qDebug() << "Find pose failed:" << poseId;
        return;
    }
    if (findPoseResult->second.turnaroundImageId == imageId)
        return;
    findPoseResult->second.turnaroundImageId = imageId;
    findPoseResult->second.dirty = true;
    emit poseTurnaroundImageIdChanged(poseId);
    emit optionsChanged();
}

void Document::setPoseYtranslationScale(QUuid poseId, float scale)
{
    auto findPoseResult = poseMap.find(poseId);
    if (findPoseResult == poseMap.end()) {
        qDebug() << "Find pose failed:" << poseId;
        return;
    }
    findPoseResult->second.yTranslationScale = scale;
    findPoseResult->second.dirty = true;
    emit poseYtranslationScaleChanged(poseId);
    emit optionsChanged();
}

void Document::renamePose(QUuid poseId, QString name)
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
    emit poseListChanged();
    emit optionsChanged();
}

bool Document::originSettled() const
{
    return !qFuzzyIsNull(getOriginX()) && !qFuzzyIsNull(getOriginY()) && !qFuzzyIsNull(getOriginZ());
}

void Document::addEdge(QUuid fromNodeId, QUuid toNodeId)
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
        updateLinkedPart(toPartId, fromNode->partId);
        removePart(toPartId);
    }
    
    checkPartGrid(fromNode->partId);
    
    emit skeletonChanged();
}

void Document::checkPartGrid(QUuid partId)
{
    SkeletonPart *part = (SkeletonPart *)findPart(partId);
    if (nullptr == part)
        return;
    bool isGrid = false;
    for (const auto &nodeId: part->nodeIds) {
        const SkeletonNode *node = findNode(nodeId);
        if (nullptr == node)
            continue;
        if (node->edgeIds.size() >= 3) {
            isGrid = true;
            break;
        }
    }
    if (part->gridded == isGrid)
        return;
    part->gridded = isGrid;
    part->dirty = true;
    emit partGridStateChanged(partId);
}

void Document::updateLinkedPart(QUuid oldPartId, QUuid newPartId)
{
    for (auto &partIt: partMap) {
        if (partIt.second.cutFaceLinkedId == oldPartId) {
            partIt.second.dirty = true;
            partIt.second.setCutFaceLinkedId(newPartId);
        }
    }
    std::set<QUuid> dirtyPartIds;
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

const Component *Document::findComponent(QUuid componentId) const
{
    if (componentId.isNull())
        return &rootComponent;
    auto it = componentMap.find(componentId);
    if (it == componentMap.end())
        return nullptr;
    return &it->second;
}

const Pose *Document::findPose(QUuid poseId) const
{
    auto it = poseMap.find(poseId);
    if (it == poseMap.end())
        return nullptr;
    return &it->second;
}

const Material *Document::findMaterial(QUuid materialId) const
{
    auto it = materialMap.find(materialId);
    if (it == materialMap.end())
        return nullptr;
    return &it->second;
}

const Motion *Document::findMotion(QUuid motionId) const
{
    auto it = motionMap.find(motionId);
    if (it == motionMap.end())
        return nullptr;
    return &it->second;
}

void Document::scaleNodeByAddRadius(QUuid nodeId, float amount)
{
    auto it = nodeMap.find(nodeId);
    if (it == nodeMap.end()) {
        qDebug() << "Find node failed:" << nodeId;
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

bool Document::isPartReadonly(QUuid partId) const
{
    const SkeletonPart *part = findPart(partId);
    if (nullptr == part) {
        qDebug() << "Find part failed:" << partId;
        return true;
    }
    return part->locked || !part->visible;
}

void Document::moveNodeBy(QUuid nodeId, float x, float y, float z)
{
    auto it = nodeMap.find(nodeId);
    if (it == nodeMap.end()) {
        qDebug() << "Find node failed:" << nodeId;
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

void Document::moveOriginBy(float x, float y, float z)
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

void Document::setNodeOrigin(QUuid nodeId, float x, float y, float z)
{
    auto it = nodeMap.find(nodeId);
    if (it == nodeMap.end()) {
        qDebug() << "Find node failed:" << nodeId;
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

void Document::setNodeRadius(QUuid nodeId, float radius)
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

void Document::switchNodeXZ(QUuid nodeId)
{
    auto it = nodeMap.find(nodeId);
    if (it == nodeMap.end()) {
        qDebug() << "Find node failed:" << nodeId;
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

void Document::setNodeBoneMark(QUuid nodeId, BoneMark mark)
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

void Document::setNodeCutRotation(QUuid nodeId, float cutRotation)
{
    auto node = nodeMap.find(nodeId);
    if (node == nodeMap.end()) {
        qDebug() << "Node not found:" << nodeId;
        return;
    }
    if (qFuzzyCompare(cutRotation, node->second.cutRotation))
        return;
    node->second.setCutRotation(cutRotation);
    auto part = partMap.find(node->second.partId);
    if (part != partMap.end())
        part->second.dirty = true;
    emit nodeCutRotationChanged(nodeId);
    emit skeletonChanged();
}

void Document::setNodeCutFace(QUuid nodeId, CutFace cutFace)
{
    auto node = nodeMap.find(nodeId);
    if (node == nodeMap.end()) {
        qDebug() << "Node not found:" << nodeId;
        return;
    }
    if (node->second.cutFace == cutFace)
        return;
    node->second.setCutFace(cutFace);
    auto part = partMap.find(node->second.partId);
    if (part != partMap.end())
        part->second.dirty = true;
    emit nodeCutFaceChanged(nodeId);
    emit skeletonChanged();
}

void Document::setNodeCutFaceLinkedId(QUuid nodeId, QUuid linkedId)
{
    auto node = nodeMap.find(nodeId);
    if (node == nodeMap.end()) {
        qDebug() << "Node not found:" << nodeId;
        return;
    }
    if (node->second.cutFace == CutFace::UserDefined &&
            node->second.cutFaceLinkedId == linkedId)
        return;
    node->second.setCutFaceLinkedId(linkedId);
    auto part = partMap.find(node->second.partId);
    if (part != partMap.end())
        part->second.dirty = true;
    emit nodeCutFaceChanged(nodeId);
    emit skeletonChanged();
}

void Document::clearNodeCutFaceSettings(QUuid nodeId)
{
    auto node = nodeMap.find(nodeId);
    if (node == nodeMap.end()) {
        qDebug() << "Node not found:" << nodeId;
        return;
    }
    if (!node->second.hasCutFaceSettings)
        return;
    node->second.clearCutFaceSettings();
    auto part = partMap.find(node->second.partId);
    if (part != partMap.end())
        part->second.dirty = true;
    emit nodeCutFaceChanged(nodeId);
    emit skeletonChanged();
}

void Document::updateTurnaround(const QImage &image)
{
    turnaround = image;
    turnaroundPngByteArray.clear();
    QBuffer pngBuffer(&turnaroundPngByteArray);
    pngBuffer.open(QIODevice::WriteOnly);
    turnaround.save(&pngBuffer, "PNG");
    emit turnaroundChanged();
}

void Document::setEditMode(SkeletonDocumentEditMode mode)
{
    if (editMode == mode)
        return;
    
    editMode = mode;
    if (editMode != SkeletonDocumentEditMode::Paint)
        m_paintMode = PaintMode::None;
    emit editModeChanged();
}

void Document::setPaintMode(PaintMode mode)
{
    if (m_paintMode == mode)
        return;
    
    m_paintMode = mode;
    emit paintModeChanged();
    
    doPickMouseTarget();
}

void Document::joinNodeAndNeiborsToGroup(std::vector<QUuid> *group, QUuid nodeId, std::set<QUuid> *visitMap, QUuid noUseEdgeId)
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

void Document::splitPartByNode(std::vector<std::vector<QUuid>> *groups, QUuid nodeId)
{
    const SkeletonNode *node = findNode(nodeId);
    std::set<QUuid> visitMap;
    visitMap.insert(nodeId);
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

void Document::splitPartByEdge(std::vector<std::vector<QUuid>> *groups, QUuid edgeId)
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

void Document::resetDirtyFlags()
{
    for (auto &part: partMap) {
        part.second.dirty = false;
    }
    for (auto &component: componentMap) {
        component.second.dirty = false;
    }
}

void Document::markAllDirty()
{
    for (auto &part: partMap) {
        part.second.dirty = true;
    }
}

void Document::toSnapshot(Snapshot *snapshot, const std::set<QUuid> &limitNodeIds,
    DocumentToSnapshotFor forWhat,
    const std::set<QUuid> &limitPoseIds,
    const std::set<QUuid> &limitMotionIds,
    const std::set<QUuid> &limitMaterialIds) const
{
    if (DocumentToSnapshotFor::Document == forWhat ||
            DocumentToSnapshotFor::Nodes == forWhat) {
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
            const Component *component = findComponent(part->componentId);
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
            if (partIt.second.zMirrored)
                part["zMirrored"] = partIt.second.zMirrored ? "true" : "false";
            if (partIt.second.base != PartBase::XYZ)
                part["base"] = PartBaseToString(partIt.second.base);
            part["rounded"] = partIt.second.rounded ? "true" : "false";
            part["chamfered"] = partIt.second.chamfered ? "true" : "false";
            if (PartTarget::Model != partIt.second.target)
                part["target"] = PartTargetToString(partIt.second.target);
            if (partIt.second.cutRotationAdjusted())
                part["cutRotation"] = QString::number(partIt.second.cutRotation);
            if (partIt.second.cutFaceAdjusted()) {
                if (CutFace::UserDefined == partIt.second.cutFace) {
                    if (!partIt.second.cutFaceLinkedId.isNull()) {
                        part["cutFace"] = partIt.second.cutFaceLinkedId.toString();
                    }
                } else {
                    part["cutFace"] = CutFaceToString(partIt.second.cutFace);
                }
            }
            part["dirty"] = partIt.second.dirty ? "true" : "false";
            if (partIt.second.hasColor)
                part["color"] = partIt.second.color.name(QColor::HexArgb);
            if (partIt.second.colorSolubilityAdjusted())
                part["colorSolubility"] = QString::number(partIt.second.colorSolubility);
            if (partIt.second.deformThicknessAdjusted())
                part["deformThickness"] = QString::number(partIt.second.deformThickness);
            if (partIt.second.deformWidthAdjusted())
                part["deformWidth"] = QString::number(partIt.second.deformWidth);
            if (!partIt.second.deformMapImageId.isNull())
                part["deformMapImageId"] = partIt.second.deformMapImageId.toString();
            if (partIt.second.deformMapScaleAdjusted())
                part["deformMapScale"] = QString::number(partIt.second.deformMapScale);
            if (partIt.second.hollowThicknessAdjusted())
                part["hollowThickness"] = QString::number(partIt.second.hollowThickness);
            if (!partIt.second.name.isEmpty())
                part["name"] = partIt.second.name;
            if (partIt.second.materialAdjusted())
                part["materialId"] = partIt.second.materialId.toString();
            if (partIt.second.countershaded)
                part["countershaded"] = "true";
            if (partIt.second.gridded)
                part["gridded"] = "true";
            snapshot->parts[part["id"]] = part;
        }
        for (const auto &nodeIt: nodeMap) {
            if (!limitNodeIds.empty() && limitNodeIds.find(nodeIt.first) == limitNodeIds.end())
                continue;
            std::map<QString, QString> node;
            node["id"] = nodeIt.second.id.toString();
            node["radius"] = QString::number(nodeIt.second.radius);
            node["x"] = QString::number(nodeIt.second.getX());
            node["y"] = QString::number(nodeIt.second.getY());
            node["z"] = QString::number(nodeIt.second.getZ());
            node["partId"] = nodeIt.second.partId.toString();
            if (nodeIt.second.boneMark != BoneMark::None)
                node["boneMark"] = BoneMarkToString(nodeIt.second.boneMark);
            if (nodeIt.second.hasCutFaceSettings) {
                node["cutRotation"] = QString::number(nodeIt.second.cutRotation);
                if (CutFace::UserDefined == nodeIt.second.cutFace) {
                    if (!nodeIt.second.cutFaceLinkedId.isNull()) {
                        node["cutFace"] = nodeIt.second.cutFaceLinkedId.toString();
                    }
                } else {
                    node["cutFace"] = CutFaceToString(nodeIt.second.cutFace);
                }
            }
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
            component["combineMode"] = CombineModeToString(componentIt.second.combineMode);
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
    if (DocumentToSnapshotFor::Document == forWhat ||
            DocumentToSnapshotFor::Materials == forWhat) {
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
                if (!qFuzzyCompare((float)layer.tileScale, (float)1.0))
                    layerAttributes["tileScale"] = QString::number(layer.tileScale);
                layers.push_back({layerAttributes, maps});
            }
            snapshot->materials.push_back(std::make_pair(material, layers));
        }
    }
    if (DocumentToSnapshotFor::Document == forWhat ||
            DocumentToSnapshotFor::Poses == forWhat) {
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
            if (!poseIt.second.turnaroundImageId.isNull())
                pose["canvasImageId"] = poseIt.second.turnaroundImageId.toString();
            if (poseIt.second.yTranslationScaleAdjusted())
                pose["yTranslationScale"] = QString::number(poseIt.second.yTranslationScale);
            snapshot->poses.push_back(std::make_pair(pose, poseIt.second.frames));
        }
    }
    if (DocumentToSnapshotFor::Document == forWhat ||
            DocumentToSnapshotFor::Motions == forWhat) {
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
            std::vector<std::map<QString, QString>> clips;
            for (const auto &clip: motionIt.second.clips) {
                std::map<QString, QString> attributes;
                attributes["duration"] = QString::number(clip.duration);
                attributes["linkDataType"] = clip.linkDataType();
                attributes["linkData"] = clip.linkData();
                clips.push_back(attributes);
            }
            snapshot->motions.push_back(std::make_pair(motion, clips));
        }
    }
    if (DocumentToSnapshotFor::Document == forWhat) {
        std::map<QString, QString> canvas;
        canvas["originX"] = QString::number(getOriginX());
        canvas["originY"] = QString::number(getOriginY());
        canvas["originZ"] = QString::number(getOriginZ());
        canvas["rigType"] = RigTypeToString(rigType);
        snapshot->canvas = canvas;
    }
}

void Document::createSinglePartFromEdges(const std::vector<QVector3D> &nodes,
        const std::vector<std::pair<size_t, size_t>> &edges)
{
    std::vector<QUuid> newAddedNodeIds;
    std::vector<QUuid> newAddedEdgeIds;
    std::vector<QUuid> newAddedPartIds;
    std::map<size_t, QUuid> nodeIndexToIdMap;
    
    QUuid partId = QUuid::createUuid();
    
    Component component(QUuid(), partId.toString(), "partId");
    component.combineMode = CombineMode::Normal;
    componentMap[component.id] = component;
    rootComponent.addChild(component.id);
    
    auto &newPart = partMap[partId];
    newPart.id = partId;
    newPart.componentId = component.id;
    newAddedPartIds.push_back(newPart.id);
    
    auto nodeToId = [&](size_t nodeIndex) {
        auto findId = nodeIndexToIdMap.find(nodeIndex);
        if (findId != nodeIndexToIdMap.end())
            return findId->second;
        const auto &position = nodes[nodeIndex];
        SkeletonNode newNode;
        newNode.partId = newPart.id;
        newNode.setX(getOriginX() + position.x());
        newNode.setY(getOriginY() - position.y());
        newNode.setZ(getOriginZ() - position.z());
        newNode.setRadius(0);
        nodeMap[newNode.id] = newNode;
        newPart.nodeIds.push_back(newNode.id);
        newAddedNodeIds.push_back(newNode.id);
        nodeIndexToIdMap.insert({nodeIndex, newNode.id});
        return newNode.id;
    };
    
    for (const auto &edge: edges) {
        QUuid firstNodeId = nodeToId(edge.first);
        QUuid secondNodeId = nodeToId(edge.second);
        
        SkeletonEdge newEdge;
        newEdge.nodeIds.push_back(firstNodeId);
        newEdge.nodeIds.push_back(secondNodeId);
        newEdge.partId = newPart.id;
        
        nodeMap[firstNodeId].edgeIds.push_back(newEdge.id);
        nodeMap[secondNodeId].edgeIds.push_back(newEdge.id);
        
        newAddedEdgeIds.push_back(newEdge.id);
        
        edgeMap[newEdge.id] = newEdge;
    }
    
    for (const auto &nodeIt: newAddedNodeIds) {
        qDebug() << "new node:" << nodeIt;
        emit nodeAdded(nodeIt);
    }
    for (const auto &edgeIt: newAddedEdgeIds) {
        qDebug() << "new edge:" << edgeIt;
        emit edgeAdded(edgeIt);
    }
    for (const auto &partIt: newAddedPartIds) {
        qDebug() << "new part:" << partIt;
        emit partAdded(partIt);
    }
    
    for (const auto &partIt : newAddedPartIds) {
        checkPartGrid(partIt);
        emit partVisibleStateChanged(partIt);
    }
    
    emit uncheckAll();
    for (const auto &nodeIt: newAddedNodeIds) {
        emit checkNode(nodeIt);
    }
    for (const auto &edgeIt: newAddedEdgeIds) {
        emit checkEdge(edgeIt);
    }
    
    emit componentChildrenChanged(QUuid());
    emit skeletonChanged();
}

void Document::createFromNodesAndEdges(const std::vector<QVector3D> &nodes,
        const std::vector<std::pair<size_t, size_t>> &edges)
{
    std::map<size_t, std::vector<size_t>> edgeLinks;
    for (const auto &it: edges) {
        edgeLinks[it.first].push_back(it.second);
        edgeLinks[it.second].push_back(it.first);
    }
    if (edgeLinks.empty())
        return;
    std::vector<std::set<size_t>> islands;
    std::set<size_t> visited;
    for (size_t i = 0; i < nodes.size(); ++i) {
        std::set<size_t> island;
        std::queue<size_t> waitVertices;
        waitVertices.push(i);
        while (!waitVertices.empty()) {
            size_t vertexIndex = waitVertices.front();
            waitVertices.pop();
            if (visited.find(vertexIndex) == visited.end()) {
                visited.insert(vertexIndex);
                island.insert(vertexIndex);
            }
            auto findLink = edgeLinks.find(vertexIndex);
            if (findLink == edgeLinks.end()) {
                continue;
            }
            for (const auto &it: findLink->second) {
                if (visited.find(it) == visited.end())
                    waitVertices.push(it);
            }
        }
        if (!island.empty())
            islands.push_back(island);
    }
    
    std::map<size_t, size_t> vertexIslandMap;
    for (size_t islandIndex = 0; islandIndex < islands.size(); ++islandIndex) {
        const auto &island = islands[islandIndex];
        for (const auto &it: island)
            vertexIslandMap.insert({it, islandIndex});
    }
    
    std::vector<std::vector<std::pair<size_t, size_t>>> edgesGroupByIsland(islands.size());
    for (const auto &it: edges) {
        auto findFirstVertex = vertexIslandMap.find(it.first);
        if (findFirstVertex != vertexIslandMap.end()) {
            edgesGroupByIsland[findFirstVertex->second].push_back(it);
            continue;
        }
        auto findSecondVertex = vertexIslandMap.find(it.second);
        if (findSecondVertex != vertexIslandMap.end()) {
            edgesGroupByIsland[findSecondVertex->second].push_back(it);
            continue;
        }
    }
    
    for (size_t islandIndex = 0; islandIndex < islands.size(); ++islandIndex) {
        const auto &islandEdges = edgesGroupByIsland[islandIndex];
        createSinglePartFromEdges(nodes, islandEdges);
    }
}

void Document::addFromSnapshot(const Snapshot &snapshot, bool fromPaste)
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
            setOriginX(originXit->second.toFloat());
            setOriginY(originYit->second.toFloat());
            setOriginZ(originZit->second.toFloat());
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
            MaterialLayer layer;
            auto findTileScale = layerIt.first.find("tileScale");
            if (findTileScale != layerIt.first.end())
                layer.tileScale = findTileScale->second.toFloat();
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
                MaterialMap materialMap;
                materialMap.imageId = imageId;
                materialMap.forWhat = textureType;
                layer.maps.push_back(materialMap);
            }
            newMaterial.layers.push_back(layer);
        }
        materialIdList.push_back(newMaterialId);
        emit materialAdded(newMaterialId);
    }
    std::map<QUuid, QUuid> cutFaceLinkedIdModifyMap;
    for (const auto &partKv: snapshot.parts) {
        const auto newUuid = QUuid::createUuid();
        SkeletonPart &part = partMap[newUuid];
        part.id = newUuid;
        oldNewIdMap[QUuid(partKv.first)] = part.id;
        part.name = valueOfKeyInMapOrEmpty(partKv.second, "name");
        const auto &visibleIt = partKv.second.find("visible");
        if (visibleIt != partKv.second.end()) {
            part.visible = isTrueValueString(visibleIt->second);
        } else {
            part.visible = true;
        }
        part.locked = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "locked"));
        part.subdived = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "subdived"));
        part.disabled = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "disabled"));
        part.xMirrored = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "xMirrored"));
        part.zMirrored = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "zMirrored"));
        part.base = PartBaseFromString(valueOfKeyInMapOrEmpty(partKv.second, "base").toUtf8().constData());
        part.rounded = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "rounded"));
        part.chamfered = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "chamfered"));
        part.target = PartTargetFromString(valueOfKeyInMapOrEmpty(partKv.second, "target").toUtf8().constData());
        const auto &cutRotationIt = partKv.second.find("cutRotation");
        if (cutRotationIt != partKv.second.end())
            part.setCutRotation(cutRotationIt->second.toFloat());
        const auto &cutFaceIt = partKv.second.find("cutFace");
        if (cutFaceIt != partKv.second.end()) {
            QUuid cutFaceLinkedId = QUuid(cutFaceIt->second);
            if (cutFaceLinkedId.isNull()) {
                part.setCutFace(CutFaceFromString(cutFaceIt->second.toUtf8().constData()));
            } else {
                part.setCutFaceLinkedId(cutFaceLinkedId);
                cutFaceLinkedIdModifyMap.insert({part.id, cutFaceLinkedId});
            }
        }
        if (isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "inverse")))
            inversePartIds.insert(part.id);
        const auto &colorIt = partKv.second.find("color");
        if (colorIt != partKv.second.end()) {
            part.color = QColor(colorIt->second);
            part.hasColor = true;
        }
        const auto &colorSolubilityIt = partKv.second.find("colorSolubility");
        if (colorSolubilityIt != partKv.second.end())
            part.colorSolubility = colorSolubilityIt->second.toFloat();
        const auto &deformThicknessIt = partKv.second.find("deformThickness");
        if (deformThicknessIt != partKv.second.end())
            part.setDeformThickness(deformThicknessIt->second.toFloat());
        const auto &deformWidthIt = partKv.second.find("deformWidth");
        if (deformWidthIt != partKv.second.end())
            part.setDeformWidth(deformWidthIt->second.toFloat());
        const auto &deformMapImageIdIt = partKv.second.find("deformMapImageId");
        if (deformMapImageIdIt != partKv.second.end())
            part.deformMapImageId = QUuid(deformMapImageIdIt->second);
        const auto &deformMapScaleIt = partKv.second.find("deformMapScale");
        if (deformMapScaleIt != partKv.second.end())
            part.deformMapScale = deformMapScaleIt->second.toFloat();
        const auto &hollowThicknessIt = partKv.second.find("hollowThickness");
        if (hollowThicknessIt != partKv.second.end())
            part.hollowThickness = hollowThicknessIt->second.toFloat();
        const auto &materialIdIt = partKv.second.find("materialId");
        if (materialIdIt != partKv.second.end())
            part.materialId = oldNewIdMap[QUuid(materialIdIt->second)];
        part.countershaded = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "countershaded"));
        part.gridded = isTrueValueString(valueOfKeyInMapOrEmpty(partKv.second, "gridded"));;
        newAddedPartIds.insert(part.id);
    }
    for (const auto &it: cutFaceLinkedIdModifyMap) {
        SkeletonPart &part = partMap[it.first];
        auto findNewLinkedId = oldNewIdMap.find(it.second);
        if (findNewLinkedId == oldNewIdMap.end()) {
            if (partMap.find(it.second) == partMap.end()) {
                part.setCutFaceLinkedId(QUuid());
            }
        } else {
            part.setCutFaceLinkedId(findNewLinkedId->second);
        }
    }
    for (const auto &nodeKv: snapshot.nodes) {
        if (nodeKv.second.find("radius") == nodeKv.second.end() ||
                nodeKv.second.find("x") == nodeKv.second.end() ||
                nodeKv.second.find("y") == nodeKv.second.end() ||
                nodeKv.second.find("z") == nodeKv.second.end() ||
                nodeKv.second.find("partId") == nodeKv.second.end())
            continue;
        QUuid oldNodeId = QUuid(nodeKv.first);
        SkeletonNode node(nodeMap.find(oldNodeId) == nodeMap.end() ? oldNodeId : QUuid::createUuid());
        oldNewIdMap[oldNodeId] = node.id;
        node.name = valueOfKeyInMapOrEmpty(nodeKv.second, "name");
        node.radius = valueOfKeyInMapOrEmpty(nodeKv.second, "radius").toFloat();
        node.setX(valueOfKeyInMapOrEmpty(nodeKv.second, "x").toFloat());
        node.setY(valueOfKeyInMapOrEmpty(nodeKv.second, "y").toFloat());
        node.setZ(valueOfKeyInMapOrEmpty(nodeKv.second, "z").toFloat());
        node.partId = oldNewIdMap[QUuid(valueOfKeyInMapOrEmpty(nodeKv.second, "partId"))];
        node.boneMark = BoneMarkFromString(valueOfKeyInMapOrEmpty(nodeKv.second, "boneMark").toUtf8().constData());
        const auto &cutRotationIt = nodeKv.second.find("cutRotation");
        if (cutRotationIt != nodeKv.second.end())
            node.setCutRotation(cutRotationIt->second.toFloat());
        const auto &cutFaceIt = nodeKv.second.find("cutFace");
        if (cutFaceIt != nodeKv.second.end()) {
            QUuid cutFaceLinkedId = QUuid(cutFaceIt->second);
            if (cutFaceLinkedId.isNull()) {
                node.setCutFace(CutFaceFromString(cutFaceIt->second.toUtf8().constData()));
            } else {
                node.setCutFaceLinkedId(cutFaceLinkedId);
                auto findNewLinkedId = oldNewIdMap.find(cutFaceLinkedId);
                if (findNewLinkedId == oldNewIdMap.end()) {
                    if (partMap.find(cutFaceLinkedId) == partMap.end()) {
                        node.setCutFaceLinkedId(QUuid());
                    }
                } else {
                    node.setCutFaceLinkedId(findNewLinkedId->second);
                }
            }
        }
        nodeMap[node.id] = node;
        newAddedNodeIds.insert(node.id);
    }
    for (const auto &edgeKv: snapshot.edges) {
        if (edgeKv.second.find("from") == edgeKv.second.end() ||
                edgeKv.second.find("to") == edgeKv.second.end() ||
                edgeKv.second.find("partId") == edgeKv.second.end())
            continue;
        QUuid oldEdgeId = QUuid(edgeKv.first);
        SkeletonEdge edge(edgeMap.find(oldEdgeId) == edgeMap.end() ? oldEdgeId : QUuid::createUuid());
        oldNewIdMap[oldEdgeId] = edge.id;
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
        Component component(QUuid(), linkData, linkDataType);
        oldNewIdMap[QUuid(componentKv.first)] = component.id;
        component.name = valueOfKeyInMapOrEmpty(componentKv.second, "name");
        component.expanded = isTrueValueString(valueOfKeyInMapOrEmpty(componentKv.second, "expanded"));
        component.combineMode = CombineModeFromString(valueOfKeyInMapOrEmpty(componentKv.second, "combineMode").toUtf8().constData());
        if (component.combineMode == CombineMode::Normal) {
            if (isTrueValueString(valueOfKeyInMapOrEmpty(componentKv.second, "inverse")))
                component.combineMode = CombineMode::Inversion;
        }
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
                component.combineMode = CombineMode::Inversion;
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
        auto findCanvasImageId = poseAttributes.find("canvasImageId");
        if (findCanvasImageId != poseAttributes.end())
            newPose.turnaroundImageId = QUuid(findCanvasImageId->second);
        auto findYtranslationScale = poseAttributes.find("yTranslationScale");
        if (findYtranslationScale != poseAttributes.end())
            newPose.yTranslationScale = findYtranslationScale->second.toFloat();
        newPose.frames = poseIt.second;
        oldNewIdMap[QUuid(valueOfKeyInMapOrEmpty(poseAttributes, "id"))] = newPoseId;
        poseIdList.push_back(newPoseId);
        emit poseAdded(newPoseId);
    }
    for (const auto &motionIt: snapshot.motions) {
        QUuid newMotionId = QUuid::createUuid();
        auto &newMotion = motionMap[newMotionId];
        newMotion.id = newMotionId;
        const auto &motionAttributes = motionIt.first;
        newMotion.name = valueOfKeyInMapOrEmpty(motionAttributes, "name");
        for (const auto &attributes: motionIt.second) {
            auto linkData = valueOfKeyInMapOrEmpty(attributes, "linkData");
            QUuid testId = QUuid(linkData);
            if (!testId.isNull()) {
                auto findInOldNewIdMapResult = oldNewIdMap.find(testId);
                if (findInOldNewIdMapResult != oldNewIdMap.end()) {
                    linkData = findInOldNewIdMapResult->second.toString();
                }
            }
            MotionClip clip(linkData,
                valueOfKeyInMapOrEmpty(attributes, "linkDataType"));
            clip.duration = valueOfKeyInMapOrEmpty(attributes, "duration").toFloat();
            newMotion.clips.push_back(clip);
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
        checkPartGrid(partIt);
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

void Document::silentReset()
{
    setOriginX(0.0);
    setOriginY(0.0);
    setOriginZ(0.0);
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
    rootComponent = Component();
    removeRigResults();
}

void Document::silentResetScript()
{
    m_script.clear();
    m_mergedVariables.clear();
    m_cachedVariables.clear();
    m_scriptError.clear();
    m_scriptConsoleLog.clear();
}

void Document::reset()
{
    silentReset();
    emit cleanup();
    emit skeletonChanged();
}

void Document::resetScript()
{
    silentResetScript();
    emit cleanupScript();
    emit scriptChanged();
    emit scriptErrorChanged();
    emit scriptConsoleLogChanged();
    emit mergedVaraiblesChanged();
}

void Document::fromSnapshot(const Snapshot &snapshot)
{
    reset();
    addFromSnapshot(snapshot, false);
    emit uncheckAll();
}

MeshLoader *Document::takeResultMesh()
{
    if (nullptr == m_resultMesh)
        return nullptr;
    MeshLoader *resultMesh = new MeshLoader(*m_resultMesh);
    return resultMesh;
}

bool Document::isMeshGenerationSucceed()
{
    return m_isMeshGenerationSucceed;
}

MeshLoader *Document::takeResultTextureMesh()
{
    if (nullptr == m_resultTextureMesh)
        return nullptr;
    MeshLoader *resultTextureMesh = new MeshLoader(*m_resultTextureMesh);
    return resultTextureMesh;
}

MeshLoader *Document::takeResultRigWeightMesh()
{
    if (nullptr == m_resultRigWeightMesh)
        return nullptr;
    MeshLoader *resultMesh = new MeshLoader(*m_resultRigWeightMesh);
    return resultMesh;
}

void Document::meshReady()
{
    MeshLoader *resultMesh = m_meshGenerator->takeResultMesh();
    Outcome *outcome = m_meshGenerator->takeOutcome();
    bool isSucceed = m_meshGenerator->isSucceed();
    
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
    
    delete m_resultMeshCutFaceTransforms;
    m_resultMeshCutFaceTransforms = m_meshGenerator->takeCutFaceTransforms();
    
    delete m_resultMeshNodesCutFaces;
    m_resultMeshNodesCutFaces = m_meshGenerator->takeNodesCutFaces();
    
    //addToolToMesh(m_resultMesh);
    
    m_isMeshGenerationSucceed = isSucceed;
    
    delete m_currentOutcome;
    m_currentOutcome = outcome;
    
    if (nullptr == m_resultMesh) {
        qDebug() << "Result mesh is null";
    }
    
    delete m_meshGenerator;
    m_meshGenerator = nullptr;
    
    qDebug() << "Mesh generation done";
    
    m_isPostProcessResultObsolete = true;
    m_isRigObsolete = true;
    
    emit resultMeshChanged();
    
    if (m_isResultMeshObsolete) {
        generateMesh();
    }
}

//void Document::addToolToMesh(MeshLoader *mesh)
//{
//    if (nullptr == mesh)
//        return;
//
//    if (nullptr == m_resultMeshCutFaceTransforms ||
//            nullptr == m_resultMeshNodesCutFaces ||
//            m_resultMeshCutFaceTransforms->empty() ||
//            m_resultMeshNodesCutFaces->empty())
//        return;
//
//    ToolMesh toolMesh;
//    for (const auto &transformIt: *m_resultMeshCutFaceTransforms) {
//        const auto &nodeId = transformIt.first;
//        const auto &transform = transformIt.second;
//        qDebug() << "nodeId:" << nodeId;
//        for (const auto &cutFaceIt: (*m_resultMeshNodesCutFaces)[nodeId]) {
//            const auto &cutFaceId = cutFaceIt.first;
//            const auto &cutFace2d = cutFaceIt.second;
//            QVector3D position = transform.translation + transform.rotation * (transform.uFactor * cutFace2d.x() + transform.vFactor * cutFace2d.y());
//            qDebug() << "cutFaceId:" << cutFaceId;
//            toolMesh.addNode(nodeId.toString() + cutFaceId, position);
//        }
//    }
//    toolMesh.generate();
//    int shaderVertexCount = 0;
//    ShaderVertex *shaderVertices = toolMesh.takeShaderVertices(&shaderVertexCount);
//    mesh->updateTool(shaderVertices, shaderVertexCount);
//}

bool Document::isPostProcessResultObsolete() const
{
    return m_isPostProcessResultObsolete;
}

void Document::batchChangeBegin()
{
    m_batchChangeRefCount++;
}

void Document::batchChangeEnd()
{
    m_batchChangeRefCount--;
    if (0 == m_batchChangeRefCount) {
        if (m_isResultMeshObsolete) {
            generateMesh();
        }
    }
}

void Document::regenerateMesh()
{
    markAllDirty();
    generateMesh();
}

void Document::toggleSmoothNormal()
{
    m_smoothNormal = !m_smoothNormal;
    regenerateMesh();
}

void Document::enableWeld(bool enabled)
{
    weldEnabled = enabled;
    regenerateMesh();
}

void Document::generateMesh()
{
    if (nullptr != m_meshGenerator || m_batchChangeRefCount > 0) {
        m_isResultMeshObsolete = true;
        return;
    }
    
    qDebug() << "Mesh generating..";
    
    settleOrigin();
    
    m_isResultMeshObsolete = false;
    
    QThread *thread = new QThread;
    
    Snapshot *snapshot = new Snapshot;
    toSnapshot(snapshot);
    resetDirtyFlags();
    m_meshGenerator = new MeshGenerator(snapshot);
    m_meshGenerator->setId(m_nextMeshGenerationId++);
    m_meshGenerator->setDefaultPartColor(Preferences::instance().partColor());
    m_meshGenerator->setGeneratedCacheContext(&m_generatedCacheContext);
    if (!m_smoothNormal) {
        m_meshGenerator->setSmoothShadingThresholdAngleDegrees(0);
    }
    m_meshGenerator->moveToThread(thread);
    connect(thread, &QThread::started, m_meshGenerator, &MeshGenerator::process);
    connect(m_meshGenerator, &MeshGenerator::finished, this, &Document::meshReady);
    connect(m_meshGenerator, &MeshGenerator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    emit meshGenerating();
    thread->start();
}

void Document::generateTexture()
{
    if (nullptr != m_textureGenerator) {
        m_isTextureObsolete = true;
        return;
    }
    
    qDebug() << "Texture guide generating..";
    
    m_isTextureObsolete = false;
    
    Snapshot *snapshot = new Snapshot;
    toSnapshot(snapshot);
    
    QThread *thread = new QThread;
    m_textureGenerator = new TextureGenerator(*m_postProcessedOutcome, snapshot);
    m_textureGenerator->moveToThread(thread);
    connect(thread, &QThread::started, m_textureGenerator, &TextureGenerator::process);
    connect(m_textureGenerator, &TextureGenerator::finished, this, &Document::textureReady);
    connect(m_textureGenerator, &TextureGenerator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    emit textureGenerating();
    thread->start();
}

void Document::textureReady()
{
    delete textureGuideImage;
    textureGuideImage = m_textureGenerator->takeResultTextureGuideImage();
    
    delete textureImage;
    textureImage = m_textureGenerator->takeResultTextureImage();
    
    delete textureBorderImage;
    textureBorderImage = m_textureGenerator->takeResultTextureBorderImage();
    
    delete textureColorImage;
    textureColorImage = m_textureGenerator->takeResultTextureColorImage();
    
    delete textureNormalImage;
    textureNormalImage = m_textureGenerator->takeResultTextureNormalImage();
    
    delete textureMetalnessRoughnessAmbientOcclusionImage;
    textureMetalnessRoughnessAmbientOcclusionImage = m_textureGenerator->takeResultTextureMetalnessRoughnessAmbientOcclusionImage();
    
    delete textureMetalnessImage;
    textureMetalnessImage = m_textureGenerator->takeResultTextureMetalnessImage();
    
    delete textureRoughnessImage;
    textureRoughnessImage = m_textureGenerator->takeResultTextureRoughnessImage();
    
    delete textureAmbientOcclusionImage;
    textureAmbientOcclusionImage = m_textureGenerator->takeResultTextureAmbientOcclusionImage();
    
    delete m_resultTextureMesh;
    m_resultTextureMesh = m_textureGenerator->takeResultMesh();
    
    //addToolToMesh(m_resultTextureMesh);
    
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

void Document::postProcess()
{
    if (nullptr != m_postProcessor) {
        m_isPostProcessResultObsolete = true;
        return;
    }

    m_isPostProcessResultObsolete = false;

    if (!m_currentOutcome) {
        qDebug() << "MeshLoader is null";
        return;
    }

    qDebug() << "Post processing..";

    QThread *thread = new QThread;
    m_postProcessor = new MeshResultPostProcessor(*m_currentOutcome);
    m_postProcessor->moveToThread(thread);
    connect(thread, &QThread::started, m_postProcessor, &MeshResultPostProcessor::process);
    connect(m_postProcessor, &MeshResultPostProcessor::finished, this, &Document::postProcessedMeshResultReady);
    connect(m_postProcessor, &MeshResultPostProcessor::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    emit postProcessing();
    thread->start();
}

void Document::postProcessedMeshResultReady()
{
    delete m_postProcessedOutcome;
    m_postProcessedOutcome = m_postProcessor->takePostProcessedOutcome();

    delete m_postProcessor;
    m_postProcessor = nullptr;

    qDebug() << "Post process done";

    emit postProcessedResultChanged();

    if (m_isPostProcessResultObsolete) {
        postProcess();
    }
}

void Document::pickMouseTarget(const QVector3D &nearPosition, const QVector3D &farPosition)
{
    m_mouseRayNear = nearPosition;
    m_mouseRayFar = farPosition;
    
    doPickMouseTarget();
}

void Document::doPickMouseTarget()
{
    if (nullptr != m_mousePicker) {
        m_isMouseTargetResultObsolete = true;
        return;
    }
    
    m_isMouseTargetResultObsolete = false;
    
    if (!m_currentOutcome) {
        qDebug() << "MeshLoader is null";
        return;
    }
    
    //qDebug() << "Mouse picking..";

    QThread *thread = new QThread;
    m_mousePicker = new MousePicker(*m_currentOutcome, m_mouseRayNear, m_mouseRayFar);
    
    std::map<QUuid, QUuid> paintImages;
    for (const auto &it: partMap) {
        if (!it.second.deformMapImageId.isNull()) {
            paintImages[it.first] = it.second.deformMapImageId;
        }
    }
    if (SkeletonDocumentEditMode::Paint == editMode) {
        m_mousePicker->setPaintImages(paintImages);
        m_mousePicker->setPaintMode(m_paintMode);
        m_mousePicker->setRadius(m_mousePickRadius);
        m_mousePicker->setMaskNodeIds(m_mousePickMaskNodeIds);
    }
    
    m_mousePicker->moveToThread(thread);
    connect(thread, &QThread::started, m_mousePicker, &MousePicker::process);
    connect(m_mousePicker, &MousePicker::finished, this, &Document::mouseTargetReady);
    connect(m_mousePicker, &MousePicker::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void Document::mouseTargetReady()
{
    m_mouseTargetPosition = m_mousePicker->targetPosition();
    const auto &changedPartIds = m_mousePicker->changedPartIds();
    for (const auto &it: m_mousePicker->resultPaintImages()) {
        const auto &partId = it.first;
        if (changedPartIds.find(partId) == changedPartIds.end())
            continue;
        const auto &imageId = it.second;
        m_intermediatePaintImageIds.insert(imageId);
        setPartDeformMapImageId(partId, imageId);
    }
    
    delete m_mousePicker;
    m_mousePicker = nullptr;
    
    if (!m_isMouseTargetResultObsolete && m_saveNextPaintSnapshot) {
        m_saveNextPaintSnapshot = false;
        stopPaint();
    }
    
    emit mouseTargetChanged();

    //qDebug() << "Mouse pick done";

    if (m_isMouseTargetResultObsolete) {
        pickMouseTarget(m_mouseRayNear, m_mouseRayFar);
    }
}

const QVector3D &Document::mouseTargetPosition() const
{
    return m_mouseTargetPosition;
}

float Document::mousePickRadius() const
{
    return m_mousePickRadius;
}

void Document::setMousePickRadius(float radius)
{
    m_mousePickRadius = radius;
    emit mousePickRadiusChanged();
}

const Outcome &Document::currentPostProcessedOutcome() const
{
    return *m_postProcessedOutcome;
}

void Document::setPartLockState(QUuid partId, bool locked)
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

void Document::setPartVisibleState(QUuid partId, bool visible)
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

void Document::setComponentCombineMode(QUuid componentId, CombineMode combineMode)
{
    auto component = componentMap.find(componentId);
    if (component == componentMap.end()) {
        qDebug() << "Component not found:" << componentId;
        return;
    }
    if (component->second.combineMode == combineMode)
        return;
    component->second.combineMode = combineMode;
    component->second.dirty = true;
    emit componentCombineModeChanged(componentId);
    emit skeletonChanged();
}

void Document::setComponentSmoothAll(QUuid componentId, float toSmoothAll)
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

void Document::setComponentSmoothSeam(QUuid componentId, float toSmoothSeam)
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

void Document::setPartSubdivState(QUuid partId, bool subdived)
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

void Document::setPartDisableState(QUuid partId, bool disabled)
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

const Component *Document::findComponentParent(QUuid componentId) const
{
    auto component = componentMap.find(componentId);
    if (component == componentMap.end()) {
        qDebug() << "Component not found:" << componentId;
        return nullptr;
    }
    
    if (component->second.parentId.isNull())
        return &rootComponent;
    
    return (Component *)findComponent(component->second.parentId);
}

QUuid Document::findComponentParentId(QUuid componentId) const
{
    auto component = componentMap.find(componentId);
    if (component == componentMap.end()) {
        qDebug() << "Component not found:" << componentId;
        return QUuid();
    }
    
    return component->second.parentId;
}

void Document::moveComponentUp(QUuid componentId)
{
    Component *parent = (Component *)findComponentParent(componentId);
    if (nullptr == parent)
        return;
    
    QUuid parentId = findComponentParentId(componentId);
    
    parent->moveChildUp(componentId);
    parent->dirty = true;
    emit componentChildrenChanged(parentId);
    emit skeletonChanged();
}

void Document::moveComponentDown(QUuid componentId)
{
    Component *parent = (Component *)findComponentParent(componentId);
    if (nullptr == parent)
        return;
    
    QUuid parentId = findComponentParentId(componentId);
    
    parent->moveChildDown(componentId);
    parent->dirty = true;
    emit componentChildrenChanged(parentId);
    emit skeletonChanged();
}

void Document::moveComponentToTop(QUuid componentId)
{
    Component *parent = (Component *)findComponentParent(componentId);
    if (nullptr == parent)
        return;
    
    QUuid parentId = findComponentParentId(componentId);
    
    parent->moveChildToTop(componentId);
    parent->dirty = true;
    emit componentChildrenChanged(parentId);
    emit skeletonChanged();
}

void Document::moveComponentToBottom(QUuid componentId)
{
    Component *parent = (Component *)findComponentParent(componentId);
    if (nullptr == parent)
        return;
    
    QUuid parentId = findComponentParentId(componentId);
    
    parent->moveChildToBottom(componentId);
    parent->dirty = true;
    emit componentChildrenChanged(parentId);
    emit skeletonChanged();
}

void Document::renameComponent(QUuid componentId, QString name)
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

void Document::setComponentExpandState(QUuid componentId, bool expanded)
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

void Document::createNewComponentAndMoveThisIn(QUuid componentId)
{
    auto component = componentMap.find(componentId);
    if (component == componentMap.end()) {
        qDebug() << "Component not found:" << componentId;
        return;
    }
    
    Component *oldParent = (Component *)findComponentParent(componentId);
    
    Component newParent(QUuid::createUuid());
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

void Document::createNewChildComponent(QUuid parentComponentId)
{
    Component *parentComponent = (Component *)findComponent(parentComponentId);
    if (!parentComponent->linkToPartId.isNull()) {
        parentComponentId = parentComponent->parentId;
        parentComponent = (Component *)findComponent(parentComponentId);
    }
    
    Component newComponent(QUuid::createUuid());
    newComponent.name = tr("Group") + " " + QString::number(componentMap.size() - partMap.size() + 1);
    
    parentComponent->addChild(newComponent.id);
    newComponent.parentId = parentComponentId;
    
    componentMap[newComponent.id] = newComponent;
    
    emit componentChildrenChanged(parentComponentId);
    emit componentAdded(newComponent.id);
    emit optionsChanged();
}

void Document::removePart(QUuid partId)
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

void Document::removePartDontCareComponent(QUuid partId)
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

void Document::addPartToComponent(QUuid partId, QUuid componentId)
{
    Component child(QUuid::createUuid());
    
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

void Document::removeComponent(QUuid componentId)
{
    removeComponentRecursively(componentId);
    emit skeletonChanged();
}

void Document::removeComponentRecursively(QUuid componentId)
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

void Document::setCurrentCanvasComponentId(QUuid componentId)
{
    m_currentCanvasComponentId = componentId;
    const Component *component = findComponent(m_currentCanvasComponentId);
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

void Document::addComponent(QUuid parentId)
{
    Component component(QUuid::createUuid());
    
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

bool Document::isDescendantComponent(QUuid componentId, QUuid suspiciousId)
{
    const Component *loopComponent = findComponentParent(suspiciousId);
    while (nullptr != loopComponent) {
        if (loopComponent->id == componentId)
            return true;
        loopComponent = findComponentParent(loopComponent->parentId);
    }
    return false;
}

void Document::moveComponent(QUuid componentId, QUuid toParentId)
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

void Document::settleOrigin()
{
    if (originSettled())
        return;
    Snapshot snapshot;
    toSnapshot(&snapshot);
    QRectF mainProfile;
    QRectF sideProfile;
    snapshot.resolveBoundingBox(&mainProfile, &sideProfile);
    setOriginX(mainProfile.x() + mainProfile.width() / 2);
    setOriginY(mainProfile.y() + mainProfile.height() / 2);
    setOriginZ(sideProfile.x() + sideProfile.width() / 2);
    markAllDirty();
    emit originChanged();
}

void Document::setPartXmirrorState(QUuid partId, bool mirrored)
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

//void Document::setPartZmirrorState(QUuid partId, bool mirrored)
//{
//    auto part = partMap.find(partId);
//    if (part == partMap.end()) {
//        qDebug() << "Part not found:" << partId;
//        return;
//    }
//    if (part->second.zMirrored == mirrored)
//        return;
//    part->second.zMirrored = mirrored;
//    part->second.dirty = true;
//    settleOrigin();
//    emit partZmirrorStateChanged(partId);
//    emit skeletonChanged();
//}

void Document::setPartDeformThickness(QUuid partId, float thickness)
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

void Document::setPartBase(QUuid partId, PartBase base)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        qDebug() << "Part not found:" << partId;
        return;
    }
    if (part->second.base == base)
        return;
    part->second.base = base;
    part->second.dirty = true;
    emit partBaseChanged(partId);
    emit skeletonChanged();
}

void Document::setPartDeformWidth(QUuid partId, float width)
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

void Document::setPartDeformMapImageId(QUuid partId, QUuid imageId)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        qDebug() << "Part not found:" << partId;
        return;
    }
    if (part->second.deformMapImageId == imageId)
        return;
    part->second.deformMapImageId = imageId;
    part->second.dirty = true;
    emit partDeformMapImageIdChanged(partId);
    emit skeletonChanged();
}

void Document::setPartDeformMapScale(QUuid partId, float scale)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        qDebug() << "Part not found:" << partId;
        return;
    }
    if (qFuzzyCompare(part->second.deformMapScale, scale))
        return;
    part->second.deformMapScale = scale;
    part->second.dirty = true;
    emit partDeformMapScaleChanged(partId);
    emit skeletonChanged();
}

void Document::setPartMaterialId(QUuid partId, QUuid materialId)
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
    emit textureChanged();
}

void Document::setPartRoundState(QUuid partId, bool rounded)
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

void Document::setPartChamferState(QUuid partId, bool chamfered)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        qDebug() << "Part not found:" << partId;
        return;
    }
    if (part->second.chamfered == chamfered)
        return;
    part->second.chamfered = chamfered;
    part->second.dirty = true;
    emit partChamferStateChanged(partId);
    emit skeletonChanged();
}

void Document::setPartTarget(QUuid partId, PartTarget target)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        qDebug() << "Part not found:" << partId;
        return;
    }
    if (part->second.target == target)
        return;
    part->second.target = target;
    part->second.dirty = true;
    emit partTargetChanged(partId);
    emit skeletonChanged();
}

void Document::setPartColorSolubility(QUuid partId, float solubility)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        qDebug() << "Part not found:" << partId;
        return;
    }
    if (qFuzzyCompare(part->second.colorSolubility, solubility))
        return;
    part->second.colorSolubility = solubility;
    part->second.dirty = true;
    emit partColorSolubilityChanged(partId);
    emit skeletonChanged();
}

void Document::setPartHollowThickness(QUuid partId, float hollowThickness)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        qDebug() << "Part not found:" << partId;
        return;
    }
    if (qFuzzyCompare(part->second.hollowThickness, hollowThickness))
        return;
    part->second.hollowThickness = hollowThickness;
    part->second.dirty = true;
    emit partHollowThicknessChanged(partId);
    emit skeletonChanged();
}

void Document::setPartCountershaded(QUuid partId, bool countershaded)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        qDebug() << "Part not found:" << partId;
        return;
    }
    if (part->second.countershaded == countershaded)
        return;
    part->second.countershaded = countershaded;
    part->second.dirty = true;
    emit partCountershadeStateChanged(partId);
    emit textureChanged();
}

void Document::setPartCutRotation(QUuid partId, float cutRotation)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        qDebug() << "Part not found:" << partId;
        return;
    }
    if (qFuzzyCompare(cutRotation, part->second.cutRotation))
        return;
    part->second.setCutRotation(cutRotation);
    part->second.dirty = true;
    emit partCutRotationChanged(partId);
    emit skeletonChanged();
}

void Document::setPartCutFace(QUuid partId, CutFace cutFace)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        qDebug() << "Part not found:" << partId;
        return;
    }
    if (part->second.cutFace == cutFace)
        return;
    part->second.setCutFace(cutFace);
    part->second.dirty = true;
    emit partCutFaceChanged(partId);
    emit skeletonChanged();
}

void Document::setPartCutFaceLinkedId(QUuid partId, QUuid linkedId)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        qDebug() << "Part not found:" << partId;
        return;
    }
    if (part->second.cutFace == CutFace::UserDefined &&
            part->second.cutFaceLinkedId == linkedId)
        return;
    part->second.setCutFaceLinkedId(linkedId);
    part->second.dirty = true;
    emit partCutFaceChanged(partId);
    emit skeletonChanged();
}

void Document::setPartColorState(QUuid partId, bool hasColor, QColor color)
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

void Document::saveSnapshot()
{
    HistoryItem item;
    QElapsedTimer elapsedTimer;
    elapsedTimer.start();
    toSnapshot(&item.snapshot);
    //item.hash = item.snapshot.hash();
    //if (!m_undoItems.empty() && item.hash == m_undoItems[m_undoItems.size() - 1].hash) {
    //    qDebug() << "Snapshot has the same hash:" << item.hash << "skipped";
    //    return;
    //}
    if (m_undoItems.size() + 1 > m_maxSnapshot)
        m_undoItems.pop_front();
    m_undoItems.push_back(item);
    qDebug() << "Snapshot saved with hash:" << item.hash << " Time consumed:" << elapsedTimer.elapsed() << "History count:" << m_undoItems.size();
}

void Document::undo()
{
    if (!undoable())
        return;
    m_redoItems.push_back(m_undoItems.back());
    m_undoItems.pop_back();
    const auto &item = m_undoItems.back();
    fromSnapshot(item.snapshot);
    qDebug() << "Undo/Redo items:" << m_undoItems.size() << m_redoItems.size();
}

void Document::redo()
{
    if (m_redoItems.empty())
        return;
    m_undoItems.push_back(m_redoItems.back());
    const auto &item = m_redoItems.back();
    fromSnapshot(item.snapshot);
    m_redoItems.pop_back();
    qDebug() << "Undo/Redo items:" << m_undoItems.size() << m_redoItems.size();
}

void Document::clearHistories()
{
    m_undoItems.clear();
    m_redoItems.clear();
}

void Document::paste()
{
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();
    if (mimeData->hasText()) {
        QXmlStreamReader xmlStreamReader(mimeData->text());
        Snapshot snapshot;
        loadSkeletonFromXmlStream(&snapshot, xmlStreamReader);
        addFromSnapshot(snapshot, true);
    }
}

bool Document::hasPastableNodesInClipboard() const
{
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();
    if (mimeData->hasText()) {
        if (-1 != mimeData->text().indexOf("<node "))
            return true;
    }
    return false;
}

bool Document::hasPastableMaterialsInClipboard() const
{
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();
    if (mimeData->hasText()) {
        if (-1 != mimeData->text().indexOf("<material "))
            return true;
    }
    return false;
}

bool Document::hasPastablePosesInClipboard() const
{
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();
    if (mimeData->hasText()) {
        if (-1 != mimeData->text().indexOf("<pose "))
            return true;
    }
    return false;
}

bool Document::hasPastableMotionsInClipboard() const
{
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();
    if (mimeData->hasText()) {
        if (-1 != mimeData->text().indexOf("<motion "))
            return true;
    }
    return false;
}

bool Document::undoable() const
{
    return m_undoItems.size() >= 2;
}

bool Document::redoable() const
{
    return !m_redoItems.empty();
}

bool Document::isNodeEditable(QUuid nodeId) const
{
    const SkeletonNode *node = findNode(nodeId);
    if (!node) {
        qDebug() << "Node id not found:" << nodeId;
        return false;
    }
    return !isPartReadonly(node->partId);
}

bool Document::isEdgeEditable(QUuid edgeId) const
{
    const SkeletonEdge *edge = findEdge(edgeId);
    if (!edge) {
        qDebug() << "Edge id not found:" << edgeId;
        return false;
    }
    return !isPartReadonly(edge->partId);
}

void Document::setXlockState(bool locked)
{
    if (xlocked == locked)
        return;
    xlocked = locked;
    emit xlockStateChanged();
}

void Document::setYlockState(bool locked)
{
    if (ylocked == locked)
        return;
    ylocked = locked;
    emit ylockStateChanged();
}

void Document::setZlockState(bool locked)
{
    if (zlocked == locked)
        return;
    zlocked = locked;
    emit zlockStateChanged();
}

void Document::setRadiusLockState(bool locked)
{
    if (radiusLocked == locked)
        return;
    radiusLocked = locked;
    emit radiusLockStateChanged();
}

bool Document::isExportReady() const
{
    if (m_isResultMeshObsolete ||
            m_isTextureObsolete ||
            m_isPostProcessResultObsolete ||
            m_isRigObsolete ||
            m_meshGenerator ||
            m_textureGenerator ||
            m_postProcessor ||
            m_rigGenerator ||
            m_motionsGenerator)
        return false;
    return true;
}

void Document::checkExportReadyState()
{
    if (isExportReady())
        emit exportReady();
}

void Document::setSharedContextWidget(QOpenGLWidget *sharedContextWidget)
{
    m_sharedContextWidget = sharedContextWidget;
}

void Document::collectComponentDescendantParts(QUuid componentId, std::vector<QUuid> &partIds) const
{
    const Component *component = findComponent(componentId);
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

void Document::collectComponentDescendantComponents(QUuid componentId, std::vector<QUuid> &componentIds) const
{
    const Component *component = findComponent(componentId);
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

void Document::hideOtherComponents(QUuid componentId)
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

void Document::lockOtherComponents(QUuid componentId)
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

void Document::hideAllComponents()
{
    for (const auto &part: partMap) {
        setPartVisibleState(part.first, false);
    }
}

void Document::showAllComponents()
{
    for (const auto &part: partMap) {
        setPartVisibleState(part.first, true);
    }
}

void Document::showOrHideAllComponents()
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

void Document::collapseAllComponents()
{
    for (const auto &component: componentMap) {
        if (!component.second.linkToPartId.isNull())
            continue;
        setComponentExpandState(component.first, false);
    }
}

void Document::expandAllComponents()
{
    for (const auto &component: componentMap) {
        if (!component.second.linkToPartId.isNull())
            continue;
        setComponentExpandState(component.first, true);
    }
}

void Document::lockAllComponents()
{
    for (const auto &part: partMap) {
        setPartLockState(part.first, true);
    }
}

void Document::unlockAllComponents()
{
    for (const auto &part: partMap) {
        setPartLockState(part.first, false);
    }
}

void Document::hideDescendantComponents(QUuid componentId)
{
    std::vector<QUuid> partIds;
    collectComponentDescendantParts(componentId, partIds);
    for (const auto &partId: partIds) {
        setPartVisibleState(partId, false);
    }
}

void Document::showDescendantComponents(QUuid componentId)
{
    std::vector<QUuid> partIds;
    collectComponentDescendantParts(componentId, partIds);
    for (const auto &partId: partIds) {
        setPartVisibleState(partId, true);
    }
}

void Document::lockDescendantComponents(QUuid componentId)
{
    std::vector<QUuid> partIds;
    collectComponentDescendantParts(componentId, partIds);
    for (const auto &partId: partIds) {
        setPartLockState(partId, true);
    }
}

void Document::unlockDescendantComponents(QUuid componentId)
{
    std::vector<QUuid> partIds;
    collectComponentDescendantParts(componentId, partIds);
    for (const auto &partId: partIds) {
        setPartLockState(partId, false);
    }
}

void Document::generateRig()
{
    if (nullptr != m_rigGenerator) {
        m_isRigObsolete = true;
        return;
    }
    
    m_isRigObsolete = false;
    
    if (RigType::None == rigType || nullptr == m_currentOutcome) {
        removeRigResults();
        return;
    }
    
    qDebug() << "Rig generating..";
    
    QThread *thread = new QThread;
    m_rigGenerator = new RigGenerator(rigType, *m_postProcessedOutcome);
    m_rigGenerator->moveToThread(thread);
    connect(thread, &QThread::started, m_rigGenerator, &RigGenerator::process);
    connect(m_rigGenerator, &RigGenerator::finished, this, &Document::rigReady);
    connect(m_rigGenerator, &RigGenerator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void Document::rigReady()
{
    m_currentRigSucceed = m_rigGenerator->isSucceed();

    delete m_resultRigWeightMesh;
    m_resultRigWeightMesh = m_rigGenerator->takeResultMesh();
    
    delete m_resultRigBones;
    m_resultRigBones = m_rigGenerator->takeResultBones();
    
    delete m_resultRigWeights;
    m_resultRigWeights = m_rigGenerator->takeResultWeights();
    
    m_resultRigMessages = m_rigGenerator->messages();
    
    delete m_riggedOutcome;
    m_riggedOutcome = m_rigGenerator->takeOutcome();
    if (nullptr == m_riggedOutcome)
        m_riggedOutcome = new Outcome;
    
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

const std::vector<RiggerBone> *Document::resultRigBones() const
{
    return m_resultRigBones;
}

const std::map<int, RiggerVertexWeights> *Document::resultRigWeights() const
{
    return m_resultRigWeights;
}

void Document::removeRigResults()
{
    delete m_resultRigBones;
    m_resultRigBones = nullptr;
    
    delete m_resultRigWeights;
    m_resultRigWeights = nullptr;
    
    delete m_resultRigWeightMesh;
    m_resultRigWeightMesh = nullptr;
    
    m_resultRigMessages.clear();
    
    m_currentRigSucceed = false;
    
    emit resultRigChanged();
}

void Document::setRigType(RigType toRigType)
{
    if (rigType == toRigType)
        return;
    
    rigType = toRigType;
    
    m_isRigObsolete = true;
    
    removeRigResults();
    
    emit rigTypeChanged();
    emit rigChanged();
}

const std::vector<std::pair<QtMsgType, QString>> &Document::resultRigMessages() const
{
    return m_resultRigMessages;
}

const Outcome &Document::currentRiggedOutcome() const
{
    return *m_riggedOutcome;
}

bool Document::currentRigSucceed() const
{
    return m_currentRigSucceed;
}

void Document::generateMotions()
{
    if (nullptr != m_motionsGenerator) {
        return;
    }
    
    const std::vector<RiggerBone> *rigBones = resultRigBones();
    const std::map<int, RiggerVertexWeights> *rigWeights = resultRigWeights();
    
    if (nullptr == rigBones || nullptr == rigWeights) {
        return;
    }
    
    m_motionsGenerator = new MotionsGenerator(rigType, rigBones, rigWeights, currentRiggedOutcome());
    bool hasDirtyMotion = false;
    for (const auto &pose: poseMap) {
        m_motionsGenerator->addPoseToLibrary(pose.first, pose.second.frames, pose.second.yTranslationScale);
    }
    for (auto &motion: motionMap) {
        if (motion.second.dirty) {
            hasDirtyMotion = true;
            motion.second.dirty = false;
            m_motionsGenerator->addRequirement(motion.first);
        }
        m_motionsGenerator->addMotionToLibrary(motion.first, motion.second.clips);
    }
    if (!hasDirtyMotion) {
        delete m_motionsGenerator;
        m_motionsGenerator = nullptr;
        checkExportReadyState();
        return;
    }
    
    qDebug() << "Motions generating..";
    
    QThread *thread = new QThread;
    m_motionsGenerator->moveToThread(thread);
    connect(thread, &QThread::started, m_motionsGenerator, &MotionsGenerator::process);
    connect(m_motionsGenerator, &MotionsGenerator::finished, this, &Document::motionsReady);
    connect(m_motionsGenerator, &MotionsGenerator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void Document::motionsReady()
{
    for (auto &motionId: m_motionsGenerator->generatedMotionIds()) {
        auto motion = motionMap.find(motionId);
        if (motion != motionMap.end()) {
            auto resultPreviewMeshs = m_motionsGenerator->takeResultPreviewMeshs(motionId);
            motion->second.updatePreviewMeshs(resultPreviewMeshs);
            motion->second.jointNodeTrees = m_motionsGenerator->takeResultJointNodeTrees(motionId);
            emit motionPreviewChanged(motionId);
            emit motionResultChanged(motionId);
        }
    }
    
    delete m_motionsGenerator;
    m_motionsGenerator = nullptr;
    
    qDebug() << "Motions generation done";
    
    generateMotions();
}

void Document::generatePosePreviews()
{
    if (nullptr != m_posePreviewsGenerator) {
        return;
    }
    
    const std::vector<RiggerBone> *rigBones = resultRigBones();
    const std::map<int, RiggerVertexWeights> *rigWeights = resultRigWeights();
    
    if (nullptr == rigBones || nullptr == rigWeights) {
        return;
    }

    m_posePreviewsGenerator = new PosePreviewsGenerator(rigType, rigBones,
        rigWeights, *m_riggedOutcome);
    bool hasDirtyPose = false;
    for (auto &poseIt: poseMap) {
        if (!poseIt.second.dirty)
            continue;
        if (poseIt.second.frames.empty())
            continue;
        int middle = poseIt.second.frames.size() / 2;
        if (middle >= (int)poseIt.second.frames.size())
            middle = 0;
        m_posePreviewsGenerator->addPose({poseIt.first, middle}, poseIt.second.frames[middle].second);
        poseIt.second.dirty = false;
        hasDirtyPose = true;
    }
    if (!hasDirtyPose) {
        delete m_posePreviewsGenerator;
        m_posePreviewsGenerator = nullptr;
        return;
    }
    
    qDebug() << "Pose previews generating..";
    
    QThread *thread = new QThread;
    m_posePreviewsGenerator->moveToThread(thread);
    connect(thread, &QThread::started, m_posePreviewsGenerator, &PosePreviewsGenerator::process);
    connect(m_posePreviewsGenerator, &PosePreviewsGenerator::finished, this, &Document::posePreviewsReady);
    connect(m_posePreviewsGenerator, &PosePreviewsGenerator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void Document::posePreviewsReady()
{
    for (const auto &poseIdAndFrame: m_posePreviewsGenerator->generatedPreviewPoseIdAndFrames()) {
        auto pose = poseMap.find(poseIdAndFrame.first);
        if (pose != poseMap.end()) {
            MeshLoader *resultPartPreviewMesh = m_posePreviewsGenerator->takePreview(poseIdAndFrame);
            pose->second.updatePreviewMesh(resultPartPreviewMesh);
            emit posePreviewChanged(poseIdAndFrame.first);
        }
    }

    delete m_posePreviewsGenerator;
    m_posePreviewsGenerator = nullptr;
    
    qDebug() << "Pose previews generation done";
    
    generatePosePreviews();
}

void Document::addMaterial(QUuid materialId, QString name, std::vector<MaterialLayer> layers)
{
    QUuid newMaterialId = materialId;
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

void Document::removeMaterial(QUuid materialId)
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

void Document::setMaterialLayers(QUuid materialId, std::vector<MaterialLayer> layers)
{
    auto findMaterialResult = materialMap.find(materialId);
    if (findMaterialResult == materialMap.end()) {
        qDebug() << "Find material failed:" << materialId;
        return;
    }
    findMaterialResult->second.layers = layers;
    findMaterialResult->second.dirty = true;
    emit materialLayersChanged(materialId);
    emit textureChanged();
    emit optionsChanged();
}

void Document::renameMaterial(QUuid materialId, QString name)
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
    emit materialListChanged();
    emit optionsChanged();
}

void Document::generateMaterialPreviews()
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
    connect(m_materialPreviewsGenerator, &MaterialPreviewsGenerator::finished, this, &Document::materialPreviewsReady);
    connect(m_materialPreviewsGenerator, &MaterialPreviewsGenerator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void Document::materialPreviewsReady()
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

bool Document::isMeshGenerating() const
{
    return nullptr != m_meshGenerator;
}

void Document::copyNodes(std::set<QUuid> nodeIdSet) const
{
    Snapshot snapshot;
    toSnapshot(&snapshot, nodeIdSet, DocumentToSnapshotFor::Nodes);
    QString snapshotXml;
    QXmlStreamWriter xmlStreamWriter(&snapshotXml);
    saveSkeletonToXmlStream(&snapshot, &xmlStreamWriter);
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(snapshotXml);
}

void Document::initScript(const QString &script)
{
    m_script = script;
    emit scriptModifiedFromExternal();
}

void Document::updateScript(const QString &script)
{
    if (m_script == script)
        return;
    
    m_script = script;
    emit scriptChanged();
}

void Document::updateVariable(const QString &name, const std::map<QString, QString> &value)
{
    bool needRunScript = false;
    auto variable = m_cachedVariables.find(name);
    if (variable == m_cachedVariables.end()) {
        m_cachedVariables[name] = value;
        needRunScript = true;
    } else if (variable->second != value) {
        variable->second = value;
    } else {
        return;
    }
    m_mergedVariables[name] = value;
    emit mergedVaraiblesChanged();
    if (needRunScript)
        runScript();
}

void Document::updateVariableValue(const QString &name, const QString &value)
{
    auto variable = m_cachedVariables.find(name);
    if (variable == m_cachedVariables.end()) {
        qDebug() << "Update a nonexist variable:" << name << "value:" << value;
        return;
    }
    auto &variableValue = variable->second["value"];
    if (variableValue == value)
        return;
    variableValue = value;
    m_mergedVariables[name] = variable->second;
    runScript();
}

bool Document::updateDefaultVariables(const std::map<QString, std::map<QString, QString>> &defaultVariables)
{
    bool updated = false;
    for (const auto &it: defaultVariables) {
        auto findMergedVariable = m_mergedVariables.find(it.first);
        if (findMergedVariable != m_mergedVariables.end()) {
            bool hasChangedAttribute = false;
            for (const auto &attribute: it.second) {
                if (attribute.first == "value")
                    continue;
                const auto &findMatch = findMergedVariable->second.find(attribute.first);
                if (findMatch != findMergedVariable->second.end()) {
                    if (findMatch->second == attribute.second)
                        continue;
                }
                hasChangedAttribute = true;
            }
            if (!hasChangedAttribute)
                continue;
        }
        updated = true;
        auto findCached = m_cachedVariables.find(it.first);
        if (findCached != m_cachedVariables.end()) {
            m_mergedVariables[it.first] = it.second;
            m_mergedVariables[it.first]["value"] = valueOfKeyInMapOrEmpty(findCached->second, "value");
        } else {
            m_mergedVariables[it.first] = it.second;
            m_cachedVariables[it.first] = it.second;
        }
    }
    std::vector<QString> eraseList;
    for (const auto &it: m_mergedVariables) {
        if (defaultVariables.end() == defaultVariables.find(it.first)) {
            eraseList.push_back(it.first);
            updated = true;
        }
    }
    for (const auto &it: eraseList) {
        m_mergedVariables.erase(it);
    }
    if (updated) {
        emit mergedVaraiblesChanged();
    }
    return updated;
}

void Document::runScript()
{
    if (nullptr != m_scriptRunner) {
        m_isScriptResultObsolete = true;
        return;
    }
    
    m_isScriptResultObsolete = false;
    
    qDebug() << "Script running..";
    
    QThread *thread = new QThread;

    m_scriptRunner = new ScriptRunner();
    m_scriptRunner->moveToThread(thread);
    m_scriptRunner->setScript(new QString(m_script));
    m_scriptRunner->setVariables(new std::map<QString, std::map<QString, QString>>(
        m_mergedVariables.empty() ? m_cachedVariables : m_mergedVariables
        ));
    connect(thread, &QThread::started, m_scriptRunner, &ScriptRunner::process);
    connect(m_scriptRunner, &ScriptRunner::finished, this, &Document::scriptResultReady);
    connect(m_scriptRunner, &ScriptRunner::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    emit scriptRunning();
    thread->start();
}

void Document::scriptResultReady()
{
    Snapshot *snapshot = m_scriptRunner->takeResultSnapshot();
    std::map<QString, std::map<QString, QString>> *defaultVariables = m_scriptRunner->takeDefaultVariables();
    bool errorChanged = false;
    bool consoleLogChanged = false;
    bool mergedVariablesChanged = false;
    
    const QString &scriptError = m_scriptRunner->scriptError();
    if (m_scriptError != scriptError) {
        m_scriptError = scriptError;
        errorChanged = true;
    }
    
    const QString &consoleLog = m_scriptRunner->consoleLog();
    if (m_scriptConsoleLog != consoleLog) {
        m_scriptConsoleLog = consoleLog;
        consoleLogChanged = true;
    }
    
    if (nullptr != snapshot) {
        fromSnapshot(*snapshot);
        delete snapshot;
        saveSnapshot();
    }
    
    if (nullptr != defaultVariables) {
        mergedVariablesChanged = updateDefaultVariables(*defaultVariables);
        delete defaultVariables;
    }

    delete m_scriptRunner;
    m_scriptRunner = nullptr;
    
    if (errorChanged)
        emit scriptErrorChanged();
    
    if (consoleLogChanged)
        emit scriptConsoleLogChanged();
    
    qDebug() << "Script run done";

    if (m_isScriptResultObsolete || mergedVariablesChanged) {
        runScript();
    }
}

const QString &Document::script() const
{
    return m_script;
}

const std::map<QString, std::map<QString, QString>> &Document::variables() const
{
    return m_mergedVariables;
}

const QString &Document::scriptError() const
{
    return m_scriptError;
}

const QString &Document::scriptConsoleLog() const
{
    return m_scriptConsoleLog;
}

void Document::startPaint(void)
{
}

void Document::stopPaint(void)
{
    if (m_mousePicker || m_isMouseTargetResultObsolete) {
        m_saveNextPaintSnapshot = true;
        return;
    }
    saveSnapshot();
    for (const auto &it: partMap) {
        m_intermediatePaintImageIds.erase(it.second.deformMapImageId);
    }
    for (const auto &it: m_intermediatePaintImageIds) {
        //qDebug() << "Remove intermediate image:" << it;
        ImageForever::remove(it);
    }
    m_intermediatePaintImageIds.clear();
}

void Document::setMousePickMaskNodeIds(const std::set<QUuid> &nodeIds)
{
    m_mousePickMaskNodeIds = nodeIds;
}

void Document::createGriddedPartsFromNodes(const std::set<QUuid> &nodeIds)
{
    if (nullptr == m_currentOutcome)
        return;
    
    const auto &vertices = m_currentOutcome->vertices;
    const auto &vertexSourceNodes = m_currentOutcome->vertexSourceNodes;
    std::set<size_t> selectedVertices;
    for (size_t i = 0; i < vertexSourceNodes.size(); ++i) {
        if (nodeIds.find(vertexSourceNodes[i].second) == nodeIds.end())
            continue;
        selectedVertices.insert(i);
    }
    
    std::vector<QVector3D> newVertices;
    std::map<size_t, size_t> oldToNewMap;
    std::vector<std::pair<size_t, size_t>> newEdges;
    
    auto oldToNew = [&](size_t oldIndex) {
        auto findNewIndex = oldToNewMap.find(oldIndex);
        if (findNewIndex != oldToNewMap.end())
            return findNewIndex->second;
        size_t newIndex = newVertices.size();
        newVertices.push_back(vertices[oldIndex]);
        oldToNewMap.insert({oldIndex, newIndex});
        return newIndex;
    };
    
    std::set<std::pair<size_t, size_t>> visitedOldEdges;
    const auto &faces = m_currentOutcome->triangleAndQuads;
    for (const auto &face: faces) {
        bool isFaceSelected = false;
        for (size_t i = 0; i < face.size(); ++i) {
            if (selectedVertices.find(face[i]) != selectedVertices.end()) {
                isFaceSelected = true;
                break;
            }
        }
        if (!isFaceSelected)
            continue;
        for (size_t i = 0; i < face.size(); ++i) {
            size_t j = (i + 1) % face.size();
            auto oldEdge = std::make_pair(face[i], face[j]);
            if (visitedOldEdges.find(oldEdge) != visitedOldEdges.end())
                continue;
            visitedOldEdges.insert(oldEdge);
            visitedOldEdges.insert(std::make_pair(oldEdge.second, oldEdge.first));
            newEdges.push_back({
                oldToNew(oldEdge.first),
                oldToNew(oldEdge.second)
            });
        }
    }
    
    createFromNodesAndEdges(newVertices, newEdges);
}
