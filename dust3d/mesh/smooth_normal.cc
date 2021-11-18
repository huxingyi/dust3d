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
#include <dust3d/mesh/smooth_normal.h>

namespace dust3d
{
    
void smoothNormal(const std::vector<Vector3> &vertices,
    const std::vector<std::vector<size_t>> &triangles,
    const std::vector<Vector3> &triangleNormals,
    float thresholdAngleDegrees,
    std::vector<Vector3> &triangleVertexNormals)
{
    std::vector<std::vector<std::pair<size_t, size_t>>> triangleVertexNormalsMapByIndices(vertices.size());
    std::vector<Vector3> angleAreaWeightedNormals;
    for (size_t triangleIndex = 0; triangleIndex < triangles.size(); ++triangleIndex) {
        const auto &sourceTriangle = triangles[triangleIndex];
        if (sourceTriangle.size() != 3) {
            continue;
        }
        const auto &v1 = vertices[sourceTriangle[0]];
        const auto &v2 = vertices[sourceTriangle[1]];
        const auto &v3 = vertices[sourceTriangle[2]];
        float area = Vector3::area(v1, v2, v3);
        float angles[] = {(float)Math::radiansToDegrees(Vector3::angleBetween(v2-v1, v3-v1)),
            (float)Math::radiansToDegrees(Vector3::angleBetween(v1-v2, v3-v2)),
            (float)Math::radiansToDegrees(Vector3::angleBetween(v1-v3, v2-v3))};
        for (int i = 0; i < 3; ++i) {
            if (sourceTriangle[i] >= vertices.size()) {
                continue;
            }
            triangleVertexNormalsMapByIndices[sourceTriangle[i]].push_back({triangleIndex, angleAreaWeightedNormals.size()});
            angleAreaWeightedNormals.push_back(triangleNormals[triangleIndex] * area * angles[i]);
        }
    }
    triangleVertexNormals = angleAreaWeightedNormals;
    std::map<std::pair<size_t, size_t>, float> degreesBetweenFacesMap;
    for (size_t vertexIndex = 0; vertexIndex < vertices.size(); ++vertexIndex) {
        const auto &triangleVertices = triangleVertexNormalsMapByIndices[vertexIndex];
        for (const auto &triangleVertex: triangleVertices) {
            for (const auto &otherTriangleVertex: triangleVertices) {
                if (triangleVertex.first == otherTriangleVertex.first)
                    continue;
                float degrees = 0;
                auto findDegreesResult = degreesBetweenFacesMap.find({triangleVertex.first, otherTriangleVertex.first});
                if (findDegreesResult == degreesBetweenFacesMap.end()) {
                    degrees = Math::radiansToDegrees(Vector3::angleBetween(triangleNormals[triangleVertex.first], triangleNormals[otherTriangleVertex.first]));
                    degreesBetweenFacesMap.insert({{triangleVertex.first, otherTriangleVertex.first}, degrees});
                    degreesBetweenFacesMap.insert({{otherTriangleVertex.first, triangleVertex.first}, degrees});
                } else {
                    degrees = findDegreesResult->second;
                }
                if (degrees > thresholdAngleDegrees) {
                    continue;
                }
                triangleVertexNormals[triangleVertex.second] += angleAreaWeightedNormals[otherTriangleVertex.second];
            }
        }
    }
    for (auto &item: triangleVertexNormals)
        item.normalize();
}

}
