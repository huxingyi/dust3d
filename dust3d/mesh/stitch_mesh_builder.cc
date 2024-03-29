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

StitchMeshBuilder::StitchMeshBuilder(std::vector<Spline>&& splines,
    bool frontClosed,
    bool backClosed,
    bool sideClosed,
    size_t targetSegments)
{
    m_frontClosed = frontClosed;
    m_backClosed = backClosed;
    m_sideClosed = sideClosed;
    m_targetSegments = targetSegments;
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

const std::vector<StitchMeshBuilder::Spline>& StitchMeshBuilder::splines() const
{
    return m_splines;
}

void StitchMeshBuilder::build()
{
    if (m_splines.size() < 2)
        return;

    if (0 == m_targetSegments) {
        // Calculate target segments
        m_targetSegments = 1;
        for (const auto& it : m_splines) {
            m_targetSegments = std::max(m_targetSegments, it.nodes.size() - 1);
        }
    }
    if (1 == m_targetSegments % 2)
        ++m_targetSegments;

    // Interpolate splines to make sure every spline have equal size of nodes
    if (!interpolateSplinesToHaveEqualSizeOfNodes())
        return;

    std::vector<std::vector<size_t>> stitchingPoints(m_splines.front().nodes.size() + 1);
    std::vector<std::vector<double>> stitchingRadiuses(m_splines.front().nodes.size() + 1);
    std::vector<std::vector<size_t>> innerPoints(stitchingPoints.size());
    std::vector<std::vector<Vector2>> uvs(stitchingPoints.size() + 1, std::vector<Vector2>(m_splines.size()));
    stitchingPoints[0].resize(m_splines.size());
    stitchingRadiuses[0].resize(m_splines.size());
    for (size_t i = 1; i < stitchingPoints.size(); ++i) {
        stitchingPoints[i].resize(m_splines.size());
        stitchingRadiuses[i].resize(m_splines.size());
        for (size_t k = 0; k < m_splines.size(); ++k) {
            stitchingPoints[i][k] = m_generatedVertices.size();
            stitchingRadiuses[i][k] = m_splines[k].nodes[i - 1].radius;
            m_generatedVertexSources.push_back(m_splines[k].nodes[i - 1].sourceId);
            m_generatedVertices.push_back(m_splines[k].nodes[i - 1].origin);
        }
    }
    if (m_sideClosed) {
        for (size_t k = 0; k < m_splines.size(); ++k) {
            stitchingPoints[0][k] = m_generatedVertices.size();
            stitchingRadiuses[0][k] = (m_splines[k].nodes.front().radius + m_splines[k].nodes.back().radius) * 0.5;
            m_generatedVertexSources.push_back(m_splines[k].sourceId);
            m_generatedVertices.push_back((m_splines[k].nodes.front().origin + m_splines[k].nodes.back().origin) * 0.5);
        }
    } else {
        stitchingPoints[0].resize(m_splines.size());
        if (m_frontClosed) {
            stitchingPoints[0][0] = m_generatedVertices.size();
            stitchingRadiuses[0][0] = (m_splines[0].nodes.front().radius + m_splines[0].nodes.back().radius) * 0.5;
            m_generatedVertexSources.push_back(m_splines[0].sourceId);
            m_generatedVertices.push_back((m_splines[0].nodes.front().origin + m_splines[0].nodes.back().origin) * 0.5);
        }
        if (m_backClosed) {
            stitchingPoints[0][m_splines.size() - 1] = m_generatedVertices.size();
            stitchingRadiuses[0][m_splines.size() - 1] = (m_splines[m_splines.size() - 1].nodes.front().radius + m_splines[m_splines.size() - 1].nodes.back().radius) * 0.5;
            m_generatedVertexSources.push_back(m_splines[m_splines.size() - 1].sourceId);
            m_generatedVertices.push_back((m_splines[m_splines.size() - 1].nodes.front().origin + m_splines[m_splines.size() - 1].nodes.back().origin) * 0.5);
        }
    }

    // Calculate V of UV
    if (m_frontClosed && m_backClosed) {
        for (size_t i = 0; i < stitchingPoints.size(); ++i) {
            if (i > 0)
                uvs[i][0][1] = (m_generatedVertices[stitchingPoints[i][0]] - m_generatedVertices[stitchingPoints[stitchingPoints.size() - i][0]]).length() * 0.5;
            for (size_t n = 1; n < m_splines.size(); ++n) {
                uvs[i][n][1] = uvs[i][n - 1][1] + (m_generatedVertices[stitchingPoints[i][n]] - m_generatedVertices[stitchingPoints[i][n - 1]]).length();
            }
            auto totalLength = uvs[i][m_splines.size() - 1][1];
            if (i > 0)
                totalLength += (m_generatedVertices[stitchingPoints[i][m_splines.size() - 1]] - m_generatedVertices[stitchingPoints[stitchingPoints.size() - i][m_splines.size() - 1]]).length() * 0.5;
            for (size_t n = 0; n < m_splines.size(); ++n) {
                uvs[i][n][1] /= totalLength;
            }
        }
    } else if (m_backClosed) {
        // Front open
        for (size_t i = 0; i < stitchingPoints.size(); ++i) {
            uvs[i][0][1] = stitchingRadiuses[i][0];
            for (size_t n = 1; n < m_splines.size(); ++n) {
                uvs[i][n][1] = uvs[i][n - 1][1] + (m_generatedVertices[stitchingPoints[i][n]] - m_generatedVertices[stitchingPoints[i][n - 1]]).length();
            }
            auto totalLength = uvs[i][m_splines.size() - 1][1];
            if (i > 0)
                totalLength += (m_generatedVertices[stitchingPoints[i][m_splines.size() - 1]] - m_generatedVertices[stitchingPoints[stitchingPoints.size() - i][m_splines.size() - 1]]).length() * 0.5;
            for (size_t n = 0; n < m_splines.size(); ++n) {
                uvs[i][n][1] /= totalLength;
            }
        }
    } else if (m_frontClosed) {
        // Back open
        for (size_t i = 0; i < stitchingPoints.size(); ++i) {
            if (i > 0)
                uvs[i][0][1] = (m_generatedVertices[stitchingPoints[i][0]] - m_generatedVertices[stitchingPoints[stitchingPoints.size() - i][0]]).length() * 0.5;
            for (size_t n = 1; n < m_splines.size(); ++n) {
                uvs[i][n][1] = uvs[i][n - 1][1] + (m_generatedVertices[stitchingPoints[i][n]] - m_generatedVertices[stitchingPoints[i][n - 1]]).length();
            }
            auto totalLength = uvs[i][m_splines.size() - 1][1] + stitchingRadiuses[i][m_splines.size() - 1];
            for (size_t n = 0; n < m_splines.size(); ++n) {
                uvs[i][n][1] /= totalLength;
            }
        }
    } else {
        // Front open and back open
        for (size_t i = 0; i < stitchingPoints.size(); ++i) {
            uvs[i][0][1] = stitchingRadiuses[i][0];
            for (size_t n = 1; n < m_splines.size(); ++n) {
                uvs[i][n][1] = uvs[i][n - 1][1] + (m_generatedVertices[stitchingPoints[i][n]] - m_generatedVertices[stitchingPoints[i][n - 1]]).length();
            }
            auto totalLength = uvs[i][m_splines.size() - 1][1] + stitchingRadiuses[i][m_splines.size() - 1];
            for (size_t n = 0; n < m_splines.size(); ++n) {
                uvs[i][n][1] /= totalLength;
            }
        }
    }

    // Calculate U of UV
    if (m_sideClosed) {
        for (size_t n = 0; n < m_splines.size(); ++n) {
            for (size_t i = 1; i < stitchingPoints.size(); ++i) {
                uvs[i][n][0] = uvs[i - 1][n][0] + (m_generatedVertices[stitchingPoints[i][n]] - m_generatedVertices[stitchingPoints[i - 1][n]]).length();
            }
            auto totalLength = uvs[stitchingPoints.size() - 1][n][0] + (m_generatedVertices[stitchingPoints[stitchingPoints.size() - 1][n]] - m_generatedVertices[stitchingPoints[0][n]]).length();
            for (size_t i = 0; i < stitchingPoints.size(); ++i) {
                uvs[i][n][0] /= totalLength;
            }
            uvs[stitchingPoints.size()][n][0] = 1.0;
            uvs[stitchingPoints.size()][n][1] = uvs[0][n][1];
        }
    } else {
        for (size_t n = 0; n < m_splines.size(); ++n) {
            uvs[1][n][0] = stitchingRadiuses[1][n];
            for (size_t i = 2; i < stitchingPoints.size(); ++i) {
                uvs[i][n][0] = uvs[i - 1][n][0] + (m_generatedVertices[stitchingPoints[i][n]] - m_generatedVertices[stitchingPoints[i - 1][n]]).length();
            }
            auto totalLength = uvs[stitchingPoints.size() - 1][n][0] + stitchingRadiuses[m_splines.size() - 1][n];
            for (size_t i = 0; i < stitchingPoints.size(); ++i) {
                uvs[i][n][0] /= totalLength;
            }
        }
    }

    std::vector<std::vector<Vector2>> innerUvs = uvs;
    if (!m_sideClosed) {
        if (!m_backClosed) {
            for (size_t i = 0; i < stitchingPoints.size(); ++i) {
                innerUvs[i][m_splines.size() - 1][1] = 1.0;
            }
        }
        if (!m_frontClosed) {
            for (size_t i = 0; i < stitchingPoints.size(); ++i) {
                innerUvs[i][0][1] = 0.0;
            }
        }
        for (size_t n = 1; n + 1 < m_splines.size(); ++n) {
            innerUvs[1][n][0] = 0.0;
            innerUvs[stitchingPoints.size() - 1][n][0] = 1.0;
        }
    }

    if (m_sideClosed) {
        for (size_t i = 0; i < stitchingPoints.size(); ++i) {
            size_t j = (i + 1) % stitchingPoints.size();
            for (size_t n = 1; n < m_splines.size(); ++n) {
                size_t m = n - 1;
                m_generatedFaces.push_back(std::vector<size_t> {
                    stitchingPoints[i][n],
                    stitchingPoints[i][m],
                    stitchingPoints[j][m],
                    stitchingPoints[j][n] });
                m_generatedFaceUvs.push_back(std::vector<Vector2> {
                    uvs[i][n],
                    uvs[i][m],
                    uvs[i + 1][m],
                    uvs[i + 1][n] });
            }
        }
    } else {
        for (size_t j = 2; j < stitchingPoints.size(); ++j) {
            size_t i = j - 1;
            for (size_t n = 1; n < m_splines.size(); ++n) {
                size_t m = n - 1;
                m_generatedFaces.push_back(std::vector<size_t> {
                    stitchingPoints[i][n],
                    stitchingPoints[i][m],
                    stitchingPoints[j][m],
                    stitchingPoints[j][n] });
                m_generatedFaceUvs.push_back(std::vector<Vector2> {
                    uvs[i][n],
                    uvs[i][m],
                    uvs[i + 1][m],
                    uvs[i + 1][n] });
            }
        }
    }

    // Generate normal for stitching points
    std::vector<Vector3> normals;
    normals.resize(m_generatedVertices.size());
    for (const auto& quad : m_generatedFaces) {
        if (4 != quad.size())
            continue;
        for (size_t i = 0; i < 4; ++i) {
            size_t j = (i + 1) % 4;
            size_t k = (j + 1) % 4;
            auto faceNormal = Vector3::normal(m_generatedVertices[quad[i]], m_generatedVertices[quad[j]], m_generatedVertices[quad[k]]);
            normals[quad[i]] += faceNormal;
            normals[quad[j]] += faceNormal;
            normals[quad[k]] += faceNormal;
        }
    }
    for (auto& it : normals)
        it.normalize();

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
                middleVertexIndex,
                stitchingPoints[left][0],
                stitchingPoints[left - 1][0],
                lastMiddleVertexIndex });
            m_generatedFaceUvs.push_back(std::vector<Vector2> {
                Vector2(uvs[left][0][0], 0.0),
                uvs[left][0],
                uvs[left - 1][0],
                Vector2(uvs[left - 1][0][0], 0.0) });
            m_generatedFaces.push_back(std::vector<size_t> {
                middleVertexIndex,
                lastMiddleVertexIndex,
                stitchingPoints[right + 1][0],
                stitchingPoints[right][0] });
            m_generatedFaceUvs.push_back(std::vector<Vector2> {
                Vector2(uvs[right][0][0], 0.0),
                Vector2(uvs[right + 1][0][0], 0.0),
                uvs[right + 1][0],
                uvs[right][0] });
            lastMiddleVertexIndex = middleVertexIndex;
        }
        if (left == right) {
            m_generatedFaces.push_back(std::vector<size_t> {
                lastMiddleVertexIndex,
                stitchingPoints[left][0],
                stitchingPoints[left - 1][0] });
            m_generatedFaceUvs.push_back(std::vector<Vector2> {
                Vector2(uvs[left - 1][0][0], 0.0),
                uvs[left][0],
                uvs[left - 1][0] });
            m_generatedFaces.push_back(std::vector<size_t> {
                lastMiddleVertexIndex,
                stitchingPoints[right + 1][0],
                stitchingPoints[right][0] });
            m_generatedFaceUvs.push_back(std::vector<Vector2> {
                Vector2(uvs[right + 1][0][0], 0.0),
                uvs[right + 1][0],
                uvs[right][0] });
        }
    }
    if (m_backClosed) {
        size_t lastMiddleVertexIndex = stitchingPoints[0][m_splines.size() - 1];
        int right = 2, left = (int)stitchingPoints.size() - 2;
        for (;
             right + 1 < left;
             --left, ++right) {
            size_t middleVertexIndex = m_generatedVertices.size();
            m_generatedVertexSources.push_back(m_splines.back().sourceId);
            m_generatedVertices.push_back((m_generatedVertices[stitchingPoints[left][m_splines.size() - 1]] + m_generatedVertices[stitchingPoints[right][m_splines.size() - 1]]) * 0.5);
            m_generatedFaces.push_back(std::vector<size_t> {
                middleVertexIndex,
                stitchingPoints[left][m_splines.size() - 1],
                stitchingPoints[left + 1][m_splines.size() - 1],
                lastMiddleVertexIndex });
            m_generatedFaceUvs.push_back(std::vector<Vector2> {
                Vector2(uvs[left][m_splines.size() - 1][0], 1.0),
                uvs[left][m_splines.size() - 1],
                uvs[left + 1][m_splines.size() - 1],
                Vector2(uvs[left + 1][m_splines.size() - 1][0], 1.0) });
            m_generatedFaces.push_back(std::vector<size_t> {
                middleVertexIndex,
                lastMiddleVertexIndex,
                stitchingPoints[right - 1][m_splines.size() - 1],
                stitchingPoints[right][m_splines.size() - 1] });
            m_generatedFaceUvs.push_back(std::vector<Vector2> {
                Vector2(uvs[right][m_splines.size() - 1][0], 1.0),
                Vector2(uvs[right - 1][m_splines.size() - 1][0], 1.0),
                uvs[right - 1][m_splines.size() - 1],
                uvs[right][m_splines.size() - 1] });
            lastMiddleVertexIndex = middleVertexIndex;
        }
        if (left == right) {
            m_generatedFaces.push_back(std::vector<size_t> {
                lastMiddleVertexIndex,
                stitchingPoints[left][m_splines.size() - 1],
                stitchingPoints[left + 1][m_splines.size() - 1] });
            m_generatedFaceUvs.push_back(std::vector<Vector2> {
                Vector2(uvs[left + 1][m_splines.size() - 1][0], 1.0),
                uvs[left][m_splines.size() - 1],
                uvs[left + 1][m_splines.size() - 1] });
            m_generatedFaces.push_back(std::vector<size_t> {
                lastMiddleVertexIndex,
                stitchingPoints[right - 1][m_splines.size() - 1],
                stitchingPoints[right][m_splines.size() - 1] });
            m_generatedFaceUvs.push_back(std::vector<Vector2> {
                Vector2(uvs[right - 1][m_splines.size() - 1][0], 1.0),
                uvs[right - 1][m_splines.size() - 1],
                uvs[right][m_splines.size() - 1] });
        }
    }

    // Make thick layer
    if (m_sideClosed) {
        if (m_frontClosed && m_backClosed) {
            // All good, whole space closed, no thick layer needed
        } else {
            if (m_backClosed) {
                // Front open
                for (size_t i = 0; i < stitchingPoints.size(); ++i) {
                    innerPoints[i].resize(m_splines.size());
                    for (size_t k = 0; k < m_splines.size(); ++k) {
                        Vector3 offset;
                        if (k == m_splines.size() - 1) {
                            offset = (m_generatedVertices[stitchingPoints[i][k - 1]] - m_generatedVertices[stitchingPoints[i][k]]).normalized() * stitchingRadiuses[i][k];
                        }
                        innerPoints[i][k] = m_generatedVertices.size();
                        m_generatedVertexSources.push_back(m_generatedVertexSources[stitchingPoints[i][k]]);
                        m_generatedVertices.push_back(m_generatedVertices[stitchingPoints[i][k]] - normals[stitchingPoints[i][k]] * stitchingRadiuses[i][k] + offset);
                    }
                }
                for (size_t i = 0; i < innerPoints.size(); ++i) {
                    size_t j = (i + 1) % innerPoints.size();
                    m_generatedFaces.push_back(std::vector<size_t> {
                        innerPoints[j][0],
                        stitchingPoints[j][0],
                        stitchingPoints[i][0],
                        innerPoints[i][0] });
                    m_generatedFaceUvs.push_back(std::vector<Vector2> {
                        Vector2(uvs[i + 1][0][0], 0.0),
                        uvs[i + 1][0],
                        uvs[i][0],
                        Vector2(uvs[i][0][0], 0.0) });
                }
                for (size_t i = 0; i < innerPoints.size(); ++i) {
                    size_t j = (i + 1) % innerPoints.size();
                    for (size_t n = 1; n < m_splines.size(); ++n) {
                        size_t m = n - 1;
                        m_generatedFaces.push_back(std::vector<size_t> {
                            innerPoints[i][n],
                            innerPoints[j][n],
                            innerPoints[j][m],
                            innerPoints[i][m] });
                        m_generatedFaceUvs.push_back(std::vector<Vector2> {
                            innerUvs[i][n],
                            innerUvs[i + 1][n],
                            innerUvs[i + 1][m],
                            innerUvs[i][m] });
                    }
                }
                // Close lid
                size_t lastMiddleVertexIndex = innerPoints[0][m_splines.size() - 1];
                int left = 2, right = (int)innerPoints.size() - 2;
                for (;
                     left + 1 < right;
                     ++left, --right) {
                    size_t middleVertexIndex = m_generatedVertices.size();
                    m_generatedVertexSources.push_back(m_splines[m_splines.size() - 1].sourceId);
                    m_generatedVertices.push_back((m_generatedVertices[innerPoints[left][m_splines.size() - 1]] + m_generatedVertices[innerPoints[right][m_splines.size() - 1]]) * 0.5);
                    m_generatedFaces.push_back(std::vector<size_t> {
                        middleVertexIndex,
                        innerPoints[left][m_splines.size() - 1],
                        innerPoints[left - 1][m_splines.size() - 1],
                        lastMiddleVertexIndex });
                    m_generatedFaceUvs.push_back(std::vector<Vector2> {
                        Vector2(innerUvs[left][m_splines.size() - 1][0], 1.0),
                        innerUvs[left][m_splines.size() - 1],
                        innerUvs[left - 1][m_splines.size() - 1],
                        Vector2(innerUvs[left - 1][m_splines.size() - 1][0], 1.0) });
                    m_generatedFaces.push_back(std::vector<size_t> {
                        middleVertexIndex,
                        lastMiddleVertexIndex,
                        innerPoints[right + 1][m_splines.size() - 1],
                        innerPoints[right][m_splines.size() - 1] });
                    m_generatedFaceUvs.push_back(std::vector<Vector2> {
                        Vector2(innerUvs[right][m_splines.size() - 1][0], 1.0),
                        Vector2(innerUvs[right + 1][m_splines.size() - 1][0], 1.0),
                        innerUvs[right + 1][m_splines.size() - 1],
                        innerUvs[right][m_splines.size() - 1] });
                    lastMiddleVertexIndex = middleVertexIndex;
                }
                if (left == right) {
                    m_generatedFaces.push_back(std::vector<size_t> {
                        lastMiddleVertexIndex,
                        innerPoints[left][m_splines.size() - 1],
                        innerPoints[left - 1][m_splines.size() - 1] });
                    m_generatedFaceUvs.push_back(std::vector<Vector2> {
                        Vector2(innerUvs[left - 1][m_splines.size() - 1][0], 1.0),
                        innerUvs[left][m_splines.size() - 1],
                        innerUvs[left - 1][m_splines.size() - 1] });
                    m_generatedFaces.push_back(std::vector<size_t> {
                        lastMiddleVertexIndex,
                        innerPoints[right + 1][m_splines.size() - 1],
                        innerPoints[right][m_splines.size() - 1] });
                    m_generatedFaceUvs.push_back(std::vector<Vector2> {
                        Vector2(innerUvs[right + 1][m_splines.size() - 1][0], 1.0),
                        innerUvs[right + 1][m_splines.size() - 1],
                        innerUvs[right][m_splines.size() - 1] });
                }
            } else if (m_frontClosed) {
                // Back open
                for (size_t i = 0; i < stitchingPoints.size(); ++i) {
                    innerPoints[i].resize(m_splines.size());
                    for (size_t k = 0; k < m_splines.size(); ++k) {
                        Vector3 offset;
                        if (0 == k) {
                            offset = (m_generatedVertices[stitchingPoints[i][k + 1]] - m_generatedVertices[stitchingPoints[i][k]]).normalized() * stitchingRadiuses[i][k];
                        }
                        innerPoints[i][k] = m_generatedVertices.size();
                        m_generatedVertexSources.push_back(m_generatedVertexSources[stitchingPoints[i][k]]);
                        m_generatedVertices.push_back(m_generatedVertices[stitchingPoints[i][k]] - normals[stitchingPoints[i][k]] * stitchingRadiuses[i][k] + offset);
                    }
                }
                for (size_t i = 0; i < innerPoints.size(); ++i) {
                    size_t j = (i + 1) % innerPoints.size();
                    m_generatedFaces.push_back(std::vector<size_t> {
                        innerPoints[i][m_splines.size() - 1],
                        stitchingPoints[i][m_splines.size() - 1],
                        stitchingPoints[j][m_splines.size() - 1],
                        innerPoints[j][m_splines.size() - 1] });
                    m_generatedFaceUvs.push_back(std::vector<Vector2> {
                        Vector2(uvs[i][m_splines.size() - 1][0], 1.0),
                        uvs[i][m_splines.size() - 1],
                        uvs[i + 1][m_splines.size() - 1],
                        Vector2(uvs[i + 1][m_splines.size() - 1][0], 1.0) });
                }
                for (size_t i = 0; i < innerPoints.size(); ++i) {
                    size_t j = (i + 1) % innerPoints.size();
                    for (size_t n = 1; n < m_splines.size(); ++n) {
                        size_t m = n - 1;
                        m_generatedFaces.push_back(std::vector<size_t> {
                            innerPoints[i][n],
                            innerPoints[j][n],
                            innerPoints[j][m],
                            innerPoints[i][m] });
                        m_generatedFaceUvs.push_back(std::vector<Vector2> {
                            innerUvs[i][n],
                            innerUvs[i + 1][n],
                            innerUvs[i + 1][m],
                            innerUvs[i][m] });
                    }
                }
                // Close lid
                size_t lastMiddleVertexIndex = innerPoints[0][0];
                int left = 2, right = (int)innerPoints.size() - 2;
                for (;
                     left + 1 < right;
                     ++left, --right) {
                    size_t middleVertexIndex = m_generatedVertices.size();
                    m_generatedVertexSources.push_back(m_splines[0].sourceId);
                    m_generatedVertices.push_back((m_generatedVertices[innerPoints[left][0]] + m_generatedVertices[innerPoints[right][0]]) * 0.5);
                    m_generatedFaces.push_back(std::vector<size_t> {
                        middleVertexIndex,
                        lastMiddleVertexIndex,
                        innerPoints[left - 1][0],
                        innerPoints[left][0] });
                    m_generatedFaceUvs.push_back(std::vector<Vector2> {
                        Vector2(innerUvs[left][0][0], 0.0),
                        Vector2(innerUvs[left - 1][0][0], 0.0),
                        innerUvs[left - 1][0],
                        innerUvs[left][0] });
                    m_generatedFaces.push_back(std::vector<size_t> {
                        middleVertexIndex,
                        innerPoints[right][0],
                        innerPoints[right + 1][0],
                        lastMiddleVertexIndex });
                    m_generatedFaceUvs.push_back(std::vector<Vector2> {
                        Vector2(innerUvs[right][0][0], 0.0),
                        innerUvs[right][0],
                        innerUvs[right + 1][0],
                        Vector2(innerUvs[right + 1][0][0], 0.0) });
                    lastMiddleVertexIndex = middleVertexIndex;
                }
                if (left == right) {
                    m_generatedFaces.push_back(std::vector<size_t> {
                        lastMiddleVertexIndex,
                        innerPoints[left - 1][0],
                        innerPoints[left][0] });
                    m_generatedFaceUvs.push_back(std::vector<Vector2> {
                        Vector2(innerUvs[left - 1][0][0], 0.0),
                        innerUvs[left - 1][0],
                        innerUvs[left][0] });
                    m_generatedFaces.push_back(std::vector<size_t> {
                        lastMiddleVertexIndex,
                        innerPoints[right][0],
                        innerPoints[right + 1][0] });
                    m_generatedFaceUvs.push_back(std::vector<Vector2> {
                        Vector2(innerUvs[right + 1][0][0], 0.0),
                        innerUvs[right][0],
                        innerUvs[right + 1][0] });
                }
            } else {
                for (size_t i = 0; i < stitchingPoints.size(); ++i) {
                    innerPoints[i].resize(m_splines.size());
                    for (size_t k = 0; k < m_splines.size(); ++k) {
                        innerPoints[i][k] = m_generatedVertices.size();
                        m_generatedVertexSources.push_back(m_generatedVertexSources[stitchingPoints[i][k]]);
                        m_generatedVertices.push_back(m_generatedVertices[stitchingPoints[i][k]] - normals[stitchingPoints[i][k]] * stitchingRadiuses[i][k]);
                    }
                }
                for (size_t i = 0; i < innerPoints.size(); ++i) {
                    size_t j = (i + 1) % innerPoints.size();
                    m_generatedFaces.push_back(std::vector<size_t> {
                        innerPoints[j][0],
                        stitchingPoints[j][0],
                        stitchingPoints[i][0],
                        innerPoints[i][0] });
                    m_generatedFaceUvs.push_back(std::vector<Vector2> {
                        uvs[i + 1][0],
                        uvs[i + 1][0],
                        uvs[i][0],
                        uvs[i][0] });
                    m_generatedFaces.push_back(std::vector<size_t> {
                        innerPoints[i][m_splines.size() - 1],
                        stitchingPoints[i][m_splines.size() - 1],
                        stitchingPoints[j][m_splines.size() - 1],
                        innerPoints[j][m_splines.size() - 1] });
                    m_generatedFaceUvs.push_back(std::vector<Vector2> {
                        uvs[i][m_splines.size() - 1],
                        uvs[i][m_splines.size() - 1],
                        uvs[i + 1][m_splines.size() - 1],
                        uvs[i + 1][m_splines.size() - 1] });
                }
                for (size_t i = 0; i < innerPoints.size(); ++i) {
                    size_t j = (i + 1) % innerPoints.size();
                    for (size_t n = 1; n < m_splines.size(); ++n) {
                        size_t m = n - 1;
                        m_generatedFaces.push_back(std::vector<size_t> {
                            innerPoints[i][n],
                            innerPoints[j][n],
                            innerPoints[j][m],
                            innerPoints[i][m] });
                        m_generatedFaceUvs.push_back(std::vector<Vector2> {
                            innerUvs[i][n],
                            innerUvs[i + 1][n],
                            innerUvs[i + 1][m],
                            innerUvs[i][m] });
                    }
                }
            }
        }
    } else {
        // Side not closed
        if (m_frontClosed && m_backClosed) {
            for (size_t i = 1; i < stitchingPoints.size(); ++i) {
                innerPoints[i].resize(m_splines.size());
                for (size_t k = 0; k < m_splines.size(); ++k) {
                    Vector3 offset;
                    if (0 == k) {
                        offset = (m_generatedVertices[stitchingPoints[i][k + 1]] - m_generatedVertices[stitchingPoints[i][k]]).normalized() * stitchingRadiuses[i][k];
                    } else if (k == m_splines.size() - 1) {
                        offset = (m_generatedVertices[stitchingPoints[i][k - 1]] - m_generatedVertices[stitchingPoints[i][k]]).normalized() * stitchingRadiuses[i][k];
                    }
                    innerPoints[i][k] = m_generatedVertices.size();
                    m_generatedVertexSources.push_back(m_generatedVertexSources[stitchingPoints[i][k]]);
                    m_generatedVertices.push_back(m_generatedVertices[stitchingPoints[i][k]] - normals[stitchingPoints[i][k]] * stitchingRadiuses[i][k] + offset);
                }
            }
            innerPoints[0].resize(m_splines.size());
            innerPoints[0][0] = m_generatedVertices.size();
            m_generatedVertexSources.push_back(m_generatedVertexSources[stitchingPoints[0][0]]);
            m_generatedVertices.push_back((m_generatedVertices[innerPoints[1][0]] + m_generatedVertices[innerPoints[innerPoints.size() - 1][0]]) * 0.5);
            innerPoints[0][m_splines.size() - 1] = m_generatedVertices.size();
            m_generatedVertexSources.push_back(m_generatedVertexSources[stitchingPoints[0][m_splines.size() - 1]]);
            m_generatedVertices.push_back((m_generatedVertices[innerPoints[1][m_splines.size() - 1]] + m_generatedVertices[innerPoints[innerPoints.size() - 1][m_splines.size() - 1]]) * 0.5);
            for (size_t j = 2; j < innerPoints.size(); ++j) {
                size_t i = j - 1;
                for (size_t n = 1; n < m_splines.size(); ++n) {
                    size_t m = n - 1;
                    m_generatedFaces.push_back(std::vector<size_t> {
                        innerPoints[i][n],
                        innerPoints[j][n],
                        innerPoints[j][m],
                        innerPoints[i][m] });
                    m_generatedFaceUvs.push_back(std::vector<Vector2> {
                        innerUvs[i][n],
                        innerUvs[i + 1][n],
                        innerUvs[i + 1][m],
                        innerUvs[i][m] });
                }
            }
            for (size_t j = 1; j < m_splines.size(); ++j) {
                size_t i = j - 1;
                m_generatedFaces.push_back(std::vector<size_t> {
                    innerPoints[1][i],
                    stitchingPoints[1][i],
                    stitchingPoints[1][j],
                    innerPoints[1][j] });
                m_generatedFaceUvs.push_back(std::vector<Vector2> {
                    innerUvs[1][i],
                    uvs[1][i],
                    uvs[1][i + 1],
                    innerUvs[1][i + 1] });
                m_generatedFaces.push_back(std::vector<size_t> {
                    innerPoints[innerPoints.size() - 1][i],
                    innerPoints[innerPoints.size() - 1][j],
                    stitchingPoints[innerPoints.size() - 1][j],
                    stitchingPoints[innerPoints.size() - 1][i] });
                m_generatedFaceUvs.push_back(std::vector<Vector2> {
                    innerUvs[innerPoints.size() - 1][i],
                    innerUvs[innerPoints.size() - 1][j],
                    uvs[innerPoints.size() - 1][j],
                    uvs[innerPoints.size() - 1][i] });
            }
            // Close lid
            {
                size_t lastMiddleVertexIndex = innerPoints[0][0];
                int left = 2, right = (int)innerPoints.size() - 2;
                for (;
                     left + 1 < right;
                     ++left, --right) {
                    size_t middleVertexIndex = m_generatedVertices.size();
                    m_generatedVertexSources.push_back(m_splines.front().sourceId);
                    m_generatedVertices.push_back((m_generatedVertices[innerPoints[left][0]] + m_generatedVertices[innerPoints[right][0]]) * 0.5);
                    m_generatedFaces.push_back(std::vector<size_t> {
                        middleVertexIndex,
                        lastMiddleVertexIndex,
                        innerPoints[left - 1][0],
                        innerPoints[left][0] });
                    m_generatedFaceUvs.push_back(std::vector<Vector2> {
                        Vector2(innerUvs[left][0][0], 0.0),
                        Vector2(innerUvs[left - 1][0][0], 0.0),
                        innerUvs[left - 1][0],
                        innerUvs[left][0] });
                    m_generatedFaces.push_back(std::vector<size_t> {
                        middleVertexIndex,
                        innerPoints[right][0],
                        innerPoints[right + 1][0],
                        lastMiddleVertexIndex });
                    m_generatedFaceUvs.push_back(std::vector<Vector2> {
                        Vector2(innerUvs[right][0][0], 0.0),
                        innerUvs[right][0],
                        innerUvs[right + 1][0],
                        Vector2(innerUvs[right + 1][0][0], 0.0) });
                    lastMiddleVertexIndex = middleVertexIndex;
                }
                if (left == right) {
                    m_generatedFaces.push_back(std::vector<size_t> {
                        lastMiddleVertexIndex,
                        innerPoints[left - 1][0],
                        innerPoints[left][0] });
                    m_generatedFaceUvs.push_back(std::vector<Vector2> {
                        Vector2(innerUvs[left - 1][0][0], 0.0),
                        innerUvs[left - 1][0],
                        innerUvs[left][0] });
                    m_generatedFaces.push_back(std::vector<size_t> {
                        lastMiddleVertexIndex,
                        innerPoints[right][0],
                        innerPoints[right + 1][0] });
                    m_generatedFaceUvs.push_back(std::vector<Vector2> {
                        Vector2(innerUvs[right + 1][0][0], 0.0),
                        innerUvs[right][0],
                        innerUvs[right + 1][0] });
                }
            }
            {
                size_t lastMiddleVertexIndex = innerPoints[0][m_splines.size() - 1];
                int left = 2, right = (int)innerPoints.size() - 2;
                for (;
                     left + 1 < right;
                     ++left, --right) {
                    size_t middleVertexIndex = m_generatedVertices.size();
                    m_generatedVertexSources.push_back(m_splines.back().sourceId);
                    m_generatedVertices.push_back((m_generatedVertices[innerPoints[left][m_splines.size() - 1]] + m_generatedVertices[innerPoints[right][m_splines.size() - 1]]) * 0.5);
                    m_generatedFaces.push_back(std::vector<size_t> {
                        middleVertexIndex,
                        innerPoints[left][m_splines.size() - 1],
                        innerPoints[left - 1][m_splines.size() - 1],
                        lastMiddleVertexIndex });
                    m_generatedFaceUvs.push_back(std::vector<Vector2> {
                        Vector2(innerUvs[left][m_splines.size() - 1][0], 1.0),
                        innerUvs[left][m_splines.size() - 1],
                        innerUvs[left - 1][m_splines.size() - 1],
                        Vector2(innerUvs[left - 1][m_splines.size() - 1][0], 1.0) });
                    m_generatedFaces.push_back(std::vector<size_t> {
                        middleVertexIndex,
                        lastMiddleVertexIndex,
                        innerPoints[right + 1][m_splines.size() - 1],
                        innerPoints[right][m_splines.size() - 1] });
                    m_generatedFaceUvs.push_back(std::vector<Vector2> {
                        Vector2(innerUvs[right][m_splines.size() - 1][0], 1.0),
                        Vector2(innerUvs[right + 1][m_splines.size() - 1][0], 1.0),
                        innerUvs[right + 1][m_splines.size() - 1],
                        innerUvs[right][m_splines.size() - 1] });
                    lastMiddleVertexIndex = middleVertexIndex;
                }
                if (left == right) {
                    m_generatedFaces.push_back(std::vector<size_t> {
                        lastMiddleVertexIndex,
                        innerPoints[left][m_splines.size() - 1],
                        innerPoints[left - 1][m_splines.size() - 1] });
                    m_generatedFaceUvs.push_back(std::vector<Vector2> {
                        Vector2(innerUvs[left - 1][m_splines.size() - 1][0], 1.0),
                        innerUvs[left][m_splines.size() - 1],
                        innerUvs[left - 1][m_splines.size() - 1] });
                    m_generatedFaces.push_back(std::vector<size_t> {
                        lastMiddleVertexIndex,
                        innerPoints[right + 1][m_splines.size() - 1],
                        innerPoints[right][m_splines.size() - 1] });
                    m_generatedFaceUvs.push_back(std::vector<Vector2> {
                        Vector2(innerUvs[right + 1][m_splines.size() - 1][0], 1.0),
                        innerUvs[right + 1][m_splines.size() - 1],
                        innerUvs[right][m_splines.size() - 1] });
                }
            }
            // Close jaw
            m_generatedFaces.push_back(std::vector<size_t> {
                innerPoints[0][0],
                stitchingPoints[0][0],
                stitchingPoints[1][0],
                innerPoints[1][0] });
            m_generatedFaceUvs.push_back(std::vector<Vector2> {
                innerUvs[0][0],
                uvs[0][0],
                uvs[1][0],
                innerUvs[1][0] });
            m_generatedFaces.push_back(std::vector<size_t> {
                innerPoints[0][0],
                innerPoints[innerPoints.size() - 1][0],
                stitchingPoints[stitchingPoints.size() - 1][0],
                stitchingPoints[0][0] });
            m_generatedFaceUvs.push_back(std::vector<Vector2> {
                innerUvs[innerPoints.size()][0],
                innerUvs[innerPoints.size() - 1][0],
                uvs[stitchingPoints.size() - 1][0],
                uvs[stitchingPoints.size()][0] });
            m_generatedFaces.push_back(std::vector<size_t> {
                innerPoints[0][m_splines.size() - 1],
                innerPoints[1][m_splines.size() - 1],
                stitchingPoints[1][m_splines.size() - 1],
                stitchingPoints[0][m_splines.size() - 1] });
            m_generatedFaceUvs.push_back(std::vector<Vector2> {
                innerUvs[0][m_splines.size() - 1],
                innerUvs[1][m_splines.size() - 1],
                uvs[1][m_splines.size() - 1],
                uvs[0][m_splines.size() - 1] });
            m_generatedFaces.push_back(std::vector<size_t> {
                innerPoints[0][m_splines.size() - 1],
                stitchingPoints[0][m_splines.size() - 1],
                stitchingPoints[stitchingPoints.size() - 1][m_splines.size() - 1],
                innerPoints[innerPoints.size() - 1][m_splines.size() - 1] });
            m_generatedFaceUvs.push_back(std::vector<Vector2> {
                innerUvs[innerPoints.size()][m_splines.size() - 1],
                uvs[stitchingPoints.size()][m_splines.size() - 1],
                uvs[stitchingPoints.size() - 1][m_splines.size() - 1],
                innerUvs[innerPoints.size() - 1][m_splines.size() - 1] });
        } else if (m_frontClosed) {
            // Back open
            for (size_t i = 1; i < stitchingPoints.size(); ++i) {
                innerPoints[i].resize(m_splines.size());
                for (size_t k = 0; k < m_splines.size(); ++k) {
                    Vector3 offset;
                    if (0 == k) {
                        offset = (m_generatedVertices[stitchingPoints[i][k + 1]] - m_generatedVertices[stitchingPoints[i][k]]).normalized() * stitchingRadiuses[i][k];
                    }
                    innerPoints[i][k] = m_generatedVertices.size();
                    m_generatedVertexSources.push_back(m_generatedVertexSources[stitchingPoints[i][k]]);
                    m_generatedVertices.push_back(m_generatedVertices[stitchingPoints[i][k]] - normals[stitchingPoints[i][k]] * stitchingRadiuses[i][k] + offset);
                }
            }
            innerPoints[0].resize(m_splines.size());
            innerPoints[0][0] = m_generatedVertices.size();
            m_generatedVertexSources.push_back(m_generatedVertexSources[stitchingPoints[0][0]]);
            m_generatedVertices.push_back((m_generatedVertices[innerPoints[1][0]] + m_generatedVertices[innerPoints[innerPoints.size() - 1][0]]) * 0.5);
            for (size_t j = 2; j < innerPoints.size(); ++j) {
                size_t i = j - 1;
                for (size_t n = 1; n < m_splines.size(); ++n) {
                    size_t m = n - 1;
                    m_generatedFaces.push_back(std::vector<size_t> {
                        innerPoints[i][n],
                        innerPoints[j][n],
                        innerPoints[j][m],
                        innerPoints[i][m] });
                    m_generatedFaceUvs.push_back(std::vector<Vector2> {
                        innerUvs[i][n],
                        innerUvs[i + 1][n],
                        innerUvs[i + 1][m],
                        innerUvs[i][m] });
                }
            }
            for (size_t j = 1; j < m_splines.size(); ++j) {
                size_t i = j - 1;
                m_generatedFaces.push_back(std::vector<size_t> {
                    innerPoints[1][i],
                    stitchingPoints[1][i],
                    stitchingPoints[1][j],
                    innerPoints[1][j] });
                m_generatedFaceUvs.push_back(std::vector<Vector2> {
                    innerUvs[1][i],
                    uvs[1][i],
                    uvs[1][i + 1],
                    innerUvs[1][i + 1] });
                m_generatedFaces.push_back(std::vector<size_t> {
                    innerPoints[innerPoints.size() - 1][i],
                    innerPoints[innerPoints.size() - 1][j],
                    stitchingPoints[innerPoints.size() - 1][j],
                    stitchingPoints[innerPoints.size() - 1][i] });
                m_generatedFaceUvs.push_back(std::vector<Vector2> {
                    innerUvs[innerPoints.size() - 1][i],
                    innerUvs[innerPoints.size() - 1][i + 1],
                    uvs[innerPoints.size() - 1][i + 1],
                    uvs[innerPoints.size() - 1][i] });
            }
            // Close lid
            {
                size_t lastMiddleVertexIndex = innerPoints[0][0];
                int left = 2, right = (int)innerPoints.size() - 2;
                for (;
                     left + 1 < right;
                     ++left, --right) {
                    size_t middleVertexIndex = m_generatedVertices.size();
                    m_generatedVertexSources.push_back(m_splines.front().sourceId);
                    m_generatedVertices.push_back((m_generatedVertices[innerPoints[left][0]] + m_generatedVertices[innerPoints[right][0]]) * 0.5);
                    m_generatedFaces.push_back(std::vector<size_t> {
                        middleVertexIndex,
                        lastMiddleVertexIndex,
                        innerPoints[left - 1][0],
                        innerPoints[left][0] });
                    m_generatedFaceUvs.push_back(std::vector<Vector2> {
                        Vector2(innerUvs[left][0][0], 0.0),
                        Vector2(innerUvs[left - 1][0][0], 0.0),
                        innerUvs[left - 1][0],
                        innerUvs[left][0] });
                    m_generatedFaces.push_back(std::vector<size_t> {
                        middleVertexIndex,
                        innerPoints[right][0],
                        innerPoints[right + 1][0],
                        lastMiddleVertexIndex });
                    m_generatedFaceUvs.push_back(std::vector<Vector2> {
                        Vector2(innerUvs[right][0][0], 0.0),
                        innerUvs[right][0],
                        innerUvs[right + 1][0],
                        Vector2(innerUvs[right + 1][0][0], 0.0) });
                    lastMiddleVertexIndex = middleVertexIndex;
                }
                if (left == right) {
                    m_generatedFaces.push_back(std::vector<size_t> {
                        lastMiddleVertexIndex,
                        innerPoints[left - 1][0],
                        innerPoints[left][0] });
                    m_generatedFaceUvs.push_back(std::vector<Vector2> {
                        Vector2(innerUvs[left - 1][0][0], 0.0),
                        innerUvs[left - 1][0],
                        innerUvs[left][0] });
                    m_generatedFaces.push_back(std::vector<size_t> {
                        lastMiddleVertexIndex,
                        innerPoints[right][0],
                        innerPoints[right + 1][0] });
                    m_generatedFaceUvs.push_back(std::vector<Vector2> {
                        Vector2(innerUvs[right + 1][0][0], 0.0),
                        innerUvs[right][0],
                        innerUvs[right + 1][0] });
                }
            }
            for (size_t j = 2; j < innerPoints.size(); ++j) {
                size_t i = j - 1;
                m_generatedFaces.push_back(std::vector<size_t> {
                    innerPoints[i][m_splines.size() - 1],
                    stitchingPoints[i][m_splines.size() - 1],
                    stitchingPoints[j][m_splines.size() - 1],
                    innerPoints[j][m_splines.size() - 1] });
                m_generatedFaceUvs.push_back(std::vector<Vector2> {
                    innerUvs[i][m_splines.size() - 1],
                    uvs[i][m_splines.size() - 1],
                    uvs[j][m_splines.size() - 1],
                    innerUvs[j][m_splines.size() - 1] });
            }
            // Close jaw
            m_generatedFaces.push_back(std::vector<size_t> {
                innerPoints[0][0],
                stitchingPoints[0][0],
                stitchingPoints[1][0],
                innerPoints[1][0] });
            m_generatedFaceUvs.push_back(std::vector<Vector2> {
                innerUvs[0][0],
                uvs[0][0],
                uvs[1][0],
                innerUvs[1][0] });
            m_generatedFaces.push_back(std::vector<size_t> {
                innerPoints[0][0],
                innerPoints[innerPoints.size() - 1][0],
                stitchingPoints[stitchingPoints.size() - 1][0],
                stitchingPoints[0][0] });
            m_generatedFaceUvs.push_back(std::vector<Vector2> {
                innerUvs[innerPoints.size()][0],
                innerUvs[innerPoints.size() - 1][0],
                uvs[stitchingPoints.size() - 1][0],
                uvs[stitchingPoints.size()][0] });
        } else if (m_backClosed) {
            // Front open
            for (size_t i = 1; i < stitchingPoints.size(); ++i) {
                innerPoints[i].resize(m_splines.size());
                for (size_t k = 0; k < m_splines.size(); ++k) {
                    Vector3 offset;
                    if (k == m_splines.size() - 1) {
                        offset = (m_generatedVertices[stitchingPoints[i][k - 1]] - m_generatedVertices[stitchingPoints[i][k]]).normalized() * stitchingRadiuses[i][k];
                    }
                    innerPoints[i][k] = m_generatedVertices.size();
                    m_generatedVertexSources.push_back(m_generatedVertexSources[stitchingPoints[i][k]]);
                    m_generatedVertices.push_back(m_generatedVertices[stitchingPoints[i][k]] - normals[stitchingPoints[i][k]] * stitchingRadiuses[i][k] + offset);
                }
            }
            innerPoints[0].resize(m_splines.size());
            innerPoints[0][m_splines.size() - 1] = m_generatedVertices.size();
            m_generatedVertexSources.push_back(m_generatedVertexSources[stitchingPoints[0][m_splines.size() - 1]]);
            m_generatedVertices.push_back((m_generatedVertices[innerPoints[1][m_splines.size() - 1]] + m_generatedVertices[innerPoints[innerPoints.size() - 1][m_splines.size() - 1]]) * 0.5);
            for (size_t j = 2; j < innerPoints.size(); ++j) {
                size_t i = j - 1;
                for (size_t n = 1; n < m_splines.size(); ++n) {
                    size_t m = n - 1;
                    m_generatedFaces.push_back(std::vector<size_t> {
                        innerPoints[i][n],
                        innerPoints[j][n],
                        innerPoints[j][m],
                        innerPoints[i][m] });
                    m_generatedFaceUvs.push_back(std::vector<Vector2> {
                        innerUvs[i][n],
                        innerUvs[i + 1][n],
                        innerUvs[i + 1][m],
                        innerUvs[i][m] });
                }
            }
            // Close lid
            {
                size_t lastMiddleVertexIndex = innerPoints[0][m_splines.size() - 1];
                int left = 2, right = (int)innerPoints.size() - 2;
                for (;
                     left + 1 < right;
                     ++left, --right) {
                    size_t middleVertexIndex = m_generatedVertices.size();
                    m_generatedVertexSources.push_back(m_splines.back().sourceId);
                    m_generatedVertices.push_back((m_generatedVertices[innerPoints[left][m_splines.size() - 1]] + m_generatedVertices[innerPoints[right][m_splines.size() - 1]]) * 0.5);
                    m_generatedFaces.push_back(std::vector<size_t> {
                        middleVertexIndex,
                        innerPoints[left][m_splines.size() - 1],
                        innerPoints[left - 1][m_splines.size() - 1],
                        lastMiddleVertexIndex });
                    m_generatedFaceUvs.push_back(std::vector<Vector2> {
                        Vector2(innerUvs[left][m_splines.size() - 1][0], 1.0),
                        innerUvs[left][m_splines.size() - 1],
                        innerUvs[left - 1][m_splines.size() - 1],
                        Vector2(innerUvs[left - 1][m_splines.size() - 1][0], 1.0) });
                    m_generatedFaces.push_back(std::vector<size_t> {
                        middleVertexIndex,
                        lastMiddleVertexIndex,
                        innerPoints[right + 1][m_splines.size() - 1],
                        innerPoints[right][m_splines.size() - 1] });
                    m_generatedFaceUvs.push_back(std::vector<Vector2> {
                        Vector2(innerUvs[right][m_splines.size() - 1][0], 1.0),
                        Vector2(innerUvs[right + 1][m_splines.size() - 1][0], 1.0),
                        innerUvs[right + 1][m_splines.size() - 1],
                        innerUvs[right][m_splines.size() - 1] });
                    lastMiddleVertexIndex = middleVertexIndex;
                }
                if (left == right) {
                    m_generatedFaces.push_back(std::vector<size_t> {
                        lastMiddleVertexIndex,
                        innerPoints[left][m_splines.size() - 1],
                        innerPoints[left - 1][m_splines.size() - 1] });
                    m_generatedFaceUvs.push_back(std::vector<Vector2> {
                        Vector2(innerUvs[left - 1][m_splines.size() - 1][0], 1.0),
                        innerUvs[left][m_splines.size() - 1],
                        innerUvs[left - 1][m_splines.size() - 1] });
                    m_generatedFaces.push_back(std::vector<size_t> {
                        lastMiddleVertexIndex,
                        innerPoints[right + 1][m_splines.size() - 1],
                        innerPoints[right][m_splines.size() - 1] });
                    m_generatedFaceUvs.push_back(std::vector<Vector2> {
                        Vector2(innerUvs[right + 1][m_splines.size() - 1][0], 1.0),
                        innerUvs[right + 1][m_splines.size() - 1],
                        innerUvs[right][m_splines.size() - 1] });
                }
            }
            for (size_t j = 1; j < m_splines.size(); ++j) {
                size_t i = j - 1;
                m_generatedFaces.push_back(std::vector<size_t> {
                    innerPoints[1][i],
                    stitchingPoints[1][i],
                    stitchingPoints[1][j],
                    innerPoints[1][j] });
                m_generatedFaceUvs.push_back(std::vector<Vector2> {
                    innerUvs[1][i],
                    uvs[1][i],
                    uvs[1][i + 1],
                    innerUvs[1][i + 1] });
                m_generatedFaces.push_back(std::vector<size_t> {
                    innerPoints[innerPoints.size() - 1][i],
                    innerPoints[innerPoints.size() - 1][j],
                    stitchingPoints[innerPoints.size() - 1][j],
                    stitchingPoints[innerPoints.size() - 1][i] });
                m_generatedFaceUvs.push_back(std::vector<Vector2> {
                    innerUvs[innerPoints.size() - 1][i],
                    innerUvs[innerPoints.size() - 1][j],
                    uvs[innerPoints.size() - 1][j],
                    uvs[innerPoints.size() - 1][i] });
            }
            for (size_t j = 2; j < innerPoints.size(); ++j) {
                size_t i = j - 1;
                m_generatedFaces.push_back(std::vector<size_t> {
                    innerPoints[j][0],
                    stitchingPoints[j][0],
                    stitchingPoints[i][0],
                    innerPoints[i][0] });
                m_generatedFaceUvs.push_back(std::vector<Vector2> {
                    innerUvs[i + 1][0],
                    uvs[i + 1][0],
                    uvs[i][0],
                    innerUvs[i][0] });
            }
            // Close jaw
            m_generatedFaces.push_back(std::vector<size_t> {
                innerPoints[0][m_splines.size() - 1],
                innerPoints[1][m_splines.size() - 1],
                stitchingPoints[1][m_splines.size() - 1],
                stitchingPoints[0][m_splines.size() - 1] });
            m_generatedFaceUvs.push_back(std::vector<Vector2> {
                innerUvs[0][m_splines.size() - 1],
                innerUvs[1][m_splines.size() - 1],
                uvs[1][m_splines.size() - 1],
                uvs[0][m_splines.size() - 1] });
            m_generatedFaces.push_back(std::vector<size_t> {
                innerPoints[0][m_splines.size() - 1],
                stitchingPoints[0][m_splines.size() - 1],
                stitchingPoints[stitchingPoints.size() - 1][m_splines.size() - 1],
                innerPoints[innerPoints.size() - 1][m_splines.size() - 1] });
            m_generatedFaceUvs.push_back(std::vector<Vector2> {
                innerUvs[0][m_splines.size() - 1],
                uvs[0][m_splines.size() - 1],
                uvs[stitchingPoints.size() - 1][m_splines.size() - 1],
                innerUvs[innerPoints.size() - 1][m_splines.size() - 1] });
        } else {
            // Front open and back open
            for (size_t i = 1; i < stitchingPoints.size(); ++i) {
                innerPoints[i].resize(m_splines.size());
                for (size_t k = 0; k < m_splines.size(); ++k) {
                    innerPoints[i][k] = m_generatedVertices.size();
                    m_generatedVertexSources.push_back(m_generatedVertexSources[stitchingPoints[i][k]]);
                    m_generatedVertices.push_back(m_generatedVertices[stitchingPoints[i][k]] - normals[stitchingPoints[i][k]] * stitchingRadiuses[i][k]);
                }
            }
            for (size_t j = 2; j < innerPoints.size(); ++j) {
                size_t i = j - 1;
                for (size_t n = 1; n < m_splines.size(); ++n) {
                    size_t m = n - 1;
                    m_generatedFaces.push_back(std::vector<size_t> {
                        innerPoints[i][n],
                        innerPoints[j][n],
                        innerPoints[j][m],
                        innerPoints[i][m] });
                    m_generatedFaceUvs.push_back(std::vector<Vector2> {
                        innerUvs[i][n],
                        innerUvs[i + 1][n],
                        innerUvs[i + 1][m],
                        innerUvs[i][m] });
                }
            }
            for (size_t j = 2; j < innerPoints.size(); ++j) {
                size_t i = j - 1;
                m_generatedFaces.push_back(std::vector<size_t> {
                    innerPoints[j][0],
                    stitchingPoints[j][0],
                    stitchingPoints[i][0],
                    innerPoints[i][0] });
                m_generatedFaceUvs.push_back(std::vector<Vector2> {
                    innerUvs[i + 1][0],
                    uvs[i + 1][0],
                    uvs[i][0],
                    innerUvs[i][0] });
                m_generatedFaces.push_back(std::vector<size_t> {
                    innerPoints[i][m_splines.size() - 1],
                    stitchingPoints[i][m_splines.size() - 1],
                    stitchingPoints[j][m_splines.size() - 1],
                    innerPoints[j][m_splines.size() - 1] });
                m_generatedFaceUvs.push_back(std::vector<Vector2> {
                    innerUvs[i][m_splines.size() - 1],
                    uvs[i][m_splines.size() - 1],
                    uvs[i + 1][m_splines.size() - 1],
                    innerUvs[i + 1][m_splines.size() - 1] });
            }
            for (size_t j = 1; j < m_splines.size(); ++j) {
                size_t i = j - 1;
                m_generatedFaces.push_back(std::vector<size_t> {
                    innerPoints[1][i],
                    stitchingPoints[1][i],
                    stitchingPoints[1][j],
                    innerPoints[1][j] });
                m_generatedFaceUvs.push_back(std::vector<Vector2> {
                    innerUvs[1][i],
                    uvs[1][i],
                    uvs[1][i + 1],
                    innerUvs[1][i + 1] });
                m_generatedFaces.push_back(std::vector<size_t> {
                    innerPoints[innerPoints.size() - 1][i],
                    innerPoints[innerPoints.size() - 1][j],
                    stitchingPoints[innerPoints.size() - 1][j],
                    stitchingPoints[innerPoints.size() - 1][i] });
                m_generatedFaceUvs.push_back(std::vector<Vector2> {
                    innerUvs[innerPoints.size() - 1][i],
                    innerUvs[innerPoints.size() - 1][i + 1],
                    uvs[innerPoints.size() - 1][i + 1],
                    uvs[innerPoints.size() - 1][i] });
            }
        }
    }
}

}
