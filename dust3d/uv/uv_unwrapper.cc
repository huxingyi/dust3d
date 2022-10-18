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

#include <Eigen/Dense>
#include <cmath>
#include <dust3d/uv/chart_packer.h>
#include <dust3d/uv/parametrize.h>
#include <dust3d/uv/triangulate.h>
#include <dust3d/uv/uv_unwrapper.h>
#include <map>
#include <queue>
#include <set>
#include <unordered_set>

namespace dust3d {
namespace uv {

    const std::vector<float> UvUnwrapper::m_rotateDegrees = { 5, 15, 20, 25, 30, 35, 40, 45 };

    static float areaOf3dTriangle(const Eigen::Vector3d& a, const Eigen::Vector3d& b, const Eigen::Vector3d& c)
    {
        auto ab = b - a;
        auto ac = c - a;
        return 0.5 * (ab.cross(ac)).norm();
    }

    static float areaOf2dTriangle(const Eigen::Vector2d& a, const Eigen::Vector2d& b, const Eigen::Vector2d& c)
    {
        return areaOf3dTriangle(Eigen::Vector3d(a.x(), a.y(), 0),
            Eigen::Vector3d(b.x(), b.y(), 0),
            Eigen::Vector3d(c.x(), c.y(), 0));
    }

    void UvUnwrapper::setMesh(const Mesh& mesh)
    {
        m_mesh = mesh;
    }

    void UvUnwrapper::setTexelSize(float texelSize)
    {
        m_texelSizePerUnit = texelSize;
    }

    const std::vector<FaceTextureCoords>& UvUnwrapper::getFaceUvs() const
    {
        return m_faceUvs;
    }

    const std::vector<Rect>& UvUnwrapper::getChartRects() const
    {
        return m_chartRects;
    }

    const std::vector<int>& UvUnwrapper::getChartSourcePartitions() const
    {
        return m_chartSourcePartitions;
    }

    void UvUnwrapper::buildEdgeToFaceMap(const std::vector<Face>& faces, std::map<std::pair<size_t, size_t>, size_t>& edgeToFaceMap)
    {
        edgeToFaceMap.clear();
        for (decltype(faces.size()) index = 0; index < faces.size(); ++index) {
            const auto& face = faces[index];
            for (size_t i = 0; i < 3; i++) {
                size_t j = (i + 1) % 3;
                edgeToFaceMap[{ face.indices[i], face.indices[j] }] = index;
            }
        }
    }

    void UvUnwrapper::buildEdgeToFaceMap(const std::vector<size_t>& group, std::map<std::pair<size_t, size_t>, size_t>& edgeToFaceMap)
    {
        edgeToFaceMap.clear();
        for (const auto& index : group) {
            const auto& face = m_mesh.faces[index];
            for (size_t i = 0; i < 3; i++) {
                size_t j = (i + 1) % 3;
                edgeToFaceMap[{ face.indices[i], face.indices[j] }] = index;
            }
        }
    }

    void UvUnwrapper::splitPartitionToIslands(const std::vector<size_t>& group, std::vector<std::vector<size_t>>& islands)
    {
        std::map<std::pair<size_t, size_t>, size_t> edgeToFaceMap;
        buildEdgeToFaceMap(group, edgeToFaceMap);
        bool segmentByNormal = !m_mesh.faceNormals.empty() && m_segmentByNormal;

        std::unordered_set<size_t> processedFaces;
        std::queue<size_t> waitFaces;
        for (const auto& indexInGroup : group) {
            if (processedFaces.find(indexInGroup) != processedFaces.end())
                continue;
            waitFaces.push(indexInGroup);
            std::vector<size_t> island;
            while (!waitFaces.empty()) {
                size_t index = waitFaces.front();
                waitFaces.pop();
                if (processedFaces.find(index) != processedFaces.end())
                    continue;
                const auto& face = m_mesh.faces[index];
                for (size_t i = 0; i < 3; i++) {
                    size_t j = (i + 1) % 3;
                    auto findOppositeFaceResult = edgeToFaceMap.find({ face.indices[j], face.indices[i] });
                    if (findOppositeFaceResult == edgeToFaceMap.end())
                        continue;
                    if (segmentByNormal) {
                        if (dotProduct(m_mesh.faceNormals[findOppositeFaceResult->second],
                                m_mesh.faceNormals[m_segmentPreferMorePieces ? indexInGroup : index])
                            < m_segmentDotProductThreshold) {
                            continue;
                        }
                    }
                    waitFaces.push(findOppositeFaceResult->second);
                }
                island.push_back(index);
                processedFaces.insert(index);
            }
            if (island.empty())
                continue;
            islands.push_back(island);
        }
    }

