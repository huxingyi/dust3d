/*
 *  Copyright (c) 2026 Jeremy HU <jeremy-at-dust3d dot org>. All rights reserved. 
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
#include <dust3d/base/debug.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/part_target.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/string.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>
#include <limits>

namespace dust3d {

static bool targetPartIsModel(const Snapshot* snapshot, const std::string& partIdString)
{
    if (!snapshot || partIdString.empty())
        return false;

    auto partIt = snapshot->parts.find(partIdString);
    if (partIt == snapshot->parts.end())
        return false;

    return PartTarget::Model == PartTargetFromString(String::valueOrEmpty(partIt->second, "target").c_str());
}

static bool nodeBelongsToModelPart(const Snapshot* snapshot, const Uuid& nodeId)
{
    if (!snapshot)
        return false;

    auto it = snapshot->nodes.find(nodeId.toString());
    if (it == snapshot->nodes.end())
        return false;

    return targetPartIsModel(snapshot, String::valueOrEmpty(it->second, "partId"));
}

static bool edgeBelongsToModelPart(const Snapshot* snapshot,
    const std::map<std::string, std::string>& edgeAttributes)
{
    if (!snapshot)
        return false;

    std::string fromNode = String::valueOrEmpty(edgeAttributes, "from");
    std::string toNode = String::valueOrEmpty(edgeAttributes, "to");
    if (fromNode.empty() || toNode.empty())
        return false;

    Uuid fromId(fromNode);
    Uuid toId(toNode);
    return nodeBelongsToModelPart(snapshot, fromId)
        && nodeBelongsToModelPart(snapshot, toId);
}

RigGenerator::RigGenerator()
{
}

RigGenerator::~RigGenerator()
{
}

bool RigGenerator::generateRig(const Snapshot* snapshot, const RigStructure& templateRig, RigStructure& actualRig)
{
    if (!snapshot) {
        m_errorMessage = "Snapshot not initialized";
        return false;
    }

    actualRig = templateRig; // Copy template structure

    // Extract coordinate transformation offsets from snapshot's canvas
    m_mainProfileMiddleX = String::toFloat(String::valueOrEmpty(snapshot->canvas, "originX"));
    m_mainProfileMiddleY = String::toFloat(String::valueOrEmpty(snapshot->canvas, "originY"));
    m_sideProfileMiddleX = String::toFloat(String::valueOrEmpty(snapshot->canvas, "originZ"));

    // Clear template positions - they are only for template visualization
    for (auto& bone : actualRig.bones) {
        bone.posX = 0.0f;
        bone.posY = 0.0f;
        bone.posZ = 0.0f;
        bone.endX = 0.0f;
        bone.endY = 0.0f;
        bone.endZ = 0.0f;
    }

    // Build bone name -> index map for parent lookups
    std::map<std::string, size_t> boneNameToIndex;
    for (size_t i = 0; i < actualRig.bones.size(); ++i) {
        boneNameToIndex[actualRig.bones[i].name] = i;
    }

    // Pre-collect all nodes that appear in any edge with a bone name (across all bones).
    // Used later to identify isolated single nodes.
    std::set<Uuid> allEdgeNodes;
    std::map<std::string, std::set<Uuid>> boneEdgeNodesMap;
    for (const auto& edgePair : snapshot->edges) {
        const auto& edgeAttributes = edgePair.second;
        std::string edgeBoneName = String::valueOrEmpty(edgeAttributes, "boneName");
        if (edgeBoneName.empty())
            continue;
        if (!edgeBelongsToModelPart(snapshot, edgeAttributes))
            continue;
        std::string fromNode = String::valueOrEmpty(edgeAttributes, "from");
        std::string toNode = String::valueOrEmpty(edgeAttributes, "to");
        if (!fromNode.empty()) {
            Uuid id(fromNode);
            allEdgeNodes.insert(id);
            boneEdgeNodesMap[edgeBoneName].insert(id);
        }
        if (!toNode.empty()) {
            Uuid id(toNode);
            allEdgeNodes.insert(id);
            boneEdgeNodesMap[edgeBoneName].insert(id);
        }
    }

    // Process bones in topological order (parents before children)
    std::vector<size_t> processingOrder;
    std::set<std::string> processed;
    while (processingOrder.size() < actualRig.bones.size()) {
        bool progress = false;
        for (size_t i = 0; i < actualRig.bones.size(); ++i) {
            const auto& bone = actualRig.bones[i];
            if (processed.count(bone.name))
                continue;
            if (bone.parent.empty() || processed.count(bone.parent)) {
                processingOrder.push_back(i);
                processed.insert(bone.name);
                progress = true;
            }
        }
        if (!progress) {
            // Circular or missing parent - add remaining in order
            for (size_t i = 0; i < actualRig.bones.size(); ++i) {
                if (!processed.count(actualRig.bones[i].name)) {
                    processingOrder.push_back(i);
                    processed.insert(actualRig.bones[i].name);
                }
            }
            break;
        }
    }

    // For each bone, compute actual position from edge assignments
    for (size_t boneIdx : processingOrder) {
        auto& bone = actualRig.bones[boneIdx];

        std::vector<std::vector<Uuid>> nodeChains;
        if (!extractNodeChainsForBone(snapshot, bone.name, nodeChains)) {
            dust3dDebug << "No edges assigned to bone:" << bone.name;
            if (bone.parent.empty()) {
                // Root bone with no bindings: give it a tiny upward tail so it has
                // a valid, non-degenerate orientation for exporters and game engines.
                bone.endY = -0.25f;
            }
            continue;
        }

        // Attach isolated single nodes (nodes with no edge-bone assignment) to
        // this bone if they are nearest to this bone's edge-connected nodes.
        if (!boneEdgeNodesMap[bone.name].empty()) {
            attachSingleNodesToBone(snapshot, bone.name,
                boneEdgeNodesMap[bone.name], allEdgeNodes, nodeChains);
        }

        // Determine reference point from parent bone' position.
        // Root bones (no parent) use the origin.
        float refX = 0, refY = 0, refZ = 0;
        if (!bone.parent.empty()) {
            auto parentIt = boneNameToIndex.find(bone.parent);
            if (parentIt != boneNameToIndex.end()) {
                const auto& parentBone = actualRig.bones[parentIt->second];
                refX = parentBone.posX;
                refY = parentBone.posY;
                refZ = parentBone.posZ;
            }
        }

        // Orient each chain so its end closest to the reference comes first
        for (auto& chain : nodeChains) {
            orientChainTowardPoint(snapshot, chain, refX, refY, refZ);
        }

        // Sort chains by distance of their front node to the reference
        std::sort(nodeChains.begin(), nodeChains.end(),
            [&](const std::vector<Uuid>& a, const std::vector<Uuid>& b) {
                float ax = 0, ay = 0, az = 0, bx = 0, by = 0, bz = 0;
                getNodePosition(snapshot, a.front(), ax, ay, az);
                getNodePosition(snapshot, b.front(), bx, by, bz);
                float da = (ax - refX) * (ax - refX) + (ay - refY) * (ay - refY) + (az - refZ) * (az - refZ);
                float db = (bx - refX) * (bx - refX) + (by - refY) * (by - refY) + (bz - refZ) * (bz - refZ);
                return da < db;
            });

        // Compute the central bone axis across all chains:
        //   1. Average all chain-front positions -> approximate bone root
        //   2. Average all chain-back  positions -> approximate bone tip
        //   3. Build a unit direction from root to tip
        //   4. Project every chain endpoint onto that axis to find the true extents
        // This handles multiple parallel chains (e.g. cross-section rings of a limb)
        // by producing a single representative centre line rather than picking one
        // arbitrary chain.
        float sumBeginX = 0, sumBeginY = 0, sumBeginZ = 0;
        float sumEndX = 0, sumEndY = 0, sumEndZ = 0;
        size_t validChains = 0;

        for (const auto& chain : nodeChains) {
            float fx, fy, fz, bx, by, bz;
            if (getNodePosition(snapshot, chain.front(), fx, fy, fz) && getNodePosition(snapshot, chain.back(), bx, by, bz)) {
                sumBeginX += fx;
                sumBeginY += fy;
                sumBeginZ += fz;
                sumEndX += bx;
                sumEndY += by;
                sumEndZ += bz;
                ++validChains;
            }
        }

        if (validChains > 0) {
            float avgBeginX = sumBeginX / validChains;
            float avgBeginY = sumBeginY / validChains;
            float avgBeginZ = sumBeginZ / validChains;
            float avgEndX = sumEndX / validChains;
            float avgEndY = sumEndY / validChains;
            float avgEndZ = sumEndZ / validChains;

            float dx = avgEndX - avgBeginX;
            float dy = avgEndY - avgBeginY;
            float dz = avgEndZ - avgBeginZ;
            float axisLen = std::sqrt(dx * dx + dy * dy + dz * dz);

            if (axisLen > 1e-6f) {
                dx /= axisLen;
                dy /= axisLen;
                dz /= axisLen;

                // Project all chain endpoints onto the central axis and track extents
                float minT = 0.0f, maxT = 0.0f;
                bool firstProjection = true;
                for (const auto& chain : nodeChains) {
                    float px, py, pz;
                    for (const Uuid& endNode : { chain.front(), chain.back() }) {
                        if (!getNodePosition(snapshot, endNode, px, py, pz))
                            continue;
                        float t = (px - avgBeginX) * dx + (py - avgBeginY) * dy + (pz - avgBeginZ) * dz;
                        if (firstProjection) {
                            minT = maxT = t;
                            firstProjection = false;
                        } else {
                            minT = std::min(minT, t);
                            maxT = std::max(maxT, t);
                        }
                    }
                }

                bone.posX = avgBeginX + minT * dx;
                bone.posY = avgBeginY + minT * dy;
                bone.posZ = avgBeginZ + minT * dz;
                bone.endX = avgBeginX + maxT * dx;
                bone.endY = avgBeginY + maxT * dy;
                bone.endZ = avgBeginZ + maxT * dz;
            } else {
                bone.posX = avgBeginX;
                bone.posY = avgBeginY;
                bone.posZ = avgBeginZ;
                bone.endX = avgEndX;
                bone.endY = avgEndY;
                bone.endZ = avgEndZ;
            }
        }

        size_t totalNodes = 0;
        for (const auto& chain : nodeChains)
            totalNodes += chain.size();

        dust3dDebug << "Computed bone" << bone.name.c_str()
                    << "from" << totalNodes << "nodes in" << nodeChains.size() << "chains"
                    << "position:" << bone.posX << bone.posY << bone.posZ
                    << "endPosition:" << bone.endX << bone.endY << bone.endZ;
    }

    // Fish rig patch: ensure "Head" bone direction aligns with "BodyFront"
    if (actualRig.type == "Fish") {
        RigNode* headBone = nullptr;
        RigNode* bodyFrontBone = nullptr;
        for (auto& bone : actualRig.bones) {
            if (bone.name == "Head")
                headBone = &bone;
            else if (bone.name == "BodyFront")
                bodyFrontBone = &bone;
        }
        if (headBone && bodyFrontBone) {
            Vector3 headDir(headBone->endX - headBone->posX,
                headBone->endY - headBone->posY,
                headBone->endZ - headBone->posZ);
            Vector3 bodyFrontDir(bodyFrontBone->endX - bodyFrontBone->posX,
                bodyFrontBone->endY - bodyFrontBone->posY,
                bodyFrontBone->endZ - bodyFrontBone->posZ);
            float dot = Vector3::dotProduct(headDir, bodyFrontDir);
            if (dot < 0.0f) {
                // Head is pointing away from BodyFront — reverse it
                std::swap(headBone->posX, headBone->endX);
                std::swap(headBone->posY, headBone->endY);
                std::swap(headBone->posZ, headBone->endZ);
                dust3dDebug << "Fish rig patch: reversed Head bone direction to align with BodyFront";
            }
        }
    }

    // Generate collision capsule radius for each bone in rig data
    for (auto& bone : actualRig.bones) {
        Vector3 start(bone.posX, bone.posY, bone.posZ);
        Vector3 end(bone.endX, bone.endY, bone.endZ);
        float length = (end - start).length();
        if (length < 1e-6f) {
            bone.capsuleRadius = 0.01f;
            continue;
        }

        // Determine capsule radius from node radius average on the bone's node chains.
        std::vector<std::vector<Uuid>> nodeChains;
        float radiusSum = 0.0f;
        size_t radiusCount = 0;
        if (extractNodeChainsForBone(snapshot, bone.name, nodeChains)) {
            for (const auto& chain : nodeChains) {
                for (const auto& nodeId : chain) {
                    float nx = 0, ny = 0, nz = 0;
                    if (getNodePosition(snapshot, nodeId, nx, ny, nz)) {
                        auto it = snapshot->nodes.find(nodeId.toString());
                        if (it != snapshot->nodes.end()) {
                            float nodeRadius = String::toFloat(String::valueOrEmpty(it->second, "radius"));
                            if (nodeRadius > 1e-6f) {
                                radiusSum += nodeRadius;
                                ++radiusCount;
                            }
                        }
                    }
                }
            }
        }

        float radius = 0.0f;
        if (radiusCount > 0)
            radius = radiusSum / static_cast<float>(radiusCount);
        else
            radius = std::max(0.01f, length * 0.12f);

        bone.capsuleRadius = std::max(0.01f, radius);
    }

    m_errorMessage = "";
    return true;
}

bool RigGenerator::computeBoneWorldTransforms(const RigStructure& rigStructure,
    std::map<std::string, Matrix4x4>& boneWorldTransforms)
{
    boneWorldTransforms.clear();

    // Build bone name map for parent lookup and ensure order
    std::map<std::string, size_t> boneNameToIndex;
    for (size_t i = 0; i < rigStructure.bones.size(); ++i) {
        boneNameToIndex[rigStructure.bones[i].name] = i;
    }

    std::vector<size_t> processingOrder;
    std::set<std::string> processed;
    while (processingOrder.size() < rigStructure.bones.size()) {
        bool progress = false;
        for (size_t i = 0; i < rigStructure.bones.size(); ++i) {
            const auto& bone = rigStructure.bones[i];
            if (processed.count(bone.name))
                continue;
            if (bone.parent.empty() || processed.count(bone.parent)) {
                processingOrder.push_back(i);
                processed.insert(bone.name);
                progress = true;
            }
        }
        if (!progress) {
            for (size_t i = 0; i < rigStructure.bones.size(); ++i) {
                if (!processed.count(rigStructure.bones[i].name)) {
                    processingOrder.push_back(i);
                    processed.insert(rigStructure.bones[i].name);
                }
            }
            break;
        }
    }

    for (size_t boneIdx : processingOrder) {
        const auto& node = rigStructure.bones[boneIdx];

        Vector3 boneHead(node.posX, node.posY, node.posZ);
        Vector3 boneTail(node.endX, node.endY, node.endZ);
        Vector3 boneDirection = boneTail - boneHead;

        Matrix4x4 restTransform;
        restTransform.translate(boneHead);

        if (!boneDirection.isZero()) {
            Quaternion orient = Quaternion::rotationTo(Vector3(0.0, 0.0, 1.0), boneDirection.normalized());
            restTransform.rotate(orient);
        }

        if (!node.parent.empty()) {
            auto parentIt = boneNameToIndex.find(node.parent);
            if (parentIt != boneNameToIndex.end()) {
                auto worldIt = boneWorldTransforms.find(node.parent);
                if (worldIt != boneWorldTransforms.end()) {
                    const Matrix4x4& parentWorld = worldIt->second;
                    Matrix4x4 parentInverse = parentWorld.inverted();
                    Matrix4x4 localTransform = parentInverse;
                    localTransform *= restTransform;

                    Matrix4x4 finalWorld = parentWorld;
                    finalWorld *= localTransform;
                    boneWorldTransforms.emplace(node.name, finalWorld);
                    continue;
                }
            }
        }

        boneWorldTransforms.emplace(node.name, restTransform);
    }

    return true;
}

bool RigGenerator::computeBoneInverseBindMatrices(const RigStructure& rigStructure,
    std::map<std::string, Matrix4x4>& inverseBindMatrices)
{
    inverseBindMatrices.clear();

    std::map<std::string, Matrix4x4> boneWorldTransforms;
    if (!computeBoneWorldTransforms(rigStructure, boneWorldTransforms)) {
        m_errorMessage = "Failed to compute bone world transforms";
        return false;
    }

    for (const auto& pair : boneWorldTransforms) {
        inverseBindMatrices.emplace(pair.first, pair.second.inverted());
    }

    m_errorMessage = "";
    return true;
}

bool RigGenerator::computeNodeBoneInfluences(const Snapshot* snapshot,
    std::map<Uuid, NodeBoneInfluence>& nodeBoneInfluences)
{
    if (!snapshot) {
        m_errorMessage = "Snapshot not initialized";
        return false;
    }

    nodeBoneInfluences.clear();

    // For each node in the snapshot, determine which bones influence it
    for (const auto& nodePair : snapshot->nodes) {
        const std::string& nodeIdString = nodePair.first;
        const std::map<std::string, std::string>& nodeAttributes = nodePair.second;
        Uuid nodeId(nodeIdString);

        if (!nodeBelongsToModelPart(snapshot, nodeId))
            continue;

        // Find all model edges connected to this node
        std::set<std::string> boneNames;
        for (const auto& edgePair : snapshot->edges) {
            const std::map<std::string, std::string>& edgeAttributes = edgePair.second;
            if (!edgeBelongsToModelPart(snapshot, edgeAttributes))
                continue;

            std::string fromNode = String::valueOrEmpty(edgeAttributes, "from");
            std::string toNode = String::valueOrEmpty(edgeAttributes, "to");

            if (fromNode == nodeIdString || toNode == nodeIdString) {
                std::string boneName = String::valueOrEmpty(edgeAttributes, "boneName");
                if (!boneName.empty()) {
                    boneNames.insert(boneName);
                }
            }
        }

        if (boneNames.empty()) {
            // Node has no assigned edges — find nearest node that has a bone assignment
            // and inherit its bone influence.
            float nx = 0, ny = 0, nz = 0;
            if (!getNodePosition(snapshot, nodeId, nx, ny, nz))
                continue;

            float bestDist = std::numeric_limits<float>::max();
            std::string nearestBone;
            for (const auto& edgePair : snapshot->edges) {
                const auto& edgeAttributes = edgePair.second;
                if (!edgeBelongsToModelPart(snapshot, edgeAttributes))
                    continue;
                std::string edgeBoneName = String::valueOrEmpty(edgeAttributes, "boneName");
                if (edgeBoneName.empty())
                    continue;
                for (const auto& key : { std::string("from"), std::string("to") }) {
                    std::string candidateIdStr = String::valueOrEmpty(edgeAttributes, key);
                    if (candidateIdStr.empty() || candidateIdStr == nodeIdString)
                        continue;
                    Uuid candidateId(candidateIdStr);
                    float cx = 0, cy = 0, cz = 0;
                    if (!getNodePosition(snapshot, candidateId, cx, cy, cz))
                        continue;
                    float dx = nx - cx, dy = ny - cy, dz = nz - cz;
                    float dist = dx * dx + dy * dy + dz * dz;
                    if (dist < bestDist) {
                        bestDist = dist;
                        nearestBone = edgeBoneName;
                    }
                }
            }

            if (!nearestBone.empty()) {
                NodeBoneInfluence influence(nearestBone);
                nodeBoneInfluences[nodeId] = influence;
                dust3dDebug << "Single node" << nodeIdString.c_str()
                            << "inherited bone from nearest edge node:" << nearestBone.c_str();
            }
            continue;
        }

        if (boneNames.size() == 1) {
            // Single bone influence
            std::string boneName = *boneNames.begin();
            NodeBoneInfluence influence(boneName);
            nodeBoneInfluences[nodeId] = influence;
            dust3dDebug << "Node" << nodeIdString.c_str() << "bound to single bone:" << boneName.c_str();
        } else if (boneNames.size() == 2) {
            // Two bone influences - interpolate
            auto it = boneNames.begin();
            std::string bone1 = *it;
            std::string bone2 = *(++it);

            // For now, use equal weight (0.5 lerp)
            // Could be refined based on edge positions or weights
            float lerp = 0.5f;
            NodeBoneInfluence influence(bone1, bone2, lerp);
            nodeBoneInfluences[nodeId] = influence;
            dust3dDebug << "Node" << nodeIdString.c_str()
                        << "bound to two bones:" << bone1.c_str() << "+" << bone2.c_str()
                        << "lerp:" << lerp;
        } else {
            // More than 2 bones: pick the two most prominent
            // For now, pick first two alphabetically
            auto it = boneNames.begin();
            std::string bone1 = *it;
            std::string bone2 = *(++it);
            float lerp = 0.5f;
            NodeBoneInfluence influence(bone1, bone2, lerp);
            nodeBoneInfluences[nodeId] = influence;
            dust3dDebug << "Node" << nodeIdString.c_str()
                        << "has" << boneNames.size() << "different bones, using:" << bone1.c_str() << bone2.c_str();
        }
    }

    m_errorMessage = "";
    return true;
}

bool RigGenerator::computeVertexBoneBindings(Object* object,
    const std::map<Uuid, NodeBoneInfluence>& nodeBoneInfluences)
{
    if (!object) {
        m_errorMessage = "Object not initialized";
        return false;
    }

    if (object->vertices.empty()) {
        m_errorMessage = "Object has no vertices";
        return false;
    }

    // Initialize vertex bone arrays parallel to vertices
    object->vertexBone1.resize(object->vertices.size());
    object->vertexBone2.resize(object->vertices.size());

    // For each vertex, trace back to its source node and apply bone influence
    for (size_t i = 0; i < object->vertices.size(); ++i) {
        const Vector3& vertexPos = object->vertices[i];
        PositionKey posKey(vertexPos);

        // Find source node for this vertex
        auto it = object->positionToNodeIdMap.find(posKey);
        if (it == object->positionToNodeIdMap.end()) {
            // Vertex has no source node mapping
            continue;
        }

        Uuid nodeId = it->second;

        // Look up bone influence for this node
        auto boneIt = nodeBoneInfluences.find(nodeId);
        if (boneIt == nodeBoneInfluences.end()) {
            // Node has no bone influence
            continue;
        }

        const NodeBoneInfluence& influence = boneIt->second;
        VertexBoneBinding binding = influence.toVertexBinding();

        // Store binding in parallel arrays
        object->vertexBone1[i] = { binding.bone1, binding.weight1 };
        object->vertexBone2[i] = { binding.bone2, binding.weight2 };
    }

    m_errorMessage = "";
    return true;
}

void RigGenerator::attachSingleNodesToBone(const Snapshot* snapshot,
    const std::string& boneName,
    const std::set<Uuid>& boneEdgeNodes,
    const std::set<Uuid>& allEdgeNodes,
    std::vector<std::vector<Uuid>>& nodeChains)
{
    // Collect all nodes in the snapshot
    for (const auto& nodePair : snapshot->nodes) {
        Uuid nodeId(nodePair.first);

        if (!nodeBelongsToModelPart(snapshot, nodeId))
            continue;

        // Skip nodes that already participate in any bone edge
        if (allEdgeNodes.count(nodeId))
            continue;

        // This is an isolated node (no edge with any bone name).
        // Find the nearest node among this bone's edge-connected nodes.
        float nx = 0, ny = 0, nz = 0;
        if (!getNodePosition(snapshot, nodeId, nx, ny, nz))
            continue;

        float bestDist = std::numeric_limits<float>::max();
        bool foundNearest = false;
        for (const auto& candidateId : boneEdgeNodes) {
            float cx = 0, cy = 0, cz = 0;
            if (!getNodePosition(snapshot, candidateId, cx, cy, cz))
                continue;
            float dx = nx - cx, dy = ny - cy, dz = nz - cz;
            float dist = dx * dx + dy * dy + dz * dz;
            if (dist < bestDist) {
                bestDist = dist;
                foundNearest = true;
            }
        }

        if (foundNearest) {
            // Also verify this bone is truly the nearest bone overall:
            // compare bestDist against nearest node in allEdgeNodes minus boneEdgeNodes.
            bool nearerBoneExists = false;
            for (const auto& candidateId : allEdgeNodes) {
                if (boneEdgeNodes.count(candidateId))
                    continue;
                float cx = 0, cy = 0, cz = 0;
                if (!getNodePosition(snapshot, candidateId, cx, cy, cz))
                    continue;
                float dx = nx - cx, dy = ny - cy, dz = nz - cz;
                float dist = dx * dx + dy * dy + dz * dz;
                if (dist < bestDist) {
                    nearerBoneExists = true;
                    break;
                }
            }
            if (!nearerBoneExists) {
                dust3dDebug << "Single node" << nodeId.toString().c_str()
                            << "attached to bone" << boneName.c_str();
                nodeChains.push_back({ nodeId });
            }
        }
    }
}

bool RigGenerator::extractNodeChainsForBone(const Snapshot* snapshot,
    const std::string& boneName,
    std::vector<std::vector<Uuid>>& nodeChains)
{
    nodeChains.clear();

    std::map<Uuid, std::vector<Uuid>> adjacency;
    std::set<Uuid> allNodes;
    buildNodeAdjacency(snapshot, boneName, adjacency, allNodes);

    if (allNodes.empty()) {
        return false;
    }

    // Find all connected components and traverse each as a chain
    std::set<Uuid> globalVisited;

    for (const auto& seedNode : allNodes) {
        if (globalVisited.count(seedNode))
            continue;

        // Discover connected component via DFS
        std::set<Uuid> componentNodes;
        std::vector<Uuid> stack;
        stack.push_back(seedNode);
        while (!stack.empty()) {
            Uuid n = stack.back();
            stack.pop_back();
            if (componentNodes.count(n))
                continue;
            componentNodes.insert(n);
            for (const auto& neighbor : adjacency[n]) {
                if (!componentNodes.count(neighbor))
                    stack.push_back(neighbor);
            }
        }

        // Find a degree-1 node in this component to start the linear walk
        Uuid startNode;
        for (const auto& n : componentNodes) {
            if (adjacency[n].size() == 1) {
                startNode = n;
                break;
            }
        }
        if (startNode.isNull())
            startNode = *componentNodes.begin();

        // Traverse chain linearly
        std::vector<Uuid> chain;
        Uuid current = startNode;
        Uuid prev;

        while (!current.isNull() && !globalVisited.count(current)) {
            chain.push_back(current);
            globalVisited.insert(current);

            Uuid next;
            const auto& neighbors = adjacency[current];
            for (const auto& neighbor : neighbors) {
                if (neighbor != prev) {
                    next = neighbor;
                    break;
                }
            }
            prev = current;
            current = next;
        }

        if (!chain.empty())
            nodeChains.push_back(chain);
    }

    return !nodeChains.empty();
}

void RigGenerator::orientChainTowardPoint(const Snapshot* snapshot,
    std::vector<Uuid>& chain,
    float refX, float refY, float refZ)
{
    if (chain.size() < 2)
        return;

    float frontX, frontY, frontZ;
    float backX, backY, backZ;

    if (!getNodePosition(snapshot, chain.front(), frontX, frontY, frontZ))
        return;
    if (!getNodePosition(snapshot, chain.back(), backX, backY, backZ))
        return;

    float distFront = (frontX - refX) * (frontX - refX)
        + (frontY - refY) * (frontY - refY)
        + (frontZ - refZ) * (frontZ - refZ);
    float distBack = (backX - refX) * (backX - refX)
        + (backY - refY) * (backY - refY)
        + (backZ - refZ) * (backZ - refZ);

    if (distBack < distFront) {
        std::reverse(chain.begin(), chain.end());
    }
}

bool RigGenerator::getNodePositionInternal(const Snapshot* snapshot, const Uuid& nodeId,
    float& x, float& y, float& z, std::set<std::string>& visited)
{
    if (!snapshot)
        return false;

    std::string nodeIdString = nodeId.toString();
    if (visited.count(nodeIdString))
        return false; // cycle detected

    visited.insert(nodeIdString);

    auto it = snapshot->nodes.find(nodeIdString);
    if (it == snapshot->nodes.end())
        return false;

    std::string mirrorFromNodeId = String::valueOrEmpty(it->second, "__mirrorFromNodeId");
    if (!mirrorFromNodeId.empty()) {
        Uuid mirrorNode(mirrorFromNodeId);
        float mx = 0, my = 0, mz = 0;
        if (getNodePositionInternal(snapshot, mirrorNode, mx, my, mz, visited)) {
            x = -mx;
            y = my;
            z = mz;
            return true;
        }

        // Fall back to this node when mirror source fails
    }

    // Apply same coordinate transformation as MeshGenerator uses
    x = (String::toFloat(String::valueOrEmpty(it->second, "x")) - m_mainProfileMiddleX);
    y = (m_mainProfileMiddleY - String::toFloat(String::valueOrEmpty(it->second, "y")));
    z = (m_sideProfileMiddleX - String::toFloat(String::valueOrEmpty(it->second, "z")));
    return true;
}

bool RigGenerator::getNodePosition(const Snapshot* snapshot, const Uuid& nodeId,
    float& x, float& y, float& z)
{
    std::set<std::string> visited;
    return getNodePositionInternal(snapshot, nodeId, x, y, z, visited);
}

void RigGenerator::buildNodeAdjacency(const Snapshot* snapshot,
    const std::string& boneName,
    std::map<Uuid, std::vector<Uuid>>& adjacency,
    std::set<Uuid>& allNodes)
{
    adjacency.clear();
    allNodes.clear();

    auto edgesForBone = getEdgesWithBoneName(snapshot, boneName);

    for (const auto* edge : edgesForBone) {
        if (!edge)
            continue;

        std::string fromNodeIdString = String::valueOrEmpty(*edge, "from");
        std::string toNodeIdString = String::valueOrEmpty(*edge, "to");

        if (fromNodeIdString.empty() || toNodeIdString.empty())
            continue;

        Uuid n1(fromNodeIdString);
        Uuid n2(toNodeIdString);

        adjacency[n1].push_back(n2);
        adjacency[n2].push_back(n1);

        allNodes.insert(n1);
        allNodes.insert(n2);
    }
}

std::vector<const std::map<std::string, std::string>*> RigGenerator::getEdgesWithBoneName(
    const Snapshot* snapshot,
    const std::string& boneName)
{
    std::vector<const std::map<std::string, std::string>*> result;

    for (const auto& edgePair : snapshot->edges) {
        const auto& edgeAttributes = edgePair.second;
        std::string edgeBoneName = String::valueOrEmpty(edgeAttributes, "boneName");
        if (edgeBoneName == boneName && edgeBelongsToModelPart(snapshot, edgeAttributes)) {
            result.push_back(&edgeAttributes);
        }
    }

    return result;
}

bool RigGenerator::nodeHasEdgeWithBoneName(const Snapshot* snapshot,
    const Uuid& nodeId,
    const std::string& boneName)
{
    std::string nodeIdString = nodeId.toString();

    for (const auto& edgePair : snapshot->edges) {
        const auto& edgeAttributes = edgePair.second;
        std::string fromNode = String::valueOrEmpty(edgeAttributes, "from");
        std::string toNode = String::valueOrEmpty(edgeAttributes, "to");
        std::string edgeBoneName = String::valueOrEmpty(edgeAttributes, "boneName");

        if ((fromNode == nodeIdString || toNode == nodeIdString) && edgeBoneName == boneName) {
            return true;
        }
    }

    return false;
}

std::string RigGenerator::getEdgeBoneName(const std::map<std::string, std::string>* edge)
{
    if (!edge)
        return "";
    return String::valueOrEmpty(*edge, "boneName");
}

bool RigGenerator::applyRigBindings(Object* object, const Snapshot* snapshot)
{
    if (!object || !snapshot) {
        m_errorMessage = "Object or snapshot not initialized";
        return false;
    }

    // Use RigGenerator to compute bone influences and bind vertices
    std::map<Uuid, NodeBoneInfluence> nodeBoneInfluences;

    if (!computeNodeBoneInfluences(snapshot, nodeBoneInfluences)) {
        dust3dDebug << "Failed to compute node bone influences:" << getErrorMessage().c_str();
        return false;
    }

    if (!computeVertexBoneBindings(object, nodeBoneInfluences)) {
        dust3dDebug << "Failed to compute vertex bone bindings:" << getErrorMessage().c_str();
        return false;
    }

    dust3dDebug << "Applied rig bindings to" << object->vertices.size() << "vertices";
    return true;
}

}
