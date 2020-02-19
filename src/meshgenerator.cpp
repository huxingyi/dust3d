#include <QDebug>
#include <QElapsedTimer>
#include <QVector2D>
#include <QGuiApplication>
#include <QMatrix4x4>
#include <iostream>
#include "strokemeshbuilder.h"
#include "strokemodifier.h"
#include "meshrecombiner.h"
#include "meshgenerator.h"
#include "util.h"
#include "trianglesourcenoderesolve.h"
#include "cutface.h"
#include "parttarget.h"
#include "theme.h"
#include "partbase.h"
#include "imageforever.h"
#include "triangulatefaces.h"
#include "remesher.h"
#include "polycount.h"
#include "clothsimulator.h"
#include "isotropicremesh.h"
#include "projectfacestonodes.h"
#include "document.h"
#include "simulateclothmeshes.h"

MeshGenerator::MeshGenerator(Snapshot *snapshot) :
    m_snapshot(snapshot)
{
}

MeshGenerator::~MeshGenerator()
{
    for (auto &it: m_partPreviewMeshes)
        delete it.second;
    delete m_resultMesh;
    delete m_snapshot;
    delete m_outcome;
    delete m_cutFaceTransforms;
    delete m_nodesCutFaces;
}

void MeshGenerator::setId(quint64 id)
{
    m_id = id;
}

quint64 MeshGenerator::id()
{
    return m_id;
}

bool MeshGenerator::isSucceed()
{
    return m_isSucceed;
}

MeshLoader *MeshGenerator::takeResultMesh()
{
    MeshLoader *resultMesh = m_resultMesh;
    m_resultMesh = nullptr;
    return resultMesh;
}

MeshLoader *MeshGenerator::takePartPreviewMesh(const QUuid &partId)
{
    MeshLoader *resultMesh = m_partPreviewMeshes[partId];
    m_partPreviewMeshes[partId] = nullptr;
    return resultMesh;
}

const std::set<QUuid> &MeshGenerator::generatedPreviewPartIds()
{
    return m_generatedPreviewPartIds;
}

Outcome *MeshGenerator::takeOutcome()
{
    Outcome *outcome = m_outcome;
    m_outcome = nullptr;
    return outcome;
}

std::map<QUuid, StrokeMeshBuilder::CutFaceTransform> *MeshGenerator::takeCutFaceTransforms()
{
    auto cutFaceTransforms = m_cutFaceTransforms;
    m_cutFaceTransforms = nullptr;
    return cutFaceTransforms;
}

std::map<QUuid, std::map<QString, QVector2D>> *MeshGenerator::takeNodesCutFaces()
{
    auto nodesCutFaces = m_nodesCutFaces;
    m_nodesCutFaces = nullptr;
    return nodesCutFaces;
}

void MeshGenerator::collectParts()
{
    for (const auto &node: m_snapshot->nodes) {
        QString partId = valueOfKeyInMapOrEmpty(node.second, "partId");
        if (partId.isEmpty())
            continue;
        m_partNodeIds[partId].insert(node.first);
    }
    for (const auto &edge: m_snapshot->edges) {
        QString partId = valueOfKeyInMapOrEmpty(edge.second, "partId");
        if (partId.isEmpty())
            continue;
        m_partEdgeIds[partId].insert(edge.first);
    }
}

bool MeshGenerator::checkIsPartDirty(const QString &partIdString)
{
    auto findPart = m_snapshot->parts.find(partIdString);
    if (findPart == m_snapshot->parts.end()) {
        qDebug() << "Find part failed:" << partIdString;
        return false;
    }
    return isTrueValueString(valueOfKeyInMapOrEmpty(findPart->second, "dirty"));
}

bool MeshGenerator::checkIsPartDependencyDirty(const QString &partIdString)
{
    auto findPart = m_snapshot->parts.find(partIdString);
    if (findPart == m_snapshot->parts.end()) {
        qDebug() << "Find part failed:" << partIdString;
        return false;
    }
    QString cutFaceString = valueOfKeyInMapOrEmpty(findPart->second, "cutFace");
    QUuid cutFaceLinkedPartId = QUuid(cutFaceString);
    if (!cutFaceLinkedPartId.isNull()) {
        if (checkIsPartDirty(cutFaceString))
            return true;
    }
    for (const auto &nodeIdString: m_partNodeIds[partIdString]) {
        auto findNode = m_snapshot->nodes.find(nodeIdString);
        if (findNode == m_snapshot->nodes.end()) {
            qDebug() << "Find node failed:" << nodeIdString;
            continue;
        }
        QString cutFaceString = valueOfKeyInMapOrEmpty(findNode->second, "cutFace");
        QUuid cutFaceLinkedPartId = QUuid(cutFaceString);
        if (!cutFaceLinkedPartId.isNull()) {
            if (checkIsPartDirty(cutFaceString))
                return true;
        }
    }
    return false;
}

bool MeshGenerator::checkIsComponentDirty(const QString &componentIdString)
{
    bool isDirty = false;
    
    const std::map<QString, QString> *component = &m_snapshot->rootComponent;
    if (componentIdString != QUuid().toString()) {
        auto findComponent = m_snapshot->components.find(componentIdString);
        if (findComponent == m_snapshot->components.end()) {
            qDebug() << "Component not found:" << componentIdString;
            return isDirty;
        }
        component = &findComponent->second;
    }
    
    if (isTrueValueString(valueOfKeyInMapOrEmpty(*component, "dirty"))) {
        isDirty = true;
    }
    
    QString linkDataType = valueOfKeyInMapOrEmpty(*component, "linkDataType");
    if ("partId" == linkDataType) {
        QString partId = valueOfKeyInMapOrEmpty(*component, "linkData");
        if (checkIsPartDirty(partId)) {
            m_dirtyPartIds.insert(partId);
            isDirty = true;
        }
        if (!isDirty) {
            if (checkIsPartDependencyDirty(partId)) {
                isDirty = true;
            }
        }
    }
    
    for (const auto &childId: valueOfKeyInMapOrEmpty(*component, "children").split(",")) {
        if (childId.isEmpty())
            continue;
        if (checkIsComponentDirty(childId)) {
            isDirty = true;
        }
    }
    
    if (isDirty)
        m_dirtyComponentIds.insert(componentIdString);
    
    return isDirty;
}

void MeshGenerator::checkDirtyFlags()
{
    checkIsComponentDirty(QUuid().toString());
}

