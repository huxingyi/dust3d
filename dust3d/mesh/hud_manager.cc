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
    float expandedRadius = radius + 0.05;
    auto firstDirection = direction.normalized();
    auto axis = Predefined::findNearestAxis(firstDirection);
    auto secondDirection = Predefined::nextAxisDirection(axis.first) * axis.second;
    auto thirdDirection = Vector3::crossProduct(secondDirection, firstDirection);
    auto originIndex = addVertex(origin);
    auto firstPointIndex = addVertex(origin + firstDirection * expandedRadius);
    auto secondPointIndex = addVertex(origin + secondDirection * expandedRadius);
    auto thirdPointIndex = addVertex(origin + thirdDirection * expandedRadius);
    addLine(originIndex, firstPointIndex, Color("#ff0000"));
    addLine(originIndex, secondPointIndex, Color("#00ff00"));
    addLine(originIndex, thirdPointIndex, Color("#0000ff"));
}

void HudManager::addController(const Vector3 &/*origin*/, const std::vector<Vector3> &points, const Color &color)
{
    std::vector<size_t> indices(points.size());
    for (size_t i = 0; i < points.size(); ++i) {
        indices[i] = addVertex(points[i]);
    }
    for (size_t i = 0; i < indices.size(); ++i) {
        size_t j = (i + 1) % indices.size();
        addLine(indices[i], indices[j], color);
    }
}

size_t HudManager::addVertex(const Vector3 &position)
{
    size_t index = m_vertices.size();
    m_vertices.push_back(position);
    return index;
}

void HudManager::addLine(size_t from, size_t to, const Color &color)
{
    m_lines.emplace_back(std::tuple<size_t, size_t, Color> {from, to, color});
}

void HudManager::generate()
{
    m_lineVertices = std::make_unique<std::vector<float>>();
    m_lineVertices->reserve(m_lines.size() * 2 * (size_t)LineVertex::Max);
    for (const auto &line: m_lines) {
        const auto &color = std::get<2>(line);
        const auto &fromVertex = m_vertices[std::get<0>(line)];
        m_lineVertices->push_back(fromVertex.x());
        m_lineVertices->push_back(fromVertex.y());
        m_lineVertices->push_back(fromVertex.z());
        m_lineVertices->push_back(color.r());
        m_lineVertices->push_back(color.g());
        m_lineVertices->push_back(color.b());
        m_lineVertices->push_back(color.alpha());
        const auto &toVertex = m_vertices[std::get<1>(line)];
        m_lineVertices->push_back(toVertex.x());
        m_lineVertices->push_back(toVertex.y());
        m_lineVertices->push_back(toVertex.z());
        m_lineVertices->push_back(color.r());
        m_lineVertices->push_back(color.g());
        m_lineVertices->push_back(color.b());
        m_lineVertices->push_back(color.alpha());
    }
}

std::unique_ptr<std::vector<float>> HudManager::takeLineVertices()
{
    return std::move(m_lineVertices);
}

}
