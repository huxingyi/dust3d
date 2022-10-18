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

#include <dust3d/mesh/section_preview_mesh_builder.h>
#include <dust3d/mesh/triangulate.h>

namespace dust3d {

SectionPreviewMeshBuilder::SectionPreviewMeshBuilder(const std::vector<Vector2>& cutFace)
    : m_cutFace(cutFace)
{
}

const std::vector<Vector3>& SectionPreviewMeshBuilder::resultVertices()
{
    return m_resultVertices;
}

const std::vector<std::vector<size_t>>& SectionPreviewMeshBuilder::resultTriangles()
{
    return m_resultTriangles;
}

void SectionPreviewMeshBuilder::build()
{
    m_resultVertices.resize(m_cutFace.size());
    for (size_t i = 0; i < m_cutFace.size(); ++i) {
        m_resultVertices[i] = Vector3(m_cutFace[i][0], m_cutFace[i][1], 0);
    }
    std::vector<size_t> cutFaceIndices(m_resultVertices.size());
    for (size_t i = 0; i < cutFaceIndices.size(); ++i)
        cutFaceIndices[i] = i;
    triangulate(m_resultVertices, cutFaceIndices, &m_resultTriangles);
}

}