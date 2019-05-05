#include <QDebug>
#include <QElapsedTimer>
#include <QVector2D>
#include <QGuiApplication>
#include <QMatrix4x4>
#include <nodemesh/builder.h>
#include <nodemesh/modifier.h>
#include <nodemesh/misc.h>
#include <nodemesh/recombiner.h>
#include "meshgenerator.h"
#include "util.h"
#include "trianglesourcenoderesolve.h"
#include "cutface.h"

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

nodemesh::Combiner::Mesh *MeshGenerator::combinePartMesh(const QString &partIdString)
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
    QString colorString = valueOfKeyInMapOrEmpty(part, "color");
    QColor partColor = colorString.isEmpty() ? m_defaultPartColor : QColor(colorString);
    float deformThickness = 1.0;
    float deformWidth = 1.0;
    float cutRotation = 0.0;
    
    CutFace cutFace = CutFaceFromString(valueOfKeyInMapOrEmpty(part, "cutFace").toUtf8().constData());
    std::vector<QVector2D> cutTemplate = CutFaceToPoints(cutFace);
    if (chamfered)
        nodemesh::chamferFace2D(&cutTemplate);
    //normalizeCutFacePoints(&cutTemplate);
    QString cutRotationString = valueOfKeyInMapOrEmpty(part, "cutRotation");
    if (!cutRotationString.isEmpty()) {
        cutRotation = cutRotationString.toFloat();
    }
    
    QString thicknessString = valueOfKeyInMapOrEmpty(part, "deformThickness");
    if (!thicknessString.isEmpty()) {
        deformThickness = thicknessString.toFloat();
    }
    
    QString widthString = valueOfKeyInMapOrEmpty(part, "deformWidth");
    if (!widthString.isEmpty()) {
        deformWidth = widthString.toFloat();
    }
    
    QUuid materialId;
    QString materialIdString = valueOfKeyInMapOrEmpty(part, "materialId");
    if (!materialIdString.isEmpty())
        materialId = QUuid(materialIdString);
    
    auto &partCache = m_cacheContext->parts[partIdString];
    partCache.outcomeNodes.clear();
    partCache.outcomeNodeVertices.clear();
    partCache.vertices.clear();
    partCache.faces.clear();
    partCache.previewTriangles.clear();
    partCache.isSucceed = false;
    delete partCache.mesh;
    partCache.mesh = nullptr;
    
    struct NodeInfo
    {
        float radius = 0;
        QVector3D position;
        BoneMark boneMark = BoneMark::None;
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
        
        auto &nodeInfo = nodeInfos[nodeIdString];
        nodeInfo.position = QVector3D(x, y, z);
        nodeInfo.radius = radius;
        nodeInfo.boneMark = boneMark;
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
    
    std::map<QString, int> nodeIdStringToIndexMap;
    std::map<int, QString> nodeIndexToIdStringMap;
    
    nodemesh::Modifier *modifier = new nodemesh::Modifier;
    
    QString mirroredPartIdString;
    QUuid mirroredPartId;
    if (xMirrored) {
        mirroredPartId = QUuid().createUuid();
        mirroredPartIdString = mirroredPartId.toString();
        m_cacheContext->partMirrorIdMap[mirroredPartIdString] = partIdString;
    }
    
    for (const auto &nodeIt: nodeInfos) {
        const auto &nodeIdString = nodeIt.first;
        const auto &nodeInfo = nodeIt.second;
        size_t nodeIndex = modifier->addNode(nodeInfo.position, nodeInfo.radius, cutTemplate);
        nodeIdStringToIndexMap[nodeIdString] = nodeIndex;
        nodeIndexToIdStringMap[nodeIndex] = nodeIdString;
        
        OutcomeNode outcomeNode;
        outcomeNode.partId = QUuid(partIdString);
        outcomeNode.nodeId = QUuid(nodeIdString);
        outcomeNode.origin = nodeInfo.position;
        outcomeNode.radius = nodeInfo.radius;
        outcomeNode.color = partColor;
        outcomeNode.materialId = materialId;
        outcomeNode.boneMark = nodeInfo.boneMark;
        outcomeNode.mirroredByPartId = mirroredPartIdString;
        partCache.outcomeNodes.push_back(outcomeNode);
        if (xMirrored) {
            outcomeNode.partId = mirroredPartId;
            outcomeNode.mirrorFromPartId = QUuid(partId);
            outcomeNode.mirroredByPartId = QUuid();
            outcomeNode.origin.setX(-nodeInfo.position.x());
            partCache.outcomeNodes.push_back(outcomeNode);
        }
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
        
        modifier->addEdge(findFromNodeIndex->second, findToNodeIndex->second);
    }
    
    if (subdived)
        modifier->subdivide();
    
    if (rounded)
        modifier->roundEnd();
    
    modifier->finalize();
    
    nodemesh::Builder *builder = new nodemesh::Builder;
    builder->setDeformThickness(deformThickness);
    builder->setDeformWidth(deformWidth);
    builder->setCutRotation(cutRotation);
    
    for (const auto &node: modifier->nodes())
        builder->addNode(node.position, node.radius, node.cutTemplate);
    for (const auto &edge: modifier->edges())
        builder->addEdge(edge.firstNodeIndex, edge.secondNodeIndex);
    bool buildSucceed = builder->build();
    
    partCache.vertices = builder->generatedVertices();
    partCache.faces = builder->generatedFaces();
    for (size_t i = 0; i < partCache.vertices.size(); ++i) {
        const auto &position = partCache.vertices[i];
        const auto &source = builder->generatedVerticesSourceNodeIndices()[i];
        size_t nodeIndex = modifier->nodes()[source].originNodeIndex;
        const auto &nodeIdString = nodeIndexToIdStringMap[nodeIndex];
        partCache.outcomeNodeVertices.push_back({position, {partIdString, nodeIdString}});
    }
    
    nodemesh::Combiner::Mesh *mesh = nullptr;
    
    if (buildSucceed) {
        mesh = new nodemesh::Combiner::Mesh(partCache.vertices, partCache.faces);
        if (!mesh->isNull()) {
            if (xMirrored) {
                std::vector<QVector3D> xMirroredVertices;
                std::vector<std::vector<size_t>> xMirroredFaces;
                makeXmirror(partCache.vertices, partCache.faces, &xMirroredVertices, &xMirroredFaces);
                for (size_t i = 0; i < xMirroredVertices.size(); ++i) {
                    const auto &position = xMirroredVertices[i];
                    const auto &source = builder->generatedVerticesSourceNodeIndices()[i];
                    size_t nodeIndex = modifier->nodes()[source].originNodeIndex;
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
                nodemesh::Combiner::Mesh *xMirroredMesh = new nodemesh::Combiner::Mesh(xMirroredVertices, xMirroredFaces);
                nodemesh::Combiner::Mesh *newMesh = combineTwoMeshes(*mesh,
                    *xMirroredMesh, nodemesh::Combiner::Method::Union);
                delete xMirroredMesh;
                if (newMesh && !newMesh->isNull()) {
                    delete mesh;
                    mesh = newMesh;
                } else {
                    m_isSucceed = false;
                    qDebug() << "Xmirrored mesh generate failed";
                    delete newMesh;
                }
            }
        } else {
            m_isSucceed = false;
            qDebug() << "Mesh built is uncombinable";
        }
    } else {
        m_isSucceed = false;
        qDebug() << "Mesh build failed";
    }
    
    m_partPreviewMeshes[partId] = nullptr;
    m_generatedPreviewPartIds.insert(partId);
    
    std::vector<QVector3D> partPreviewVertices;
    QColor partPreviewColor = partColor;
    if (nullptr != mesh) {
        partCache.mesh = new nodemesh::Combiner::Mesh(*mesh);
        mesh->fetch(partPreviewVertices, partCache.previewTriangles);
        partCache.isSucceed = true;
    }
    if (partCache.previewTriangles.empty()) {
        partPreviewVertices = partCache.vertices;
        nodemesh::triangulate(partPreviewVertices, partCache.faces, partCache.previewTriangles);
        partPreviewColor = Qt::red;
        partCache.isSucceed = false;
    }
    
    nodemesh::trim(&partPreviewVertices, true);
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
        m_partPreviewMeshes[partId] = new MeshLoader(partPreviewVertices,
            partCache.previewTriangles,
            partPreviewTriangleVertexNormals,
            partPreviewColor);
    }
    
    delete builder;
    delete modifier;
    
    if (mesh && mesh->isNull()) {
        delete mesh;
        mesh = nullptr;
    }
    
    if (isDisabled) {
        delete mesh;
        mesh = nullptr;
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
    }
    return combineMode;
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
        QString colorName = valueOfKeyInMapOrEmpty(part, "color");
        if (colorName.isEmpty())
            return QString("-");
        return colorName;
    }
    return QString();
}

