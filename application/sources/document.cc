#include <QFileDialog>
#include <QDebug>
#include <QThread>
#include <QGuiApplication>
#include <QClipboard>
#include <QMimeData>
#include <QApplication>
#include <QVector3D>
#include <functional>
#include <QtCore/qbuffer.h>
#include <QElapsedTimer>
#include <queue>
#include <dust3d/base/snapshot_xml.h>
#include <dust3d/base/texture_type.h>
#include "document.h"
#include "image_forever.h"
#include "mesh_generator.h"
#include "mesh_result_post_processor.h"
#include "texture_generator.h"
#include "material_previews_generator.h"

unsigned long Document::m_maxSnapshot = 1000;

Document::Document() :
    SkeletonDocument()
{
}

Document::~Document()
{
    delete (dust3d::MeshGenerator::GeneratedCacheContext *)m_generatedCacheContext;
    delete m_resultMesh;
    delete m_postProcessedObject;
    delete textureImage;
    delete textureImageByteArray;
    delete textureNormalImage;
    delete textureNormalImageByteArray;
    delete textureMetalnessImage;
    delete textureMetalnessImageByteArray;
    delete textureRoughnessImage;
    delete textureRoughnessImageByteArray;
    delete textureAmbientOcclusionImage;
    delete textureAmbientOcclusionImageByteArray;
    delete m_resultTextureMesh;
}

void Document::uiReady()
{
    qDebug() << "uiReady";
    emit editModeChanged();
}

bool Document::originSettled() const
{
    return !qFuzzyIsNull(getOriginX()) && !qFuzzyIsNull(getOriginY()) && !qFuzzyIsNull(getOriginZ());
}

const Material *Document::findMaterial(dust3d::Uuid materialId) const
{
    auto it = materialMap.find(materialId);
    if (it == materialMap.end())
        return nullptr;
    return &it->second;
}

void Document::setNodeCutRotation(dust3d::Uuid nodeId, float cutRotation)
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

void Document::setNodeCutFace(dust3d::Uuid nodeId, dust3d::CutFace cutFace)
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

void Document::setNodeCutFaceLinkedId(dust3d::Uuid nodeId, dust3d::Uuid linkedId)
{
    auto node = nodeMap.find(nodeId);
    if (node == nodeMap.end()) {
        qDebug() << "Node not found:" << nodeId;
        return;
    }
    if (node->second.cutFace == dust3d::CutFace::UserDefined &&
            node->second.cutFaceLinkedId == linkedId)
        return;
    node->second.setCutFaceLinkedId(linkedId);
    auto part = partMap.find(node->second.partId);
    if (part != partMap.end())
        part->second.dirty = true;
    emit nodeCutFaceChanged(nodeId);
    emit skeletonChanged();
}

void Document::clearNodeCutFaceSettings(dust3d::Uuid nodeId)
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

void Document::clearTurnaround()
{
    turnaround = QImage();
    turnaroundPngByteArray.clear();
    emit turnaroundChanged();
}

void Document::updateTextureImage(QImage *image)
{
    delete textureImageByteArray;
    textureImageByteArray = nullptr;
    
    delete textureImage;
    textureImage = image;
}

void Document::updateTextureNormalImage(QImage *image)
{
    delete textureNormalImageByteArray;
    textureNormalImageByteArray = nullptr;
    
    delete textureNormalImage;
    textureNormalImage = image;
}

void Document::updateTextureMetalnessImage(QImage *image)
{
    delete textureMetalnessImageByteArray;
    textureMetalnessImageByteArray = nullptr;
    
    delete textureMetalnessImage;
    textureMetalnessImage = image;
}

void Document::updateTextureRoughnessImage(QImage *image)
{
    delete textureRoughnessImageByteArray;
    textureRoughnessImageByteArray = nullptr;
    
    delete textureRoughnessImage;
    textureRoughnessImage = image;
}

void Document::updateTextureAmbientOcclusionImage(QImage *image)
{
    delete textureAmbientOcclusionImageByteArray;
    textureAmbientOcclusionImageByteArray = nullptr;
    
    delete textureAmbientOcclusionImage;
    textureAmbientOcclusionImage = image;
}

void Document::setEditMode(SkeletonDocumentEditMode mode)
{
    if (editMode == mode)
        return;
    
    editMode = mode;
    emit editModeChanged();
}

