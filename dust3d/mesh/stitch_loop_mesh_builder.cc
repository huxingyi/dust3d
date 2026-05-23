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

#include <array>
#include <cmath>
#include <dust3d/base/debug.h>
#include <dust3d/mesh/stitch_loop_mesh_builder.h>
#include <fstream>
#include <functional>
#include <limits>
#include <map>
#include <mapbox/earcut.hpp>
#include <queue>
#include <set>

namespace dust3d {

StitchLoopMeshBuilder::StitchLoopMeshBuilder(std::vector<Loop>&& loops, size_t targetSegments)
    : m_loops(std::move(loops))
    , m_targetSegments(targetSegments)
{
}

void StitchLoopMeshBuilder::setBackClosed(bool backClosed)
{
    m_backClosed = backClosed;
}

void StitchLoopMeshBuilder::setBackCloseDepthRatio(float depthRatio)
{
    m_backCloseDepthRatio = depthRatio;
}

void StitchLoopMeshBuilder::setBackCloseSharpness(float sharpness)
{
    m_backCloseSharpness = sharpness;
}

const std::vector<Vector3>& StitchLoopMeshBuilder::generatedVertices() const
{
    return m_generatedVertices;
}

const std::vector<Uuid>& StitchLoopMeshBuilder::generatedVertexSources() const
{
    return m_generatedVertexSources;
}

const std::vector<Uuid>& StitchLoopMeshBuilder::generatedVertexLoopSourceIds() const
{
    return m_generatedVertexLoopSourceIds;
}

const std::vector<std::vector<size_t>>& StitchLoopMeshBuilder::generatedFaces() const
{
    return m_generatedFaces;
}

const std::vector<std::vector<Vector2>>& StitchLoopMeshBuilder::generatedFaceUvs() const
{
    return m_generatedFaceUvs;
}

const std::vector<StitchLoopMeshBuilder::Loop>& StitchLoopMeshBuilder::loops() const
{
    return m_loops;
}

void StitchLoopMeshBuilder::build()
{
    if (m_loops.empty())
        return;
    correctLoopWindings();
    if (!calculateTargetEdgeLength())
        return;
    if (m_xMirrored) {
        snapOpenLoopEndNodesToMiddle();
        mirrorClosedLoops();
        mirrorOpenLoops();
    }
    calculateBoundingSquare();
    if (!buildQuadGrid())
        return;
    std::vector<size_t> seedCells;
    assignLoopNodesToCells(seedCells);
    bfsColorCells(seedCells);
    buildNodeAdjacency();
    buildFaces();
    fillIsolatedLoops();
    if (m_backClosed)
        closeOuterBoundary();
#ifndef NDEBUG
    debugVisualizeCellColors();
#endif
}

void StitchLoopMeshBuilder::mirrorClosedLoops()
{
    // For each closed loop, check if any node has a negative x coordinate
    // (i.e. is already on the "other side" of the mirror plane). If so,
    // the loop already spans both halves and must not be duplicated.
    // Otherwise, add a new closed loop whose nodes are the mirror image
    // (x negated) of the original, with the node order reversed so that
    // the winding direction is preserved for the mirrored half.
    size_t originalCount = m_loops.size();
    for (size_t li = 0; li < originalCount; ++li) {
        const auto& loop = m_loops[li];
        if (!loop.closed)
            continue;
        bool hasOtherSide = false;
        for (const auto& node : loop.nodes) {
            if (node.origin.x() < 0.0) {
                hasOtherSide = true;
                break;
            }
        }
        bool hasThisSide = false;
        for (const auto& node : loop.nodes) {
            if (node.origin.x() > 0.0) {
                hasThisSide = true;
                break;
            }
        }
        if (hasOtherSide && hasThisSide)
            continue;
        Loop mirrored;
        mirrored.sourceId = loop.sourceId;
        mirrored.closed = true;
        mirrored.fillInterior = loop.fillInterior;
        mirrored.nodes.reserve(loop.nodes.size());
        for (auto it = loop.nodes.rbegin(); it != loop.nodes.rend(); ++it) {
            MeshNode mn = *it;
            mn.origin.setX(-mn.origin.x());
            mirrored.nodes.push_back(mn);
        }
        m_loops.push_back(std::move(mirrored));
    }
}

void StitchLoopMeshBuilder::mirrorOpenLoops()
{
    // Three mirroring cases based on whether the end nodes lie on the mirror plane
    // (|x| <= m_targetGridCellLength):
    //
    //  Case 1 – both ends in middle: A-B-C-D → A-B-C-D-C'-B'-A (close loop).
    //           Interior nodes are mirrored; the shared end nodes are the pivot points.
    //
    //  Case 2 – neither end in middle: the original open loop is kept as-is and a
    //           new separate mirrored open loop D'-C'-B'-A' is appended.
    //
    //  Case 3 – exactly one end in middle: normalize so the middle node is last,
    //           then mirror all non-pivot nodes in reverse and append:
    //           A-B-C-D(middle) → A-B-C-D-C'-B'-A' (open loop, A' ≠ A).
    auto isMiddle = [&](const MeshNode& node) {
        return std::abs(node.origin.x()) <= m_targetGridCellLength;
    };

    size_t originalCount = m_loops.size();
    for (size_t li = 0; li < originalCount; ++li) {
        auto& loop = m_loops[li];
        if (loop.closed || loop.nodes.size() < 2)
            continue;

        bool firstInMiddle = isMiddle(loop.nodes.front());
        bool lastInMiddle = isMiddle(loop.nodes.back());

        if (firstInMiddle && lastInMiddle) {
            // Case 1: both ends are pivots — mirror interior nodes only.
            size_t mirrorEnd = loop.nodes.size() - 1;
            size_t mirrorStart = 1;
            for (size_t i = mirrorEnd - 1; i >= mirrorStart && i < loop.nodes.size(); --i) {
                MeshNode mn = loop.nodes[i];
                mn.origin.setX(-mn.origin.x());
                loop.nodes.push_back(mn);
                if (i == 0)
                    break;
            }
            loop.closed = true;
        } else if (!firstInMiddle && !lastInMiddle) {
            // Case 2: no end is a pivot — add a new reversed-mirrored open loop D'-C'-B'-A'.
            Loop mirrored;
            mirrored.sourceId = loop.sourceId;
            mirrored.closed = false;
            mirrored.nodes.reserve(loop.nodes.size());
            for (auto it = loop.nodes.rbegin(); it != loop.nodes.rend(); ++it) {
                MeshNode mn = *it;
                mn.origin.setX(-mn.origin.x());
                mirrored.nodes.push_back(mn);
            }
            m_loops.push_back(std::move(mirrored));
        } else {
            // Case 3: exactly one end is a pivot.
            // Normalize so the pivot is the last node.
            if (firstInMiddle && !lastInMiddle)
                std::reverse(loop.nodes.begin(), loop.nodes.end());
            // Mirror all nodes except the last (pivot) in reverse and append.
            // e.g. A-B-C-D(pivot) → A-B-C-D-C'-B'-A'
            size_t n = loop.nodes.size();
            for (size_t i = n - 2;; --i) {
                MeshNode mn = loop.nodes[i];
                mn.origin.setX(-mn.origin.x());
                loop.nodes.push_back(mn);
                if (i == 0)
                    break;
            }
            // loop remains open: starts at A, ends at A' (mirror of A)
        }
    }
}

void StitchLoopMeshBuilder::snapOpenLoopEndNodesToMiddle()
{
    // For each open loop, check if either end node is within one targetEdgeLength
    // of x=0 (the mirror plane). If so, snap its x coordinate to 0 so that
    // mirrored halves share a clean seam.
    for (auto& loop : m_loops) {
        if (loop.closed || loop.nodes.empty())
            continue;
        auto snapIfNear = [&](MeshNode& node) {
            if (std::abs(node.origin.x()) <= m_targetGridCellLength)
                node.origin.setX(0.0);
        };
        snapIfNear(loop.nodes.front());
        snapIfNear(loop.nodes.back());
    }
}

void StitchLoopMeshBuilder::correctLoopWindings()
{
    // Ensure every closed loop is wound CCW in the XY plane (exterior half-edge direction).
    // Compute the signed area via the shoelace formula; if negative the loop is CW, so reverse it.
    for (auto& loop : m_loops) {
        if (!loop.closed || loop.nodes.size() < 3)
            continue;
        double signedArea = 0.0;
        size_t n = loop.nodes.size();
        for (size_t i = 0; i < n; ++i) {
            const auto& a = loop.nodes[i].origin;
            const auto& b = loop.nodes[(i + 1) % n].origin;
            signedArea += (a.x() * b.y()) - (b.x() * a.y());
        }
        if (signedArea < 0.0)
            std::reverse(loop.nodes.begin(), loop.nodes.end());
    }
}

bool StitchLoopMeshBuilder::calculateTargetEdgeLength()
{
    // Calculate average edge length of all loops in XY 2D plane,
    // and determine the target 2d edge length for remeshing.
    double totalEdgeLength = 0.0;
    size_t totalEdgeCount = 0;
    for (const auto& loop : m_loops) {
        size_t n = loop.nodes.size();
        if (n < 2)
            continue;
        size_t edgeEnd = loop.closed ? n : n - 1;
        for (size_t i = 0; i < edgeEnd; ++i) {
            const auto& a = loop.nodes[i].origin;
            const auto& b = loop.nodes[(i + 1) % n].origin;
            double dx = b.x() - a.x();
            double dy = b.y() - a.y();
            totalEdgeLength += std::sqrt(dx * dx + dy * dy);
            ++totalEdgeCount;
        }
    }
    if (totalEdgeCount == 0)
        return false;
    m_targetEdgeLength = (totalEdgeLength / static_cast<double>(totalEdgeCount));
    double rawCellLength = m_targetEdgeLength * 0.1;
    // Snap to the first level >= rawCellLength so that small variations in edge
    // length do not cause a different grid resolution. For values larger than
    // the last fixed level, round up to the nearest 0.001 step instead of
    // falling back to the raw float (which would be unquantized).
    m_targetGridCellLength = std::ceil(rawCellLength / 0.001) * 0.001;
    return true;
}

void StitchLoopMeshBuilder::calculateBoundingSquare()
{
    // Get a bounding square to contain all loops in the XY 2D plane.
    double minX = std::numeric_limits<double>::max();
    double minY = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    double maxY = std::numeric_limits<double>::lowest();
    for (const auto& loop : m_loops) {
        for (const auto& node : loop.nodes) {
            minX = std::min(minX, node.origin.x());
            minY = std::min(minY, node.origin.y());
            maxX = std::max(maxX, node.origin.x());
            maxY = std::max(maxY, node.origin.y());
        }
    }
    // Expand by one cell margin so loops near the edge are fully enclosed.
    m_minX = minX - m_targetGridCellLength;
    m_minY = minY - m_targetGridCellLength;
    maxX += m_targetGridCellLength;
    maxY += m_targetGridCellLength;
    m_squareSide = std::max(maxX - m_minX, maxY - m_minY);
}

bool StitchLoopMeshBuilder::buildQuadGrid()
{
    // Use targetEdgeLength to make a regular quad grid for the bounding square.
    m_gridCols = static_cast<size_t>(std::ceil(m_squareSide / m_targetGridCellLength));
    m_gridRows = m_gridCols;
    if (m_gridCols == 0 || m_gridRows == 0)
        return false;
    m_cellColor.assign(m_gridRows * m_gridCols, CellColor {});
    return true;
}

void StitchLoopMeshBuilder::assignLoopNodesToCells(std::vector<size_t>& seedCells)
{
    // For each edge in every loop, densify it at m_targetGridCellLength intervals so
    // that every grid cell the edge passes through gets seeded. Each interpolated
    // point is attributed to whichever original endpoint is nearer, so the cell's
    // stored (loopIndex, nodeIndex) always refers to a real loop node.
    // The interpolated points are temporary and are not stored anywhere else.
    auto seedCell = [&](double px, double py, size_t li, size_t ni) {
        if (px < m_minX || py < m_minY || px > m_minX + m_squareSide || py > m_minY + m_squareSide)
            return;
        int col = static_cast<int>((px - m_minX) / m_targetGridCellLength);
        int row = static_cast<int>((py - m_minY) / m_targetGridCellLength);
        col = std::max(0, std::min(col, static_cast<int>(m_gridCols) - 1));
        row = std::max(0, std::min(row, static_cast<int>(m_gridRows) - 1));
        size_t cellIdx = static_cast<size_t>(row) * m_gridCols + static_cast<size_t>(col);
        if (!m_cellColor[cellIdx].valid()) {
            m_cellColor[cellIdx] = { li, ni };
            seedCells.push_back(cellIdx);
        }
    };

    for (size_t li = 0; li < m_loops.size(); ++li) {
        const auto& loop = m_loops[li];
        size_t n = loop.nodes.size();
        if (n == 0)
            continue;
        size_t edgeEnd = loop.closed ? n : n - 1;
        for (size_t ni = 0; ni < edgeEnd; ++ni) {
            size_t nextNi = (ni + 1) % n;
            double ax = loop.nodes[ni].origin.x();
            double ay = loop.nodes[ni].origin.y();
            double bx = loop.nodes[nextNi].origin.x();
            double by = loop.nodes[nextNi].origin.y();
            double dx = bx - ax;
            double dy = by - ay;
            double edgeLen = std::sqrt(dx * dx + dy * dy);
            // Seed the start node itself.
            seedCell(ax, ay, li, ni);
            // Interpolate along the edge at m_targetGridCellLength steps.
            if (edgeLen > 0.0) {
                size_t steps = static_cast<size_t>(std::ceil(edgeLen / m_targetGridCellLength));
                for (size_t s = 1; s < steps; ++s) {
                    double t = static_cast<double>(s) / static_cast<double>(steps);
                    double px = ax + t * dx;
                    double py = ay + t * dy;
                    // Attribute to nearer endpoint.
                    size_t attrNi = (t <= 0.5) ? ni : nextNi;
                    seedCell(px, py, li, attrNi);
                }
            }
        }
        // Seed the last node of open loops (closed loops wrap back to ni=0).
        if (!loop.closed)
            seedCell(loop.nodes[n - 1].origin.x(), loop.nodes[n - 1].origin.y(), li, n - 1);
    }
}

void StitchLoopMeshBuilder::bfsColorCells(std::vector<size_t>& seedCells)
{
    // BFS from seed cells to color every cell with the nearest loop node.
    std::queue<size_t> bfsQueue;
    for (size_t s : seedCells)
        bfsQueue.push(s);
    const int dr[] = { -1, 0, 1, 0 };
    const int dc[] = { 0, 1, 0, -1 };
    while (!bfsQueue.empty()) {
        size_t ci = bfsQueue.front();
        bfsQueue.pop();
        size_t r = ci / m_gridCols;
        size_t c = ci % m_gridCols;
        for (int d = 0; d < 4; ++d) {
            int nr = static_cast<int>(r) + dr[d];
            int nc = static_cast<int>(c) + dc[d];
            if (nr < 0 || nr >= static_cast<int>(m_gridRows) || nc < 0 || nc >= static_cast<int>(m_gridCols))
                continue;
            size_t neighborIdx = static_cast<size_t>(nr) * m_gridCols + static_cast<size_t>(nc);
            if (!m_cellColor[neighborIdx].valid()) {
                m_cellColor[neighborIdx] = m_cellColor[ci];
                bfsQueue.push(neighborIdx);
            }
        }
    }
}

void StitchLoopMeshBuilder::buildFaces()
{
    // Map (loopIndex, nodeIndex) -> index in m_generatedVertices.
    m_loopNodeVertex.resize(m_loops.size());
    for (size_t li = 0; li < m_loops.size(); ++li) {
        m_loopNodeVertex[li].resize(m_loops[li].nodes.size());
        for (size_t ni = 0; ni < m_loops[li].nodes.size(); ++ni) {
            m_loopNodeVertex[li][ni] = m_generatedVertices.size();
            m_generatedVertices.push_back(m_loops[li].nodes[ni].origin);
            m_generatedVertexSources.push_back(m_loops[li].nodes[ni].sourceId);
            m_generatedVertexLoopSourceIds.push_back(m_loops[li].sourceId);
        }
    }

    // Reverse map from vertex index to its loop index. Used below to group
    // candidate triangles by the loops they span (locality key).
    std::vector<size_t> vertexLoop(m_generatedVertices.size(), SIZE_MAX);
    for (size_t li = 0; li < m_loops.size(); ++li)
        for (size_t ni = 0; ni < m_loops[li].nodes.size(); ++ni)
            vertexLoop[m_loopNodeVertex[li][ni]] = li;

    // Build a unified adjacency set that includes both same-loop consecutive edges
    // and cross-loop edges from m_nodeAdjacency.
    std::map<NodeKey, std::set<NodeKey>> allNeighbors;
    for (size_t li = 0; li < m_loops.size(); ++li) {
        const auto& loop = m_loops[li];
        size_t n = loop.nodes.size();
        if (n < 2)
            continue;
        size_t edgeEnd = loop.closed ? n : n - 1;
        for (size_t ni = 0; ni < edgeEnd; ++ni) {
            NodeKey a = { li, ni };
            NodeKey b = { li, (ni + 1) % n };
            allNeighbors[a].insert(b);
            allNeighbors[b].insert(a);
        }
    }
    for (const auto& [node, neighbors] : m_nodeAdjacency) {
        for (const auto& nb : neighbors)
            allNeighbors[node].insert(nb);
    }

    // Collect all candidate triangles from angle-sorted neighbor fans, then emit
    // them in quality order (smallest area first) so that compact triangles win
    // half-edge claims over large slivers, reducing gaps.
    using HalfEdge = std::pair<size_t, size_t>;

    auto nodePos = [&](const NodeKey& k) -> const Vector3& {
        return m_loops[k.first].nodes[k.second].origin;
    };

    struct CandidateTriangle {
        size_t v0, v1, v2;
        double score;
    };
    std::vector<CandidateTriangle> candidates;

    for (const auto& [node, neighborSet] : allNeighbors) {
        if (neighborSet.size() < 2)
            continue;
        double cx = nodePos(node).x();
        double cy = nodePos(node).y();

        std::vector<std::pair<double, NodeKey>> byAngle;
        byAngle.reserve(neighborSet.size());
        for (const auto& nb : neighborSet) {
            double dx = nodePos(nb).x() - cx;
            double dy = nodePos(nb).y() - cy;
            byAngle.push_back({ std::atan2(dy, dx), nb });
        }
        std::sort(byAngle.begin(), byAngle.end());

        size_t m = byAngle.size();
        for (size_t i = 0; i < m; ++i) {
            const NodeKey& n1 = byAngle[i].second;
            const NodeKey& n2 = byAngle[(i + 1) % m].second;

            size_t v0 = m_loopNodeVertex[node.first][node.second];
            size_t v1 = m_loopNodeVertex[n1.first][n1.second];
            size_t v2 = m_loopNodeVertex[n2.first][n2.second];
            if (v0 == v1 || v1 == v2 || v0 == v2)
                continue;

            if (node.first == n1.first && node.first == n2.first)
                continue;

            // Normalize vertex order so duplicates from different fans are detected.
            size_t tri[3] = { v0, v1, v2 };
            size_t minIdx = 0;
            if (tri[1] < tri[minIdx])
                minIdx = 1;
            if (tri[2] < tri[minIdx])
                minIdx = 2;
            size_t sv0 = tri[minIdx];
            size_t sv1 = tri[(minIdx + 1) % 3];
            size_t sv2 = tri[(minIdx + 2) % 3];

            // Score: prefer compact, well-shaped triangles.
            // Use the ratio of shortest edge to longest edge (higher = more equilateral).
            const Vector3& p0 = m_generatedVertices[sv0];
            const Vector3& p1 = m_generatedVertices[sv1];
            const Vector3& p2 = m_generatedVertices[sv2];
            double e0x = p1.x() - p0.x(), e0y = p1.y() - p0.y();
            double e1x = p2.x() - p1.x(), e1y = p2.y() - p1.y();
            double e2x = p0.x() - p2.x(), e2y = p0.y() - p2.y();
            double l0 = e0x * e0x + e0y * e0y;
            double l1 = e1x * e1x + e1y * e1y;
            double l2 = e2x * e2x + e2y * e2y;
            double maxL = std::max({ l0, l1, l2 });
            double minL = std::min({ l0, l1, l2 });
            double score = maxL > 0.0 ? minL / maxL : 0.0;

            candidates.push_back({ sv0, sv1, sv2, score });
        }
    }

    // Deduplicate candidates (same triangle from different node fans).
    std::sort(candidates.begin(), candidates.end(), [](const CandidateTriangle& a, const CandidateTriangle& b) {
        if (a.v0 != b.v0)
            return a.v0 < b.v0;
        if (a.v1 != b.v1)
            return a.v1 < b.v1;
        return a.v2 < b.v2;
    });
    candidates.erase(std::unique(candidates.begin(), candidates.end(),
                         [](const CandidateTriangle& a, const CandidateTriangle& b) {
                             return a.v0 == b.v0 && a.v1 == b.v1 && a.v2 == b.v2;
                         }),
        candidates.end());

    // Sort candidates for locality and determinism:
    //   Primary key:   sorted triple of loop indices involved in the triangle.
    //                  Triangles between loops {A,B} never share half-edges with
    //                  triangles between loops {C,D} (disjoint vertex sets), so
    //                  processing them in separate groups means a position change
    //                  in loop L only re-orders triangles that involve L — triangles
    //                  between other loops are completely unaffected.
    //   Secondary key: quality score rounded down to 0.05 steps, descending.
    //                  Discretization prevents tiny floating-point jitter from
    //                  flipping the relative order of nearly-equal triangles.
    //   Tertiary key:  vertex indices ascending — guarantees a strict total order
    //                  so the sort result is identical across runs and platforms.
    constexpr double kScoreStep = 0.05;
    std::sort(candidates.begin(), candidates.end(), [&](const CandidateTriangle& a, const CandidateTriangle& b) {
        std::array<size_t, 3> ka = { vertexLoop[a.v0], vertexLoop[a.v1], vertexLoop[a.v2] };
        std::array<size_t, 3> kb = { vertexLoop[b.v0], vertexLoop[b.v1], vertexLoop[b.v2] };
        std::sort(ka.begin(), ka.end());
        std::sort(kb.begin(), kb.end());
        if (ka != kb)
            return ka < kb;
        double sa = std::floor(a.score / kScoreStep) * kScoreStep;
        double sb = std::floor(b.score / kScoreStep) * kScoreStep;
        if (sa != sb)
            return sa > sb;
        if (a.v0 != b.v0)
            return a.v0 < b.v0;
        if (a.v1 != b.v1)
            return a.v1 < b.v1;
        return a.v2 < b.v2;
    });

    // Emit triangles greedily in quality order.
    std::set<HalfEdge> usedHalfEdges;
    for (const auto& tri : candidates) {
        if (usedHalfEdges.count({ tri.v0, tri.v1 }) || usedHalfEdges.count({ tri.v1, tri.v2 }) || usedHalfEdges.count({ tri.v2, tri.v0 }))
            continue;

        usedHalfEdges.insert({ tri.v0, tri.v1 });
        usedHalfEdges.insert({ tri.v1, tri.v2 });
        usedHalfEdges.insert({ tri.v2, tri.v0 });
        m_generatedFaces.push_back({ tri.v0, tri.v1, tri.v2 });
    }

    // Fill small holes using earcut.
    {
        constexpr size_t maxHoleSize = 8;

        std::map<size_t, std::vector<size_t>> boundaryAdj;
        for (const auto& he : usedHalfEdges) {
            if (!usedHalfEdges.count({ he.second, he.first }))
                boundaryAdj[he.first].push_back(he.second);
        }

        size_t totalBoundaryEdges = 0;
        for (const auto& [v, nexts] : boundaryAdj)
            totalBoundaryEdges += nexts.size();
        dust3dDebug << "buildFaces: boundary edges=" << totalBoundaryEdges
                    << " total half-edges=" << usedHalfEdges.size()
                    << " faces=" << m_generatedFaces.size();

        // Find all closed boundary loops using DFS on the boundary adjacency.
        std::set<std::pair<size_t, size_t>> visitedEdges;
        std::vector<std::vector<size_t>> holeLoops;
        size_t skippedLarge = 0;

        for (const auto& [start, nexts] : boundaryAdj) {
            for (size_t next : nexts) {
                if (visitedEdges.count({ start, next }))
                    continue;
                std::vector<size_t> hole;
                hole.push_back(start);
                visitedEdges.insert({ start, next });
                size_t cur = next;
                bool valid = true;
                while (cur != start) {
                    hole.push_back(cur);
                    if (hole.size() > maxHoleSize) {
                        valid = false;
                        break;
                    }
                    auto it = boundaryAdj.find(cur);
                    if (it == boundaryAdj.end() || it->second.empty()) {
                        valid = false;
                        break;
                    }
                    // Pick the outgoing edge closest in angle to continuing straight.
                    size_t prev = hole[hole.size() - 2];
                    const Vector3& pPrev = m_generatedVertices[prev];
                    const Vector3& pCur = m_generatedVertices[cur];
                    double inDx = pCur.x() - pPrev.x();
                    double inDy = pCur.y() - pPrev.y();
                    double inAngle = std::atan2(inDy, inDx);
                    size_t bestNext = SIZE_MAX;
                    double bestAngleDiff = std::numeric_limits<double>::max();
                    for (size_t candidate : it->second) {
                        if (visitedEdges.count({ cur, candidate }))
                            continue;
                        const Vector3& pNext = m_generatedVertices[candidate];
                        double outDx = pNext.x() - pCur.x();
                        double outDy = pNext.y() - pCur.y();
                        double outAngle = std::atan2(outDy, outDx);
                        double diff = outAngle - inAngle;
                        if (diff > M_PI)
                            diff -= 2.0 * M_PI;
                        if (diff < -M_PI)
                            diff += 2.0 * M_PI;
                        double absDiff = std::abs(diff);
                        if (absDiff < bestAngleDiff) {
                            bestAngleDiff = absDiff;
                            bestNext = candidate;
                        }
                    }
                    if (bestNext == SIZE_MAX) {
                        valid = false;
                        break;
                    }
                    visitedEdges.insert({ cur, bestNext });
                    cur = bestNext;
                }
                if (!valid) {
                    ++skippedLarge;
                    continue;
                }
                if (hole.size() >= 3)
                    holeLoops.push_back(std::move(hole));
            }
        }

        size_t filledCount = 0;
        for (const auto& hole : holeLoops) {

            // Use earcut to triangulate the hole polygon in XY.
            std::vector<std::vector<std::array<double, 2>>> polygon;
            std::vector<std::array<double, 2>> ring;
            ring.reserve(hole.size());
            for (size_t vi : hole)
                ring.push_back({ m_generatedVertices[vi].x(), m_generatedVertices[vi].y() });
            polygon.push_back(ring);
            std::vector<size_t> indices = mapbox::earcut<size_t>(polygon);

            // Earcut triangles follow the input winding. The boundary chain goes
            // in the USED direction; fill faces need the REVERSE direction. So
            // flip each triangle's winding.
            for (size_t i = 0; i + 2 < indices.size(); i += 3) {
                size_t va = hole[indices[i]];
                size_t vb = hole[indices[i + 1]];
                size_t vc = hole[indices[i + 2]];
                m_generatedFaces.push_back({ vc, vb, va });
            }
            ++filledCount;
            dust3dDebug << "  filled hole: " << hole.size() << " edges";
        }
        dust3dDebug << "buildFaces hole-fill: filled=" << filledCount
                    << " skippedLarge=" << skippedLarge;
    }

    // Generate UV coordinates for every face using planar XY projection.
    // u = (x - m_minX) / m_squareSide,  v = (y - m_minY) / m_squareSide
    m_generatedFaceUvs.resize(m_generatedFaces.size());
    for (size_t fi = 0; fi < m_generatedFaces.size(); ++fi) {
        const auto& face = m_generatedFaces[fi];
        m_generatedFaceUvs[fi].resize(face.size());
        for (size_t vi = 0; vi < face.size(); ++vi) {
            const auto& pos = m_generatedVertices[face[vi]];
            double u = m_squareSide > 0.0 ? (pos.x() - m_minX) / m_squareSide : 0.0;
            double v = m_squareSide > 0.0 ? (pos.y() - m_minY) / m_squareSide : 0.0;
            m_generatedFaceUvs[fi][vi] = Vector2(u, v);
        }
    }
}

void StitchLoopMeshBuilder::fillIsolatedLoops()
{
    // Collect all directed half-edges that appear in existing faces.
    // A closed loop is "isolated" if none of its directed half-edges appear in any face —
    // meaning buildFaces() produced no cross-loop connections for it.
    std::set<std::pair<size_t, size_t>> usedHalfEdges;
    for (const auto& face : m_generatedFaces) {
        size_t fn = face.size();
        for (size_t i = 0; i < fn; ++i)
            usedHalfEdges.insert({ face[i], face[(i + 1) % fn] });
    }

    auto toUv = [&](const Vector3& p) -> Vector2 {
        double u = m_squareSide > 0.0 ? (p.x() - m_minX) / m_squareSide : 0.0;
        double v = m_squareSide > 0.0 ? (p.y() - m_minY) / m_squareSide : 0.0;
        return Vector2(u, v);
    };

    for (size_t li = 0; li < m_loops.size(); ++li) {
        const auto& loop = m_loops[li];
        if (!loop.closed || !loop.fillInterior || loop.nodes.size() < 3)
            continue;

        // Check isolation: none of this loop's directed half-edges appear in any existing face.
        bool isIsolated = true;
        size_t edgeEnd = loop.nodes.size();
        for (size_t ni = 0; ni < edgeEnd; ++ni) {
            size_t va = m_loopNodeVertex[li][ni];
            size_t vb = m_loopNodeVertex[li][(ni + 1) % edgeEnd];
            if (usedHalfEdges.count({ va, vb })) {
                isIsolated = false;
                break;
            }
        }
        if (!isIsolated)
            continue;

        // Fill algorithm:
        // Repeatedly find the nearest pair of non-adjacent vertices in the ring,
        // connect them (splitting the ring into two smaller rings), and recurse
        // until only triangles remain.
        const size_t n = loop.nodes.size();

        // Build the initial ring of vertex indices.
        std::vector<size_t> initRing;
        initRing.reserve(n);
        for (size_t i = 0; i < n; ++i)
            initRing.push_back(m_loopNodeVertex[li][i]);

        // Recursively split the ring by its shortest diagonal, emitting triangles.
        std::function<void(const std::vector<size_t>&)> fillRing =
            [&](const std::vector<size_t>& ring) {
                size_t sz = ring.size();
                if (sz < 3)
                    return;
                if (sz == 3) {
                    m_generatedFaces.push_back({ ring[0], ring[1], ring[2] });
                    m_generatedFaceUvs.push_back({ toUv(m_generatedVertices[ring[0]]),
                        toUv(m_generatedVertices[ring[1]]),
                        toUv(m_generatedVertices[ring[2]]) });
                    return;
                }
                // Find the nearest pair of non-adjacent vertices (diagonal).
                double minDist2 = std::numeric_limits<double>::max();
                size_t bestI = 0, bestJ = 2;
                for (size_t i = 0; i < sz; ++i) {
                    const Vector3& a = m_generatedVertices[ring[i]];
                    for (size_t j = i + 2; j < sz; ++j) {
                        // Skip the wrap-around adjacent pair (i=0, j=sz-1).
                        if (i == 0 && j == sz - 1)
                            continue;
                        const Vector3& b = m_generatedVertices[ring[j]];
                        double dx = b.x() - a.x();
                        double dy = b.y() - a.y();
                        double dz = b.z() - a.z();
                        double d2 = dx * dx + dy * dy + dz * dz;
                        if (d2 < minDist2) {
                            minDist2 = d2;
                            bestI = i;
                            bestJ = j;
                        }
                    }
                }
                // Split ring into two sub-rings at the diagonal bestI–bestJ.
                // Sub-ring A: ring[bestI .. bestJ]
                std::vector<size_t> ringA(ring.begin() + static_cast<ptrdiff_t>(bestI),
                    ring.begin() + static_cast<ptrdiff_t>(bestJ) + 1);
                // Sub-ring B: ring[bestJ .. sz-1] + ring[0 .. bestI]
                std::vector<size_t> ringB;
                ringB.reserve(sz - bestJ + bestI + 1);
                for (size_t k = bestJ; k < sz; ++k)
                    ringB.push_back(ring[k]);
                for (size_t k = 0; k <= bestI; ++k)
                    ringB.push_back(ring[k]);
                fillRing(ringA);
                fillRing(ringB);
            };

        fillRing(initRing);
    }
}

void StitchLoopMeshBuilder::buildNodeAdjacency()
{
    // Collect unique neighbor pairs between nodes of different loops that share a grid edge.
    std::set<std::pair<NodeKey, NodeKey>> neighborPairSet;
    for (size_t r = 0; r < m_gridRows; ++r) {
        for (size_t c = 0; c < m_gridCols; ++c) {
            const CellColor& cc = m_cellColor[r * m_gridCols + c];
            if (!cc.valid())
                continue;
            auto addPair = [&](const CellColor& cn) {
                if (!cn.valid() || cn.loopIndex == cc.loopIndex)
                    return;
                NodeKey a = { cc.loopIndex, cc.nodeIndex };
                NodeKey b = { cn.loopIndex, cn.nodeIndex };
                if (a > b)
                    std::swap(a, b);
                neighborPairSet.insert({ a, b });
            };
            if (c + 1 < m_gridCols)
                addPair(m_cellColor[r * m_gridCols + c + 1]);
            if (r + 1 < m_gridRows)
                addPair(m_cellColor[(r + 1) * m_gridCols + c]);
        }
    }

    // Build adjacency map: node -> set of adjacent nodes from other loops.
    m_nodeAdjacency.clear();
    for (const auto& p : neighborPairSet) {
        m_nodeAdjacency[p.first].insert(p.second);
        m_nodeAdjacency[p.second].insert(p.first);
    }
}

void StitchLoopMeshBuilder::closeOuterBoundary()
{
    // Find boundary half-edges: directed edges used by exactly one face.
    std::map<std::pair<size_t, size_t>, int> halfEdgeCount;
    for (const auto& face : m_generatedFaces) {
        size_t n = face.size();
        for (size_t i = 0; i < n; ++i) {
            size_t a = face[i];
            size_t b = face[(i + 1) % n];
            halfEdgeCount[{ a, b }]++;
        }
    }

    // boundaryNext: for each boundary half-edge a->b, record next[a] = b.
    std::map<size_t, size_t> boundaryNext;
    for (const auto& [he, cnt] : halfEdgeCount) {
        if (halfEdgeCount.count({ he.second, he.first }) == 0)
            boundaryNext[he.first] = he.second;
    }

    if (boundaryNext.empty())
        return;

    // Extract all boundary loops by chaining.
    std::set<size_t> visited;
    std::vector<std::vector<size_t>> boundaryLoops;
    for (auto& [start, _] : boundaryNext) {
        if (visited.count(start))
            continue;
        std::vector<size_t> loop;
        size_t cur = start;
        while (!visited.count(cur) && boundaryNext.count(cur)) {
            visited.insert(cur);
            loop.push_back(cur);
            cur = boundaryNext[cur];
        }
        if (loop.size() >= 3)
            boundaryLoops.push_back(std::move(loop));
    }

    if (boundaryLoops.empty())
        return;

    // Select only the outermost boundary loop — the one with the largest perimeter.
    size_t outerIdx = 0;
    double maxPerimeter = 0.0;
    for (size_t i = 0; i < boundaryLoops.size(); ++i) {
        const auto& loop = boundaryLoops[i];
        size_t ln = loop.size();
        double perimeter = 0.0;
        for (size_t j = 0; j < ln; ++j)
            perimeter += (m_generatedVertices[loop[(j + 1) % ln]] - m_generatedVertices[loop[j]]).length();
        if (perimeter > maxPerimeter) {
            maxPerimeter = perimeter;
            outerIdx = i;
        }
    }

    const auto& outerLoop = boundaryLoops[outerIdx];
    const size_t n = outerLoop.size();

    // Compute centroid of the boundary loop.
    Vector3 centroid;
    for (size_t vi : outerLoop)
        centroid += m_generatedVertices[vi];
    centroid /= static_cast<double>(n);

    // Compute the boundary loop normal via Newell's method.
    Vector3 normal;
    for (size_t i = 0; i < n; ++i) {
        const Vector3& a = m_generatedVertices[outerLoop[i]];
        const Vector3& b = m_generatedVertices[outerLoop[(i + 1) % n]];
        normal += Vector3(
            (a.y() - b.y()) * (a.z() + b.z()),
            (a.z() - b.z()) * (a.x() + b.x()),
            (a.x() - b.x()) * (a.y() + b.y()));
    }
    normal = -normal.normalized();

    // Average radius: how far the boundary verts are from the centroid.
    double avgRadius = 0.0;
    for (size_t vi : outerLoop)
        avgRadius += (m_generatedVertices[vi] - centroid).length();
    avgRadius /= static_cast<double>(n);

    const size_t numRings = std::max(size_t(4), n / 6);
    const double capHeight = avgRadius * static_cast<double>(m_backCloseDepthRatio);
    const double sharpness = std::clamp(static_cast<double>(m_backCloseSharpness), 0.0, 1.0);

    // Compute per-boundary-vertex flow direction from the existing mesh surface.
    // For each boundary vertex, find adjacent interior vertices (from faces that use
    // the boundary edge) and compute the outward surface continuation direction.
    std::set<size_t> boundarySet(outerLoop.begin(), outerLoop.end());
    std::vector<Vector3> flowDirections(n);
    for (size_t i = 0; i < n; ++i) {
        size_t bv = outerLoop[i];
        Vector3 interiorSum;
        int interiorCount = 0;
        for (const auto& face : m_generatedFaces) {
            bool hasBv = false;
            for (size_t vi : face) {
                if (vi == bv) {
                    hasBv = true;
                    break;
                }
            }
            if (!hasBv)
                continue;
            for (size_t vi : face) {
                if (vi != bv && boundarySet.count(vi) == 0) {
                    interiorSum += m_generatedVertices[vi];
                    ++interiorCount;
                }
            }
        }
        if (interiorCount > 0) {
            Vector3 avgInterior = interiorSum / static_cast<double>(interiorCount);
            flowDirections[i] = (m_generatedVertices[bv] - avgInterior).normalized();
        } else {
            flowDirections[i] = normal;
        }
    }

    auto toUv = [&](const Vector3& p) -> Vector2 {
        double u = m_squareSide > 0.0 ? (p.x() - m_minX) / m_squareSide : 0.0;
        double v = m_squareSide > 0.0 ? (p.y() - m_minY) / m_squareSide : 0.0;
        return Vector2(u, v);
    };

    // Build closing rings. Each vertex position is a blend between:
    //   "sharp" path: shrink toward centroid + lift along boundary normal (cos/sin profile)
    //   "smooth" path: continue along per-vertex flow direction, gradually curving to apex
    // sharpness=1 uses the sharp path; sharpness=0 follows surface curvature fully.

    // Collect source IDs from boundary vertices so back-close vertices inherit them.
    std::vector<Uuid> boundaryVertexSources(n);
    std::vector<Uuid> boundaryVertexLoopSources(n);
    for (size_t i = 0; i < n; ++i) {
        size_t vi = outerLoop[i];
        boundaryVertexSources[i] = vi < m_generatedVertexSources.size() ? m_generatedVertexSources[vi] : Uuid();
        boundaryVertexLoopSources[i] = vi < m_generatedVertexLoopSourceIds.size() ? m_generatedVertexLoopSourceIds[vi] : Uuid();
    }

    // Pick a representative source for the apex vertex (most common among boundary).
    std::map<Uuid, size_t> sourceVotes;
    for (size_t i = 0; i < n; ++i) {
        if (!boundaryVertexSources[i].isNull())
            sourceVotes[boundaryVertexSources[i]]++;
    }
    Uuid apexSource;
    Uuid apexLoopSource;
    if (!sourceVotes.empty()) {
        auto best = std::max_element(sourceVotes.begin(), sourceVotes.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });
        apexSource = best->first;
        for (size_t i = 0; i < n; ++i) {
            if (boundaryVertexSources[i] == apexSource) {
                apexLoopSource = boundaryVertexLoopSources[i];
                break;
            }
        }
    }

