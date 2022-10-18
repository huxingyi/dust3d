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

#include <dust3d/uv/unwrap_uv.h>
#include <dust3d/uv/uv_unwrapper.h>

namespace dust3d {

void unwrapUv(const Object& object,
    std::vector<std::vector<Vector2>>& triangleVertexUvs,
    std::set<int>& seamVertices,
    std::map<Uuid, std::vector<Rectangle>>& uvRects)
{
    const auto& choosenVertices = object.vertices;
    const auto& choosenTriangles = object.triangles;
    const auto& choosenTriangleNormals = object.triangleNormals;
    triangleVertexUvs.resize(choosenTriangles.size(), { Vector2(), Vector2(), Vector2() });

    if (nullptr == object.triangleSourceNodes())
        return;

    const std::vector<std::pair<Uuid, Uuid>>& triangleSourceNodes = *object.triangleSourceNodes();

    uv::Mesh inputMesh;
    for (const auto& vertex : choosenVertices) {
        uv::Vertex v;
        v.xyz[0] = vertex.x();
        v.xyz[1] = vertex.y();
        v.xyz[2] = vertex.z();
        inputMesh.vertices.push_back(v);
    }
    std::map<Uuid, int> partIdToPartitionMap;
    std::vector<Uuid> partitionPartUuids;
    for (decltype(choosenTriangles.size()) i = 0; i < choosenTriangles.size(); ++i) {
        const auto& triangle = choosenTriangles[i];
        const auto& sourceNode = triangleSourceNodes[i];
        const auto& normal = choosenTriangleNormals[i];
        uv::Face f;
        f.indices[0] = triangle[0];
        f.indices[1] = triangle[1];
        f.indices[2] = triangle[2];
        inputMesh.faces.push_back(f);
        uv::Vector3 n;
        n.xyz[0] = normal.x();
        n.xyz[1] = normal.y();
        n.xyz[2] = normal.z();
        inputMesh.faceNormals.push_back(n);
        auto findPartitionResult = partIdToPartitionMap.find(sourceNode.first);
        if (findPartitionResult == partIdToPartitionMap.end()) {
            partitionPartUuids.push_back(sourceNode.first);
            partIdToPartitionMap.insert({ sourceNode.first, (int)partitionPartUuids.size() });
            inputMesh.facePartitions.push_back((int)partitionPartUuids.size());
        } else {
            inputMesh.facePartitions.push_back(findPartitionResult->second);
        }
    }

    uv::UvUnwrapper uvUnwrapper;
    uvUnwrapper.setMesh(inputMesh);
    uvUnwrapper.unwrap();
    const std::vector<uv::FaceTextureCoords>& resultFaceUvs = uvUnwrapper.getFaceUvs();
    const std::vector<uv::Rect>& resultChartRects = uvUnwrapper.getChartRects();
    const std::vector<int>& resultChartSourcePartitions = uvUnwrapper.getChartSourcePartitions();
    std::map<int, Vector2> vertexUvMap;
    for (decltype(choosenTriangles.size()) i = 0; i < choosenTriangles.size(); ++i) {
        const auto& triangle = choosenTriangles[i];
        const auto& src = resultFaceUvs[i];
        auto& dest = triangleVertexUvs[i];
        for (size_t j = 0; j < 3; ++j) {
            Vector2 uvCoord = Vector2(src.coords[j].uv[0], src.coords[j].uv[1]);
            dest[j][0] = uvCoord.x();
            dest[j][1] = uvCoord.y();
            int vertexIndex = triangle[j];
            auto findVertexUvResult = vertexUvMap.find(vertexIndex);
            if (findVertexUvResult == vertexUvMap.end()) {
                vertexUvMap.insert({ vertexIndex, uvCoord });
            } else {
                if (findVertexUvResult->second != uvCoord) {
                    seamVertices.insert(vertexIndex);
                }
            }
        }
    }
    for (size_t i = 0; i < resultChartRects.size(); ++i) {
        const auto& rect = resultChartRects[i];
        const auto& source = resultChartSourcePartitions[i];
        if (0 == source || source > (int)partitionPartUuids.size()) {
            continue;
        }
        uvRects[partitionPartUuids[source - 1]].push_back({ rect.left, rect.top, rect.width, rect.height });
    }
}

}
