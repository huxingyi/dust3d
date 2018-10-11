// This code is in the public domain -- Ignacio Castaño <castano@gmail.com>

#pragma once
#ifndef NV_MATH_CONVEXHULL_H
#define NV_MATH_CONVEXHULL_H

#include "nvmath.h"
#include "nvcore/Array.h"

namespace nv {
    class Vector2;
 
    void convexHull(const Array<Vector2> & input, Array<Vector2> & output, float epsilon = 0);

    //ACS: moved these down from collision_volume.cpp
    bool isClockwise(const Array<Vector2> & input);
    void reduceConvexHullToNSides(Array<Vector2> *input, uint num_sides);
    void flipWinding(Array<Vector2> *input);

    bool bestFitPolygon(const Array<Vector2> & input, uint vertexCount, Array<Vector2> * output);

    // Basic ear-clipping algorithm.
    void triangulate(const Array<Vector2> & input, Array<uint> * output);

} // namespace nv

#endif // NV_MATH_CONVEXHULL_H
