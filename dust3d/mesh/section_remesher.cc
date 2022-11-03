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

#include <dust3d/mesh/section_remesher.h>

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

bool SectionRemesher::isConvex(const std::vector<Vector3>& vertices)
{
    if (vertices.size() <= 3)
        return true;

    Vector3 previousNormal = Vector3::normal(vertices[0], vertices[1], vertices[2]);
    for (size_t i = 1; i < vertices.size(); ++i) {
        size_t j = (i + 1) % vertices.size();
        size_t k = (i + 2) % vertices.size();
        Vector3 currentNormal = Vector3::normal(vertices[i], vertices[j], vertices[k]);
        if (Vector3::dotProduct(previousNormal, currentNormal) < 0)
            return false;
        previousNormal = currentNormal;
    }

    return true;
}

void SectionRemesher::remesh()
{
    m_ringSize = m_vertices.size();

    if (isConvex(m_vertices)) {
        Vector3 center;
        for (const auto& it : m_vertices)
            center += it;
        center /= m_vertices.size();
        m_vertices.push_back(center);
        double offsetU = 0;
        std::vector<double> maxUs(m_ringSize + 1);
        for (size_t i = 0; i < m_ringSize; ++i) {
            size_t j = (i + 1) % m_ringSize;
            maxUs[i] = offsetU;
            offsetU += (m_vertices[i] - m_vertices[j]).length();
        }
        maxUs[m_ringSize] = offsetU;
        offsetU = std::max(offsetU, std::numeric_limits<double>::epsilon());
        for (auto& it : maxUs)
            it /= offsetU;
        for (size_t i = 0; i < m_ringSize; ++i) {
            size_t j = (i + 1) % m_ringSize;
            m_generatedFaces.emplace_back(std::vector<size_t> { i, j, m_ringSize });
            m_generatedFaceUvs.emplace_back(std::vector<Vector2> {
                Vector2(maxUs[i], m_ringV),
                Vector2(maxUs[j], m_ringV),
                Vector2((maxUs[i] + maxUs[j]) * 0.5, m_centerV) });
        }
        return;
    }

    // TODO: Process non convex
}

}
