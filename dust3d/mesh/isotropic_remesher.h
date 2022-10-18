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

#ifndef DUST3D_MESH_ISOTROPIC_REMESHER_H_
#define DUST3D_MESH_ISOTROPIC_REMESHER_H_

#include <dust3d/base/axis_aligned_bounding_box.h>
#include <dust3d/base/axis_aligned_bounding_box_tree.h>
#include <dust3d/base/vector3.h>
#include <vector>

namespace dust3d {

class IsotropicHalfedgeMesh;

class IsotropicRemesher {
public:
    IsotropicRemesher(const std::vector<Vector3>* vertices,
        const std::vector<std::vector<size_t>>* triangles);
    ~IsotropicRemesher();
    double initialAverageEdgeLength();
    void setSharpEdgeIncludedAngle(double degrees);
    void setTargetEdgeLength(double edgeLength);
    void setTargetTriangleCount(size_t triangleCount);
    void remesh(size_t iteration);
    IsotropicHalfedgeMesh* remeshedHalfedgeMesh();

private:
    const std::vector<Vector3>* m_vertices = nullptr;
    const std::vector<std::vector<size_t>>* m_triangles = nullptr;
    std::vector<Vector3>* m_triangleNormals = nullptr;
    IsotropicHalfedgeMesh* m_halfedgeMesh = nullptr;
    std::vector<AxisAlignedBoudingBox>* m_triangleBoxes = nullptr;
    AxisAlignedBoudingBoxTree* m_axisAlignedBoundingBoxTree = nullptr;
    double m_sharpEdgeThresholdRadians = 0;
    double m_targetEdgeLength = 0;
    double m_initialAverageEdgeLength = 0;
    size_t m_targetTriangleCount = 0;

    void addTriagleToAxisAlignedBoundingBox(const std::vector<size_t>& triangle, AxisAlignedBoudingBox* box)
    {
        for (size_t i = 0; i < 3; ++i)
            box->update((*m_vertices)[triangle[i]]);
    }

    void splitLongEdges(double maxEdgeLength);
    void collapseShortEdges(double minEdgeLengthSquared, double maxEdgeLengthSquared);
    void flipEdges();
    void shiftVertices();
    void projectVertices();
    void buildAxisAlignedBoundingBoxTree();
};

}

#endif
