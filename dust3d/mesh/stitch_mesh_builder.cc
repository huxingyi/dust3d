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

namespace dust3d
{

StitchMeshBuilder::StitchMeshBuilder(std::vector<Spline> &&splines)
{
    m_splines = std::move(splines);
}

const std::vector<Vector3> &StitchMeshBuilder::generatedVertices()
{
    return m_generatedVertices;
}

const std::vector<std::vector<size_t>> &StitchMeshBuilder::generatedFaces()
{
    return m_generatedFaces;
}

std::vector<std::vector<StitchMeshBuilder::StitchingPoint>> StitchMeshBuilder::convertSplinesToStitchingPoints(const std::vector<Spline> &splines)
{
    std::vector<std::vector<StitchingPoint>> stitchingPoints(splines.size());
    
    for (size_t i = 0; i < splines.size(); ++i) {
        const auto &spline = splines[i];
        std::vector<Vector3> polyline(spline.nodes.size());
        std::vector<double> radiuses(spline.nodes.size());
        for (size_t j = 0; j < spline.nodes.size(); ++j) {
            polyline[j] = spline.nodes[j].origin;
            radiuses[j] = spline.nodes[j].radius;
        }
        std::vector<Vector3> segmentPoints;
        std::vector<double> segmentRadiuses;
        splitPolylineToSegments(polyline, radiuses, m_targetSegments, &segmentPoints, &segmentRadiuses);
        stitchingPoints[i].resize(segmentPoints.size());
        for (size_t k = 0; k < segmentPoints.size(); ++k) {
            stitchingPoints[i][k].originVertex = m_generatedVertices.size();
            stitchingPoints[i][k].radius = segmentRadiuses[k];
            m_generatedVertices.push_back(segmentPoints[k]);
            m_generatedStitchingPoints.push_back(stitchingPoints[i][k]);
        }
    }

    return stitchingPoints;
}

double StitchMeshBuilder::segmentsLength(const std::vector<Vector3> &segmentPoints)
{
    double totalLength = 0.0;
    for (size_t j = 1; j < segmentPoints.size(); ++j) {
        size_t i = j - 1;
        totalLength += (segmentPoints[i] - segmentPoints[j]).length();
    }
    return totalLength;
}

void StitchMeshBuilder::splitPolylineToSegments(const std::vector<Vector3> &polyline,
    const std::vector<double> &radiuses,
    size_t targetSegments,
    std::vector<Vector3> *targetPoints,
    std::vector<double> *targetRadiuses)
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

void StitchMeshBuilder::generateMeshFromStitchingPoints(const std::vector<std::vector<StitchMeshBuilder::StitchingPoint>> &stitchingPoints)
{
    for (size_t j = 1; j < stitchingPoints.size(); ++j) {
        size_t i = j - 1;
        generateMeshFromStitchingPoints(stitchingPoints[i], stitchingPoints[j]);
    }
}

void StitchMeshBuilder::generateMeshFromStitchingPoints(const std::vector<StitchMeshBuilder::StitchingPoint> &a, 
    const std::vector<StitchMeshBuilder::StitchingPoint> &b)
{
    if (a.size() != b.size()) {
        dust3dDebug << "Unmatch sizes of stitching points, a:" << a.size() << "b:" << b.size();
        return;
    }
    if (a.size() < 2) {
        dust3dDebug << "Expected at least two stitching points, current:" << a.size();
        return;
    }
    for (size_t j = 1; j < a.size(); ++j) {
        size_t i = j - 1;
        m_generatedFaces.emplace_back(std::vector<size_t> {
            a[i].originVertex,
            b[i].originVertex,
            b[j].originVertex,
            a[j].originVertex
        });
    }
}

void StitchMeshBuilder::build()
{
    std::vector<std::vector<StitchingPoint>> stitchingPoints = convertSplinesToStitchingPoints(m_splines);
    generateMeshFromStitchingPoints(stitchingPoints);

    // Generate normals for all vertices
    m_generatedNormals.resize(m_generatedVertices.size());
    for (const auto &quad: m_generatedFaces) {
        for (size_t i = 0; i < 4; ++i) {
            size_t j = (i + 1) % 4;
            size_t k = (j + 1) % 4;
            auto faceNormal = Vector3::normal(m_generatedVertices[quad[i]], m_generatedVertices[quad[j]], m_generatedVertices[quad[k]]);
            m_generatedNormals[quad[i]] += faceNormal;
            m_generatedNormals[quad[j]] += faceNormal;
            m_generatedNormals[quad[k]] += faceNormal;
        }
    }
    for (auto &it: m_generatedNormals)
        it.normalize();

    // Move all vertices off distance at radius
    for (size_t i = 0; i < m_generatedVertices.size(); ++i) {
        m_generatedVertices[i] += m_generatedNormals[i] * m_generatedStitchingPoints[i].radius;
    }

    // Generate other side of faces
    auto oneSideVertexCount = m_generatedVertices.size();
    m_generatedVertices.reserve(oneSideVertexCount * 2);
    for (size_t i = 0; i < oneSideVertexCount; ++i) {
        m_generatedVertices.emplace_back(m_generatedVertices[i] - m_generatedNormals[i] * m_generatedStitchingPoints[i].radius * 2.0);
    }
    m_generatedFaces.reserve(m_generatedFaces.size() * 2);
    auto oneSideFaceCount = m_generatedFaces.size();
    for (size_t i = 0; i < oneSideFaceCount; ++i) {
        auto quad = m_generatedFaces[i];
        m_generatedFaces.emplace_back(std::vector<size_t> {
            oneSideVertexCount + quad[3], 
            oneSideVertexCount + quad[2], 
            oneSideVertexCount + quad[1], 
            oneSideVertexCount + quad[0]
        });
    }

    // Stitch layers for first stitching line
    for (size_t j = 1; j <= m_targetSegments; ++j) {
        size_t i = j - 1;
        m_generatedFaces.emplace_back(std::vector<size_t> {
            oneSideVertexCount + i, 
            i, 
            j, 
            oneSideVertexCount + j
        });
    }

    // Stitch layers for last stitching line
    size_t offsetBetweenFirstAndLastBorder = oneSideVertexCount - (m_targetSegments + 1);
    for (size_t j = 1; j <= m_targetSegments; ++j) {
        size_t i = j - 1;
        m_generatedFaces.emplace_back(std::vector<size_t> {
            oneSideVertexCount + j + offsetBetweenFirstAndLastBorder,
            j + offsetBetweenFirstAndLastBorder, 
            i + offsetBetweenFirstAndLastBorder, 
            oneSideVertexCount + i + offsetBetweenFirstAndLastBorder
        });
    }

    // Stitch layers for all head nodes of stitching lines
    for (size_t j = 1; j < m_splines.size(); ++j) {
        size_t i = j - 1;
        m_generatedFaces.emplace_back(std::vector<size_t> {
            oneSideVertexCount + j * (m_targetSegments + 1),
            j * (m_targetSegments + 1), 
            i * (m_targetSegments + 1), 
            oneSideVertexCount + i * (m_targetSegments + 1)
        });
    }

    // Stitch layers for all tail nodes of stitching lines
    size_t offsetBetweenSecondAndThirdBorder = m_targetSegments;
    for (size_t j = 1; j < m_splines.size(); ++j) {
        size_t i = j - 1;
        m_generatedFaces.emplace_back(std::vector<size_t> {
            oneSideVertexCount + i * (m_targetSegments + 1) + offsetBetweenSecondAndThirdBorder,
            i * (m_targetSegments + 1) + offsetBetweenSecondAndThirdBorder, 
            j * (m_targetSegments + 1) + offsetBetweenSecondAndThirdBorder, 
            oneSideVertexCount + j * (m_targetSegments + 1) + offsetBetweenSecondAndThirdBorder
        });
    }

    // TODO: Process circle

    // TODO: Process interpolate
}

}
