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

#include <array>
#include <dust3d/base/vector2.h>
#include <dust3d/mesh/isotropic_halfedge_mesh.h>
#include <dust3d/mesh/isotropic_remesher.h>
#include <dust3d/mesh/triangulate.h>
#include <mapbox/earcut.hpp>

namespace dust3d {

void triangulate(const std::vector<Vector3>& vertices,
    const std::vector<size_t>& faceIndices,
    std::vector<std::vector<size_t>>* triangles)
{
    if (4 == faceIndices.size()) {
        triangles->push_back({ faceIndices[0],
            faceIndices[1],
            faceIndices[2] });
        triangles->push_back({ faceIndices[2],
            faceIndices[3],
            faceIndices[0] });
        return;
    }

    Vector3 normal;
    for (size_t i = 0; i < faceIndices.size(); ++i) {
        auto j = (i + 1) % faceIndices.size();
        auto k = (i + 2) % faceIndices.size();
        const auto& enter = vertices[faceIndices[i]];
        const auto& cone = vertices[faceIndices[j]];
        const auto& leave = vertices[faceIndices[k]];
        normal += Vector3::normal(enter, cone, leave);
    }
    normal.normalize();

    Vector3 axis = (vertices[1] - vertices[0]).normalized();
    Vector3 origin = vertices[0];

    std::vector<Vector3> pointsIn3d(faceIndices.size());
    for (size_t i = 0; i < faceIndices.size(); ++i)
        pointsIn3d[i] = vertices[faceIndices[i]];

    std::vector<Vector2> pointsIn2d;
    Vector3::project(pointsIn3d, &pointsIn2d,
        normal, axis, origin);

    std::vector<std::vector<std::array<double, 2>>> polygons;
    std::vector<size_t> pointIndices;

    std::vector<std::array<double, 2>> border;
    border.reserve(pointsIn2d.size());
    for (const auto& v : pointsIn2d) {
        border.push_back(std::array<double, 2> { v.x(), v.y() });
    }
    polygons.push_back(border);

    std::vector<size_t> indices = mapbox::earcut<size_t>(polygons);
    for (size_t i = 0; i < indices.size(); i += 3) {
        triangles->push_back({ faceIndices[indices[i]],
            faceIndices[indices[i + 1]],
            faceIndices[indices[i + 2]] });
    }
}

void triangulate(const std::vector<Vector3>& vertices,
    const std::vector<std::vector<size_t>>& faces,
    std::vector<std::vector<size_t>>* triangles)
{
    for (const auto& faceIndices : faces) {
        if (faceIndices.size() <= 3) {
            triangles->push_back(faceIndices);
            continue;
        }
        triangulate(vertices, faceIndices, triangles);
    }
}

void isotropicTriangulate(std::vector<Vector3>& vertices,
    const std::vector<size_t>& faceIndices,
    std::vector<std::vector<size_t>>* triangles)
{
    if (faceIndices.empty())
        return;

    double edgeLength = 0.0;
    for (size_t i = 0; i < faceIndices.size(); ++i) {
        size_t j = (i + 1) % faceIndices.size();
        edgeLength += (vertices[faceIndices[i]] - vertices[faceIndices[j]]).length();
    }
    double targetEdgeLength = 1.2 * edgeLength / faceIndices.size();

    std::vector<std::vector<size_t>> firstStepTriangles;
    triangulate(vertices, faceIndices, &firstStepTriangles);

    IsotropicRemesher isotropicRemesher(&vertices, &firstStepTriangles);
    isotropicRemesher.setTargetEdgeLength(targetEdgeLength);
    isotropicRemesher.remesh(3);

    size_t outputIndex = vertices.size();
    IsotropicHalfedgeMesh* halfedgeMesh = isotropicRemesher.remeshedHalfedgeMesh();
    for (IsotropicHalfedgeMesh::Vertex* vertex = halfedgeMesh->moveToNextVertex(nullptr);
         nullptr != vertex;
         vertex = halfedgeMesh->moveToNextVertex(vertex)) {
        if (-1 != vertex->sourceIndex)
            continue;
        vertex->sourceIndex = outputIndex++;
        vertices.push_back({ vertex->position[0],
            vertex->position[1],
            vertex->position[2] });
    }
    for (IsotropicHalfedgeMesh::Face* face = halfedgeMesh->moveToNextFace(nullptr);
         nullptr != face;
         face = halfedgeMesh->moveToNextFace(face)) {
        triangles->push_back({ (size_t)face->halfedge->previousHalfedge->startVertex->sourceIndex,
            (size_t)face->halfedge->startVertex->sourceIndex,
            (size_t)face->halfedge->nextHalfedge->startVertex->sourceIndex });
    }
}

}