void MeshGenerator::cutFaceStringToCutTemplate(const QString &cutFaceString, std::vector<QVector2D> &cutTemplate)
{
    //std::map<QString, QVector2D> cutTemplateMapByName;
    QUuid cutFaceLinkedPartId = QUuid(cutFaceString);
    if (!cutFaceLinkedPartId.isNull()) {
        std::map<QString, std::tuple<float, float, float>> cutFaceNodeMap;
        auto findCutFaceLinkedPart = m_snapshot->parts.find(cutFaceString);
        if (findCutFaceLinkedPart == m_snapshot->parts.end()) {
            qDebug() << "Find cut face linked part failed:" << cutFaceString;
        } else {
            // Build node info map
            for (const auto &nodeIdString: m_partNodeIds[cutFaceString]) {
                auto findNode = m_snapshot->nodes.find(nodeIdString);
                if (findNode == m_snapshot->nodes.end()) {
                    qDebug() << "Find node failed:" << nodeIdString;
                    continue;
                }
                auto &node = findNode->second;
                float radius = valueOfKeyInMapOrEmpty(node, "radius").toFloat();
                float x = (valueOfKeyInMapOrEmpty(node, "x").toFloat() - m_mainProfileMiddleX);
                float y = (m_mainProfileMiddleY - valueOfKeyInMapOrEmpty(node, "y").toFloat());
                cutFaceNodeMap.insert({nodeIdString, std::make_tuple(radius, x, y)});
            }
            // Build edge link
            std::map<QString, std::vector<QString>> cutFaceNodeLinkMap;
            for (const auto &edgeIdString: m_partEdgeIds[cutFaceString]) {
                auto findEdge = m_snapshot->edges.find(edgeIdString);
                if (findEdge == m_snapshot->edges.end()) {
                    qDebug() << "Find edge failed:" << edgeIdString;
                    continue;
                }
                auto &edge = findEdge->second;
                QString fromNodeIdString = valueOfKeyInMapOrEmpty(edge, "from");
                QString toNodeIdString = valueOfKeyInMapOrEmpty(edge, "to");
                cutFaceNodeLinkMap[fromNodeIdString].push_back(toNodeIdString);
                cutFaceNodeLinkMap[toNodeIdString].push_back(fromNodeIdString);
            }
            // Find endpoint
            QString endPointNodeIdString;
            std::vector<std::pair<QString, std::tuple<float, float, float>>> endpointNodes;
            for (const auto &it: cutFaceNodeLinkMap) {
                if (1 == it.second.size()) {
                    const auto &findNode = cutFaceNodeMap.find(it.first);
                    if (findNode != cutFaceNodeMap.end())
                        endpointNodes.push_back({it.first, findNode->second});
                }
            }
            bool isRing = endpointNodes.empty();
            if (endpointNodes.empty()) {
                for (const auto &it: cutFaceNodeMap) {
                    endpointNodes.push_back({it.first, it.second});
                }
            }
            if (!endpointNodes.empty()) {
                // Calculate the center points
                QVector2D sumOfPositions;
                for (const auto &it: endpointNodes) {
                    sumOfPositions += QVector2D(std::get<1>(it.second), std::get<2>(it.second));
                }
                QVector2D center = sumOfPositions / endpointNodes.size();
                
                // Calculate all the directions emit from center to the endpoint,
                // choose the minimal angle, angle: (0, 0 -> -1, -1) to the direction
                const QVector3D referenceDirection = QVector3D(-1, -1, 0).normalized();
                int choosenEndpoint = -1;
                float choosenRadian = 0;
                for (int i = 0; i < (int)endpointNodes.size(); ++i) {
                    const auto &it = endpointNodes[i];
                    QVector2D direction2d = (QVector2D(std::get<1>(it.second), std::get<2>(it.second)) -
                        center);
                    QVector3D direction = QVector3D(direction2d.x(), direction2d.y(), 0).normalized();
                    float radian = radianBetweenVectors(referenceDirection, direction);
                    if (-1 == choosenEndpoint || radian < choosenRadian) {
                        choosenRadian = radian;
                        choosenEndpoint = i;
                    }
                }
                endPointNodeIdString = endpointNodes[choosenEndpoint].first;
            }
            // Loop all linked nodes
            std::vector<std::tuple<float, float, float, QString>> cutFaceNodes;
            std::set<QString> cutFaceVisitedNodeIds;
            std::function<void (const QString &)> loopNodeLink;
            loopNodeLink = [&](const QString &fromNodeIdString) {
                auto findCutFaceNode = cutFaceNodeMap.find(fromNodeIdString);
                if (findCutFaceNode == cutFaceNodeMap.end())
                    return;
                if (cutFaceVisitedNodeIds.find(fromNodeIdString) != cutFaceVisitedNodeIds.end())
                    return;
                cutFaceVisitedNodeIds.insert(fromNodeIdString);
                cutFaceNodes.push_back(std::make_tuple(std::get<0>(findCutFaceNode->second),
                    std::get<1>(findCutFaceNode->second),
                    std::get<2>(findCutFaceNode->second),
                    fromNodeIdString));
                auto findNeighbor = cutFaceNodeLinkMap.find(fromNodeIdString);
                if (findNeighbor == cutFaceNodeLinkMap.end())
                    return;
                for (const auto &it: findNeighbor->second) {
                    if (cutFaceVisitedNodeIds.find(it) == cutFaceVisitedNodeIds.end()) {
                        loopNodeLink(it);
                        break;
                    }
                }
            };
            if (!endPointNodeIdString.isEmpty()) {
                loopNodeLink(endPointNodeIdString);
            }
            // Fetch points from linked nodes
            std::vector<QString> cutTemplateNames;
            cutFacePointsFromNodes(cutTemplate, cutFaceNodes, isRing, &cutTemplateNames);
            //for (size_t i = 0; i < cutTemplateNames.size(); ++i) {
            //    cutTemplateMapByName.insert({cutTemplateNames[i], cutTemplate[i]});
            //}
        }
    }
    if (cutTemplate.size() < 3) {
        CutFace cutFace = CutFaceFromString(cutFaceString.toUtf8().constData());
        cutTemplate = CutFaceToPoints(cutFace);
        //cutTemplateMapByName.clear();
        //for (size_t i = 0; i < cutTemplate.size(); ++i) {
        //    cutTemplateMapByName.insert({cutFaceString + "/" + QString::number(i + 1), cutTemplate[i]});
        //}
    }
}