void Document::toSnapshot(dust3d::Snapshot *snapshot, const std::set<dust3d::Uuid> &limitNodeIds,
    DocumentToSnapshotFor forWhat,
    const std::set<dust3d::Uuid> &limitMaterialIds) const
{
    if (DocumentToSnapshotFor::Document == forWhat ||
            DocumentToSnapshotFor::Nodes == forWhat) {
        std::set<dust3d::Uuid> limitPartIds;
        std::set<dust3d::Uuid> limitComponentIds;
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
            std::map<std::string, std::string> part;
            part["id"] = partIt.second.id.toString();
            part["visible"] = partIt.second.visible ? "true" : "false";
            part["locked"] = partIt.second.locked ? "true" : "false";
            part["subdived"] = partIt.second.subdived ? "true" : "false";
            part["disabled"] = partIt.second.disabled ? "true" : "false";
            part["xMirrored"] = partIt.second.xMirrored ? "true" : "false";
            if (partIt.second.base != dust3d::PartBase::XYZ)
                part["base"] = PartBaseToString(partIt.second.base);
            part["rounded"] = partIt.second.rounded ? "true" : "false";
            part["chamfered"] = partIt.second.chamfered ? "true" : "false";
            if (dust3d::PartTarget::Model != partIt.second.target)
                part["target"] = PartTargetToString(partIt.second.target);
            if (partIt.second.cutRotationAdjusted())
                part["cutRotation"] = std::to_string(partIt.second.cutRotation);
            if (partIt.second.cutFaceAdjusted()) {
                if (dust3d::CutFace::UserDefined == partIt.second.cutFace) {
                    if (!partIt.second.cutFaceLinkedId.isNull()) {
                        part["cutFace"] = partIt.second.cutFaceLinkedId.toString();
                    }
                } else {
                    part["cutFace"] = CutFaceToString(partIt.second.cutFace);
                }
            }
            part["__dirty"] = partIt.second.dirty ? "true" : "false";
            if (partIt.second.hasColor)
                part["color"] = partIt.second.color.name(QColor::HexArgb).toUtf8().constData();
            if (partIt.second.colorSolubilityAdjusted())
                part["colorSolubility"] = std::to_string(partIt.second.colorSolubility);
            if (partIt.second.metalnessAdjusted())
                part["metallic"] = std::to_string(partIt.second.metalness);
            if (partIt.second.roughnessAdjusted())
                part["roughness"] = std::to_string(partIt.second.roughness);
            if (partIt.second.deformThicknessAdjusted())
                part["deformThickness"] = std::to_string(partIt.second.deformThickness);
            if (partIt.second.deformWidthAdjusted())
                part["deformWidth"] = std::to_string(partIt.second.deformWidth);
            if (partIt.second.deformUnified)
                part["deformUnified"] = "true";
            if (partIt.second.hollowThicknessAdjusted())
                part["hollowThickness"] = std::to_string(partIt.second.hollowThickness);
            if (!partIt.second.name.isEmpty())
                part["name"] = partIt.second.name.toUtf8().constData();
            if (partIt.second.materialAdjusted())
                part["materialId"] = partIt.second.materialId.toString();
            if (partIt.second.countershaded)
                part["countershaded"] = "true";
            if (partIt.second.smooth)
                part["smooth"] = "true";
            snapshot->parts[part["id"]] = part;
        }
        for (const auto &nodeIt: nodeMap) {
            if (!limitNodeIds.empty() && limitNodeIds.find(nodeIt.first) == limitNodeIds.end())
                continue;
            std::map<std::string, std::string> node;
            node["id"] = nodeIt.second.id.toString();
            node["radius"] = std::to_string(nodeIt.second.radius);
            node["x"] = std::to_string(nodeIt.second.getX());
            node["y"] = std::to_string(nodeIt.second.getY());
            node["z"] = std::to_string(nodeIt.second.getZ());
            node["partId"] = nodeIt.second.partId.toString();
            if (nodeIt.second.hasCutFaceSettings) {
                node["cutRotation"] = std::to_string(nodeIt.second.cutRotation);
                if (dust3d::CutFace::UserDefined == nodeIt.second.cutFace) {
                    if (!nodeIt.second.cutFaceLinkedId.isNull()) {
                        node["cutFace"] = nodeIt.second.cutFaceLinkedId.toString();
                    }
                } else {
                    node["cutFace"] = CutFaceToString(nodeIt.second.cutFace);
                }
            }
            if (!nodeIt.second.name.isEmpty())
                node["name"] = nodeIt.second.name.toUtf8().constData();
            snapshot->nodes[node["id"]] = node;
        }
        for (const auto &edgeIt: edgeMap) {
            if (edgeIt.second.nodeIds.size() != 2)
                continue;
            if (!limitNodeIds.empty() &&
                    (limitNodeIds.find(edgeIt.second.nodeIds[0]) == limitNodeIds.end() ||
                        limitNodeIds.find(edgeIt.second.nodeIds[1]) == limitNodeIds.end()))
                continue;
            std::map<std::string, std::string> edge;
            edge["id"] = edgeIt.second.id.toString();
            edge["from"] = edgeIt.second.nodeIds[0].toString();
            edge["to"] = edgeIt.second.nodeIds[1].toString();
            edge["partId"] = edgeIt.second.partId.toString();
            if (!edgeIt.second.name.isEmpty())
                edge["name"] = edgeIt.second.name.toUtf8().constData();
            snapshot->edges[edge["id"]] = edge;
        }
        for (const auto &componentIt: componentMap) {
            if (!limitComponentIds.empty() && limitComponentIds.find(componentIt.first) == limitComponentIds.end())
                continue;
            std::map<std::string, std::string> component;
            component["id"] = componentIt.second.id.toString();
            if (!componentIt.second.name.isEmpty())
                component["name"] = componentIt.second.name.toUtf8().constData();
            component["expanded"] = componentIt.second.expanded ? "true" : "false";
            component["combineMode"] = CombineModeToString(componentIt.second.combineMode);
            component["__dirty"] = componentIt.second.dirty ? "true" : "false";
            std::vector<std::string> childIdList;
            for (const auto &childId: componentIt.second.childrenIds) {
                childIdList.push_back(childId.toString());
            }
            std::string children = dust3d::String::join(childIdList, ",");
            if (!children.empty())
                component["children"] = children;
            std::string linkData = componentIt.second.linkData().toUtf8().constData();
            if (!linkData.empty()) {
                component["linkData"] = linkData;
                component["linkDataType"] = componentIt.second.linkDataType().toUtf8().constData();
            }
            if (!componentIt.second.name.isEmpty())
                component["name"] = componentIt.second.name.toUtf8().constData();
            snapshot->components[component["id"]] = component;
        }
        if (limitComponentIds.empty() || limitComponentIds.find(dust3d::Uuid()) != limitComponentIds.end()) {
            std::vector<std::string> childIdList;
            for (const auto &childId: rootComponent.childrenIds) {
                childIdList.push_back(childId.toString());
            }
            std::string children = dust3d::String::join(childIdList, ",");
            if (!children.empty())
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
            std::map<std::string, std::string> material;
            material["id"] = materialIt.second.id.toString();
            material["type"] = "MetalRoughness";
            if (!materialIt.second.name.isEmpty())
                material["name"] = materialIt.second.name.toUtf8().constData();
            std::vector<std::pair<std::map<std::string, std::string>, std::vector<std::map<std::string, std::string>>>> layers;
            for (const auto &layer: materialIt.second.layers) {
                std::vector<std::map<std::string, std::string>> maps;
                for (const auto &mapItem: layer.maps) {
                    std::map<std::string, std::string> textureMap;
                    textureMap["for"] = TextureTypeToString(mapItem.forWhat);
                    textureMap["linkDataType"] = "imageId";
                    textureMap["linkData"] = mapItem.imageId.toString();
                    maps.push_back(textureMap);
                }
                std::map<std::string, std::string> layerAttributes;
                if (!qFuzzyCompare((float)layer.tileScale, (float)1.0))
                    layerAttributes["tileScale"] = std::to_string(layer.tileScale);
                layers.push_back({layerAttributes, maps});
            }
            snapshot->materials.push_back(std::make_pair(material, layers));
        }
    }
    if (DocumentToSnapshotFor::Document == forWhat) {
        std::map<std::string, std::string> canvas;
        canvas["originX"] = std::to_string(getOriginX());
        canvas["originY"] = std::to_string(getOriginY());
        canvas["originZ"] = std::to_string(getOriginZ());
        snapshot->canvas = canvas;
    }
}

