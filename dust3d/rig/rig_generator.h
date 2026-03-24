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
};

struct RigStructure {
    std::string type;
    std::string name;
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
    bool computeNodeBoneInfluences(const Snapshot* snapshot,
                                   std::map<Uuid, NodeBoneInfluence>& nodeBoneInfluences);

    // Compute vertex bone bindings for a generated mesh object
    // Uses the mesh's positionToNodeIdMap to trace vertices back to nodes,
    // then applies node bone influences to create vertex bindings
    bool computeVertexBoneBindings(Object* object,
                                   const std::map<Uuid, NodeBoneInfluence>& nodeBoneInfluences);

    // Apply rig bindings to the generated mesh using the snapshot's edge bone assignments
    // This should be called after generate() to apply skeletal rig weights to vertices
    bool applyRigBindings(Object* object, const Snapshot* snapshot);

    // Get error message from last operation
    const std::string& getErrorMessage() const { return m_errorMessage; }

private:
    std::string m_errorMessage;

    // Helper: Extract linear chain of nodes for a given bone name
    // Returns node UUIDs in chain order (root to tip)
    bool extractNodeChainForBone(const Snapshot* snapshot,
                                 const std::string& boneName,
                                 std::vector<Uuid>& nodeChain);

    // Helper: Build node connectivity graph from edges with a given bone name
    // Returns adjacency structure: node UUID -> connected node UUIDs
    void buildNodeAdjacency(const Snapshot* snapshot,
                           const std::string& boneName,
                           std::map<Uuid, std::vector<Uuid>>& adjacency,
                           std::set<Uuid>& allNodes);

    // Helper: Traverse chain to find nodes with degree 1 (endpoints)
    // Returns two lists: root nodes and tip nodes
    void findChainEndpoints(const std::vector<Uuid>& nodeChain,
                           const std::map<Uuid, std::vector<Uuid>>& adjacency,
                           std::vector<Uuid>& rootNodes,
                           std::vector<Uuid>& tipNodes);

    // Helper: Average positions of given nodes
    bool averageNodePositions(const Snapshot* snapshot,
                             const std::vector<Uuid>& nodeUuids,
                             float& outX, float& outY, float& outZ);

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
};

}

#endif
