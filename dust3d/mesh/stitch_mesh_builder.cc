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
#include <dust3d/mesh/stitch_mesh_builder.h>
#include <unordered_set>

namespace dust3d {

StitchMeshBuilder::StitchMeshBuilder(std::vector<Spline>&& splines, bool frontClosed, bool backClosed, bool sideClosed)
{
    m_frontClosed = frontClosed;
    m_backClosed = backClosed;
    m_sideClosed = sideClosed;
    m_splines = std::move(splines);
}

const std::vector<Vector3>& StitchMeshBuilder::generatedVertices() const
{
    return m_generatedVertices;
}

const std::vector<Uuid>& StitchMeshBuilder::generatedVertexSources() const
{
    return m_generatedVertexSources;
}

const std::vector<std::vector<size_t>>& StitchMeshBuilder::generatedFaces() const
{
    return m_generatedFaces;
}

const std::vector<std::vector<Vector2>>& StitchMeshBuilder::generatedFaceUvs() const
{
    return m_generatedFaceUvs;
}

bool StitchMeshBuilder::interpolateSplinesToHaveEqualSizeOfNodes()
{
    std::vector<Spline> interpolatedSplines = m_splines;
    for (size_t i = 0; i < m_splines.size(); ++i) {
        const auto& spline = m_splines[i];
        std::vector<Vector3> polyline(spline.nodes.size());
        std::vector<double> radiuses(spline.nodes.size());
        for (size_t j = 0; j < spline.nodes.size(); ++j) {
            polyline[j] = spline.nodes[j].origin;
            radiuses[j] = spline.nodes[j].radius;
        }
        std::vector<Vector3> segmentPoints;
        std::vector<double> segmentRadiuses;
        splitPolylineToSegments(polyline, radiuses, m_targetSegments, &segmentPoints, &segmentRadiuses);
        if (segmentPoints.size() != m_targetSegments + 1) {
            dust3dDebug << "Interpolate spline failed";
            return false;
        }
        auto& interpolatedSpline = interpolatedSplines[i];
        interpolatedSpline.nodes.resize(segmentPoints.size());
        for (size_t j = 0; j < interpolatedSpline.nodes.size(); ++j) {
            interpolatedSpline.nodes[j].origin = segmentPoints[j];
            interpolatedSpline.nodes[j].radius = segmentRadiuses[j];
        }
    }
    m_splines = std::move(interpolatedSplines);
    return true;
}

double StitchMeshBuilder::segmentsLength(const std::vector<Vector3>& segmentPoints)
{
    double totalLength = 0.0;
    for (size_t j = 1; j < segmentPoints.size(); ++j) {
        size_t i = j - 1;
        totalLength += (segmentPoints[i] - segmentPoints[j]).length();
    }
    return totalLength;
}

void StitchMeshBuilder::splitPolylineToSegments(const std::vector<Vector3>& polyline,
    const std::vector<double>& radiuses,
    size_t targetSegments,
    std::vector<Vector3>* targetPoints,
    std::vector<double>* targetRadiuses)
{
    if (polyline.size() < 2)
        return;
    double totalLength = segmentsLength(polyline);
    if (Math::isZero(totalLength)) {
        Vector3 middle = (polyline.front() + polyline.back()) * 0.5;
        double radius = (radiuses.front() + radiuses.back()) * 0.5;
        for (size_t i = 0; i < targetSegments + 1; ++i) {
            targetPoints->push_back(middle);
            targetRadiuses->push_back(radius);
        }
        return;
    }
    double segmentLength = totalLength / targetSegments;
    double wantLength = segmentLength;
    targetPoints->push_back(polyline.front());
    targetRadiuses->push_back(radiuses.front());
    for (size_t j = 1; j < polyline.size(); ++j) {
        size_t i = j - 1;
        double lineLength = (polyline[j] - polyline[i]).length();
        Vector3 polylineStart = polyline[i];
        double radiusStart = radiuses[i];
        while (true) {
            if (lineLength < wantLength) {
                wantLength -= lineLength;
                break;
            }
            Vector3 newPosition = (polylineStart * (lineLength - wantLength) + polyline[j] * wantLength) / lineLength;
            double newRadius = (radiusStart * (lineLength - wantLength) + radiuses[j] * wantLength) / lineLength;
            targetPoints->push_back(newPosition);
            targetRadiuses->push_back(newRadius);
            lineLength -= wantLength;
            polylineStart = newPosition;
            radiusStart = newRadius;
            wantLength = segmentLength;
        }
    }
    if (targetPoints->size() < targetSegments + 1) {
        targetPoints->push_back(polyline.back());
        targetRadiuses->push_back(radiuses.back());
    } else {
        targetPoints->back() = polyline.back();
        targetRadiuses->back() = radiuses.back();
    }
}

void StitchMeshBuilder::generateMeshFromStitchingPoints(const std::vector<std::vector<StitchMeshBuilder::StitchingPoint>>& stitchingPoints)
{
    for (size_t j = 1; j < stitchingPoints.size(); ++j) {
        size_t i = j - 1;
        generateMeshFromStitchingPoints(stitchingPoints[i], stitchingPoints[j]);
    }
}

void StitchMeshBuilder::generateMeshFromStitchingPoints(const std::vector<StitchMeshBuilder::StitchingPoint>& a,
    const std::vector<StitchMeshBuilder::StitchingPoint>& b)
{
    if (a.size() != b.size()) {
        dust3dDebug << "Unmatch sizes of stitching points, a:" << a.size() << "b:" << b.size();
        return;
    }
    if (a.size() < 2) {
        dust3dDebug << "Expected at least two stitching points, current:" << a.size();
        return;
    }
    std::vector<double> us(a.size());
    for (size_t j = 1; j < a.size(); ++j) {
        us[j] = us[j - 1] + (m_generatedVertices[a[j].originVertex] - m_generatedVertices[a[j - 1].originVertex]).length();
    }
    double totalLength = std::max(us.back(), std::numeric_limits<double>::epsilon());
    for (size_t j = 1; j < a.size(); ++j) {
        us[j] /= totalLength;
    }
    for (size_t j = 1; j < a.size(); ++j) {
        size_t i = j - 1;
        addQuadButMaybeTriangle(std::vector<size_t> {
                                    a[i].originVertex,
                                    b[i].originVertex,
                                    b[j].originVertex,
                                    a[j].originVertex },
            std::vector<Vector2> { Vector2(us[i], a[i].v), Vector2(us[i], b[i].v), Vector2(us[j], b[j].v), Vector2(us[j], a[j].v) });
    }
}

void StitchMeshBuilder::addQuadButMaybeTriangle(const std::vector<size_t>& quadButMaybeTriangle, const std::vector<Vector2>& quadUv)
{
    std::vector<size_t> finalFace;
    std::vector<Vector2> finalUv;
    std::unordered_set<size_t> used;
    finalFace.reserve(4);
    finalUv.reserve(4);
    for (size_t i = 0; i < quadButMaybeTriangle.size(); ++i) {
        const auto& it = quadButMaybeTriangle[i];
        auto insertResult = used.insert(it);
        if (insertResult.second) {
            finalFace.push_back(it);
            finalUv.push_back(quadUv[i]);
        }
    }
    if (finalFace.size() < 3)
        return;
    m_generatedFaces.emplace_back(finalFace);
    m_generatedFaceUvs.emplace_back(finalUv);
}

const std::vector<StitchMeshBuilder::Spline>& StitchMeshBuilder::splines() const
{
    return m_splines;
}

void StitchMeshBuilder::build()
{
    if (m_splines.empty())
        return;

    // Calculate target segments
    m_targetSegments = 1;
    for (const auto& it : m_splines) {
        m_targetSegments = std::max(m_targetSegments, it.nodes.size() - 1);
    }
    if (1 == m_targetSegments % 2)
        ++m_targetSegments;

    // Interpolate splines to make sure every spline have equal size of nodes
    if (!interpolateSplinesToHaveEqualSizeOfNodes())
        return;

    std::vector<std::vector<size_t>> stitchingPoints(m_splines.front().nodes.size() + 1);
    for (size_t i = 1; i < stitchingPoints.size(); ++i) {
        stitchingPoints[i].resize(m_splines.size());
        for (size_t k = 0; k < m_splines.size(); ++k) {
            stitchingPoints[i][k] = m_generatedVertices.size();
            m_generatedVertexSources.push_back(m_splines[k].nodes[i - 1].sourceId);
            m_generatedVertices.push_back(m_splines[k].nodes[i - 1].origin);
        }
    }

    if (m_sideClosed) {
        stitchingPoints[0].resize(m_splines.size());
        for (size_t k = 0; k < m_splines.size(); ++k) {
            stitchingPoints[0][k] = m_generatedVertices.size();
            m_generatedVertexSources.push_back(m_splines[k].sourceId);
            m_generatedVertices.push_back((m_splines[k].nodes.front().origin + m_splines[k].nodes.back().origin) * 0.5);
        }
        for (size_t i = 0; i < stitchingPoints.size(); ++i) {
            size_t j = (i + 1) % stitchingPoints.size();
            for (size_t n = 1; n < m_splines.size(); ++n) {
                size_t m = n - 1;
                m_generatedFaces.push_back(std::vector<size_t> {
                    stitchingPoints[i][m],
                    stitchingPoints[i][n],
                    stitchingPoints[j][n],
                    stitchingPoints[j][m] });
            }
        }
        // TODO:
    } else {
        for (size_t j = 2; j < stitchingPoints.size(); ++j) {
            size_t i = j - 1;
            for (size_t n = 1; n < m_splines.size(); ++n) {
                size_t m = n - 1;
                m_generatedFaces.push_back(std::vector<size_t> {
                    stitchingPoints[i][m],
                    stitchingPoints[i][n],
                    stitchingPoints[j][n],
                    stitchingPoints[j][m] });
            }
        }
        stitchingPoints[0].resize(m_splines.size());
        if (m_frontClosed) {
            stitchingPoints[0][0] = m_generatedVertices.size();
            m_generatedVertexSources.push_back(m_splines[0].sourceId);
            m_generatedVertices.push_back((m_splines[0].nodes.front().origin + m_splines[0].nodes.back().origin) * 0.5);
        }
        if (m_backClosed) {
            stitchingPoints[0][m_splines.size() - 1] = m_generatedVertices.size();
            m_generatedVertexSources.push_back(m_splines[m_splines.size() - 1].sourceId);
            m_generatedVertices.push_back((m_splines[m_splines.size() - 1].nodes.front().origin + m_splines[m_splines.size() - 1].nodes.back().origin) * 0.5);
        }
        // TODO:
    }
    if (m_frontClosed) {
        size_t lastMiddleVertexIndex = stitchingPoints[0][0];
        int left = 2, right = (int)stitchingPoints.size() - 2;
        for (;
             left + 1 < right;
             ++left, --right) {
            size_t middleVertexIndex = m_generatedVertices.size();
            m_generatedVertexSources.push_back(m_splines.front().sourceId);
            m_generatedVertices.push_back((m_generatedVertices[stitchingPoints[left][0]] + m_generatedVertices[stitchingPoints[right][0]]) * 0.5);
            m_generatedFaces.push_back(std::vector<size_t> {
                stitchingPoints[left - 1][0],
                stitchingPoints[left][0],
                middleVertexIndex,
                lastMiddleVertexIndex });
            m_generatedFaces.push_back(std::vector<size_t> {
                stitchingPoints[right + 1][0],
                lastMiddleVertexIndex,
                middleVertexIndex,
                stitchingPoints[right][0] });
            lastMiddleVertexIndex = middleVertexIndex;
        }
        if (left == right) {
            m_generatedFaces.push_back(std::vector<size_t> {
                stitchingPoints[left - 1][0],
                stitchingPoints[left][0],
                lastMiddleVertexIndex });
            m_generatedFaces.push_back(std::vector<size_t> {
                stitchingPoints[right + 1][0],
                lastMiddleVertexIndex,
                stitchingPoints[right][0] });
        }
    }
    if (m_backClosed) {
        size_t lastMiddleVertexIndex = stitchingPoints[0][m_splines.size() - 1];
        int right = 2, left = (int)stitchingPoints.size() - 2;
        for (;
             right + 1 < left;
             --left, ++right) {
            size_t middleVertexIndex = m_generatedVertices.size();
            m_generatedVertexSources.push_back(m_splines.front().sourceId);
            m_generatedVertices.push_back((m_generatedVertices[stitchingPoints[left][m_splines.size() - 1]] + m_generatedVertices[stitchingPoints[right][m_splines.size() - 1]]) * 0.5);
            m_generatedFaces.push_back(std::vector<size_t> {
                stitchingPoints[left + 1][m_splines.size() - 1],
                stitchingPoints[left][m_splines.size() - 1],
                middleVertexIndex,
                lastMiddleVertexIndex });
            m_generatedFaces.push_back(std::vector<size_t> {
                stitchingPoints[right - 1][m_splines.size() - 1],
                lastMiddleVertexIndex,
                middleVertexIndex,
                stitchingPoints[right][m_splines.size() - 1] });
            lastMiddleVertexIndex = middleVertexIndex;
        }
        if (left == right) {
            m_generatedFaces.push_back(std::vector<size_t> {
                stitchingPoints[left + 1][m_splines.size() - 1],
                stitchingPoints[left][m_splines.size() - 1],
                lastMiddleVertexIndex });
            m_generatedFaces.push_back(std::vector<size_t> {
                stitchingPoints[right - 1][m_splines.size() - 1],
                lastMiddleVertexIndex,
                stitchingPoints[right][m_splines.size() - 1] });
        }
    }
    // TODO:
}

}
