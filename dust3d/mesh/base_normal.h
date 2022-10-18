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

#ifndef DUST3D_MESH_BASE_NORMAL_H_
#define DUST3D_MESH_BASE_NORMAL_H_

#include <dust3d/base/vector3.h>
#include <vector>

namespace dust3d {

class BaseNormal {
public:
    static std::pair<size_t, int> findNearestAxis(const Vector3& direction);
    static inline const Vector3& axisDirection(size_t index)
    {
        static const std::vector<Vector3> axisList = {
            Vector3 { 1, 0, 0 },
            Vector3 { 0, 1, 0 },
            Vector3 { 0, 0, 1 },
        };
        return axisList[index];
    }
    static inline const Vector3& nextAxisDirection(size_t index)
    {
        return axisDirection((index + 1) % 3);
    }
    static std::vector<Vector3> calculateCircleVertices(double radius,
        size_t points,
        const Vector3& aroundAxis = Vector3(0.0, 0.0, 1.0),
        const Vector3& startDirection = Vector3(0.0, 1.0, 0.0),
        const Vector3& origin = Vector3(0.0, 0.0, 0.0));
    static Vector3 calculateCircleBaseNormal(const std::vector<Vector3>& vertices);
    static Vector3 calculateTubeBaseNormal(const std::vector<Vector3>& vertices);
};

}

#endif
