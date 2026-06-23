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

#include <algorithm>
#include <array>
#include <cmath>
#include <dust3d/base/position_key.h>
#include <dust3d/mesh/hole_wrapper.h>
#include <dust3d/mesh/mesh_recombiner.h>
#include <limits>
#include <queue>
#include <set>

namespace dust3d {

#define MAX_EDGE_LOOP_LENGTH 1000

void MeshRecombiner::setVertices(const std::vector<Vector3>* vertices,
    const std::vector<std::pair<MeshCombiner::Source, size_t>>* verticesSourceIndices)
{
    m_vertices = vertices;
    m_verticesSourceIndices = verticesSourceIndices;
}

void MeshRecombiner::setFaces(const std::vector<std::vector<size_t>>* faces)
{
    m_faces = faces;
}

bool MeshRecombiner::convertHalfEdgesToEdgeLoops(const std::vector<std::pair<size_t, size_t>>& halfEdges,
    std::vector<std::vector<size_t>>* edgeLoops)
{
    std::map<size_t, size_t> vertexLinkMap;
    for (const auto& halfEdge : halfEdges) {
        auto inserResult = vertexLinkMap.insert(halfEdge);
        if (!inserResult.second) {
            return false;
        }
    }
    while (!vertexLinkMap.empty()) {
        std::vector<size_t> edgeLoop;
        size_t vertex = vertexLinkMap.begin()->first;
        size_t head = vertex;
        bool loopBack = false;
        size_t limitLoop = MAX_EDGE_LOOP_LENGTH;
        while ((limitLoop--) > 0) {
            edgeLoop.push_back(vertex);
            auto findNext = vertexLinkMap.find(vertex);
            if (findNext == vertexLinkMap.end())
                break;
            vertex = findNext->second;
            if (vertex == head) {
                loopBack = true;
                break;
            }
        }
        if (!loopBack) {
            return false;
        }
        if (edgeLoop.size() < 3) {
            return false;
        }
        for (const auto& vertex : edgeLoop) {
            vertexLinkMap.erase(vertex);
        }
        bool collapsed = true;
        while (collapsed) {
            collapsed = false;
            std::map<PositionKey, size_t> positionToIndex;
            for (size_t i = 0; i < edgeLoop.size(); ++i) {
                PositionKey key((*m_vertices)[edgeLoop[i]]);
                auto it = positionToIndex.find(key);
                if (it != positionToIndex.end()) {
                    size_t firstIndex = it->second;
                    std::vector<size_t> newLoop;
                    for (size_t j = 0; j <= firstIndex; ++j)
                        newLoop.push_back(edgeLoop[j]);
                    for (size_t j = i + 1; j < edgeLoop.size(); ++j)
                        newLoop.push_back(edgeLoop[j]);
                    edgeLoop = newLoop;
                    collapsed = true;
                    break;
                }
                positionToIndex.insert({ key, i });
            }
        }
        if (edgeLoop.size() < 3)
            continue;
        edgeLoops->push_back(edgeLoop);
    }
    return true;
}

size_t MeshRecombiner::splitSeamVerticesToIslands(const std::map<size_t, std::vector<size_t>>& seamEdges,
    std::map<size_t, size_t>* vertexToIslandMap)
{
    std::set<size_t> visited;
    size_t nextIslandId = 0;
    for (const auto& it : seamEdges) {
        std::queue<size_t> vertices;
        vertices.push(it.first);
        bool hasVertexJoin = false;
        while (!vertices.empty()) {
            auto v = vertices.front();
            vertices.pop();
            if (visited.find(v) != visited.end())
                continue;
            visited.insert(v);
            vertexToIslandMap->insert({ v, nextIslandId });
            hasVertexJoin = true;
            const auto findNeighbors = seamEdges.find(v);
            if (findNeighbors != seamEdges.end()) {
                for (const auto& neighbor : findNeighbors->second) {
                    vertices.push(neighbor);
                }
            }
        }
        if (hasVertexJoin)
            ++nextIslandId;
    }
    return nextIslandId;
}

bool MeshRecombiner::buildHalfEdgeToFaceMap(std::map<std::pair<size_t, size_t>, size_t>& halfEdgeToFaceMap)
{
    bool isSuccessful = true;
    for (size_t faceIndex = 0; faceIndex < m_faces->size(); ++faceIndex) {
        const auto& face = (*m_faces)[faceIndex];
        for (size_t i = 0; i < face.size(); ++i) {
            size_t j = (i + 1) % face.size();
            const auto insertResult = halfEdgeToFaceMap.insert({ { face[i], face[j] }, faceIndex });
            if (!insertResult.second) {
                isSuccessful = false;
            }
        }
    }
    return isSuccessful;
}

bool MeshRecombiner::recombine()
{
    buildHalfEdgeToFaceMap(m_halfEdgeToFaceMap);

    std::map<size_t, std::vector<size_t>> seamLink;
    for (const auto& face : *m_faces) {
        for (size_t i = 0; i < face.size(); ++i) {
            const auto& index = face[i];
            auto source = (*m_verticesSourceIndices)[index];
            if (MeshCombiner::Source::None == source.first) {
                auto next = face[(i + 1) % face.size()];
                auto nextSource = (*m_verticesSourceIndices)[next];
                if (MeshCombiner::Source::None == nextSource.first) {
                    seamLink[index].push_back(next);
                }
            }
        }
    }
    std::map<size_t, size_t> seamVertexToIslandMap;
    splitSeamVerticesToIslands(seamLink, &seamVertexToIslandMap);

    std::map<std::pair<size_t, size_t>, std::pair<size_t, bool>> edgesInSeamArea;
    for (size_t faceIndex = 0; faceIndex < (*m_faces).size(); ++faceIndex) {
        const auto& face = (*m_faces)[faceIndex];
        bool containsSeamVertex = false;
        bool inFirstGroup = false;
        size_t island = 0;
        for (size_t i = 0; i < face.size(); ++i) {
            const auto& index = face[i];
            auto source = (*m_verticesSourceIndices)[index];
            if (MeshCombiner::Source::None == source.first) {
                const auto& findIsland = seamVertexToIslandMap.find(index);
                if (findIsland != seamVertexToIslandMap.end()) {
                    containsSeamVertex = true;
                    island = findIsland->second;
                }
            } else if (MeshCombiner::Source::First == source.first) {
                inFirstGroup = true;
            }
        }
        if (containsSeamVertex) {
            m_facesInSeamArea.insert({ faceIndex, island });
            for (size_t i = 0; i < face.size(); ++i) {
                const auto& index = face[i];
                const auto& next = face[(i + 1) % face.size()];
                std::pair<size_t, size_t> edge = { index, next };
                edgesInSeamArea.insert({ edge, { island, inFirstGroup } });
            }
        }
    }

    struct IslandData {
        std::vector<std::pair<size_t, size_t>> halfedges[2];
        std::vector<std::vector<size_t>> edgeLoops[2];
    };
    std::map<size_t, IslandData> islandsMap;

    for (const auto& edge : edgesInSeamArea) {
        if (edgesInSeamArea.find({ edge.first.second, edge.first.first }) == edgesInSeamArea.end()) {
            islandsMap[edge.second.first].halfedges[edge.second.second ? 0 : 1].push_back(edge.first);
        }
    }
    for (auto& it : islandsMap) {
        for (size_t side = 0; side < 2; ++side) {
            if (!convertHalfEdgesToEdgeLoops(it.second.halfedges[side], &it.second.edgeLoops[side])) {
                it.second.edgeLoops[side].clear();
            }
        }
    }

    for (auto& it : islandsMap) {
        for (size_t side = 0; side < 2; ++side) {
            for (size_t i = 0; i < it.second.edgeLoops[side].size(); ++i) {
                auto& edgeLoop = it.second.edgeLoops[side][i];
                size_t totalAdjustedTriangles = 0;
                size_t adjustedTriangles = 0;
                while ((adjustedTriangles = adjustTrianglesFromSeam(edgeLoop, it.first)) > 0) {
                    totalAdjustedTriangles += adjustedTriangles;
                }
            }
        }
    }

    for (auto& it : islandsMap) {
        if (1 == it.second.edgeLoops[0].size() && it.second.edgeLoops[0].size() == it.second.edgeLoops[1].size()) {
            if (bridge(it.second.edgeLoops[0][0], it.second.edgeLoops[1][0])) {
                m_goodSeams.insert(it.first);
            }
        }
    }

    copyNonSeamFacesAsRegenerated();
    removeReluctantVertices();

    return true;
}

const std::map<size_t, size_t>& MeshRecombiner::inputFacesInSeamArea() const
{
    return m_facesInSeamArea;
}

size_t MeshRecombiner::adjustTrianglesFromSeam(std::vector<size_t>& edgeLoop, size_t seamIndex)
{
    if (edgeLoop.size() <= 3)
        return 0;

    std::vector<size_t> halfEdgeToFaces;
    for (size_t i = 0; i < edgeLoop.size(); ++i) {
        size_t j = (i + 1) % edgeLoop.size();
        auto findFace = m_halfEdgeToFaceMap.find({ edgeLoop[j], edgeLoop[i] });
        if (findFace == m_halfEdgeToFaceMap.end()) {
            return 0;
        }
        halfEdgeToFaces.push_back(findFace->second);
    }

    std::vector<size_t> removedFaceIndices;
    std::set<size_t> ignored;
    for (size_t i = 0; i < edgeLoop.size(); ++i) {
        size_t j = (i + 1) % edgeLoop.size();
        if (halfEdgeToFaces[i] == halfEdgeToFaces[j]) {
            removedFaceIndices.push_back(halfEdgeToFaces[i]);
            ignored.insert(edgeLoop[j]);
            ++i;
            continue;
        }
    }

    if (!ignored.empty()) {
        std::vector<size_t> newEdgeLoop;
        for (const auto& v : edgeLoop) {
            if (ignored.find(v) != ignored.end())
                continue;
            newEdgeLoop.push_back(v);
        }
        if (newEdgeLoop.size() < 3)
            return 0;
        edgeLoop = newEdgeLoop;
        for (const auto& faceIndex : removedFaceIndices)
            m_facesInSeamArea.insert({ faceIndex, seamIndex });
    }

    return ignored.size();
}

size_t MeshRecombiner::otherVertexOfTriangle(const std::vector<size_t>& face, const std::vector<size_t>& indices)
{
    for (const auto& v : face) {
        bool found = false;
        for (const auto& u : indices) {
            if (u == v) {
                found = true;
                break;
            }
        }
        if (!found)
            return v;
    }
    return face[0];
}

void MeshRecombiner::copyNonSeamFacesAsRegenerated()
{
    for (size_t faceIndex = 0; faceIndex < m_faces->size(); ++faceIndex) {
        const auto& findFaceInSeam = m_facesInSeamArea.find(faceIndex);
        if (findFaceInSeam != m_facesInSeamArea.end() && m_goodSeams.find(findFaceInSeam->second) != m_goodSeams.end())
            continue;
        m_regeneratedFaces.push_back((*m_faces)[faceIndex]);
    }
}

const std::vector<Vector3>& MeshRecombiner::regeneratedVertices()
{
    return m_regeneratedVertices;
}

const std::vector<std::pair<MeshCombiner::Source, size_t>>& MeshRecombiner::regeneratedVerticesSourceIndices()
{
    return m_regeneratedVerticesSourceIndices;
}

const std::vector<std::vector<size_t>>& MeshRecombiner::regeneratedFaces()
{
    return m_regeneratedFaces;
}

size_t MeshRecombiner::nearestIndex(const Vector3& position, const std::vector<size_t>& edgeLoop)
{
    float minDist2 = std::numeric_limits<float>::max();
    size_t choosenIndex = 0;
    for (size_t i = 0; i < edgeLoop.size(); ++i) {
        float dist2 = ((*m_vertices)[edgeLoop[i]] - position).lengthSquared();
        // Use only strictly less than to ensure deterministic first-match behavior
        if (dist2 < minDist2) {
            minDist2 = dist2;
            choosenIndex = i;
        }
    }
    return choosenIndex;
}

bool MeshRecombiner::bridge(const std::vector<size_t>& first, const std::vector<size_t>& second)
{
    const std::vector<size_t>* large = &first;
    const std::vector<size_t>* small = &second;
    if (large->size() < small->size())
        std::swap(large, small);

    if (small->size() < 3)
        return false; // Need at least triangular loops

    std::vector<std::pair<size_t, size_t>> matchedPairs;
    std::map<size_t, size_t> nearestIndicesFromLargeToSmall;
    std::map<size_t, bool> largeVertexMatched; // Track which large vertices are matched

    // First pass: find best matches from small to large
    for (size_t i = 0; i < small->size(); ++i) {
        const auto& positionOnSmall = (*m_vertices)[(*small)[i]];
        size_t nearestIndexOnLarge = nearestIndex(positionOnSmall, *large);

        // Always record the bidirectional check, overwriting if we find a better one
        const auto& positionOnLarge = (*m_vertices)[(*large)[nearestIndexOnLarge]];
        size_t nearestIndexOnSmall = nearestIndex(positionOnLarge, *small);

        // Use this match if it's bidirectional or if it's the best we can find
        if (nearestIndexOnSmall == i) {
            // Perfect bidirectional match
            matchedPairs.push_back({ i, nearestIndexOnLarge });
            largeVertexMatched[nearestIndexOnLarge] = true;
        } else {
            // Store for fallback matching below
            nearestIndicesFromLargeToSmall[nearestIndexOnLarge] = i;
        }
    }

    // If we don't have enough bidirectional matches, use the best nearby matches
    // to ensure we always have valid pairs for bridging
    if (matchedPairs.size() < 3) {
        matchedPairs.clear();
        largeVertexMatched.clear();

        // Simpler matching: just find nearest from small to large
        std::map<size_t, size_t> smallToLargeNearest;
        for (size_t i = 0; i < small->size(); ++i) {
            const auto& positionOnSmall = (*m_vertices)[(*small)[i]];
            size_t nearestIndexOnLarge = nearestIndex(positionOnSmall, *large);
            smallToLargeNearest[i] = nearestIndexOnLarge;
        }

        // Remove duplicates: keep only one small vertex per large vertex
        std::map<size_t, size_t> largeToSmallBest;
        for (const auto& kv : smallToLargeNearest) {
            size_t smallIdx = kv.first;
            size_t largeIdx = kv.second;

            auto existingMatch = largeToSmallBest.find(largeIdx);
            if (existingMatch == largeToSmallBest.end()) {
                largeToSmallBest[largeIdx] = smallIdx;
            } else {
                // Keep the one with smaller index for determinism
                if (smallIdx < existingMatch->second) {
                    largeToSmallBest[largeIdx] = smallIdx;
                }
            }
        }

        for (const auto& kv : largeToSmallBest) {
            matchedPairs.push_back({ kv.second, kv.first });
        }
    }

    // Sort matched pairs by small index to ensure deterministic ordering
    std::sort(matchedPairs.begin(), matchedPairs.end(),
        [](const std::pair<size_t, size_t>& a, const std::pair<size_t, size_t>& b) {
            return a.first < b.first;
        });

    if (matchedPairs.empty())
        return false;

    for (size_t i = 0; i < matchedPairs.size(); ++i) {
        size_t j = (i + 1) % matchedPairs.size();
        std::vector<size_t> smallSide;
        std::vector<size_t> largeSide;
        for (size_t indexOnSmall = matchedPairs[i].first, steps = 0;
            ;
            indexOnSmall = (indexOnSmall + 1) % small->size(), ++steps) {
            smallSide.push_back((*small)[indexOnSmall]);
            if (steps > 0 && indexOnSmall == matchedPairs[j].first)
                break;
            if (steps >= small->size())
                break;
        }
        for (size_t indexOnLarge = matchedPairs[j].second, steps = 0;
            ;
            indexOnLarge = (indexOnLarge + 1) % large->size(), ++steps) {
            largeSide.push_back((*large)[indexOnLarge]);
            if (steps > 0 && indexOnLarge == matchedPairs[i].second)
                break;
            if (steps >= large->size())
                break;
        }
        std::reverse(largeSide.begin(), largeSide.end());
        fillPairs(smallSide, largeSide);
    }

    return true;
}

bool MeshRecombiner::advanceSmallForBridgeQuad(const Vector3& smallCurrent, const Vector3& smallNext,
    const Vector3& largeCurrent, const Vector3& largeNext)
{
    // The current bridge quad (smallCurrent, smallNext, largeNext, largeCurrent) can be split
    // along one of two diagonals:
    //   advance small -> diagonal (smallNext, largeCurrent)
    //   advance large -> diagonal (smallCurrent, largeNext)
    // The choice must be a deterministic function of geometry that is *equivariant* under the
    // x = 0 mirror, so the left and right seams of an x-mirrored model are triangulated as exact
    // reflections of each other (no mirror flag is carried into the recombiner).
    const Vector3& diagonalIfAdvanceSmallA = smallNext;
    const Vector3& diagonalIfAdvanceSmallB = largeCurrent;
    const Vector3& diagonalIfAdvanceLargeA = smallCurrent;
    const Vector3& diagonalIfAdvanceLargeB = largeNext;

    // 1) Prefer the shorter diagonal. Lengths are invariant under reflection, so the mirrored
    //    quad's shorter diagonal is the reflection of this quad's shorter diagonal.
    const double LENGTH_EPSILON = 1e-6;
    double diagonalSmall2 = (diagonalIfAdvanceSmallA - diagonalIfAdvanceSmallB).lengthSquared();
    double diagonalLarge2 = (diagonalIfAdvanceLargeA - diagonalIfAdvanceLargeB).lengthSquared();
    if (diagonalSmall2 + LENGTH_EPSILON < diagonalLarge2)
        return true;
    if (diagonalLarge2 + LENGTH_EPSILON < diagonalSmall2)
        return false;

    // 2) Equal-length diagonals (e.g. rectangular box seams). Break the tie purely from the
    //    x-axis: orient each diagonal from its lower (y, then z) endpoint to its upper endpoint
    //    (y and z never change under an x-mirror) and prefer the one whose ascent moves toward the
    //    mirror plane (|x| decreasing). |x| and the (y, z) ordering are unchanged by negating x,
    //    so the reflected seam evaluates the same predicate and selects the reflected diagonal.
    auto towardPlaneScore = [](const Vector3& p, const Vector3& q) -> int {
        const Vector3* lower = &p;
        const Vector3* upper = &q;
        if (upper->y() < lower->y() || (upper->y() == lower->y() && upper->z() < lower->z()))
            std::swap(lower, upper);
        double deltaAbsX = std::fabs(upper->x()) - std::fabs(lower->x());
        if (deltaAbsX < 0.0)
            return 1; // ascends toward the mirror plane
        if (deltaAbsX > 0.0)
            return -1; // ascends away from the mirror plane
        return 0; // diagonal is vertical in x (e.g. lies in the symmetry plane)
    };
    int scoreSmall = towardPlaneScore(diagonalIfAdvanceSmallA, diagonalIfAdvanceSmallB);
    int scoreLarge = towardPlaneScore(diagonalIfAdvanceLargeA, diagonalIfAdvanceLargeB);
    if (scoreSmall != scoreLarge)
        return scoreSmall > scoreLarge;

    // 3) Both diagonals lie in (or parallel to) the symmetry plane: either split is self-mirrored.
    //    Pick deterministically from x-invariant coordinates so the result is still stable.
    double keyYSmall = std::min(diagonalIfAdvanceSmallA.y(), diagonalIfAdvanceSmallB.y());
    double keyYLarge = std::min(diagonalIfAdvanceLargeA.y(), diagonalIfAdvanceLargeB.y());
    if (keyYSmall != keyYLarge)
        return keyYSmall < keyYLarge;
    double keyZSmall = std::min(diagonalIfAdvanceSmallA.z(), diagonalIfAdvanceSmallB.z());
    double keyZLarge = std::min(diagonalIfAdvanceLargeA.z(), diagonalIfAdvanceLargeB.z());
    return keyZSmall <= keyZLarge;
}

void MeshRecombiner::fillPairs(const std::vector<size_t>& small, const std::vector<size_t>& large)
{
    std::pair<std::vector<std::array<Vector3, 3>>, std::vector<std::array<Vector3, 3>>> bridgingTriangles;

    size_t smallIndex = 0;
    size_t largeIndex = 0;
    while (smallIndex + 1 < small.size() || largeIndex + 1 < large.size()) {
        if (smallIndex + 1 < small.size() && largeIndex + 1 < large.size()) {
            // Choose the diagonal with a deterministic, x-mirror-equivariant rule so the left and
            // right seams of an x-mirrored model are stitched as reflections of each other.
            if (advanceSmallForBridgeQuad((*m_vertices)[small[smallIndex]],
                    (*m_vertices)[small[smallIndex + 1]],
                    (*m_vertices)[large[largeIndex]],
                    (*m_vertices)[large[largeIndex + 1]])) {
                m_regeneratedFaces.push_back({ small[smallIndex],
                    small[smallIndex + 1],
                    large[largeIndex] });
                bridgingTriangles.second.push_back({ (*m_vertices)[small[smallIndex]], (*m_vertices)[small[smallIndex + 1]], (*m_vertices)[large[largeIndex]] });
                ++smallIndex;
                continue;
            }
            m_regeneratedFaces.push_back({ large[largeIndex + 1],
                large[largeIndex],
                small[smallIndex] });
            bridgingTriangles.first.push_back({ (*m_vertices)[large[largeIndex + 1]], (*m_vertices)[large[largeIndex]], (*m_vertices)[small[smallIndex]] });
            ++largeIndex;
            continue;
        }
        if (smallIndex + 1 >= small.size()) {
            m_regeneratedFaces.push_back({ large[largeIndex + 1],
                large[largeIndex],
                small[smallIndex] });
            bridgingTriangles.first.push_back({ (*m_vertices)[large[largeIndex + 1]], (*m_vertices)[large[largeIndex]], (*m_vertices)[small[smallIndex]] });
            ++largeIndex;
            continue;
        }
        if (largeIndex + 1 >= large.size()) {
            m_regeneratedFaces.push_back({ small[smallIndex],
                small[smallIndex + 1],
                large[largeIndex] });
            bridgingTriangles.second.push_back({ (*m_vertices)[small[smallIndex]], (*m_vertices)[small[smallIndex + 1]], (*m_vertices)[large[largeIndex]] });
            ++smallIndex;
            continue;
        }
        break;
    }

    m_generatedBridgingTriangles.emplace_back(bridgingTriangles);
}

void MeshRecombiner::removeReluctantVertices()
{
    std::vector<std::vector<size_t>> rearrangedFaces;
    std::map<size_t, size_t> oldToNewIndexMap;
    for (const auto& face : m_regeneratedFaces) {
        std::vector<size_t> newFace;
        for (const auto& index : face) {
            const auto& findIndex = oldToNewIndexMap.find(index);
            if (findIndex != oldToNewIndexMap.end()) {
                newFace.push_back(findIndex->second);
            } else {
                size_t newIndex = m_regeneratedVertices.size();
                m_regeneratedVertices.push_back((*m_vertices)[index]);
                m_regeneratedVerticesSourceIndices.push_back((*m_verticesSourceIndices)[index]);
                oldToNewIndexMap.insert({ index, newIndex });
                newFace.push_back(newIndex);
            }
        }
        rearrangedFaces.push_back(newFace);
    }
    m_regeneratedFaces = rearrangedFaces;
}

const std::vector<std::pair<std::vector<std::array<Vector3, 3>>, std::vector<std::array<Vector3, 3>>>>& MeshRecombiner::generatedBridgingTriangles()
{
    return m_generatedBridgingTriangles;
}

}
