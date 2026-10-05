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

#ifndef DUST3D_UV_PROCEDURAL_NOISE_H_
#define DUST3D_UV_PROCEDURAL_NOISE_H_

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <dust3d/base/vector3.h>

// Small, deterministic 3D noise for surface details baked into textures (SurfacePattern
// and others). Evaluated on the 3D surface, so the details run across UV seams.
namespace dust3d {
namespace noise {

    inline uint32_t hash3(int x, int y, int z, uint32_t seed)
    {
        uint32_t h = seed;
        h ^= (uint32_t)x * 0x8da6b343u;
        h ^= (uint32_t)y * 0xd8163841u;
        h ^= (uint32_t)z * 0xcb1ab31fu;
        h ^= h >> 16;
        h *= 0x7feb352du;
        h ^= h >> 15;
        h *= 0x846ca68bu;
        h ^= h >> 16;
        return h;
    }

    inline double unit(uint32_t h)
    {
        return (double)(h & 0xffffffu) / (double)0x1000000;
    }

    inline double smoothstep(double edge0, double edge1, double x)
    {
        if (edge1 <= edge0)
            return x < edge0 ? 0.0 : 1.0;
        double t = std::max(0.0, std::min(1.0, (x - edge0) / (edge1 - edge0)));
        return t * t * (3.0 - 2.0 * t);
    }

    inline double mix(double a, double b, double t)
    {
        return a + (b - a) * t;
    }

    // Gradient noise in about [-1, 1].
    inline double gradient(const Vector3& p, uint32_t seed)
    {
        static const double gradients[12][3] = {
            { 1, 1, 0 }, { -1, 1, 0 }, { 1, -1, 0 }, { -1, -1, 0 },
            { 1, 0, 1 }, { -1, 0, 1 }, { 1, 0, -1 }, { -1, 0, -1 },
            { 0, 1, 1 }, { 0, -1, 1 }, { 0, 1, -1 }, { 0, -1, -1 }
        };
        int ix = (int)std::floor(p.x()), iy = (int)std::floor(p.y()), iz = (int)std::floor(p.z());
        double fx = p.x() - ix, fy = p.y() - iy, fz = p.z() - iz;
        auto fade = [](double t) { return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); };
        double ux = fade(fx), uy = fade(fy), uz = fade(fz);
        auto corner = [&](int dx, int dy, int dz) {
            const double* g = gradients[hash3(ix + dx, iy + dy, iz + dz, seed) % 12];
            return g[0] * (fx - dx) + g[1] * (fy - dy) + g[2] * (fz - dz);
        };
        double x00 = mix(corner(0, 0, 0), corner(1, 0, 0), ux);
        double x10 = mix(corner(0, 1, 0), corner(1, 1, 0), ux);
        double x01 = mix(corner(0, 0, 1), corner(1, 0, 1), ux);
        double x11 = mix(corner(0, 1, 1), corner(1, 1, 1), ux);
        return mix(mix(x00, x10, uy), mix(x01, x11, uy), uz);
    }

    // Fractal sum of a few octaves, about [-1, 1].
    inline double fbm(const Vector3& p, uint32_t seed, int octaves = 4)
    {
        double sum = 0.0, amplitude = 0.5, frequency = 1.0, norm = 0.0;
        for (int i = 0; i < octaves; ++i) {
            sum += amplitude * gradient(p * frequency, seed + (uint32_t)i * 101u);
            norm += amplitude;
            amplitude *= 0.5;
            frequency *= 2.03;
        }
        return sum / norm * 1.6;
    }

}
}

#endif
