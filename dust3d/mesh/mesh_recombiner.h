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

#ifndef DUST3D_MESH_MESH_RECOMBINER_H_
#define DUST3D_MESH_MESH_RECOMBINER_H_

#include <dust3d/base/vector2.h>
#include <dust3d/mesh/mesh_combiner.h>
#include <map>
#include <set>
#include <vector>

namespace dust3d {

class MeshRecombiner {
public:
    void setVertices(const std::vector<Vector3>* vertices,
        const std::vector<std::pair<MeshCombiner::Source, size_t>>* verticesSourceIndices);
    void setFaces(const std::vector<std::vector<size_t>>* faces);
    const std::vector<Vector3>& regeneratedVertices();
    const std::vector<std::pair<MeshCombiner::Source, size_t>>& regeneratedVerticesSourceIndices();
    const std::vector<std::vector<size_t>>& regeneratedFaces();
    const std::vector<std::vector<std::pair<std::array<Vector3, 3>, std::array<Vector2, 3>>>>& generatedBridgingTriangleUvs();
    bool recombine();

private:
    const std::vector<Vector3>* m_vertices = nullptr;
    const std::vector<std::pair<MeshCombiner::Source, size_t>>* m_verticesSourceIndices = nullptr;
    const std::vector<std::vector<size_t>>* m_faces = nullptr;
    std::vector<Vector3> m_regeneratedVertices;
    std::vector<std::pair<MeshCombiner::Source, size_t>> m_regeneratedVerticesSourceIndices;
    std::vector<std::vector<size_t>> m_regeneratedFaces;
    std::map<std::pair<size_t, size_t>, size_t> m_halfEdgeToFaceMap;
    std::map<size_t, size_t> m_facesInSeamArea;
    std::set<size_t> m_goodSeams;
    std::vector<std::vector<std::pair<std::array<Vector3, 3>, std::array<Vector2, 3>>>> m_generatedBridgingTriangleUvs;

    bool addFaceToHalfEdgeToFaceMap(size_t faceIndex,
        std::map<std::pair<size_t, size_t>, size_t>& halfEdgeToFaceMap);
    bool buildHalfEdgeToFaceMap(std::map<std::pair<size_t, size_t>, size_t>& halfEdgeToFaceMap);
    bool convertHalfEdgesToEdgeLoops(const std::vector<std::pair<size_t, size_t>>& halfEdges,
        std::vector<std::vector<size_t>>* edgeLoops);
    size_t splitSeamVerticesToIslands(const std::map<size_t, std::vector<size_t>>& seamEdges,
        std::map<size_t, size_t>* vertexToIslandMap);
    void copyNonSeamFacesAsRegenerated();
    size_t adjustTrianglesFromSeam(std::vector<size_t>& edgeLoop, size_t seamIndex);
    size_t otherVertexOfTriangle(const std::vector<size_t>& face, const std::vector<size_t>& indices);
    bool bridge(const std::vector<size_t>& first, const std::vector<size_t>& second);
    size_t nearestIndex(const Vector3& position, const std::vector<size_t>& edgeLoop);
    void removeReluctantVertices();
    void fillPairs(const std::vector<size_t>& small, const std::vector<size_t>& large);
    void updateEdgeLoopNeighborVertices(const std::vector<size_t>& edgeLoop);
};

}

#endif