    double UvUnwrapper::distanceBetweenVertices(const Vertex& first, const Vertex& second)
    {
        float x = first.xyz[0] - second.xyz[0];
        float y = first.xyz[1] - second.xyz[1];
        float z = first.xyz[2] - second.xyz[2];
        return std::sqrt(x * x + y * y + z * z);
    }

    void UvUnwrapper::calculateFaceTextureBoundingBox(const std::vector<FaceTextureCoords>& faceTextureCoords,
        float& left, float& top, float& right, float& bottom)
    {
        bool leftFirstTime = true;
        bool topFirstTime = true;
        bool rightFirstTime = true;
        bool bottomFirstTime = true;
        for (const auto& item : faceTextureCoords) {
            for (int i = 0; i < 3; ++i) {
                const auto& x = item.coords[i].uv[0];
                const auto& y = item.coords[i].uv[1];
                if (leftFirstTime || x < left) {
                    left = x;
                    leftFirstTime = false;
                }
                if (rightFirstTime || x > right) {
                    right = x;
                    rightFirstTime = false;
                }
                if (topFirstTime || y < top) {
                    top = y;
                    topFirstTime = false;
                }
                if (bottomFirstTime || y > bottom) {
                    bottom = y;
                    bottomFirstTime = false;
                }
            }
        }
    }

    void UvUnwrapper::triangulateRing(const std::vector<Vertex>& verticies,
        std::vector<Face>& faces, const std::vector<size_t>& ring)
    {
        triangulate(verticies, faces, ring);
    }

