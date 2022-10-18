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

#include <dust3d/base/vector2.h>
#include <dust3d/mesh/resolve_triangle_tangent.h>

namespace dust3d {

void resolveTriangleTangent(const dust3d::Object& object, std::vector<dust3d::Vector3>& tangents)
{
    tangents.resize(object.triangles.size());

    if (nullptr == object.triangleVertexUvs())
        return;

    const std::vector<std::vector<Vector2>>& triangleVertexUvs = *object.triangleVertexUvs();

    for (decltype(object.triangles.size()) i = 0; i < object.triangles.size(); i++) {
        tangents[i] = { 0, 0, 0 };
        const auto& uv = triangleVertexUvs[i];
        Vector2 uv1 = { uv[0][0], uv[0][1] };
        Vector2 uv2 = { uv[1][0], uv[1][1] };
        Vector2 uv3 = { uv[2][0], uv[2][1] };
        const auto& triangle = object.triangles[i];
        const Vector3& pos1 = object.vertices[triangle[0]];
        const Vector3& pos2 = object.vertices[triangle[1]];
        const Vector3& pos3 = object.vertices[triangle[2]];
        Vector3 edge1 = pos2 - pos1;
        Vector3 edge2 = pos3 - pos1;
        Vector2 deltaUv1 = uv2 - uv1;
        Vector2 deltaUv2 = uv3 - uv1;
        auto bottom = deltaUv1.x() * deltaUv2.y() - deltaUv2.x() * deltaUv1.y();
        if (dust3d::Math::isZero(bottom)) {
            bottom = 0.000001;
        }
        float f = 1.0 / bottom;
        Vector3 tangent = {
            f * (deltaUv2.y() * edge1.x() - deltaUv1.y() * edge2.x()),
            f * (deltaUv2.y() * edge1.y() - deltaUv1.y() * edge2.y()),
            f * (deltaUv2.y() * edge1.z() - deltaUv1.y() * edge2.z())
        };
        tangents[i] = tangent.normalized();
    }
}

}
