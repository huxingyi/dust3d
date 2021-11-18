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
 
#ifndef DUST3D_MESH_CENTRIPETAL_CATMULL_ROM_SPLINE_H_
#define DUST3D_MESH_CENTRIPETAL_CATMULL_ROM_SPLINE_H_

#include <vector>
#include <dust3d/base/vector3.h>

namespace dust3d
{
    
class CentripetalCatmullRomSpline
{
public:
    struct SplineNode
    {
        int source = -1;
        Vector3 position;
    };
    
    CentripetalCatmullRomSpline(bool isClosed);
    void addPoint(int source, const Vector3 &position, bool isKnot);
    bool interpolate();
    const std::vector<SplineNode> &splineNodes();
private:
    std::vector<SplineNode> m_splineNodes;
    std::vector<size_t> m_splineKnots;
    
    bool m_isClosed = false;
    bool interpolateClosed();
    bool interpolateOpened();
    float atKnot(float t, const Vector3 &p0, const Vector3 &p1);
    void interpolateSegment(std::vector<Vector3> &knots,
        size_t from, size_t to);
};

}

#endif
