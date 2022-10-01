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

#ifndef DUST3D_MESH_HUD_MANAGER_H_
#define DUST3D_MESH_HUD_MANAGER_H_

#include <memory>
#include <vector>
#include <dust3d/base/vector3.h>
#include <dust3d/base/color.h>
#include <dust3d/base/object.h>

namespace dust3d
{

class HudManager
{
public:
    enum class OutputVertexAttribute
    {
        X = 0, Y, Z, R, G, B, A, Max
    };

    HudManager();
    void addFromObject(const Object &object);
    void addController(const Vector3 &origin, float radius, const Vector3 &direction, const Color &color);
    void addController(const Vector3 &origin, const std::vector<Vector3> &points, const Color &color);
    void generate();
    std::unique_ptr<std::vector<float>> takeVertices(int *lineVertexCount, int *triangleVertexCount=nullptr);
private:
    size_t addPosition(const Vector3 &position);
    void addLine(size_t from, size_t to, const Color &color, const Vector3 &baseNormal);

    std::vector<Vector3> m_positions;
    std::vector<std::tuple<size_t, size_t, Color, Vector3>> m_lines;
    std::unique_ptr<std::vector<float>> m_vertices;
    int m_lineVertexCount = 0;
    int m_triangleVertexCount = 0;
};

}

#endif