MeshCombiner::Mesh *MeshGenerator::combinePartMesh(const QString &partIdString, bool *hasError, bool addIntermediateNodes)
{
    auto findPart = m_snapshot->parts.find(partIdString);
    if (findPart == m_snapshot->parts.end()) {
        qDebug() << "Find part failed:" << partIdString;
        return nullptr;
    }
    
    QUuid partId = QUuid(partIdString);
    auto &part = findPart->second;
    bool isDisabled = isTrueValueString(valueOfKeyInMapOrEmpty(part, "disabled"));
    bool xMirrored = isTrueValueString(valueOfKeyInMapOrEmpty(part, "xMirrored"));
    bool subdived = isTrueValueString(valueOfKeyInMapOrEmpty(part, "subdived"));
    bool rounded = isTrueValueString(valueOfKeyInMapOrEmpty(part, "rounded"));
    bool chamfered = isTrueValueString(valueOfKeyInMapOrEmpty(part, "chamfered"));
    bool countershaded = isTrueValueString(valueOfKeyInMapOrEmpty(part, "countershaded"));
    QString colorString = valueOfKeyInMapOrEmpty(part, "color");
    QColor partColor = colorString.isEmpty() ? m_defaultPartColor : QColor(colorString);
    float deformThickness = 1.0;
    float deformWidth = 1.0;
    float cutRotation = 0.0;
    float hollowThickness = 0.0;
    auto target = PartTargetFromString(valueOfKeyInMapOrEmpty(part, "target").toUtf8().constData());
    auto base = PartBaseFromString(valueOfKeyInMapOrEmpty(part, "base").toUtf8().constData());
    //bool gridded = isTrueValueString(valueOfKeyInMapOrEmpty(part, "gridded"));

    QString cutFaceString = valueOfKeyInMapOrEmpty(part, "cutFace");
    std::vector<QVector2D> cutTemplate;
    cutFaceStringToCutTemplate(cutFaceString, cutTemplate);
    if (chamfered)
        chamferFace2D(&cutTemplate);
    
    QString cutRotationString = valueOfKeyInMapOrEmpty(part, "cutRotation");
    if (!cutRotationString.isEmpty()) {
        cutRotation = cutRotationString.toFloat();
    }
    
    QString hollowThicknessString = valueOfKeyInMapOrEmpty(part, "hollowThickness");
    if (!hollowThicknessString.isEmpty()) {
        hollowThickness = hollowThicknessString.toFloat();
    }
    
    QString thicknessString = valueOfKeyInMapOrEmpty(part, "deformThickness");
    if (!thicknessString.isEmpty()) {
        deformThickness = thicknessString.toFloat();
    }
    
    QString widthString = valueOfKeyInMapOrEmpty(part, "deformWidth");
    if (!widthString.isEmpty()) {
        deformWidth = widthString.toFloat();
    }
    
    QImage deformImageStruct;
    const QImage *deformImage = nullptr;
    QString deformMapImageIdString = valueOfKeyInMapOrEmpty(part, "deformMapImageId");
    if (!deformMapImageIdString.isEmpty()) {
        ImageForever::copy(QUuid(deformMapImageIdString), deformImageStruct);
        if (!deformImageStruct.isNull())
            deformImage = &deformImageStruct;
        if (nullptr == deformImage) {
            qDebug() << "Deform image id not found:" << deformMapImageIdString;
        }
    }
    
    float deformMapScale = 1.0;
    QString deformMapScaleString = valueOfKeyInMapOrEmpty(part, "deformMapScale");
    if (!deformMapScaleString.isEmpty())
        deformMapScale = deformMapScaleString.toFloat();
    
    QUuid materialId;
    QString materialIdString = valueOfKeyInMapOrEmpty(part, "materialId");
    if (!materialIdString.isEmpty())
        materialId = QUuid(materialIdString);
    
    float colorSolubility = 0;
    QString colorSolubilityString = valueOfKeyInMapOrEmpty(part, "colorSolubility");
    if (!colorSolubilityString.isEmpty())
        colorSolubility = colorSolubilityString.toFloat();
    
    auto &partCache = m_cacheContext->parts[partIdString];
    partCache.outcomeNodes.clear();
    partCache.outcomeEdges.clear();
    partCache.outcomeNodeVertices.clear();
    partCache.outcomePaintMap.clear();
    partCache.outcomePaintMap.partId = partId;
    partCache.vertices.clear();
    partCache.faces.clear();
    partCache.previewTriangles.clear();
    partCache.previewVertices.clear();
    partCache.isSucceed = false;
    partCache.joined = (target == PartTarget::Model && !isDisabled);
    delete partCache.mesh;
    partCache.mesh = nullptr;
    
    struct NodeInfo
    {
        float radius = 0;
        QVector3D position;
        BoneMark boneMark = BoneMark::None;
        bool hasCutFaceSettings = false;
        float cutRotation = 0.0;
        QString cutFace;
    };
    std::map<QString, NodeInfo> nodeInfos;
    for (const auto &nodeIdString: m_partNodeIds[partIdString]) {
        auto findNode = m_snapshot->nodes.find(nodeIdString);
        if (findNode == m_snapshot->nodes.end()) {
            qDebug() << "Find node failed:" << nodeIdString;
            continue;
        }
        auto &node = findNode->second;
        
        float radius = valueOfKeyInMapOrEmpty(node, "radius").toFloat();
        float x = (valueOfKeyInMapOrEmpty(node, "x").toFloat() - m_mainProfileMiddleX);
        float y = (m_mainProfileMiddleY - valueOfKeyInMapOrEmpty(node, "y").toFloat());
        float z = (m_sideProfileMiddleX - valueOfKeyInMapOrEmpty(node, "z").toFloat());

        BoneMark boneMark = BoneMarkFromString(valueOfKeyInMapOrEmpty(node, "boneMark").toUtf8().constData());
        
        bool hasCutFaceSettings = false;
        float cutRotation = 0.0;
        QString cutFace;
        
        const auto &cutFaceIt = node.find("cutFace");
        if (cutFaceIt != node.end()) {
            cutFace = cutFaceIt->second;
            hasCutFaceSettings = true;
            const auto &cutRotationIt = node.find("cutRotation");
            if (cutRotationIt != node.end()) {
                cutRotation = cutRotationIt->second.toFloat();
            }
        }
        
        auto &nodeInfo = nodeInfos[nodeIdString];
        nodeInfo.position = QVector3D(x, y, z);
        nodeInfo.radius = radius;
        nodeInfo.boneMark = boneMark;
        nodeInfo.hasCutFaceSettings = hasCutFaceSettings;
        nodeInfo.cutRotation = cutRotation;
        nodeInfo.cutFace = cutFace;
    }
    
    std::set<std::pair<QString, QString>> edges;
    for (const auto &edgeIdString: m_partEdgeIds[partIdString]) {
        auto findEdge = m_snapshot->edges.find(edgeIdString);
        if (findEdge == m_snapshot->edges.end()) {
            qDebug() << "Find edge failed:" << edgeIdString;
            continue;
        }
        auto &edge = findEdge->second;
        
        QString fromNodeIdString = valueOfKeyInMapOrEmpty(edge, "from");
        QString toNodeIdString = valueOfKeyInMapOrEmpty(edge, "to");
        
        const auto &findFromNodeInfo = nodeInfos.find(fromNodeIdString);
        if (findFromNodeInfo == nodeInfos.end()) {
            qDebug() << "Find from-node info failed:" << fromNodeIdString;
            continue;
        }
        
        const auto &findToNodeInfo = nodeInfos.find(toNodeIdString);
        if (findToNodeInfo == nodeInfos.end()) {
            qDebug() << "Find to-node info failed:" << toNodeIdString;
            continue;
        }
        
        edges.insert({fromNodeIdString, toNodeIdString});
    }
    
    bool buildSucceed = false;
    std::map<QString, int> nodeIdStringToIndexMap;
    std::map<int, QString> nodeIndexToIdStringMap;
    StrokeModifier *nodeMeshModifier = nullptr;
    StrokeMeshBuilder *nodeMeshBuilder = nullptr;
    
    QString mirroredPartIdString;
    QUuid mirroredPartId;
    if (xMirrored) {
        mirroredPartId = QUuid().createUuid();
        mirroredPartIdString = mirroredPartId.toString();
        m_cacheContext->partMirrorIdMap[mirroredPartIdString] = partIdString;
    }
    
    auto addNode = [&](const QString &nodeIdString, const NodeInfo &nodeInfo) {
        OutcomeNode outcomeNode;
        outcomeNode.partId = QUuid(partIdString);
        outcomeNode.nodeId = QUuid(nodeIdString);
        outcomeNode.origin = nodeInfo.position;
        outcomeNode.radius = nodeInfo.radius;
        outcomeNode.color = partColor;
        outcomeNode.materialId = materialId;
        outcomeNode.countershaded = countershaded;
        outcomeNode.colorSolubility = colorSolubility;
        outcomeNode.boneMark = nodeInfo.boneMark;
        outcomeNode.mirroredByPartId = mirroredPartIdString;
        outcomeNode.joined = partCache.joined;
        partCache.outcomeNodes.push_back(outcomeNode);
        if (xMirrored) {
            outcomeNode.partId = mirroredPartId;
            outcomeNode.mirrorFromPartId = QUuid(partId);
            outcomeNode.mirroredByPartId = QUuid();
            outcomeNode.origin.setX(-nodeInfo.position.x());
            partCache.outcomeNodes.push_back(outcomeNode);
        }
    };
    auto addEdge = [&](const QString &firstNodeIdString, const QString &secondNodeIdString) {
        partCache.outcomeEdges.push_back({
            {QUuid(partIdString), QUuid(firstNodeIdString)},
            {QUuid(partIdString), QUuid(secondNodeIdString)}
        });
        if (xMirrored) {
            partCache.outcomeEdges.push_back({
                {mirroredPartId, QUuid(firstNodeIdString)},
                {mirroredPartId, QUuid(secondNodeIdString)}
            });
        }
    };
    
    nodeMeshModifier = new StrokeModifier;
    
    if (addIntermediateNodes)
        nodeMeshModifier->enableIntermediateAddition();
    
    for (const auto &nodeIt: nodeInfos) {
        const auto &nodeIdString = nodeIt.first;
        const auto &nodeInfo = nodeIt.second;
        size_t nodeIndex = 0;
        if (nodeInfo.hasCutFaceSettings) {
            std::vector<QVector2D> nodeCutTemplate;
            cutFaceStringToCutTemplate(nodeInfo.cutFace, nodeCutTemplate);
            if (chamfered)
                chamferFace2D(&nodeCutTemplate);
            nodeIndex = nodeMeshModifier->addNode(nodeInfo.position, nodeInfo.radius, nodeCutTemplate, nodeInfo.cutRotation);
        } else {
            nodeIndex = nodeMeshModifier->addNode(nodeInfo.position, nodeInfo.radius, cutTemplate, cutRotation);
        }
        nodeIdStringToIndexMap[nodeIdString] = nodeIndex;
        nodeIndexToIdStringMap[nodeIndex] = nodeIdString;
        
        addNode(nodeIdString, nodeInfo);
    }
    
    for (const auto &edgeIt: edges) {
        const QString &fromNodeIdString = edgeIt.first;
        const QString &toNodeIdString = edgeIt.second;
        
        auto findFromNodeIndex = nodeIdStringToIndexMap.find(fromNodeIdString);
        if (findFromNodeIndex == nodeIdStringToIndexMap.end()) {
            qDebug() << "Find from-node failed:" << fromNodeIdString;
            continue;
        }
        
        auto findToNodeIndex = nodeIdStringToIndexMap.find(toNodeIdString);
        if (findToNodeIndex == nodeIdStringToIndexMap.end()) {
            qDebug() << "Find to-node failed:" << toNodeIdString;
            continue;
        }
        
        nodeMeshModifier->addEdge(findFromNodeIndex->second, findToNodeIndex->second);
        addEdge(fromNodeIdString, toNodeIdString);
    }
    
    if (subdived)
        nodeMeshModifier->subdivide();
    
    if (rounded)
        nodeMeshModifier->roundEnd();
    
    nodeMeshModifier->finalize();
    
    nodeMeshBuilder = new StrokeMeshBuilder;
    nodeMeshBuilder->setDeformThickness(deformThickness);
    nodeMeshBuilder->setDeformWidth(deformWidth);
    nodeMeshBuilder->setDeformMapScale(deformMapScale);
    nodeMeshBuilder->setHollowThickness(hollowThickness);
    if (nullptr != deformImage)
        nodeMeshBuilder->setDeformMapImage(deformImage);
    if (PartBase::YZ == base) {
        nodeMeshBuilder->enableBaseNormalOnX(false);
    } else if (PartBase::Average == base) {
        nodeMeshBuilder->enableBaseNormalAverage(true);
    } else if (PartBase::XY == base) {
        nodeMeshBuilder->enableBaseNormalOnZ(false);
    } else if (PartBase::ZX == base) {
        nodeMeshBuilder->enableBaseNormalOnY(false);
    }
    
    std::vector<size_t> builderNodeIndices;
    for (const auto &node: nodeMeshModifier->nodes()) {
        auto nodeIndex = nodeMeshBuilder->addNode(node.position, node.radius, node.cutTemplate, node.cutRotation);
        nodeMeshBuilder->setNodeOriginInfo(nodeIndex, node.nearOriginNodeIndex, node.farOriginNodeIndex);
        builderNodeIndices.push_back(nodeIndex);
        
        const auto &originNodeIdString = nodeIndexToIdStringMap[node.originNodeIndex];
        
        OutcomePaintNode paintNode;
        paintNode.originNodeIndex = node.originNodeIndex;
        paintNode.originNodeId = QUuid(originNodeIdString);
        paintNode.radius = node.radius;
        paintNode.origin = node.position;
        
        partCache.outcomePaintMap.paintNodes.push_back(paintNode);
    }
    for (const auto &edge: nodeMeshModifier->edges())
        nodeMeshBuilder->addEdge(edge.firstNodeIndex, edge.secondNodeIndex);
    buildSucceed = nodeMeshBuilder->build();
    
    partCache.vertices = nodeMeshBuilder->generatedVertices();
    partCache.faces = nodeMeshBuilder->generatedFaces();
    for (size_t i = 0; i < partCache.vertices.size(); ++i) {
        const auto &position = partCache.vertices[i];
        const auto &source = nodeMeshBuilder->generatedVerticesSourceNodeIndices()[i];
        size_t nodeIndex = nodeMeshModifier->nodes()[source].originNodeIndex;
        const auto &nodeIdString = nodeIndexToIdStringMap[nodeIndex];
        partCache.outcomeNodeVertices.push_back({position, {partIdString, nodeIdString}});
        
        auto &paintNode = partCache.outcomePaintMap.paintNodes[source];
        paintNode.vertices.push_back(position);
    }
    
    for (size_t i = 0; i < partCache.outcomePaintMap.paintNodes.size(); ++i) {
        auto &paintNode = partCache.outcomePaintMap.paintNodes[i];
        paintNode.baseNormal = nodeMeshBuilder->nodeBaseNormal(i);
        paintNode.direction = nodeMeshBuilder->nodeTraverseDirection(i);
        paintNode.order = nodeMeshBuilder->nodeTraverseOrder(i);
        
        partCache.outcomeNodes[paintNode.originNodeIndex].direction = paintNode.direction;
    }
    
    bool hasMeshError = false;
    MeshCombiner::Mesh *mesh = nullptr;
    
    if (buildSucceed) {
        mesh = new MeshCombiner::Mesh(partCache.vertices, partCache.faces, false);
        if (!mesh->isNull()) {
            if (xMirrored) {
                std::vector<QVector3D> xMirroredVertices;
                std::vector<std::vector<size_t>> xMirroredFaces;
                makeXmirror(partCache.vertices, partCache.faces, &xMirroredVertices, &xMirroredFaces);
                for (size_t i = 0; i < xMirroredVertices.size(); ++i) {
                    const auto &position = xMirroredVertices[i];
                    size_t nodeIndex = 0;
                    const auto &source = nodeMeshBuilder->generatedVerticesSourceNodeIndices()[i];
                    nodeIndex = nodeMeshModifier->nodes()[source].originNodeIndex;
                    const auto &nodeIdString = nodeIndexToIdStringMap[nodeIndex];
                    partCache.outcomeNodeVertices.push_back({position, {mirroredPartIdString, nodeIdString}});
                }
                size_t xMirrorStart = partCache.vertices.size();
                for (const auto &vertex: xMirroredVertices)
                    partCache.vertices.push_back(vertex);
                for (const auto &face: xMirroredFaces) {
                    std::vector<size_t> newFace = face;
                    for (auto &it: newFace)
                        it += xMirrorStart;
                    partCache.faces.push_back(newFace);
                }
                MeshCombiner::Mesh *newMesh = nullptr;
                MeshCombiner::Mesh *xMirroredMesh = new MeshCombiner::Mesh(xMirroredVertices, xMirroredFaces, false);
                if (!xMirroredMesh->isNull()) {
                    newMesh = combineTwoMeshes(*mesh,
                        *xMirroredMesh, MeshCombiner::Method::Union);
                }
                delete xMirroredMesh;
                if (newMesh && !newMesh->isNull()) {
                    delete mesh;
                    mesh = newMesh;
                } else {
                    hasMeshError = true;
                    qDebug() << "Xmirrored mesh generate failed";
                    delete newMesh;
                }
            }
        } else {
            hasMeshError = true;
            qDebug() << "Mesh built is uncombinable";
        }
    } else {
        hasMeshError = true;
        qDebug() << "Mesh build failed";
    }
    
    delete m_partPreviewMeshes[partId];
    m_partPreviewMeshes[partId] = nullptr;
    m_generatedPreviewPartIds.insert(partId);
    
    std::vector<QVector3D> partPreviewVertices;
    QColor partPreviewColor = partColor;
    if (nullptr != mesh) {
        partCache.mesh = new MeshCombiner::Mesh(*mesh);
        mesh->fetch(partPreviewVertices, partCache.previewTriangles);
        partCache.previewVertices = partPreviewVertices;
        partCache.isSucceed = true;
    }
    if (partCache.previewTriangles.empty()) {
        partPreviewVertices = partCache.vertices;
        triangulateFacesWithoutKeepVertices(partPreviewVertices, partCache.faces, partCache.previewTriangles);
        partCache.previewVertices = partPreviewVertices;
        partPreviewColor = Qt::red;
        partCache.isSucceed = false;
    }
    
    trim(&partPreviewVertices, true);
    for (auto &it: partPreviewVertices) {
        it *= 2.0;
    }
    std::vector<QVector3D> partPreviewTriangleNormals;
    for (const auto &face: partCache.previewTriangles) {
        partPreviewTriangleNormals.push_back(QVector3D::normal(
            partPreviewVertices[face[0]],
            partPreviewVertices[face[1]],
            partPreviewVertices[face[2]]
        ));
    }
    std::vector<std::vector<QVector3D>> partPreviewTriangleVertexNormals;
    generateSmoothTriangleVertexNormals(partPreviewVertices,
        partCache.previewTriangles,
        partPreviewTriangleNormals,
        &partPreviewTriangleVertexNormals);
    if (!partCache.previewTriangles.empty()) {
        if (target == PartTarget::CutFace)
            partPreviewColor = Theme::red;
        m_partPreviewMeshes[partId] = new MeshLoader(partPreviewVertices,
            partCache.previewTriangles,
            partPreviewTriangleVertexNormals,
            partPreviewColor);
    }
    
    delete nodeMeshBuilder;
    delete nodeMeshModifier;
    
    if (mesh && mesh->isNull()) {
        delete mesh;
        mesh = nullptr;
    }
    
    if (isDisabled) {
        delete mesh;
        mesh = nullptr;
    }
    
    if (target != PartTarget::Model) {
        delete mesh;
        mesh = nullptr;
    }
    
    if (hasMeshError && target == PartTarget::Model) {
        *hasError = true;
        //m_isSucceed = false;
    }
    
    return mesh;
}

