/*
 *  Copyright (c) 2016-2021 Jeremy HU <jeremy-at-dust3d dot org>. All rights reserved. 
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

#include <dust3d/mesh/centripetal_catmull_rom_spline.h>

namespace dust3d {

CentripetalCatmullRomSpline::CentripetalCatmullRomSpline(bool isClosed)
    : m_isClosed(isClosed)
{
}

const std::vector<CentripetalCatmullRomSpline::SplineNode>& CentripetalCatmullRomSpline::splineNodes()
{
    return m_splineNodes;
}

void CentripetalCatmullRomSpline::addPoint(int source, const Vector3& position, bool isKnot)
{
    if (isKnot)
        m_splineKnots.push_back(m_splineNodes.size());
    m_splineNodes.push_back({ source, position });
}

float CentripetalCatmullRomSpline::atKnot(float t, const Vector3& p0, const Vector3& p1)
{
    const float alpha = 0.5f;
    Vector3 d = p1 - p0;
    float a = Vector3::dotProduct(d, d);
    float b = std::pow(a, alpha * 0.5f);
    return (b + t);
}

void CentripetalCatmullRomSpline::interpolateSegment(std::vector<Vector3>& knots,
    size_t from, size_t to)
{
    const Vector3& p0 = knots[0];
    const Vector3& p1 = knots[1];
    const Vector3& p2 = knots[2];
    const Vector3& p3 = knots[3];

    float s1 = (p2 - p1).length();

    float t0 = 0.0f;
    float t1 = atKnot(t0, p0, p1);
    float t2 = atKnot(t1, p1, p2);
    float t3 = atKnot(t2, p2, p3);

    for (size_t index = (from + 1) % m_splineNodes.size(); index != to; index = (index + 1) % m_splineNodes.size()) {
        auto& position = m_splineNodes[index].position;

        float factor = (position - p1).length() / s1;
        float t = t1 * (1.0f - factor) + t2 * factor;

        Vector3 a1 = (t1 - t) / (t1 - t0) * p0 + (t - t0) / (t1 - t0) * p1;
        Vector3 a2 = (t2 - t) / (t2 - t1) * p1 + (t - t1) / (t2 - t1) * p2;
        Vector3 a3 = (t3 - t) / (t3 - t2) * p2 + (t - t2) / (t3 - t2) * p3;

        Vector3 b1 = (t2 - t) / (t2 - t0) * a1 + (t - t0) / (t2 - t0) * a2;
        Vector3 b2 = (t3 - t) / (t3 - t1) * a2 + (t - t1) / (t3 - t1) * a3;

        position = ((t2 - t) / (t2 - t1) * b1 + (t - t1) / (t2 - t1) * b2);
    }
}

bool CentripetalCatmullRomSpline::interpolate()
{
    if (m_isClosed)
        return interpolateClosed();

    return interpolateOpened();
}

bool CentripetalCatmullRomSpline::interpolateOpened()
{
    if (m_splineKnots.size() < 3)
        return false;

    {
        std::vector<Vector3> knots = {
            (m_splineNodes[m_splineKnots[0]].position + (m_splineNodes[m_splineKnots[0]].position - m_splineNodes[m_splineKnots[1]].position)),
            m_splineNodes[m_splineKnots[0]].position,
            m_splineNodes[m_splineKnots[1]].position,
            m_splineNodes[m_splineKnots[2]].position
        };
        interpolateSegment(knots, m_splineKnots[0], m_splineKnots[1]);
    }

    {
        size_t tail = m_splineKnots.size() - 1;
        std::vector<Vector3> knots = {
            m_splineNodes[m_splineKnots[tail - 2]].position,
            m_splineNodes[m_splineKnots[tail - 1]].position,
            m_splineNodes[m_splineKnots[tail]].position,
            (m_splineNodes[m_splineKnots[tail]].position + (m_splineNodes[m_splineKnots[tail]].position - m_splineNodes[m_splineKnots[tail - 1]].position))
        };
        interpolateSegment(knots, m_splineKnots[tail - 1], m_splineKnots[tail]);
    }

    for (size_t i = 1; i + 2 < m_splineKnots.size(); ++i) {
        size_t h = i - 1;
        size_t j = i + 1;
        size_t k = i + 2;

        std::vector<Vector3> knots = {
            m_splineNodes[m_splineKnots[h]].position,
            m_splineNodes[m_splineKnots[i]].position,
            m_splineNodes[m_splineKnots[j]].position,
            m_splineNodes[m_splineKnots[k]].position
        };
        interpolateSegment(knots, m_splineKnots[i], m_splineKnots[j]);
    }

    return true;
}

bool CentripetalCatmullRomSpline::interpolateClosed()
{
    if (m_splineKnots.size() < 3)
        return false;

    for (size_t h = 0; h < m_splineKnots.size(); ++h) {
        size_t i = (h + 1) % m_splineKnots.size();
        size_t j = (h + 2) % m_splineKnots.size();
        size_t k = (h + 3) % m_splineKnots.size();

        std::vector<Vector3> knots = {
            m_splineNodes[m_splineKnots[h]].position,
            m_splineNodes[m_splineKnots[i]].position,
            m_splineNodes[m_splineKnots[j]].position,
            m_splineNodes[m_splineKnots[k]].position
        };
        interpolateSegment(knots, m_splineKnots[i], m_splineKnots[j]);
    }

    return true;
}

}
