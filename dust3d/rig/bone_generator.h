/*
 *  Copyright (c) 2016-2022 Jeremy HU <jeremy-at-dust3d dot org>. All rights reserved. 
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

#ifndef DUST3D_RIG_BONE_GENERATOR_H_
#define DUST3D_RIG_BONE_GENERATOR_H_

#include <array>
#include <dust3d/base/color.h>
#include <dust3d/base/position_key.h>
#include <dust3d/base/uuid.h>
#include <dust3d/base/vector3.h>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace dust3d {

class BoneGenerator {
public:
    struct NodeBinding {
        std::set<Uuid> boneIds;
    };

    struct VertexWeight {
        size_t vertex;
        double weight;
    };

    struct Bone {
        std::string name;
        std::vector<Uuid> joints;
        std::vector<Vector3> startPositions;
        std::vector<Vector3> forwardVectors;
        std::vector<VertexWeight> vertexWeights;
    };

    struct Node {
        Vector3 position;
    };

    struct BonePreview {
        std::vector<Vector3> vertices;
        std::vector<std::vector<size_t>> triangles;
    };

    BoneGenerator();
    void generate();
    std::map<Uuid, BonePreview>& bonePreviews();

protected:
    void setVertices(const std::vector<Vector3>& vertices);
    void setTriangles(const std::vector<std::vector<size_t>>& triangles);
    void setPositionToNodeMap(const std::map<PositionKey, Uuid>& positionToNodeMap);
    void addBone(const Uuid& boneId, const Bone& bone);
    void addNodeBinding(const Uuid& nodeId, const NodeBinding& nodeBidning);
    void addNode(const Uuid& nodeId, const Node& node);

private:
    std::vector<Vector3> m_vertices;
    std::vector<std::vector<size_t>> m_triangles;
    std::map<PositionKey, Uuid> m_positionToNodeMap;
    std::map<Uuid, NodeBinding> m_nodeBindingMap;
    std::map<Uuid, Bone> m_boneMap;
    std::map<Uuid, Node> m_nodeMap;
    std::map<size_t, std::set<size_t>> m_edges;
    std::vector<Uuid> m_vertexSourceNodes;
    std::map<Uuid, std::unordered_set<size_t>> m_boneVertices;
    std::map<Uuid, BonePreview> m_bonePreviews;

    void buildEdges();
    void resolveVertexSources();
    Uuid resolveVertexSourceByBreadthFirstSearch(size_t vertexIndex, std::unordered_set<size_t>& visited);
    void groupBoneVertices();
    void buildBoneJoints();
    void assignVerticesToBoneJoints();
    void generateBonePreviews();
    void addBonePreviewTriangle(BonePreview& bonePreview,
        std::unordered_map<size_t, size_t>& oldToNewVertexMap,
        const std::vector<size_t>& triangle);
};

}

#endif
