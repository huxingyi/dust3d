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

#include <dust3d/mesh/box_mesh.h>

namespace dust3d {

static const std::vector<Vector3> g_subdivedBoxObjVertices = {
    { -0.025357f, -0.025357f, 0.025357f },
    { -0.025357f, 0.025357f, 0.025357f },
    { -0.025357f, 0.025357f, -0.025357f },
    { -0.025357f, -0.025357f, -0.025357f },
    { 0.025357f, -0.025357f, 0.025357f },
    { 0.025357f, -0.025357f, -0.025357f },
    { 0.025357f, 0.025357f, 0.025357f },
    { 0.025357f, 0.025357f, -0.025357f },
    { -0.030913f, -0.030913f, -0.000000f },
    { -0.030913f, -0.000000f, 0.030913f },
    { -0.030913f, 0.030913f, 0.000000f },
    { -0.030913f, 0.000000f, -0.030913f },
    { 0.030913f, -0.030913f, -0.000000f },
    { 0.000000f, -0.030913f, 0.030913f },
    { -0.000000f, -0.030913f, -0.030913f },
    { 0.030913f, -0.000000f, 0.030913f },
    { -0.000000f, 0.030913f, 0.030913f },
    { 0.030913f, 0.030913f, 0.000000f },
    { 0.030913f, 0.000000f, -0.030913f },
    { -0.000000f, 0.030913f, -0.030913f },
    { -0.042574f, 0.000000f, -0.000000f },
    { -0.000000f, -0.042574f, -0.000000f },
    { 0.000000f, -0.000000f, 0.042574f },
    { 0.042574f, -0.000000f, -0.000000f },
    { -0.000000f, 0.000000f, -0.042574f },
    { 0.000000f, 0.042574f, 0.000000f },
};

static const std::vector<std::vector<size_t>> g_subdivedBoxObjFaces = {
    { 1, 10, 21, 9 },
    { 10, 2, 11, 21 },
    { 21, 11, 3, 12 },
    { 9, 21, 12, 4 },
    { 5, 14, 22, 13 },
    { 14, 1, 9, 22 },
    { 22, 9, 4, 15 },
    { 13, 22, 15, 6 },
    { 1, 14, 23, 10 },
    { 14, 5, 16, 23 },
    { 23, 16, 7, 17 },
    { 10, 23, 17, 2 },
    { 7, 16, 24, 18 },
    { 16, 5, 13, 24 },
    { 24, 13, 6, 19 },
    { 18, 24, 19, 8 },
    { 4, 12, 25, 15 },
    { 12, 3, 20, 25 },
    { 25, 20, 8, 19 },
    { 15, 25, 19, 6 },
    { 2, 17, 26, 11 },
    { 17, 7, 18, 26 },
    { 26, 18, 8, 20 },
    { 11, 26, 20, 3 },
};

void buildBoxMesh(const Vector3& position, float radius, size_t subdivideTimes, std::vector<Vector3>& vertices, std::vector<std::vector<size_t>>& faces)
{
    if (subdivideTimes > 0) {
        vertices.reserve(g_subdivedBoxObjVertices.size());
        auto ratio = 24 * radius;
        for (const auto& vertex : g_subdivedBoxObjVertices) {
            auto newVertex = vertex * ratio + position;
            vertices.push_back(newVertex);
        }
        faces.reserve(g_subdivedBoxObjFaces.size());
        for (const auto& face : g_subdivedBoxObjFaces) {
            auto newFace = {
                face[0] - 1,
                face[1] - 1,
                face[2] - 1,
                face[3] - 1,
            };
            faces.push_back(newFace);
        }
        return;
    }
    std::vector<Vector3> beginPlane = {
        { -radius, -radius, radius },
        { radius, -radius, radius },
        { radius, radius, radius },
        { -radius, radius, radius },
    };
    std::vector<Vector3> endPlane = {
        beginPlane[0],
        beginPlane[3],
        beginPlane[2],
        beginPlane[1]
    };
    for (auto& vertex : endPlane) {
        vertex.setZ(vertex.z() - radius - radius);
    }
    for (const auto& vertex : beginPlane) {
        vertices.push_back(vertex);
    }
    for (const auto& vertex : endPlane) {
        vertices.push_back(vertex);
    }
    std::vector<size_t> beginLoop = {
        0, 1, 2, 3
    };
    std::vector<size_t> endLoop = {
        4, 5, 6, 7
    };
    std::vector<size_t> alignedEndLoop = {
        4, 7, 6, 5
    };
    faces.push_back(beginLoop);
    faces.push_back(endLoop);
    for (size_t i = 0; i < beginLoop.size(); ++i) {
        size_t j = (i + 1) % beginLoop.size();
        faces.push_back({ beginLoop[j],
            beginLoop[i],
            alignedEndLoop[i],
            alignedEndLoop[j] });
    }
    for (auto& vertex : vertices) {
        vertex += position;
    }
}

}
