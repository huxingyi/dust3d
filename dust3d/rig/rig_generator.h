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

#ifndef DUST3D_RIG_RIG_GENERATOR_H_
#define DUST3D_RIG_RIG_GENERATOR_H_

#include <dust3d/base/bone_binding.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/object.h>
#include <dust3d/base/snapshot.h>
#include <dust3d/base/uuid.h>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace dust3d {

// RigStructure: Defines a skeletal rig with bones and hierarchy
struct RigNode {
    std::string name;
    std::string parent;
    float posX = 0.0f, posY = 0.0f, posZ = 0.0f;
    float endX = 0.0f, endY = 0.0f, endZ = 0.0f;
    float capsuleRadius = 0.0f;
};

struct RigStructure {
    std::string type;
    std::string description;
    std::vector<RigNode> bones;
};

// RigGenerator: Computes actual bone positions from edge assignments
// and calculates node-to-bone influence mappings.
//
// Workflow:
// 1. Edges in Snapshot have "boneName" property
// 2. RigGenerator extracts node chains from these edge assignments
// 3. Computes actual bone positions based on node locations
// 4. Computes node-to-bone influences (which bones affect each node)
// 5. During mesh generation, this data is used to bind mesh vertices to bones
class RigGenerator {
public:
    RigGenerator();
    ~RigGenerator();

    // Generate rig: compute actual bone positions from edge assignments in snapshot
    // input: snapshot - contains nodes, edges, and edges may have "boneName" property
    // input: templateRig - the rig template structure (skeleton hierarchy)
    // output: actualRig - updated with computed bone positions based on snapshot edges
    bool generateRig(const Snapshot* snapshot, const RigStructure& templateRig, RigStructure& actualRig);

    // Compute node-to-bone influences from snapshot edges
    // output: nodeBoneInfluences - maps node UUID -> bone influence
    // rigStructure is used to determine parent-child relationships for lerp weights
    bool computeNodeBoneInfluences(const Snapshot* snapshot,
        const RigStructure& rigStructure,
        std::map<Uuid, NodeBoneInfluence>& nodeBoneInfluences);

    // Compute vertex bone bindings for a generated mesh object
    // Uses the mesh's positionToNodeIdMap to trace vertices back to nodes,
    // then applies node bone influences to create vertex bindings
    bool computeVertexBoneBindings(Object* object,
        const std::map<Uuid, NodeBoneInfluence>& nodeBoneInfluences);

    // Apply rig bindings to the generated mesh using the snapshot's edge bone assignments
    // This should be called after generate() to apply skeletal rig weights to vertices
    // If actualRig is provided and has foot bones, the model is grounded so feet touch Y=0
    bool applyRigBindings(Object* object, const Snapshot* snapshot, RigStructure* actualRig = nullptr);

    // Compute world transforms for each bone in rest pose
    // If a child bone begin position is different from parent end, it still uses its own rest position.
    bool computeBoneWorldTransforms(const RigStructure& rigStructure,
        std::map<std::string, Matrix4x4>& boneWorldTransforms);

    // Compute inverse bind matrices for each bone in rest pose
    bool computeBoneInverseBindMatrices(const RigStructure& rigStructure,
        std::map<std::string, Matrix4x4>& inverseBindMatrices);

    // Get error message from last operation
    const std::string& getErrorMessage() const { return m_errorMessage; }

private:
    std::string m_errorMessage;

    // Coordinate transformation offsets (from snapshot canvas)
    float m_mainProfileMiddleX = 0.0f;
    float m_mainProfileMiddleY = 0.0f;
    float m_sideProfileMiddleX = 0.0f;

    // Helper: Extract all connected chains of nodes for a given bone name
    // Each chain is an ordered list of node UUIDs.
    // Multiple disconnected groups of edges produce multiple chains.
    bool extractNodeChainsForBone(const Snapshot* snapshot,
        const std::string& boneName,
        std::vector<std::vector<Uuid>>& nodeChains);

    // Helper: Build node connectivity graph from edges with a given bone name
    void buildNodeAdjacency(const Snapshot* snapshot,
        const std::string& boneName,
        std::map<Uuid, std::vector<Uuid>>& adjacency,
        std::set<Uuid>& allNodes);

    // Helper: Orient a chain so its end closest to refPoint comes first
    void orientChainTowardPoint(const Snapshot* snapshot,
        std::vector<Uuid>& chain,
        float refX, float refY, float refZ);

    // Helper: Get position of a single node from the snapshot
    bool getNodePosition(const Snapshot* snapshot, const Uuid& nodeId,
        float& x, float& y, float& z);

    // Internal helper that tracks visited node ids to prevent mirror loops
    bool getNodePositionInternal(const Snapshot* snapshot, const Uuid& nodeId,
        float& x, float& y, float& z, std::set<std::string>& visited);

    // Helper: Get all edges with a specific bone name
    std::vector<const std::map<std::string, std::string>*> getEdgesWithBoneName(
        const Snapshot* snapshot,
        const std::string& boneName);

    // Helper: Check if node has edge with given boneName
    bool nodeHasEdgeWithBoneName(const Snapshot* snapshot,
        const Uuid& nodeId,
        const std::string& boneName);

    // Helper: Get bone name from edge (or empty string if not assigned)
    std::string getEdgeBoneName(const std::map<std::string, std::string>* edge);

    // Helper: Compute lerp weight for a node influenced by two bones,
    // using rig hierarchy and bone semantics
    float computeTwoBoneLerp(const RigStructure& rigStructure,
        const std::string& bone1, const std::string& bone2,
        const Snapshot* snapshot, const Uuid& nodeId);

    // Helper: Find truly isolated nodes (no edges at all) that are nearest
    // to the given bone's edge-connected nodes, and append them as single-node chains.
    // Also records the mapping in m_singleNodeBoneMap for use by computeNodeBoneInfluences.
    void attachSingleNodesToBone(const Snapshot* snapshot,
        const std::string& boneName,
        const std::set<Uuid>& boneEdgeNodes,
        const std::set<Uuid>& allEdgeNodes,
        const std::set<Uuid>& nodesWithAnyEdge,
        std::vector<std::vector<Uuid>>& nodeChains);

    // Map from isolated node -> bone name, populated by attachSingleNodesToBone
    std::map<Uuid, std::string> m_singleNodeBoneMap;
};

}

#endif
