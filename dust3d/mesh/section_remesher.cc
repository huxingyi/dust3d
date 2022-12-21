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

#include <algorithm>
#include <dust3d/base/debug.h>
#include <dust3d/mesh/section_remesher.h>
#include <dust3d/mesh/triangulate.h>

namespace dust3d {

SectionRemesher::SectionRemesher(const std::vector<Vector3>& vertices, double ringV, double centerV)
    : m_vertices(vertices)
    , m_ringV(ringV)
    , m_centerV(centerV)
{
}

const std::vector<Vector3>& SectionRemesher::generatedVertices()
{
    return m_vertices;
}

const std::vector<std::vector<size_t>>& SectionRemesher::generatedFaces()
{
    return m_generatedFaces;
}

const std::vector<std::vector<Vector2>>& SectionRemesher::generatedFaceUvs()
{
    return m_generatedFaceUvs;
}

bool SectionRemesher::isConvex(const std::vector<size_t>& ringVertices)
{
    if (ringVertices.size() <= 3)
        return true;

    Vector3 previousNormal = Vector3::normal(m_vertices[ringVertices[1]], m_vertices[ringVertices[2]], m_vertices[ringVertices[0]]);
    for (size_t i = 1; i < ringVertices.size(); ++i) {
        size_t j = (i + 1) % ringVertices.size();
        size_t k = (i + 2) % ringVertices.size();
        Vector3 currentNormal = Vector3::normal(m_vertices[ringVertices[j]], m_vertices[ringVertices[k]], m_vertices[ringVertices[i]]);
        if (Vector3::dotProduct(previousNormal, currentNormal) < 0)
            return false;
        previousNormal = currentNormal;
    }

    return true;
}

void SectionRemesher::remeshConvex(const std::vector<size_t>& ringVertices, const std::vector<double>& ringUs)
{
    Vector3 center;
    for (const auto& it : ringVertices)
        center += m_vertices[it];
    center /= ringVertices.size();
    size_t centerVertex = m_vertices.size();
    m_vertices.push_back(center);

    for (size_t i = 0; i < ringVertices.size(); ++i) {
        size_t j = (i + 1) % ringVertices.size();
        m_generatedFaces.emplace_back(std::vector<size_t> { ringVertices[i], ringVertices[j], centerVertex });
        m_generatedFaceUvs.emplace_back(std::vector<Vector2> {
            Vector2(ringUs[i], m_ringV),
            Vector2(ringUs[i + 1], m_ringV),
            Vector2((ringUs[i] + ringUs[i + 1]) * 0.5, m_centerV) });
    }
}

void SectionRemesher::remesh()
{
    m_ringUs.resize(m_vertices.size() + 1);
    double offsetU = 0;
    for (size_t i = 0; i < m_vertices.size(); ++i) {
        size_t j = (i + 1) % m_vertices.size();
        m_ringUs[i] = offsetU;
        offsetU += (m_vertices[i] - m_vertices[j]).length();
    }
    m_ringUs[m_vertices.size()] += offsetU;
    offsetU = std::max(offsetU, std::numeric_limits<double>::epsilon());
    for (auto& it : m_ringUs)
        it /= offsetU;

    std::vector<size_t> ring(m_vertices.size());
    for (size_t i = 0; i < ring.size(); ++i)
        ring[i] = i;

    if (isConvex(ring)) {
        remeshConvex(ring, m_ringUs);
        return;
    }

    std::vector<std::vector<size_t>> triangles;
    dust3d::triangulate(m_vertices, ring, &triangles);
    size_t halfSize = m_vertices.size() / 2;
    for (const auto& triangle : triangles) {
        m_generatedFaces.emplace_back(std::vector<size_t> { triangle[0], triangle[1], triangle[2] });
        m_generatedFaceUvs.emplace_back(std::vector<Vector2> {
            Vector2(m_ringUs[triangle[0]], triangle[0] < halfSize ? m_ringV : m_centerV),
            Vector2(m_ringUs[triangle[1]], triangle[1] < halfSize ? m_ringV : m_centerV),
            Vector2(m_ringUs[triangle[2]], triangle[2] < halfSize ? m_ringV : m_centerV) });
    }
}

}
