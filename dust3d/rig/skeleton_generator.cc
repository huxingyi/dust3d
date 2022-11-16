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

#include <dust3d/rig/skeleton_generator.h>

namespace dust3d {

SkeletonGenerator::SkeletonGenerator()
{
}

void SkeletonGenerator::setVertices(const std::vector<Vector3>& vertices)
{
    m_vertices = vertices;
}

void SkeletonGenerator::setFaces(const std::vector<std::vector<size_t>>& faces)
{
    m_faces = faces;
}

void SkeletonGenerator::setPositionToNodeMap(const std::map<PositionKey, Uuid>& positionToNodeMap)
{
    m_positionToNodeMap = positionToNodeMap;
}

void SkeletonGenerator::addBone(const Uuid& boneId, const Bone& bone)
{
    m_boneMap.emplace(std::make_pair(boneId, bone));
}

void SkeletonGenerator::addNodeBinding(const Uuid& nodeId, const NodeBinding& nodeBinding)
{
    m_nodeBindingMap.emplace(std::make_pair(nodeId, nodeBinding));
}

void SkeletonGenerator::buildEdges()
{
    for (const auto& face : m_faces) {
        for (size_t i = 0; i < face.size(); ++i) {
            size_t j = (i + 1) % face.size();
            m_edges[face[i]].insert(face[j]);
            m_edges[face[j]].insert(face[i]);
        }
    }
}

Uuid SkeletonGenerator::resolveVertexSourceByBreadthFirstSearch(size_t vertexIndex, std::unordered_set<size_t>& visited)
{
    visited.insert(vertexIndex);
    auto findNeighbors = m_edges.find(vertexIndex);
    if (findNeighbors == m_edges.end())
        return Uuid();
    for (const auto& it : findNeighbors->second) {
        if (!m_vertexSourceNodes[it].isNull())
            return m_vertexSourceNodes[it];
    }
    for (const auto& it : findNeighbors->second) {
        if (visited.end() != visited.find(it))
            continue;
        Uuid foundId = resolveVertexSourceByBreadthFirstSearch(it, visited);
        if (!foundId.isNull())
            return foundId;
    }
    return Uuid();
}

void SkeletonGenerator::resolveVertexSources()
{
    m_vertexSourceNodes.resize(m_vertices.size());
    for (size_t i = 0; i < m_vertices.size(); ++i) {
        auto findNode = m_positionToNodeMap.find(m_vertices[i]);
        if (findNode == m_positionToNodeMap.end())
            continue;
        m_vertexSourceNodes[i] = findNode->second;
    }

    for (size_t i = 0; i < m_vertexSourceNodes.size(); ++i) {
        if (!m_vertexSourceNodes[i].isNull())
            continue;
        std::unordered_set<size_t> visited;
        m_vertexSourceNodes[i] = resolveVertexSourceByBreadthFirstSearch(i, visited);
    }
}

void SkeletonGenerator::buildBoneJoints()
{
    // TODO:
}

void SkeletonGenerator::assignBoneJointToVertices()
{
    // TODO:
}

void SkeletonGenerator::groupBoneVertices()
{
    /*
    for (size_t i = 0; i < m_vertexSourceNodes.size(); ++i) {
        const Uuid& sourceNodeId = m_vertexSourceNodes[i];
        if (sourceNodeId.isNull())
            continue;
        auto findBinding = m_nodeBindingMap.find(sourceNodeId);
        if (findBinding == m_nodeBindingMap.end())
            continue;
        // TODO:
    }
    */
    // TODO:
}

void SkeletonGenerator::bind()
{
    buildEdges();
    resolveVertexSources();
    groupBoneVertices();
    buildBoneJoints();
    assignBoneJointToVertices();
}

}
