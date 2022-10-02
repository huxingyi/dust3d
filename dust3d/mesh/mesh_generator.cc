/*
 *  Copyright (c) 2016-2021 Jeremy HU <jeremy-at-dust3d dot org>. All rights reserved. 
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:

 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.

 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#include <functional>
#include <dust3d/base/string.h>
#include <dust3d/base/part_target.h>
#include <dust3d/base/part_base.h>
#include <dust3d/base/snapshot_xml.h>
#include <dust3d/base/cut_face.h>
#include <dust3d/mesh/stitch_mesh_builder.h>
#include <dust3d/mesh/stroke_mesh_builder.h>
#include <dust3d/mesh/stroke_modifier.h>
#include <dust3d/mesh/mesh_recombiner.h>
#include <dust3d/mesh/triangulate.h>
#include <dust3d/mesh/mesh_generator.h>
#include <dust3d/mesh/weld_vertices.h>
#include <dust3d/mesh/trim_vertices.h>
#include <dust3d/mesh/smooth_normal.h>
#include <dust3d/mesh/resolve_triangle_source_node.h>

namespace dust3d
{

double MeshGenerator::m_minimalRadius = 0.001;

MeshGenerator::MeshGenerator(Snapshot *snapshot) :
    m_snapshot(snapshot)
{
}

MeshGenerator::~MeshGenerator()
{
    delete m_snapshot;
    delete m_object;
}

void MeshGenerator::setId(uint64_t id)
{
    m_id = id;
}

uint64_t MeshGenerator::id()
{
    return m_id;
}

bool MeshGenerator::isSuccessful()
{
    return m_isSuccessful;
}

const std::set<Uuid> &MeshGenerator::generatedPreviewComponentIds()
{
    return m_generatedPreviewComponentIds;
}

Object *MeshGenerator::takeObject()
{
    Object *object = m_object;
    m_object = nullptr;
    return object;
}

void MeshGenerator::chamferFace(std::vector<Vector2> *face)
{
    auto oldFace = *face;
    face->clear();
    for (size_t i = 0; i < oldFace.size(); ++i) {
        size_t j = (i + 1) % oldFace.size();
        face->push_back(oldFace[i] * 0.8 + oldFace[j] * 0.2);
        face->push_back(oldFace[i] * 0.2 + oldFace[j] * 0.8);
    }
}

bool MeshGenerator::isWatertight(const std::vector<std::vector<size_t>> &faces)
{
    std::set<std::pair<size_t, size_t>> halfEdges;
    for (const auto &face: faces) {
        for (size_t i = 0; i < face.size(); ++i) {
            size_t j = (i + 1) % face.size();
            auto insertResult = halfEdges.insert({face[i], face[j]});
            if (!insertResult.second)
                return false;
        }
    }
    for (const auto &it: halfEdges) {
        if (halfEdges.find({it.second, it.first}) == halfEdges.end())
            return false;
    }
    return true;
}

void MeshGenerator::recoverQuads(const std::vector<Vector3> &vertices, const std::vector<std::vector<size_t>> &triangles, const std::set<std::pair<PositionKey, PositionKey>> &sharedQuadEdges, std::vector<std::vector<size_t>> &triangleAndQuads)
{
    std::vector<PositionKey> verticesPositionKeys;
    for (const auto &position: vertices) {
        verticesPositionKeys.push_back(PositionKey(position));
    }
    std::map<std::pair<size_t, size_t>, std::pair<size_t, size_t>> triangleEdgeMap;
    for (size_t i = 0; i < triangles.size(); i++) {
        const auto &faceIndices = triangles[i];
        if (faceIndices.size() == 3) {
            triangleEdgeMap[std::make_pair(faceIndices[0], faceIndices[1])] = std::make_pair(i, faceIndices[2]);
            triangleEdgeMap[std::make_pair(faceIndices[1], faceIndices[2])] = std::make_pair(i, faceIndices[0]);
            triangleEdgeMap[std::make_pair(faceIndices[2], faceIndices[0])] = std::make_pair(i, faceIndices[1]);
        }
    }
    std::unordered_set<size_t> unionedFaces;
    std::vector<std::vector<size_t>> newUnionedFaceIndices;
    for (const auto &edge: triangleEdgeMap) {
        if (unionedFaces.find(edge.second.first) != unionedFaces.end())
            continue;
        auto pair = std::make_pair(verticesPositionKeys[edge.first.first], verticesPositionKeys[edge.first.second]);
        if (sharedQuadEdges.find(pair) != sharedQuadEdges.end()) {
            auto oppositeEdge = triangleEdgeMap.find(std::make_pair(edge.first.second, edge.first.first));
            if (oppositeEdge == triangleEdgeMap.end()) {
                //void
            } else {
                if (unionedFaces.find(oppositeEdge->second.first) == unionedFaces.end()) {
                    unionedFaces.insert(edge.second.first);
                    unionedFaces.insert(oppositeEdge->second.first);
                    std::vector<size_t> indices;
                    indices.push_back(edge.second.second);
                    indices.push_back(edge.first.first);
                    indices.push_back(oppositeEdge->second.second);
                    indices.push_back(edge.first.second);
                    triangleAndQuads.push_back(indices);
                }
            }
        }
    }
    for (size_t i = 0; i < triangles.size(); i++) {
        if (unionedFaces.find(i) == unionedFaces.end()) {
            triangleAndQuads.push_back(triangles[i]);
        }
    }
}

void MeshGenerator::collectParts()
{
    for (const auto &node: m_snapshot->nodes) {
        std::string partId = String::valueOrEmpty(node.second, "partId");
        if (partId.empty())
            continue;
        m_partNodeIds[partId].insert(node.first);
    }
    for (const auto &edge: m_snapshot->edges) {
        std::string partId = String::valueOrEmpty(edge.second, "partId");
        if (partId.empty())
            continue;
        m_partEdgeIds[partId].insert(edge.first);
    }
}

bool MeshGenerator::checkIsPartDirty(const std::string &partIdString)
{
    auto findPart = m_snapshot->parts.find(partIdString);
    if (findPart == m_snapshot->parts.end()) {
        return false;
    }
    return String::isTrue(String::valueOrEmpty(findPart->second, "__dirty"));
}

bool MeshGenerator::checkIsPartDependencyDirty(const std::string &partIdString)
{
    auto findPart = m_snapshot->parts.find(partIdString);
    if (findPart == m_snapshot->parts.end()) {
        return false;
    }
    std::string cutFaceString = String::valueOrEmpty(findPart->second, "cutFace");
    Uuid cutFaceLinkedPartId = Uuid(cutFaceString);
    if (!cutFaceLinkedPartId.isNull()) {
        if (checkIsPartDirty(cutFaceString))
            return true;
    }
    for (const auto &nodeIdString: m_partNodeIds[partIdString]) {
        auto findNode = m_snapshot->nodes.find(nodeIdString);
        if (findNode == m_snapshot->nodes.end()) {
            continue;
        }
        std::string cutFaceString = String::valueOrEmpty(findNode->second, "cutFace");
        Uuid cutFaceLinkedPartId = Uuid(cutFaceString);
        if (!cutFaceLinkedPartId.isNull()) {
            if (checkIsPartDirty(cutFaceString))
                return true;
        }
    }
    return false;
}

bool MeshGenerator::checkIsComponentDirty(const std::string &componentIdString)
{
    bool isDirty = false;
    
    const std::map<std::string, std::string> *component = &m_snapshot->rootComponent;
    if (componentIdString != to_string(Uuid())) {
        auto findComponent = m_snapshot->components.find(componentIdString);
        if (findComponent == m_snapshot->components.end()) {
            return isDirty;
        }
        component = &findComponent->second;
    }
    
    if (String::isTrue(String::valueOrEmpty(*component, "__dirty"))) {
        isDirty = true;
    }
    
    std::string linkDataType = String::valueOrEmpty(*component, "linkDataType");
    if ("partId" == linkDataType) {
        std::string partId = String::valueOrEmpty(*component, "linkData");
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
    
    for (const auto &childId: String::split(String::valueOrEmpty(*component, "children"), ',')) {
        if (childId.empty())
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
    checkIsComponentDirty(to_string(Uuid()));
}

void MeshGenerator::cutFaceStringToCutTemplate(const std::string &cutFaceString, std::vector<Vector2> &cutTemplate)
{
    Uuid cutFaceLinkedPartId = Uuid(cutFaceString);
    if (!cutFaceLinkedPartId.isNull()) {
        std::map<std::string, std::tuple<float, float, float>> cutFaceNodeMap;
        auto findCutFaceLinkedPart = m_snapshot->parts.find(cutFaceString);
        if (findCutFaceLinkedPart == m_snapshot->parts.end()) {
            // void
        } else {
            // Build node info map
            for (const auto &nodeIdString: m_partNodeIds[cutFaceString]) {
                auto findNode = m_snapshot->nodes.find(nodeIdString);
                if (findNode == m_snapshot->nodes.end()) {
                    continue;
                }
                auto &node = findNode->second;
                float radius = String::toFloat(String::valueOrEmpty(node, "radius"));
                float x = (String::toFloat(String::valueOrEmpty(node, "x")) - m_mainProfileMiddleX);
                float y = (m_mainProfileMiddleY - String::toFloat(String::valueOrEmpty(node, "y")));
                cutFaceNodeMap.insert({nodeIdString, std::make_tuple(radius, x, y)});
            }
            // Build edge link
            std::map<std::string, std::vector<std::string>> cutFaceNodeLinkMap;
            for (const auto &edgeIdString: m_partEdgeIds[cutFaceString]) {
                auto findEdge = m_snapshot->edges.find(edgeIdString);
                if (findEdge == m_snapshot->edges.end()) {
                    continue;
                }
                auto &edge = findEdge->second;
                std::string fromNodeIdString = String::valueOrEmpty(edge, "from");
                std::string toNodeIdString = String::valueOrEmpty(edge, "to");
                cutFaceNodeLinkMap[fromNodeIdString].push_back(toNodeIdString);
                cutFaceNodeLinkMap[toNodeIdString].push_back(fromNodeIdString);
            }
            // Find endpoint
            std::string endPointNodeIdString;
            std::vector<std::pair<std::string, std::tuple<float, float, float>>> endpointNodes;
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
                Vector2 sumOfPositions;
                for (const auto &it: endpointNodes) {
                    sumOfPositions += Vector2(std::get<1>(it.second), std::get<2>(it.second));
                }
                Vector2 center = sumOfPositions / endpointNodes.size();
                
                // Calculate all the directions emit from center to the endpoint,
                // choose the minimal angle, angle: (0, 0 -> -1, -1) to the direction
                const Vector3 referenceDirection = Vector3(-1, -1, 0).normalized();
                int choosenEndpoint = -1;
                float choosenRadian = 0;
                for (int i = 0; i < (int)endpointNodes.size(); ++i) {
                    const auto &it = endpointNodes[i];
                    Vector2 direction2d = (Vector2(std::get<1>(it.second), std::get<2>(it.second)) -
                        center);
                    Vector3 direction = Vector3(direction2d.x(), direction2d.y(), 0).normalized();
                    float radian = Vector3::angleBetween(referenceDirection, direction);
                    if (-1 == choosenEndpoint || radian < choosenRadian) {
                        choosenRadian = radian;
                        choosenEndpoint = i;
                    }
                }
                endPointNodeIdString = endpointNodes[choosenEndpoint].first;
            }
            // Loop all linked nodes
            std::vector<std::tuple<float, float, float, std::string>> cutFaceNodes;
            std::set<std::string> cutFaceVisitedNodeIds;
            std::function<void (const std::string &)> loopNodeLink;
            loopNodeLink = [&](const std::string &fromNodeIdString) {
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
            if (!endPointNodeIdString.empty()) {
                loopNodeLink(endPointNodeIdString);
            }
            // Fetch points from linked nodes
            std::vector<std::string> cutTemplateNames;
            cutFacePointsFromNodes(cutTemplate, cutFaceNodes, isRing, &cutTemplateNames);
        }
    }
    if (cutTemplate.size() < 3) {
        CutFace cutFace = CutFaceFromString(cutFaceString.c_str());
        cutTemplate = CutFaceToPoints(cutFace);
    }
}

void MeshGenerator::flattenLinks(const std::unordered_map<size_t, size_t> &links,
    std::vector<size_t> *array,
    bool *isCircle)
{
    if (links.empty())
        return;
    for (const auto &it: links) {
        if (links.end() == links.find(it.second)) {
            *isCircle = false;
            std::unordered_map<size_t, size_t> reversedLinks;
            for (const auto &it: links)
                reversedLinks.insert({it.second, it.first});
            size_t current = it.second;
            for (;;) {
                array->push_back(current);
                auto findNext = reversedLinks.find(current);
                if (findNext == reversedLinks.end())
                    break;
                current = findNext->second;
            }
            std::reverse(array->begin(), array->end());
            return;
        }
    }
    *isCircle = true;
    size_t startIndex = links.begin()->first;
    size_t current = startIndex;
    for (;;) {
        array->push_back(current);
        auto findNext = links.find(current);
        if (findNext == links.end())
            break;
        current = findNext->second;
        if (current == startIndex)
            break;
    }
}

std::unique_ptr<MeshCombiner::Mesh> MeshGenerator::combineStitchingMesh(const std::vector<std::string> &partIdStrings,
    GeneratedComponent &componentCache)
{
    std::vector<StitchMeshBuilder::Spline> splines;
    splines.reserve(partIdStrings.size());
    for (const auto &partIdString: partIdStrings) {
        std::vector<StitchMeshBuilder::Node> builderNodes;
        std::map<std::string, size_t> builderNodeIdStringToIndexMap;
        for (const auto &nodeIdString: m_partNodeIds[partIdString]) {
            auto findNode = m_snapshot->nodes.find(nodeIdString);
            if (findNode == m_snapshot->nodes.end()) {
                continue;
            }
            auto &node = findNode->second;
            
            float radius = String::toFloat(String::valueOrEmpty(node, "radius"));
            float x = (String::toFloat(String::valueOrEmpty(node, "x")) - m_mainProfileMiddleX);
            float y = (m_mainProfileMiddleY - String::toFloat(String::valueOrEmpty(node, "y")));
            float z = (m_sideProfileMiddleX - String::toFloat(String::valueOrEmpty(node, "z")));

            builderNodeIdStringToIndexMap.insert({nodeIdString, builderNodes.size()});
            builderNodes.emplace_back(StitchMeshBuilder::Node {
                Vector3((double)x, (double)y, (double)z), (double)radius
            });
        }

        std::unordered_map<size_t, size_t> builderNodeLinks;
        for (const auto &edgeIdString: m_partEdgeIds[partIdString]) {
            auto findEdge = m_snapshot->edges.find(edgeIdString);
            if (findEdge == m_snapshot->edges.end()) {
                continue;
            }
            auto &edge = findEdge->second;
            
            std::string fromNodeIdString = String::valueOrEmpty(edge, "from");
            std::string toNodeIdString = String::valueOrEmpty(edge, "to");

            auto findFrom = builderNodeIdStringToIndexMap.find(fromNodeIdString);
            if (findFrom == builderNodeIdStringToIndexMap.end())
                continue;
            auto findTo = builderNodeIdStringToIndexMap.find(toNodeIdString);
            if (findTo == builderNodeIdStringToIndexMap.end())
                continue;
            builderNodeLinks[findFrom->second] = findTo->second;
        }

        std::vector<size_t> orderedIndices;
        bool isCircle = false;
        bool isClosing = false;
        std::vector<StitchMeshBuilder::Node> orderedBuilderNodes;
        if (!builderNodeLinks.empty()) {
            flattenLinks(builderNodeLinks, &orderedIndices, &isCircle);
            orderedBuilderNodes.resize(orderedIndices.size());
            for (size_t i = 0; i < orderedIndices.size(); ++i)
                orderedBuilderNodes[i] = builderNodes[orderedIndices[i]];
        } else {
            orderedBuilderNodes.push_back(builderNodes[0]);
            isClosing = true;
        }

        splines.emplace_back(StitchMeshBuilder::Spline {
            std::move(orderedBuilderNodes),
            isCircle,
            isClosing
        });
    }

    auto stitchMeshBuilder = std::make_unique<StitchMeshBuilder>(std::move(splines));
    stitchMeshBuilder->build();

    collectSharedQuadEdges(stitchMeshBuilder->generatedVertices(), 
        stitchMeshBuilder->generatedFaces(),
        &componentCache.sharedQuadEdges);

    auto mesh = std::make_unique<MeshCombiner::Mesh>(stitchMeshBuilder->generatedVertices(), 
        stitchMeshBuilder->generatedFaces());
    if (mesh && mesh->isNull())
        mesh.reset();

    return mesh;
}

std::unique_ptr<MeshCombiner::Mesh> MeshGenerator::combinePartMesh(const std::string &partIdString, bool *hasError, bool *retryable, bool addIntermediateNodes)
{
    auto findPart = m_snapshot->parts.find(partIdString);
    if (findPart == m_snapshot->parts.end()) {
        return nullptr;
    }
    
    Uuid partId = Uuid(partIdString);
    auto &part = findPart->second;
    
    *retryable = true;
    
    bool isDisabled = String::isTrue(String::valueOrEmpty(part, "disabled"));
    std::string __mirroredByPartId = String::valueOrEmpty(part, "__mirroredByPartId");
    std::string __mirrorFromPartId = String::valueOrEmpty(part, "__mirrorFromPartId");
    bool subdived = String::isTrue(String::valueOrEmpty(part, "subdived"));
    bool rounded = String::isTrue(String::valueOrEmpty(part, "rounded"));
    bool chamfered = String::isTrue(String::valueOrEmpty(part, "chamfered"));
    bool countershaded = String::isTrue(String::valueOrEmpty(part, "countershaded"));
    bool smooth = String::isTrue(String::valueOrEmpty(part, "smooth"));
    std::string colorString = String::valueOrEmpty(part, "color");
    Color partColor = colorString.empty() ? m_defaultPartColor : Color(colorString);
    float deformThickness = 1.0;
    float deformWidth = 1.0;
    float cutRotation = 0.0;
    float hollowThickness = 0.0;
    auto target = PartTargetFromString(String::valueOrEmpty(part, "target").c_str());
    auto base = PartBaseFromString(String::valueOrEmpty(part, "base").c_str());
    
    std::string searchPartIdString = __mirrorFromPartId.empty() ? partIdString : __mirrorFromPartId;

    std::string cutFaceString = String::valueOrEmpty(part, "cutFace");
    std::vector<Vector2> cutTemplate;
    cutFaceStringToCutTemplate(cutFaceString, cutTemplate);
    if (chamfered)
        chamferFace(&cutTemplate);
    
    std::string cutRotationString = String::valueOrEmpty(part, "cutRotation");
    if (!cutRotationString.empty()) {
        cutRotation = String::toFloat(cutRotationString);
    }
    
    std::string hollowThicknessString = String::valueOrEmpty(part, "hollowThickness");
    if (!hollowThicknessString.empty()) {
        hollowThickness = String::toFloat(hollowThicknessString);
    }
    
    std::string thicknessString = String::valueOrEmpty(part, "deformThickness");
    if (!thicknessString.empty()) {
        deformThickness = String::toFloat(thicknessString);
    }
    
    std::string widthString = String::valueOrEmpty(part, "deformWidth");
    if (!widthString.empty()) {
        deformWidth = String::toFloat(widthString);
    }
    
    bool deformUnified = String::isTrue(String::valueOrEmpty(part, "deformUnified"));
    
    Uuid materialId;
    std::string materialIdString = String::valueOrEmpty(part, "materialId");
    if (!materialIdString.empty())
        materialId = Uuid(materialIdString);
    
    float colorSolubility = 0;
    std::string colorSolubilityString = String::valueOrEmpty(part, "colorSolubility");
    if (!colorSolubilityString.empty())
        colorSolubility = String::toFloat(colorSolubilityString);
    
    float metalness = 0;
    std::string metalnessString = String::valueOrEmpty(part, "metallic");
    if (!metalnessString.empty())
        metalness = String::toFloat(metalnessString);
    
    float roughness = 1.0;
    std::string roughnessString = String::valueOrEmpty(part, "roughness");
    if (!roughnessString.empty())
        roughness = String::toFloat(roughnessString);
    
    Uuid fillMeshFileId;
    std::string fillMeshString = String::valueOrEmpty(part, "fillMesh");
    if (!fillMeshString.empty()) {
        fillMeshFileId = Uuid(fillMeshString);
        if (!fillMeshFileId.isNull()) {
            *retryable = false;
        }
    }
    
    auto &partCache = m_cacheContext->parts[partIdString];
    partCache.objectNodes.clear();
    partCache.objectEdges.clear();
    partCache.objectNodeVertices.clear();
    partCache.vertices.clear();
    partCache.faces.clear();
    partCache.previewTriangles.clear();
    partCache.previewVertices.clear();
    partCache.isSuccessful = false;
    partCache.joined = (target == PartTarget::Model && !isDisabled);
    
    struct NodeInfo
    {
        float radius = 0;
        Vector3 position;
        bool hasCutFaceSettings = false;
        float cutRotation = 0.0;
        std::string cutFace;
    };
    std::map<std::string, NodeInfo> nodeInfos;
    for (const auto &nodeIdString: m_partNodeIds[searchPartIdString]) {
        auto findNode = m_snapshot->nodes.find(nodeIdString);
        if (findNode == m_snapshot->nodes.end()) {
            continue;
        }
        auto &node = findNode->second;
        
        float radius = String::toFloat(String::valueOrEmpty(node, "radius"));
        float x = (String::toFloat(String::valueOrEmpty(node, "x")) - m_mainProfileMiddleX);
        float y = (m_mainProfileMiddleY - String::toFloat(String::valueOrEmpty(node, "y")));
        float z = (m_sideProfileMiddleX - String::toFloat(String::valueOrEmpty(node, "z")));

        bool hasCutFaceSettings = false;
        float cutRotation = 0.0;
        std::string cutFace;
        
        const auto &cutFaceIt = node.find("cutFace");
        if (cutFaceIt != node.end()) {
            cutFace = cutFaceIt->second;
            hasCutFaceSettings = true;
            const auto &cutRotationIt = node.find("cutRotation");
            if (cutRotationIt != node.end()) {
                cutRotation = String::toFloat(cutRotationIt->second);
            }
        }
        
        auto &nodeInfo = nodeInfos[nodeIdString];
        nodeInfo.position = Vector3(x, y, z);
        nodeInfo.radius = radius;
        nodeInfo.hasCutFaceSettings = hasCutFaceSettings;
        nodeInfo.cutRotation = cutRotation;
        nodeInfo.cutFace = cutFace;
    }
    
    std::set<std::pair<std::string, std::string>> edges;
    for (const auto &edgeIdString: m_partEdgeIds[searchPartIdString]) {
        auto findEdge = m_snapshot->edges.find(edgeIdString);
        if (findEdge == m_snapshot->edges.end()) {
            continue;
        }
        auto &edge = findEdge->second;
        
        std::string fromNodeIdString = String::valueOrEmpty(edge, "from");
        std::string toNodeIdString = String::valueOrEmpty(edge, "to");
        
        const auto &findFromNodeInfo = nodeInfos.find(fromNodeIdString);
        if (findFromNodeInfo == nodeInfos.end()) {
            continue;
        }
        
        const auto &findToNodeInfo = nodeInfos.find(toNodeIdString);
        if (findToNodeInfo == nodeInfos.end()) {
            continue;
        }
        
        edges.insert({fromNodeIdString, toNodeIdString});
    }
    
    bool buildSucceed = false;
    std::map<std::string, int> nodeIdStringToIndexMap;
    std::map<int, std::string> nodeIndexToIdStringMap;
    
    auto addNodeToPartCache = [&](const std::string &nodeIdString, const NodeInfo &nodeInfo) {
        ObjectNode objectNode;
        objectNode.partId = Uuid(partIdString);
        objectNode.nodeId = Uuid(nodeIdString);
        objectNode.origin = nodeInfo.position;
        objectNode.radius = nodeInfo.radius;
        objectNode.color = partColor;
        objectNode.materialId = materialId;
        objectNode.countershaded = countershaded;
        objectNode.colorSolubility = colorSolubility;
        objectNode.metalness = metalness;
        objectNode.roughness = roughness;
        if (!__mirroredByPartId.empty())
            objectNode.mirroredByPartId = Uuid(__mirroredByPartId);
        if (!__mirrorFromPartId.empty()) {
            objectNode.mirrorFromPartId = Uuid(__mirrorFromPartId);
            objectNode.origin.setX(-nodeInfo.position.x());
        }
        objectNode.joined = partCache.joined;
        partCache.objectNodes.push_back(objectNode);
    };
    auto addEdgeToPartCache = [&](const std::string &firstNodeIdString, const std::string &secondNodeIdString) {
        partCache.objectEdges.push_back({
            {Uuid(partIdString), Uuid(firstNodeIdString)},
            {Uuid(partIdString), Uuid(secondNodeIdString)}
        });
    };
    
    auto strokeModifier = std::make_unique<StrokeModifier>();
    
    if (smooth)
        strokeModifier->enableSmooth();
    if (addIntermediateNodes)
        strokeModifier->enableIntermediateAddition();
    
    for (const auto &nodeIt: nodeInfos) {
        const auto &nodeIdString = nodeIt.first;
        const auto &nodeInfo = nodeIt.second;
        size_t nodeIndex = 0;
        if (nodeInfo.hasCutFaceSettings) {
            std::vector<Vector2> nodeCutTemplate;
            cutFaceStringToCutTemplate(nodeInfo.cutFace, nodeCutTemplate);
            if (chamfered)
                chamferFace(&nodeCutTemplate);
            nodeIndex = strokeModifier->addNode(nodeInfo.position, nodeInfo.radius, nodeCutTemplate, nodeInfo.cutRotation);
        } else {
            nodeIndex = strokeModifier->addNode(nodeInfo.position, nodeInfo.radius, cutTemplate, cutRotation);
        }
        nodeIdStringToIndexMap[nodeIdString] = nodeIndex;
        nodeIndexToIdStringMap[nodeIndex] = nodeIdString;
    }
    
    for (const auto &edgeIt: edges) {
        const std::string &fromNodeIdString = edgeIt.first;
        const std::string &toNodeIdString = edgeIt.second;
        
        auto findFromNodeIndex = nodeIdStringToIndexMap.find(fromNodeIdString);
        if (findFromNodeIndex == nodeIdStringToIndexMap.end()) {
            continue;
        }
        
        auto findToNodeIndex = nodeIdStringToIndexMap.find(toNodeIdString);
        if (findToNodeIndex == nodeIdStringToIndexMap.end()) {
            continue;
        }
        
        strokeModifier->addEdge(findFromNodeIndex->second, findToNodeIndex->second);
    }
    
    if (subdived)
        strokeModifier->subdivide();
    
    if (rounded)
        strokeModifier->roundEnd();
    
    strokeModifier->finalize();
    
    std::vector<size_t> sourceNodeIndices;
    
    auto strokeMeshBuilder = std::make_unique<StrokeMeshBuilder>();
        
    strokeMeshBuilder->setDeformThickness(deformThickness);
    strokeMeshBuilder->setDeformWidth(deformWidth);
    strokeMeshBuilder->setDeformUnified(deformUnified);
    strokeMeshBuilder->setHollowThickness(hollowThickness);
    if (PartBase::YZ == base) {
        strokeMeshBuilder->enableBaseNormalOnX(false);
    } else if (PartBase::Average == base) {
        strokeMeshBuilder->enableBaseNormalAverage(true);
    } else if (PartBase::XY == base) {
        strokeMeshBuilder->enableBaseNormalOnZ(false);
    } else if (PartBase::ZX == base) {
        strokeMeshBuilder->enableBaseNormalOnY(false);
    }
    
    for (const auto &node: strokeModifier->nodes()) {
        auto nodeIndex = strokeMeshBuilder->addNode(node.position, node.radius, node.cutTemplate, node.cutRotation);
        strokeMeshBuilder->setNodeOriginInfo(nodeIndex, node.nearOriginNodeIndex, node.farOriginNodeIndex);
    }
    for (const auto &edge: strokeModifier->edges())
        strokeMeshBuilder->addEdge(edge.firstNodeIndex, edge.secondNodeIndex);
    
    for (const auto &nodeIt: nodeInfos) {
        const auto &nodeIdString = nodeIt.first;
        const auto &nodeInfo = nodeIt.second;
        addNodeToPartCache(nodeIdString, nodeInfo);
    }
    
    for (const auto &edgeIt: edges) {
        const std::string &fromNodeIdString = edgeIt.first;
        const std::string &toNodeIdString = edgeIt.second;
        addEdgeToPartCache(fromNodeIdString, toNodeIdString);
    }

    buildSucceed = strokeMeshBuilder->build();
    
    partCache.vertices = strokeMeshBuilder->generatedVertices();
    partCache.faces = strokeMeshBuilder->generatedFaces();
    if (!__mirrorFromPartId.empty()) {
        for (auto &it: partCache.vertices)
            it.setX(-it.x());
        for (auto &it: partCache.faces)
            std::reverse(it.begin(), it.end());
    }
    sourceNodeIndices = strokeMeshBuilder->generatedVerticesSourceNodeIndices();
    for (size_t i = 0; i < partCache.vertices.size(); ++i) {
        const auto &position = partCache.vertices[i];
        const auto &source = strokeMeshBuilder->generatedVerticesSourceNodeIndices()[i];
        size_t nodeIndex = strokeModifier->nodes()[source].originNodeIndex;
        const auto &nodeIdString = nodeIndexToIdStringMap[nodeIndex];
        partCache.objectNodeVertices.push_back({position, {partIdString, nodeIdString}});
    }
    
    bool hasMeshError = false;
    std::unique_ptr<MeshCombiner::Mesh> mesh;
    
    if (buildSucceed) {
        mesh = std::make_unique<MeshCombiner::Mesh>(partCache.vertices, partCache.faces);
        if (mesh->isNull()) {
            hasMeshError = true;
        }
    } else {
        hasMeshError = true;
    }
    
    if (mesh && mesh->isNull()) {
        mesh.reset();
    }
    
    if (isDisabled) {
        mesh.reset();
    }
    
    if (target != PartTarget::Model) {
        mesh.reset();
    }
    
    if (hasMeshError && target == PartTarget::Model) {
        *hasError = true;
    }
    
    return mesh;
}

const std::map<std::string, std::string> *MeshGenerator::findComponent(const std::string &componentIdString)
{
    const std::map<std::string, std::string> *component = &m_snapshot->rootComponent;
    if (componentIdString != to_string(Uuid())) {
        auto findComponent = m_snapshot->components.find(componentIdString);
        if (findComponent == m_snapshot->components.end()) {
            return nullptr;
        }
        return &findComponent->second;
    }
    return component;
}

CombineMode MeshGenerator::componentCombineMode(const std::map<std::string, std::string> *component)
{
    if (nullptr == component)
        return CombineMode::Normal;
    CombineMode combineMode = CombineModeFromString(String::valueOrEmpty(*component, "combineMode").c_str());
    if (combineMode == CombineMode::Normal) {
        if (String::isTrue(String::valueOrEmpty(*component, "inverse")))
            combineMode = CombineMode::Inversion;
    }
    return combineMode;
}

std::string MeshGenerator::componentColorName(const std::map<std::string, std::string> *component)
{
    if (nullptr == component)
        return std::string();
    std::string linkDataType = String::valueOrEmpty(*component, "linkDataType");
    if ("partId" == linkDataType) {
        std::string partIdString = String::valueOrEmpty(*component, "linkData");
        auto findPart = m_snapshot->parts.find(partIdString);
        if (findPart == m_snapshot->parts.end()) {
            return std::string();
        }
        auto &part = findPart->second;
        std::string colorSolubility = String::valueOrEmpty(part, "colorSolubility");
        if (!colorSolubility.empty()) {
            return std::string("+");
        }
        std::string colorName = String::valueOrEmpty(part, "color");
        if (colorName.empty())
            return std::string("-");
        return colorName;
    }
    return std::string();
}

std::unique_ptr<MeshCombiner::Mesh> MeshGenerator::combineComponentMesh(const std::string &componentIdString, CombineMode *combineMode)
{
    std::unique_ptr<MeshCombiner::Mesh> mesh;
    
    Uuid componentId;
    const std::map<std::string, std::string> *component = &m_snapshot->rootComponent;
    if (componentIdString != to_string(Uuid())) {
        componentId = Uuid(componentIdString);
        auto findComponent = m_snapshot->components.find(componentIdString);
        if (findComponent == m_snapshot->components.end()) {
            return nullptr;
        }
        component = &findComponent->second;
    }

    *combineMode = componentCombineMode(component);
    
    auto &componentCache = m_cacheContext->components[componentIdString];
    
    if (m_cacheEnabled) {
        if (m_dirtyComponentIds.find(componentIdString) == m_dirtyComponentIds.end()) {
            if (nullptr != componentCache.mesh)
                return std::make_unique<MeshCombiner::Mesh>(*componentCache.mesh);
        }
    }
    
    componentCache.sharedQuadEdges.clear();
    componentCache.noneSeamVertices.clear();
    componentCache.objectNodes.clear();
    componentCache.objectEdges.clear();
    componentCache.objectNodeVertices.clear();
    componentCache.mesh.reset();
    
    std::string linkDataType = String::valueOrEmpty(*component, "linkDataType");
    if ("partId" == linkDataType) {
        std::string partIdString = String::valueOrEmpty(*component, "linkData");
        bool hasError = false;
        bool retryable = true;
        mesh = combinePartMesh(partIdString, &hasError, &retryable, m_interpolationEnabled);
        if (hasError) {
            mesh.reset();
            if (retryable && m_interpolationEnabled) {
                hasError = false;
                mesh = combinePartMesh(partIdString, &hasError, &retryable, false);
            }
            if (hasError) {
                m_isSuccessful = false;
            }
        }
        const auto &partCache = m_cacheContext->parts[partIdString];
        for (const auto &vertex: partCache.vertices)
            componentCache.noneSeamVertices.insert(vertex);
        collectSharedQuadEdges(partCache.vertices, partCache.faces, &componentCache.sharedQuadEdges);
        for (const auto &it: partCache.objectNodes)
            componentCache.objectNodes.push_back(it);
        for (const auto &it: partCache.objectEdges)
            componentCache.objectEdges.push_back(it);
        for (const auto &it: partCache.objectNodeVertices)
            componentCache.objectNodeVertices.push_back(it);
        addComponentPreview(componentId, ComponentPreview {partCache.vertices, partCache.faces});
    } else {
        std::vector<std::pair<CombineMode, std::vector<std::string>>> combineGroups;
        int currentGroupIndex = -1;
        auto lastCombineMode = CombineMode::Count;
        std::vector<std::string> stitchingParts;
        std::vector<std::string> stitchingComponents;
        for (const auto &childIdString: String::split(String::valueOrEmpty(*component, "children"), ',')) {
            if (childIdString.empty())
                continue;
            const auto &child = findComponent(childIdString);
            if (nullptr == child)
                continue;
            if ("partId" == String::valueOrEmpty(*child, "linkDataType")) {
                auto partIdString = String::valueOrEmpty(*child, "linkData");
                auto findPart = m_snapshot->parts.find(partIdString);
                if (findPart != m_snapshot->parts.end()) {
                    if ("StitchingLine" == String::valueOrEmpty(findPart->second, "target")) {
                        stitchingParts.emplace_back(partIdString);
                        stitchingComponents.emplace_back(childIdString);
                        continue;
                    }
                }
            }
            auto combineMode = componentCombineMode(child);
            if (lastCombineMode != combineMode || lastCombineMode == CombineMode::Inversion) {
                combineGroups.push_back({combineMode, {}});
                ++currentGroupIndex;
                lastCombineMode = combineMode;
            }
            if (-1 == currentGroupIndex) {
                continue;
            }
            combineGroups[currentGroupIndex].second.push_back(childIdString);
        }
        std::vector<std::tuple<std::unique_ptr<MeshCombiner::Mesh>, CombineMode, std::string>> groupMeshes;
        for (const auto &group: combineGroups) {
            auto childMesh = combineComponentChildGroupMesh(group.second, componentCache);
            if (nullptr == childMesh || childMesh->isNull())
                continue;
            groupMeshes.emplace_back(std::make_tuple(std::move(childMesh), group.first, String::join(group.second, "|")));
        }
        if (!stitchingParts.empty()) {
            auto stitchingMesh = combineStitchingMesh(stitchingParts, componentCache);
            if (stitchingMesh && !stitchingMesh->isNull())
                groupMeshes.emplace_back(std::make_tuple(std::move(stitchingMesh), CombineMode::Normal, String::join(stitchingComponents, ":")));
        }
        mesh = combineMultipleMeshes(std::move(groupMeshes), true);
        ComponentPreview preview;
        if (mesh)
            mesh->fetch(preview.vertices, preview.triangles);
        addComponentPreview(componentId, std::move(preview));
    }
    
    if (nullptr != mesh)
        componentCache.mesh = std::make_unique<MeshCombiner::Mesh>(*mesh);
    
    if (nullptr != mesh && mesh->isNull()) {
        mesh.reset();
    }
    
    return mesh;
}

std::unique_ptr<MeshCombiner::Mesh> MeshGenerator::combineMultipleMeshes(std::vector<std::tuple<std::unique_ptr<MeshCombiner::Mesh>, CombineMode, std::string>> &&multipleMeshes, bool recombine)
{
    std::unique_ptr<MeshCombiner::Mesh> mesh;
    std::string meshIdStrings;
    for (auto &it: multipleMeshes) {
        auto subMesh = std::move(std::get<0>(it));
        const auto &childCombineMode = std::get<1>(it);
        const std::string &subMeshIdString = std::get<2>(it);
        if (nullptr == subMesh || subMesh->isNull()) {
            continue;
        }
        if (nullptr == mesh) {
            mesh = std::move(subMesh);
            meshIdStrings = subMeshIdString;
            continue;
        } 
        auto combinerMethod = childCombineMode == CombineMode::Inversion ?
                MeshCombiner::Method::Diff : MeshCombiner::Method::Union;
        auto combinerMethodString = combinerMethod == MeshCombiner::Method::Union ?
            "+" : "-";
        meshIdStrings += combinerMethodString + subMeshIdString;
        if (recombine)
            meshIdStrings += "!";
        std::unique_ptr<MeshCombiner::Mesh> newMesh;
        auto findCached = m_cacheContext->cachedCombination.find(meshIdStrings);
        if (findCached != m_cacheContext->cachedCombination.end()) {
            if (nullptr != findCached->second) {
                newMesh = std::make_unique<MeshCombiner::Mesh>(*findCached->second);
            }
        } else {
            newMesh = combineTwoMeshes(*mesh,
                *subMesh,
                combinerMethod,
                recombine);
            if (nullptr != newMesh)
                m_cacheContext->cachedCombination.insert({meshIdStrings, std::make_unique<MeshCombiner::Mesh>(*newMesh)});
            else
                m_cacheContext->cachedCombination.insert({meshIdStrings, nullptr});
        }
        if (newMesh && !newMesh->isNull()) {
            mesh = std::move(newMesh);
        } else {
            m_isSuccessful = false;
        }
    }
    if (nullptr != mesh && mesh->isNull()) {
        mesh.reset();
    }
    return mesh;
}

std::unique_ptr<MeshCombiner::Mesh> MeshGenerator::combineComponentChildGroupMesh(const std::vector<std::string> &componentIdStrings, GeneratedComponent &componentCache)
{
    std::vector<std::tuple<std::unique_ptr<MeshCombiner::Mesh>, CombineMode, std::string>> multipleMeshes;
    for (const auto &childIdString: componentIdStrings) {
        CombineMode childCombineMode = CombineMode::Normal;
        std::unique_ptr<MeshCombiner::Mesh> subMesh = combineComponentMesh(childIdString, &childCombineMode);
        
        if (CombineMode::Uncombined == childCombineMode) {
            continue;
        }
        
        const auto &childComponentCache = m_cacheContext->components[childIdString];
        for (const auto &vertex: childComponentCache.noneSeamVertices)
            componentCache.noneSeamVertices.insert(vertex);
        for (const auto &it: childComponentCache.sharedQuadEdges)
            componentCache.sharedQuadEdges.insert(it);
        for (const auto &it: childComponentCache.objectNodes)
            componentCache.objectNodes.push_back(it);
        for (const auto &it: childComponentCache.objectEdges)
            componentCache.objectEdges.push_back(it);
        for (const auto &it: childComponentCache.objectNodeVertices)
            componentCache.objectNodeVertices.push_back(it);
        
        if (nullptr == subMesh || subMesh->isNull()) {
            continue;
        }
    
        multipleMeshes.emplace_back(std::make_tuple(std::move(subMesh), childCombineMode, childIdString));
    }
    return combineMultipleMeshes(std::move(multipleMeshes));
}

std::unique_ptr<MeshCombiner::Mesh> MeshGenerator::combineTwoMeshes(const MeshCombiner::Mesh &first, const MeshCombiner::Mesh &second,
    MeshCombiner::Method method,
    bool recombine)
{
    if (first.isNull() || second.isNull())
        return nullptr;
    std::vector<std::pair<MeshCombiner::Source, size_t>> combinedVerticesSources;
    auto newMesh = std::unique_ptr<MeshCombiner::Mesh>(MeshCombiner::combine(first,
        second,
        method,
        &combinedVerticesSources));
    if (nullptr == newMesh)
        return nullptr;
    if (!newMesh->isNull() && recombine) {
        MeshRecombiner recombiner;
        std::vector<Vector3> combinedVertices;
        std::vector<std::vector<size_t>> combinedFaces;
        newMesh->fetch(combinedVertices, combinedFaces);
        recombiner.setVertices(&combinedVertices, &combinedVerticesSources);
        recombiner.setFaces(&combinedFaces);
        if (recombiner.recombine()) {
            if (isWatertight(recombiner.regeneratedFaces())) {
                auto reMesh = std::make_unique<MeshCombiner::Mesh>(recombiner.regeneratedVertices(), recombiner.regeneratedFaces());
                if (!reMesh->isNull()) {
                    newMesh = std::move(reMesh);
                }
            }
        }
    }
    if (newMesh->isNull()) {
        return nullptr;
    }
    return newMesh;
}

void MeshGenerator::makeXmirror(const std::vector<Vector3> &sourceVertices, const std::vector<std::vector<size_t>> &sourceFaces,
        std::vector<Vector3> *destVertices, std::vector<std::vector<size_t>> *destFaces)
{
    for (const auto &mirrorFrom: sourceVertices) {
        destVertices->push_back(Vector3(-mirrorFrom.x(), mirrorFrom.y(), mirrorFrom.z()));
    }
    std::vector<std::vector<size_t>> newFaces;
    for (const auto &mirrorFrom: sourceFaces) {
        auto newFace = mirrorFrom;
        std::reverse(newFace.begin(), newFace.end());
        destFaces->push_back(newFace);
    }
}

void MeshGenerator::collectSharedQuadEdges(const std::vector<Vector3> &vertices, const std::vector<std::vector<size_t>> &faces,
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

void MeshGenerator::setInterpolationEnabled(bool interpolationEnabled)
{
    m_interpolationEnabled = interpolationEnabled;
}

void MeshGenerator::setWeldEnabled(bool enabled)
{
    m_weldEnabled = enabled;
}

void MeshGenerator::collectErroredParts()
{
    for (const auto &it: m_cacheContext->parts) {
        if (!it.second.isSuccessful) {
            if (!it.second.joined)
                continue;
            
            auto updateVertexIndices = [=](std::vector<std::vector<size_t>> &faces, size_t vertexStartIndex) {
                for (auto &it: faces) {
                    for (auto &subIt: it)
                        subIt += vertexStartIndex;
                }
            };
            
            auto errorTriangleAndQuads = it.second.faces;
            updateVertexIndices(errorTriangleAndQuads, m_object->vertices.size());
            m_object->vertices.insert(m_object->vertices.end(), it.second.vertices.begin(), it.second.vertices.end());
            m_object->triangleAndQuads.insert(m_object->triangleAndQuads.end(), errorTriangleAndQuads.begin(), errorTriangleAndQuads.end());
            
            auto errorTriangles = it.second.previewTriangles;
            updateVertexIndices(errorTriangles, m_object->vertices.size());
            m_object->vertices.insert(m_object->vertices.end(), it.second.previewVertices.begin(), it.second.previewVertices.end());
            m_object->triangles.insert(m_object->triangles.end(), errorTriangles.begin(), errorTriangles.end());
        }
    }
}

void MeshGenerator::postprocessObject(Object *object) 
{
    std::vector<Vector3> combinedFacesNormals;
    for (const auto &face: object->triangles) {
        combinedFacesNormals.push_back(Vector3::normal(
            object->vertices[face[0]],
            object->vertices[face[1]],
            object->vertices[face[2]]
        ));
    }
    
    object->triangleNormals = combinedFacesNormals;
    
    std::vector<std::pair<Uuid, Uuid>> sourceNodes;
    resolveTriangleSourceNode(*object, m_nodeVertices, sourceNodes, &object->vertexSourceNodes);
    object->setTriangleSourceNodes(sourceNodes);
    
    std::map<std::pair<Uuid, Uuid>, Color> sourceNodeToColorMap;
    for (const auto &node: object->nodes)
        sourceNodeToColorMap.insert({{node.partId, node.nodeId}, node.color});
    
    object->triangleColors.resize(object->triangles.size(), Color::createWhite());
    const std::vector<std::pair<Uuid, Uuid>> *triangleSourceNodes = object->triangleSourceNodes();
    if (nullptr != triangleSourceNodes) {
        for (size_t triangleIndex = 0; triangleIndex < object->triangles.size(); triangleIndex++) {
            const auto &source = (*triangleSourceNodes)[triangleIndex];
            object->triangleColors[triangleIndex] = sourceNodeToColorMap[source];
        }
    }
    
    std::vector<std::vector<Vector3>> triangleVertexNormals;
    smoothNormal(object->vertices,
        object->triangles,
        object->triangleNormals,
        m_smoothShadingThresholdAngleDegrees,
        &triangleVertexNormals);
    object->setTriangleVertexNormals(triangleVertexNormals);
}

void MeshGenerator::collectIncombinableMesh(const MeshCombiner::Mesh *mesh, const GeneratedComponent &componentCache)
{
    if (nullptr == mesh)
        return;

    std::vector<Vector3> uncombinedVertices;
    std::vector<std::vector<size_t>> uncombinedFaces;
    mesh->fetch(uncombinedVertices, uncombinedFaces);
    std::vector<std::vector<size_t>> uncombinedTriangleAndQuads;
    
    recoverQuads(uncombinedVertices, uncombinedFaces, componentCache.sharedQuadEdges, uncombinedTriangleAndQuads);
    
    auto vertexStartIndex = m_object->vertices.size();
    auto updateVertexIndices = [=](std::vector<std::vector<size_t>> &faces) {
        for (auto &it: faces) {
            for (auto &subIt: it)
                subIt += vertexStartIndex;
        }
    };
    updateVertexIndices(uncombinedFaces);
    updateVertexIndices(uncombinedTriangleAndQuads);
    
    m_object->vertices.insert(m_object->vertices.end(), uncombinedVertices.begin(), uncombinedVertices.end());
    m_object->triangles.insert(m_object->triangles.end(), uncombinedFaces.begin(), uncombinedFaces.end());
    m_object->triangleAndQuads.insert(m_object->triangleAndQuads.end(), uncombinedTriangleAndQuads.begin(), uncombinedTriangleAndQuads.end());
}

void MeshGenerator::collectUncombinedComponent(const std::string &componentIdString)
{
    const auto &component = findComponent(componentIdString);
    if (CombineMode::Uncombined == componentCombineMode(component)) {
        const auto &componentCache = m_cacheContext->components[componentIdString];
        if (nullptr == componentCache.mesh || componentCache.mesh->isNull()) {
            return;
        }
        
        m_object->nodes.insert(m_object->nodes.end(), componentCache.objectNodes.begin(), componentCache.objectNodes.end());
        m_object->edges.insert(m_object->edges.end(), componentCache.objectEdges.begin(), componentCache.objectEdges.end());
        m_nodeVertices.insert(m_nodeVertices.end(), componentCache.objectNodeVertices.begin(), componentCache.objectNodeVertices.end());
        
        collectIncombinableMesh(componentCache.mesh.get(), componentCache);
        return;
    }
    for (const auto &childIdString: String::split(String::valueOrEmpty(*component, "children"), ',')) {
        if (childIdString.empty())
            continue;
        collectUncombinedComponent(childIdString);
    }
}

void MeshGenerator::setDefaultPartColor(const Color &color)
{
    m_defaultPartColor = color;
}

std::string MeshGenerator::reverseUuid(const std::string &uuidString)
{
    Uuid uuid(uuidString);
    std::string newIdString = to_string(uuid);
    std::string newRawId = newIdString.substr(1, 8) +
        newIdString.substr(10, 4) +
        newIdString.substr(15, 4) +
        newIdString.substr(20, 4) +
        newIdString.substr(25, 12);
    std::reverse(newRawId.begin(), newRawId.end());
    return "{" + newRawId.substr(0, 8) + "-" +
        newRawId.substr(8, 4) + "-" +
        newRawId.substr(12, 4) + "-" +
        newRawId.substr(16, 4) + "-" +
        newRawId.substr(20, 12) + "}";
}

void MeshGenerator::preprocessMirror()
{
    std::vector<std::map<std::string, std::string>> newParts;
    std::map<std::string, std::string> partOldToNewMap;
    for (auto &partIt: m_snapshot->parts) {
        bool xMirrored = String::isTrue(String::valueOrEmpty(partIt.second, "xMirrored"));
        if (!xMirrored)
            continue;
        std::map<std::string, std::string> mirroredPart = partIt.second;
        
        std::string newPartIdString = reverseUuid(mirroredPart["id"]);
        partOldToNewMap.insert({mirroredPart["id"], newPartIdString});

        mirroredPart["__mirrorFromPartId"] = mirroredPart["id"];
        mirroredPart["id"] = newPartIdString;
        mirroredPart["__dirty"] = "true";
        newParts.push_back(mirroredPart);
    }
    
    for (const auto &it: partOldToNewMap)
        m_snapshot->parts[it.second]["__mirroredByPartId"] = it.first;
    
    std::map<std::string, std::string> parentMap;
    for (auto &componentIt: m_snapshot->components) {
        for (const auto &childId: String::split(String::valueOrEmpty(componentIt.second, "children"), ',')) {
            if (childId.empty())
                continue;
            parentMap[childId] = componentIt.first;
        }
    }
    for (const auto &childId: String::split(String::valueOrEmpty(m_snapshot->rootComponent, "children"), ',')) {
        if (childId.empty())
            continue;
        parentMap[childId] = std::string();
    }
    
    std::vector<std::map<std::string, std::string>> newComponents;
    for (auto &componentIt: m_snapshot->components) {
        std::string linkDataType = String::valueOrEmpty(componentIt.second, "linkDataType");
        if ("partId" != linkDataType)
            continue;
        std::string partIdString = String::valueOrEmpty(componentIt.second, "linkData");
        auto findPart = partOldToNewMap.find(partIdString);
        if (findPart == partOldToNewMap.end())
            continue;
        std::map<std::string, std::string> mirroredComponent = componentIt.second;
        std::string newComponentIdString = reverseUuid(mirroredComponent["id"]);
        mirroredComponent["linkData"] = findPart->second;
        mirroredComponent["id"] = newComponentIdString;
        mirroredComponent["__dirty"] = "true";
        parentMap[newComponentIdString] = parentMap[String::valueOrEmpty(componentIt.second, "id")];
        newComponents.push_back(mirroredComponent);
    }

    for (const auto &it: newParts) {
        m_snapshot->parts[String::valueOrEmpty(it, "id")] = it;
    }
    for (const auto &it: newComponents) {
        std::string idString = String::valueOrEmpty(it, "id");
        std::string parentIdString = parentMap[idString];
        m_snapshot->components[idString] = it;
        if (parentIdString.empty()) {
            m_snapshot->rootComponent["children"] += "," + idString;
        } else {
            m_snapshot->components[parentIdString]["children"] += "," + idString;
        }
    }
}

void MeshGenerator::addComponentPreview(const Uuid &componentId, ComponentPreview &&preview)
{
    m_generatedPreviewComponentIds.insert(componentId);
    m_generatedComponentPreviews[componentId] = std::move(preview);
}

void MeshGenerator::generate()
{
    if (nullptr == m_snapshot)
        return;

    m_isSuccessful = true;
    
    m_mainProfileMiddleX = String::toFloat(String::valueOrEmpty(m_snapshot->canvas, "originX"));
    m_mainProfileMiddleY = String::toFloat(String::valueOrEmpty(m_snapshot->canvas, "originY"));
    m_sideProfileMiddleX = String::toFloat(String::valueOrEmpty(m_snapshot->canvas, "originZ"));
    
    preprocessMirror();
    
    m_object = new Object;
    m_object->meshId = m_id;

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
                    if (std::string::npos != combinationIt->first.find(it->first)) {
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
            if (std::string::npos != combinationIt->first.find(dirtyComponentId)) {
                combinationIt = m_cacheContext->cachedCombination.erase(combinationIt);
                continue;
            }
            combinationIt++;
        }
    }
    
    m_dirtyComponentIds.insert(to_string(Uuid()));
    
    CombineMode combineMode;
    auto combinedMesh = combineComponentMesh(to_string(Uuid()), &combineMode);
    
    const auto &componentCache = m_cacheContext->components[to_string(Uuid())];
    
    m_object->nodes = componentCache.objectNodes;
    m_object->edges = componentCache.objectEdges;
    m_nodeVertices = componentCache.objectNodeVertices;
        
    std::vector<Vector3> combinedVertices;
    std::vector<std::vector<size_t>> combinedFaces;
    if (nullptr != combinedMesh) {
        combinedMesh->fetch(combinedVertices, combinedFaces);
        if (m_weldEnabled) {
            size_t totalAffectedNum = 0;
            size_t affectedNum = 0;
            do {
                std::vector<Vector3> weldedVertices;
                std::vector<std::vector<size_t>> weldedFaces;
                affectedNum = weldVertices(combinedVertices, combinedFaces,
                    m_minimalRadius, componentCache.noneSeamVertices,
                    weldedVertices, weldedFaces);
                combinedVertices = weldedVertices;
                combinedFaces = weldedFaces;
                totalAffectedNum += affectedNum;
            } while (affectedNum > 0);
        }
        recoverQuads(combinedVertices, combinedFaces, componentCache.sharedQuadEdges, m_object->triangleAndQuads);
        m_object->vertices = combinedVertices;
        m_object->triangles = combinedFaces;
    }
    
    // Recursively check uncombined components
    collectUncombinedComponent(to_string(Uuid()));
    
    collectErroredParts();
    postprocessObject(m_object);

    if (needDeleteCacheContext) {
        delete m_cacheContext;
        m_cacheContext = nullptr;
    }
}

}