    std::vector<std::vector<size_t>> rings;
    rings.push_back(std::vector<size_t>(outerLoop.begin(), outerLoop.end()));

    for (size_t k = 1; k <= numRings; ++k) {
        double t = static_cast<double>(k) / static_cast<double>(numRings);
        double scale = std::cos(t * M_PI / 2.0);
        double offset = std::sin(t * M_PI / 2.0) * capHeight;

        const auto& prevRing = rings.back();

        if (k == numRings) {
            Vector3 apex = centroid + normal * capHeight;
            size_t apexIdx = m_generatedVertices.size();
            m_generatedVertices.push_back(apex);
            m_generatedVertexSources.push_back(apexSource);
            m_generatedVertexLoopSourceIds.push_back(apexLoopSource);

            for (size_t i = 0; i < n; ++i) {
                size_t a = prevRing[i];
                size_t b = prevRing[(i + 1) % n];
                m_generatedFaces.push_back({ a, apexIdx, b });
                m_generatedFaceUvs.push_back({ toUv(m_generatedVertices[a]),
                    toUv(apex),
                    toUv(m_generatedVertices[b]) });
            }
        } else {
            std::vector<size_t> ring;
            ring.reserve(n);
            for (size_t i = 0; i < n; ++i) {
                const Vector3& orig = m_generatedVertices[outerLoop[i]];

                Vector3 sharpPos = centroid + (orig - centroid) * scale + normal * offset;

                double flowDistance = avgRadius * t;
                Vector3 flowTarget = orig + flowDirections[i] * flowDistance;
                Vector3 apex = centroid + normal * capHeight;
                Vector3 smoothPos = flowTarget * (1.0 - t * t) + apex * (t * t);

                Vector3 newPos = sharpPos * sharpness + smoothPos * (1.0 - sharpness);

                size_t newIdx = m_generatedVertices.size();
                m_generatedVertices.push_back(newPos);
                m_generatedVertexSources.push_back(boundaryVertexSources[i]);
                m_generatedVertexLoopSourceIds.push_back(boundaryVertexLoopSources[i]);
                ring.push_back(newIdx);
            }

            for (size_t i = 0; i < n; ++i) {
                size_t a = prevRing[i];
                size_t b = prevRing[(i + 1) % n];
                size_t c = ring[(i + 1) % n];
                size_t d = ring[i];
                m_generatedFaces.push_back({ a, d, c, b });
                m_generatedFaceUvs.push_back({ toUv(m_generatedVertices[a]),
                    toUv(m_generatedVertices[d]),
                    toUv(m_generatedVertices[c]),
                    toUv(m_generatedVertices[b]) });
            }

            rings.push_back(std::move(ring));
        }
    }
}

