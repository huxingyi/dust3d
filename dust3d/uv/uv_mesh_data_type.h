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

#ifndef DUST3D_UV_UV_MESH_DATA_TYPE_H_
#define DUST3D_UV_UV_MESH_DATA_TYPE_H_

#include <vector>
#include <cstdlib>

namespace dust3d 
{
namespace uv
{

struct Vector3
{
    float xyz[3];
};

typedef Vector3 Vertex;

struct Rect
{
    float left;
    float top;
    float width;
    float height;
};

struct Face
{
    size_t indices[3];
};

struct TextureCoord
{
    float uv[2];
};

struct FaceTextureCoords
{
    TextureCoord coords[3];
};

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<Face> faces;
    std::vector<Vector3> faceNormals;
    std::vector<int> facePartitions;
};

float dotProduct(const Vector3 &first, const Vector3 &second);
Vector3 crossProduct(const Vector3 &first, const Vector3 &second);

}
}

#endif
