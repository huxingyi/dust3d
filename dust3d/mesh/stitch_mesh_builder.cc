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
    
    size_t targetSegments = 5; // FIXME: decide target segments by ...
    for (size_t i = 0; i < splines.size(); ++i) {
        const auto &spline = splines[i];
        std::vector<Vector3> polyline(spline.nodes.size());
        for (size_t j = 0; j < spline.nodes.size(); ++j)
            polyline[j] = spline.nodes[j].origin;
        auto segmentPoints = splitPolylineToSegments(polyline, targetSegments);
        stitchingPoints[i].resize(segmentPoints.size());
        for (size_t k = 0; k < segmentPoints.size(); ++k) {
            stitchingPoints[i][k].originVertex = m_generatedVertices.size();
            // TODO: stitchingPoints[i][k].radius = ...
            m_generatedVertices.emplace_back(segmentPoints[k]);
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

std::vector<Vector3> StitchMeshBuilder::splitPolylineToSegments(const std::vector<Vector3> &polyline,
    size_t targetSegments)
{
    std::vector<Vector3> segmentPoints;
    if (polyline.size() < 2)
        return segmentPoints;
    double totalLength = segmentsLength(polyline);
    if (Math::isZero(totalLength)) {
        Vector3 middle = (polyline.front() + polyline.back()) * 0.5;
        for (size_t i = 0; i < targetSegments + 1; ++i) {
            segmentPoints.push_back(middle);
        }
        return segmentPoints;
    }
    double segmentLength = totalLength / targetSegments;
    double wantLength = segmentLength;
    segmentPoints.push_back(polyline.front());
    for (size_t j = 1; j < polyline.size(); ++j) {
        size_t i = j - 1;
        double lineLength = (polyline[j] - polyline[i]).length();
        Vector3 polylineStart = polyline[i];
        while (true) {
            if (lineLength < wantLength) {
                wantLength -= lineLength;
                break;
            }
            Vector3 newPosition = (polylineStart * (lineLength - wantLength) + polyline[j] * wantLength) / lineLength;
            segmentPoints.push_back(newPosition);
            lineLength -= wantLength;
            polylineStart = newPosition;
            wantLength = segmentLength;
        }
    }
    if (segmentPoints.size() < targetSegments + 1) {
        segmentPoints.push_back(polyline.back());
    } else {
        segmentPoints.back() = polyline.back();
    }
    return segmentPoints;
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
}

}
