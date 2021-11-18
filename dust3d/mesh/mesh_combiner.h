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

#ifndef DUST3D_MESH_MESH_COMBINER_H_
#define DUST3D_MESH_MESH_COMBINER_H_

#include <vector>
#include <dust3d/base/vector3.h>
#include <dust3d/mesh/solid_mesh.h>

namespace dust3d
{
  
class MeshCombiner
{
public:
    enum class Method
    {
        Union,
        Diff
    };
    
    enum class Source
    {
        None,
        First,
        Second
    };

    class Mesh
    {
    public:
        Mesh() = default;
        Mesh(const std::vector<Vector3> &vertices, const std::vector<std::vector<size_t>> &faces);
        Mesh(const Mesh &other);
        ~Mesh();
        void fetch(std::vector<Vector3> &vertices, std::vector<std::vector<size_t>> &faces) const;
        bool isNull() const;
        bool isCombinable() const;
        
        friend MeshCombiner;
        
    private:
        SolidMesh *m_solidMesh = nullptr;
        std::vector<Vector3> *m_vertices = nullptr;
        std::vector<std::vector<size_t>> *m_triangles = nullptr;
    };
    
    static Mesh *combine(const Mesh &firstMesh, const Mesh &secondMesh, Method method,
        std::vector<std::pair<Source, size_t>> *combinedVerticesComeFrom=nullptr);
};

}

#endif