    // The hole filling faces should be put in the back of faces vector, so these uv coords of appended faces will be disgarded.
    bool UvUnwrapper::fixHolesExceptTheLongestRing(const std::vector<Vertex>& verticies, std::vector<Face>& faces, size_t* remainingHoleNum)
    {
        std::map<std::pair<size_t, size_t>, size_t> edgeToFaceMap;
        buildEdgeToFaceMap(faces, edgeToFaceMap);

        std::map<size_t, std::vector<size_t>> holeVertexLink;
        for (const auto& face : faces) {
            for (size_t i = 0; i < 3; i++) {
                size_t j = (i + 1) % 3;
                auto findOppositeFaceResult = edgeToFaceMap.find({ face.indices[j], face.indices[i] });
                if (findOppositeFaceResult != edgeToFaceMap.end())
                    continue;
                holeVertexLink[face.indices[j]].push_back(face.indices[i]);
            }
        }

        std::vector<std::pair<std::vector<size_t>, double>> holeRings;
        while (!holeVertexLink.empty()) {
            bool foundRing = false;
            std::vector<size_t> ring;
            std::unordered_set<size_t> visited;
            std::set<std::pair<size_t, size_t>> visitedPath;
            double ringLength = 0;
            while (!foundRing) {
                ring.clear();
                visited.clear();
                ringLength = 0;
                auto first = holeVertexLink.begin()->first;
                auto index = first;
                auto prev = first;
                ring.push_back(first);
                visited.insert(first);
                while (true) {
                    auto findLinkResult = holeVertexLink.find(index);
                    if (findLinkResult == holeVertexLink.end()) {
                        return false;
                    }
                    for (const auto& item : findLinkResult->second) {
                        if (item == first) {
                            foundRing = true;
                            break;
                        }
                    }
                    if (foundRing)
                        break;
                    if (findLinkResult->second.size() > 1) {
                        bool foundNewPath = false;
                        for (const auto& item : findLinkResult->second) {
                            if (visitedPath.find({ prev, item }) == visitedPath.end()) {
                                index = item;
                                foundNewPath = true;
                                break;
                            }
                        }
                        if (!foundNewPath) {
                            return false;
                        }
                        visitedPath.insert({ prev, index });
                    } else {
                        index = *findLinkResult->second.begin();
                    }
                    if (visited.find(index) != visited.end()) {
                        while (index != *ring.begin())
                            ring.erase(ring.begin());
                        foundRing = true;
                        break;
                    }
                    ring.push_back(index);
                    visited.insert(index);
                    ringLength += distanceBetweenVertices(verticies[index], verticies[prev]);
                    prev = index;
                }
            }
            if (ring.size() < 3) {
                return false;
            }
            holeRings.push_back({ ring, ringLength });
            for (size_t i = 0; i < ring.size(); ++i) {
                size_t j = (i + 1) % ring.size();
                auto findLinkResult = holeVertexLink.find(ring[i]);
                for (auto it = findLinkResult->second.begin(); it != findLinkResult->second.end(); ++it) {
                    if (*it == ring[j]) {
                        findLinkResult->second.erase(it);
                        if (findLinkResult->second.empty())
                            holeVertexLink.erase(ring[i]);
                        break;
                    }
                }
            }
        }

        if (holeRings.size() > 1) {
            // Sort by ring length, the longer ring sit in the lower array indices
            std::sort(holeRings.begin(), holeRings.end(), [](const std::pair<std::vector<size_t>, double>& first, const std::pair<std::vector<size_t>, double>& second) {
                return first.second > second.second;
            });
            for (size_t i = 1; i < holeRings.size(); ++i) {
                triangulateRing(verticies, faces, holeRings[i].first);
            }
            holeRings.resize(1);
        }

        if (remainingHoleNum)
            *remainingHoleNum = holeRings.size();
        return true;
    }

    void UvUnwrapper::makeSeamAndCut(const std::vector<Vertex>& verticies,
        const std::vector<Face>& faces,
        std::map<size_t, size_t>& localToGlobalFacesMap,
        std::vector<size_t>& firstGroup, std::vector<size_t>& secondGroup)
    {
        // We group the chart by first pick the top(max y) triangle, then join the adjecent traigles until the joint count reach to half of total

        double maxY = 0;
        bool firstTime = true;
        int choosenIndex = -1;
        for (decltype(faces.size()) i = 0; i < faces.size(); ++i) {
            const auto& face = faces[i];
            for (int j = 0; j < 3; ++j) {
                const auto& vertex = verticies[face.indices[j]];
                if (firstTime || vertex.xyz[2] > maxY) {
                    maxY = vertex.xyz[2];
                    firstTime = false;
                    choosenIndex = i;
                }
            }
        }
        if (-1 == choosenIndex)
            return;

        std::map<std::pair<size_t, size_t>, size_t> edgeToFaceMap;
        buildEdgeToFaceMap(faces, edgeToFaceMap);

        std::unordered_set<size_t> processedFaces;
        std::queue<size_t> waitFaces;
        waitFaces.push(choosenIndex);
        while (!waitFaces.empty()) {
            auto index = waitFaces.front();
            waitFaces.pop();
            if (processedFaces.find(index) != processedFaces.end())
                continue;
            const auto& face = faces[index];
            for (size_t i = 0; i < 3; i++) {
                size_t j = (i + 1) % 3;
                auto findOppositeFaceResult = edgeToFaceMap.find({ face.indices[j], face.indices[i] });
                if (findOppositeFaceResult == edgeToFaceMap.end())
                    continue;
                waitFaces.push(findOppositeFaceResult->second);
            }
            processedFaces.insert(index);
            firstGroup.push_back(localToGlobalFacesMap[index]);
            if (firstGroup.size() * 2 >= faces.size())
                break;
        }
        for (decltype(faces.size()) index = 0; index < faces.size(); ++index) {
            if (processedFaces.find(index) != processedFaces.end())
                continue;
            secondGroup.push_back(localToGlobalFacesMap[index]);
        }
    }

