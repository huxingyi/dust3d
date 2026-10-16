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

#ifndef DUST3D_UV_SURFACE_ORNAMENT_H_
#define DUST3D_UV_SURFACE_ORNAMENT_H_

#include <dust3d/base/color.h>
#include <dust3d/base/vector3.h>
#include <string>

namespace dust3d {

// SurfaceOrnament: worked decoration on a part's surface: engraved armour, tooled leather,
// embroidered cloth, a braided strap.
//
// Unlike a SurfacePattern (an animal coat, a function of the 3D position), an ornament is
// laid out in the part's own surface coordinates (its UV chart), so it runs around a
// bracer or along a belt, and it repeats a whole number of times across the chart: around
// a tube it closes on itself without a seam. It gives an ink mask for the colour texture
// and a height for the normal map (the bands stand proud of the surface).
//
//   Knotwork  interlaced diagonal bands passing over and under each other
//   Braid     one three-strand plait running along the part's long side (belts, straps, trim)
//   Diamonds  quilting: a lattice of stitched lines with the cells padded between them
//   Strands   hair, fur, thatch, fringe: fine strands running along the part's long side, each
//             a little lighter or darker than its neighbours, swaying together
class SurfaceOrnament {
public:
    enum class Type {
        None = 0,
        Knotwork,
        Braid,
        Diamonds,
        Strands
    };

    struct Settings {
        Type type = Type::None;
        Color baseColor = Color(1.0, 1.0, 1.0);
        Color ornamentColor = Color(0.3, 0.3, 0.3);
        // Size of one repeat in world units.
        double scale = 0.03;
        // 0..2: how far the ornament stands out in the normal map (0: colour only).
        double relief = 1.0;
    };

    struct Sample {
        // 0..1: how much of the ornament colour
        double ink = 0.0;
        // 0..1: raised above the ground of the surface
        double height = 0.0;
    };

    static Type typeFromString(const std::string& name);
    static const char* typeToString(Type type);
    // The colour an ornament defaults to when none is set: a darker tone of the base colour.
    static Color defaultOrnamentColor(const Color& baseColor);

    SurfaceOrnament(const Settings& settings);

    // Lay the ornament out on a chart `lengthU` by `lengthV` world units: a whole number
    // of repeats each way. A braid runs along the longer side, one plait wide.
    void setChartSize(double lengthU, double lengthV);

    // `u`, `v`: 0..1 across the chart. `footprint`: the world size of a texel (edges are
    // filtered to it).
    Sample sampleAt(double u, double v, double footprint) const;
    Color colorAt(const Color& ground, double u, double v, double footprint) const;
    // World height of the relief at (u, v), for the normal map.
    double heightAt(double u, double v, double footprint) const;
    // World lengths of the chart's sides, as set.
    double lengthU() const { return m_lengthU; }
    double lengthV() const { return m_lengthV; }

    const Settings& settings() const { return m_settings; }

    // Strands hang: they are laid out from where the surface is, not from its UV chart
    // (whose directions mean nothing on a wrapped surface). They run down (world -y) and
    // are counted around the vertical axis through `centre`, `radius` away from it, so on a
    // head they flow from the crown and on a sheet of hair they fall straight.
    bool hangs() const { return Type::Strands == m_settings.type; }
    void setHangAxis(const Vector3& centre, double radius);
    Color colorAtPosition(const Color& ground, const Vector3& position, double footprint) const;

private:
    Vector3 m_hangCentre;
    double m_hangRadius = 0.0;
    Settings m_settings;
    double m_lengthU = 1.0;
    double m_lengthV = 1.0;
    double m_repeatsU = 1.0;
    double m_repeatsV = 1.0;
    bool m_alongV = false;
};

}

#endif
