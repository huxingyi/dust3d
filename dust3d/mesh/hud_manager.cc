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

#include <dust3d/base/predefined.h>
#include <dust3d/mesh/hud_manager.h>

namespace dust3d
{

HudManager::HudManager()
{
}

void HudManager::addFromObject(const Object &object)
{
    for (const auto &node: object.nodes) {
        addController(node.origin, node.radius, node.direction, node.color);
    }
}

void HudManager::addController(const Vector3 &origin, float radius, const Vector3 &direction, const Color &color)
{
    float expandedRadius = radius + 0.15;
    auto firstDirection = direction.normalized();
    auto axis = Predefined::findNearestAxis(firstDirection);
    auto secondDirection = Predefined::nextAxisDirection(axis.first) * axis.second;
    auto thirdDirection = Vector3::crossProduct(firstDirection, secondDirection);
    secondDirection = Vector3::crossProduct(thirdDirection, firstDirection);
    auto originIndex = addPosition(origin);
    auto firstPointIndex = addPosition(origin + firstDirection * expandedRadius);
    auto secondPointIndex = addPosition(origin + secondDirection * expandedRadius);
    auto thirdPointIndex = addPosition(origin + thirdDirection * expandedRadius);
    addLine(originIndex, firstPointIndex, Color("#ff0000"));
    addLine(originIndex, secondPointIndex, Color("#00ff00"));
    addLine(originIndex, thirdPointIndex, Color("#0000ff"));
    auto addRound = [&](Vector3 origin, double radius) {
        auto steps = 60;
        auto roundAngle = 2.0 * Math::Pi;
        auto stepAngle = 2.0 * Math::Pi / steps;
        std::vector<Vector3> roundPoints;
        roundPoints.reserve(steps + 1);
        for (double angle = 0.0; angle < roundAngle; angle += stepAngle) {
            roundPoints.push_back(origin + secondDirection.rotated(firstDirection, angle) * radius);
        }
        addController(origin, roundPoints, color);
    };
    addRound(origin, expandedRadius);
    /*
    addRound(origin, expandedRadius + 0.0005);
    addRound(origin, expandedRadius + 0.0010);
    addRound(origin + firstDirection * 0.0005, expandedRadius);
    addRound(origin - firstDirection * 0.0005, expandedRadius);
    addRound(origin + firstDirection * 0.0005, expandedRadius + 0.0005);
    addRound(origin - firstDirection * 0.0005, expandedRadius + 0.0005);
    addRound(origin + firstDirection * 0.0005, expandedRadius + 0.0010);
    addRound(origin - firstDirection * 0.0005, expandedRadius + 0.0010);
    */
}

void HudManager::addController(const Vector3 &/*origin*/, const std::vector<Vector3> &points, const Color &color)
{
    std::vector<size_t> indices(points.size());
    for (size_t i = 0; i < points.size(); ++i) {
        indices[i] = addPosition(points[i]);
    }
    for (size_t i = 0; i < indices.size(); ++i) {
        size_t j = (i + 1) % indices.size();
        addLine(indices[i], indices[j], color);
    }
}

size_t HudManager::addPosition(const Vector3 &position)
{
    size_t index = m_positions.size();
    m_positions.push_back(position);
    return index;
}

void HudManager::addLine(size_t from, size_t to, const Color &color)
{
    m_lines.emplace_back(std::tuple<size_t, size_t, Color> {from, to, color});
}

void HudManager::generate()
{
    // m_vertices = std::make_unique<std::vector<float>>();
    // m_vertices->reserve(m_lines.size() * 2 * (size_t)OutputVertexAttribute::Max);
    // for (const auto &line: m_lines) {
    //     const auto &color = std::get<2>(line);
    //     const auto &fromVertex = m_positions[std::get<0>(line)];
    //     m_vertices->push_back(fromVertex.x());
    //     m_vertices->push_back(fromVertex.y());
    //     m_vertices->push_back(fromVertex.z());
    //     m_vertices->push_back(color.r());
    //     m_vertices->push_back(color.g());
    //     m_vertices->push_back(color.b());
    //     m_vertices->push_back(color.alpha());
    //     const auto &toVertex = m_positions[std::get<1>(line)];
    //     m_vertices->push_back(toVertex.x());
    //     m_vertices->push_back(toVertex.y());
    //     m_vertices->push_back(toVertex.z());
    //     m_vertices->push_back(color.r());
    //     m_vertices->push_back(color.g());
    //     m_vertices->push_back(color.b());
    //     m_vertices->push_back(color.alpha());
    // }
    // m_lineVertexCount = m_vertices->size() / (size_t)OutputVertexAttribute::Max;

    m_vertices = std::make_unique<std::vector<float>>();
    m_vertices->reserve(m_lines.size() * 24 * (size_t)OutputVertexAttribute::Max);
    auto buildSection = [](const Vector3 &origin, 
            const Vector3 &faceNormal, 
            const Vector3 &tangent, 
            double radius)
    {
        auto upDirection = Vector3::crossProduct(tangent, faceNormal);
        return std::vector<Vector3> {
            origin + tangent * radius,
            origin - upDirection * radius,
            origin - tangent * radius,
            origin + upDirection * radius
        };
    };
    auto addPoint = [&](const Vector3 &position, const auto &color) {
        m_vertices->push_back(position.x());
        m_vertices->push_back(position.y());
        m_vertices->push_back(position.z());
        m_vertices->push_back(color.r());
        m_vertices->push_back(color.g());
        m_vertices->push_back(color.b());
        m_vertices->push_back(color.alpha());
    };
    for (const auto &line: m_lines) {
        const auto &color = std::get<2>(line);
        const auto &from = m_positions[std::get<0>(line)];
        const auto &to = m_positions[std::get<1>(line)];
        auto direction = to - from;
        auto sectionNormal = direction.normalized();
        auto axis = Predefined::findNearestAxis(sectionNormal);
        auto sectionTangent = Vector3::crossProduct(sectionNormal, Predefined::nextAxisDirection(axis.first)) * axis.second;
        auto sectionTo = buildSection(to, sectionNormal, sectionTangent, 0.005);
        auto sectionFrom = sectionTo;
        for (auto &it: sectionFrom)
            it = it - direction;
        for (size_t i = 0; i < sectionTo.size(); ++i) {
            size_t j = (i + 1) % sectionTo.size();
            addPoint(sectionTo[i], color);
            addPoint(sectionFrom[i], color);
            addPoint(sectionTo[j], color);
            addPoint(sectionFrom[i], color);
            addPoint(sectionFrom[j], color);
            addPoint(sectionTo[j], color);
        }
    }
    m_triangleVertexCount = m_vertices->size() / (size_t)OutputVertexAttribute::Max;
}

std::unique_ptr<std::vector<float>> HudManager::takeVertices(int *lineVertexCount, int *triangleVertexCount)
{
    if (nullptr != lineVertexCount)
        *lineVertexCount = m_lineVertexCount;
    if (nullptr != triangleVertexCount)
        *triangleVertexCount = m_triangleVertexCount;
    return std::move(m_vertices);
}

}
