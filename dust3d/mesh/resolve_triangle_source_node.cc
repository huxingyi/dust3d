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

#include <map>
#include <dust3d/base/position_key.h>
#include <dust3d/mesh/resolve_triangle_source_node.h>

namespace dust3d
{

struct HalfColorEdge
{
    int cornVertexIndex;
    std::pair<Uuid, Uuid> source;
};

struct CandidateEdge
{
    std::pair<Uuid, Uuid> source;
    int fromVertexIndex;
    int toVertexIndex;
    float dot;
    float length;
};

static void fixRemainVertexSourceNodes(const Object &object, std::vector<std::pair<Uuid, Uuid>> &triangleSourceNodes,
    std::vector<std::pair<Uuid, Uuid>> *vertexSourceNodes)
{
    if (nullptr != vertexSourceNodes) {
        std::map<size_t, std::map<std::pair<Uuid, Uuid>, size_t>> remainVertexSourcesMap;
        for (size_t faceIndex = 0; faceIndex < object.triangles.size(); ++faceIndex) {
            for (const auto &vertexIndex: object.triangles[faceIndex]) {
                if (!(*vertexSourceNodes)[vertexIndex].second.isNull())
                    continue;
                remainVertexSourcesMap[vertexIndex][triangleSourceNodes[faceIndex]]++;
            }
        }
        for (const auto &it: remainVertexSourcesMap) {
            (*vertexSourceNodes)[it.first] = std::max_element(it.second.begin(), it.second.end(), [](
                    const std::map<std::pair<Uuid, Uuid>, size_t>::value_type &first,
                    const std::map<std::pair<Uuid, Uuid>, size_t>::value_type &second) {
                return first.second < second.second;
            })->first;
        }
    }
}

void resolveTriangleSourceNode(const Object &object, 
    const std::vector<std::pair<Vector3, std::pair<Uuid, Uuid>>> &nodeVertices,
    std::vector<std::pair<Uuid, Uuid>> &triangleSourceNodes,
    std::vector<std::pair<Uuid, Uuid>> *vertexSourceNodes)
{
    std::map<int, std::pair<Uuid, Uuid>> vertexSourceMap;
    std::map<PositionKey, std::pair<Uuid, Uuid>> positionMap;
    std::map<std::pair<int, int>, HalfColorEdge> halfColorEdgeMap;
    std::set<int> brokenTriangleSet;
    for (const auto &it: nodeVertices) {
        positionMap.insert({PositionKey(it.first), it.second});
    }
    if (nullptr != vertexSourceNodes)
        vertexSourceNodes->resize(object.vertices.size());
    for (auto x = 0u; x < object.vertices.size(); x++) {
        const Vector3 *resultVertex = &object.vertices[x];
        std::pair<Uuid, Uuid> source;
        auto findPosition = positionMap.find(PositionKey(*resultVertex));
        if (findPosition != positionMap.end()) {
            (*vertexSourceNodes)[x] = findPosition->second;
            vertexSourceMap[x] = findPosition->second;
        }
    }
    for (auto x = 0u; x < object.triangles.size(); x++) {
        const auto triangle = object.triangles[x];
        std::vector<std::pair<std::pair<Uuid, Uuid>, int>> colorTypes;
        for (int i = 0; i < 3; i++) {
            int index = triangle[i];
            const auto &findResult = vertexSourceMap.find(index);
            if (findResult != vertexSourceMap.end()) {
                std::pair<Uuid, Uuid> source = findResult->second;
                bool colorExisted = false;
                for (auto j = 0u; j < colorTypes.size(); j++) {
                    if (colorTypes[j].first == source) {
                        colorTypes[j].second++;
                        colorExisted = true;
                        break;
                    }
                }
                if (!colorExisted) {
                    colorTypes.push_back(std::make_pair(source, 1));
                }
            }
        }
        if (colorTypes.empty()) {
            triangleSourceNodes.push_back(std::make_pair(Uuid(), Uuid()));
            brokenTriangleSet.insert(x);
            continue;
        }
        if (colorTypes.size() != 1 || 3 == colorTypes[0].second) {
            std::sort(colorTypes.begin(), colorTypes.end(), [](const std::pair<std::pair<Uuid, Uuid>, int> &a, const std::pair<std::pair<Uuid, Uuid>, int> &b) -> bool {
                return a.second > b.second;
            });
        }
        std::pair<Uuid, Uuid> choosenColor = colorTypes[0].first;
        triangleSourceNodes.push_back(choosenColor);
        for (int i = 0; i < 3; i++) {
            int oppositeStartIndex = triangle[(i + 1) % 3];
            int oppositeStopIndex = triangle[i];
            auto oppositePair = std::make_pair(oppositeStartIndex, oppositeStopIndex);
            if (halfColorEdgeMap.find(oppositePair) != halfColorEdgeMap.end()) {
                halfColorEdgeMap.erase(oppositePair);
                continue;
            }
            auto selfPair = std::make_pair(oppositeStopIndex, oppositeStartIndex);
            HalfColorEdge edge;
            edge.cornVertexIndex = triangle[(i + 2) % 3];
            edge.source = choosenColor;
            halfColorEdgeMap[selfPair] = edge;
        }
    }
    std::map<std::pair<int, int>, int> brokenTriangleMapByEdge;
    std::vector<CandidateEdge> candidateEdges;
    for (const auto &x: brokenTriangleSet) {
        const auto triangle = object.triangles[x];
        for (int i = 0; i < 3; i++) {
            int oppositeStartIndex = triangle[(i + 1) % 3];
            int oppositeStopIndex = triangle[i];
            auto selfPair = std::make_pair(oppositeStopIndex, oppositeStartIndex);
            brokenTriangleMapByEdge[selfPair] = x;
            auto oppositePair = std::make_pair(oppositeStartIndex, oppositeStopIndex);
            const auto &findOpposite = halfColorEdgeMap.find(oppositePair);
            if (findOpposite == halfColorEdgeMap.end())
                continue;
            Vector3 selfPositions[3] = {
                object.vertices[triangle[i]], // A
                object.vertices[triangle[(i + 1) % 3]], // B
                object.vertices[triangle[(i + 2) % 3]] // C
            };
            Vector3 oppositeCornPosition = object.vertices[findOpposite->second.cornVertexIndex]; // D
            Vector3 AB = selfPositions[1] - selfPositions[0];
            float length = AB.length();
            Vector3 AC = selfPositions[2] - selfPositions[0];
            Vector3 AD = oppositeCornPosition - selfPositions[0];
            AB.normalize();
            AC.normalize();
            AD.normalize();
            Vector3 ABxAC = Vector3::crossProduct(AB, AC);
            Vector3 ADxAB = Vector3::crossProduct(AD, AB);
            ABxAC.normalize();
            ADxAB.normalize();
            float dot = Vector3::dotProduct(ABxAC, ADxAB);
            CandidateEdge candidate;
            candidate.dot = dot;
            candidate.length = length;
            candidate.fromVertexIndex = triangle[i];
            candidate.toVertexIndex = triangle[(i + 1) % 3];
            candidate.source = findOpposite->second.source;
            candidateEdges.push_back(candidate);
        }
    }
    if (candidateEdges.empty()) {
        fixRemainVertexSourceNodes(object, triangleSourceNodes, vertexSourceNodes);
        return;
    }
    std::sort(candidateEdges.begin(), candidateEdges.end(), [](const CandidateEdge &a, const CandidateEdge &b) -> bool {
        if (a.dot > b.dot)
            return true;
        else if (a.dot < b.dot)
            return false;
        return a.length > b.length;
    });
    for (auto cand = 0u; cand < candidateEdges.size(); cand++) {
        const auto &candidate = candidateEdges[cand];
        if (brokenTriangleSet.empty())
            break;
        std::vector<std::pair<int, int>> toResolvePairs;
        toResolvePairs.push_back(std::make_pair(candidate.fromVertexIndex, candidate.toVertexIndex));
        for (auto order = 0u; order < toResolvePairs.size(); order++) {
            const auto &findTriangle = brokenTriangleMapByEdge.find(toResolvePairs[order]);
            if (findTriangle == brokenTriangleMapByEdge.end())
                continue;
            int x = findTriangle->second;
            if (brokenTriangleSet.find(x) == brokenTriangleSet.end())
                continue;
            brokenTriangleSet.erase(x);
            triangleSourceNodes[x] = candidate.source;
            const auto triangle = object.triangles[x];
            for (int i = 0; i < 3; i++) {
                int oppositeStartIndex = triangle[(i + 1) % 3];
                int oppositeStopIndex = triangle[i];
                auto oppositePair = std::make_pair(oppositeStartIndex, oppositeStopIndex);
                toResolvePairs.push_back(oppositePair);
            }
        }
    }
    fixRemainVertexSourceNodes(object, triangleSourceNodes, vertexSourceNodes);
}

}