const std::map<QString, QString> *MeshGenerator::findComponent(const QString &componentIdString)
{
    const std::map<QString, QString> *component = &m_snapshot->rootComponent;
    if (componentIdString != QUuid().toString()) {
        auto findComponent = m_snapshot->components.find(componentIdString);
        if (findComponent == m_snapshot->components.end()) {
            qDebug() << "Component not found:" << componentIdString;
            return nullptr;
        }
        return &findComponent->second;
    }
    return component;
}

CombineMode MeshGenerator::componentCombineMode(const std::map<QString, QString> *component)
{
    if (nullptr == component)
        return CombineMode::Normal;
    CombineMode combineMode = CombineModeFromString(valueOfKeyInMapOrEmpty(*component, "combineMode").toUtf8().constData());
    if (combineMode == CombineMode::Normal) {
        if (isTrueValueString(valueOfKeyInMapOrEmpty(*component, "inverse")))
            combineMode = CombineMode::Inversion;
        //if (componentRemeshed(component))
        //    combineMode = CombineMode::Uncombined;
        if (combineMode == CombineMode::Normal) {
            if (ComponentLayer::Body != ComponentLayerFromString(valueOfKeyInMapOrEmpty(*component, "layer").toUtf8().constData())) {
                combineMode = CombineMode::Uncombined;
            }
        }
    }
    return combineMode;
}

bool MeshGenerator::componentRemeshed(const std::map<QString, QString> *component, float *polyCountValue)
{
    if (nullptr == component)
        return false;
    bool isCloth = false;
    if (ComponentLayer::Cloth == ComponentLayerFromString(valueOfKeyInMapOrEmpty(*component, "layer").toUtf8().constData())) {
        if (nullptr != polyCountValue)
            *polyCountValue = PolyCountToValue(PolyCount::VeryHighPoly);
        isCloth = true;
    }
    auto polyCount = PolyCountFromString(valueOfKeyInMapOrEmpty(*component, "polyCount").toUtf8().constData());
    if (nullptr != polyCountValue)
        *polyCountValue = PolyCountToValue(polyCount);
    if (isCloth)
        return true;
    return polyCount != PolyCount::Original;
}

QString MeshGenerator::componentColorName(const std::map<QString, QString> *component)
{
    if (nullptr == component)
        return QString();
    QString linkDataType = valueOfKeyInMapOrEmpty(*component, "linkDataType");
    if ("partId" == linkDataType) {
        QString partIdString = valueOfKeyInMapOrEmpty(*component, "linkData");
        auto findPart = m_snapshot->parts.find(partIdString);
        if (findPart == m_snapshot->parts.end()) {
            qDebug() << "Find part failed:" << partIdString;
            return QString();
        }
        auto &part = findPart->second;
        QString colorSolubility = valueOfKeyInMapOrEmpty(part, "colorSolubility");
        if (!colorSolubility.isEmpty()) {
            return QString("+");
        }
        QString colorName = valueOfKeyInMapOrEmpty(part, "color");
        if (colorName.isEmpty())
            return QString("-");
        return colorName;
    }
    return QString();
}

ComponentLayer MeshGenerator::componentLayer(const std::map<QString, QString> *component)
{
    if (nullptr == component)
        return ComponentLayer::Body;
    return ComponentLayerFromString(valueOfKeyInMapOrEmpty(*component, "layer").toUtf8().constData());
}

float MeshGenerator::componentClothStiffness(const std::map<QString, QString> *component)
{
    if (nullptr == component)
        return Component::defaultClothStiffness;
    auto findClothStiffness = component->find("clothStiffness");
    if (findClothStiffness == component->end())
        return Component::defaultClothStiffness;
    return findClothStiffness->second.toFloat();
}