void StitchLoopMeshBuilder::debugVisualizeCellColors()
{
    // Write an SVG showing each BFS-colored cell filled with a color derived from
    // its assigned loop index, plus the original loop nodes drawn as small circles.
    // The file is written to /tmp/stitch_loop_debug.svg for easy inspection.
    static const char* kColors[] = {
        "#e6194b",
        "#3cb44b",
        "#4363d8",
        "#f58231",
        "#911eb4",
        "#42d4f4",
        "#f032e6",
        "#bfef45",
        "#fabed4",
        "#469990",
        "#dcbeff",
        "#9a6324",
        "#fffac8",
        "#800000",
        "#aaffc3",
    };
    static constexpr size_t kColorCount = sizeof(kColors) / sizeof(kColors[0]);

    // Map grid coordinates to SVG pixels: use a fixed canvas size.
    constexpr int kCanvasSize = 800;
    double scale = static_cast<double>(kCanvasSize) / m_squareSide;

    auto toSvgX = [&](double worldX) { return (worldX - m_minX) * scale; };
    auto toSvgY = [&](double worldY) { return kCanvasSize - (worldY - m_minY) * scale; };

    std::ofstream out("/tmp/stitch_loop_debug.svg");
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << kCanvasSize
        << "\" height=\"" << kCanvasSize << "\">\n";
    out << "<rect width=\"100%\" height=\"100%\" fill=\"#222\"/>\n";

    double cellSvg = m_targetGridCellLength * scale;
    for (size_t r = 0; r < m_gridRows; ++r) {
        for (size_t c = 0; c < m_gridCols; ++c) {
            const CellColor& cc = m_cellColor[r * m_gridCols + c];
            if (!cc.valid())
                continue;
            double worldX = m_minX + static_cast<double>(c) * m_targetGridCellLength;
            double worldY = m_minY + static_cast<double>(r) * m_targetGridCellLength;
            double svgX = toSvgX(worldX);
            double svgY = toSvgY(worldY) - cellSvg; // top-left corner in SVG coords
            const char* fill = kColors[cc.loopIndex % kColorCount];
            out << "<rect x=\"" << svgX << "\" y=\"" << svgY
                << "\" width=\"" << cellSvg << "\" height=\"" << cellSvg
                << "\" fill=\"" << fill << "\" fill-opacity=\"0.45\" stroke=\"none\"/>\n";
        }
    }

    // Draw loop nodes as labeled circles.
    double nodeRadius = std::max(2.0, cellSvg * 1.5);
    for (size_t li = 0; li < m_loops.size(); ++li) {
        const char* stroke = kColors[li % kColorCount];
        const auto& loop = m_loops[li];
        for (size_t ni = 0; ni < loop.nodes.size(); ++ni) {
            double sx = toSvgX(loop.nodes[ni].origin.x());
            double sy = toSvgY(loop.nodes[ni].origin.y());
            out << "<circle cx=\"" << sx << "\" cy=\"" << sy
                << "\" r=\"" << nodeRadius
                << "\" fill=\"" << stroke << "\" stroke=\"white\" stroke-width=\"0.5\"/>\n";
            out << "<text x=\"" << sx + nodeRadius + 1 << "\" y=\"" << sy + 4
                << "\" font-size=\"8\" fill=\"white\">" << li << "," << ni << "</text>\n";
        }
        // Draw loop edges.
        size_t n = loop.nodes.size();
        size_t edgeEnd = loop.closed ? n : n - 1;
        for (size_t ni = 0; ni < edgeEnd; ++ni) {
            double x1 = toSvgX(loop.nodes[ni].origin.x());
            double y1 = toSvgY(loop.nodes[ni].origin.y());
            double x2 = toSvgX(loop.nodes[(ni + 1) % n].origin.x());
            double y2 = toSvgY(loop.nodes[(ni + 1) % n].origin.y());
            out << "<line x1=\"" << x1 << "\" y1=\"" << y1
                << "\" x2=\"" << x2 << "\" y2=\"" << y2
                << "\" stroke=\"" << stroke << "\" stroke-width=\"1.5\"/>\n";
        }
    }

    // Draw adjacency connections between nodes of different loops.
    for (const auto& [nodeA, neighbors] : m_nodeAdjacency) {
        double x1 = toSvgX(m_loops[nodeA.first].nodes[nodeA.second].origin.x());
        double y1 = toSvgY(m_loops[nodeA.first].nodes[nodeA.second].origin.y());
        for (const auto& nodeB : neighbors) {
            if (nodeB < nodeA)
                continue; // draw each pair once
            double x2 = toSvgX(m_loops[nodeB.first].nodes[nodeB.second].origin.x());
            double y2 = toSvgY(m_loops[nodeB.first].nodes[nodeB.second].origin.y());
            out << "<line x1=\"" << x1 << "\" y1=\"" << y1
                << "\" x2=\"" << x2 << "\" y2=\"" << y2
                << "\" stroke=\"white\" stroke-width=\"0.8\" stroke-dasharray=\"3,2\" stroke-opacity=\"0.6\"/>\n";
        }
    }

    out << "</svg>\n";
    dust3dDebug << "debugVisualizeCellColors: wrote /tmp/stitch_loop_debug.svg";
}