    void UvUnwrapper::calculateSizeAndRemoveInvalidCharts()
    {
        auto charts = m_charts;
        auto chartSourcePartitions = m_chartSourcePartitions;
        m_charts.clear();
        m_chartSourcePartitions.clear();
        for (size_t chartIndex = 0; chartIndex < charts.size(); ++chartIndex) {
            auto& chart = charts[chartIndex];
            float left, top, right, bottom;
            left = top = right = bottom = 0;
            calculateFaceTextureBoundingBox(chart.second, left, top, right, bottom);
            std::pair<float, float> size = { right - left, bottom - top };
            if (size.first <= 0 || std::isnan(size.first) || std::isinf(size.first) || size.second <= 0 || std::isnan(size.second) || std::isinf(size.second)) {
                continue;
            }
            float surfaceArea = 0;
            for (const auto& item : chart.first) {
                const auto& face = m_mesh.faces[item];
                surfaceArea += areaOf3dTriangle(Eigen::Vector3d(m_mesh.vertices[face.indices[0]].xyz[0],
                                                    m_mesh.vertices[face.indices[0]].xyz[1],
                                                    m_mesh.vertices[face.indices[0]].xyz[2]),
                    Eigen::Vector3d(m_mesh.vertices[face.indices[1]].xyz[0],
                        m_mesh.vertices[face.indices[1]].xyz[1],
                        m_mesh.vertices[face.indices[1]].xyz[2]),
                    Eigen::Vector3d(m_mesh.vertices[face.indices[2]].xyz[0],
                        m_mesh.vertices[face.indices[2]].xyz[1],
                        m_mesh.vertices[face.indices[2]].xyz[2]));
            }
            float uvArea = 0;
            for (auto& item : chart.second) {
                for (int i = 0; i < 3; ++i) {
                    item.coords[i].uv[0] -= left;
                    item.coords[i].uv[1] -= top;
                }
                uvArea += areaOf2dTriangle(Eigen::Vector2d(item.coords[0].uv[0], item.coords[0].uv[1]),
                    Eigen::Vector2d(item.coords[1].uv[0], item.coords[1].uv[1]),
                    Eigen::Vector2d(item.coords[2].uv[0], item.coords[2].uv[1]));
            }
            if (m_enableRotation) {
                Eigen::Vector3d center(size.first * 0.5, size.second * 0.5, 0);
                float minRectArea = size.first * size.second;
                float minRectLeft = 0;
                float minRectTop = 0;
                bool rotated = false;
                for (const auto& degree : m_rotateDegrees) {
                    Eigen::Matrix3d matrix;
                    matrix = Eigen::AngleAxisd(degree * 180.0 / 3.1415926, Eigen::Vector3d::UnitZ());
                    std::vector<FaceTextureCoords> rotatedUvs;
                    for (auto& item : chart.second) {
                        FaceTextureCoords rotatedCoords;
                        for (int i = 0; i < 3; ++i) {
                            Eigen::Vector3d point(item.coords[i].uv[0], item.coords[i].uv[1], 0);
                            point -= center;
                            point = matrix * point;
                            rotatedCoords.coords[i].uv[0] = point.x();
                            rotatedCoords.coords[i].uv[1] = point.y();
                        }
                        rotatedUvs.push_back(rotatedCoords);
                    }
                    left = top = right = bottom = 0;
                    calculateFaceTextureBoundingBox(rotatedUvs, left, top, right, bottom);
                    std::pair<float, float> newSize = { right - left, bottom - top };
                    float newRectArea = newSize.first * newSize.second;
                    if (newRectArea < minRectArea) {
                        minRectArea = newRectArea;
                        size = newSize;
                        minRectLeft = left;
                        minRectTop = top;
                        rotated = true;
                        chart.second = rotatedUvs;
                    }
                }
                if (rotated) {
                    for (auto& item : chart.second) {
                        for (int i = 0; i < 3; ++i) {
                            item.coords[i].uv[0] -= minRectLeft;
                            item.coords[i].uv[1] -= minRectTop;
                        }
                    }
                }
            }
            float ratioOfSurfaceAreaAndUvArea = uvArea > 0 ? surfaceArea / uvArea : 1.0;
            float scale = ratioOfSurfaceAreaAndUvArea * m_texelSizePerUnit;
            m_chartSizes.push_back(size);
            m_scaledChartSizes.push_back(std::make_pair(size.first * scale, size.second * scale));
            m_charts.push_back(chart);
            m_chartSourcePartitions.push_back(chartSourcePartitions[chartIndex]);
        }
    }

