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
#include <dust3d/base/string.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

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
            continue;
        }

        // Determine reference point from parent bone's tip position.
        // Root bones (no parent) use the origin.
        float refX = 0, refY = 0, refZ = 0;
        if (!bone.parent.empty()) {
            auto parentIt = boneNameToIndex.find(bone.parent);
            if (parentIt != boneNameToIndex.end()) {
                const auto& parentBone = actualRig.bones[parentIt->second];
                refX = parentBone.endX;
                refY = parentBone.endY;
                refZ = parentBone.endZ;
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

        // Bone root = first node of closest chain (nearest to parent tip)
        // Bone tip  = last node of farthest chain
        if (getNodePosition(snapshot, nodeChains.front().front(),
                bone.posX, bone.posY, bone.posZ)) {
            // set successfully
        }
        if (getNodePosition(snapshot, nodeChains.back().back(),
                bone.endX, bone.endY, bone.endZ)) {
            // set successfully
        }

        size_t totalNodes = 0;
        for (const auto& chain : nodeChains)
            totalNodes += chain.size();

        dust3dDebug << "Computed bone" << bone.name.c_str()
                    << "from" << totalNodes << "nodes in" << nodeChains.size() << "chains"
                    << "position:" << bone.posX << bone.posY << bone.posZ
                    << "endPosition:" << bone.endX << bone.endY << bone.endZ;
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

        // Find all edges connected to this node
        std::set<std::string> boneNames;
        for (const auto& edgePair : snapshot->edges) {
            const std::map<std::string, std::string>& edgeAttributes = edgePair.second;
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
            // Node has no assigned edges
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

bool RigGenerator::getNodePosition(const Snapshot* snapshot, const Uuid& nodeId,
    float& x, float& y, float& z)
{
    auto it = snapshot->nodes.find(nodeId.toString());
    if (it == snapshot->nodes.end())
        return false;

    x = String::toFloat(String::valueOrEmpty(it->second, "x"));
    y = String::toFloat(String::valueOrEmpty(it->second, "y"));
    z = String::toFloat(String::valueOrEmpty(it->second, "z"));
    return true;
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
        if (edgeBoneName == boneName) {
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