size_t MeshGenerator::componentClothIteration(const std::map<QString, QString> *component)
{
    if (nullptr == component)
        return Component::defaultClothIteration;
    auto findClothIteration = component->find("clothIteration");
    if (findClothIteration == component->end())
        return Component::defaultClothIteration;
    return findClothIteration->second.toUInt();
}

ClothForce MeshGenerator::componentClothForce(const std::map<QString, QString> *component)
{
    if (nullptr == component)
        return ClothForce::Gravitational;
    return ClothForceFromString(valueOfKeyInMapOrEmpty(*component, "clothForce").toUtf8().constData());
}

float MeshGenerator::componentClothOffset(const std::map<QString, QString> *component)
{
    if (nullptr == component)
        return 0.0f;
    return valueOfKeyInMapOrEmpty(*component, "clothOffset").toFloat();
}

MeshCombiner::Mesh *MeshGenerator::combineComponentMesh(const QString &componentIdString, CombineMode *combineMode)
{
    MeshCombiner::Mesh *mesh = nullptr;
    
    QUuid componentId;
    const std::map<QString, QString> *component = &m_snapshot->rootComponent;
    if (componentIdString != QUuid().toString()) {
        componentId = QUuid(componentIdString);
        auto findComponent = m_snapshot->components.find(componentIdString);
        if (findComponent == m_snapshot->components.end()) {
            qDebug() << "Component not found:" << componentIdString;
            return nullptr;
        }
        component = &findComponent->second;
    }

    *combineMode = componentCombineMode(component);
    
    auto &componentCache = m_cacheContext->components[componentIdString];
    
    if (m_cacheEnabled) {
        if (m_dirtyComponentIds.find(componentIdString) == m_dirtyComponentIds.end()) {
            if (nullptr != componentCache.mesh)
                return new MeshCombiner::Mesh(*componentCache.mesh);
        }
    }
    
    componentCache.sharedQuadEdges.clear();
    componentCache.noneSeamVertices.clear();
    componentCache.outcomeNodes.clear();
    componentCache.outcomeEdges.clear();
    componentCache.outcomeNodeVertices.clear();
    componentCache.outcomePaintMaps.clear();
    delete componentCache.mesh;
    componentCache.mesh = nullptr;
    
    QString linkDataType = valueOfKeyInMapOrEmpty(*component, "linkDataType");
    if ("partId" == linkDataType) {
        QString partIdString = valueOfKeyInMapOrEmpty(*component, "linkData");
        bool hasError = false;
        mesh = combinePartMesh(partIdString, &hasError);
        if (hasError) {
            delete mesh;
            hasError = false;
            qDebug() << "Try combine part again without adding intermediate nodes";
            mesh = combinePartMesh(partIdString, &hasError, false);
            if (hasError) {
                m_isSucceed = false;
            }
        }
        
        const auto &partCache = m_cacheContext->parts[partIdString];
        for (const auto &vertex: partCache.vertices)
            componentCache.noneSeamVertices.insert(vertex);
        collectSharedQuadEdges(partCache.vertices, partCache.faces, &componentCache.sharedQuadEdges);
        for (const auto &it: partCache.outcomeNodes)
            componentCache.outcomeNodes.push_back(it);
        for (const auto &it: partCache.outcomeEdges)
            componentCache.outcomeEdges.push_back(it);
        for (const auto &it: partCache.outcomeNodeVertices)
            componentCache.outcomeNodeVertices.push_back(it);
        componentCache.outcomePaintMaps.push_back(partCache.outcomePaintMap);
    } else {
        std::vector<std::pair<CombineMode, std::vector<std::pair<QString, QString>>>> combineGroups;
        // Firstly, group by combine mode
        int currentGroupIndex = -1;
        auto lastCombineMode = CombineMode::Count;
        bool foundColorSolubilitySetting = false;
        for (const auto &childIdString: valueOfKeyInMapOrEmpty(*component, "children").split(",")) {
            if (childIdString.isEmpty())
                continue;
            const auto &child = findComponent(childIdString);
            QString colorName = componentColorName(child);
            if (colorName == "+") {
                foundColorSolubilitySetting = true;
            }
            auto combineMode = componentCombineMode(child);
            if (lastCombineMode != combineMode || lastCombineMode == CombineMode::Inversion) {
                qDebug() << "New group[" << currentGroupIndex << "] for combine mode[" << CombineModeToString(combineMode) << "]";
                combineGroups.push_back({combineMode, {}});
                ++currentGroupIndex;
                lastCombineMode = combineMode;
            }
            if (-1 == currentGroupIndex) {
                qDebug() << "Should not happen: -1 == currentGroupIndex";
                continue;
            }
            combineGroups[currentGroupIndex].second.push_back({childIdString, colorName});
        }
        // Secondly, sub group by color
        std::vector<std::tuple<MeshCombiner::Mesh *, CombineMode, QString>> groupMeshes;
        for (const auto &group: combineGroups) {
            std::set<size_t> used;
            std::vector<std::vector<QString>> componentIdStrings;
            int currentSubGroupIndex = -1;
            auto lastColorName = QString();
            for (size_t i = 0; i < group.second.size(); ++i) {
                if (used.find(i) != used.end())
                    continue;
                const auto &colorName = group.second[i].second;
                if (lastColorName != colorName || lastColorName.isEmpty()) {
                    //qDebug() << "New sub group[" << currentSubGroupIndex << "] for color[" << colorName << "]";
                    componentIdStrings.push_back({});
                    ++currentSubGroupIndex;
                    lastColorName = colorName;
                }
                if (-1 == currentSubGroupIndex) {
                    qDebug() << "Should not happen: -1 == currentSubGroupIndex";
                    continue;
                }
                used.insert(i);
                componentIdStrings[currentSubGroupIndex].push_back(group.second[i].first);
                if (colorName.isEmpty())
                    continue;
                for (size_t j = i + 1; j < group.second.size(); ++j) {
                    if (used.find(j) != used.end())
                        continue;
                    const auto &otherColorName = group.second[j].second;
                    if (otherColorName.isEmpty())
                        continue;
                    if (otherColorName != colorName)
                        continue;
                    used.insert(j);
                    componentIdStrings[currentSubGroupIndex].push_back(group.second[j].first);
                }
            }
            std::vector<std::tuple<MeshCombiner::Mesh *, CombineMode, QString>> multipleMeshes;
            QStringList subGroupMeshIdStringList;
            for (const auto &it: componentIdStrings) {
                QStringList componentChildGroupIdStringList;
                for (const auto &componentChildGroupIdString: it)
                    componentChildGroupIdStringList += componentChildGroupIdString;
                MeshCombiner::Mesh *childMesh = combineComponentChildGroupMesh(it, componentCache);
                if (nullptr == childMesh)
                    continue;
                if (childMesh->isNull()) {
                    delete childMesh;
                    continue;
                }
                QString componentChildGroupIdStringListString = componentChildGroupIdStringList.join("|");
                subGroupMeshIdStringList += componentChildGroupIdStringListString;
                multipleMeshes.push_back(std::make_tuple(childMesh, CombineMode::Normal, componentChildGroupIdStringListString));
            }
            MeshCombiner::Mesh *subGroupMesh = combineMultipleMeshes(multipleMeshes, true/*foundColorSolubilitySetting*/);
            if (nullptr == subGroupMesh)
                continue;
            groupMeshes.push_back(std::make_tuple(subGroupMesh, group.first, subGroupMeshIdStringList.join("&")));
        }
        mesh = combineMultipleMeshes(groupMeshes, true);
    }
    
    if (nullptr != mesh)
        componentCache.mesh = new MeshCombiner::Mesh(*mesh);
    
    if (nullptr != mesh && mesh->isNull()) {
        delete mesh;
        mesh = nullptr;
    }
    
    if (componentId.isNull()) {
        // Prepare cloth collision shap
        if (nullptr != mesh && !mesh->isNull()) {
            m_clothCollisionVertices.clear();
            m_clothCollisionTriangles.clear();
            mesh->fetch(m_clothCollisionVertices, m_clothCollisionTriangles);
        } else {
            // TODO: when no body is valid, may add ground plane as collision shape
            // ... ...
        }
    }
    
    if (nullptr != mesh) {
        float polyCountValue = 1.0f;
        bool remeshed = componentId.isNull() ? componentRemeshed(&m_snapshot->canvas, &polyCountValue) : componentRemeshed(component, &polyCountValue);
        if (remeshed) {
            std::vector<QVector3D> combinedVertices;
            std::vector<std::vector<size_t>> combinedFaces;
            mesh->fetch(combinedVertices, combinedFaces);
            std::vector<QVector3D> newVertices;
            std::vector<std::vector<size_t>> newQuads;
            std::vector<std::vector<size_t>> newTriangles;
            std::vector<std::tuple<QVector3D, float, size_t>> interpolatedNodes;
            Outcome::buildInterpolatedNodes(componentCache.outcomeNodes,
                componentCache.outcomeEdges,
                &interpolatedNodes);
            remesh(componentCache.outcomeNodes,
                interpolatedNodes,
                combinedVertices,
                combinedFaces,
                polyCountValue,
                &newVertices,
                &newQuads,
                &newTriangles,
                &componentCache.outcomeNodeVertices);
            componentCache.sharedQuadEdges.clear();
            for (const auto &face: newQuads) {
                if (face.size() != 4)
                    continue;
                componentCache.sharedQuadEdges.insert({
                    PositionKey(newVertices[face[0]]),
                    PositionKey(newVertices[face[2]])
                });
                componentCache.sharedQuadEdges.insert({
                    PositionKey(newVertices[face[1]]),
                    PositionKey(newVertices[face[3]])
                });
            }
            delete mesh;
            bool disableSelfIntersectionTest = componentId.isNull() ||
                CombineMode::Uncombined == componentCombineMode(component);
            mesh = new MeshCombiner::Mesh(newVertices, newTriangles, disableSelfIntersectionTest);
            if (nullptr != mesh) {
                if (!disableSelfIntersectionTest) {
                    if (mesh->isNull()) {
                        delete mesh;
                        mesh = nullptr;
                    }
                }
                delete componentCache.mesh;
                componentCache.mesh = nullptr;
                if (nullptr != mesh)
                    componentCache.mesh = new MeshCombiner::Mesh(*mesh);
            }
        }
    }
    
    return mesh;
}

