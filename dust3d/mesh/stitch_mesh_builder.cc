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

StitchMeshBuilder::StitchMeshBuilder(std::vector<Spline>&& splines)
{
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
        if (spline.isClosing) {
            auto& interpolatedSpline = interpolatedSplines[i];
            interpolatedSpline.nodes.resize(m_targetSegments + 1, spline.nodes.front());
            continue;
        }
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

std::vector<std::vector<StitchMeshBuilder::StitchingPoint>> StitchMeshBuilder::convertSplinesToStitchingPoints(const std::vector<Spline>& splines)
{
    std::vector<std::vector<StitchingPoint>> stitchingPoints(splines.size());

    std::vector<std::vector<double>> offsetVs(splines.front().nodes.size());
    for (size_t k = 0; k < splines.front().nodes.size(); ++k) {
        offsetVs[k].resize(splines.size());
        for (size_t i = 1; i < splines.size(); ++i) {
            offsetVs[k][i] = offsetVs[k][i - 1] + (splines[i - 1].nodes[k].origin - splines[i].nodes[k].origin).length();
        }
        double totalLength = std::max(offsetVs[k].back(), std::numeric_limits<double>::epsilon());
        for (size_t i = 1; i < splines.size(); ++i) {
            offsetVs[k][i] /= totalLength;
        }
    }

    for (size_t i = 0; i < splines.size(); ++i) {
        const auto& spline = splines[i];
        stitchingPoints[i].resize(spline.nodes.size());
        for (size_t k = 0; k < spline.nodes.size(); ++k) {
            if (spline.isClosing && k > 0) {
                stitchingPoints[i][k].originVertex = stitchingPoints[i][0].originVertex;
            } else {
                stitchingPoints[i][k].originVertex = m_generatedVertices.size();
                m_generatedVertices.push_back(spline.nodes[k].origin);
            }
            stitchingPoints[i][k].radius = spline.nodes[k].radius;
            stitchingPoints[i][k].v = offsetVs[k][i];
            m_generatedStitchingPoints.push_back(stitchingPoints[i][k]);
        }
    }

    return stitchingPoints;
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

    // TODO: Make sure there is no closing nodes in the middle

    // Interpolate splines to make sure every spline have equal size of nodes
    if (!interpolateSplinesToHaveEqualSizeOfNodes())
        return;

    // Generate one side faces
    std::vector<std::vector<StitchingPoint>> stitchingPoints = convertSplinesToStitchingPoints(m_splines);
    generateMeshFromStitchingPoints(stitchingPoints);

    if (m_splines.back().isClosing && stitchingPoints.size() >= 2) {
        addQuadButMaybeTriangle(std::vector<size_t> {
                                    stitchingPoints[stitchingPoints.size() - 2].back().originVertex,
                                    stitchingPoints[stitchingPoints.size() - 2].front().originVertex,
                                    stitchingPoints.back().front().originVertex },
            std::vector<Vector2> { Vector2(1.0, stitchingPoints[stitchingPoints.size() - 2].back().v), Vector2(0.0, stitchingPoints[stitchingPoints.size() - 2].back().v), Vector2(0.5, stitchingPoints.back().front().v) });
    }

    if (m_splines.front().isClosing && stitchingPoints.size() >= 2) {
        addQuadButMaybeTriangle(std::vector<size_t> {
                                    stitchingPoints[1].back().originVertex,
                                    stitchingPoints[1].front().originVertex,
                                    stitchingPoints.front().front().originVertex },
            std::vector<Vector2> { Vector2(1.0, stitchingPoints[1].back().v), Vector2(0.0, stitchingPoints[1].back().v), Vector2(0.5, stitchingPoints.front().front().v) });
    }

    /*

    // Generate normals for all vertices
    m_generatedNormals.resize(m_generatedVertices.size());
    for (const auto& quad : m_generatedFaces) {
        for (size_t i = 0; i < 4; ++i) {
            size_t j = (i + 1) % 4;
            size_t k = (j + 1) % 4;
            auto faceNormal = Vector3::normal(m_generatedVertices[quad[i]], m_generatedVertices[quad[j]], m_generatedVertices[quad[k]]);
            m_generatedNormals[quad[i]] += faceNormal;
            m_generatedNormals[quad[j]] += faceNormal;
            m_generatedNormals[quad[k]] += faceNormal;
        }
    }
    for (auto& it : m_generatedNormals)
        it.normalize();

    // Generate other side of faces
    auto oneSideVertexCount = m_generatedVertices.size();
    m_generatedVertices.reserve(oneSideVertexCount * 2);
    for (size_t i = 0; i < oneSideVertexCount; ++i) {
        m_generatedVertices.emplace_back(m_generatedVertices[i]);
    }
    m_generatedFaces.reserve(m_generatedFaces.size() * 2);
    auto oneSideFaceCount = m_generatedFaces.size();
    for (size_t i = 0; i < oneSideFaceCount; ++i) {
        auto quad = m_generatedFaces[i];
        // The following quad vertices should follow the order of 2,1,0,3 strictly,
        // This is to allow 2,1,0 to be grouped as triangle in the later quad to triangles processing.
        // If not follow this order, the front triangle and back triangle maybe cross over because of not be parallel.
        m_generatedFaces.emplace_back(std::vector<size_t> {
            oneSideVertexCount + quad[2],
            oneSideVertexCount + quad[1],
            oneSideVertexCount + quad[0],
            oneSideVertexCount + quad[3] });
        m_generatedFaceUvs.emplace_back(std::vector<Vector2> {
            m_generatedFaceUvs[i][2],
            m_generatedFaceUvs[i][1],
            m_generatedFaceUvs[i][0],
            m_generatedFaceUvs[i][3] });
    }

    // Move all vertices off distance at radius
    for (size_t i = 0; i < oneSideVertexCount; ++i) {
        auto moveDistance = m_generatedNormals[i] * m_generatedStitchingPoints[i].radius;
        m_generatedVertices[i] += moveDistance;
        m_generatedVertices[i + oneSideVertexCount] -= moveDistance;
    }

    // Stitch layers for first stitching line
    for (size_t j = 1; j <= m_targetSegments; ++j) {
        size_t i = j - 1;
        m_generatedFaces.emplace_back(std::vector<size_t> {
            oneSideVertexCount + i,
            i,
            j,
            oneSideVertexCount + j });
    }

    // Stitch layers for last stitching line
    size_t offsetBetweenFirstAndLastBorder = oneSideVertexCount - (m_targetSegments + 1);
    if (m_splines.back().isClosing) {
        size_t half = m_targetSegments / 2;
        for (size_t j = 1; j <= half; ++j) {
            size_t i = j - 1;
            addQuadButMaybeTriangle(std::vector<size_t> {
                oneSideVertexCount + j + offsetBetweenFirstAndLastBorder,
                oneSideVertexCount + i + offsetBetweenFirstAndLastBorder,
                oneSideVertexCount + (m_targetSegments - i) + offsetBetweenFirstAndLastBorder,
                oneSideVertexCount + (m_targetSegments - j) + offsetBetweenFirstAndLastBorder });
            addQuadButMaybeTriangle(std::vector<size_t> {
                (m_targetSegments - j) + offsetBetweenFirstAndLastBorder,
                (m_targetSegments - i) + offsetBetweenFirstAndLastBorder,
                i + offsetBetweenFirstAndLastBorder,
                j + offsetBetweenFirstAndLastBorder });
        }
        m_generatedFaces.emplace_back(std::vector<size_t> {
            (m_targetSegments - 0) + offsetBetweenFirstAndLastBorder,
            0 + offsetBetweenFirstAndLastBorder,
            oneSideVertexCount + 0 + offsetBetweenFirstAndLastBorder,
            oneSideVertexCount + (m_targetSegments - 0) + offsetBetweenFirstAndLastBorder });
    } else {
        for (size_t j = 1; j <= m_targetSegments; ++j) {
            size_t i = j - 1;
            m_generatedFaces.emplace_back(std::vector<size_t> {
                oneSideVertexCount + j + offsetBetweenFirstAndLastBorder,
                j + offsetBetweenFirstAndLastBorder,
                i + offsetBetweenFirstAndLastBorder,
                oneSideVertexCount + i + offsetBetweenFirstAndLastBorder });
        }
    }

    // Stitch layers for all head nodes of stitching lines
    for (size_t j = 1; j < m_splines.size(); ++j) {
        size_t i = j - 1;
        m_generatedFaces.emplace_back(std::vector<size_t> {
            oneSideVertexCount + j * (m_targetSegments + 1),
            j * (m_targetSegments + 1),
            i * (m_targetSegments + 1),
            oneSideVertexCount + i * (m_targetSegments + 1) });
    }

    // Stitch layers for all tail nodes of stitching lines
    size_t offsetBetweenSecondAndThirdBorder = m_targetSegments;
    for (size_t j = 1; j < m_splines.size(); ++j) {
        size_t i = j - 1;
        m_generatedFaces.emplace_back(std::vector<size_t> {
            oneSideVertexCount + i * (m_targetSegments + 1) + offsetBetweenSecondAndThirdBorder,
            i * (m_targetSegments + 1) + offsetBetweenSecondAndThirdBorder,
            j * (m_targetSegments + 1) + offsetBetweenSecondAndThirdBorder,
            oneSideVertexCount + j * (m_targetSegments + 1) + offsetBetweenSecondAndThirdBorder });
    }

    */

    // TODO: Process circle

    // TODO: Process interpolate
}

}
