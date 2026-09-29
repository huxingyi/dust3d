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

    // Newell's method: the polygon's normal, robust for concave outlines (an I-beam or an L
    // section), where summing corner normals cancels out at the reflex corners
    Vector3 normal;
    for (size_t i = 0; i < faceIndices.size(); ++i) {
        auto j = (i + 1) % faceIndices.size();
        const auto& a = vertices[faceIndices[i]];
        const auto& b = vertices[faceIndices[j]];
        normal += Vector3((a.y() - b.y()) * (a.z() + b.z()),
            (a.z() - b.z()) * (a.x() + b.x()),
            (a.x() - b.x()) * (a.y() + b.y()));
    }
    normal.normalize();

    std::vector<Vector3> pointsIn3d(faceIndices.size());
    for (size_t i = 0; i < faceIndices.size(); ++i)
        pointsIn3d[i] = vertices[faceIndices[i]];

    // an in-plane axis from the face's own points (not the mesh's first two vertices, which
    // may have nothing to do with this face, or lie along its normal)
    Vector3 origin = pointsIn3d[0];
    Vector3 axis;
    for (size_t i = 1; i < pointsIn3d.size(); ++i) {
        Vector3 d = pointsIn3d[i] - origin;
        d -= normal * Vector3::dotProduct(d, normal);
        if (d.lengthSquared() > 1e-18) {
            axis = d.normalized();
            break;
        }
    }

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

}