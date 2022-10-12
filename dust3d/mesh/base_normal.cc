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

#include <dust3d/mesh/base_normal.h>

namespace dust3d
{

std::pair<size_t, int> BaseNormal::findNearestAxis(const Vector3 &direction)
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

std::vector<Vector3> BaseNormal::calculateCircleVertices(double radius, 
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

Vector3 BaseNormal::calculateCircleBaseNormal(const std::vector<Vector3> &vertices)
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

Vector3 BaseNormal::calculateTubeBaseNormal(const std::vector<Vector3> &vertices)
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
            baseNormal += Vector3::crossProduct(-edgeDirections[h], edgeDirections[i]);
    }
    if (baseNormal.isZero()) {
        for (size_t h = 0; h + 1 < edgeDirections.size(); ++h) {
            const auto &sectionNormal = edgeDirections[h];
            auto axis = BaseNormal::findNearestAxis(sectionNormal);
            baseNormal += axis.second * 
                Vector3::crossProduct(sectionNormal, BaseNormal::nextAxisDirection(axis.first)).normalized();
        }
    }
    return baseNormal.normalized();
}

}