nodemesh::Combiner::Mesh *MeshGenerator::combineComponentMesh(const QString &componentIdString, CombineMode *combineMode)
{
    nodemesh::Combiner::Mesh *mesh = nullptr;
    
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
                return new nodemesh::Combiner::Mesh(*componentCache.mesh);
        }
    }
    
    componentCache.sharedQuadEdges.clear();
    componentCache.noneSeamVertices.clear();
    componentCache.outcomeNodes.clear();
    componentCache.outcomeNodeVertices.clear();
    delete componentCache.mesh;
    componentCache.mesh = nullptr;
    
    QString linkDataType = valueOfKeyInMapOrEmpty(*component, "linkDataType");
    if ("partId" == linkDataType) {
        QString partIdString = valueOfKeyInMapOrEmpty(*component, "linkData");
        mesh = combinePartMesh(partIdString);
        
        const auto &partCache = m_cacheContext->parts[partIdString];
        for (const auto &vertex: partCache.vertices)
            componentCache.noneSeamVertices.insert(vertex);
        collectSharedQuadEdges(partCache.vertices, partCache.faces, &componentCache.sharedQuadEdges);
        for (const auto &it: partCache.outcomeNodes)
            componentCache.outcomeNodes.push_back(it);
        for (const auto &it: partCache.outcomeNodeVertices)
            componentCache.outcomeNodeVertices.push_back(it);
    } else {
        std::vector<std::pair<CombineMode, std::vector<std::pair<QString, QString>>>> combineGroups;
        // Firstly, group by combine mode
        int currentGroupIndex = -1;
        auto lastCombineMode = CombineMode::Count;
        for (const auto &childIdString: valueOfKeyInMapOrEmpty(*component, "children").split(",")) {
            if (childIdString.isEmpty())
                continue;
            const auto &child = findComponent(childIdString);
            QString colorName = componentColorName(child);
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
        std::vector<std::pair<nodemesh::Combiner::Mesh *, CombineMode>> groupMeshes;
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
                    qDebug() << "New sub group[" << currentSubGroupIndex << "] for color[" << colorName << "]";
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
            std::vector<std::pair<nodemesh::Combiner::Mesh *, CombineMode>> multipleMeshes;
            for (const auto &it: componentIdStrings) {
                nodemesh::Combiner::Mesh *childMesh = combineComponentChildGroupMesh(it, componentCache);
                multipleMeshes.push_back({childMesh, CombineMode::Normal});
            }
            nodemesh::Combiner::Mesh *subGroupMesh = combineMultipleMeshes(multipleMeshes, false);
            groupMeshes.push_back({subGroupMesh, group.first});
        }
        mesh = combineMultipleMeshes(groupMeshes, false);
    }
    
    if (nullptr != mesh)
        componentCache.mesh = new nodemesh::Combiner::Mesh(*mesh);
    
    if (nullptr != mesh && mesh->isNull()) {
        delete mesh;
        mesh = nullptr;
    }
    
    return mesh;
}

