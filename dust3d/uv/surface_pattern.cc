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

#include <algorithm>
#include <cmath>
#include <dust3d/uv/surface_pattern.h>

namespace dust3d {

namespace {

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

    inline Color mixColor(const Color& a, const Color& b, double t)
    {
        return Color(mix(a.r(), b.r(), t), mix(a.g(), b.g(), t), mix(a.b(), b.b(), t), a.alpha());
    }

    // Gradient noise in about [-1, 1].
    double gradientNoise(const Vector3& p, uint32_t seed)
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
    double fbm(const Vector3& p, uint32_t seed, int octaves = 4)
    {
        double sum = 0.0, amplitude = 0.5, frequency = 1.0, norm = 0.0;
        for (int i = 0; i < octaves; ++i) {
            sum += amplitude * gradientNoise(p * frequency, seed + (uint32_t)i * 101u);
            norm += amplitude;
            amplitude *= 0.5;
            frequency *= 2.03;
        }
        return sum / norm * 1.6;
    }

    Vector3 warp(const Vector3& p, double amount, uint32_t seed)
    {
        if (amount <= 0.0)
            return p;
        return p + Vector3(fbm(p, seed + 11u, 3), fbm(p, seed + 23u, 3), fbm(p, seed + 37u, 3)) * amount;
    }

    struct Cells {
        double f1 = 1e9;
        double f2 = 1e9;
        uint32_t id = 0;
    };

    // Distances to the nearest and second nearest of one jittered point per unit cell.
    Cells worley(const Vector3& p, uint32_t seed)
    {
        Cells cells;
        int ix = (int)std::floor(p.x()), iy = (int)std::floor(p.y()), iz = (int)std::floor(p.z());
        for (int dz = -1; dz <= 1; ++dz) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    int cx = ix + dx, cy = iy + dy, cz = iz + dz;
                    uint32_t h = hash3(cx, cy, cz, seed);
                    Vector3 feature(cx + 0.15 + 0.7 * unit(h),
                        cy + 0.15 + 0.7 * unit(h * 747796405u + 2891336453u),
                        cz + 0.15 + 0.7 * unit(h * 2654435761u + 40503u));
                    double d = (feature - p).length();
                    if (d < cells.f1) {
                        cells.f2 = cells.f1;
                        cells.f1 = d;
                        cells.id = h;
                    } else if (d < cells.f2) {
                        cells.f2 = d;
                    }
                }
            }
        }
        return cells;
    }

}

SurfacePattern::Type SurfacePattern::typeFromString(const std::string& name)
{
    if ("Spots" == name)
        return Type::Spots;
    if ("Rosettes" == name)
        return Type::Rosettes;
    if ("Stripes" == name)
        return Type::Stripes;
    if ("Patches" == name)
        return Type::Patches;
    if ("Mottled" == name)
        return Type::Mottled;
    return Type::None;
}

const char* SurfacePattern::typeToString(Type type)
{
    switch (type) {
    case Type::Spots:
        return "Spots";
    case Type::Rosettes:
        return "Rosettes";
    case Type::Stripes:
        return "Stripes";
    case Type::Patches:
        return "Patches";
    case Type::Mottled:
        return "Mottled";
    default:
        return "";
    }
}

Color SurfacePattern::defaultPatternColor(const Color& baseColor)
{
    return Color(baseColor.r() * 0.22, baseColor.g() * 0.18, baseColor.b() * 0.15, baseColor.alpha());
}

uint32_t SurfacePattern::seedFromString(const std::string& text)
{
    uint32_t h = 2166136261u;
    for (unsigned char c : text) {
        h ^= c;
        h *= 16777619u;
    }
    return h;
}

SurfacePattern::SurfacePattern(const Settings& settings, const std::vector<Vector3>& surfacePoints)
    : m_settings(settings)
{
    m_settings.scale = std::max(1e-4, m_settings.scale);
    m_settings.belly = std::max(0.0, std::min(1.0, m_settings.belly));
    if (surfacePoints.empty())
        return;
    for (const auto& p : surfacePoints)
        m_center += p;
    m_center = m_center / (double)surfacePoints.size();
    // the long axis: the principal direction of the points (power iteration on the covariance)
    double c[3][3] = { { 0 } };
    for (const auto& p : surfacePoints) {
        Vector3 d = p - m_center;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                c[i][j] += d[i] * d[j];
    }
    Vector3 axis(0.3, 1.0, 0.2);
    for (int iteration = 0; iteration < 32; ++iteration) {
        Vector3 next(c[0][0] * axis[0] + c[0][1] * axis[1] + c[0][2] * axis[2],
            c[1][0] * axis[0] + c[1][1] * axis[1] + c[1][2] * axis[2],
            c[2][0] * axis[0] + c[2][1] * axis[1] + c[2][2] * axis[2]);
        if (next.lengthSquared() < 1e-24)
            break;
        axis = next.normalized();
    }
    m_longAxis = axis;
}

