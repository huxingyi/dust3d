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
 *
 *  
 *  Based on the Public Domain MaxRectsBinPack.cpp source by Jukka Jylänki
 *  https://github.com/juj/RectangleBinPack/
 *
 *  Ported to C# by Sven Magnus
 *  This version is also public domain - do whatever you want with it.
 *
 */

#ifndef DUST3D_UV_MAX_RECTANGLES_H_
#define DUST3D_UV_MAX_RECTANGLES_H_

namespace dust3d {
namespace uv {

    typedef struct MaxRectanglesSize {
        int width;
        int height;
    } MaxRectanglesSize;

    typedef struct MaxRectanglesPosition {
        int left;
        int top;
        int rotated : 1;
    } MaxRectanglesPosition;

    enum MaxRectanglesFreeRectChoiceHeuristic {
        kMaxRectanglesBestShortSideFit, ///< -BSSF: Positions the rectangle against the short side of a free rectangle into which it fits the best.
        kMaxRectanglesBestLongSideFit, ///< -BLSF: Positions the rectangle against the long side of a free rectangle into which it fits the best.
        kMaxRectanglesBestAreaFit, ///< -BAF: Positions the rectangle into the smallest free rect into which it fits.
        kMaxRectanglesBottomLeftRule, ///< -BL: Does the Tetris placement.
        kMaxRectanglesContactPointRule ///< -CP: Choosest the placement where the rectangle touches other rects as much as possible.
    };

    int maxRectangles(int width, int height, int rectCount, MaxRectanglesSize* rects,
        enum MaxRectanglesFreeRectChoiceHeuristic method, int allowRotations,
        MaxRectanglesPosition* layoutResults, float* occupancy);

}
}

#endif
