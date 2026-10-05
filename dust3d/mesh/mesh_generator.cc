/*
 *  Copyright (c) 2016-2026 Jeremy HU <jeremy-at-dust3d dot org>. All rights reserved. 
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

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <dust3d/base/cut_face.h>
#include <dust3d/base/part_target.h>
#include <dust3d/base/snapshot_xml.h>
#include <dust3d/base/string.h>
#include <dust3d/mesh/mesh_generator.h>
#include <dust3d/mesh/mesh_recombiner.h>
#include <dust3d/mesh/rope_mesh.h>
#include <dust3d/mesh/smooth_normal.h>
#include <dust3d/mesh/spine_deformer.h>
#include <dust3d/mesh/stitch_loop_mesh_builder.h>
#include <dust3d/mesh/stitch_mesh_builder.h>
#include <dust3d/mesh/triangulate.h>
#include <dust3d/mesh/trim_vertices.h>
#include <dust3d/mesh/tube_mesh_builder.h>
#include <dust3d/rig/rig_generator.h>
#include <dust3d/uv/cloth_folds.h>
#include <dust3d/uv/surface_pattern.h>
#include <functional>
#include <iostream>
#include <limits>
#include <locale>
#include <memory>
#include <sstream>

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

void MeshGenerator::setImportedModelData(std::map<std::string, ImportedModelData>&& importedModelData)
{
    m_importedModelData = std::move(importedModelData);
}

bool MeshGenerator::isSuccessful()
{
    return m_isSuccessful;
}

const std::set<Uuid>& MeshGenerator::generatedPreviewComponentIds()
{
    return m_generatedPreviewComponentIds;
}

const std::map<Uuid, MeshGenerator::ComponentPreview>& MeshGenerator::generatedComponentPreviews()
{
    return m_generatedComponentPreviews;
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

    // A garment that takes its skin weights from another group (wrapBindTo) changes when
    // that group changes, though it is not one of its children.
    std::map<std::string, std::string> parentMap;
    for (const auto& componentIt : m_snapshot->components) {
        for (const auto& childId : String::split(String::valueOrEmpty(componentIt.second, "children"), ','))
            parentMap[childId] = componentIt.first;
    }
    for (bool changed = true; changed;) {
        changed = false;
        for (const auto& componentIt : m_snapshot->components) {
            std::string bindTo = String::valueOrEmpty(componentIt.second, "wrapBindTo");
            if (bindTo.empty() || m_dirtyComponentIds.find(bindTo) == m_dirtyComponentIds.end())
                continue;
            if (m_dirtyComponentIds.find(componentIt.first) != m_dirtyComponentIds.end())
                continue;
            for (std::string id = componentIt.first; !id.empty();) {
                m_dirtyComponentIds.insert(id);
                auto findParent = parentMap.find(id);
                id = findParent == parentMap.end() ? std::string() : findParent->second;
            }
            changed = true;
        }
    }
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
                float choosenRadian = std::numeric_limits<float>::max();
                for (int i = 0; i < (int)endpointNodes.size(); ++i) {
                    const auto& it = endpointNodes[i];
                    Vector2 direction2d = (Vector2(std::get<1>(it.second), std::get<2>(it.second)) - center);
                    Vector3 direction = Vector3(direction2d.x(), direction2d.y(), 0).normalized();
                    float radian = Vector3::angleBetween(referenceDirection, direction);
                    // Use strict less-than for deterministic first-match behavior
                    if (radian < choosenRadian) {
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

void MeshGenerator::flattenLinks(const std::map<size_t, size_t>& links,
    std::vector<size_t>* array,
    bool* isCircle)
{
    if (links.empty())
        return;
    for (const auto& it : links) {
        if (links.end() == links.find(it.second)) {
            *isCircle = false;
            std::map<size_t, size_t> reversedLinks;
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

bool MeshGenerator::fetchPartOrderedNodes(const std::string& partIdString, bool xMirrored, std::vector<MeshNode>* meshNodes, bool* isCircle)
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
            Vector3((double)x, (double)y, (double)z), (double)radius, Uuid(xMirrored ? String::valueOrEmpty(node, "__mirroredByNodeId") : nodeIdString) });
        // optional per-node cross-section scale (see MeshNode)
        std::string nodeDeformWidth = String::valueOrEmpty(node, "deformWidth");
        if (!nodeDeformWidth.empty() && String::toFloat(nodeDeformWidth) > 0)
            builderNodes.back().deformWidth = String::toFloat(nodeDeformWidth);
        std::string nodeDeformThickness = String::valueOrEmpty(node, "deformThickness");
        if (!nodeDeformThickness.empty() && String::toFloat(nodeDeformThickness) > 0)
            builderNodes.back().deformThickness = String::toFloat(nodeDeformThickness);
    }

    if (builderNodes.empty()) {
        dust3dDebug << "Expected at least one node in part:" << partIdString;
        return false;
    }

    std::map<size_t, size_t> builderNodeLinks;
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
    std::map<Uuid, Color> splineColors;
    for (size_t partIndex = 0; partIndex < partIdStrings.size(); ++partIndex) {
        const auto& partIdString = partIdStrings[partIndex];
        auto findPart = m_snapshot->parts.find(partIdString);
        if (findPart != m_snapshot->parts.end()) {
            if (String::isTrue(String::valueOrEmpty(findPart->second, "disabled")))
                continue;
        }
        bool isCircle = false;
        std::vector<MeshNode> orderedBuilderNodes;
        if (!fetchPartOrderedNodes(partIdString, false, &orderedBuilderNodes, &isCircle))
            continue;
        if (isCircle)
            continue;
        if (orderedBuilderNodes.size() < 2)
            continue;
        for (const auto& meshNode : orderedBuilderNodes) {
            componentCache.nodeMap.emplace(std::make_pair(meshNode.sourceId,
                ObjectNode { meshNode.origin, color, smoothCutoffDegrees }));
        }
        Color splineColor = color;
        auto findComponent = m_snapshot->components.find(componentIdStrings[partIndex]);
        if (findComponent != m_snapshot->components.end()) {
            std::string componentColorString = String::valueOrEmpty(findComponent->second, "color");
            if (!componentColorString.empty())
                splineColor = Color(componentColorString);
        }
        splineColors[componentIds[partIndex]] = splineColor;
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
        buildParameters.defaultRadius = 0.006;
        RopeMesh ropeMesh(buildParameters);
        std::vector<Vector3> positions(spline.nodes.size());
        for (size_t i = 0; i < spline.nodes.size(); ++i)
            positions[i] = spline.nodes[i].origin;
        ropeMesh.addRope(positions, false);

        ComponentPreview stitchingLinePreview;
        if (mesh)
            mesh->fetch(stitchingLinePreview.vertices, stitchingLinePreview.triangles);
        size_t startIndex = stitchingLinePreview.vertices.size();

        stitchingLinePreview.color = Color(color[0], color[1], color[2], 0.15);
        for (const auto& ropeVertex : ropeMesh.resultVertices()) {
            stitchingLinePreview.vertices.emplace_back(ropeVertex);
        }
        stitchingLinePreview.vertexProperties.resize(stitchingLinePreview.vertices.size());
        auto modelProperty = std::tuple<dust3d::Color, float /*metalness*/, float /*roughness*/> {
            stitchingLinePreview.color,
            stitchingLinePreview.metalness,
            stitchingLinePreview.roughness
        };
        auto findSplineColor = splineColors.find(spline.sourceId);
        Color ropeColor = (findSplineColor != splineColors.end()) ? findSplineColor->second : color;
        auto lineProperty = std::tuple<dust3d::Color, float /*metalness*/, float /*roughness*/> {
            Color(ropeColor.r(), ropeColor.g(), ropeColor.b(), 1.0),
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

std::unique_ptr<MeshState> MeshGenerator::combineStitchingLoopMesh(const std::string& componentIdString,
    const std::vector<std::string>& partIdStrings,
    const std::vector<std::string>& componentIdStrings,
    bool backClosed,
    float backCloseDepthRatio,
    float backCloseSharpness,
    size_t targetSegments,
    Color color,
    float smoothCutoffDegrees,
    GeneratedComponent& componentCache)
{
    std::vector<StitchLoopMeshBuilder::Loop> loops;
    loops.reserve(partIdStrings.size());
    std::vector<Uuid> componentIds(componentIdStrings.size());
    for (size_t i = 0; i < componentIdStrings.size(); ++i)
        componentIds[i] = componentIdStrings[i];
    std::map<Uuid, Color> loopPartColors;
    for (size_t partIndex = 0; partIndex < partIdStrings.size(); ++partIndex) {
        const auto& partIdString = partIdStrings[partIndex];
        auto findPart = m_snapshot->parts.find(partIdString);
        Color partColor = color;
        if (findPart != m_snapshot->parts.end()) {
            if (String::isTrue(String::valueOrEmpty(findPart->second, "disabled")))
                continue;
            std::string partColorString = String::valueOrEmpty(findPart->second, "color");
            if (!partColorString.empty())
                partColor = Color(partColorString);
        }
        bool isCircle = false;
        std::vector<MeshNode> orderedBuilderNodes;
        if (!fetchPartOrderedNodes(partIdString, false, &orderedBuilderNodes, &isCircle))
            continue;
        if (orderedBuilderNodes.size() < 2)
            continue;
        for (const auto& meshNode : orderedBuilderNodes) {
            componentCache.nodeMap.emplace(std::make_pair(meshNode.sourceId,
                ObjectNode { meshNode.origin, partColor, smoothCutoffDegrees }));
        }
        Color loopColor = color;
        auto findComponent = m_snapshot->components.find(componentIdStrings[partIndex]);
        if (findComponent != m_snapshot->components.end()) {
            std::string componentColorString = String::valueOrEmpty(findComponent->second, "color");
            if (!componentColorString.empty())
                loopColor = Color(componentColorString);
        }
        loopPartColors[componentIds[partIndex]] = loopColor;
        StitchLoopMeshBuilder::Loop loop;
        loop.nodes = std::move(orderedBuilderNodes);
        loop.sourceId = componentIds[partIndex];
        loop.closed = isCircle;
        if (findPart != m_snapshot->parts.end())
            loop.fillInterior = String::isTrue(String::valueOrEmpty(findPart->second, "fillLoopInterior"));
        loops.emplace_back(std::move(loop));
    }

    auto loopMeshBuilder = std::make_unique<StitchLoopMeshBuilder>(std::move(loops), targetSegments);
    loopMeshBuilder->setBackClosed(backClosed);
    loopMeshBuilder->setBackCloseDepthRatio(backCloseDepthRatio);
    loopMeshBuilder->setBackCloseSharpness(backCloseSharpness);
    loopMeshBuilder->build();

    const auto& generatedVertices = loopMeshBuilder->generatedVertices();
    const auto& generatedFaces = loopMeshBuilder->generatedFaces();

    collectSharedQuadEdges(generatedVertices, generatedFaces, &componentCache.sharedQuadEdges);

    auto mesh = std::make_unique<MeshState>(generatedVertices, generatedFaces);
    if (mesh && mesh->isNull())
        mesh.reset();

    const auto& faceUvs = loopMeshBuilder->generatedFaceUvs();
    Uuid componentId = Uuid(componentIdString);

    // Determine whether the component has a texture image configured.
    // If yes: use a single chart keyed by componentId with the 2D-projection UVs so the
    //         texture image is mapped onto the whole mesh.
    // If no:  split faces into per-part sub-charts keyed by each loop's sourceId (which is
    //         a child component ID in the snapshot). Each sub-chart uses that part's color
    //         (or its own colorImageId if configured), painted as a solid fill or textured tile.
    bool componentHasImage = false;
    {
        auto findComp = m_snapshot->components.find(componentIdString);
        if (findComp != m_snapshot->components.end())
            componentHasImage = !String::valueOrEmpty(findComp->second, "colorImageId").empty();
    }

    auto insertTriangleUv = [&](std::map<std::array<PositionKey, 3>, std::array<Vector2, 3>>& uvMap,
                                const std::vector<size_t>& face, const std::vector<Vector2>& uv) {
        if (3 == face.size()) {
            uvMap.insert({ { PositionKey(generatedVertices[face[0]]),
                               PositionKey(generatedVertices[face[1]]),
                               PositionKey(generatedVertices[face[2]]) },
                { uv[0], uv[1], uv[2] } });
        } else if (4 == face.size()) {
            uvMap.insert({ { PositionKey(generatedVertices[face[0]]),
                               PositionKey(generatedVertices[face[1]]),
                               PositionKey(generatedVertices[face[2]]) },
                { uv[0], uv[1], uv[2] } });
            uvMap.insert({ { PositionKey(generatedVertices[face[2]]),
                               PositionKey(generatedVertices[face[3]]),
                               PositionKey(generatedVertices[face[0]]) },
                { uv[2], uv[3], uv[0] } });
        }
    };

    if (componentHasImage) {
        // Single chart: all faces mapped to the component texture via 2D projection UVs.
        auto& triangleUvs = componentCache.componentTriangleUvs[componentId];
        for (size_t i = 0; i < faceUvs.size(); ++i)
            insertTriangleUv(triangleUvs, generatedFaces[i], faceUvs[i]);
    } else {
        // Per-part sub-charts: delegate to the builder.
        for (auto& [id, uvMap] : loopMeshBuilder->buildPerLoopTriangleUvs()) {
            auto& dest = componentCache.componentTriangleUvs[id.isNull() ? componentId : id];
            dest.insert(uvMap.begin(), uvMap.end());
        }
    }
    const auto& vertexSources = loopMeshBuilder->generatedVertexSources();
    for (size_t i = 0; i < vertexSources.size(); ++i) {
        componentCache.positionToNodeIdMap.emplace(std::make_pair(PositionKey(generatedVertices[i]), vertexSources[i]));
    }

    // Generate preview for each stitching loop
    for (const auto& loop : loopMeshBuilder->loops()) {
        RopeMesh::BuildParameters buildParameters;
        buildParameters.defaultRadius = 0.03;
        RopeMesh ropeMesh(buildParameters);
        std::vector<Vector3> positions(loop.nodes.size());
        for (size_t i = 0; i < loop.nodes.size(); ++i)
            positions[i] = loop.nodes[i].origin;
        ropeMesh.addRope(positions, loop.closed);

        ComponentPreview stitchingLoopPreview;
        if (mesh)
            mesh->fetch(stitchingLoopPreview.vertices, stitchingLoopPreview.triangles);
        size_t startIndex = stitchingLoopPreview.vertices.size();

        stitchingLoopPreview.color = Color(color[0], color[1], color[2], 0.15);
        for (const auto& ropeVertex : ropeMesh.resultVertices())
            stitchingLoopPreview.vertices.emplace_back(ropeVertex);
        stitchingLoopPreview.vertexProperties.resize(stitchingLoopPreview.vertices.size());
        auto modelProperty = std::tuple<dust3d::Color, float, float> {
            stitchingLoopPreview.color,
            stitchingLoopPreview.metalness,
            stitchingLoopPreview.roughness
        };
        auto findLoopColor = loopPartColors.find(loop.sourceId);
        Color ropeColor = (findLoopColor != loopPartColors.end()) ? findLoopColor->second : color;
        auto lineProperty = std::tuple<dust3d::Color, float, float> {
            Color(ropeColor.r(), ropeColor.g(), ropeColor.b(), 1.0),
            stitchingLoopPreview.metalness,
            stitchingLoopPreview.roughness
        };
        for (size_t i = 0; i < startIndex; ++i)
            stitchingLoopPreview.vertexProperties[i] = modelProperty;
        for (size_t i = startIndex; i < stitchingLoopPreview.vertexProperties.size(); ++i)
            stitchingLoopPreview.vertexProperties[i] = lineProperty;
        for (const auto& ropeTriangles : ropeMesh.resultTriangles()) {
            stitchingLoopPreview.triangles.emplace_back(std::vector<size_t> {
                startIndex + ropeTriangles[0],
                startIndex + ropeTriangles[1],
                startIndex + ropeTriangles[2] });
        }
        addComponentPreview(loop.sourceId, ComponentPreview(stitchingLoopPreview));
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

    std::string thicknessString = String::valueOrEmpty(part, "deformThickness");
    if (!thicknessString.empty()) {
        deformThickness = String::toFloat(thicknessString);
    }

    std::string widthString = String::valueOrEmpty(part, "deformWidth");
    if (!widthString.empty()) {
        deformWidth = String::toFloat(widthString);
    }

    bool deformUnified = String::isTrue(String::valueOrEmpty(part, "deformUnified"));
    // "interpolated" = "false" keeps only the part's own nodes as rings (no extra rings along
    // long edges): far fewer triangles for rigid thin parts such as grass blades and stakes.
    std::string interpolatedString = String::valueOrEmpty(part, "interpolated");
    bool interpolated = interpolatedString.empty() || String::isTrue(interpolatedString);

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
    if (!fetchPartOrderedNodes(searchPartIdString, !__mirrorFromPartId.empty(), &meshNodes, &isCircle))
        return nullptr;

    auto& partCache = m_cacheContext->parts[partIdString];
    partCache.reset();

    partCache.color = color;
    partCache.metalness = metalness;
    partCache.roughness = roughness;
    partCache.isSuccessful = false;
    partCache.joined = ((target == PartTarget::Model || target == PartTarget::ImportedModel) && !isDisabled);

    partCache.nodeMap.clear();
    {
        for (const auto& meshNode : meshNodes) {
            partCache.nodeMap.emplace(std::make_pair(meshNode.sourceId,
                ObjectNode { meshNode.origin, color, smoothCutoffDegrees }));
        }
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
        buildParameters.interpolationEnabled = interpolated;
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
    } else if (PartTarget::ImportedModel == target) {
        std::string importedModelIdString = String::valueOrEmpty(part, "importedModelId");
        auto findImportedModel = m_importedModelData.find(importedModelIdString);
        if (findImportedModel != m_importedModelData.end()) {
            const auto& importedData = findImportedModel->second;
            if (!importedData.vertices.empty() && !importedData.faces.empty()) {
                // Compute imported mesh bounding box
                Vector3 importedMin = importedData.vertices[0];
                Vector3 importedMax = importedData.vertices[0];
                for (const auto& v : importedData.vertices) {
                    importedMin.setX(std::min(importedMin.x(), v.x()));
                    importedMin.setY(std::min(importedMin.y(), v.y()));
                    importedMin.setZ(std::min(importedMin.z(), v.z()));
                    importedMax.setX(std::max(importedMax.x(), v.x()));
                    importedMax.setY(std::max(importedMax.y(), v.y()));
                    importedMax.setZ(std::max(importedMax.z(), v.z()));
                }

                // Sweep the imported mesh along the tube spine: Y becomes the
                // primary (spine) axis, X/Z become the cross-section.
                SpineDeformer spineDeformer(meshNodes, importedMin, importedMax,
                    deformWidth, deformThickness, cutRotation);

                partCache.vertices.resize(importedData.vertices.size());
                for (size_t vi = 0; vi < importedData.vertices.size(); ++vi) {
                    partCache.vertices[vi] = spineDeformer.deformVertex(importedData.vertices[vi]);
                }

                if (!__mirrorFromPartId.empty()) {
                    for (auto& it : partCache.vertices)
                        it.setX(-it.x());
                }

                partCache.faces = importedData.faces;
                if (!__mirrorFromPartId.empty()) {
                    for (auto& it : partCache.faces)
                        std::reverse(it.begin(), it.end());
                }

                // Build triangleUvs using deformed vertex positions as keys.
                // importedData.triangleUvs uses pre-deformation position keys,
                // but the packer/renderer looks up by deformed position keys.
                if (!importedData.triangleUvs.empty()) {
                    std::vector<Vector2> perVertexUv(importedData.vertices.size(), Vector2(0, 0));
                    for (const auto& face : importedData.faces) {
                        if (face.size() < 3)
                            continue;
                        auto findUv = importedData.triangleUvs.find({ PositionKey(importedData.vertices[face[0]]),
                            PositionKey(importedData.vertices[face[1]]),
                            PositionKey(importedData.vertices[face[2]]) });
                        if (findUv != importedData.triangleUvs.end()) {
                            perVertexUv[face[0]] = findUv->second[0];
                            perVertexUv[face[1]] = findUv->second[1];
                            perVertexUv[face[2]] = findUv->second[2];
                        }
                    }
                    for (const auto& face : partCache.faces) {
                        if (face.size() < 3)
                            continue;
                        partCache.triangleUvs.insert({ { PositionKey(partCache.vertices[face[0]]),
                                                           PositionKey(partCache.vertices[face[1]]),
                                                           PositionKey(partCache.vertices[face[2]]) },
                            { perVertexUv[face[0]], perVertexUv[face[1]], perVertexUv[face[2]] } });
                    }
                }

                // Store per-vertex colors from imported model
                if (!importedData.vertexColors.empty()) {
                    for (size_t i = 0; i < partCache.vertices.size(); ++i) {
                        if (i < importedData.vertexColors.size()) {
                            partCache.importedVertexColorMap[PositionKey(partCache.vertices[i])] = importedData.vertexColors[i];
                        }
                    }
                }

                // Transform and store per-face-vertex normals from imported model
                // When smoothCutoffDegrees is set, skip GLB normals and let smoothNormal handle it
                if (!importedData.vertexNormals.empty() && smoothCutoffDegrees < 0.01f) {
                    auto transformNormal = [&](size_t vi) -> Vector3 {
                        const auto& sv = importedData.vertices[vi];
                        Vector3 transformed = spineDeformer.deformNormal(importedData.vertexNormals[vi], sv.y());
                        if (!__mirrorFromPartId.empty())
                            transformed.setX(-transformed.x());
                        return transformed;
                    };

                    for (const auto& face : partCache.faces) {
                        if (face.size() < 3)
                            continue;
                        if (face[0] >= importedData.vertexNormals.size()
                            || face[1] >= importedData.vertexNormals.size()
                            || face[2] >= importedData.vertexNormals.size())
                            continue;
                        std::array<PositionKey, 3> triKey = {
                            PositionKey(partCache.vertices[face[0]]),
                            PositionKey(partCache.vertices[face[1]]),
                            PositionKey(partCache.vertices[face[2]])
                        };
                        partCache.importedTriangleNormals[triKey] = {
                            transformNormal(face[0]),
                            transformNormal(face[1]),
                            transformNormal(face[2])
                        };
                    }
                }

                for (size_t i = 0; i < partCache.vertices.size(); ++i) {
                    // Map each vertex to the nearest node for source tracking
                    double bestDist = std::numeric_limits<double>::max();
                    Uuid bestNodeId;
                    for (const auto& mn : meshNodes) {
                        double d = (partCache.vertices[i] - mn.origin).lengthSquared();
                        if (d < bestDist) {
                            bestDist = d;
                            bestNodeId = mn.sourceId;
                        }
                    }
                    partCache.positionToNodeIdMap.emplace(std::make_pair(PositionKey(partCache.vertices[i]), bestNodeId));
                }
            }
        }
    }

    bool hasMeshError = false;
    std::unique_ptr<MeshState> mesh;

    mesh = std::make_unique<MeshState>(partCache.vertices, partCache.faces);
    if (mesh->isNull()) {
        hasMeshError = true;
    }

    if (PartTarget::Model == target || PartTarget::ImportedModel == target) {
        ComponentPreview preview;
        if (mesh)
            mesh->fetch(preview.vertices, preview.triangles);
        preview.color = partCache.color;
        preview.metalness = partCache.metalness;
        preview.roughness = partCache.roughness;
        preview.triangleUvs = partCache.triangleUvs;
        if (!partCache.importedVertexColorMap.empty()) {
            for (const auto& vertex : preview.vertices) {
                auto findColor = partCache.importedVertexColorMap.find(vertex);
                preview.vertexProperties.emplace_back(
                    findColor == partCache.importedVertexColorMap.end() ? preview.color : findColor->second,
                    preview.metalness, preview.roughness);
            }
        }
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

    if (hasMeshError && (target == PartTarget::Model || target == PartTarget::ImportedModel)) {
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
            if (nullptr != componentCache.mesh) {
                if (!componentCache.wrapColor.empty() && String::valueOrEmpty(*component, "color").empty())
                    m_snapshot->components[componentIdString]["color"] = componentCache.wrapColor;
                if (!componentCache.wrapFolds.empty())
                    m_snapshot->components[componentIdString]["__wrapFolds"] = componentCache.wrapFolds;
                m_generatedComponentIds.insert(componentIdString);
                return std::make_unique<MeshState>(*componentCache.mesh);
            }
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
            for (const auto& it : partCache.importedVertexColorMap)
                componentCache.importedVertexColorMap.emplace(it);
            for (const auto& it : partCache.importedTriangleNormals)
                componentCache.importedTriangleNormals.emplace(it);
            for (const auto& it : partCache.nodeMap)
                componentCache.nodeMap.emplace(it);
        }
        if (!partCache.joined) {
            if (mesh)
                mesh.reset();
        }
    } else if (isWrapComponent(component) && !wrapKeepsChildren(component)) {
        // The children are wrapped by one new surface, which replaces them (a creature
        // skin over bones and muscle shapes, or a garment over guide shapes).
        mesh = buildWrapMesh(componentIdString, *component, color, smoothCutoffDegrees, componentCache);
    } else {
        std::vector<std::pair<CombineMode, std::vector<std::string>>> combineGroups;
        int currentGroupIndex = -1;
        auto lastCombineMode = CombineMode::Count;
        std::vector<std::string> stitchingParts;
        std::vector<std::string> stitchingComponents;
        std::vector<std::string> stitchingLoopParts;
        std::vector<std::string> stitchingLoopComponents;
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
                    if ("StitchingLoop" == String::valueOrEmpty(findPart->second, "target")) {
                        stitchingLoopParts.emplace_back(partIdString);
                        stitchingLoopComponents.emplace_back(childIdString);
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
            auto childMesh = combineComponentChildGroupMesh(group.second, componentCache, &componentCache.brokenTriangles);
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
        if (!stitchingLoopParts.empty()) {
            float backCloseDepthRatio = 1.0f;
            float backCloseSharpness = 0.0f;
            {
                auto it = component->find("backCloseDepthRatio");
                if (it != component->end())
                    backCloseDepthRatio = String::toFloat(it->second);
                it = component->find("backCloseSharpness");
                if (it != component->end())
                    backCloseSharpness = String::toFloat(it->second);
            }
            auto stitchingLoopMesh = combineStitchingLoopMesh(componentIdString,
                stitchingLoopParts,
                stitchingLoopComponents,
                String::isTrue(String::valueOrEmpty(*component, "backClosed")),
                backCloseDepthRatio,
                backCloseSharpness,
                targetSegments,
                color,
                smoothCutoffDegrees,
                componentCache);
            if (stitchingLoopMesh && !stitchingLoopMesh->isNull()) {
                groupMeshes.emplace_back(std::make_tuple(std::move(stitchingLoopMesh), CombineMode::Normal, String::join(stitchingLoopComponents, ":")));
            }
        }
        mesh = combineMultipleMeshes(std::move(groupMeshes), &componentCache.brokenTriangles);
        if (isWrapComponent(component)) {
            // The children stay (a garment over the body), the wrap surface is emitted next to them.
            componentCache.wrapOutput = std::make_unique<GeneratedComponent>();
            componentCache.wrapOutput->mesh = buildWrapMesh(componentIdString, *component, color, smoothCutoffDegrees, *componentCache.wrapOutput);
        }
    }

    if (nullptr != mesh)
        componentCache.mesh = std::make_unique<MeshState>(*mesh);
    if ("partId" != linkDataType) {
        ComponentPreview preview;
        preview.color = color;
        collectComponentPreview(componentIdString, true, preview);
        addComponentPreview(componentId, std::move(preview));
    }
    m_generatedComponentIds.insert(componentIdString);

    if (nullptr != mesh && mesh->isNull()) {
        mesh.reset();
    }

    return mesh;
}

bool MeshGenerator::seamReportEnabled()
{
    // Opt-in diagnostics for tools: DUST3D_SEAM_REPORT=1 prints one SEAM_REPORT line per union
    // to stdout. Unset, empty, "0", "false" or "off" keep it disabled. Read once per process.
    static const bool enabled = []() {
        const char* value = std::getenv("DUST3D_SEAM_REPORT");
        if (nullptr == value)
            return false;
        std::string text(value);
        return !(text.empty() || "0" == text || "false" == text || "off" == text);
    }();
    return enabled;
}

std::string MeshGenerator::componentDisplayName(const std::string& componentIdString)
{
    auto findComponent = m_snapshot->components.find(componentIdString);
    if (findComponent == m_snapshot->components.end())
        return componentIdString;
    std::string name = String::valueOrEmpty(findComponent->second, "name");
    if (name.empty())
        name = componentIdString;
    if ("partId" == String::valueOrEmpty(findComponent->second, "linkDataType")) {
        auto findPart = m_snapshot->parts.find(String::valueOrEmpty(findComponent->second, "linkData"));
        if (findPart != m_snapshot->parts.end() && !String::valueOrEmpty(findPart->second, "__mirrorFromPartId").empty())
            name += "~mirror";
    }
    return name;
}

std::string MeshGenerator::seamReportNames(const std::string& subMeshIdString)
{
    std::string names;
    for (const auto& idString : String::split(subMeshIdString, '|')) {
        for (const auto& id : String::split(idString, ':')) {
            if (id.empty())
                continue;
            if (!names.empty())
                names += "|";
            std::string name = componentDisplayName(id);
            for (auto& c : name) {
                if (' ' == c)
                    c = '_';
            }
            names += name;
        }
    }
    return names;
}

void MeshGenerator::reportFailedCombine(const std::string& subMeshIdString, const std::string& method)
{
    // The boolean failed (usually coincident or grazing surfaces) and the parts were dropped:
    // SEAM_REPORT <method> <joined component names> failed
    std::cout << "SEAM_REPORT " << method << " " << seamReportNames(subMeshIdString) << " failed" << std::endl;
}

void MeshGenerator::reportSeams(const std::string& subMeshIdString, const std::string& method,
    const std::vector<MeshRecombiner::SeamReport>& reports)
{
    // One machine-readable line per combine, for tools that check seam quality:
    // SEAM_REPORT <method> <joined component names> <island count> [island ...]
    // island = bridged,firstLoops,secondLoops,firstLoopVertices,secondLoopVertices,x,y,z,radius,
    //          bridgeTriangles,bridgeMinAngle,bridgeMaxFan,bridgeMaxWidth,firstLoopEdgeLength,secondLoopEdgeLength
    std::ostringstream line;
    line.imbue(std::locale::classic());
    line << "SEAM_REPORT " << method << " " << seamReportNames(subMeshIdString) << " " << reports.size();
    for (const auto& it : reports) {
        line << " " << (it.bridged ? 1 : 0) << "," << it.firstLoops << "," << it.secondLoops << ","
             << it.firstLoopVertices << "," << it.secondLoopVertices << ","
             << it.center.x() << "," << it.center.y() << "," << it.center.z() << "," << it.radius << ","
             << it.bridgeTriangles << "," << it.bridgeMinAngle << "," << it.bridgeMaxFan << "," << it.bridgeMaxWidth << ","
             << it.firstLoopEdgeLength << "," << it.secondLoopEdgeLength;
    }
    std::cout << line.str() << std::endl;
}

bool MeshGenerator::isHardComponent(const std::string& componentIdString, int depth)
{
    // A part component is hard when its part says "hard" = "true". A group is hard when it
    // says so itself, or when every child in it is hard.
    if (depth > 64)
        return false;
    auto findComponent = m_snapshot->components.find(componentIdString);
    if (findComponent == m_snapshot->components.end())
        return false;
    const auto& component = findComponent->second;
    if (String::isTrue(String::valueOrEmpty(component, "hard")))
        return true;
    std::string linkDataType = String::valueOrEmpty(component, "linkDataType");
    if ("partId" == linkDataType) {
        auto findPart = m_snapshot->parts.find(String::valueOrEmpty(component, "linkData"));
        return findPart != m_snapshot->parts.end() && String::isTrue(String::valueOrEmpty(findPart->second, "hard"));
    }
    auto children = String::split(String::valueOrEmpty(component, "children"), ',');
    bool any = false;
    for (const auto& childId : children) {
        if (childId.empty())
            continue;
        if (!isHardComponent(childId, depth + 1))
            return false;
        any = true;
    }
    return any;
}

std::unique_ptr<MeshState> MeshGenerator::combineMultipleMeshes(std::vector<std::tuple<std::unique_ptr<MeshState>, CombineMode, std::string>>&& multipleMeshes,
    std::set<std::array<PositionKey, 3>>* brokenTriangles)
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
            // a hard-surface part (or a group of them) joins with a crisp boolean edge
            bool hard = isHardComponent(subMeshIdString);
            newMesh = MeshState::combine(*mesh,
                *subMesh,
                combinerMethod,
                !hard);
            if (nullptr != newMesh)
                m_cacheContext->cachedCombination.insert({ meshIdStrings, std::make_unique<MeshState>(*newMesh) });
            else
                m_cacheContext->cachedCombination.insert({ meshIdStrings, nullptr });
        }
        if (seamReportEnabled()) {
            if (newMesh && !newMesh->isNull() && isHardComponent(subMeshIdString))
                std::cout << "SEAM_REPORT " << combinerMethodString << " " << seamReportNames(subMeshIdString) << " hard" << std::endl;
            else if (newMesh && !newMesh->isNull())
                reportSeams(subMeshIdString, combinerMethodString, newMesh->seamReports);
            else
                reportFailedCombine(subMeshIdString, combinerMethodString);
        }
        if (newMesh && !newMesh->isNull()) {
            if (nullptr != brokenTriangles) {
                for (const auto& brokenTriangle : newMesh->brokenTriangles)
                    brokenTriangles->insert(brokenTriangle);
            }
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

std::unique_ptr<MeshState> MeshGenerator::combineComponentChildGroupMesh(const std::vector<std::string>& componentIdStrings,
    GeneratedComponent& componentCache,
    std::set<std::array<PositionKey, 3>>* brokenTriangles)
{
    std::vector<std::tuple<std::unique_ptr<MeshState>, CombineMode, std::string>> multipleMeshes;
    for (const auto& childIdString : componentIdStrings) {
        CombineMode childCombineMode = CombineMode::Normal;
        std::unique_ptr<MeshState> subMesh = combineComponentMesh(childIdString, &childCombineMode);

        if (CombineMode::Uncombined == childCombineMode) {
            const auto& uncombinedCache = m_cacheContext->components[childIdString];
            for (const auto& it : uncombinedCache.importedTriangleNormals)
                componentCache.importedTriangleNormals.emplace(it);
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
        for (const auto& it : childComponentCache.importedVertexColorMap)
            componentCache.importedVertexColorMap.emplace(it);
        for (const auto& it : childComponentCache.importedTriangleNormals)
            componentCache.importedTriangleNormals.emplace(it);
        for (const auto& it : childComponentCache.positionToNodeWeights)
            componentCache.positionToNodeWeights.emplace(it);
        for (const auto& it : childComponentCache.positionToVertexAttribute)
            componentCache.positionToVertexAttribute.emplace(it);
        if (nullptr == subMesh || subMesh->isNull()) {
            continue;
        }

        multipleMeshes.emplace_back(std::make_tuple(std::move(subMesh), childCombineMode, childIdString));
    }
    return combineMultipleMeshes(std::move(multipleMeshes), brokenTriangles);
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
        if (findSourceNode == object->positionToNodeIdMap.end()) {
            auto findAttribute = object->positionToVertexAttribute.find(object->vertices[i]);
            if (findAttribute != object->positionToVertexAttribute.end()) {
                object->vertexColors[i] = findAttribute->second.color;
                object->vertexSmoothCutoffDegrees[i] = findAttribute->second.smoothCutoffDegrees;
            }
            continue;
        }
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

    // Position-based normal merge for imported meshes with user-configured smoothCutoffDegrees.
    // smoothNormal uses vertex-index adjacency, so GLTF meshes with per-face-vertex data
    // (where each triangle corner has a unique index even at the same position) always get
    // flat normals from smoothNormal. This pass groups face normals by position and only
    // merges those within the cutoff angle.
    if (!m_importedModelData.empty()) {
        std::map<PositionKey, std::vector<size_t>> posToTriangles;
        for (size_t ti = 0; ti < object->triangles.size(); ++ti) {
            const auto& face = object->triangles[ti];
            for (size_t j = 0; j < face.size() && j < 3; ++j) {
                if (face[j] < object->vertexSmoothCutoffDegrees.size()
                    && object->vertexSmoothCutoffDegrees[face[j]] > 0.0f) {
                    posToTriangles[PositionKey(object->vertices[face[j]])].push_back(ti);
                }
            }
        }
        for (size_t ti = 0; ti < object->triangles.size(); ++ti) {
            const auto& face = object->triangles[ti];
            for (size_t j = 0; j < face.size() && j < 3; ++j) {
                if (face[j] >= object->vertexSmoothCutoffDegrees.size()
                    || object->vertexSmoothCutoffDegrees[face[j]] <= 0.0f)
                    continue;
                float cutoff = object->vertexSmoothCutoffDegrees[face[j]];
                double cosLimit = std::cos(cutoff * Math::Pi / 180.0);
                auto it = posToTriangles.find(PositionKey(object->vertices[face[j]]));
                if (it == posToTriangles.end())
                    continue;
                Vector3 sum;
                for (size_t neighborTi : it->second) {
                    if (Vector3::dotProduct(object->triangleNormals[ti], object->triangleNormals[neighborTi]) >= cosLimit)
                        sum += object->triangleNormals[neighborTi];
                }
                sum.normalize();
                triangleVertexNormals[ti][j] = sum;
            }
        }
    }

    object->setTriangleVertexNormals(triangleVertexNormals);
}

void MeshGenerator::collectIncombinableMesh(const MeshState* mesh, const GeneratedComponent& componentCache, const Uuid& componentId)
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
    for (const auto& it : m_snapshot->components) {
        std::string name = String::valueOrEmpty(it.second, "name");
        if (!name.empty())
            m_object->componentNames[Uuid(it.first)] = name;
    }
    for (const auto& it : componentCache.positionToNodeIdMap)
        m_object->positionToNodeIdMap.emplace(it);
    for (const auto& it : componentCache.nodeMap)
        m_object->nodeMap.emplace(it);
    for (const auto& it : componentCache.positionToNodeWeights)
        m_object->positionToNodeWeights.emplace(it);
    for (const auto& it : componentCache.positionToVertexAttribute)
        m_object->positionToVertexAttribute.emplace(it);

    m_object->vertices.insert(m_object->vertices.end(), uncombinedVertices.begin(), uncombinedVertices.end());
    // Uncombined parts are kept whole, so their triangles are known to be theirs: record it
    // (the UV generator places the rest by position, which cannot tell apart two parts that
    // overlap exactly, e.g. equipment variants in the same place).
    m_object->triangleComponentIds.resize(m_object->triangles.size());
    m_object->triangleComponentIds.insert(m_object->triangleComponentIds.end(), uncombinedFaces.size(), componentId);
    m_object->triangles.insert(m_object->triangles.end(), uncombinedFaces.begin(), uncombinedFaces.end());
    m_object->triangleAndQuads.insert(m_object->triangleAndQuads.end(), uncombinedTriangleAndQuads.begin(), uncombinedTriangleAndQuads.end());
}

void MeshGenerator::collectUncombinedComponent(const std::string& componentIdString)
{
    const auto& component = findComponent(componentIdString);
    if (nullptr == component)
        return;
    bool isWrap = isWrapComponent(component);
    const auto& componentCache = m_cacheContext->components[componentIdString];
    if (isWrap && componentCache.wrapOutput && componentCache.wrapOutput->mesh && !componentCache.wrapOutput->mesh->isNull())
        collectIncombinableMesh(componentCache.wrapOutput->mesh.get(), *componentCache.wrapOutput, Uuid(componentIdString));
    if (CombineMode::Uncombined == componentCombineMode(component)) {
        if (nullptr != componentCache.mesh && !componentCache.mesh->isNull()) {
            bool isPart = "partId" == String::valueOrEmpty(*component, "linkDataType");
            bool isSkin = isWrap && !wrapKeepsChildren(component);
            collectIncombinableMesh(componentCache.mesh.get(), componentCache, (isPart || isSkin) ? Uuid(componentIdString) : Uuid());
        }
        // a wrap component still lets its own uncombined children through (eyes in a skin)
        if (!isWrap)
            return;
    }
    for (const auto& childIdString : String::split(String::valueOrEmpty(*component, "children"), ',')) {
        if (childIdString.empty())
            continue;
        collectUncombinedComponent(childIdString);
    }
}

bool MeshGenerator::isWrapComponent(const std::map<std::string, std::string>* component)
{
    if (nullptr == component)
        return false;
    if ("partId" == String::valueOrEmpty(*component, "linkDataType"))
        return false;
    std::string wrap = String::valueOrEmpty(*component, "wrap");
    return "Skin" == wrap || "Cloth" == wrap;
}

bool MeshGenerator::wrapKeepsChildren(const std::map<std::string, std::string>* component)
{
    if (nullptr == component)
        return false;
    std::string keep = String::valueOrEmpty(*component, "wrapKeep");
    if (!keep.empty())
        return String::isTrue(keep);
    // by default a creature skin replaces what it wraps, a garment is worn over it
    return "Cloth" == String::valueOrEmpty(*component, "wrap");
}

void MeshGenerator::collectBindSamples(const std::string& componentIdString,
    WrapMeshBuilder* builder,
    int depth,
    std::vector<Vector3>* surfaceVertices,
    std::vector<std::vector<size_t>>* surfaceFaces)
{
    if (depth > 16)
        return;
    const auto* component = findComponent(componentIdString);
    if (nullptr == component)
        return;
    auto addSamples = [&](const std::string& idString) {
        std::unique_ptr<MeshState> mesh;
        if (m_generatedComponentIds.find(idString) != m_generatedComponentIds.end()) {
            const auto& cached = m_cacheContext->components[idString].mesh;
            if (nullptr != cached)
                mesh = std::make_unique<MeshState>(*cached);
        } else {
            CombineMode mode;
            mesh = combineComponentMesh(idString, &mode);
        }
        if (nullptr == mesh || mesh->isNull())
            return;
        const auto& cache = m_cacheContext->components[idString];
        std::vector<Vector3> vertices;
        std::vector<std::vector<size_t>> faces;
        mesh->fetch(vertices, faces);
        for (const auto& vertex : vertices) {
            auto findNode = cache.positionToNodeIdMap.find(PositionKey(vertex));
            if (findNode != cache.positionToNodeIdMap.end())
                builder->addBindSample(vertex, findNode->second);
        }
        if (nullptr != surfaceVertices && nullptr != surfaceFaces) {
            size_t offset = surfaceVertices->size();
            surfaceVertices->insert(surfaceVertices->end(), vertices.begin(), vertices.end());
            for (auto face : faces) {
                for (auto& index : face)
                    index += offset;
                surfaceFaces->push_back(face);
            }
        }
    };
    if ("partId" == String::valueOrEmpty(*component, "linkDataType")) {
        auto findPart = m_snapshot->parts.find(String::valueOrEmpty(*component, "linkData"));
        if (findPart == m_snapshot->parts.end())
            return;
        std::string target = String::valueOrEmpty(findPart->second, "target");
        if (target.empty() || "Model" == target || "ImportedModel" == target)
            addSamples(componentIdString);
        return;
    }
    // the parts that make a group, wherever they are in it (a skin's own surface is made
    // from them, so its weights are theirs)
    for (const auto& childIdString : String::split(String::valueOrEmpty(*component, "children"), ',')) {
        if (childIdString.empty())
            continue;
        const auto* child = findComponent(childIdString);
        if (nullptr == child || CombineMode::Uncombined == componentCombineMode(child) || CombineMode::Inversion == componentCombineMode(child))
            continue;
        collectBindSamples(childIdString, builder, depth + 1, surfaceVertices, surfaceFaces);
    }
}

void MeshGenerator::collectWrapSources(const std::string& componentIdString,
    bool subtract,
    WrapMeshBuilder* builder,
    size_t* sourceCount)
{
    const auto* component = findComponent(componentIdString);
    if (nullptr == component)
        return;

    auto partTarget = [&](const std::map<std::string, std::string>* child) -> std::string {
        if ("partId" != String::valueOrEmpty(*child, "linkDataType"))
            return std::string();
        auto findPart = m_snapshot->parts.find(String::valueOrEmpty(*child, "linkData"));
        if (findPart == m_snapshot->parts.end())
            return std::string();
        std::string target = String::valueOrEmpty(findPart->second, "target");
        return target.empty() ? std::string("Model") : target;
    };
    auto hasStitchingChildren = [&](const std::map<std::string, std::string>* group) {
        for (const auto& childIdString : String::split(String::valueOrEmpty(*group, "children"), ',')) {
            const auto* child = findComponent(childIdString);
            if (nullptr == child)
                continue;
            std::string target = partTarget(child);
            if ("StitchingLine" == target || "StitchingLoop" == target)
                return true;
        }
        return false;
    };
    auto addSource = [&](const MeshState* mesh, const GeneratedComponent& cache, bool carve) {
        if (nullptr == mesh || mesh->isNull())
            return;
        std::vector<Vector3> vertices;
        std::vector<std::vector<size_t>> faces;
        mesh->fetch(vertices, faces);
        if (faces.empty())
            return;
        builder->addSource(vertices, faces, carve);
        ++(*sourceCount);
        if (carve)
            return;
        for (const auto& vertex : vertices) {
            PositionKey key(vertex);
            auto findNode = cache.positionToNodeIdMap.find(key);
            if (findNode != cache.positionToNodeIdMap.end()) {
                builder->addBindSample(vertex, findNode->second);
                continue;
            }
            auto findWeights = cache.positionToNodeWeights.find(key);
            if (findWeights != cache.positionToNodeWeights.end() && !findWeights->second.empty())
                builder->addBindSample(vertex, findWeights->second.front().first);
        }
    };

    bool hasStitching = false;
    for (const auto& childIdString : String::split(String::valueOrEmpty(*component, "children"), ',')) {
        if (childIdString.empty())
            continue;
        const auto* child = findComponent(childIdString);
        if (nullptr == child)
            continue;
        std::string target = partTarget(child);
        if ("StitchingLine" == target || "StitchingLoop" == target) {
            hasStitching = true;
            continue;
        }
        CombineMode childCombineMode = componentCombineMode(child);
        if (CombineMode::Uncombined == childCombineMode) {
            // not wrapped: generated as usual, and collected as its own mesh
            CombineMode mode;
            combineComponentMesh(childIdString, &mode);
            continue;
        }
        bool carve = subtract || CombineMode::Inversion == childCombineMode;
        bool isLeaf = !target.empty() || isWrapComponent(child) || hasStitchingChildren(child);
        if (!isLeaf) {
            // a plain group: wrap its children directly, no boolean union needed
            collectWrapSources(childIdString, carve, builder, sourceCount);
            continue;
        }
        std::unique_ptr<MeshState> childMesh;
        if (m_generatedComponentIds.find(childIdString) != m_generatedComponentIds.end()) {
            const auto& cached = m_cacheContext->components[childIdString].mesh;
            if (nullptr != cached)
                childMesh = std::make_unique<MeshState>(*cached);
        } else {
            CombineMode mode;
            childMesh = combineComponentMesh(childIdString, &mode);
        }
        const auto& childCache = m_cacheContext->components[childIdString];
        addSource(childMesh.get(), childCache, carve);
        // a garment inside a garment: the outer one goes over both (layering)
        if (childCache.wrapOutput && childCache.wrapOutput->mesh)
            addSource(childCache.wrapOutput->mesh.get(), *childCache.wrapOutput, carve);
    }

    if (hasStitching) {
        // stitched shells (fins, collars, a loft over ribs) inside the wrap group
        std::vector<std::string> stitchingParts, stitchingComponents, stitchingLoopParts, stitchingLoopComponents;
        for (const auto& childIdString : String::split(String::valueOrEmpty(*component, "children"), ',')) {
            const auto* child = findComponent(childIdString);
            if (nullptr == child)
                continue;
            std::string target = partTarget(child);
            if ("StitchingLine" == target) {
                stitchingParts.push_back(String::valueOrEmpty(*child, "linkData"));
                stitchingComponents.push_back(childIdString);
            } else if ("StitchingLoop" == target) {
                stitchingLoopParts.push_back(String::valueOrEmpty(*child, "linkData"));
                stitchingLoopComponents.push_back(childIdString);
            }
        }
        std::string colorString = String::valueOrEmpty(*component, "color");
        Color color = colorString.empty() ? m_defaultPartColor : Color(colorString);
        size_t targetSegments = (size_t)String::toInt(String::valueOrEmpty(*component, "targetSegments"));
        if (targetSegments > 100)
            targetSegments = 0;
        if (!stitchingParts.empty()) {
            GeneratedComponent stitchingCache;
            auto stitchingMesh = combineStitchingMesh(componentIdString, stitchingParts, stitchingComponents,
                String::isTrue(String::valueOrEmpty(*component, "frontClosed")),
                String::isTrue(String::valueOrEmpty(*component, "backClosed")),
                String::isTrue(String::valueOrEmpty(*component, "sideClosed")),
                targetSegments, color, 0.0f, stitchingCache);
            addSource(stitchingMesh.get(), stitchingCache, subtract);
        }
        if (!stitchingLoopParts.empty()) {
            GeneratedComponent stitchingCache;
            float backCloseDepthRatio = 1.0f;
            float backCloseSharpness = 0.0f;
            auto it = component->find("backCloseDepthRatio");
            if (it != component->end())
                backCloseDepthRatio = String::toFloat(it->second);
            it = component->find("backCloseSharpness");
            if (it != component->end())
                backCloseSharpness = String::toFloat(it->second);
            auto stitchingLoopMesh = combineStitchingLoopMesh(componentIdString, stitchingLoopParts, stitchingLoopComponents,
                String::isTrue(String::valueOrEmpty(*component, "backClosed")),
                backCloseDepthRatio, backCloseSharpness, targetSegments, color, 0.0f, stitchingCache);
            addSource(stitchingLoopMesh.get(), stitchingCache, subtract);
        }
    }
}

std::unique_ptr<MeshState> MeshGenerator::buildWrapMesh(const std::string& componentIdString,
    const std::map<std::string, std::string>& component,
    const Color& color,
    float smoothCutoffDegrees,
    GeneratedComponent& output)
{
    bool cloth = "Cloth" == String::valueOrEmpty(component, "wrap");
    auto readFloat = [&](const char* name, double defaultValue) {
        auto it = component.find(name);
        if (it == component.end() || it->second.empty())
            return defaultValue;
        return (double)String::toFloat(it->second);
    };

    WrapMeshBuilder::Parameters parameters;
    parameters.mode = cloth ? WrapMeshBuilder::Mode::Cloth : WrapMeshBuilder::Mode::Skin;
    parameters.offset = readFloat("wrapOffset", cloth ? 0.012 : 0.0);
    parameters.smoothness = std::max(0.0, readFloat("wrapSmoothness", cloth ? 0.05 : 0.02));
    parameters.drape = std::max(0.0, std::min(1.0, readFloat("wrapDrape", cloth ? 0.5 : 0.0)));
    parameters.drapeLength = std::max(0.0, readFloat("wrapDrapeLength", 0.0));
    parameters.openTop = std::max(0.0, std::min(0.45, readFloat("wrapOpenTop", 0.0)));
    parameters.openBottom = std::max(0.0, std::min(0.45, readFloat("wrapOpenBottom", 0.0)));
    parameters.thickness = std::max(0.0, readFloat("wrapThickness", cloth ? 0.004 : 0.0));
    parameters.targetFaces = (size_t)std::max(64.0, std::min(40000.0, readFloat("wrapFaces", cloth ? 1200.0 : 1600.0)));
    parameters.weightRadius = std::max(0.0, readFloat("wrapWeightRadius", 0.0));
    // the group's "Normal Smooth" cutoff is AutoRemesher's "Smooth Normal" (0 = off)
    parameters.smoothNormalDegrees = std::max(0.0, std::min(180.0, (double)smoothCutoffDegrees));
    parameters.label = String::valueOrEmpty(component, "name");

    WrapMeshBuilder builder;
    builder.setParameters(parameters);
    builder.setCache(&m_cacheContext->wrapCache);
    size_t sourceCount = 0;
    collectWrapSources(componentIdString, false, &builder, &sourceCount);
    if (0 == sourceCount)
        return nullptr;

    // A garment can take its skin weights from the body it is worn over (another group,
    // usually the creature skin): then the body and the garment bend alike everywhere,
    // whatever shapes the garment was made from.
    // The body is also what the cloth rests on, where it is not free to fold (see
    // ClothFolds): its surface is kept for placing the folds.
    std::string bindTo = String::valueOrEmpty(component, "wrapBindTo");
    std::vector<Vector3> bodyVertices;
    std::vector<std::vector<size_t>> bodyFaces;
    if (!bindTo.empty() && bindTo != componentIdString && nullptr != findComponent(bindTo)) {
        WrapMeshBuilder probe;
        collectBindSamples(bindTo, &probe, 0, &bodyVertices, &bodyFaces);
        if (probe.bindSampleCount() > 0) {
            builder.clearBindSamples();
            collectBindSamples(bindTo, &builder);
        }
        // the body's own wrap (a creature skin) is its visible surface
        const auto& bodyCache = m_cacheContext->components[bindTo];
        if (bodyCache.wrapOutput && bodyCache.wrapOutput->mesh && !bodyCache.wrapOutput->mesh->isNull()) {
            bodyVertices.clear();
            bodyFaces.clear();
            bodyCache.wrapOutput->mesh->fetch(bodyVertices, bodyFaces);
        }
    }
    // the node spheres and part edges behind the weights (see WrapMeshBuilder::BindNode),
    // also the skeleton the folds are placed from
    std::vector<WrapMeshBuilder::BindNode> skeletonNodes;
    std::vector<std::pair<Uuid, Uuid>> skeletonLinks;
    {
        std::set<std::string> nodeIdStrings;
        for (const auto& nodeId : builder.bindNodeIds()) {
            std::string nodeIdString = nodeId.toString();
            auto findNode = m_snapshot->nodes.find(nodeIdString);
            if (findNode == m_snapshot->nodes.end())
                continue;
            const auto& node = findNode->second;
            std::string partIdString = String::valueOrEmpty(node, "partId");
            auto findPart = m_snapshot->parts.find(partIdString);
            if (findPart == m_snapshot->parts.end())
                continue;
            std::string mirrorFrom = String::valueOrEmpty(findPart->second, "__mirrorFromPartId");
            double x = String::toFloat(String::valueOrEmpty(node, "x")) - m_mainProfileMiddleX;
            if (!String::valueOrEmpty(node, "__mirrorFromNodeId").empty())
                x = -x;
            WrapMeshBuilder::BindNode bindNode;
            bindNode.id = nodeId;
            bindNode.position = Vector3(x,
                m_mainProfileMiddleY - String::toFloat(String::valueOrEmpty(node, "y")),
                m_sideProfileMiddleX - String::toFloat(String::valueOrEmpty(node, "z")));
            bindNode.radius = String::toFloat(String::valueOrEmpty(node, "radius"));
            bindNode.group = partIdString;
            bindNode.twinGroup = mirrorFrom.empty() ? partIdString : mirrorFrom;
            builder.addBindNode(bindNode);
            skeletonNodes.push_back(bindNode);
            nodeIdStrings.insert(nodeIdString);
        }
        for (const auto& edgeIt : m_snapshot->edges) {
            std::string from = String::valueOrEmpty(edgeIt.second, "from");
            std::string to = String::valueOrEmpty(edgeIt.second, "to");
            if (nodeIdStrings.count(from) && nodeIdStrings.count(to)) {
                builder.addBindLink(Uuid(from), Uuid(to));
                skeletonLinks.push_back({ Uuid(from), Uuid(to) });
            }
        }
    }
    if (!builder.build()) {
        dust3dDebug << "Wrap of component" << componentIdString.c_str() << "failed:" << builder.errorMessage().c_str();
        m_isSuccessful = false;
        return nullptr;
    }

    const auto& vertices = builder.resultVertices();
    const auto& triangles = builder.resultTriangles();
    const auto& triangleUvs = builder.resultTriangleUvs();
    const auto& nodeWeights = builder.resultVertexNodeWeights();

    // Folds and wrinkles: placed where and the way the cloth folds (from where it rests on
    // the body and where it stands free, the body's skeleton and the openings), baked into
    // the normal map by the texture generator.
    {
        std::string foldText;
        if (readFloat("wrapWrinkles", 0.0) > 0.0) {
            ClothFolds::PlacementInput input;
            input.vertices = vertices;
            input.triangles = triangles;
            std::map<Uuid, size_t> nodeIndices;
            for (const auto& node : skeletonNodes) {
                nodeIndices[node.id] = input.nodes.size();
                input.nodes.push_back({ node.position, node.radius, node.group });
            }
            for (const auto& link : skeletonLinks) {
                auto first = nodeIndices.find(link.first), second = nodeIndices.find(link.second);
                if (first != nodeIndices.end() && second != nodeIndices.end())
                    input.links.push_back({ first->second, second->second });
            }
            input.bodyVertices = bodyVertices;
            input.bodyTriangles = bodyFaces;
            input.cloth = cloth;
            input.drape = parameters.drape;
            input.sizeScale = std::max(0.2, std::min(4.0, readFloat("wrapWrinkleSize", 1.0)));
            input.seed = SurfacePattern::seedFromString(componentIdString);
            foldText = ClothFolds::serialize(ClothFolds::place(input));
        }
        if (componentIdString != to_string(Uuid()))
            m_snapshot->components[componentIdString]["__wrapFolds"] = foldText;
        m_cacheContext->components[componentIdString].wrapFolds = foldText;
    }

    // A wrap without a colour of its own takes the colour of most of what it wraps.
    Color wrapColor = color;
    if (String::valueOrEmpty(component, "color").empty()) {
        std::map<std::string, size_t> colorVotes;
        std::map<std::string, Color> colorByName;
        for (size_t v = 0; v < vertices.size(); ++v) {
            if (nodeWeights[v].empty())
                continue;
            for (const auto& cacheIt : m_cacheContext->components) {
                auto findNode = cacheIt.second.nodeMap.find(nodeWeights[v].front().nodeId);
                if (findNode == cacheIt.second.nodeMap.end())
                    continue;
                std::string name = findNode->second.color.toString();
                colorVotes[name]++;
                colorByName[name] = findNode->second.color;
                break;
            }
        }
        size_t best = 0;
        for (const auto& it : colorVotes) {
            if (it.second > best) {
                best = it.second;
                wrapColor = colorByName[it.first];
            }
        }
        if (best > 0 && componentIdString != to_string(Uuid())) {
            m_snapshot->components[componentIdString]["color"] = wrapColor.toString();
            m_cacheContext->components[componentIdString].wrapColor = wrapColor.toString();
        }
    }

    float cutoff = smoothCutoffDegrees > 0.0f ? smoothCutoffDegrees : (cloth ? 60.0f : 89.0f);
    Uuid componentId(componentIdString);
    auto& uvs = output.componentTriangleUvs[componentId];
    for (size_t t = 0; t < triangles.size(); ++t) {
        const auto& triangle = triangles[t];
        uvs.insert({ { PositionKey(vertices[triangle[0]]), PositionKey(vertices[triangle[1]]), PositionKey(vertices[triangle[2]]) },
            triangleUvs[t] });
    }
    for (const auto& diagonal : builder.resultQuadDiagonals())
        output.sharedQuadEdges.insert({ PositionKey(vertices[diagonal.first]), PositionKey(vertices[diagonal.second]) });
    for (size_t v = 0; v < vertices.size(); ++v) {
        PositionKey key(vertices[v]);
        std::vector<std::pair<Uuid, float>> weights;
        for (const auto& w : nodeWeights[v])
            weights.push_back({ w.nodeId, w.weight });
        output.positionToNodeWeights[key] = weights;
        output.positionToVertexAttribute[key] = ObjectVertexAttribute { wrapColor, cutoff };
    }
    dust3dDebug << "Wrap of component" << componentIdString.c_str() << ":" << (cloth ? "cloth" : "skin")
                << sourceCount << "sources," << vertices.size() << "vertices," << triangles.size() << "triangles, cell" << builder.cellSize();
    return std::make_unique<MeshState>(vertices, triangles);
}

void MeshGenerator::collectBrokenTriangles(const std::string& componentIdString)
{
    const auto& component = findComponent(componentIdString);
    for (const auto& childIdString : String::split(String::valueOrEmpty(*component, "children"), ',')) {
        if (childIdString.empty())
            continue;
        collectBrokenTriangles(childIdString);
    }
    const auto& componentCache = m_cacheContext->components[componentIdString];
    for (const auto& triangle : componentCache.brokenTriangles) {
        m_object->brokenTrianglesToComponentIdMap.insert({ triangle, Uuid(componentIdString) });
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

void MeshGenerator::interpolateEdgesAroundJoints()
{
    // Build part-to-edges mapping from snapshot
    std::map<std::string, std::set<std::string>> partEdgeIds;
    for (const auto& edge : m_snapshot->edges) {
        std::string partId = String::valueOrEmpty(edge.second, "partId");
        if (!partId.empty())
            partEdgeIds[partId].insert(edge.first);
    }

    for (auto& partEntry : partEdgeIds) {
        const std::string& partIdString = partEntry.first;
        auto findPart = m_snapshot->parts.find(partIdString);
        if (findPart == m_snapshot->parts.end())
            continue;
        auto target = PartTargetFromString(String::valueOrEmpty(findPart->second, "target").c_str());
        if (PartTarget::Model != target && PartTarget::ImportedModel != target)
            continue;
        // "interpolated" = "false": rings only at the part's own nodes, no end rings either
        // (rigid, low-poly and hard-surface parts)
        std::string interpolatedString = String::valueOrEmpty(findPart->second, "interpolated");
        if (!interpolatedString.empty() && !String::isTrue(interpolatedString))
            continue;
        std::vector<std::string> edgesToInterpolate;
        for (const auto& edgeIdString : partEntry.second) {
            auto findEdge = m_snapshot->edges.find(edgeIdString);
            if (findEdge == m_snapshot->edges.end())
                continue;
            auto& edge = findEdge->second;
            std::string fromNodeId = String::valueOrEmpty(edge, "from");
            std::string toNodeId = String::valueOrEmpty(edge, "to");
            auto findFromNode = m_snapshot->nodes.find(fromNodeId);
            auto findToNode = m_snapshot->nodes.find(toNodeId);
            if (findFromNode == m_snapshot->nodes.end() || findToNode == m_snapshot->nodes.end())
                continue;
            auto& fromNode = findFromNode->second;
            auto& toNode = findToNode->second;
            float fromX = String::toFloat(String::valueOrEmpty(fromNode, "x"));
            float fromY = String::toFloat(String::valueOrEmpty(fromNode, "y"));
            float fromZ = String::toFloat(String::valueOrEmpty(fromNode, "z"));
            float fromRadius = String::toFloat(String::valueOrEmpty(fromNode, "radius"));
            float toX = String::toFloat(String::valueOrEmpty(toNode, "x"));
            float toY = String::toFloat(String::valueOrEmpty(toNode, "y"));
            float toZ = String::toFloat(String::valueOrEmpty(toNode, "z"));
            float toRadius = String::toFloat(String::valueOrEmpty(toNode, "radius"));
            float dx = toX - fromX;
            float dy = toY - fromY;
            float dz = toZ - fromZ;
            float edgeLength = std::sqrt(dx * dx + dy * dy + dz * dz);
            if (edgeLength <= (fromRadius + toRadius) * 1.5f)
                continue;
            edgesToInterpolate.push_back(edgeIdString);
        }
        for (const auto& edgeIdString : edgesToInterpolate) {
            auto& edge = m_snapshot->edges[edgeIdString];
            std::string fromNodeId = String::valueOrEmpty(edge, "from");
            std::string toNodeId = String::valueOrEmpty(edge, "to");
            std::string boneName = String::valueOrEmpty(edge, "boneName");
            auto& fromNode = m_snapshot->nodes[fromNodeId];
            auto& toNode = m_snapshot->nodes[toNodeId];
            float fromX = String::toFloat(String::valueOrEmpty(fromNode, "x"));
            float fromY = String::toFloat(String::valueOrEmpty(fromNode, "y"));
            float fromZ = String::toFloat(String::valueOrEmpty(fromNode, "z"));
            float fromRadius = String::toFloat(String::valueOrEmpty(fromNode, "radius"));
            float toX = String::toFloat(String::valueOrEmpty(toNode, "x"));
            float toY = String::toFloat(String::valueOrEmpty(toNode, "y"));
            float toZ = String::toFloat(String::valueOrEmpty(toNode, "z"));
            float toRadius = String::toFloat(String::valueOrEmpty(toNode, "radius"));
            float dx = toX - fromX;
            float dy = toY - fromY;
            float dz = toZ - fromZ;
            float edgeLength = std::sqrt(dx * dx + dy * dy + dz * dz);
            if (edgeLength < 0.0001f)
                continue;
            float ndx = dx / edgeLength;
            float ndy = dy / edgeLength;
            float ndz = dz / edgeLength;
            float a1x = fromX + ndx * fromRadius;
            float a1y = fromY + ndy * fromRadius;
            float a1z = fromZ + ndz * fromRadius;
            float a2x = toX - ndx * toRadius;
            float a2y = toY - ndy * toRadius;
            float a2z = toZ - ndz * toRadius;
            float t1 = fromRadius / edgeLength;
            float t2 = toRadius / edgeLength;
            float a1Radius = fromRadius * (1.0f - t1) + toRadius * t1;
            float a2Radius = toRadius * (1.0f - t2) + fromRadius * t2;

            // Build deterministic IDs by combining parts of existing from/to node IDs and edge ID
            // UUID format: {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
            //               1        10   15   20   25
            auto extractRaw = [](const std::string& uuid) {
                return uuid.substr(1, 8) + uuid.substr(10, 4) + uuid.substr(15, 4) + uuid.substr(20, 4) + uuid.substr(25, 12);
            };
            auto formatUuid = [](const std::string& raw) {
                return "{" + raw.substr(0, 8) + "-" + raw.substr(8, 4) + "-" + raw.substr(12, 4) + "-" + raw.substr(16, 4) + "-" + raw.substr(20, 12) + "}";
            };
            std::string fromRaw = extractRaw(fromNodeId);
            std::string toRaw = extractRaw(toNodeId);
            std::string edgeRaw = extractRaw(edgeIdString);
            // newNodeId1: first half from fromNodeId, second half from edgeId
            std::string newNodeId1 = formatUuid(fromRaw.substr(0, 16) + edgeRaw.substr(16, 16));
            // newNodeId2: first half from toNodeId, second half from edgeId
            std::string newNodeId2 = formatUuid(toRaw.substr(0, 16) + edgeRaw.substr(16, 16));
            // newEdgeId1: first half from edgeId, second half from fromNodeId
            std::string newEdgeId1 = formatUuid(edgeRaw.substr(0, 16) + fromRaw.substr(16, 16));
            // newEdgeId2: first half from fromNodeId, second half from toNodeId
            std::string newEdgeId2 = formatUuid(fromRaw.substr(0, 16) + toRaw.substr(16, 16));
            // newEdgeId3: first half from edgeId, second half from toNodeId
            std::string newEdgeId3 = formatUuid(edgeRaw.substr(0, 16) + toRaw.substr(16, 16));

            std::map<std::string, std::string> node1;
            node1["id"] = newNodeId1;
            node1["x"] = String::fromDouble(a1x);
            node1["y"] = String::fromDouble(a1y);
            node1["z"] = String::fromDouble(a1z);
            node1["radius"] = String::fromDouble(a1Radius);
            node1["partId"] = partIdString;
            std::map<std::string, std::string> node2;
            node2["id"] = newNodeId2;
            node2["x"] = String::fromDouble(a2x);
            node2["y"] = String::fromDouble(a2y);
            node2["z"] = String::fromDouble(a2z);
            node2["radius"] = String::fromDouble(a2Radius);
            node2["partId"] = partIdString;
            // carry a per-node cross-section scale along the edge like the radius
            for (const char* key : { "deformWidth", "deformThickness" }) {
                std::string fromString = String::valueOrEmpty(fromNode, key);
                std::string toString = String::valueOrEmpty(toNode, key);
                if (fromString.empty() && toString.empty())
                    continue;
                float fromValue = fromString.empty() ? 1.0f : String::toFloat(fromString);
                float toValue = toString.empty() ? 1.0f : String::toFloat(toString);
                node1[key] = String::fromDouble(fromValue * (1.0f - t1) + toValue * t1);
                node2[key] = String::fromDouble(toValue * (1.0f - t2) + fromValue * t2);
            }
            m_snapshot->nodes[newNodeId1] = node1;
            m_snapshot->nodes[newNodeId2] = node2;
            auto createEdge = [&](const std::string& id, const std::string& from, const std::string& to) {
                std::map<std::string, std::string> e;
                e["id"] = id;
                e["from"] = from;
                e["to"] = to;
                e["partId"] = partIdString;
                if (!boneName.empty())
                    e["boneName"] = boneName;
                return e;
            };
            m_snapshot->edges[newEdgeId1] = createEdge(newEdgeId1, fromNodeId, newNodeId1);
            m_snapshot->edges[newEdgeId2] = createEdge(newEdgeId2, newNodeId1, newNodeId2);
            m_snapshot->edges[newEdgeId3] = createEdge(newEdgeId3, newNodeId2, toNodeId);
            m_snapshot->edges.erase(edgeIdString);
        }
    }
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

    // Create mirrored nodes and edges for mirrored parts
    std::map<std::string, std::string> nodeOldToNewMap;
    std::vector<std::map<std::string, std::string>> newNodes;
    std::vector<std::map<std::string, std::string>> newEdges;

    // Find all nodes that belong to mirrored parts and create mirrored versions
    for (const auto& nodeIt : m_snapshot->nodes) {
        std::string nodePartId = String::valueOrEmpty(nodeIt.second, "partId");
        auto findMirroredPart = partOldToNewMap.find(nodePartId);
        if (findMirroredPart == partOldToNewMap.end())
            continue;

        // Create mirrored node with flipped X coordinate
        std::map<std::string, std::string> mirroredNode = nodeIt.second;
        std::string newNodeIdString = reverseUuid(nodeIt.first);
        nodeOldToNewMap.insert({ nodeIt.first, newNodeIdString });

        // Update partId to point to the new mirrored part
        mirroredNode["partId"] = findMirroredPart->second;
        mirroredNode["id"] = newNodeIdString;
        mirroredNode["__mirrorFromNodeId"] = nodeIt.first;
        newNodes.push_back(mirroredNode);
    }

    // Find all edges that belong to mirrored parts and create mirrored versions
    for (const auto& edgeIt : m_snapshot->edges) {
        std::string edgePartId = String::valueOrEmpty(edgeIt.second, "partId");
        auto findMirroredPart = partOldToNewMap.find(edgePartId);
        if (findMirroredPart == partOldToNewMap.end())
            continue;

        // Create mirrored edge
        std::map<std::string, std::string> mirroredEdge = edgeIt.second;
        std::string newEdgeIdString = reverseUuid(edgeIt.first);

        // Update edge endpoints to use new mirrored nodes
        std::string fromNodeId = String::valueOrEmpty(mirroredEdge, "from");
        std::string toNodeId = String::valueOrEmpty(mirroredEdge, "to");

        auto findFromNode = nodeOldToNewMap.find(fromNodeId);
        auto findToNode = nodeOldToNewMap.find(toNodeId);

        if (findFromNode != nodeOldToNewMap.end())
            mirroredEdge["from"] = findFromNode->second;
        if (findToNode != nodeOldToNewMap.end())
            mirroredEdge["to"] = findToNode->second;

        // Update partId to point to the new mirrored part
        mirroredEdge["partId"] = findMirroredPart->second;
        mirroredEdge["id"] = newEdgeIdString;
        mirroredEdge["__mirrorFromEdgeId"] = edgeIt.first;

        auto swapBoneNameLeftRight = [](const std::string& value) {
            std::string swapped;
            swapped.reserve(value.size());
            size_t i = 0;
            while (i < value.size()) {
                if (i + 4 <= value.size() && value.compare(i, 4, "Left") == 0) {
                    swapped += "Right";
                    i += 4;
                } else if (i + 5 <= value.size() && value.compare(i, 5, "Right") == 0) {
                    swapped += "Left";
                    i += 5;
                } else {
                    swapped.push_back(value[i]);
                    ++i;
                }
            }
            return swapped;
        };

        std::string boneName = String::valueOrEmpty(mirroredEdge, "boneName");
        if (!boneName.empty()) {
            std::string newBoneName = swapBoneNameLeftRight(boneName);
            if (newBoneName != boneName)
                mirroredEdge["boneName"] = newBoneName;
        }

        newEdges.push_back(mirroredEdge);
    }

    // Add new mirrored nodes and edges to snapshot
    for (const auto& node : newNodes)
        m_snapshot->nodes[String::valueOrEmpty(node, "id")] = node;
    for (const auto& edge : newEdges)
        m_snapshot->edges[String::valueOrEmpty(edge, "id")] = edge;

    // Mark original nodes with mirror references
    for (const auto& it : nodeOldToNewMap)
        m_snapshot->nodes[it.first]["__mirroredByNodeId"] = it.second;

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

    // Components are keyed by id, and ids are regenerated randomly whenever a document
    // is loaded, so iterating the map directly would union mirrored parts in a random
    // order (making the output mesh differ from run to run). Order them by where their
    // source component appears in the component tree instead.
    std::map<std::string, size_t> componentTreeOrder;
    {
        std::function<void(const std::string&)> visitChildren;
        visitChildren = [&](const std::string& childrenString) {
            for (const auto& childId : String::split(childrenString, ',')) {
                if (childId.empty() || componentTreeOrder.count(childId))
                    continue;
                componentTreeOrder.insert({ childId, componentTreeOrder.size() });
                auto findChild = m_snapshot->components.find(childId);
                if (findChild != m_snapshot->components.end())
                    visitChildren(String::valueOrEmpty(findChild->second, "children"));
            }
        };
        visitChildren(String::valueOrEmpty(m_snapshot->rootComponent, "children"));
    }
    std::vector<std::pair<size_t, std::map<std::string, std::string>>> orderedNewComponents;
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
        auto findOrder = componentTreeOrder.find(componentIt.first);
        orderedNewComponents.emplace_back(findOrder == componentTreeOrder.end() ? std::numeric_limits<size_t>::max() : findOrder->second,
            std::move(mirroredComponent));
    }
    std::stable_sort(orderedNewComponents.begin(), orderedNewComponents.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });
    for (auto& it : orderedNewComponents)
        newComponents.push_back(std::move(it.second));

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

// Preview the complete visible output of a group, including meshes which bypass
// boolean combination. Resolve properties before the application normalizes positions.
void MeshGenerator::collectComponentPreview(const std::string& componentIdString,
    bool includeMesh, ComponentPreview& preview)
{
    const auto* component = findComponent(componentIdString);
    auto findCache = m_cacheContext->components.find(componentIdString);
    if (nullptr == component || findCache == m_cacheContext->components.end())
        return;
    const auto& cache = findCache->second;
    auto appendMesh = [&](const MeshState* mesh, const GeneratedComponent& source) {
        if (nullptr == mesh || mesh->isNull())
            return;
        std::vector<Vector3> vertices;
        std::vector<std::vector<size_t>> triangles;
        mesh->fetch(vertices, triangles);
        size_t offset = preview.vertices.size();
        for (auto triangle : triangles) {
            for (auto& index : triangle)
                index += offset;
            preview.triangles.push_back(std::move(triangle));
        }
        for (const auto& vertex : vertices) {
            Color vertexColor = preview.color;
            float metalness = 0.0f;
            float roughness = 1.0f;
            auto findNodeId = source.positionToNodeIdMap.find(vertex);
            if (findNodeId != source.positionToNodeIdMap.end()) {
                auto findNode = source.nodeMap.find(findNodeId->second);
                if (findNode != source.nodeMap.end())
                    vertexColor = findNode->second.color;
                auto snapshotNode = m_snapshot->nodes.find(findNodeId->second.toString());
                if (snapshotNode != m_snapshot->nodes.end()) {
                    auto part = m_cacheContext->parts.find(String::valueOrEmpty(snapshotNode->second, "partId"));
                    if (part != m_cacheContext->parts.end()) {
                        metalness = part->second.metalness;
                        roughness = part->second.roughness;
                    }
                }
            } else {
                auto attribute = source.positionToVertexAttribute.find(vertex);
                if (attribute != source.positionToVertexAttribute.end())
                    vertexColor = attribute->second.color;
            }
            auto importedColor = source.importedVertexColorMap.find(vertex);
            if (importedColor != source.importedVertexColorMap.end())
                vertexColor = importedColor->second;
            preview.vertexProperties.emplace_back(vertexColor, metalness, roughness);
        }
        preview.vertices.insert(preview.vertices.end(), vertices.begin(), vertices.end());
        for (const auto& componentUvs : source.componentTriangleUvs)
            preview.triangleUvs.insert(componentUvs.second.begin(), componentUvs.second.end());
    };
    if (includeMesh)
        appendMesh(cache.mesh.get(), cache);
    if (cache.wrapOutput)
        appendMesh(cache.wrapOutput->mesh.get(), *cache.wrapOutput);
    for (const auto& childId : String::split(String::valueOrEmpty(*component, "children"), ',')) {
        const auto* child = findComponent(childId);
        if (nullptr == child)
            continue;
        collectComponentPreview(childId, CombineMode::Uncombined == componentCombineMode(child), preview);
    }
}

void MeshGenerator::generateDisabledComponentPreviews()
{
    bool hasDisabledParts = false;
    for (const auto& part : m_snapshot->parts)
        hasDisabledParts |= String::isTrue(String::valueOrEmpty(part.second, "disabled"));
    if (!hasDisabledParts)
        return;

    // Disabled parts still have leaf thumbnails. Give their empty parent groups
    // thumbnails as well, without enabling them in the document or render cache.
    auto* previewSnapshot = new Snapshot(*m_snapshot);
    for (auto& part : previewSnapshot->parts)
        part.second["disabled"] = "false";
    GeneratedCacheContext previewCache;
    MeshGenerator previewGenerator(previewSnapshot);
    previewGenerator.m_cacheContext = &previewCache;
    previewGenerator.m_defaultPartColor = m_defaultPartColor;
    previewGenerator.m_mainProfileMiddleX = m_mainProfileMiddleX;
    previewGenerator.m_mainProfileMiddleY = m_mainProfileMiddleY;
    previewGenerator.m_sideProfileMiddleX = m_sideProfileMiddleX;
    previewGenerator.m_importedModelData = m_importedModelData;
    // The snapshot already has interpolated and mirrored nodes.
    previewGenerator.collectParts();
    for (auto& entry : m_generatedComponentPreviews) {
        if (!entry.second.triangles.empty() || !entry.second.cutFaceTemplate.empty())
            continue;
        const auto* component = findComponent(entry.first.toString());
        if (nullptr == component || "partId" == String::valueOrEmpty(*component, "linkDataType"))
            continue;
        CombineMode mode;
        previewGenerator.combineComponentMesh(entry.first.toString(), &mode);
        auto generated = previewGenerator.m_generatedComponentPreviews.find(entry.first);
        if (generated != previewGenerator.m_generatedComponentPreviews.end() && !generated->second.triangles.empty())
            entry.second = generated->second;
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

    if (seamReportEnabled()) {
        // Tells tools this build reports seams, even for a model with nothing to combine.
        static bool announced = false;
        if (!announced) {
            announced = true;
            std::cout << "SEAM_REPORT_SUPPORTED 1" << std::endl;
        }
    }

    m_mainProfileMiddleX = String::toFloat(String::valueOrEmpty(m_snapshot->canvas, "originX"));
    m_mainProfileMiddleY = String::toFloat(String::valueOrEmpty(m_snapshot->canvas, "originY"));
    m_sideProfileMiddleX = String::toFloat(String::valueOrEmpty(m_snapshot->canvas, "originZ"));

    interpolateEdgesAroundJoints();
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
    m_generatedComponentIds.clear();

    CombineMode combineMode;
    auto combinedMesh = combineComponentMesh(to_string(Uuid()), &combineMode);

    const auto& componentCache = m_cacheContext->components[to_string(Uuid())];

    m_object->positionToNodeIdMap = componentCache.positionToNodeIdMap;
    m_object->nodeMap = componentCache.nodeMap;
    m_object->positionToNodeWeights = componentCache.positionToNodeWeights;
    m_object->positionToVertexAttribute = componentCache.positionToVertexAttribute;
    m_object->componentTriangleUvs = componentCache.componentTriangleUvs;

    std::vector<Vector3> combinedVertices;
    std::vector<std::vector<size_t>> combinedFaces;
    if (nullptr != combinedMesh) {
        combinedMesh->fetch(combinedVertices, combinedFaces);
        m_object->seamTriangleUvs = combinedMesh->seamTriangleUvs;
        recoverQuads(combinedVertices, combinedFaces, componentCache.sharedQuadEdges, m_object->triangleAndQuads);
        m_object->vertices = combinedVertices;
        m_object->triangles = combinedFaces;
    }

    // Recursively check uncombined components
    collectUncombinedComponent(to_string(Uuid()));
    collectBrokenTriangles(to_string(Uuid()));

    postprocessObject(m_object);

    // Override vertex colors from imported models
    if (!componentCache.importedVertexColorMap.empty()) {
        for (size_t i = 0; i < m_object->vertices.size(); ++i) {
            auto findColor = componentCache.importedVertexColorMap.find(m_object->vertices[i]);
            if (findColor != componentCache.importedVertexColorMap.end()) {
                m_object->vertexColors[i] = findColor->second;
            }
        }
    }

    // Override vertex normals from imported models
    if (!componentCache.importedTriangleNormals.empty()) {
        const auto* triNormals = m_object->triangleVertexNormals();
        if (triNormals && triNormals->size() == m_object->triangles.size()) {
            std::vector<std::vector<Vector3>> newTriNormals = *triNormals;
            for (size_t ti = 0; ti < m_object->triangles.size(); ++ti) {
                const auto& face = m_object->triangles[ti];
                if (face.size() < 3)
                    continue;
                std::array<PositionKey, 3> triKey = {
                    PositionKey(m_object->vertices[face[0]]),
                    PositionKey(m_object->vertices[face[1]]),
                    PositionKey(m_object->vertices[face[2]])
                };
                auto findTriNormals = componentCache.importedTriangleNormals.find(triKey);
                if (findTriNormals != componentCache.importedTriangleNormals.end()) {
                    for (size_t j = 0; j < 3; ++j)
                        newTriNormals[ti][j] = findTriNormals->second[j];
                }
            }
            m_object->setTriangleVertexNormals(newTriNormals);
        }
    }

    generateDisabledComponentPreviews();

    if (needDeleteCacheContext) {
        delete m_cacheContext;
        m_cacheContext = nullptr;
    }
}

}