Color SurfacePattern::colorAt(const Vector3& position, const Vector3& normal, double footprint) const
{
    const Color& base = m_settings.baseColor;
    if (Type::None == m_settings.type)
        return base;
    const uint32_t seed = m_settings.seed;
    const Vector3 p = position / m_settings.scale;
    // the texel's size in pattern units: edges are blurred over it, and features smaller
    // than a few texels fade to their average coverage instead of aliasing
    const double texel = std::max(1e-6, footprint / m_settings.scale);
    const double aa = std::max(0.02, texel * 0.75);
    const double fade = smoothstep(0.2, 0.6, texel);

    // a little variation in the base tone, as in any real coat
    Color color = base * (1.0 + 0.06 * fbm(p * 2.5, seed + 401u, 3));
    color.setAlpha(base.alpha());

    double mask = 0.0;
    double average = 0.3;
    Color ink = m_settings.patternColor;
    switch (m_settings.type) {
    case Type::Spots: {
        Cells cells = worley(warp(p, 0.22, seed), seed);
        double radius = 0.27 + 0.13 * unit(cells.id);
        mask = 1.0 - smoothstep(radius - aa, radius + aa, cells.f1);
        average = 0.22;
        break;
    }
    case Type::Rosettes: {
        Vector3 q = warp(p, 0.3, seed);
        Cells cells = worley(q, seed);
        double radius = 0.30 + 0.06 * unit(cells.id);
        double width = 0.075 + 0.03 * unit(cells.id * 2246822519u);
        double ring = 1.0 - smoothstep(width - aa, width + aa, std::abs(cells.f1 - radius));
        // the ring is broken into a few blotches
        double breaks = gradientNoise(q * 2.8 + Vector3(0.37, 0.11, 0.73) * (double)(cells.id % 97u), seed + 59u);
        ring *= smoothstep(-0.3, 0.05, breaks);
        double inside = 1.0 - smoothstep(radius - width - aa, radius - width + aa, cells.f1);
        color = mixColor(color, mixColor(base, ink, 0.28), inside * (1.0 - fade));
        mask = ring;
        average = 0.25;
        break;
    }
    case Type::Stripes: {
        double u = Vector3::dotProduct(position - m_center, m_longAxis) / m_settings.scale;
        u += 0.55 * fbm(p * 0.45, seed + 71u, 3);
        double wave = std::sin(2.0 * M_PI * u);
        // the stripes thin, break and fork along their length
        double threshold = 0.25 + 0.45 * fbm(p * 1.1, seed + 83u, 3);
        double edge = std::max(0.05, aa * 2.0 * M_PI);
        mask = smoothstep(threshold - edge, threshold + edge, wave);
        average = 0.3;
        break;
    }
    case Type::Patches: {
        Cells cells = worley(warp(p, 0.18, seed), seed);
        double line = 0.06 + 0.04 * (0.5 + 0.5 * fbm(p * 1.7, seed + 97u, 2));
        mask = smoothstep(line - aa, line + aa, cells.f2 - cells.f1);
        // each patch a slightly different shade
        ink = ink * (0.88 + 0.24 * unit(cells.id));
        ink.setAlpha(m_settings.patternColor.alpha());
        average = 0.8;
        break;
    }
    case Type::Mottled: {
        double n = fbm(warp(p, 0.5, seed), seed + 113u, 4);
        double edge = std::max(0.06, aa * 1.5);
        mask = smoothstep(0.12 - edge, 0.12 + edge, n);
        // a second, fainter layer of blotches
        double n2 = fbm(p * 1.9, seed + 131u, 3);
        color = mixColor(color, mixColor(base, ink, 0.35), smoothstep(0.2 - edge, 0.2 + edge, n2) * (1.0 - fade));
        average = 0.35;
        break;
    }
    default:
        break;
    }
    mask = mix(mask, average, fade);

    // countershading: the underside lighter, the pattern fading on it
    double belly = 0.0;
    if (m_settings.belly > 0.0)
        belly = m_settings.belly * smoothstep(0.05, 0.75, -normal.y());
    mask *= 1.0 - 0.85 * belly;
    color = mixColor(color, ink, mask);
    if (belly > 0.0) {
        Color light = mixColor(base, Color(1.0, 0.97, 0.92), 0.6);
        color = mixColor(color, light, belly);
    }
    for (int i = 0; i < 3; ++i)
        color[i] = std::max(0.0, std::min(1.0, color[i]));
    return color;
}

}
