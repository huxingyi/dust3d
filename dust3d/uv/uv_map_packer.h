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

#ifndef DUST3D_UV_MAP_PACKER_H_
#define DUST3D_UV_MAP_PACKER_H_

#include <array>
#include <dust3d/base/position_key.h>
#include <dust3d/base/uuid.h>
#include <dust3d/base/vector2.h>
#include <map>
#include <vector>

namespace dust3d {

class UvMapPacker {
public:
    struct Part {
        Uuid id;
        double width;
        double height;
        std::map<std::array<PositionKey, 3>, std::array<Vector2, 3>> localUv;
    };

    struct Layout {
        Uuid id;
        double left;
        double top;
        double width;
        double height;
        bool flipped;
        std::map<std::array<PositionKey, 3>, std::array<Vector2, 3>> globalUv;
    };

    UvMapPacker();
    void addPart(const Part& part);
    void pack();
    const std::vector<Layout>& packedLayouts();
    double packedTextureSize();

private:
    std::vector<Part> m_partTriangleUvs;
    std::vector<Layout> m_packedLayouts;
    double m_packedTextureSize = 0.0;
};

}

#endif
