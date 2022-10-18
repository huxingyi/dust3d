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

#ifndef DUST3D_MESH_SECTION_PREVIEW_MESH_BUILDER_H_
#define DUST3D_MESH_SECTION_PREVIEW_MESH_BUILDER_H_

#include <dust3d/base/color.h>
#include <dust3d/base/vector2.h>
#include <dust3d/base/vector3.h>
#include <tuple>
#include <vector>

namespace dust3d {

class SectionPreviewMeshBuilder {
public:
    SectionPreviewMeshBuilder(const std::vector<Vector2>& cutFace, const Color& mainColor);
    void build();
    const std::vector<Vector3>& resultVertices();
    const std::vector<std::vector<size_t>>& resultTriangles();
    const std::vector<std::tuple<Color, float /*metalness*/, float /*roughness*/>>& resultVertexProperties();

private:
    std::vector<Vector2> m_cutFace;
    std::vector<Vector3> m_resultVertices;
    std::vector<std::tuple<dust3d::Color, float /*metalness*/, float /*roughness*/>> m_resultVertexProperties;
    std::vector<std::vector<size_t>> m_resultTriangles;
    Color m_mainColor;
};

}

#endif
