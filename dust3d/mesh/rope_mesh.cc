/*
 *  Copyright (c) 2016-2022 Jeremy HU <jeremy-at-dust3d dot org>. All rights reserved. 
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

#include <dust3d/base/debug.h>
#include <dust3d/base/math.h>
#include <dust3d/mesh/base_normal.h>
#include <dust3d/mesh/rope_mesh.h>

namespace dust3d {

RopeMesh::RopeMesh(const BuildParameters& parameters)
    : m_buildParameters(parameters)
{
}

const std::vector<Vector3>& RopeMesh::resultVertices()
{
    return m_resultVertices;
}

const std::vector<std::vector<size_t>>& RopeMesh::resultTriangles()
{
    return m_resultTriangles;
}

void RopeMesh::addRope(const std::vector<Vector3>& positions, bool isCircle)
{
    if (positions.size() < 2) {
        dust3dDebug << "Expected at least 2 nodes, current:" << positions.size();
        return;
    }
    Vector3 baseNormal = isCircle ? BaseNormal::calculateCircleBaseNormal(positions) : BaseNormal::calculateTubeBaseNormal(positions);
    std::vector<std::vector<size_t>> circles;
    circles.reserve(positions.size());
    Vector3 forwardDirection = (positions[1] - positions[0]).normalized();
    for (size_t i = isCircle ? 0 : 1; i < positions.size(); ++i) {
        size_t j = (i + 1) % positions.size();
        auto circlePositions = BaseNormal::calculateCircleVertices(m_buildParameters.defaultRadius,
            m_buildParameters.sectionSegments,
            forwardDirection,
            baseNormal,
            positions[i]);
        std::vector<size_t> indices(circlePositions.size());
        for (size_t k = 0; k < indices.size(); ++k) {
            indices[k] = m_resultVertices.size();
            m_resultVertices.push_back(circlePositions[k]);
        }
        circles.emplace_back(indices);
        forwardDirection = (positions[j] - positions[i]).normalized();
    }
    for (size_t j = isCircle ? 0 : 1; j < circles.size(); ++j) {
        size_t i = (j + circles.size() - 1) % circles.size();
        const auto& circlesI = circles[i];
        const auto& circlesJ = circles[j];
        for (size_t m = 0; m < circlesI.size(); ++m) {
            size_t n = (m + 1) % circlesI.size();
            m_resultTriangles.emplace_back(std::vector<size_t> {
                circlesI[m], circlesI[n], circlesJ[n] });
            m_resultTriangles.emplace_back(std::vector<size_t> {
                circlesJ[n], circlesJ[m], circlesI[m] });
        }
    }
}

};
