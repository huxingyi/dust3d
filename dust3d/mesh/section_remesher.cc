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

Vector3 SectionRemesher::sectionNormal(const std::vector<size_t>& ringVertices)
{
    if (ringVertices.size() < 3)
        return Vector3();

    Vector3 normal = Vector3::normal(m_vertices[ringVertices[1]], m_vertices[ringVertices[2]], m_vertices[ringVertices[0]]);
    for (size_t i = 1; i < ringVertices.size(); ++i) {
        size_t j = (i + 1) % ringVertices.size();
        size_t k = (i + 2) % ringVertices.size();
        normal += Vector3::normal(m_vertices[ringVertices[j]], m_vertices[ringVertices[k]], m_vertices[ringVertices[i]]);
    }
    return normal.normalized();
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

int SectionRemesher::findNonConvexVertex(const std::vector<size_t>& ringVertices)
{
    int nonConvexVertex = -1;
    double maxEdgeLength = 0.0;
    Vector3 positiveNormal = sectionNormal(ringVertices);
    for (size_t i = 0; i < ringVertices.size(); ++i) {
        size_t j = (i + 1) % ringVertices.size();
        size_t k = (i + 2) % ringVertices.size();
        Vector3 currentNormal = Vector3::normal(m_vertices[ringVertices[j]], m_vertices[ringVertices[k]], m_vertices[ringVertices[i]]);
        if (Vector3::dotProduct(positiveNormal, currentNormal) >= 0)
            continue;
        double edgeLength = (m_vertices[ringVertices[j]] - m_vertices[ringVertices[k]]).length() + (m_vertices[ringVertices[j]] - m_vertices[ringVertices[i]]).length();
        if (edgeLength > maxEdgeLength) {
            maxEdgeLength = edgeLength;
            nonConvexVertex = (int)j;
        }
    }
    return nonConvexVertex;
}

int SectionRemesher::findPairVertex(const std::vector<size_t>& ringVertices, int vertex)
{
    size_t halfSize = ringVertices.size() / 2;
    size_t quarterSize = std::max(ringVertices.size() / 4, (size_t)2);
    double maxDotProduct = std::numeric_limits<double>::lowest();
    int pairVertex = -1;
    Vector3 forwardDirection = ((m_vertices[ringVertices[vertex]] - m_vertices[ringVertices[(vertex + 1) % ringVertices.size()]]) + (m_vertices[ringVertices[vertex]] - m_vertices[ringVertices[(vertex + ringVertices.size() - 1) % ringVertices.size()]])).normalized();
    for (size_t i = 0; i < halfSize; ++i) {
        size_t index = (vertex + quarterSize + i) % ringVertices.size();
        if ((int)index == vertex)
            continue;
        double dot = Vector3::dotProduct(forwardDirection, (m_vertices[ringVertices[index]] - m_vertices[ringVertices[vertex]]).normalized());
        if (dot > maxDotProduct) {
            maxDotProduct = dot;
            pairVertex = index;
        }
    }
    return pairVertex;
}

void SectionRemesher::remeshRing(const std::vector<size_t>& ringVertices, const std::vector<double>& ringUs)
{
    if (isConvex(ringVertices)) {
        remeshConvex(ringVertices, ringUs);
        return;
    }
    int nonConvexVertex = findNonConvexVertex(ringVertices);
    if (-1 == nonConvexVertex) {
        remeshConvex(ringVertices, ringUs);
        return;
    }
    int pairVertex = findPairVertex(ringVertices, nonConvexVertex);
    if (-1 == pairVertex) {
        remeshConvex(ringVertices, ringUs);
        return;
    }

    auto nextVertex = [&](int index) {
        return (index + 1) % ringVertices.size();
    };

    std::vector<size_t> leftRing;
    std::vector<double> leftUs;
    for (int i = pairVertex;; i = nextVertex(i)) {
        leftRing.push_back(i);
        leftUs.push_back(ringUs[i]);
        if (i == nonConvexVertex)
            break;
    }
    leftUs.push_back(leftUs.back());
    remeshRing(leftRing, leftUs);

    std::vector<size_t> rightRing;
    std::vector<double> rightUs;
    for (int i = nonConvexVertex;; i = nextVertex(i)) {
        rightRing.push_back(i);
        rightUs.push_back(ringUs[i]);
        if (i == pairVertex)
            break;
    }
    rightUs.push_back(rightUs.back());
    remeshRing(rightRing, rightUs);
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
    remeshRing(ring, m_ringUs);
    /*
    int nonConvexVertex = -1;

    if (isConvex(m_vertices) || -1 == (nonConvexVertex = findNonConvexVertex(m_vertices))) {
        remeshConvex(m_vertices, m_ringUs);
        return;
    }

    int pairVertex = findPairVertex(m_vertices, nonConvexVertex);
    if (-1 == pairVertex) {
        remeshConvex(m_vertices, m_ringUs);
        return;
    }

    auto nextVertex = [&](int index) {
        return (index + 1) % m_vertices.size();
    };

    auto previousVertex = [&](int index) {
        return (index + m_vertices.size() - 1) % m_vertices.size();
    };

    struct CutLine {
        int nearVertex;
        int farVertex;
    };
    std::vector<CutLine> cutLines;
    for (int i = nonConvexVertex, j = pairVertex;;
         i = previousVertex(i), j = nextVertex(j)) {
        cutLines.push_back(CutLine { i, j });
        if (i == j || i == nextVertex(j))
            break;
    }
    std::reverse(cutLines.begin(), cutLines.end());
    for (int i = nextVertex(nonConvexVertex), j = previousVertex(pairVertex);;
         i = nextVertex(i), j = previousVertex(j)) {
        cutLines.push_back(CutLine { i, j });
        if (i == j || i == previousVertex(j))
            break;
    }

    if (cutLines.size() < 2) {
        remeshConvex(m_vertices, m_ringUs);
        return;
    }

    dust3dDebug << "cutLines: (m_vertices:" << m_vertices.size() << ")";
    for (size_t i = 0; i < cutLines.size(); ++i) {
        dust3dDebug << "line[" << i << "]:" << cutLines[i].nearVertex << (cutLines[i].nearVertex == nonConvexVertex ? "(nonConv)" : "") << cutLines[i].farVertex << (cutLines[i].farVertex == pairVertex ? "(pair)" : "");
    }

    for (size_t j = 1; j < cutLines.size(); ++j) {
        size_t i = j - 1;
        if (cutLines[i].nearVertex == cutLines[i].farVertex) {
            if (cutLines[i].nearVertex == cutLines[j].nearVertex || cutLines[i].farVertex == cutLines[j].farVertex)
                continue;
            dust3dDebug << "A" << cutLines[i].nearVertex << cutLines[j].nearVertex << cutLines[j].farVertex;
            m_generatedFaces.emplace_back(std::vector<size_t> { cutLines[i].nearVertex, cutLines[j].nearVertex, cutLines[j].farVertex });
            m_generatedFaceUvs.emplace_back(std::vector<Vector2> { Vector2(), Vector2(), Vector2() });
        } else if (cutLines[j].nearVertex == cutLines[j].farVertex) {
            if (cutLines[i].nearVertex == cutLines[j].nearVertex || cutLines[i].farVertex == cutLines[j].farVertex)
                continue;
            dust3dDebug << "B" << cutLines[i].nearVertex << cutLines[j].nearVertex << cutLines[i].farVertex;
            m_generatedFaces.emplace_back(std::vector<size_t> { cutLines[i].nearVertex, cutLines[j].nearVertex, cutLines[i].farVertex });
            m_generatedFaceUvs.emplace_back(std::vector<Vector2> { Vector2(), Vector2(), Vector2() });
        } else {
            dust3dDebug << "C" << cutLines[i].nearVertex << cutLines[j].nearVertex << cutLines[j].farVertex << cutLines[i].farVertex;
            m_generatedFaces.emplace_back(std::vector<size_t> { cutLines[i].nearVertex, cutLines[j].nearVertex, cutLines[j].farVertex, cutLines[i].farVertex });
            m_generatedFaceUvs.emplace_back(std::vector<Vector2> { Vector2(), Vector2(), Vector2(), Vector2() });
        }
    }
    */
}

}