MeshCombiner::Mesh *MeshGenerator::combineMultipleMeshes(const std::vector<std::tuple<MeshCombiner::Mesh *, CombineMode, QString>> &multipleMeshes, bool recombine)
{
    MeshCombiner::Mesh *mesh = nullptr;
    QString meshIdStrings;
    for (const auto &it: multipleMeshes) {
        const auto &childCombineMode = std::get<1>(it);
        MeshCombiner::Mesh *subMesh = std::get<0>(it);
        const QString &subMeshIdString = std::get<2>(it);
        //qDebug() << "Combine mode:" << CombineModeToString(childCombineMode);
        if (nullptr == subMesh) {
            qDebug() << "Child mesh is null";
            continue;
        }
        if (subMesh->isNull()) {
            qDebug() << "Child mesh is uncombinable";
            delete subMesh;
            continue;
        }
        if (nullptr == mesh) {
            mesh = subMesh;
            meshIdStrings = subMeshIdString;
        } else {
            auto combinerMethod = childCombineMode == CombineMode::Inversion ?
                    MeshCombiner::Method::Diff : MeshCombiner::Method::Union;
            auto combinerMethodString = combinerMethod == MeshCombiner::Method::Union ?
                "+" : "-";
            meshIdStrings += combinerMethodString + subMeshIdString;
            if (recombine)
                meshIdStrings += "!";
            MeshCombiner::Mesh *newMesh = nullptr;
            auto findCached = m_cacheContext->cachedCombination.find(meshIdStrings);
            if (findCached != m_cacheContext->cachedCombination.end()) {
                if (nullptr != findCached->second) {
                    //qDebug() << "Use cached combination:" << meshIdStrings;
                    newMesh = new MeshCombiner::Mesh(*findCached->second);
                }
            } else {
                newMesh = combineTwoMeshes(*mesh,
                    *subMesh,
                    combinerMethod,
                    recombine);
                delete subMesh;
                if (nullptr != newMesh)
                    m_cacheContext->cachedCombination.insert({meshIdStrings, new MeshCombiner::Mesh(*newMesh)});
                else
                    m_cacheContext->cachedCombination.insert({meshIdStrings, nullptr});
                //qDebug() << "Add cached combination:" << meshIdStrings;
            }
            if (newMesh && !newMesh->isNull()) {
                delete mesh;
                mesh = newMesh;
            } else {
                m_isSucceed = false;
                qDebug() << "Mesh combine failed";
                delete newMesh;
            }
        }
    }
    if (nullptr != mesh && mesh->isNull()) {
        delete mesh;
        mesh = nullptr;
    }
    return mesh;
}

MeshCombiner::Mesh *MeshGenerator::combineComponentChildGroupMesh(const std::vector<QString> &componentIdStrings, GeneratedComponent &componentCache)
{
    std::vector<std::tuple<MeshCombiner::Mesh *, CombineMode, QString>> multipleMeshes;
    for (const auto &childIdString: componentIdStrings) {
        CombineMode childCombineMode = CombineMode::Normal;
        MeshCombiner::Mesh *subMesh = combineComponentMesh(childIdString, &childCombineMode);
        
        if (CombineMode::Uncombined == childCombineMode) {
            delete subMesh;
            continue;
        }
        
        const auto &childComponentCache = m_cacheContext->components[childIdString];
        for (const auto &vertex: childComponentCache.noneSeamVertices)
            componentCache.noneSeamVertices.insert(vertex);
        for (const auto &it: childComponentCache.sharedQuadEdges)
            componentCache.sharedQuadEdges.insert(it);
        for (const auto &it: childComponentCache.outcomeNodes)
            componentCache.outcomeNodes.push_back(it);
        for (const auto &it: childComponentCache.outcomeEdges)
            componentCache.outcomeEdges.push_back(it);
        for (const auto &it: childComponentCache.outcomeNodeVertices)
            componentCache.outcomeNodeVertices.push_back(it);
        for (const auto &it: childComponentCache.outcomePaintMaps)
            componentCache.outcomePaintMaps.push_back(it);
        
        if (nullptr == subMesh || subMesh->isNull()) {
            delete subMesh;
            continue;
        }
    
        multipleMeshes.push_back(std::make_tuple(subMesh, childCombineMode, childIdString));
    }
    return combineMultipleMeshes(multipleMeshes);
}

MeshCombiner::Mesh *MeshGenerator::combineTwoMeshes(const MeshCombiner::Mesh &first, const MeshCombiner::Mesh &second,
    MeshCombiner::Method method,
    bool recombine)
{
    if (first.isNull() || second.isNull())
        return nullptr;
    std::vector<std::pair<MeshCombiner::Source, size_t>> combinedVerticesSources;
    MeshCombiner::Mesh *newMesh = MeshCombiner::combine(first,
        second,
        method,
        &combinedVerticesSources);
    if (nullptr == newMesh)
        return nullptr;
    if (!newMesh->isNull() && recombine) {
        MeshRecombiner recombiner;
        std::vector<QVector3D> combinedVertices;
        std::vector<std::vector<size_t>> combinedFaces;
        newMesh->fetch(combinedVertices, combinedFaces);
        recombiner.setVertices(&combinedVertices, &combinedVerticesSources);
        recombiner.setFaces(&combinedFaces);
        if (recombiner.recombine()) {
            if (isManifold(recombiner.regeneratedFaces())) {
                MeshCombiner::Mesh *reMesh = new MeshCombiner::Mesh(recombiner.regeneratedVertices(), recombiner.regeneratedFaces(), false);
                if (!reMesh->isNull() && !reMesh->isSelfIntersected()) {
                    delete newMesh;
                    newMesh = reMesh;
                } else {
                    delete reMesh;
                }
            }
        }
    }
    if (newMesh->isNull()) {
        delete newMesh;
        return nullptr;
    }
    return newMesh;
}

void MeshGenerator::makeXmirror(const std::vector<QVector3D> &sourceVertices, const std::vector<std::vector<size_t>> &sourceFaces,
        std::vector<QVector3D> *destVertices, std::vector<std::vector<size_t>> *destFaces)
{
    for (const auto &mirrorFrom: sourceVertices) {
        destVertices->push_back(QVector3D(-mirrorFrom.x(), mirrorFrom.y(), mirrorFrom.z()));
    }
    std::vector<std::vector<size_t>> newFaces;
    for (const auto &mirrorFrom: sourceFaces) {
        auto newFace = mirrorFrom;
        std::reverse(newFace.begin(), newFace.end());
        destFaces->push_back(newFace);
    }
}

void MeshGenerator::collectSharedQuadEdges(const std::vector<QVector3D> &vertices, const std::vector<std::vector<size_t>> &faces,
        std::set<std::pair<PositionKey, PositionKey>> *sharedQuadEdges)
{
    for (const auto &face: faces) {
        if (face.size() != 4)
            continue;
        sharedQuadEdges->insert({
            PositionKey(vertices[face[0]]),
            PositionKey(vertices[face[2]])
        });
        sharedQuadEdges->insert({
            PositionKey(vertices[face[1]]),
            PositionKey(vertices[face[3]])
        });
    }
}

void MeshGenerator::setGeneratedCacheContext(GeneratedCacheContext *cacheContext)
{
    m_cacheContext = cacheContext;
}

void MeshGenerator::setSmoothShadingThresholdAngleDegrees(float degrees)
{
    m_smoothShadingThresholdAngleDegrees = degrees;
}

void MeshGenerator::process()
{
    generate();
    
    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}

