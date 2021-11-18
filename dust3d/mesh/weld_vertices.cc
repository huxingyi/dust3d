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

#include <unordered_set>
#include <unordered_map>
#include <map>
#include <dust3d/mesh/weld_vertices.h>

namespace dust3d
{
    
size_t weldVertices(const std::vector<Vector3> &sourceVertices, const std::vector<std::vector<size_t>> &sourceTriangles,
    float allowedSmallestDistance, const std::set<PositionKey> &excludePositions,
    std::vector<Vector3> &destVertices, std::vector<std::vector<size_t>> &destTriangles)
{
    std::unordered_set<int> excludeVertices;
    for (size_t i = 0; i < sourceVertices.size(); ++i) {
        if (excludePositions.find(sourceVertices[i]) != excludePositions.end())
            excludeVertices.insert(i);
    }
    float squareOfAllowedSmallestDistance = allowedSmallestDistance * allowedSmallestDistance;
    std::map<int, int> weldVertexToMap;
    std::unordered_set<int> weldTargetVertices;
    std::unordered_set<int> processedFaces;
    std::map<std::pair<int, int>, std::pair<int, int>> triangleEdgeMap;
    std::unordered_map<int, int> vertexAdjFaceCountMap;
    for (int i = 0; i < (int)sourceTriangles.size(); i++) {
        const auto &faceIndices = sourceTriangles[i];
        if (faceIndices.size() == 3) {
            vertexAdjFaceCountMap[faceIndices[0]]++;
            vertexAdjFaceCountMap[faceIndices[1]]++;
            vertexAdjFaceCountMap[faceIndices[2]]++;
            triangleEdgeMap[std::make_pair(faceIndices[0], faceIndices[1])] = std::make_pair(i, faceIndices[2]);
            triangleEdgeMap[std::make_pair(faceIndices[1], faceIndices[2])] = std::make_pair(i, faceIndices[0]);
            triangleEdgeMap[std::make_pair(faceIndices[2], faceIndices[0])] = std::make_pair(i, faceIndices[1]);
        }
    }
    for (int i = 0; i < (int)sourceTriangles.size(); i++) {
        if (processedFaces.find(i) != processedFaces.end())
            continue;
        const auto &faceIndices = sourceTriangles[i];
        if (faceIndices.size() == 3) {
            bool indicesSeamCheck[3] = {
                excludeVertices.find(faceIndices[0]) == excludeVertices.end(),
                excludeVertices.find(faceIndices[1]) == excludeVertices.end(),
                excludeVertices.find(faceIndices[2]) == excludeVertices.end()
            };
            for (int j = 0; j < 3; j++) {
                int next = (j + 1) % 3;
                int nextNext = (j + 2) % 3;
                if (indicesSeamCheck[j] && indicesSeamCheck[next]) {
                    std::pair<int, int> edge = std::make_pair(faceIndices[j], faceIndices[next]);
                    int thirdVertexIndex = faceIndices[nextNext];
                    if ((sourceVertices[edge.first] - sourceVertices[edge.second]).lengthSquared() < squareOfAllowedSmallestDistance) {
                        auto oppositeEdge = std::make_pair(edge.second, edge.first);
                        auto findOppositeFace = triangleEdgeMap.find(oppositeEdge);
                        if (findOppositeFace == triangleEdgeMap.end()) {
                            continue;
                        }
                        int oppositeFaceIndex = findOppositeFace->second.first;
                        if (((sourceVertices[edge.first] - sourceVertices[thirdVertexIndex]).lengthSquared() <
                                    (sourceVertices[edge.second] - sourceVertices[thirdVertexIndex]).lengthSquared()) &&
                                vertexAdjFaceCountMap[edge.second] <= 4 &&
                                weldVertexToMap.find(edge.second) == weldVertexToMap.end()) {
                            weldVertexToMap[edge.second] = edge.first;
                            weldTargetVertices.insert(edge.first);
                            processedFaces.insert(i);
                            processedFaces.insert(oppositeFaceIndex);
                            break;
                        } else if (vertexAdjFaceCountMap[edge.first] <= 4 &&
                                weldVertexToMap.find(edge.first) == weldVertexToMap.end()) {
                            weldVertexToMap[edge.first] = edge.second;
                            weldTargetVertices.insert(edge.second);
                            processedFaces.insert(i);
                            processedFaces.insert(oppositeFaceIndex);
                            break;
                        }
                    }
                }
            }
        }
    }
    int weldedCount = 0;
    int faceCountAfterWeld = 0;
    std::map<int, int> oldToNewVerticesMap;
    for (int i = 0; i < (int)sourceTriangles.size(); i++) {
        const auto &faceIndices = sourceTriangles[i];
        std::vector<int> mappedFaceIndices;
        bool errored = false;
        for (const auto &index: faceIndices) {
            int finalIndex = index;
            int mapTimes = 0;
            while (mapTimes < 500) {
                auto findMapResult = weldVertexToMap.find(finalIndex);
                if (findMapResult == weldVertexToMap.end())
                    break;
                finalIndex = findMapResult->second;
                mapTimes++;
            }
            if (mapTimes >= 500) {
                errored = true;
                break;
            }
            mappedFaceIndices.push_back(finalIndex);
        }
        if (errored || mappedFaceIndices.size() < 3)
            continue;
        bool welded = false;
        for (decltype(mappedFaceIndices.size()) j = 0; j < mappedFaceIndices.size(); j++) {
            int next = (j + 1) % 3;
            if (mappedFaceIndices[j] == mappedFaceIndices[next]) {
                welded = true;
                break;
            }
        }
        if (welded) {
            weldedCount++;
            continue;
        }
        faceCountAfterWeld++;
        std::vector<size_t> newFace;
        for (const auto &index: mappedFaceIndices) {
            auto findMap = oldToNewVerticesMap.find(index);
            if (findMap == oldToNewVerticesMap.end()) {
                size_t newIndex = destVertices.size();
                newFace.push_back(newIndex);
                destVertices.push_back(sourceVertices[index]);
                oldToNewVerticesMap.insert({index, newIndex});
            } else {
                newFace.push_back(findMap->second);
            }
        }
        destTriangles.push_back(newFace);
    }
    return weldedCount;
}

}