    void UvUnwrapper::packCharts()
    {
        ChartPacker chartPacker;
        chartPacker.setCharts(m_scaledChartSizes);
        m_resultTextureSize = chartPacker.pack();
        m_chartRects.resize(m_chartSizes.size());
        const std::vector<std::tuple<float, float, float, float, bool>>& packedResult = chartPacker.getResult();
        for (decltype(m_charts.size()) i = 0; i < m_charts.size(); ++i) {
            const auto& chartSize = m_chartSizes[i];
            auto& chart = m_charts[i];
            if (i >= packedResult.size()) {
                for (auto& item : chart.second) {
                    for (int i = 0; i < 3; ++i) {
                        item.coords[i].uv[0] = 0;
                        item.coords[i].uv[1] = 0;
                    }
                }
                continue;
            }
            const auto& result = packedResult[i];
            auto& left = std::get<0>(result);
            auto& top = std::get<1>(result);
            auto& width = std::get<2>(result);
            auto& height = std::get<3>(result);
            auto& flipped = std::get<4>(result);
            if (flipped)
                m_chartRects[i] = { left, top, height, width };
            else
                m_chartRects[i] = { left, top, width, height };
            if (flipped) {
                for (auto& item : chart.second) {
                    for (int i = 0; i < 3; ++i) {
                        std::swap(item.coords[i].uv[0], item.coords[i].uv[1]);
                    }
                }
            }
            for (auto& item : chart.second) {
                for (int i = 0; i < 3; ++i) {
                    item.coords[i].uv[0] /= chartSize.first;
                    item.coords[i].uv[1] /= chartSize.second;
                    item.coords[i].uv[0] *= width;
                    item.coords[i].uv[1] *= height;
                    item.coords[i].uv[0] += left;
                    item.coords[i].uv[1] += top;
                }
            }
        }
    }

    void UvUnwrapper::finalizeUv()
    {
        m_faceUvs.resize(m_mesh.faces.size());
        for (const auto& chart : m_charts) {
            for (decltype(chart.second.size()) i = 0; i < chart.second.size(); ++i) {
                auto& faceUv = m_faceUvs[chart.first[i]];
                faceUv = chart.second[i];
            }
        }
    }

    void UvUnwrapper::partition()
    {
        m_partitions.clear();
        if (m_mesh.facePartitions.empty()) {
            for (decltype(m_mesh.faces.size()) i = 0; i < m_mesh.faces.size(); i++) {
                m_partitions[0].push_back(i);
            }
        } else {
            for (decltype(m_mesh.faces.size()) i = 0; i < m_mesh.faces.size(); i++) {
                int partition = m_mesh.facePartitions[i];
                m_partitions[partition].push_back(i);
            }
        }
    }

