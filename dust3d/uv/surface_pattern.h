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

#ifndef DUST3D_UV_SURFACE_PATTERN_H_
#define DUST3D_UV_SURFACE_PATTERN_H_

#include <cstdint>
#include <dust3d/base/color.h>
#include <dust3d/base/vector3.h>
#include <string>
#include <vector>

namespace dust3d {

// SurfacePattern: an animal coat painted onto a surface (the wrap modifier's creature skin).
//
// The pattern is a function of the 3D position on the surface, not of the UVs, so it runs
// across UV chart seams without a break and does not stretch with the unwrap: the texture
// baker evaluates it at the surface point behind every texel.
//
//   Spots     round, slightly irregular spots (cheetah, dalmatian, fawn)
//   Rosettes  broken rings around a slightly darker centre (leopard, jaguar)
//   Stripes   bands across the body's long axis that thin, break and fork (tiger, zebra)
//   Patches   irregular polygons parted by thin lines of the base colour (giraffe)
//   Mottled   soft blotches (frog, salamander, cow, camouflage)
//
// `belly` lightens the underside (countershading: nearly every animal is lighter below)
// and fades the pattern there.
class SurfacePattern {
public:
    enum class Type {
        None = 0,
        Spots,
        Rosettes,
        Stripes,
        Patches,
        Mottled
    };

    struct Settings {
        Type type = Type::None;
        Color baseColor = Color(1.0, 1.0, 1.0);
        Color patternColor = Color(0.1, 0.08, 0.06);
        // Size of one feature (a spot, a rosette, the gap between stripes) in world units.
        double scale = 0.06;
        // 0..1: how much lighter the underside is.
        double belly = 0.0;
        uint32_t seed = 0;
    };

    static Type typeFromString(const std::string& name);
    static const char* typeToString(Type type);
    // The colour a pattern defaults to when none is set: a dark tone of the base colour.
    static Color defaultPatternColor(const Color& baseColor);
    static uint32_t seedFromString(const std::string& text);

    // `surfacePoints`: points spread over the surface (its vertices), for the body's long
    // axis that stripes run across.
    SurfacePattern(const Settings& settings, const std::vector<Vector3>& surfacePoints);

    // `normal` is the unit surface normal; `footprint` is the world-space size of the texel
    // (the pattern is filtered to it, so small features fade instead of aliasing).
    Color colorAt(const Vector3& position, const Vector3& normal, double footprint) const;

    const Settings& settings() const { return m_settings; }
    const Vector3& longAxis() const { return m_longAxis; }

private:
    Settings m_settings;
    Vector3 m_center;
    Vector3 m_longAxis = Vector3(0.0, 1.0, 0.0);
};

}

#endif
