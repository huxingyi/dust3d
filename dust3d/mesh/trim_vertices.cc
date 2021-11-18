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

#include <dust3d/base/math.h>
#include <dust3d/mesh/trim_vertices.h>

namespace dust3d
{

void trimVertices(std::vector<Vector3> *vertices, bool normalize)
{
    float xLow = std::numeric_limits<float>::max();
    float xHigh = std::numeric_limits<float>::lowest();
    float yLow = std::numeric_limits<float>::max();
    float yHigh = std::numeric_limits<float>::lowest();
    float zLow = std::numeric_limits<float>::max();
    float zHigh = std::numeric_limits<float>::lowest();
    for (const auto &position: *vertices) {
        if (position.x() < xLow)
            xLow = position.x();
        else if (position.x() > xHigh)
            xHigh = position.x();
        if (position.y() < yLow)
            yLow = position.y();
        else if (position.y() > yHigh)
            yHigh = position.y();
        if (position.z() < zLow)
            zLow = position.z();
        else if (position.z() > zHigh)
            zHigh = position.z();
    }
    float xMiddle = (xHigh + xLow) * 0.5;
    float yMiddle = (yHigh + yLow) * 0.5;
    float zMiddle = (zHigh + zLow) * 0.5;
    if (normalize) {
        float xSize = xHigh - xLow;
        float ySize = yHigh - yLow;
        float zSize = zHigh - zLow;
        float longSize = ySize;
        if (xSize > longSize)
            longSize = xSize;
        if (zSize > longSize)
            longSize = zSize;
        if (Math::isZero(longSize))
            longSize = 0.000001;
        for (auto &position: *vertices) {
            position.setX((position.x() - xMiddle) / longSize);
            position.setY((position.y() - yMiddle) / longSize);
            position.setZ((position.z() - zMiddle) / longSize);
        }
    } else {
        for (auto &position: *vertices) {
            position.setX((position.x() - xMiddle));
            position.setY((position.y() - yMiddle));
            position.setZ((position.z() - zMiddle));
        }
    }
}

}
