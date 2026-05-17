/*
 *  Copyright (c) 2016-2026 Jeremy HU <jeremy-at-dust3d dot org>. All rights reserved. 
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

#ifndef DUST3D_MESH_STITCH_LOOP_MESH_BUILDER_H_
#define DUST3D_MESH_STITCH_LOOP_MESH_BUILDER_H_

#include <array>
#include <dust3d/base/position_key.h>
#include <dust3d/base/uuid.h>
#include <dust3d/base/vector2.h>
#include <dust3d/base/vector3.h>
#include <dust3d/mesh/mesh_node.h>
#include <map>
#include <set>
#include <vector>

namespace dust3d {

// StitchLoopMeshBuilder builds a quad mesh from a collection of edge loops.
//
// Algorithm overview:
//  1. Collect edge loops from splines. Closed loops are used as-is. Open loops
//     are assumed to be half of a mirrored shape: the end nodes stay at their
//     original position (on the mirror plane or at the tips) while the interior
//     nodes are duplicated and reflected across the X-axis so that both halves
//     share the full loop.
//  2. All edge loops are projected into 2D using (x, y) coordinates and
//     re-meshed as an evenly spaced quad grid. The 3D positions are then
//     recovered by interpolating back from the surrounding loops.
class StitchLoopMeshBuilder {
public:
    struct Loop {
        std::vector<MeshNode> nodes;
        Uuid sourceId;
        bool closed = false; // true when the input spline forms a closed ring
        bool fillInterior = false; // fill the loop interior with triangles when isolated
    };

    StitchLoopMeshBuilder(std::vector<Loop>&& loops, size_t targetSegments);
    void setBackClosed(bool backClosed);
    void build();

    const std::vector<Vector3>& generatedVertices() const;
    const std::vector<Uuid>& generatedVertexSources() const;
    const std::vector<Uuid>& generatedVertexLoopSourceIds() const;
    const std::vector<std::vector<size_t>>& generatedFaces() const;
    const std::vector<std::vector<Vector2>>& generatedFaceUvs() const;
    const std::vector<Loop>& loops() const;
    std::map<Uuid, std::map<std::array<PositionKey, 3>, std::array<Vector2, 3>>> buildPerLoopTriangleUvs() const;

private:
    struct CellColor {
        size_t loopIndex = SIZE_MAX;
        size_t nodeIndex = SIZE_MAX;
        bool valid() const { return loopIndex != SIZE_MAX; }
    };
    using NodeKey = std::pair<size_t, size_t>; // (loopIndex, nodeIndex)

    std::vector<Loop> m_loops;
    size_t m_targetSegments = 0;

    bool m_xMirrored = true;
    bool m_backClosed = false;

    std::vector<Vector3> m_generatedVertices;
    std::vector<Uuid> m_generatedVertexSources;
    std::vector<Uuid> m_generatedVertexLoopSourceIds;
    std::vector<std::vector<size_t>> m_generatedFaces;
    std::vector<std::vector<Vector2>> m_generatedFaceUvs;

    // Intermediate build state shared across steps.
    double m_targetEdgeLength = 0.0;
    double m_targetGridCellLength = 0.0;
    double m_minX = 0.0, m_minY = 0.0, m_squareSide = 0.0;
    size_t m_gridCols = 0, m_gridRows = 0;
    std::vector<CellColor> m_cellColor;
    std::vector<std::vector<size_t>> m_loopNodeVertex;
    std::map<NodeKey, std::set<NodeKey>> m_nodeAdjacency;

    // Per-step build helpers.
    void correctLoopWindings();
    void snapOpenLoopEndNodesToMiddle();
    void mirrorOpenLoops();
    void mirrorClosedLoops();
    bool calculateTargetEdgeLength();
    void calculateBoundingSquare();
    bool buildQuadGrid();
    void assignLoopNodesToCells(std::vector<size_t>& seedCells);
    void bfsColorCells(std::vector<size_t>& seedCells);
    void buildNodeAdjacency();
    void buildFaces();
    void fillIsolatedLoops();
    void closeOuterBoundary();
    void debugVisualizeCellColors();
};

}

#endif
