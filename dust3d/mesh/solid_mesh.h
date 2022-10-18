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

#ifndef DUST3D_MESH_SOLID_MESH_H_
#define DUST3D_MESH_SOLID_MESH_H_

#include <dust3d/base/axis_aligned_bounding_box.h>
#include <dust3d/base/axis_aligned_bounding_box_tree.h>
#include <dust3d/base/vector3.h>

namespace dust3d {

class SolidMesh {
public:
    ~SolidMesh();
    void setVertices(const std::vector<Vector3>* vertices);
    void setTriangles(const std::vector<std::vector<size_t>>* triangles);

    const std::vector<Vector3>* vertices() const
    {
        return m_vertices;
    }

    const std::vector<std::vector<size_t>>* triangles() const
    {
        return m_triangles;
    }

    const std::vector<Vector3>* triangleNormals() const
    {
        return m_triangleNormals;
    }

    const AxisAlignedBoudingBoxTree* axisAlignedBoundingBoxTree() const
    {
        return m_axisAlignedBoundingBoxTree;
    }

    const std::vector<AxisAlignedBoudingBox>* triangleAxisAlignedBoundingBoxes() const
    {
        return m_triangleAxisAlignedBoundingBoxes;
    }

    void prepare();

private:
    void addTriagleToAxisAlignedBoundingBox(const std::vector<size_t>& triangle, AxisAlignedBoudingBox* box)
    {
        for (size_t i = 0; i < 3; ++i)
            box->update((*m_vertices)[triangle[i]]);
    }

    const std::vector<Vector3>* m_vertices = nullptr;
    const std::vector<std::vector<size_t>>* m_triangles = nullptr;
    std::vector<Vector3>* m_triangleNormals = nullptr;
    AxisAlignedBoudingBoxTree* m_axisAlignedBoundingBoxTree = nullptr;
    std::vector<AxisAlignedBoudingBox>* m_triangleAxisAlignedBoundingBoxes = nullptr;
};

}

#endif