nodemesh::Combiner::Mesh *MeshGenerator::combineMultipleMeshes(const std::vector<std::pair<nodemesh::Combiner::Mesh *, CombineMode>> &multipleMeshes, bool recombine)
{
    nodemesh::Combiner::Mesh *mesh = nullptr;
    for (const auto &it: multipleMeshes) {
        const auto &childCombineMode = it.second;
        nodemesh::Combiner::Mesh *subMesh = it.first;
        qDebug() << "Combine mode:" << CombineModeToString(childCombineMode);
        if (nullptr == subMesh) {
            m_isSucceed = false;
            qDebug() << "Child mesh is null";
            continue;
        }
        if (subMesh->isNull()) {
            m_isSucceed = false;
            qDebug() << "Child mesh is uncombinable";
            delete subMesh;
            continue;
        }
        if (nullptr == mesh) {
            //if (childCombineMode == CombineMode::Inversion) {
            //    delete subMesh;
            //} else {
                mesh = subMesh;
            //}
        } else {
            nodemesh::Combiner::Mesh *newMesh = combineTwoMeshes(*mesh,
                *subMesh,
                childCombineMode == CombineMode::Inversion ?
                    nodemesh::Combiner::Method::Diff : nodemesh::Combiner::Method::Union,
                recombine);
            delete subMesh;
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
    return mesh;
}

nodemesh::Combiner::Mesh *MeshGenerator::combineComponentChildGroupMesh(const std::vector<QString> &componentIdStrings, GeneratedComponent &componentCache)
{
    std::vector<std::pair<nodemesh::Combiner::Mesh *, CombineMode>> multipleMeshes;
    for (const auto &childIdString: componentIdStrings) {
        CombineMode childCombineMode = CombineMode::Normal;
        nodemesh::Combiner::Mesh *subMesh = combineComponentMesh(childIdString, &childCombineMode);
    
        const auto &childComponentCache = m_cacheContext->components[childIdString];
        for (const auto &vertex: childComponentCache.noneSeamVertices)
            componentCache.noneSeamVertices.insert(vertex);
        for (const auto &it: childComponentCache.sharedQuadEdges)
            componentCache.sharedQuadEdges.insert(it);
        for (const auto &it: childComponentCache.outcomeNodes)
            componentCache.outcomeNodes.push_back(it);
        for (const auto &it: childComponentCache.outcomeNodeVertices)
            componentCache.outcomeNodeVertices.push_back(it);
    
        multipleMeshes.push_back({subMesh, childCombineMode});
    }
    return combineMultipleMeshes(multipleMeshes);
}

nodemesh::Combiner::Mesh *MeshGenerator::combineTwoMeshes(const nodemesh::Combiner::Mesh &first, const nodemesh::Combiner::Mesh &second,
    nodemesh::Combiner::Method method,
    bool recombine)
{
    if (first.isNull() || second.isNull())
        return nullptr;
    std::vector<std::pair<nodemesh::Combiner::Source, size_t>> combinedVerticesSources;
    nodemesh::Combiner::Mesh *newMesh = nodemesh::Combiner::combine(first,
        second,
        method,
        &combinedVerticesSources);
    if (nullptr == newMesh)
        return nullptr;
    if (!newMesh->isNull() && recombine) {
        nodemesh::Recombiner recombiner;
        std::vector<QVector3D> combinedVertices;
        std::vector<std::vector<size_t>> combinedFaces;
        newMesh->fetch(combinedVertices, combinedFaces);
        recombiner.setVertices(&combinedVertices, &combinedVerticesSources);
        recombiner.setFaces(&combinedFaces);
        if (recombiner.recombine()) {
            if (nodemesh::isManifold(recombiner.regeneratedFaces())) {
                nodemesh::Combiner::Mesh *reMesh = new nodemesh::Combiner::Mesh(recombiner.regeneratedVertices(), recombiner.regeneratedFaces(), false);
                if (!reMesh->isNull() && !reMesh->isSelfIntersected()) {
                    delete newMesh;
                    newMesh = reMesh;
                } else {
                    delete reMesh;
                }
            }
        }
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
        std::set<std::pair<nodemesh::PositionKey, nodemesh::PositionKey>> *sharedQuadEdges)
{
    for (const auto &face: faces) {
        if (face.size() != 4)
            continue;
        sharedQuadEdges->insert({
            nodemesh::PositionKey(vertices[face[0]]),
            nodemesh::PositionKey(vertices[face[2]])
        });
        sharedQuadEdges->insert({
            nodemesh::PositionKey(vertices[face[1]]),
            nodemesh::PositionKey(vertices[face[3]])
        });
    }
}

void MeshGenerator::setGeneratedCacheContext(GeneratedCacheContext *cacheContext)
{
    m_cacheContext = cacheContext;
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
                it = m_cacheContext->components.erase(it);
                continue;
            }
            it++;
        }
    }
    
    collectParts();
    checkDirtyFlags();
    
    m_dirtyComponentIds.insert(QUuid().toString());
    
    m_mainProfileMiddleX = valueOfKeyInMapOrEmpty(m_snapshot->canvas, "originX").toFloat();
    m_mainProfileMiddleY = valueOfKeyInMapOrEmpty(m_snapshot->canvas, "originY").toFloat();
    m_sideProfileMiddleX = valueOfKeyInMapOrEmpty(m_snapshot->canvas, "originZ").toFloat();
    
    CombineMode combineMode;
    auto combinedMesh = combineComponentMesh(QUuid().toString(), &combineMode);
    
    const auto &componentCache = m_cacheContext->components[QUuid().toString()];
    
    std::vector<QVector3D> combinedVertices;
    std::vector<std::vector<size_t>> combinedFaces;
    if (nullptr != combinedMesh) {
        combinedMesh->fetch(combinedVertices, combinedFaces);
        
        size_t totalAffectedNum = 0;
        size_t affectedNum = 0;
        do {
            std::vector<QVector3D> weldedVertices;
            std::vector<std::vector<size_t>> weldedFaces;
            affectedNum = nodemesh::weldSeam(combinedVertices, combinedFaces,
                0.025, componentCache.noneSeamVertices,
                weldedVertices, weldedFaces);
            combinedVertices = weldedVertices;
            combinedFaces = weldedFaces;
            totalAffectedNum += affectedNum;
        } while (affectedNum > 0);
        qDebug() << "Total weld affected triangles:" << totalAffectedNum;
        
        recoverQuads(combinedVertices, combinedFaces, componentCache.sharedQuadEdges, m_outcome->triangleAndQuads);
        
        m_outcome->nodes = componentCache.outcomeNodes;
        m_outcome->nodeVertices = componentCache.outcomeNodeVertices;
        m_outcome->vertices = combinedVertices;
        m_outcome->triangles = combinedFaces;
    }
    
    auto postprocessOutcome = [](Outcome *outcome) {
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
        triangleSourceNodeResolve(*outcome, sourceNodes);
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
    
    /*
    Outcome *previewOutcome = new Outcome(*m_outcome);
    for (const auto &partCache: m_cacheContext->parts) {
        if (partCache.second.isSucceed)
            continue;
        size_t oldVerticesCount = previewOutcome->vertices.size();
        for (const auto &vertex: partCache.second.vertices) {
            previewOutcome->vertices.push_back(vertex);
        }
        for (const auto &face: partCache.second.previewTriangles) {
            std::vector<size_t> newFace = face;
            for (auto &index: newFace)
                index += oldVerticesCount;
            previewOutcome->triangles.push_back(newFace);
        }
    }
    postprocessOutcome(previewOutcome);
    m_resultMesh = new MeshLoader(*previewOutcome);
    delete previewOutcome;
    */
    
    postprocessOutcome(m_outcome);
    
    m_resultMesh = new MeshLoader(*m_outcome);
    
    delete combinedMesh;

    if (needDeleteCacheContext) {
        delete m_cacheContext;
        m_cacheContext = nullptr;
    }
    
    qDebug() << "The mesh generation took" << countTimeConsumed.elapsed() << "milliseconds";
}

void MeshGenerator::generateSmoothTriangleVertexNormals(const std::vector<QVector3D> &vertices, const std::vector<std::vector<size_t>> &triangles,
    const std::vector<QVector3D> &triangleNormals,
    std::vector<std::vector<QVector3D>> *triangleVertexNormals)
{
    std::vector<QVector3D> smoothNormals;
    nodemesh::angleSmooth(vertices,
        triangles,
        triangleNormals,
        60,
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