std::map<Uuid, std::map<std::array<PositionKey, 3>, std::array<Vector2, 3>>> StitchLoopMeshBuilder::buildPerLoopTriangleUvs() const
{
    std::map<Uuid, std::map<std::array<PositionKey, 3>, std::array<Vector2, 3>>> result;

    auto insertTriangleUv = [&](std::map<std::array<PositionKey, 3>, std::array<Vector2, 3>>& uvMap,
                                const std::vector<size_t>& face, const std::vector<Vector2>& uv) {
        if (3 == face.size()) {
            uvMap.insert({ { PositionKey(m_generatedVertices[face[0]]),
                               PositionKey(m_generatedVertices[face[1]]),
                               PositionKey(m_generatedVertices[face[2]]) },
                { uv[0], uv[1], uv[2] } });
        } else if (4 == face.size()) {
            uvMap.insert({ { PositionKey(m_generatedVertices[face[0]]),
                               PositionKey(m_generatedVertices[face[1]]),
                               PositionKey(m_generatedVertices[face[2]]) },
                { uv[0], uv[1], uv[2] } });
            uvMap.insert({ { PositionKey(m_generatedVertices[face[2]]),
                               PositionKey(m_generatedVertices[face[3]]),
                               PositionKey(m_generatedVertices[face[0]]) },
                { uv[2], uv[3], uv[0] } });
        }
    };

    // Build a set of sourceIds that belong to open (non-closed) loops so that
    // Pass 2 can prefer them over closed loops when both are candidates.
    std::set<Uuid> openLoopSourceIds;
    for (const auto& loop : m_loops) {
        if (!loop.closed)
            openLoopSourceIds.insert(loop.sourceId);
    }

    // Pass 1 — tally total vertex votes across all faces to find global vote counts.
    std::map<Uuid, size_t> globalVoteCounts;
    for (size_t i = 0; i < m_generatedFaceUvs.size(); ++i) {
        for (size_t vi : m_generatedFaces[i]) {
            if (vi < m_generatedVertexLoopSourceIds.size())
                ++globalVoteCounts[m_generatedVertexLoopSourceIds[vi]];
        }
    }

    // Pass 2 — assign faces to sub-charts.
    // Selection key: (isOpen, voteCount) — open loops always beat closed loops
    // regardless of vote count; within the same tier, higher votes wins.
    for (size_t i = 0; i < m_generatedFaceUvs.size(); ++i) {
        const auto& face = m_generatedFaces[i];
        std::set<Uuid> candidates;
        for (size_t vi : face) {
            if (vi < m_generatedVertexLoopSourceIds.size())
                candidates.insert(m_generatedVertexLoopSourceIds[vi]);
        }
        Uuid assignedId;
        bool bestIsOpen = false;
        size_t bestVotes = 0;
        for (const auto& id : candidates) {
            bool isOpen = openLoopSourceIds.count(id) > 0;
            size_t votes = globalVoteCounts.count(id) ? globalVoteCounts.at(id) : 0;
            if (assignedId.isNull()
                || (isOpen && !bestIsOpen)
                || (isOpen == bestIsOpen && votes > bestVotes)) {
                assignedId = id;
                bestIsOpen = isOpen;
                bestVotes = votes;
            }
        }
        if (!assignedId.isNull())
            insertTriangleUv(result[assignedId], face, m_generatedFaceUvs[i]);
    }

    return result;
}

} // namespace dust3d