    void UvUnwrapper::unwrapSingleIsland(const std::vector<size_t>& group, int sourcePartition, bool skipCheckHoles)
    {
        if (group.empty())
            return;

        std::vector<Vertex> localVertices;
        std::vector<Face> localFaces;
        std::map<size_t, size_t> globalToLocalVerticesMap;
        std::map<size_t, size_t> localToGlobalFacesMap;
        for (decltype(group.size()) i = 0; i < group.size(); ++i) {
            const auto& globalFace = m_mesh.faces[group[i]];
            Face localFace;
            for (size_t j = 0; j < 3; j++) {
                int globalVertexIndex = globalFace.indices[j];
                if (globalToLocalVerticesMap.find(globalVertexIndex) == globalToLocalVerticesMap.end()) {
                    localVertices.push_back(m_mesh.vertices[globalVertexIndex]);
                    globalToLocalVerticesMap[globalVertexIndex] = (int)localVertices.size() - 1;
                }
                localFace.indices[j] = globalToLocalVerticesMap[globalVertexIndex];
            }
            localFaces.push_back(localFace);
            localToGlobalFacesMap[localFaces.size() - 1] = group[i];
        }

        decltype(localFaces.size()) faceNumBeforeFix = localFaces.size();
        size_t remainingHoleNumAfterFix = 0;
        if (!fixHolesExceptTheLongestRing(localVertices, localFaces, &remainingHoleNumAfterFix)) {
            return;
        }
        if (1 == remainingHoleNumAfterFix) {
            parametrizeSingleGroup(localVertices, localFaces, localToGlobalFacesMap, faceNumBeforeFix, sourcePartition);
            return;
        }

        if (0 == remainingHoleNumAfterFix) {
            std::vector<size_t> firstGroup;
            std::vector<size_t> secondGroup;
            makeSeamAndCut(localVertices, localFaces, localToGlobalFacesMap, firstGroup, secondGroup);
            if (firstGroup.empty() || secondGroup.empty()) {
                return;
            }
            unwrapSingleIsland(firstGroup, sourcePartition, true);
            unwrapSingleIsland(secondGroup, sourcePartition, true);
            return;
        }
    }

    void UvUnwrapper::parametrizeSingleGroup(const std::vector<Vertex>& verticies,
        const std::vector<Face>& faces,
        std::map<size_t, size_t>& localToGlobalFacesMap,
        size_t faceNumToChart,
        int sourcePartition)
    {
        std::vector<TextureCoord> localVertexUvs;
        if (!parametrize(verticies, faces, localVertexUvs))
            return;
        std::pair<std::vector<size_t>, std::vector<FaceTextureCoords>> chart;
        for (size_t i = 0; i < faceNumToChart; ++i) {
            const auto& localFace = faces[i];
            auto globalFaceIndex = localToGlobalFacesMap[i];
            FaceTextureCoords faceUv;
            for (size_t j = 0; j < 3; j++) {
                const auto& localVertexIndex = localFace.indices[j];
                const auto& vertexUv = localVertexUvs[localVertexIndex];
                faceUv.coords[j].uv[0] = vertexUv.uv[0];
                faceUv.coords[j].uv[1] = vertexUv.uv[1];
            }
            chart.first.push_back(globalFaceIndex);
            chart.second.push_back(faceUv);
        }
        if (chart.first.empty())
            return;
        m_charts.push_back(chart);
        m_chartSourcePartitions.push_back(sourcePartition);
    }

    float UvUnwrapper::getTextureSize() const
    {
        return m_resultTextureSize;
    }

    void UvUnwrapper::unwrap()
    {
        partition();

        m_faceUvs.resize(m_mesh.faces.size());
        for (const auto& group : m_partitions) {
            std::vector<std::vector<size_t>> islands;
            splitPartitionToIslands(group.second, islands);
            for (const auto& island : islands)
                unwrapSingleIsland(island, group.first);
        }

        calculateSizeAndRemoveInvalidCharts();
        packCharts();
        finalizeUv();
    }

}
}