void MeshGenerator::generate()
{
    if (nullptr == m_snapshot)
        return;
    
    m_isSucceed = true;
    
    QElapsedTimer countTimeConsumed;
    countTimeConsumed.start();
    
    m_outcome = new Outcome;
    m_outcome->meshId = m_id;
    //m_cutFaceTransforms = new std::map<QUuid, nodemesh::Builder::CutFaceTransform>;
    //m_nodesCutFaces = new std::map<QUuid, std::map<QString, QVector2D>>;
    
    bool needDeleteCacheContext = false;
    if (nullptr == m_cacheContext) {
        m_cacheContext = new GeneratedCacheContext;
        needDeleteCacheContext = true;
    } else {
        m_cacheEnabled = true;
        for (auto it = m_cacheContext->parts.begin(); it != m_cacheContext->parts.end(); ) {
            if (m_snapshot->parts.find(it->first) == m_snapshot->parts.end()) {
                auto mirrorFrom = m_cacheContext->partMirrorIdMap.find(it->first);
                if (mirrorFrom != m_cacheContext->partMirrorIdMap.end()) {
                    if (m_snapshot->parts.find(mirrorFrom->second) != m_snapshot->parts.end()) {
                        it++;
                        continue;
                    }
                    m_cacheContext->partMirrorIdMap.erase(mirrorFrom);
                }
                it = m_cacheContext->parts.erase(it);
                continue;
            }
            it++;
        }
        for (auto it = m_cacheContext->components.begin(); it != m_cacheContext->components.end(); ) {
            if (m_snapshot->components.find(it->first) == m_snapshot->components.end()) {
                for (auto combinationIt = m_cacheContext->cachedCombination.begin(); combinationIt != m_cacheContext->cachedCombination.end(); ) {
                    if (-1 != combinationIt->first.indexOf(it->first)) {
                        //qDebug() << "Removed cached combination:" << combinationIt->first;
                        delete combinationIt->second;
                        combinationIt = m_cacheContext->cachedCombination.erase(combinationIt);
                        continue;
                    }
                    combinationIt++;
                }
                it = m_cacheContext->components.erase(it);
                continue;
            }
            it++;
        }
    }
    
    collectParts();
    checkDirtyFlags();
    
    for (const auto &dirtyComponentId: m_dirtyComponentIds) {
        for (auto combinationIt = m_cacheContext->cachedCombination.begin(); combinationIt != m_cacheContext->cachedCombination.end(); ) {
            if (-1 != combinationIt->first.indexOf(dirtyComponentId)) {
                //qDebug() << "Removed dirty cached combination:" << combinationIt->first;
                delete combinationIt->second;
                combinationIt = m_cacheContext->cachedCombination.erase(combinationIt);
                continue;
            }
            combinationIt++;
        }
    }
    
    m_dirtyComponentIds.insert(QUuid().toString());
    
    m_mainProfileMiddleX = valueOfKeyInMapOrEmpty(m_snapshot->canvas, "originX").toFloat();
    m_mainProfileMiddleY = valueOfKeyInMapOrEmpty(m_snapshot->canvas, "originY").toFloat();
    m_sideProfileMiddleX = valueOfKeyInMapOrEmpty(m_snapshot->canvas, "originZ").toFloat();
    
    bool remeshed = componentRemeshed(&m_snapshot->canvas);
    
    CombineMode combineMode;
    auto combinedMesh = combineComponentMesh(QUuid().toString(), &combineMode);
    
    const auto &componentCache = m_cacheContext->components[QUuid().toString()];
    
    std::vector<QVector3D> combinedVertices;
    std::vector<std::vector<size_t>> combinedFaces;
    if (nullptr != combinedMesh) {
        combinedMesh->fetch(combinedVertices, combinedFaces);
        
        if (!remeshed) {
            size_t totalAffectedNum = 0;
            size_t affectedNum = 0;
            do {
                std::vector<QVector3D> weldedVertices;
                std::vector<std::vector<size_t>> weldedFaces;
                affectedNum = weldSeam(combinedVertices, combinedFaces,
                    0.025, componentCache.noneSeamVertices,
                    weldedVertices, weldedFaces);
                combinedVertices = weldedVertices;
                combinedFaces = weldedFaces;
                totalAffectedNum += affectedNum;
            } while (affectedNum > 0);
            qDebug() << "Total weld affected triangles:" << totalAffectedNum;
        }
        
        m_outcome->nodes = componentCache.outcomeNodes;
        m_outcome->edges = componentCache.outcomeEdges;
        m_outcome->paintMaps = componentCache.outcomePaintMaps;
        recoverQuads(combinedVertices, combinedFaces, componentCache.sharedQuadEdges, m_outcome->triangleAndQuads);
            m_outcome->nodeVertices = componentCache.outcomeNodeVertices;
            m_outcome->vertices = combinedVertices;
            m_outcome->triangles = combinedFaces;
    }
    
    // Recursively check uncombined components
    collectUncombinedComponent(QUuid().toString());
    
    // Fetch nodes as body nodes before cloth nodes collecting
    std::set<std::pair<QUuid, QUuid>> bodyNodeMap;
    m_outcome->bodyNodes.reserve(m_outcome->nodes.size());
    for (const auto &it: m_outcome->nodes) {
        if (it.joined) {
            bodyNodeMap.insert({it.partId, it.nodeId});
            m_outcome->bodyNodes.push_back(it);
        }
    }
    m_outcome->bodyEdges.reserve(m_outcome->edges.size());
    for (const auto &it: m_outcome->edges) {
        if (bodyNodeMap.find(it.first) == bodyNodeMap.end())
            continue;
        if (bodyNodeMap.find(it.second) == bodyNodeMap.end())
            continue;
        m_outcome->bodyEdges.push_back(it);
    }
    
    collectClothComponent(QUuid().toString());
    
    // Collect errored parts
    for (const auto &it: m_cacheContext->parts) {
        if (!it.second.isSucceed) {
            if (!it.second.joined)
                continue;
            
            auto updateVertexIndices = [=](std::vector<std::vector<size_t>> &faces, size_t vertexStartIndex) {
                for (auto &it: faces) {
                    for (auto &subIt: it)
                        subIt += vertexStartIndex;
                }
            };
            
            auto errorTriangleAndQuads = it.second.faces;
            updateVertexIndices(errorTriangleAndQuads, m_outcome->vertices.size());
            m_outcome->vertices.insert(m_outcome->vertices.end(), it.second.vertices.begin(), it.second.vertices.end());
            m_outcome->triangleAndQuads.insert(m_outcome->triangleAndQuads.end(), errorTriangleAndQuads.begin(), errorTriangleAndQuads.end());
            
            auto errorTriangles = it.second.previewTriangles;
            updateVertexIndices(errorTriangles, m_outcome->vertices.size());
            m_outcome->vertices.insert(m_outcome->vertices.end(), it.second.previewVertices.begin(), it.second.previewVertices.end());
            m_outcome->triangles.insert(m_outcome->triangles.end(), errorTriangles.begin(), errorTriangles.end());
        }
    }
    
    auto postprocessOutcome = [this](Outcome *outcome) {
        std::vector<QVector3D> combinedFacesNormals;
        for (const auto &face: outcome->triangles) {
            combinedFacesNormals.push_back(QVector3D::normal(
                outcome->vertices[face[0]],
                outcome->vertices[face[1]],
                outcome->vertices[face[2]]
            ));
        }
        
        outcome->triangleNormals = combinedFacesNormals;
        
        std::vector<std::pair<QUuid, QUuid>> sourceNodes;
        triangleSourceNodeResolve(*outcome, sourceNodes, &outcome->vertexSourceNodes);
        {
            for (const auto &it: outcome->vertexSourceNodes) {
                if (!it.first.isNull() && !it.second.isNull())
                    continue;
                std::cout << "source node:" << it.first.toString().toUtf8().constData() << " " << it.second.toString().toUtf8().constData() << std::endl;
            }
        }
        outcome->setTriangleSourceNodes(sourceNodes);
        
        std::map<std::pair<QUuid, QUuid>, QColor> sourceNodeToColorMap;
        for (const auto &node: outcome->nodes)
            sourceNodeToColorMap.insert({{node.partId, node.nodeId}, node.color});
        
        outcome->triangleColors.resize(outcome->triangles.size(), Qt::white);
        const std::vector<std::pair<QUuid, QUuid>> *triangleSourceNodes = outcome->triangleSourceNodes();
        if (nullptr != triangleSourceNodes) {
            for (size_t triangleIndex = 0; triangleIndex < outcome->triangles.size(); triangleIndex++) {
                const auto &source = (*triangleSourceNodes)[triangleIndex];
                outcome->triangleColors[triangleIndex] = sourceNodeToColorMap[source];
            }
        }
        
        std::vector<std::vector<QVector3D>> triangleVertexNormals;
        generateSmoothTriangleVertexNormals(outcome->vertices,
            outcome->triangles,
            outcome->triangleNormals,
            &triangleVertexNormals);
        outcome->setTriangleVertexNormals(triangleVertexNormals);
    };
    
    postprocessOutcome(m_outcome);
    
    m_resultMesh = new MeshLoader(*m_outcome);
    
    delete combinedMesh;

    if (needDeleteCacheContext) {
        delete m_cacheContext;
        m_cacheContext = nullptr;
    }
    
    qDebug() << "The mesh generation took" << countTimeConsumed.elapsed() << "milliseconds";
}

