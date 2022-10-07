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
#include <dust3d/mesh/rope_mesh.h>

namespace dust3d
{

RopeMesh::RopeMesh(const BuildParameters &parameters):
    m_buildParameters(parameters)
{
}

const std::vector<Vector3> &RopeMesh::resultVertices()
{
    return m_resultVertices;
}

const std::vector<std::vector<size_t>> &RopeMesh::resultTriangles()
{
    return m_resultTriangles;
}

std::pair<size_t, int> RopeMesh::findNearestAxis(const Vector3 &direction)
{
    float maxDot = -1;
    size_t nearAxisIndex = 0;
    int positive = 1;
    for (size_t i = 0; i < 3; ++i) {
        const auto axis = axisDirection(i);
        auto dot = Vector3::dotProduct(axis, direction);
        auto positiveDot = std::abs(dot);
        if (positiveDot >= maxDot) {
            if (dot < 0)
                positive = -1;
            maxDot = positiveDot;
            nearAxisIndex = i;
        }
    }
    return {nearAxisIndex, positive};
}

std::vector<Vector3> RopeMesh::calculateCircleVertices(double radius, 
    size_t points, 
    const Vector3 &aroundAxis, 
    const Vector3 &startDirection,
    const Vector3 &origin)
{
    constexpr auto roundAngle = 2.0 * Math::Pi;
    auto stepAngle = roundAngle / points;
    std::vector<Vector3> circlePoints;
    circlePoints.reserve(points);
    for (double angle = stepAngle * -0.5;
            circlePoints.size() < points;
            angle += stepAngle) {
        circlePoints.push_back(origin + startDirection.rotated(aroundAxis, angle) * radius);
    }
    return circlePoints;
}

Vector3 RopeMesh::calculateCircleBaseNormal(const std::vector<Vector3> &vertices)
{
    std::vector<Vector3> edgeDirections(vertices.size());
    for (size_t i = 0; i < edgeDirections.size(); ++i) {
        size_t j = (i + 1) % edgeDirections.size();
        edgeDirections[i] = (vertices[j] - vertices[i]).normalized();
    }
    Vector3 baseNormal;
    for (size_t i = 0; i < edgeDirections.size(); ++i) {
        size_t j = (i + 1) % edgeDirections.size();
        baseNormal += Vector3::crossProduct(-edgeDirections[i], edgeDirections[j]);
    }
    return baseNormal.normalized();
}

Vector3 RopeMesh::calculateTubeBaseNormal(const std::vector<Vector3> &vertices)
{
    std::vector<Vector3> edgeDirections(vertices.size());
    for (size_t i = 1; i < edgeDirections.size(); ++i) {
        size_t h = i - 1;
        edgeDirections[h] = (vertices[i] - vertices[h]).normalized();
    }
    Vector3 baseNormal;
    for (size_t i = 1; i < edgeDirections.size(); ++i) {
        size_t h = i - 1;
        // >15 degrees && < 165 degrees
        if (std::abs(Vector3::dotProduct(edgeDirections[h], edgeDirections[i])) < 0.966)
            baseNormal += Vector3::crossProduct(edgeDirections[h], edgeDirections[i]);
    }
    if (baseNormal.isZero()) {
        for (size_t h = 0; h + 1 < edgeDirections.size(); ++h) {
            const auto &sectionNormal = edgeDirections[h];
            auto axis = RopeMesh::findNearestAxis(sectionNormal);
            baseNormal += axis.second * 
                Vector3::crossProduct(sectionNormal, RopeMesh::nextAxisDirection(axis.first)).normalized();
        }
    }
    return baseNormal.normalized();
}

void RopeMesh::addRope(const std::vector<Vector3> &positions, bool isCircle)
{
    Vector3 baseNormal = isCircle ? calculateCircleBaseNormal(positions) : calculateTubeBaseNormal(positions);
    std::vector<std::vector<size_t>> circles;
    circles.reserve(positions.size());
    for (size_t j = isCircle ? 0 : 1; j < positions.size(); ++j) {
        size_t i = (j + positions.size() - 1) % positions.size();
        auto circlePositions = calculateCircleVertices(m_buildParameters.defaultRadius, 
            m_buildParameters.sectionSegments, 
            (positions[j] - positions[i]).normalized(), 
            baseNormal,
            positions[i]);
        std::vector<size_t> indices(circlePositions.size());
        for (size_t k = 0; k < indices.size(); ++k) {
            indices[k] = m_resultVertices.size();
            m_resultVertices.push_back(circlePositions[k]);
        }
        circles.emplace_back(indices);
    }
    for (size_t j = isCircle ? 0 : 1; j < circles.size(); ++j) {
        size_t i = (j + circles.size() - 1) % circles.size();
        const auto &circlesI = circles[i];
        const auto &circlesJ = circles[j];
        for (size_t m = 0; m < circlesI.size(); ++m) {
            size_t n = (m + 1) % circlesI.size();
            m_resultTriangles.emplace_back(std::vector<size_t> {
                circlesI[m], circlesI[n], circlesJ[n]
            });
            m_resultTriangles.emplace_back(std::vector<size_t> {
                circlesJ[n], circlesJ[m], circlesI[m]
            });
        }
    }
}

};