void Document::addFromSnapshot(const dust3d::Snapshot &snapshot, enum SnapshotSource source)
{
    bool isOriginChanged = false;
    if (SnapshotSource::Paste != source &&
            SnapshotSource::Import != source) {
        const auto &originXit = snapshot.canvas.find("originX");
        const auto &originYit = snapshot.canvas.find("originY");
        const auto &originZit = snapshot.canvas.find("originZ");
        if (originXit != snapshot.canvas.end() &&
                originYit != snapshot.canvas.end() &&
                originZit != snapshot.canvas.end()) {
            setOriginX(dust3d::String::toFloat(originXit->second));
            setOriginY(dust3d::String::toFloat(originYit->second));
            setOriginZ(dust3d::String::toFloat(originZit->second));
            isOriginChanged = true;
        }
    }
    
    std::set<dust3d::Uuid> newAddedNodeIds;
    std::set<dust3d::Uuid> newAddedEdgeIds;
    std::set<dust3d::Uuid> newAddedPartIds;
    std::set<dust3d::Uuid> newAddedComponentIds;
    
    std::set<dust3d::Uuid> inversePartIds;
    
    std::map<dust3d::Uuid, dust3d::Uuid> oldNewIdMap;
    for (const auto &materialIt: snapshot.materials) {
        const auto &materialAttributes = materialIt.first;
        auto materialType = dust3d::String::valueOrEmpty(materialAttributes, "type");
        if ("MetalRoughness" != materialType) {
            qDebug() << "Unsupported material type:" << materialType;
            continue;
        }
        dust3d::Uuid oldMaterialId = dust3d::Uuid(dust3d::String::valueOrEmpty(materialAttributes, "id"));
        dust3d::Uuid newMaterialId = SnapshotSource::Import == source ? oldMaterialId : dust3d::Uuid::createUuid();
        oldNewIdMap[oldMaterialId] = newMaterialId;
        if (materialMap.end() == materialMap.find(newMaterialId)) {
            auto &newMaterial = materialMap[newMaterialId];
            newMaterial.id = newMaterialId;
            newMaterial.name = dust3d::String::valueOrEmpty(materialAttributes, "name").c_str();
            for (const auto &layerIt: materialIt.second) {
                MaterialLayer layer;
                auto findTileScale = layerIt.first.find("tileScale");
                if (findTileScale != layerIt.first.end())
                    layer.tileScale = dust3d::String::toFloat(findTileScale->second);
                for (const auto &mapItem: layerIt.second) {
                    auto textureTypeString = dust3d::String::valueOrEmpty(mapItem, "for");
                    auto textureType = dust3d::TextureTypeFromString(textureTypeString.c_str());
                    if (dust3d::TextureType::None == textureType) {
                        qDebug() << "Unsupported texture type:" << textureTypeString;
                        continue;
                    }
                    auto linkTypeString = dust3d::String::valueOrEmpty(mapItem, "linkDataType");
                    if ("imageId" != linkTypeString) {
                        qDebug() << "Unsupported link data type:" << linkTypeString;
                        continue;
                    }
                    auto imageId = dust3d::Uuid(dust3d::String::valueOrEmpty(mapItem, "linkData"));
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
    }
    std::map<dust3d::Uuid, dust3d::Uuid> cutFaceLinkedIdModifyMap;
    for (const auto &partKv: snapshot.parts) {
        const auto newUuid = dust3d::Uuid::createUuid();
        SkeletonPart &part = partMap[newUuid];
        part.id = newUuid;
        oldNewIdMap[dust3d::Uuid(partKv.first)] = part.id;
        part.name = dust3d::String::valueOrEmpty(partKv.second, "name").c_str();
        const auto &visibleIt = partKv.second.find("visible");
        if (visibleIt != partKv.second.end()) {
            part.visible = dust3d::String::isTrue(visibleIt->second);
        } else {
            part.visible = true;
        }
        part.locked = dust3d::String::isTrue(dust3d::String::valueOrEmpty(partKv.second, "locked"));
        part.subdived = dust3d::String::isTrue(dust3d::String::valueOrEmpty(partKv.second, "subdived"));
        part.disabled = dust3d::String::isTrue(dust3d::String::valueOrEmpty(partKv.second, "disabled"));
        part.xMirrored = dust3d::String::isTrue(dust3d::String::valueOrEmpty(partKv.second, "xMirrored"));
        part.base = dust3d::PartBaseFromString(dust3d::String::valueOrEmpty(partKv.second, "base").c_str());
        part.rounded = dust3d::String::isTrue(dust3d::String::valueOrEmpty(partKv.second, "rounded"));
        part.chamfered = dust3d::String::isTrue(dust3d::String::valueOrEmpty(partKv.second, "chamfered"));
        part.target = dust3d::PartTargetFromString(dust3d::String::valueOrEmpty(partKv.second, "target").c_str());
        const auto &cutRotationIt = partKv.second.find("cutRotation");
        if (cutRotationIt != partKv.second.end())
            part.setCutRotation(dust3d::String::toFloat(cutRotationIt->second));
        const auto &cutFaceIt = partKv.second.find("cutFace");
        if (cutFaceIt != partKv.second.end()) {
            dust3d::Uuid cutFaceLinkedId = dust3d::Uuid(cutFaceIt->second);
            if (cutFaceLinkedId.isNull()) {
                part.setCutFace(dust3d::CutFaceFromString(cutFaceIt->second.c_str()));
            } else {
                part.setCutFaceLinkedId(cutFaceLinkedId);
                cutFaceLinkedIdModifyMap.insert({part.id, cutFaceLinkedId});
            }
        }
        if (dust3d::String::isTrue(dust3d::String::valueOrEmpty(partKv.second, "inverse")))
            inversePartIds.insert(part.id);
        const auto &colorIt = partKv.second.find("color");
        if (colorIt != partKv.second.end()) {
            part.color = QColor(colorIt->second.c_str());
            part.hasColor = true;
        }
        const auto &colorSolubilityIt = partKv.second.find("colorSolubility");
        if (colorSolubilityIt != partKv.second.end())
            part.colorSolubility = dust3d::String::toFloat(colorSolubilityIt->second);
        const auto &metalnessIt = partKv.second.find("metallic");
        if (metalnessIt != partKv.second.end())
            part.metalness = dust3d::String::toFloat(metalnessIt->second);
        const auto &roughnessIt = partKv.second.find("roughness");
        if (roughnessIt != partKv.second.end())
            part.roughness = dust3d::String::toFloat(roughnessIt->second);
        const auto &deformThicknessIt = partKv.second.find("deformThickness");
        if (deformThicknessIt != partKv.second.end())
            part.setDeformThickness(dust3d::String::toFloat(deformThicknessIt->second));
        const auto &deformWidthIt = partKv.second.find("deformWidth");
        if (deformWidthIt != partKv.second.end())
            part.setDeformWidth(dust3d::String::toFloat(deformWidthIt->second));
        const auto &deformUnifiedIt = partKv.second.find("deformUnified");
        if (deformUnifiedIt != partKv.second.end())
            part.deformUnified = dust3d::String::isTrue(dust3d::String::valueOrEmpty(partKv.second, "deformUnified"));
        const auto &hollowThicknessIt = partKv.second.find("hollowThickness");
        if (hollowThicknessIt != partKv.second.end())
            part.hollowThickness = dust3d::String::toFloat(hollowThicknessIt->second);
        const auto &materialIdIt = partKv.second.find("materialId");
        if (materialIdIt != partKv.second.end())
            part.materialId = oldNewIdMap[dust3d::Uuid(materialIdIt->second)];
        part.countershaded = dust3d::String::isTrue(dust3d::String::valueOrEmpty(partKv.second, "countershaded"));
        part.smooth = dust3d::String::isTrue(dust3d::String::valueOrEmpty(partKv.second, "smooth"));
        newAddedPartIds.insert(part.id);
    }
    for (const auto &it: cutFaceLinkedIdModifyMap) {
        SkeletonPart &part = partMap[it.first];
        auto findNewLinkedId = oldNewIdMap.find(it.second);
        if (findNewLinkedId == oldNewIdMap.end()) {
            if (partMap.find(it.second) == partMap.end()) {
                part.setCutFaceLinkedId(dust3d::Uuid());
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
        dust3d::Uuid oldNodeId = dust3d::Uuid(nodeKv.first);
        SkeletonNode node(nodeMap.find(oldNodeId) == nodeMap.end() ? oldNodeId : dust3d::Uuid::createUuid());
        oldNewIdMap[oldNodeId] = node.id;
        node.name = dust3d::String::valueOrEmpty(nodeKv.second, "name").c_str();
        node.radius = dust3d::String::toFloat(dust3d::String::valueOrEmpty(nodeKv.second, "radius"));
        node.setX(dust3d::String::toFloat(dust3d::String::valueOrEmpty(nodeKv.second, "x")));
        node.setY(dust3d::String::toFloat(dust3d::String::valueOrEmpty(nodeKv.second, "y")));
        node.setZ(dust3d::String::toFloat(dust3d::String::valueOrEmpty(nodeKv.second, "z")));
        node.partId = oldNewIdMap[dust3d::Uuid(dust3d::String::valueOrEmpty(nodeKv.second, "partId"))];
        const auto &cutRotationIt = nodeKv.second.find("cutRotation");
        if (cutRotationIt != nodeKv.second.end())
            node.setCutRotation(dust3d::String::toFloat(cutRotationIt->second));
        const auto &cutFaceIt = nodeKv.second.find("cutFace");
        if (cutFaceIt != nodeKv.second.end()) {
            dust3d::Uuid cutFaceLinkedId = dust3d::Uuid(cutFaceIt->second);
            if (cutFaceLinkedId.isNull()) {
                node.setCutFace(dust3d::CutFaceFromString(cutFaceIt->second.c_str()));
            } else {
                node.setCutFaceLinkedId(cutFaceLinkedId);
                auto findNewLinkedId = oldNewIdMap.find(cutFaceLinkedId);
                if (findNewLinkedId == oldNewIdMap.end()) {
                    if (partMap.find(cutFaceLinkedId) == partMap.end()) {
                        node.setCutFaceLinkedId(dust3d::Uuid());
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
        dust3d::Uuid oldEdgeId = dust3d::Uuid(edgeKv.first);
        SkeletonEdge edge(edgeMap.find(oldEdgeId) == edgeMap.end() ? oldEdgeId : dust3d::Uuid::createUuid());
        oldNewIdMap[oldEdgeId] = edge.id;
        edge.name = dust3d::String::valueOrEmpty(edgeKv.second, "name").c_str();
        edge.partId = oldNewIdMap[dust3d::Uuid(dust3d::String::valueOrEmpty(edgeKv.second, "partId"))];
        std::string fromNodeId = dust3d::String::valueOrEmpty(edgeKv.second, "from");
        if (!fromNodeId.empty()) {
            dust3d::Uuid fromId = oldNewIdMap[dust3d::Uuid(fromNodeId)];
            edge.nodeIds.push_back(fromId);
            nodeMap[fromId].edgeIds.push_back(edge.id);
        }
        std::string toNodeId = dust3d::String::valueOrEmpty(edgeKv.second, "to");
        if (!toNodeId.empty()) {
            dust3d::Uuid toId = oldNewIdMap[dust3d::Uuid(toNodeId)];
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
        QString linkData = dust3d::String::valueOrEmpty(componentKv.second, "linkData").c_str();
        QString linkDataType = dust3d::String::valueOrEmpty(componentKv.second, "linkDataType").c_str();
        SkeletonComponent component(dust3d::Uuid(), linkData, linkDataType);
        oldNewIdMap[dust3d::Uuid(componentKv.first)] = component.id;
        component.name = dust3d::String::valueOrEmpty(componentKv.second, "name").c_str();
        component.expanded = dust3d::String::isTrue(dust3d::String::valueOrEmpty(componentKv.second, "expanded"));
        component.combineMode = dust3d::CombineModeFromString(dust3d::String::valueOrEmpty(componentKv.second, "combineMode").c_str());
        if (component.combineMode == dust3d::CombineMode::Normal) {
            if (dust3d::String::isTrue(dust3d::String::valueOrEmpty(componentKv.second, "inverse")))
                component.combineMode = dust3d::CombineMode::Inversion;
        }
        //qDebug() << "Add component:" << component.id << " old:" << componentKv.first << "name:" << component.name;
        if ("partId" == linkDataType) {
            dust3d::Uuid partId = oldNewIdMap[dust3d::Uuid(linkData.toUtf8().constData())];
            component.linkToPartId = partId;
            //qDebug() << "Add part:" << partId << " from component:" << component.id;
            partMap[partId].componentId = component.id;
            if (inversePartIds.find(partId) != inversePartIds.end())
                component.combineMode = dust3d::CombineMode::Inversion;
        }
        componentMap.emplace(component.id, std::move(component));
        newAddedComponentIds.insert(component.id);
    }
    const auto &rootComponentChildren = snapshot.rootComponent.find("children");
    if (rootComponentChildren != snapshot.rootComponent.end()) {
        for (const auto &childId: dust3d::String::split(rootComponentChildren->second, ',')) {
            if (childId.empty())
                continue;
            dust3d::Uuid componentId = oldNewIdMap[dust3d::Uuid(childId)];
            if (componentMap.find(componentId) == componentMap.end())
                continue;
            //qDebug() << "Add root component:" << componentId;
            rootComponent.addChild(componentId);
        }
    }
    for (const auto &componentKv: snapshot.components) {
        dust3d::Uuid componentId = oldNewIdMap[dust3d::Uuid(componentKv.first)];
        if (componentMap.find(componentId) == componentMap.end())
            continue;
        for (const auto &childId: dust3d::String::split(dust3d::String::valueOrEmpty(componentKv.second, "children"), ',')) {
            if (childId.empty())
                continue;
            dust3d::Uuid childComponentId = oldNewIdMap[dust3d::Uuid(childId)];
            if (componentMap.find(childComponentId) == componentMap.end())
                continue;
            //qDebug() << "Add child component:" << childComponentId << "to" << componentId;
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
    
    emit componentChildrenChanged(dust3d::Uuid());
    if (isOriginChanged)
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
    
    if (!snapshot.materials.empty())
        emit materialListChanged();
}

void Document::silentReset()
{
    setOriginX(0.0);
    setOriginY(0.0);
    setOriginZ(0.0);
    nodeMap.clear();
    edgeMap.clear();
    partMap.clear();
    componentMap.clear();
    materialMap.clear();
    materialIdList.clear();
    rootComponent = SkeletonComponent();
}

void Document::reset()
{
    silentReset();
    emit cleanup();
    emit skeletonChanged();
}

void Document::fromSnapshot(const dust3d::Snapshot &snapshot)
{
    reset();
    addFromSnapshot(snapshot, SnapshotSource::Unknown);
    emit uncheckAll();
}

ModelMesh *Document::takeResultMesh()
{
    if (nullptr == m_resultMesh)
        return nullptr;
    ModelMesh *resultMesh = new ModelMesh(*m_resultMesh);
    return resultMesh;
}

MonochromeMesh *Document::takeWireframeMesh()
{
    if (nullptr == m_wireframeMesh)
        return nullptr;
    return new MonochromeMesh(*m_wireframeMesh);
}

bool Document::isMeshGenerationSucceed()
{
    return m_isMeshGenerationSucceed;
}

ModelMesh *Document::takeResultTextureMesh()
{
    if (nullptr == m_resultTextureMesh)
        return nullptr;
    ModelMesh *resultTextureMesh = new ModelMesh(*m_resultTextureMesh);
    return resultTextureMesh;
}

void Document::meshReady()
{
    ModelMesh *resultMesh = m_meshGenerator->takeResultMesh();
    m_wireframeMesh.reset(m_meshGenerator->takeWireframeMesh());
    dust3d::Object *object = m_meshGenerator->takeObject();
    bool isSuccessful = m_meshGenerator->isSuccessful();
    
    std::unique_ptr<std::map<dust3d::Uuid, std::unique_ptr<ModelMesh>>> componentPreviewMeshes;
    componentPreviewMeshes.reset(m_meshGenerator->takeComponentPreviewMeshes());
    bool componentPreviewsChanged = componentPreviewMeshes && !componentPreviewMeshes->empty();
    if (componentPreviewsChanged) {
        for (auto &it: *componentPreviewMeshes) {
            setComponentPreviewMesh(it.first, std::move(it.second));
        }
        emit resultComponentPreviewMeshesChanged();
    }
    
    delete m_resultMesh;
    m_resultMesh = resultMesh;
    
    m_isMeshGenerationSucceed = isSuccessful;
    
    delete m_currentObject;
    m_currentObject = object;
    
    if (nullptr == m_resultMesh) {
        qDebug() << "Result mesh is null";
    }
    
    delete m_meshGenerator;
    m_meshGenerator = nullptr;
    
    qDebug() << "Mesh generation done";
    
    m_isPostProcessResultObsolete = true;
    emit resultMeshChanged();
    
    if (m_isResultMeshObsolete) {
        generateMesh();
    }
}

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
    
    emit meshGenerating();
    
    qDebug() << "Mesh generating..";
    
    settleOrigin();
    
    m_isResultMeshObsolete = false;
    
    QThread *thread = new QThread;
    
    dust3d::Snapshot *snapshot = new dust3d::Snapshot;
    toSnapshot(snapshot);
    resetDirtyFlags();
    m_meshGenerator = new MeshGenerator(snapshot);
    m_meshGenerator->setId(m_nextMeshGenerationId++);
    m_meshGenerator->setDefaultPartColor(dust3d::Color::createWhite());
    if (nullptr == m_generatedCacheContext)
        m_generatedCacheContext = new MeshGenerator::GeneratedCacheContext;
    m_meshGenerator->setGeneratedCacheContext((dust3d::MeshGenerator::GeneratedCacheContext *)m_generatedCacheContext);
    if (!m_smoothNormal) {
        m_meshGenerator->setSmoothShadingThresholdAngleDegrees(0);
    }
    m_meshGenerator->moveToThread(thread);
    connect(thread, &QThread::started, m_meshGenerator, &MeshGenerator::process);
    connect(m_meshGenerator, &MeshGenerator::finished, this, &Document::meshReady);
    connect(m_meshGenerator, &MeshGenerator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void Document::generateTexture()
{
    if (nullptr != m_textureGenerator) {
        m_isTextureObsolete = true;
        return;
    }
    
    qDebug() << "Texture guide generating..";
    emit textureGenerating();
    
    m_isTextureObsolete = false;
    
    dust3d::Snapshot *snapshot = new dust3d::Snapshot;
    toSnapshot(snapshot);
    
    QThread *thread = new QThread;
    m_textureGenerator = new TextureGenerator(*m_postProcessedObject, snapshot);
    m_textureGenerator->moveToThread(thread);
    connect(thread, &QThread::started, m_textureGenerator, &TextureGenerator::process);
    connect(m_textureGenerator, &TextureGenerator::finished, this, &Document::textureReady);
    connect(m_textureGenerator, &TextureGenerator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void Document::textureReady()
{
    updateTextureImage(m_textureGenerator->takeResultTextureColorImage());
    updateTextureNormalImage(m_textureGenerator->takeResultTextureNormalImage());
    updateTextureMetalnessImage(m_textureGenerator->takeResultTextureMetalnessImage());
    updateTextureRoughnessImage(m_textureGenerator->takeResultTextureRoughnessImage());
    updateTextureAmbientOcclusionImage(m_textureGenerator->takeResultTextureAmbientOcclusionImage());
    
    delete m_resultTextureMesh;
    m_resultTextureMesh = m_textureGenerator->takeResultMesh();
    
    m_postProcessedObject->alphaEnabled = m_textureGenerator->hasTransparencySettings();

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

    if (!m_currentObject) {
        qDebug() << "Model is null";
        return;
    }

    qDebug() << "Post processing..";
    emit postProcessing();

    QThread *thread = new QThread;
    m_postProcessor = new MeshResultPostProcessor(*m_currentObject);
    m_postProcessor->moveToThread(thread);
    connect(thread, &QThread::started, m_postProcessor, &MeshResultPostProcessor::process);
    connect(m_postProcessor, &MeshResultPostProcessor::finished, this, &Document::postProcessedMeshResultReady);
    connect(m_postProcessor, &MeshResultPostProcessor::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void Document::postProcessedMeshResultReady()
{
    delete m_postProcessedObject;
    m_postProcessedObject = m_postProcessor->takePostProcessedObject();

    delete m_postProcessor;
    m_postProcessor = nullptr;

    qDebug() << "Post process done";

    emit postProcessedResultChanged();

    if (m_isPostProcessResultObsolete) {
        postProcess();
    }
}

const dust3d::Object &Document::currentPostProcessedObject() const
{
    return *m_postProcessedObject;
}

void Document::setComponentCombineMode(dust3d::Uuid componentId, dust3d::CombineMode combineMode)
{
    auto component = componentMap.find(componentId);
    if (component == componentMap.end()) {
        qDebug() << "SkeletonComponent not found:" << componentId;
        return;
    }
    if (component->second.combineMode == combineMode)
        return;
    component->second.combineMode = combineMode;
    component->second.dirty = true;
    emit componentCombineModeChanged(componentId);
    emit skeletonChanged();
}

void Document::setPartSubdivState(dust3d::Uuid partId, bool subdived)
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

void Document::resolveSnapshotBoundingBox(const dust3d::Snapshot &snapshot, QRectF *mainProfile, QRectF *sideProfile)
{
    float left = 0;
    bool leftFirstTime = true;
    float right = 0;
    bool rightFirstTime = true;
    float top = 0;
    bool topFirstTime = true;
    float bottom = 0;
    bool bottomFirstTime = true;
    float zLeft = 0;
    bool zLeftFirstTime = true;
    float zRight = 0;
    bool zRightFirstTime = true;
    for (const auto &nodeIt: snapshot.nodes) {
        float radius = dust3d::String::toFloat(dust3d::String::valueOrEmpty(nodeIt.second, "radius"));
        float x = dust3d::String::toFloat(dust3d::String::valueOrEmpty(nodeIt.second, "x"));
        float y = dust3d::String::toFloat(dust3d::String::valueOrEmpty(nodeIt.second, "y"));
        float z = dust3d::String::toFloat(dust3d::String::valueOrEmpty(nodeIt.second, "z"));
        if (leftFirstTime || x - radius < left) {
            left = x - radius;
            leftFirstTime = false;
        }
        if (topFirstTime || y - radius < top) {
            top = y - radius;
            topFirstTime = false;
        }
        if (rightFirstTime || x + radius > right) {
            right = x + radius;
            rightFirstTime = false;
        }
        if (bottomFirstTime || y + radius > bottom) {
            bottom = y + radius;
            bottomFirstTime = false;
        }
        if (zLeftFirstTime || z - radius < zLeft) {
            zLeft = z - radius;
            zLeftFirstTime = false;
        }
        if (zRightFirstTime || z + radius > zRight) {
            zRight = z + radius;
            zRightFirstTime = false;
        }
    }
    *mainProfile = QRectF(QPointF(left, top), QPointF(right, bottom));
    *sideProfile = QRectF(QPointF(zLeft, top), QPointF(zRight, bottom));
}

void Document::settleOrigin()
{
    if (originSettled())
        return;
    dust3d::Snapshot snapshot;
    toSnapshot(&snapshot);
    QRectF mainProfile;
    QRectF sideProfile;
    resolveSnapshotBoundingBox(snapshot, &mainProfile, &sideProfile);
    setOriginX(mainProfile.x() + mainProfile.width() / 2);
    setOriginY(mainProfile.y() + mainProfile.height() / 2);
    setOriginZ(sideProfile.x() + sideProfile.width() / 2);
    markAllDirty();
    emit originChanged();
}

void Document::setPartXmirrorState(dust3d::Uuid partId, bool mirrored)
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

void Document::setPartDeformThickness(dust3d::Uuid partId, float thickness)
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

void Document::setPartBase(dust3d::Uuid partId, dust3d::PartBase base)
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

void Document::setPartDeformWidth(dust3d::Uuid partId, float width)
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

void Document::setPartDeformUnified(dust3d::Uuid partId, bool unified)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        qDebug() << "Part not found:" << partId;
        return;
    }
    if (part->second.deformUnified == unified)
        return;
    part->second.deformUnified = unified;
    part->second.dirty = true;
    emit partDeformUnifyStateChanged(partId);
    emit skeletonChanged();
}

void Document::setPartMaterialId(dust3d::Uuid partId, dust3d::Uuid materialId)
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

void Document::setPartRoundState(dust3d::Uuid partId, bool rounded)
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

void Document::setPartChamferState(dust3d::Uuid partId, bool chamfered)
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

void Document::setPartTarget(dust3d::Uuid partId, dust3d::PartTarget target)
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

void Document::setPartColorSolubility(dust3d::Uuid partId, float solubility)
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

void Document::setPartMetalness(dust3d::Uuid partId, float metalness)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        qDebug() << "Part not found:" << partId;
        return;
    }
    if (qFuzzyCompare(part->second.metalness, metalness))
        return;
    part->second.metalness = metalness;
    part->second.dirty = true;
    emit partMetalnessChanged(partId);
    emit skeletonChanged();
}

void Document::setPartRoughness(dust3d::Uuid partId, float roughness)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        qDebug() << "Part not found:" << partId;
        return;
    }
    if (qFuzzyCompare(part->second.roughness, roughness))
        return;
    part->second.roughness = roughness;
    part->second.dirty = true;
    emit partRoughnessChanged(partId);
    emit skeletonChanged();
}

void Document::setPartHollowThickness(dust3d::Uuid partId, float hollowThickness)
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

void Document::setPartCountershaded(dust3d::Uuid partId, bool countershaded)
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

void Document::setPartSmoothState(dust3d::Uuid partId, bool smooth)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        qDebug() << "Part not found:" << partId;
        return;
    }
    if (part->second.smooth == smooth)
        return;
    part->second.smooth = smooth;
    part->second.dirty = true;
    emit partSmoothStateChanged(partId);
    emit skeletonChanged();
}

void Document::setPartCutRotation(dust3d::Uuid partId, float cutRotation)
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

void Document::setPartCutFace(dust3d::Uuid partId, dust3d::CutFace cutFace)
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

void Document::setPartCutFaceLinkedId(dust3d::Uuid partId, dust3d::Uuid linkedId)
{
    auto part = partMap.find(partId);
    if (part == partMap.end()) {
        qDebug() << "Part not found:" << partId;
        return;
    }
    if (part->second.cutFace == dust3d::CutFace::UserDefined &&
            part->second.cutFaceLinkedId == linkedId)
        return;
    part->second.setCutFaceLinkedId(linkedId);
    part->second.dirty = true;
    emit partCutFaceChanged(partId);
    emit skeletonChanged();
}

void Document::setPartColorState(dust3d::Uuid partId, bool hasColor, QColor color)
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
    toSnapshot(&item.snapshot);
    if (m_undoItems.size() + 1 > m_maxSnapshot)
        m_undoItems.pop_front();
    m_undoItems.push_back(item);
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
        dust3d::Snapshot snapshot;
        std::string text = mimeData->text().toUtf8().constData();
        loadSnapshotFromXmlString(&snapshot, (char *)text.c_str());
        addFromSnapshot(snapshot, SnapshotSource::Paste);
        saveSnapshot();
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

bool Document::undoable() const
{
    return m_undoItems.size() >= 2;
}

bool Document::redoable() const
{
    return !m_redoItems.empty();
}

bool Document::isNodeEditable(dust3d::Uuid nodeId) const
{
    const SkeletonNode *node = findNode(nodeId);
    if (!node) {
        qDebug() << "Node id not found:" << nodeId;
        return false;
    }
    return !isPartReadonly(node->partId);
}

bool Document::isEdgeEditable(dust3d::Uuid edgeId) const
{
    const SkeletonEdge *edge = findEdge(edgeId);
    if (!edge) {
        qDebug() << "Edge id not found:" << edgeId;
        return false;
    }
    return !isPartReadonly(edge->partId);
}

bool Document::isExportReady() const
{
    if (m_meshGenerator ||
            m_textureGenerator ||
            m_postProcessor)
        return false;
    
    if (m_isResultMeshObsolete ||
            m_isTextureObsolete ||
            m_isPostProcessResultObsolete)
        return false;
        
    return true;
}

void Document::checkExportReadyState()
{
    if (isExportReady())
        emit exportReady();
}

void Document::addMaterial(dust3d::Uuid materialId, QString name, std::vector<MaterialLayer> layers)
{
    auto findMaterialResult = materialMap.find(materialId);
    if (findMaterialResult != materialMap.end()) {
        qDebug() << "Material already exist:" << materialId;
        return;
    }
    
    dust3d::Uuid newMaterialId = materialId;
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

void Document::removeMaterial(dust3d::Uuid materialId)
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

void Document::setMaterialLayers(dust3d::Uuid materialId, std::vector<MaterialLayer> layers)
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

void Document::renameMaterial(dust3d::Uuid materialId, QString name)
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
            ModelMesh *resultPartPreviewMesh = m_materialPreviewsGenerator->takePreview(materialId);
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

bool Document::isPostProcessing() const
{
    return nullptr != m_postProcessor;
}

bool Document::isTextureGenerating() const
{
    return nullptr != m_textureGenerator;
}

void Document::copyNodes(std::set<dust3d::Uuid> nodeIdSet) const
{
    dust3d::Snapshot snapshot;
    toSnapshot(&snapshot, nodeIdSet, DocumentToSnapshotFor::Nodes);
    std::string snapshotXml;
    dust3d::saveSnapshotToXmlString(snapshot, snapshotXml);
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(snapshotXml.c_str());
}

void Document::collectCutFaceList(std::vector<QString> &cutFaces) const
{
    cutFaces.clear();
    
    std::vector<dust3d::Uuid> cutFacePartIdList;
    
    std::set<dust3d::Uuid> cutFacePartIds;
    for (const auto &it: partMap) {
        if (dust3d::PartTarget::CutFace == it.second.target) {
            if (cutFacePartIds.find(it.first) != cutFacePartIds.end())
                continue;
            cutFacePartIds.insert(it.first);
            cutFacePartIdList.push_back(it.first);
        }
        if (!it.second.cutFaceLinkedId.isNull()) {
            if (cutFacePartIds.find(it.second.cutFaceLinkedId) != cutFacePartIds.end())
                continue;
            cutFacePartIds.insert(it.second.cutFaceLinkedId);
            cutFacePartIdList.push_back(it.second.cutFaceLinkedId);
        }
    }
    
    // Sort cut face by center.x of front view
    std::map<dust3d::Uuid, float> centerOffsetMap;
    for (const auto &partId: cutFacePartIdList) {
        const SkeletonPart *part = findPart(partId);
        if (nullptr == part)
            continue;
        float offsetSum = 0;
        for (const auto &nodeId: part->nodeIds) {
            const SkeletonNode *node = findNode(nodeId);
            if (nullptr == node)
                continue;
            offsetSum += node->getX();
        }
        if (qFuzzyIsNull(offsetSum))
            continue;
        centerOffsetMap[partId] = offsetSum / part->nodeIds.size();
    }
    std::sort(cutFacePartIdList.begin(), cutFacePartIdList.end(),
            [&](const dust3d::Uuid &firstPartId, const dust3d::Uuid &secondPartId) {
        return centerOffsetMap[firstPartId] < centerOffsetMap[secondPartId];
    });
    
    size_t cutFaceTypeCount = (size_t)dust3d::CutFace::UserDefined;
    for (size_t i = 0; i < (size_t)cutFaceTypeCount; ++i) {
        dust3d::CutFace cutFace = (dust3d::CutFace)i;
        cutFaces.push_back(QString(dust3d::CutFaceToString(cutFace).c_str()));
    }
    
    for (const auto &it: cutFacePartIdList)
        cutFaces.push_back(QString(it.toString().c_str()));
}
