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

#ifndef DUST3D_MESH_SECTION_REMESHER_H_
#define DUST3D_MESH_SECTION_REMESHER_H_

#include <dust3d/base/vector2.h>
#include <dust3d/base/vector3.h>
#include <vector>

namespace dust3d {

class SectionRemesher {
public:
    SectionRemesher(const std::vector<Vector3>& vertices, double ringV, double centerV);
    void remesh();
    const std::vector<Vector3>& generatedVertices();
    const std::vector<std::vector<size_t>>& generatedFaces();
    const std::vector<std::vector<Vector2>>& generatedFaceUvs();

private:
    std::vector<std::vector<size_t>> m_generatedFaces;
    std::vector<std::vector<Vector2>> m_generatedFaceUvs;

    std::vector<Vector3> m_vertices;
    double m_ringV = 0.0;
    double m_centerV = 0.0;
    std::vector<double> m_ringUs;

    void remeshConvex(const std::vector<size_t>& ringVertices, const std::vector<double>& ringUs);
    bool isConvex(const std::vector<size_t>& ringVertices);
};

}

#endif
