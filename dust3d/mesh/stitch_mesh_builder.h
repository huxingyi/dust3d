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

#ifndef DUST3D_MESH_STITCH_MESH_BUILDER_H_
#define DUST3D_MESH_STITCH_MESH_BUILDER_H_

#include <dust3d/base/vector3.h>
#include <dust3d/base/uuid.h>

namespace dust3d
{

class StitchMeshBuilder
{
public:
    struct Node
    {
        Vector3 origin;
        double radius;
    };

    struct Spline
    {
        std::vector<Node> nodes;
        bool isCircle = false;
        bool isClosing = false;
        Uuid sourceId;
    };

    StitchMeshBuilder(std::vector<Spline> &&splines);
    void build();
    const std::vector<Vector3> &generatedVertices() const;
    const std::vector<std::vector<size_t>> &generatedFaces() const;
    const std::vector<Spline> &splines() const;

private:
    struct StitchingPoint
    {
        size_t originVertex;
        double radius;
    };

    std::vector<Spline> m_splines;
    std::vector<StitchingPoint> m_generatedStitchingPoints;
    std::vector<Vector3> m_generatedVertices;
    std::vector<std::vector<size_t>> m_generatedFaces;
    std::vector<Vector3> m_generatedNormals;
    size_t m_targetSegments = 15;

    void interpolateSplinesToHaveEqualSizeOfNodesExceptClosingSplines();
    std::vector<std::vector<StitchingPoint>> convertSplinesToStitchingPoints(const std::vector<Spline> &splines);
    void generateMeshFromStitchingPoints(const std::vector<std::vector<StitchingPoint>> &stitchingPoints);
    void generateMeshFromStitchingPoints(const std::vector<StitchingPoint> &a, 
        const std::vector<StitchingPoint> &b);
    void splitPolylineToSegments(const std::vector<Vector3> &polyline,
        const std::vector<double> &radiuses,
        size_t targetSegments,
        std::vector<Vector3> *targetPoints,
        std::vector<double> *targetRadiuses);
    double segmentsLength(const std::vector<Vector3> &segmentPoints);
    void interpolateSplinesForBackClosing();
    void addQuadButMaybeTriangle(const std::vector<size_t> &quadButMaybeTriangle);
};

}

#endif
