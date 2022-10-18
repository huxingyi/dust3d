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

#include <Eigen/Dense>
#include <dust3d/uv/uv_mesh_data_type.h>

namespace dust3d {
namespace uv {

    float dotProduct(const Vector3& first, const Vector3& second)
    {
        Eigen::Vector3d v(first.xyz[0], first.xyz[1], first.xyz[2]);
        Eigen::Vector3d w(second.xyz[0], second.xyz[1], second.xyz[2]);
        return v.dot(w);
    }

    Vector3 crossProduct(const Vector3& first, const Vector3& second)
    {
        Eigen::Vector3d v(first.xyz[0], first.xyz[1], first.xyz[2]);
        Eigen::Vector3d w(second.xyz[0], second.xyz[1], second.xyz[2]);
        auto u = v.cross(w);
        Vector3 result;
        result.xyz[0] = u.x();
        result.xyz[1] = u.y();
        result.xyz[2] = u.z();
        return result;
    }

}
}