void MeshGenerator::remesh(const std::vector<OutcomeNode> &inputNodes,
        const std::vector<std::tuple<QVector3D, float, size_t>> &interpolatedNodes,
        const std::vector<QVector3D> &inputVertices,
        const std::vector<std::vector<size_t>> &inputFaces,
        float targetVertexMultiplyFactor,
        std::vector<QVector3D> *outputVertices,
        std::vector<std::vector<size_t>> *outputQuads,
        std::vector<std::vector<size_t>> *outputTriangles,
        std::vector<std::pair<QVector3D, std::pair<QUuid, QUuid>>> *outputNodeVertices)
{
    std::vector<std::pair<QVector3D, float>> nodes;
    std::vector<std::pair<QUuid, QUuid>> sourceIds;
    nodes.reserve(interpolatedNodes.size());
    sourceIds.reserve(interpolatedNodes.size());
    for (const auto &it: interpolatedNodes) {
        nodes.push_back(std::make_pair(std::get<0>(it), std::get<1>(it)));
        const auto &sourceNode = inputNodes[std::get<2>(it)];
        sourceIds.push_back(std::make_pair(sourceNode.partId, sourceNode.nodeId));
    }
    Remesher remesher;
    remesher.setMesh(inputVertices, inputFaces);
    remesher.setNodes(nodes, sourceIds);
    remesher.remesh(targetVertexMultiplyFactor);
    *outputVertices = remesher.getRemeshedVertices();
    const auto &remeshedFaces = remesher.getRemeshedFaces();
    *outputQuads = remeshedFaces;
    outputTriangles->clear();
    outputTriangles->reserve(remeshedFaces.size() * 2);
    for (const auto &it: remeshedFaces) {
        outputTriangles->push_back(std::vector<size_t> {
            it[0], it[1], it[2]
        });
        if (4 == it.size()) {
            outputTriangles->push_back(std::vector<size_t> {
                it[2], it[3], it[0]
            });
        }
    }
    const auto &remeshedVertexSources = remesher.getRemeshedVertexSources();
    outputNodeVertices->clear();
    outputNodeVertices->reserve(outputVertices->size());
    for (size_t i = 0; i < outputVertices->size(); ++i) {
        const auto &vertexSource = remeshedVertexSources[i];
        if (vertexSource.first.isNull())
            continue;
        outputNodeVertices->push_back(std::make_pair((*outputVertices)[i], vertexSource));
    }
}

void MeshGenerator::collectUncombinedComponent(const QString &componentIdString)
{
    const auto &component = findComponent(componentIdString);
    if (CombineMode::Uncombined == componentCombineMode(component)) {
        if (ComponentLayer::Body != componentLayer(component))
            return;
        const auto &componentCache = m_cacheContext->components[componentIdString];
        if (nullptr == componentCache.mesh || componentCache.mesh->isNull()) {
            qDebug() << "Uncombined mesh is null";
            return;
        }
        
        m_outcome->nodes.insert(m_outcome->nodes.end(), componentCache.outcomeNodes.begin(), componentCache.outcomeNodes.end());
        m_outcome->edges.insert(m_outcome->edges.end(), componentCache.outcomeEdges.begin(), componentCache.outcomeEdges.end());
        m_outcome->nodeVertices.insert(m_outcome->nodeVertices.end(), componentCache.outcomeNodeVertices.begin(), componentCache.outcomeNodeVertices.end());
        m_outcome->paintMaps.insert(m_outcome->paintMaps.end(), componentCache.outcomePaintMaps.begin(), componentCache.outcomePaintMaps.end());
        
        std::vector<QVector3D> uncombinedVertices;
        std::vector<std::vector<size_t>> uncombinedFaces;
        componentCache.mesh->fetch(uncombinedVertices, uncombinedFaces);
        std::vector<std::vector<size_t>> uncombinedTriangleAndQuads;
        
        recoverQuads(uncombinedVertices, uncombinedFaces, componentCache.sharedQuadEdges, uncombinedTriangleAndQuads);
        
        auto vertexStartIndex = m_outcome->vertices.size();
        auto updateVertexIndices = [=](std::vector<std::vector<size_t>> &faces) {
            for (auto &it: faces) {
                for (auto &subIt: it)
                    subIt += vertexStartIndex;
            }
        };
        updateVertexIndices(uncombinedFaces);
        updateVertexIndices(uncombinedTriangleAndQuads);
        
        m_outcome->vertices.insert(m_outcome->vertices.end(), uncombinedVertices.begin(), uncombinedVertices.end());
        m_outcome->triangles.insert(m_outcome->triangles.end(), uncombinedFaces.begin(), uncombinedFaces.end());
        m_outcome->triangleAndQuads.insert(m_outcome->triangleAndQuads.end(), uncombinedTriangleAndQuads.begin(), uncombinedTriangleAndQuads.end());
        return;
    }
    for (const auto &childIdString: valueOfKeyInMapOrEmpty(*component, "children").split(",")) {
        if (childIdString.isEmpty())
            continue;
        collectUncombinedComponent(childIdString);
    }
}

void MeshGenerator::collectClothComponentIdStrings(const QString &componentIdString,
        std::vector<QString> *componentIdStrings)
{
    const auto &component = findComponent(componentIdString);
    if (ComponentLayer::Cloth == componentLayer(component)) {
        const auto &componentCache = m_cacheContext->components[componentIdString];
        if (nullptr == componentCache.mesh) {
            return;
        }
        componentIdStrings->push_back(componentIdString);
        return;
    }
    for (const auto &childIdString: valueOfKeyInMapOrEmpty(*component, "children").split(",")) {
        if (childIdString.isEmpty())
            continue;
        collectClothComponentIdStrings(childIdString, componentIdStrings);
    }
}

void MeshGenerator::collectClothComponent(const QString &componentIdString)
{
    if (m_clothCollisionTriangles.empty())
        return;
    
    std::vector<QString> componentIdStrings;
    collectClothComponentIdStrings(componentIdString, &componentIdStrings);
    
    std::vector<ClothMesh> clothMeshes(componentIdStrings.size());
    for (size_t i = 0; i < componentIdStrings.size(); ++i) {
        const auto &componentIdString = componentIdStrings[i];
        const auto &componentCache = m_cacheContext->components[componentIdString];
        if (nullptr == componentCache.mesh) {
            return;
        }
        const auto &component = findComponent(componentIdString);
        auto &clothMesh = clothMeshes[i];
        componentCache.mesh->fetch(clothMesh.vertices, clothMesh.faces);
        clothMesh.clothForce = componentClothForce(component);
        clothMesh.clothOffset = componentClothOffset(component);
        clothMesh.clothStiffness = componentClothStiffness(component);
        clothMesh.clothIteration = componentClothIteration(component);
        clothMesh.outcomeNodeVertices = &componentCache.outcomeNodeVertices;
        m_outcome->clothNodes.insert(m_outcome->clothNodes.end(), componentCache.outcomeNodes.begin(), componentCache.outcomeNodes.end());
        m_outcome->nodes.insert(m_outcome->nodes.end(), componentCache.outcomeNodes.begin(), componentCache.outcomeNodes.end());
        m_outcome->edges.insert(m_outcome->edges.end(), componentCache.outcomeEdges.begin(), componentCache.outcomeEdges.end());
    }
    simulateClothMeshes(&clothMeshes,
        &m_clothCollisionVertices,
        &m_clothCollisionTriangles);
    for (auto &clothMesh: clothMeshes) {
        auto vertexStartIndex = m_outcome->vertices.size();
        auto updateVertexIndices = [=](std::vector<std::vector<size_t>> &faces) {
            for (auto &it: faces) {
                for (auto &subIt: it)
                    subIt += vertexStartIndex;
            }
        };
        updateVertexIndices(clothMesh.faces);
        m_outcome->vertices.insert(m_outcome->vertices.end(), clothMesh.vertices.begin(), clothMesh.vertices.end());
        for (const auto &it: clothMesh.faces) {
            if (4 == it.size()) {
                m_outcome->triangles.push_back(std::vector<size_t> {
                    it[0], it[1], it[2]
                });
                m_outcome->triangles.push_back(std::vector<size_t> {
                    it[2], it[3], it[0]
                });
            } else if (3 == it.size()) {
                m_outcome->triangles.push_back(it);
            }
        }
        m_outcome->triangleAndQuads.insert(m_outcome->triangleAndQuads.end(), clothMesh.faces.begin(), clothMesh.faces.end());
        for (size_t i = 0; i < clothMesh.vertices.size(); ++i) {
            const auto &source = clothMesh.vertexSources[i];
            m_outcome->nodeVertices.push_back(std::make_pair(clothMesh.vertices[i], source));
        }
    }
}

void MeshGenerator::generateSmoothTriangleVertexNormals(const std::vector<QVector3D> &vertices, const std::vector<std::vector<size_t>> &triangles,
    const std::vector<QVector3D> &triangleNormals,
    std::vector<std::vector<QVector3D>> *triangleVertexNormals)
{
    std::vector<QVector3D> smoothNormals;
    angleSmooth(vertices,
        triangles,
        triangleNormals,
        m_smoothShadingThresholdAngleDegrees,
        smoothNormals);
    triangleVertexNormals->resize(triangles.size(), {
        QVector3D(), QVector3D(), QVector3D()
    });
    size_t index = 0;
    for (size_t i = 0; i < triangles.size(); ++i) {
        auto &normals = (*triangleVertexNormals)[i];
        for (size_t j = 0; j < 3; ++j) {
            if (index < smoothNormals.size())
                normals[j] = smoothNormals[index];
            ++index;
        }
    }
}

void MeshGenerator::setDefaultPartColor(const QColor &color)
{
    m_defaultPartColor = color;
}
