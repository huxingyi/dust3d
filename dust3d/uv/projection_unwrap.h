/*
 *  Copyright (c) 2016-2026 Jeremy HU <jeremy-at-dust3d dot org>. All rights reserved.
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

#ifndef DUST3D_UV_PROJECTION_UNWRAP_H_
#define DUST3D_UV_PROJECTION_UNWRAP_H_

#include <array>
#include <dust3d/base/vector2.h>
#include <dust3d/base/vector3.h>
#include <vector>

namespace dust3d {

// Lays out the UVs of a mesh that came without any (an imported model): the triangles
// facing the same way (+X, -X, +Y, -Y, +Z or -Z, judged by their normal smoothed over
// the neighbourhood, so a gently curved surface is not broken up) and touching each other
// are one island, seen flat from that side, at the scale of the model; the islands are
// packed into the unit square. Returns the UV of each corner of each triangle.
std::vector<std::array<Vector2, 3>> projectionUnwrap(const std::vector<Vector3>& vertices,
    const std::vector<std::vector<size_t>>& triangles);

}

#endif
