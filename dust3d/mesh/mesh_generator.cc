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

#include <dust3d/base/cut_face.h>
#include <dust3d/base/part_target.h>
#include <dust3d/base/snapshot_xml.h>
#include <dust3d/base/string.h>
#include <dust3d/mesh/mesh_generator.h>
#include <dust3d/mesh/mesh_recombiner.h>
#include <dust3d/mesh/rope_mesh.h>
#include <dust3d/mesh/smooth_normal.h>
#include <dust3d/mesh/stitch_mesh_builder.h>
#include <dust3d/mesh/triangulate.h>
#include <dust3d/mesh/trim_vertices.h>
#include <dust3d/mesh/tube_mesh_builder.h>
#include <dust3d/mesh/weld_vertices.h>
#include <functional>
#include <memory>

namespace dust3d {

double MeshGenerator::m_minimalRadius = 0.001;

MeshGenerator::MeshGenerator(Snapshot* snapshot)
    : m_snapshot(snapshot)
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

const std::set<Uuid>& MeshGenerator::generatedPreviewComponentIds()
{
    return m_generatedPreviewComponentIds;
}

Object* MeshGenerator::takeObject()
{
    Object* object = m_object;
    m_object = nullptr;
    return object;
}

Snapshot* MeshGenerator::takeSnapshot()
{
    Snapshot* snapshot = m_snapshot;
    m_snapshot = nullptr;
    return snapshot;
}

void MeshGenerator::chamferFace(std::vector<Vector2>* face)
{
    auto oldFace = *face;
    face->clear();
    for (size_t i = 0; i < oldFace.size(); ++i) {
        size_t j = (i + 1) % oldFace.size();
        face->push_back(oldFace[i] * 0.8 + oldFace[j] * 0.2);
        face->push_back(oldFace[i] * 0.2 + oldFace[j] * 0.8);
    }
}

void MeshGenerator::subdivideFace(std::vector<Vector2>* face)
{
    auto oldFace = *face;
    face->resize(oldFace.size() * 2);
    for (size_t i = 0, n = 0; i < oldFace.size(); ++i) {
        size_t h = (i + oldFace.size() - 1) % oldFace.size();
        size_t j = (i + 1) % oldFace.size();
        (*face)[n++] = oldFace[h] * 0.125 + oldFace[i] * 0.75 + oldFace[j] * 0.125;
        (*face)[n++] = (oldFace[i] + oldFace[j]) * 0.5;
    }
}

void MeshGenerator::recoverQuads(const std::vector<Vector3>& vertices, const std::vector<std::vector<size_t>>& triangles, const std::set<std::pair<PositionKey, PositionKey>>& sharedQuadEdges, std::vector<std::vector<size_t>>& triangleAndQuads)
{
    std::vector<PositionKey> verticesPositionKeys;
    for (const auto& position : vertices) {
        verticesPositionKeys.push_back(PositionKey(position));
    }
    std::map<std::pair<size_t, size_t>, std::pair<size_t, size_t>> triangleEdgeMap;
    for (size_t i = 0; i < triangles.size(); i++) {
        const auto& faceIndices = triangles[i];
        if (faceIndices.size() == 3) {
            triangleEdgeMap[std::make_pair(faceIndices[0], faceIndices[1])] = std::make_pair(i, faceIndices[2]);
            triangleEdgeMap[std::make_pair(faceIndices[1], faceIndices[2])] = std::make_pair(i, faceIndices[0]);
            triangleEdgeMap[std::make_pair(faceIndices[2], faceIndices[0])] = std::make_pair(i, faceIndices[1]);
        }
    }
    std::unordered_set<size_t> unionedFaces;
    std::vector<std::vector<size_t>> newUnionedFaceIndices;
    for (const auto& edge : triangleEdgeMap) {
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
    for (const auto& node : m_snapshot->nodes) {
        std::string partId = String::valueOrEmpty(node.second, "partId");
        if (partId.empty())
            continue;
        m_partNodeIds[partId].insert(node.first);
    }
    for (const auto& edge : m_snapshot->edges) {
        std::string partId = String::valueOrEmpty(edge.second, "partId");
        if (partId.empty())
            continue;
        m_partEdgeIds[partId].insert(edge.first);
    }
}

bool MeshGenerator::checkIsPartDirty(const std::string& partIdString)
{
    auto findPart = m_snapshot->parts.find(partIdString);
    if (findPart == m_snapshot->parts.end()) {
        return false;
    }
    return String::isTrue(String::valueOrEmpty(findPart->second, "__dirty"));
}

bool MeshGenerator::checkIsPartDependencyDirty(const std::string& partIdString)
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
    for (const auto& nodeIdString : m_partNodeIds[partIdString]) {
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

bool MeshGenerator::checkIsComponentDirty(const std::string& componentIdString)
{
    bool isDirty = false;

    const std::map<std::string, std::string>* component = &m_snapshot->rootComponent;
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

    for (const auto& childId : String::split(String::valueOrEmpty(*component, "children"), ',')) {
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

void MeshGenerator::cutFaceStringToCutTemplate(const std::string& cutFaceString, std::vector<Vector2>& cutTemplate)
{
    Uuid cutFaceLinkedPartId = Uuid(cutFaceString);
    if (!cutFaceLinkedPartId.isNull()) {
        std::map<std::string, std::tuple<float, float, float>> cutFaceNodeMap;
        auto findCutFaceLinkedPart = m_snapshot->parts.find(cutFaceString);
        if (findCutFaceLinkedPart == m_snapshot->parts.end()) {
            // void
        } else {
            // Build node info map
            for (const auto& nodeIdString : m_partNodeIds[cutFaceString]) {
                auto findNode = m_snapshot->nodes.find(nodeIdString);
                if (findNode == m_snapshot->nodes.end()) {
                    continue;
                }
                auto& node = findNode->second;
                float radius = String::toFloat(String::valueOrEmpty(node, "radius"));
                float x = (String::toFloat(String::valueOrEmpty(node, "x")) - m_mainProfileMiddleX);
                float y = (m_mainProfileMiddleY - String::toFloat(String::valueOrEmpty(node, "y")));
                cutFaceNodeMap.insert({ nodeIdString, std::make_tuple(radius, x, y) });
            }
            // Build edge link
            std::map<std::string, std::vector<std::string>> cutFaceNodeLinkMap;
            for (const auto& edgeIdString : m_partEdgeIds[cutFaceString]) {
                auto findEdge = m_snapshot->edges.find(edgeIdString);
                if (findEdge == m_snapshot->edges.end()) {
                    continue;
                }
                auto& edge = findEdge->second;
                std::string fromNodeIdString = String::valueOrEmpty(edge, "from");
                std::string toNodeIdString = String::valueOrEmpty(edge, "to");
                cutFaceNodeLinkMap[fromNodeIdString].push_back(toNodeIdString);
                cutFaceNodeLinkMap[toNodeIdString].push_back(fromNodeIdString);
            }
            // Find endpoint
            std::string endPointNodeIdString;
            std::vector<std::pair<std::string, std::tuple<float, float, float>>> endpointNodes;
            for (const auto& it : cutFaceNodeLinkMap) {
                if (1 == it.second.size()) {
                    const auto& findNode = cutFaceNodeMap.find(it.first);
                    if (findNode != cutFaceNodeMap.end())
                        endpointNodes.push_back({ it.first, findNode->second });
                }
            }
            bool isRing = endpointNodes.empty();
            if (endpointNodes.empty()) {
                for (const auto& it : cutFaceNodeMap) {
                    endpointNodes.push_back({ it.first, it.second });
                }
            }
            if (!endpointNodes.empty()) {
                // Calculate the center points
                Vector2 sumOfPositions;
                for (const auto& it : endpointNodes) {
                    sumOfPositions += Vector2(std::get<1>(it.second), std::get<2>(it.second));
                }
                Vector2 center = sumOfPositions / endpointNodes.size();

                // Calculate all the directions emit from center to the endpoint,
                // choose the minimal angle, angle: (0, 0 -> -1, -1) to the direction
                const Vector3 referenceDirection = Vector3(-1, -1, 0).normalized();
                int choosenEndpoint = -1;
                float choosenRadian = 0;
                for (int i = 0; i < (int)endpointNodes.size(); ++i) {
                    const auto& it = endpointNodes[i];
                    Vector2 direction2d = (Vector2(std::get<1>(it.second), std::get<2>(it.second)) - center);
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
            std::function<void(const std::string&)> loopNodeLink;
            loopNodeLink = [&](const std::string& fromNodeIdString) {
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
                for (const auto& it : findNeighbor->second) {
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

void MeshGenerator::flattenLinks(const std::unordered_map<size_t, size_t>& links,
    std::vector<size_t>* array,
    bool* isCircle)
{
    if (links.empty())
        return;
    for (const auto& it : links) {
        if (links.end() == links.find(it.second)) {
            *isCircle = false;
            std::unordered_map<size_t, size_t> reversedLinks;
            for (const auto& it : links)
                reversedLinks.insert({ it.second, it.first });
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

bool MeshGenerator::fetchPartOrderedNodes(const std::string& partIdString, std::vector<MeshNode>* meshNodes, bool* isCircle)
{
    std::vector<MeshNode> builderNodes;
    std::map<std::string, size_t> builderNodeIdStringToIndexMap;
    for (const auto& nodeIdString : m_partNodeIds[partIdString]) {
        auto findNode = m_snapshot->nodes.find(nodeIdString);
        if (findNode == m_snapshot->nodes.end()) {
            continue;
        }
        auto& node = findNode->second;

        float radius = String::toFloat(String::valueOrEmpty(node, "radius"));
        float x = (String::toFloat(String::valueOrEmpty(node, "x")) - m_mainProfileMiddleX);
        float y = (m_mainProfileMiddleY - String::toFloat(String::valueOrEmpty(node, "y")));
        float z = (m_sideProfileMiddleX - String::toFloat(String::valueOrEmpty(node, "z")));

        builderNodeIdStringToIndexMap.insert({ nodeIdString, builderNodes.size() });
        builderNodes.emplace_back(MeshNode {
            Vector3((double)x, (double)y, (double)z), (double)radius, Uuid(nodeIdString) });
    }

    if (builderNodes.empty()) {
        dust3dDebug << "Expected at least one node in part:" << partIdString;
        return false;
    }

    std::unordered_map<size_t, size_t> builderNodeLinks;
    for (const auto& edgeIdString : m_partEdgeIds[partIdString]) {
        auto findEdge = m_snapshot->edges.find(edgeIdString);
        if (findEdge == m_snapshot->edges.end()) {
            continue;
        }
        auto& edge = findEdge->second;

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
    if (!builderNodeLinks.empty()) {
        flattenLinks(builderNodeLinks, &orderedIndices, isCircle);
        meshNodes->resize(orderedIndices.size());
        for (size_t i = 0; i < orderedIndices.size(); ++i)
            (*meshNodes)[i] = builderNodes[orderedIndices[i]];
    } else {
        meshNodes->push_back(builderNodes[0]);
    }

    return true;
}

std::unique_ptr<MeshState> MeshGenerator::combineStitchingMesh(const std::string& componentIdString,
    const std::vector<std::string>& partIdStrings,
    const std::vector<std::string>& componentIdStrings,
    bool frontClosed,
    bool backClosed,
    bool sideClosed,
    size_t targetSegments,
    Color color,
    float smoothCutoffDegrees,
    GeneratedComponent& componentCache)
{
    std::vector<StitchMeshBuilder::Spline> splines;
    splines.reserve(partIdStrings.size());
    std::vector<Uuid> componentIds(componentIdStrings.size());
    for (size_t i = 0; i < componentIdStrings.size(); ++i)
        componentIds[i] = componentIdStrings[i];
    for (size_t partIndex = 0; partIndex < partIdStrings.size(); ++partIndex) {
        const auto& partIdString = partIdStrings[partIndex];
        auto findPart = m_snapshot->parts.find(partIdString);
        if (findPart != m_snapshot->parts.end()) {
            if (String::isTrue(String::valueOrEmpty(findPart->second, "disabled")))
                continue;
        }
        bool isCircle = false;
        std::vector<MeshNode> orderedBuilderNodes;
        if (!fetchPartOrderedNodes(partIdString, &orderedBuilderNodes, &isCircle))
            continue;
        if (isCircle)
            continue;
        if (orderedBuilderNodes.size() < 2)
            continue;
        for (const auto& meshNode : orderedBuilderNodes) {
            componentCache.nodeMap.emplace(std::make_pair(meshNode.sourceId,
                ObjectNode { meshNode.origin, color, smoothCutoffDegrees }));
        }
        splines.emplace_back(StitchMeshBuilder::Spline {
            std::move(orderedBuilderNodes),
            componentIds[partIndex] });
    }

    auto stitchMeshBuilder = std::make_unique<StitchMeshBuilder>(std::move(splines),
        frontClosed,
        backClosed,
        sideClosed,
        targetSegments);
    stitchMeshBuilder->build();

    const auto& generatedVertices = stitchMeshBuilder->generatedVertices();
    const auto& generatedFaces = stitchMeshBuilder->generatedFaces();

    collectSharedQuadEdges(generatedVertices,
        generatedFaces,
        &componentCache.sharedQuadEdges);

    auto mesh = std::make_unique<MeshState>(generatedVertices,
        generatedFaces);
    if (mesh && mesh->isNull())
        mesh.reset();

    const auto& faceUvs = stitchMeshBuilder->generatedFaceUvs();
    Uuid componentId = Uuid(componentIdString);
    auto& triangleUvs = componentCache.componentTriangleUvs[componentId];
    for (size_t i = 0; i < faceUvs.size(); ++i) {
        const auto& uv = faceUvs[i];
        const auto& face = generatedFaces[i];
        if (3 == face.size()) {
            triangleUvs.insert({ { PositionKey(generatedVertices[face[0]]),
                                     PositionKey(generatedVertices[face[1]]),
                                     PositionKey(generatedVertices[face[2]]) },
                { uv[0], uv[1], uv[2] } });
        } else if (4 == face.size()) {
            triangleUvs.insert({ { PositionKey(generatedVertices[face[0]]),
                                     PositionKey(generatedVertices[face[1]]),
                                     PositionKey(generatedVertices[face[2]]) },
                { uv[0], uv[1], uv[2] } });
            triangleUvs.insert({ { PositionKey(generatedVertices[face[2]]),
                                     PositionKey(generatedVertices[face[3]]),
                                     PositionKey(generatedVertices[face[0]]) },
                { uv[2], uv[3], uv[0] } });
        }
    }
    const auto& vertexSources = stitchMeshBuilder->generatedVertexSources();
    for (size_t i = 0; i < vertexSources.size(); ++i) {
        componentCache.positionToNodeIdMap.emplace(std::make_pair(PositionKey(generatedVertices[i]), vertexSources[i]));
    }

    // Generate preview for each stitching line
    for (const auto& spline : stitchMeshBuilder->splines()) {
        RopeMesh::BuildParameters buildParameters;
        RopeMesh ropeMesh(buildParameters);
        std::vector<Vector3> positions(spline.nodes.size());
        for (size_t i = 0; i < spline.nodes.size(); ++i)
            positions[i] = spline.nodes[i].origin;
        ropeMesh.addRope(positions, false);

        ComponentPreview stitchingLinePreview;
        if (mesh)
            mesh->fetch(stitchingLinePreview.vertices, stitchingLinePreview.triangles);
        size_t startIndex = stitchingLinePreview.vertices.size();

        stitchingLinePreview.color = Color(color[0], color[1], color[2], 0.5);
        for (const auto& ropeVertex : ropeMesh.resultVertices()) {
            stitchingLinePreview.vertices.emplace_back(ropeVertex);
        }
        stitchingLinePreview.vertexProperties.resize(stitchingLinePreview.vertices.size());
        auto modelProperty = std::tuple<dust3d::Color, float /*metalness*/, float /*roughness*/> {
            stitchingLinePreview.color,
            stitchingLinePreview.metalness,
            stitchingLinePreview.roughness
        };
        auto lineProperty = std::tuple<dust3d::Color, float /*metalness*/, float /*roughness*/> {
            Color(1.0, 1.0, 1.0, 1.0),
            stitchingLinePreview.metalness,
            stitchingLinePreview.roughness
        };
        for (size_t i = 0; i < startIndex; ++i) {
            stitchingLinePreview.vertexProperties[i] = modelProperty;
        }
        for (size_t i = startIndex; i < stitchingLinePreview.vertexProperties.size(); ++i) {
            stitchingLinePreview.vertexProperties[i] = lineProperty;
        }
        for (const auto& ropeTriangles : ropeMesh.resultTriangles()) {
            stitchingLinePreview.triangles.emplace_back(std::vector<size_t> {
                startIndex + ropeTriangles[0],
                startIndex + ropeTriangles[1],
                startIndex + ropeTriangles[2] });
        }
        addComponentPreview(spline.sourceId, ComponentPreview(stitchingLinePreview));
    }

    return mesh;
}

std::unique_ptr<MeshState> MeshGenerator::combinePartMesh(const std::string& partIdString,
    const std::string& componentIdString,
    Color color,
    float smoothCutoffDegrees,
    bool* hasError)
{
    auto findPart = m_snapshot->parts.find(partIdString);
    if (findPart == m_snapshot->parts.end()) {
        return nullptr;
    }

    Uuid partId = Uuid(partIdString);
    auto& part = findPart->second;

    bool isDisabled = String::isTrue(String::valueOrEmpty(part, "disabled"));
    std::string __mirroredByPartId = String::valueOrEmpty(part, "__mirroredByPartId");
    std::string __mirrorFromPartId = String::valueOrEmpty(part, "__mirrorFromPartId");
    bool subdived = String::isTrue(String::valueOrEmpty(part, "subdived"));
    bool rounded = String::isTrue(String::valueOrEmpty(part, "rounded"));
    bool chamfered = String::isTrue(String::valueOrEmpty(part, "chamfered"));
    float deformThickness = 1.0;
    float deformWidth = 1.0;
    float cutRotation = 0.0;
    float hollowThickness = 0.0;
    auto target = PartTargetFromString(String::valueOrEmpty(part, "target").c_str());

    std::string searchPartIdString = __mirrorFromPartId.empty() ? partIdString : __mirrorFromPartId;

    std::string cutFaceString = String::valueOrEmpty(part, "cutFace");
    std::vector<Vector2> cutTemplate;
    cutFaceStringToCutTemplate(cutFaceString, cutTemplate);
    if (chamfered)
        chamferFace(&cutTemplate);
    if (subdived)
        subdivideFace(&cutTemplate);

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

    float metalness = 0;
    std::string metalnessString = String::valueOrEmpty(part, "metallic");
    if (!metalnessString.empty())
        metalness = String::toFloat(metalnessString);

    float roughness = 1.0;
    std::string roughnessString = String::valueOrEmpty(part, "roughness");
    if (!roughnessString.empty())
        roughness = String::toFloat(roughnessString);

    std::vector<MeshNode> meshNodes;
    bool isCircle = false;
    if (!fetchPartOrderedNodes(searchPartIdString, &meshNodes, &isCircle))
        return nullptr;

    auto& partCache = m_cacheContext->parts[partIdString];
    partCache.reset();

    partCache.color = color;
    partCache.metalness = metalness;
    partCache.roughness = roughness;
    partCache.isSuccessful = false;
    partCache.joined = (target == PartTarget::Model && !isDisabled);

    partCache.nodeMap.clear();
    for (const auto& meshNode : meshNodes) {
        partCache.nodeMap.emplace(std::make_pair(meshNode.sourceId,
            ObjectNode { meshNode.origin, color, smoothCutoffDegrees }));
    }

    if (PartTarget::Model == target) {
        std::unique_ptr<TubeMeshBuilder> tubeMeshBuilder;
        TubeMeshBuilder::BuildParameters buildParameters;
        buildParameters.deformThickness = deformThickness;
        buildParameters.deformWidth = deformWidth;
        buildParameters.deformUnified = deformUnified;
        buildParameters.baseNormalRotation = cutRotation * Math::Pi;
        buildParameters.cutFace = cutTemplate;
        buildParameters.frontEndRounded = buildParameters.backEndRounded = rounded;
        tubeMeshBuilder = std::make_unique<TubeMeshBuilder>(buildParameters, std::move(meshNodes), isCircle);
        tubeMeshBuilder->build();
        partCache.vertices = tubeMeshBuilder->generatedVertices();
        partCache.faces = tubeMeshBuilder->generatedFaces();
        if (!__mirrorFromPartId.empty()) {
            for (auto& it : partCache.vertices)
                it.setX(-it.x());
            for (auto& it : partCache.faces)
                std::reverse(it.begin(), it.end());
        }
        const auto& faceUvs = tubeMeshBuilder->generatedFaceUvs();
        for (size_t i = 0; i < faceUvs.size(); ++i) {
            const auto& uv = faceUvs[i];
            const auto& face = partCache.faces[i];
            if (3 == face.size()) {
                partCache.triangleUvs.insert({ { PositionKey(partCache.vertices[face[0]]),
                                                   PositionKey(partCache.vertices[face[1]]),
                                                   PositionKey(partCache.vertices[face[2]]) },
                    { uv[0], uv[1], uv[2] } });
            } else if (4 == face.size()) {
                partCache.triangleUvs.insert({ { PositionKey(partCache.vertices[face[0]]),
                                                   PositionKey(partCache.vertices[face[1]]),
                                                   PositionKey(partCache.vertices[face[2]]) },
                    { uv[0], uv[1], uv[2] } });
                partCache.triangleUvs.insert({ { PositionKey(partCache.vertices[face[2]]),
                                                   PositionKey(partCache.vertices[face[3]]),
                                                   PositionKey(partCache.vertices[face[0]]) },
                    { uv[2], uv[3], uv[0] } });
            }
        }
        const auto& vertexSources = tubeMeshBuilder->generatedVertexSources();
        for (size_t i = 0; i < vertexSources.size(); ++i) {
            partCache.positionToNodeIdMap.emplace(std::make_pair(PositionKey(partCache.vertices[i]), vertexSources[i]));
        }
    }

    bool hasMeshError = false;
    std::unique_ptr<MeshState> mesh;

    mesh = std::make_unique<MeshState>(partCache.vertices, partCache.faces);
    if (mesh->isNull()) {
        hasMeshError = true;
    }

    if (PartTarget::Model == target) {
        ComponentPreview preview;
        if (mesh)
            mesh->fetch(preview.vertices, preview.triangles);
        preview.color = partCache.color;
        preview.metalness = partCache.metalness;
        preview.roughness = partCache.roughness;
        preview.triangleUvs = partCache.triangleUvs;
        addComponentPreview(componentIdString, std::move(preview));
    } else if (PartTarget::CutFace == target) {
        ComponentPreview preview;
        cutFaceStringToCutTemplate(partIdString, preview.cutFaceTemplate);
        addComponentPreview(componentIdString, std::move(preview));
    }

    if (nullptr != mesh) {
        partCache.isSuccessful = true;
    }

    if (mesh && mesh->isNull()) {
        mesh.reset();
    }

    if (hasMeshError && target == PartTarget::Model) {
        *hasError = true;
    }

    return mesh;
}

const std::map<std::string, std::string>* MeshGenerator::findComponent(const std::string& componentIdString)
{
    const std::map<std::string, std::string>* component = &m_snapshot->rootComponent;
    if (componentIdString != to_string(Uuid())) {
        auto findComponent = m_snapshot->components.find(componentIdString);
        if (findComponent == m_snapshot->components.end()) {
            return nullptr;
        }
        return &findComponent->second;
    }
    return component;
}

CombineMode MeshGenerator::componentCombineMode(const std::map<std::string, std::string>* component)
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

std::unique_ptr<MeshState> MeshGenerator::combineComponentMesh(const std::string& componentIdString, CombineMode* combineMode)
{
    std::unique_ptr<MeshState> mesh;

    Uuid componentId;
    const std::map<std::string, std::string>* component = &m_snapshot->rootComponent;
    if (componentIdString != to_string(Uuid())) {
        componentId = Uuid(componentIdString);
        auto findComponent = m_snapshot->components.find(componentIdString);
        if (findComponent == m_snapshot->components.end()) {
            return nullptr;
        }
        component = &findComponent->second;
    }

    *combineMode = componentCombineMode(component);

    float smoothCutoffDegrees = 0.0;
    std::string smoothCutoffDegreesString = String::valueOrEmpty(*component, "smoothCutoffDegrees");
    if (!smoothCutoffDegreesString.empty()) {
        smoothCutoffDegrees = String::toFloat(smoothCutoffDegreesString);
    }

    std::string colorString = String::valueOrEmpty(*component, "color");
    Color color = colorString.empty() ? m_defaultPartColor : Color(colorString);

    size_t targetSegments = (size_t)String::toInt(String::valueOrEmpty(*component, "targetSegments"));
    // Validate target segments, 100 should be a reasonable large number
    if (targetSegments > 100)
        targetSegments = 0;

    auto& componentCache = m_cacheContext->components[componentIdString];

    if (m_cacheEnabled) {
        if (m_dirtyComponentIds.find(componentIdString) == m_dirtyComponentIds.end()) {
            if (nullptr != componentCache.mesh)
                return std::make_unique<MeshState>(*componentCache.mesh);
        }
    }

    componentCache.reset();

    std::string linkDataType = String::valueOrEmpty(*component, "linkDataType");
    if ("partId" == linkDataType) {
        std::string partIdString = String::valueOrEmpty(*component, "linkData");
        bool hasError = false;
        mesh = combinePartMesh(partIdString, componentIdString, color, smoothCutoffDegrees, &hasError);
        if (hasError) {
            m_isSuccessful = false;
        }
        const auto& partCache = m_cacheContext->parts[partIdString];
        if (partCache.joined) {
            for (const auto& vertex : partCache.vertices)
                componentCache.noneSeamVertices.insert(vertex);
            collectSharedQuadEdges(partCache.vertices, partCache.faces, &componentCache.sharedQuadEdges);
            componentCache.componentTriangleUvs.insert({ componentId, partCache.triangleUvs });
            for (const auto& it : partCache.positionToNodeIdMap)
                componentCache.positionToNodeIdMap.emplace(it);
            for (const auto& it : partCache.nodeMap)
                componentCache.nodeMap.emplace(it);
        }
        if (!partCache.joined) {
            if (mesh)
                mesh.reset();
        }
    } else {
        std::vector<std::pair<CombineMode, std::vector<std::string>>> combineGroups;
        int currentGroupIndex = -1;
        auto lastCombineMode = CombineMode::Count;
        std::vector<std::string> stitchingParts;
        std::vector<std::string> stitchingComponents;
        for (const auto& childIdString : String::split(String::valueOrEmpty(*component, "children"), ',')) {
            if (childIdString.empty())
                continue;
            const auto& child = findComponent(childIdString);
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
                combineGroups.push_back({ combineMode, {} });
                ++currentGroupIndex;
                lastCombineMode = combineMode;
            }
            if (-1 == currentGroupIndex) {
                continue;
            }
            combineGroups[currentGroupIndex].second.push_back(childIdString);
        }
        std::vector<std::tuple<std::unique_ptr<MeshState>, CombineMode, std::string>> groupMeshes;
        for (const auto& group : combineGroups) {
            auto childMesh = combineComponentChildGroupMesh(group.second, componentCache);
            if (nullptr == childMesh || childMesh->isNull())
                continue;
            groupMeshes.emplace_back(std::make_tuple(std::move(childMesh), group.first, String::join(group.second, "|")));
        }
        if (!stitchingParts.empty()) {
            auto stitchingMesh = combineStitchingMesh(componentIdString,
                stitchingParts,
                stitchingComponents,
                String::isTrue(String::valueOrEmpty(*component, "frontClosed")),
                String::isTrue(String::valueOrEmpty(*component, "backClosed")),
                String::isTrue(String::valueOrEmpty(*component, "sideClosed")),
                targetSegments,
                color,
                smoothCutoffDegrees,
                componentCache);
            if (stitchingMesh && !stitchingMesh->isNull()) {
                groupMeshes.emplace_back(std::make_tuple(std::move(stitchingMesh), CombineMode::Normal, String::join(stitchingComponents, ":")));
            }
        }
        mesh = combineMultipleMeshes(std::move(groupMeshes));
        ComponentPreview preview;
        if (mesh) {
            mesh->fetch(preview.vertices, preview.triangles);
            preview.color = color;
            if (!stitchingParts.empty()) {
                for (const auto& it : componentCache.componentTriangleUvs) {
                    for (const auto& uvs : it.second)
                        preview.triangleUvs.insert(uvs);
                }
            }
        }
        addComponentPreview(componentId, std::move(preview));
    }

    if (nullptr != mesh)
        componentCache.mesh = std::make_unique<MeshState>(*mesh);

    if (nullptr != mesh && mesh->isNull()) {
        mesh.reset();
    }

    return mesh;
}

std::unique_ptr<MeshState> MeshGenerator::combineMultipleMeshes(std::vector<std::tuple<std::unique_ptr<MeshState>, CombineMode, std::string>>&& multipleMeshes)
{
    std::unique_ptr<MeshState> mesh;
    std::string meshIdStrings;
    for (auto& it : multipleMeshes) {
        auto subMesh = std::move(std::get<0>(it));
        const auto& childCombineMode = std::get<1>(it);
        const std::string& subMeshIdString = std::get<2>(it);
        if (nullptr == subMesh || subMesh->isNull()) {
            continue;
        }
        if (nullptr == mesh) {
            mesh = std::move(subMesh);
            meshIdStrings = subMeshIdString;
            continue;
        }
        auto combinerMethod = childCombineMode == CombineMode::Inversion ? MeshCombiner::Method::Diff : MeshCombiner::Method::Union;
        auto combinerMethodString = combinerMethod == MeshCombiner::Method::Union ? "+" : "-";
        meshIdStrings += combinerMethodString + subMeshIdString;
        std::unique_ptr<MeshState> newMesh;
        auto findCached = m_cacheContext->cachedCombination.find(meshIdStrings);
        if (findCached != m_cacheContext->cachedCombination.end()) {
            if (nullptr != findCached->second) {
                newMesh = std::make_unique<MeshState>(*findCached->second);
            }
        } else {
            newMesh = MeshState::combine(*mesh,
                *subMesh,
                combinerMethod);
            if (nullptr != newMesh)
                m_cacheContext->cachedCombination.insert({ meshIdStrings, std::make_unique<MeshState>(*newMesh) });
            else
                m_cacheContext->cachedCombination.insert({ meshIdStrings, nullptr });
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

std::unique_ptr<MeshState> MeshGenerator::combineComponentChildGroupMesh(const std::vector<std::string>& componentIdStrings, GeneratedComponent& componentCache)
{
    std::vector<std::tuple<std::unique_ptr<MeshState>, CombineMode, std::string>> multipleMeshes;
    for (const auto& childIdString : componentIdStrings) {
        CombineMode childCombineMode = CombineMode::Normal;
        std::unique_ptr<MeshState> subMesh = combineComponentMesh(childIdString, &childCombineMode);

        if (CombineMode::Uncombined == childCombineMode) {
            continue;
        }

        const auto& childComponentCache = m_cacheContext->components[childIdString];
        for (const auto& vertex : childComponentCache.noneSeamVertices)
            componentCache.noneSeamVertices.insert(vertex);
        for (const auto& it : childComponentCache.sharedQuadEdges)
            componentCache.sharedQuadEdges.insert(it);
        for (const auto& it : childComponentCache.componentTriangleUvs)
            componentCache.componentTriangleUvs.insert({ it.first, it.second });
        for (const auto& it : childComponentCache.positionToNodeIdMap)
            componentCache.positionToNodeIdMap.emplace(it);
        for (const auto& it : childComponentCache.nodeMap)
            componentCache.nodeMap.emplace(it);

        if (nullptr == subMesh || subMesh->isNull()) {
            continue;
        }

        multipleMeshes.emplace_back(std::make_tuple(std::move(subMesh), childCombineMode, childIdString));
    }
    return combineMultipleMeshes(std::move(multipleMeshes));
}

void MeshGenerator::collectSharedQuadEdges(const std::vector<Vector3>& vertices, const std::vector<std::vector<size_t>>& faces,
    std::set<std::pair<PositionKey, PositionKey>>* sharedQuadEdges)
{
    for (const auto& face : faces) {
        if (face.size() != 4)
            continue;
        sharedQuadEdges->insert({ PositionKey(vertices[face[0]]),
            PositionKey(vertices[face[2]]) });
        sharedQuadEdges->insert({ PositionKey(vertices[face[1]]),
            PositionKey(vertices[face[3]]) });
    }
}

void MeshGenerator::setGeneratedCacheContext(GeneratedCacheContext* cacheContext)
{
    m_cacheContext = cacheContext;
}

void MeshGenerator::setSmoothShadingThresholdAngleDegrees(float degrees)
{
    m_smoothShadingThresholdAngleDegrees = degrees;
}

void MeshGenerator::setWeldEnabled(bool enabled)
{
    m_weldEnabled = enabled;
}

void MeshGenerator::postprocessObject(Object* object)
{
    std::vector<Vector3> combinedFacesNormals;
    for (const auto& face : object->triangles) {
        combinedFacesNormals.push_back(Vector3::normal(
            object->vertices[face[0]],
            object->vertices[face[1]],
            object->vertices[face[2]]));
    }

    object->triangleNormals = combinedFacesNormals;

    object->vertexColors.resize(object->vertices.size(), Color::createWhite());
    object->vertexSmoothCutoffDegrees.resize(object->vertices.size(), 0.0f);
    for (size_t i = 0; i < object->vertices.size(); ++i) {
        auto findSourceNode = object->positionToNodeIdMap.find(object->vertices[i]);
        if (findSourceNode == object->positionToNodeIdMap.end())
            continue;
        auto findObjectNode = object->nodeMap.find(findSourceNode->second);
        if (findObjectNode == object->nodeMap.end())
            continue;
        object->vertexColors[i] = findObjectNode->second.color;
        object->vertexSmoothCutoffDegrees[i] = findObjectNode->second.smoothCutoffDegrees;
    }

    std::vector<std::vector<Vector3>> triangleVertexNormals;
    smoothNormal(object->vertices,
        object->triangles,
        object->triangleNormals,
        &object->vertexSmoothCutoffDegrees,
        &triangleVertexNormals);
    object->setTriangleVertexNormals(triangleVertexNormals);
}

void MeshGenerator::collectIncombinableMesh(const MeshState* mesh, const GeneratedComponent& componentCache)
{
    if (nullptr == mesh)
        return;

    std::vector<Vector3> uncombinedVertices;
    std::vector<std::vector<size_t>> uncombinedFaces;
    mesh->fetch(uncombinedVertices, uncombinedFaces);
    std::vector<std::vector<size_t>> uncombinedTriangleAndQuads;

    recoverQuads(uncombinedVertices, uncombinedFaces, componentCache.sharedQuadEdges, uncombinedTriangleAndQuads);

    auto vertexStartIndex = m_object->vertices.size();
    auto updateVertexIndices = [=](std::vector<std::vector<size_t>>& faces) {
        for (auto& it : faces) {
            for (auto& subIt : it)
                subIt += vertexStartIndex;
        }
    };
    updateVertexIndices(uncombinedFaces);
    updateVertexIndices(uncombinedTriangleAndQuads);

    for (const auto& it : componentCache.componentTriangleUvs)
        m_object->componentTriangleUvs.insert({ it.first, it.second });
    for (const auto& it : componentCache.positionToNodeIdMap)
        m_object->positionToNodeIdMap.emplace(it);
    for (const auto& it : componentCache.nodeMap)
        m_object->nodeMap.emplace(it);

    m_object->vertices.insert(m_object->vertices.end(), uncombinedVertices.begin(), uncombinedVertices.end());
    m_object->triangles.insert(m_object->triangles.end(), uncombinedFaces.begin(), uncombinedFaces.end());
    m_object->triangleAndQuads.insert(m_object->triangleAndQuads.end(), uncombinedTriangleAndQuads.begin(), uncombinedTriangleAndQuads.end());
}

void MeshGenerator::collectUncombinedComponent(const std::string& componentIdString)
{
    const auto& component = findComponent(componentIdString);
    if (CombineMode::Uncombined == componentCombineMode(component)) {
        const auto& componentCache = m_cacheContext->components[componentIdString];
        if (nullptr == componentCache.mesh || componentCache.mesh->isNull()) {
            return;
        }
        collectIncombinableMesh(componentCache.mesh.get(), componentCache);
        return;
    }
    for (const auto& childIdString : String::split(String::valueOrEmpty(*component, "children"), ',')) {
        if (childIdString.empty())
            continue;
        collectUncombinedComponent(childIdString);
    }
}

void MeshGenerator::setDefaultPartColor(const Color& color)
{
    m_defaultPartColor = color;
}

std::string MeshGenerator::reverseUuid(const std::string& uuidString)
{
    Uuid uuid(uuidString);
    std::string newIdString = to_string(uuid);
    std::string newRawId = newIdString.substr(1, 8) + newIdString.substr(10, 4) + newIdString.substr(15, 4) + newIdString.substr(20, 4) + newIdString.substr(25, 12);
    std::reverse(newRawId.begin(), newRawId.end());
    return "{" + newRawId.substr(0, 8) + "-" + newRawId.substr(8, 4) + "-" + newRawId.substr(12, 4) + "-" + newRawId.substr(16, 4) + "-" + newRawId.substr(20, 12) + "}";
}

void MeshGenerator::preprocessMirror()
{
    std::vector<std::map<std::string, std::string>> newParts;
    std::map<std::string, std::string> partOldToNewMap;
    for (auto& partIt : m_snapshot->parts) {
        bool xMirrored = String::isTrue(String::valueOrEmpty(partIt.second, "xMirrored"));
        if (!xMirrored)
            continue;
        std::map<std::string, std::string> mirroredPart = partIt.second;

        std::string newPartIdString = reverseUuid(mirroredPart["id"]);
        partOldToNewMap.insert({ mirroredPart["id"], newPartIdString });

        mirroredPart["__mirrorFromPartId"] = mirroredPart["id"];
        mirroredPart["id"] = newPartIdString;
        mirroredPart["__dirty"] = "true";
        newParts.push_back(mirroredPart);
    }

    for (const auto& it : partOldToNewMap)
        m_snapshot->parts[it.second]["__mirroredByPartId"] = it.first;

    std::map<std::string, std::string> parentMap;
    for (auto& componentIt : m_snapshot->components) {
        for (const auto& childId : String::split(String::valueOrEmpty(componentIt.second, "children"), ',')) {
            if (childId.empty())
                continue;
            parentMap[childId] = componentIt.first;
        }
    }
    for (const auto& childId : String::split(String::valueOrEmpty(m_snapshot->rootComponent, "children"), ',')) {
        if (childId.empty())
            continue;
        parentMap[childId] = std::string();
    }

    std::vector<std::map<std::string, std::string>> newComponents;
    for (auto& componentIt : m_snapshot->components) {
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

    for (const auto& it : newParts) {
        m_snapshot->parts[String::valueOrEmpty(it, "id")] = it;
    }
    for (const auto& it : newComponents) {
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

void MeshGenerator::addComponentPreview(const Uuid& componentId, ComponentPreview&& preview)
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
        for (auto it = m_cacheContext->parts.begin(); it != m_cacheContext->parts.end();) {
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
        for (auto it = m_cacheContext->components.begin(); it != m_cacheContext->components.end();) {
            if (m_snapshot->components.find(it->first) == m_snapshot->components.end()) {
                for (auto combinationIt = m_cacheContext->cachedCombination.begin(); combinationIt != m_cacheContext->cachedCombination.end();) {
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

    for (const auto& dirtyComponentId : m_dirtyComponentIds) {
        for (auto combinationIt = m_cacheContext->cachedCombination.begin(); combinationIt != m_cacheContext->cachedCombination.end();) {
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

    const auto& componentCache = m_cacheContext->components[to_string(Uuid())];

    m_object->positionToNodeIdMap = componentCache.positionToNodeIdMap;
    m_object->nodeMap = componentCache.nodeMap;
    m_object->componentTriangleUvs = componentCache.componentTriangleUvs;

    std::vector<Vector3> combinedVertices;
    std::vector<std::vector<size_t>> combinedFaces;
    if (nullptr != combinedMesh) {
        combinedMesh->fetch(combinedVertices, combinedFaces);
        m_object->seamTriangleUvs = combinedMesh->seamTriangleUvs;
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

    postprocessObject(m_object);

    if (needDeleteCacheContext) {
        delete m_cacheContext;
        m_cacheContext = nullptr;
    }
}

}
