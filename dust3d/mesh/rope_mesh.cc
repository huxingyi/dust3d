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

std::vector<Vector3> RopeMesh::cornerInterpolated(const std::vector<Vector3>& positions, double cornerRadius, bool isCircle)
{
    std::vector<Vector3> nodePositions;
    nodePositions.reserve(positions.size() * 3);
    if (isCircle) {
        std::vector<Vector3> forwardDirections(positions.size());
        for (size_t i = 0; i < positions.size(); ++i) {
            size_t j = (i + 1) % positions.size();
            forwardDirections[i] = (positions[j] - positions[i]).normalized();
        }
        for (size_t i = 0; i < positions.size(); ++i) {
            size_t j = (i + 1) % positions.size();
            nodePositions.push_back(positions[i]);
            nodePositions.push_back(positions[i] + forwardDirections[i] * cornerRadius);
            nodePositions.push_back(positions[j] - forwardDirections[i] * cornerRadius);
        }
        return nodePositions;
    } else {
        std::vector<Vector3> forwardDirections(positions.size());
        for (size_t j = 1; j < positions.size(); ++j) {
            size_t i = j - 1;
            forwardDirections[i] = (positions[j] - positions[i]).normalized();
        }
        nodePositions.push_back(positions.front());
        for (size_t j = 1; j + 1 < positions.size(); ++j) {
            size_t i = j - 1;
            nodePositions.push_back(positions[j] - forwardDirections[i] * cornerRadius);
            nodePositions.push_back(positions[j]);
            nodePositions.push_back(positions[j] + forwardDirections[j] * cornerRadius);
        }
        nodePositions.push_back(positions.back());
    }
    return nodePositions;
}

void RopeMesh::addRope(const std::vector<Vector3>& positions, bool isCircle)
{
    if (positions.size() < 2) {
        dust3dDebug << "Expected at least 2 nodes, current:" << positions.size();
        return;
    }
    auto nodePositions = cornerInterpolated(positions, m_buildParameters.defaultRadius, isCircle);
    Vector3 baseNormal = isCircle ? BaseNormal::calculateCircleBaseNormal(nodePositions) : BaseNormal::calculateTubeBaseNormal(nodePositions);
    std::vector<std::vector<size_t>> circles;
    circles.reserve(nodePositions.size());
    Vector3 forwardDirection;
    for (size_t i = 0; i < nodePositions.size(); ++i) {
        size_t j = (i + 1) % nodePositions.size();
        if (isCircle || i + 1 < nodePositions.size())
            forwardDirection = (nodePositions[j] - nodePositions[i]).normalized();
        auto circlePositions = BaseNormal::calculateCircleVertices(m_buildParameters.defaultRadius,
            m_buildParameters.sectionSegments,
            forwardDirection,
            baseNormal,
            nodePositions[i]);
        std::vector<size_t> indices(circlePositions.size());
        for (size_t k = 0; k < indices.size(); ++k) {
            indices[k] = m_resultVertices.size();
            m_resultVertices.push_back(circlePositions[k]);
        }
        circles.emplace_back(indices);
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
