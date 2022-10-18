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

#include <GuigueDevillers03/tri_tri_intersect.h>
#include <dust3d/base/debug.h>
#include <dust3d/base/position_key.h>
#include <dust3d/mesh/re_triangulator.h>
#include <dust3d/mesh/solid_mesh_boolean_operation.h>
#include <dust3d/util/obj.h>
#include <queue>
#include <set>
#include <stdio.h>

namespace dust3d {

static const std::vector<Vector3> g_testAxisList = {
    { std::numeric_limits<double>::max(), std::numeric_limits<double>::epsilon(), std::numeric_limits<double>::epsilon() },
    { std::numeric_limits<double>::epsilon(), std::numeric_limits<double>::max(), std::numeric_limits<double>::epsilon() },
    { std::numeric_limits<double>::epsilon(), std::numeric_limits<double>::epsilon(), std::numeric_limits<double>::max() },
};

SolidMeshBooleanOperation::SolidMeshBooleanOperation(const SolidMesh* m_firstMesh,
    const SolidMesh* m_secondMesh)
    : m_firstMesh(m_firstMesh)
    , m_secondMesh(m_secondMesh)
{
}

SolidMeshBooleanOperation::~SolidMeshBooleanOperation()
{
}

bool SolidMeshBooleanOperation::isPointInMesh(const Vector3& testPosition,
    const SolidMesh* targetMesh,
    const AxisAlignedBoudingBoxTree* meshBoxTree,
    const Vector3& testAxis)
{
    Vector3 testEnd = testPosition + testAxis;
    bool inside = false;
    std::vector<AxisAlignedBoudingBox> rayBox(1);
    auto& box = rayBox[0];
    box.update(testPosition);
    box.update(testEnd);
    AxisAlignedBoudingBoxTree testTree(&rayBox,
        { 0 },
        rayBox[0]);
    std::vector<std::pair<size_t, size_t>> pairs;
    meshBoxTree->test(meshBoxTree->root(), testTree.root(), &rayBox, &pairs);
    std::set<PositionKey> hits;

    for (const auto& it : pairs) {
        const auto& triangle = (*targetMesh->triangles())[it.first];
        std::vector<Vector3> trianglePositions = {
            (*targetMesh->vertices())[triangle[0]],
            (*targetMesh->vertices())[triangle[1]],
            (*targetMesh->vertices())[triangle[2]]
        };
        Vector3 intersection;
        if (Vector3::intersectSegmentAndPlane(testPosition, testEnd,
                trianglePositions[0],
                (*targetMesh->triangleNormals())[it.first],
                &intersection)) {
            std::vector<Vector3> normals;
            for (size_t i = 0; i < 3; ++i) {
                size_t j = (i + 1) % 3;
                normals.push_back(Vector3::normal(intersection, trianglePositions[i], trianglePositions[j]));
            }
            if (Vector3::dotProduct(normals[0], normals[1]) > 0 && Vector3::dotProduct(normals[0], normals[2]) > 0) {
                hits.insert(PositionKey(intersection));
            }
        }
    }
    inside = 0 != hits.size() % 2;

    return inside;
}

void SolidMeshBooleanOperation::searchPotentialIntersectedPairs()
{
    const AxisAlignedBoudingBoxTree* leftTree = m_firstMesh->axisAlignedBoundingBoxTree();
    const AxisAlignedBoudingBoxTree* rightTree = m_secondMesh->axisAlignedBoundingBoxTree();

    leftTree->test(leftTree->root(), rightTree->root(), m_secondMesh->triangleAxisAlignedBoundingBoxes(),
        &m_potentialIntersectedPairs);
}

bool SolidMeshBooleanOperation::intersectTwoFaces(size_t firstIndex, size_t secondIndex, std::pair<Vector3, Vector3>& newEdge)
{
    const auto& firstFace = (*m_firstMesh->triangles())[firstIndex];
    const auto& secondFace = (*m_secondMesh->triangles())[secondIndex];
    int coplanar = 0;
    if (!tri_tri_intersection_test_3d((double*)(*m_firstMesh->vertices())[firstFace[0]].constData(),
            (double*)(*m_firstMesh->vertices())[firstFace[1]].constData(),
            (double*)(*m_firstMesh->vertices())[firstFace[2]].constData(),
            (double*)(*m_secondMesh->vertices())[secondFace[0]].constData(),
            (double*)(*m_secondMesh->vertices())[secondFace[1]].constData(),
            (double*)(*m_secondMesh->vertices())[secondFace[2]].constData(),
            &coplanar,
            (double*)newEdge.first.constData(),
            (double*)newEdge.second.constData())) {
        return false;
    }
    if (coplanar)
        return false;
    return true;
}

bool SolidMeshBooleanOperation::buildPolygonsFromEdges(const std::unordered_map<size_t, std::unordered_set<size_t>>& edges,
    std::vector<std::vector<size_t>>& polygons)
{
    std::unordered_set<size_t> visited;
    for (const auto& edge : edges) {
        const auto& startEndpoint = edge.first;
        if (visited.find(startEndpoint) != visited.end())
            continue;
        std::queue<size_t> q;
        q.push(startEndpoint);
        std::vector<size_t> polyline;
        while (!q.empty()) {
            size_t loop = q.front();
            visited.insert(loop);
            polyline.push_back(loop);
            q.pop();
            auto neighborIt = edges.find(loop);
            if (neighborIt == edges.end())
                break;
            for (const auto& it : neighborIt->second) {
                if (visited.find(it) == visited.end()) {
                    q.push(it);
                    break;
                }
            }
        }
        if (polyline.size() <= 2) {
            dust3dDebug << "buildPolygonsFromEdges failed, too short";
            return false;
        }

        auto neighborOfLast = edges.find(polyline.back());
        if (neighborOfLast->second.find(startEndpoint) == neighborOfLast->second.end()) {
            dust3dDebug << "buildPolygonsFromEdges failed, could not form a ring";
            return false;
        }

        polygons.push_back(polyline);
    }

    return true;
}

void SolidMeshBooleanOperation::buildFaceGroups(const std::vector<std::vector<size_t>>& intersections,
    const std::unordered_map<uint64_t, size_t>& halfEdges,
    const std::vector<std::vector<size_t>>& triangles,
    size_t remainingStartTriangleIndex,
    size_t remainingTriangleCount,
    std::vector<std::vector<size_t>>& triangleGroups)
{
    std::unordered_map<uint64_t, size_t> halfEdgeGroupMap;
    size_t groupIndex = 0;
    std::queue<std::pair<size_t, size_t>> waitQ;
    for (const auto& intersection : intersections) {
        for (size_t i = 0; i < intersection.size(); ++i) {
            size_t j = (i + 1) % intersection.size();
            {
                auto halfEdge = makeHalfEdgeKey(intersection[i], intersection[j]);
                halfEdgeGroupMap.insert({ halfEdge, groupIndex });
                auto halfEdgeIt = halfEdges.find(halfEdge);
                if (halfEdgeIt != halfEdges.end()) {
                    waitQ.push({ halfEdgeIt->second, groupIndex });
                }
            }
            {
                auto halfEdge = makeHalfEdgeKey(intersection[j], intersection[i]);
                halfEdgeGroupMap.insert({ halfEdge, groupIndex + 1 });
                auto halfEdgeIt = halfEdges.find(halfEdge);
                if (halfEdgeIt != halfEdges.end()) {
                    waitQ.push({ halfEdgeIt->second, groupIndex + 1 });
                }
            }
        }
        groupIndex += 2;
    }

    triangleGroups.resize(groupIndex);
    std::unordered_set<size_t> visitedTriangles;

    auto processQueue = [&](std::queue<std::pair<size_t, size_t>>& q) {
        while (!q.empty()) {
            auto triangleAndGroupIndex = q.front();
            q.pop();
            if (visitedTriangles.find(triangleAndGroupIndex.first) != visitedTriangles.end())
                continue;
            visitedTriangles.insert(triangleAndGroupIndex.first);
            triangleGroups[triangleAndGroupIndex.second].push_back(triangleAndGroupIndex.first);
            const auto& indicies = triangles[triangleAndGroupIndex.first];
            for (size_t i = 0; i < 3; ++i) {
                size_t j = (i + 1) % 3;
                auto halfEdge = makeHalfEdgeKey(indicies[i], indicies[j]);
                if (halfEdgeGroupMap.find(halfEdge) != halfEdgeGroupMap.end())
                    continue;
                halfEdgeGroupMap.insert({ halfEdge, triangleAndGroupIndex.second });
                auto halfEdgeIt = halfEdges.find(makeHalfEdgeKey(indicies[j], indicies[i]));
                if (halfEdgeIt != halfEdges.end()) {
                    q.push({ halfEdgeIt->second, triangleAndGroupIndex.second });
                } else {
                    //dust3dDebug << "HalfEdge not found:" << halfEdge.second << " " << halfEdge.first;
                }
            }
        }
    };

    processQueue(waitQ);

    size_t endIndex = remainingStartTriangleIndex + remainingTriangleCount;
    for (size_t triangleIndex = remainingStartTriangleIndex; triangleIndex < endIndex; ++triangleIndex) {
        if (visitedTriangles.find(triangleIndex) != visitedTriangles.end())
            continue;
        triangleGroups.push_back(std::vector<size_t>());
        waitQ.push({ triangleIndex, groupIndex });
        ++groupIndex;
        processQueue(waitQ);
    }
}

size_t SolidMeshBooleanOperation::addNewPoint(const Vector3& position)
{
    auto insertResult = m_newPositionMap.insert({ PositionKey(position), m_newVertices.size() });
    if (insertResult.second) {
        m_newVertices.push_back(position);
    }
    return insertResult.first->second;
}

bool SolidMeshBooleanOperation::addUnintersectedTriangles(const SolidMesh* mesh,
    const std::unordered_set<size_t>& usedFaces,
    std::unordered_map<uint64_t, size_t>* halfEdges)
{
    size_t oldVertexCount = m_newVertices.size();
    const auto& vertices = *mesh->vertices();
    m_newVertices.reserve(m_newVertices.size() + vertices.size());
    m_newVertices.insert(m_newVertices.end(),
        vertices.begin(), vertices.end());
    size_t triangleCount = mesh->triangles()->size();
    m_newTriangles.reserve(m_newTriangles.size() + triangleCount - usedFaces.size());
    for (size_t i = 0; i < triangleCount; ++i) {
        if (usedFaces.find(i) != usedFaces.end())
            continue;
        const auto& oldTriangle = (*mesh->triangles())[i];
        size_t newInsertedIndex = m_newTriangles.size();
        m_newTriangles.push_back({ oldTriangle[0] + oldVertexCount,
            oldTriangle[1] + oldVertexCount,
            oldTriangle[2] + oldVertexCount });
        const auto& newInsertedTriangle = m_newTriangles.back();
        if (!halfEdges->insert({ makeHalfEdgeKey(newInsertedTriangle[0], newInsertedTriangle[1]), newInsertedIndex }).second) {
            dust3dDebug << "Found repeated halfedge:" << newInsertedTriangle[0] << "," << newInsertedTriangle[1];
            return false;
        }
        if (!halfEdges->insert({ makeHalfEdgeKey(newInsertedTriangle[1], newInsertedTriangle[2]), newInsertedIndex }).second) {
            dust3dDebug << "Found repeated halfedge:" << newInsertedTriangle[0] << "," << newInsertedTriangle[1];
            return false;
        }
        if (!halfEdges->insert({ makeHalfEdgeKey(newInsertedTriangle[2], newInsertedTriangle[0]), newInsertedIndex }).second) {
            dust3dDebug << "Found repeated halfedge:" << newInsertedTriangle[0] << "," << newInsertedTriangle[1];
            return false;
        }
    }
    return true;
}

bool SolidMeshBooleanOperation::combine()
{
    searchPotentialIntersectedPairs();

    struct IntersectedContext {
        std::vector<Vector3> points;
        std::map<PositionKey, size_t> positionMap;
        std::unordered_map<size_t, std::unordered_set<size_t>> neighborMap;
    };

    std::unordered_map<size_t, IntersectedContext> firstTriangleIntersectedContext;
    std::unordered_map<size_t, IntersectedContext> secondTriangleIntersectedContext;

    auto addIntersectedPoint = [](IntersectedContext& context, const Vector3& position) {
        auto insertResult = context.positionMap.insert({ PositionKey(position), context.points.size() });
        if (insertResult.second)
            context.points.push_back(position);
        return insertResult.first->second;
    };

    for (const auto& pair : m_potentialIntersectedPairs) {
        std::pair<Vector3, Vector3> newEdge;
        if (intersectTwoFaces(pair.first, pair.second, newEdge)) {
            m_firstIntersectedFaces.insert(pair.first);
            m_secondIntersectedFaces.insert(pair.second);

            {
                auto& context = firstTriangleIntersectedContext[pair.first];
                size_t firstPointIndex = 3 + addIntersectedPoint(context, newEdge.first);
                size_t secondPointIndex = 3 + addIntersectedPoint(context, newEdge.second);
                if (firstPointIndex != secondPointIndex) {
                    context.neighborMap[firstPointIndex].insert(secondPointIndex);
                    context.neighborMap[secondPointIndex].insert(firstPointIndex);
                }
            }

            {
                auto& context = secondTriangleIntersectedContext[pair.second];
                size_t firstPointIndex = 3 + addIntersectedPoint(context, newEdge.first);
                size_t secondPointIndex = 3 + addIntersectedPoint(context, newEdge.second);
                if (firstPointIndex != secondPointIndex) {
                    context.neighborMap[firstPointIndex].insert(secondPointIndex);
                    context.neighborMap[secondPointIndex].insert(firstPointIndex);
                }
            }
        }
    }

    std::unordered_map<size_t, std::unordered_set<size_t>> firstEdges;
    std::unordered_map<size_t, std::unordered_set<size_t>> secondEdges;
    std::vector<std::vector<size_t>> firstIntersections;
    std::vector<std::vector<size_t>> secondIntersections;
    std::unordered_map<uint64_t, size_t> firstHalfEdges;
    std::unordered_map<uint64_t, size_t> secondHalfEdges;

    auto reTriangulate = [&](const std::unordered_map<size_t, IntersectedContext>& context,
                             const SolidMesh* mesh,
                             size_t startOldVertex,
                             std::unordered_map<size_t, std::unordered_set<size_t>>& edges,
                             std::unordered_map<uint64_t, size_t>& halfEdges) {
        for (const auto& it : context) {
            const auto& triangle = (*mesh->triangles())[it.first];
            ReTriangulator reTriangulator({ (*mesh->vertices())[triangle[0]],
                                              (*mesh->vertices())[triangle[1]],
                                              (*mesh->vertices())[triangle[2]] },
                (*mesh->triangleNormals())[it.first]);
            reTriangulator.setEdges(it.second.points,
                &it.second.neighborMap);
            if (!reTriangulator.reTriangulate()) {
                dust3dDebug << "Retriangle failed";
                return false;
            }
            std::vector<size_t> newIndices;
            newIndices.reserve(3 + it.second.points.size());
            newIndices.push_back(startOldVertex + triangle[0]);
            newIndices.push_back(startOldVertex + triangle[1]);
            newIndices.push_back(startOldVertex + triangle[2]);
            for (const auto& point : it.second.points)
                newIndices.push_back(addNewPoint(point));
            for (const auto& triangle : reTriangulator.triangles()) {
                size_t newInsertedIndex = m_newTriangles.size();
                m_newTriangles.push_back({ newIndices[triangle[0]],
                    newIndices[triangle[1]],
                    newIndices[triangle[2]] });
                const auto& newInsertedTriangle = m_newTriangles.back();
                if (!halfEdges.insert({ makeHalfEdgeKey(newInsertedTriangle[0], newInsertedTriangle[1]), newInsertedIndex }).second) {
                    dust3dDebug << "Found repeated halfedge:" << newInsertedTriangle[0] << "," << newInsertedTriangle[1];
                }
                if (!halfEdges.insert({ makeHalfEdgeKey(newInsertedTriangle[1], newInsertedTriangle[2]), newInsertedIndex }).second) {
                    dust3dDebug << "Found repeated halfedge:" << newInsertedTriangle[1] << "," << newInsertedTriangle[2];
                }
                if (!halfEdges.insert({ makeHalfEdgeKey(newInsertedTriangle[2], newInsertedTriangle[0]), newInsertedIndex }).second) {
                    dust3dDebug << "Found repeated halfedge:" << newInsertedTriangle[3] << "," << newInsertedTriangle[0];
                }
            }
            for (const auto& it : it.second.neighborMap) {
                auto from = newIndices[it.first];
                for (const auto& it2 : it.second) {
                    auto to = newIndices[it2];
                    edges[from].insert(to);
                    edges[to].insert(from);
                }
            }
        }
        return true;
    };

    size_t firstRemainingStartTriangleIndex = m_newTriangles.size();
    if (!addUnintersectedTriangles(m_firstMesh, m_firstIntersectedFaces, &firstHalfEdges)) {
        dust3dDebug << "Add first mesh remaining triangles failed";
    }
    size_t firstRemainingTriangleCount = m_newTriangles.size() - firstRemainingStartTriangleIndex;

    size_t secondRemainingStartTriangleIndex = m_newTriangles.size();
    if (!addUnintersectedTriangles(m_secondMesh, m_secondIntersectedFaces, &secondHalfEdges)) {
        dust3dDebug << "Add second mesh remaining triangles failed";
    }
    size_t secondRemainingTriangleCount = m_newTriangles.size() - secondRemainingStartTriangleIndex;

    if (!reTriangulate(firstTriangleIntersectedContext,
            m_firstMesh, 0, firstEdges, firstHalfEdges)) {
        dust3dDebug << "Retriangulate first mesh failed";
        return false;
    }
    if (!reTriangulate(secondTriangleIntersectedContext,
            m_secondMesh, m_firstMesh->vertices()->size(), secondEdges, secondHalfEdges)) {
        dust3dDebug << "Retriangulate second mesh failed";
        return false;
    }

    if (!buildPolygonsFromEdges(firstEdges, firstIntersections)) {
        dust3dDebug << "Build polygons from edges failed";
        return false;
    }

    buildFaceGroups(firstIntersections,
        firstHalfEdges,
        m_newTriangles,
        firstRemainingStartTriangleIndex,
        firstRemainingTriangleCount,
        m_firstTriangleGroups);
    buildFaceGroups(firstIntersections,
        secondHalfEdges,
        m_newTriangles,
        secondRemainingStartTriangleIndex,
        secondRemainingTriangleCount,
        m_secondTriangleGroups);

    decideGroupSide(m_firstTriangleGroups,
        m_secondMesh,
        m_secondMesh->axisAlignedBoundingBoxTree(),
        m_firstGroupSides);
    decideGroupSide(m_secondTriangleGroups,
        m_firstMesh,
        m_firstMesh->axisAlignedBoundingBoxTree(),
        m_secondGroupSides);

    return true;
}

void SolidMeshBooleanOperation::decideGroupSide(const std::vector<std::vector<size_t>>& groups,
    const SolidMesh* mesh,
    const AxisAlignedBoudingBoxTree* tree,
    std::vector<bool>& groupSides)
{
    groupSides.resize(groups.size());
    for (size_t i = 0; i < groups.size(); ++i) {
        const auto& group = groups[i];
        if (group.empty())
            continue;
        size_t insideCount = 0;
        size_t totalCount = 0;
        for (size_t pickIndex = 0; pickIndex < 1 && pickIndex < group.size(); ++pickIndex) {
            for (size_t axisIndex = 0; axisIndex < g_testAxisList.size(); ++axisIndex) {
                const auto& pickedTriangle = m_newTriangles[group[pickIndex]];
                bool inside = isPointInMesh((m_newVertices[pickedTriangle[0]] + m_newVertices[pickedTriangle[1]] + m_newVertices[pickedTriangle[2]]) / 3.0,
                    mesh,
                    tree,
                    g_testAxisList[axisIndex]);
                if (inside)
                    ++insideCount;
                ++totalCount;
            }
        }
        groupSides[i] = (float)insideCount / totalCount > 0.5;
    }
}

void SolidMeshBooleanOperation::fetchUnion(std::vector<std::vector<size_t>>& resultTriangles)
{
    for (size_t i = 0; i < m_firstGroupSides.size(); ++i) {
        if (m_firstGroupSides[i])
            continue;
        for (const auto& it : m_firstTriangleGroups[i])
            resultTriangles.push_back(m_newTriangles[it]);
    }

    for (size_t i = 0; i < m_secondGroupSides.size(); ++i) {
        if (m_secondGroupSides[i])
            continue;
        for (const auto& it : m_secondTriangleGroups[i])
            resultTriangles.push_back(m_newTriangles[it]);
    }
}

void SolidMeshBooleanOperation::fetchDiff(std::vector<std::vector<size_t>>& resultTriangles)
{
    for (size_t i = 0; i < m_firstGroupSides.size(); ++i) {
        if (m_firstGroupSides[i])
            continue;
        for (const auto& it : m_firstTriangleGroups[i])
            resultTriangles.push_back(m_newTriangles[it]);
    }

    for (size_t i = 0; i < m_secondGroupSides.size(); ++i) {
        if (!m_secondGroupSides[i])
            continue;
        for (const auto& it : m_secondTriangleGroups[i]) {
            auto triangle = m_newTriangles[it];
            resultTriangles.push_back({ triangle[2], triangle[1], triangle[0] });
        }
    }
}

void SolidMeshBooleanOperation::fetchIntersect(std::vector<std::vector<size_t>>& resultTriangles)
{
    for (size_t i = 0; i < m_firstGroupSides.size(); ++i) {
        if (!m_firstGroupSides[i])
            continue;
        for (const auto& it : m_firstTriangleGroups[i])
            resultTriangles.push_back(m_newTriangles[it]);
    }

    for (size_t i = 0; i < m_secondGroupSides.size(); ++i) {
        if (!m_secondGroupSides[i])
            continue;
        for (const auto& it : m_secondTriangleGroups[i])
            resultTriangles.push_back(m_newTriangles[it]);
    }
}

const std::vector<Vector3>& SolidMeshBooleanOperation::resultVertices()
{
    return m_newVertices;
}

}
