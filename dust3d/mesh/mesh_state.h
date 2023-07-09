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

#ifndef DUST3D_MESH_MESH_STATE_H_
#define DUST3D_MESH_MESH_STATE_H_

#include <dust3d/base/position_key.h>
#include <dust3d/mesh/mesh_combiner.h>
#include <map>

namespace dust3d {

class MeshState {
public:
    std::unique_ptr<MeshCombiner::Mesh> mesh;
    std::vector<std::map<std::array<PositionKey, 3>, std::array<Vector2, 3>>> seamTriangleUvs;
    std::vector<std::array<PositionKey, 3>> brokenTriangles;

    MeshState() = default;
    MeshState(const std::vector<Vector3>& vertices, const std::vector<std::vector<size_t>>& faces);
    MeshState(const MeshState& other);
    void fetch(std::vector<Vector3>& vertices, std::vector<std::vector<size_t>>& faces) const;
    bool isNull() const;
    static std::unique_ptr<MeshState> combine(const MeshState& first, const MeshState& second,
        MeshCombiner::Method method);
    static bool isWatertight(const std::vector<std::vector<size_t>>& faces);
};

}

#endif
