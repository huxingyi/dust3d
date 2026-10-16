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
#include <dust3d/uv/procedural_noise.h>
#include <dust3d/uv/surface_ornament.h>

namespace dust3d {

namespace {

    using noise::smoothstep;

    // 1 inside a band of half width `half` around distance 0, 0 outside, filtered over `aa`
    inline double band(double distance, double half, double aa)
    {
        return 1.0 - smoothstep(half - aa, half + aa, distance);
    }

    // The rounded cross-section of a strand: 1 on its centre line, 0 at its edge
    inline double strandProfile(double distance, double half)
    {
        double t = std::min(1.0, distance / half);
        return std::sqrt(std::max(0.0, 1.0 - t * t));
    }

    // Interlaced diagonal bands, period 1 both ways. One family runs along s + t = k, the
    // other along s - t = m; at the crossing (k, m) the first passes over when k + m is
    // even, so every band goes over, under, over along its length.
    SurfaceOrnament::Sample knotwork(double s, double t, double aa)
    {
        const double half = 0.21;
        const double gap = 0.075;
        double a = s + t, b = s - t;
        double ka = std::round(a), kb = std::round(b);
        double da = std::abs(a - ka), db = std::abs(b - kb);
        bool firstOver = 0 == (((long long)ka + (long long)kb) & 1);
        double over = firstOver ? da : db;
        double under = firstOver ? db : da;
        // the band underneath stops short of the one above it
        double overMask = band(over, half, aa);
        double underMask = band(under, half, aa) * smoothstep(half + gap - aa, half + gap + aa, over);
        SurfaceOrnament::Sample sample;
        sample.ink = std::max(overMask, underMask);
        // a fine line down the middle of each band, as on carved interlace
        double groove = 1.0 - 0.35 * band(overMask >= underMask ? over : under, 0.035, aa);
        sample.ink *= groove;
        double overHeight = overMask * (0.55 + 0.45 * strandProfile(over, half));
        // the lower band dips towards the crossing
        double dip = smoothstep(half + gap, half + gap + 0.25, over);
        double underHeight = underMask * (0.25 + 0.3 * dip) * (0.55 + 0.45 * strandProfile(under, half));
        sample.height = std::max(overHeight, underHeight);
        return sample;
    }

    // A three-strand plait along s (period 1), filling t = 0..1.
    SurfaceOrnament::Sample braid(double s, double t, double aa)
    {
        const double half = 0.17;
        SurfaceOrnament::Sample sample;
        double topDepth = -2.0;
        for (int i = 0; i < 3; ++i) {
            double phase = 2.0 * M_PI * (s + i / 3.0);
            double centre = 0.5 + 0.27 * std::sin(phase);
            // the slope of the strand: measure the distance across it, not straight up
            double slope = 0.27 * 2.0 * M_PI * std::cos(phase);
            double distance = std::abs(t - centre) / std::sqrt(1.0 + slope * slope * 0.25);
            double mask = band(distance, half, aa);
            if (mask <= 0.0)
                continue;
            // a strand is on top while it climbs through the middle, underneath on its way back
            double depth = std::cos(phase);
            if (mask > 0.5 ? depth > topDepth : sample.ink < mask) {
                if (mask > 0.5)
                    topDepth = depth;
                sample.ink = mask * (1.0 - 0.3 * band(distance, 0.03, aa));
                sample.height = mask * (0.5 + 0.2 * depth) * (0.5 + 0.5 * strandProfile(distance, half));
            }
        }
        return sample;
    }

    // Quilting: stitched lines along both diagonals, the cells padded between them.
    SurfaceOrnament::Sample diamonds(double s, double t, double aa)
    {
        double a = s + t, b = s - t;
        double da = std::abs(a - std::round(a)), db = std::abs(b - std::round(b));
        double nearest = std::min(da, db);
        SurfaceOrnament::Sample sample;
        sample.ink = band(nearest, 0.04, aa);
        // a stud where the lines cross
        double stud = band(std::sqrt(da * da + db * db), 0.11, aa);
        sample.ink = std::max(sample.ink, stud);
        double pad = std::sin(M_PI * std::min(1.0, da * 2.0)) * std::sin(M_PI * std::min(1.0, db * 2.0));
        sample.height = std::max(0.85 * std::sqrt(std::max(0.0, pad)) * (1.0 - stud), stud);
        return sample;
    }

    // Strands along l, one per unit of a. They sway together (a slow wave along their length),
    // each has its own tone, and a dark line lies in the hollow between two of them.
    SurfaceOrnament::Sample strands(double a, double l, double aa)
    {
        double swayed = a + 0.32 * std::sin(2.0 * M_PI * (0.55 * l) + 0.7 * a) + 0.12 * std::sin(2.0 * M_PI * (1.7 * l) + 2.3 * a);
        double index = std::floor(swayed);
        double across = swayed - index;
        auto toneOf = [](double i) {
            double h = std::sin(i * 12.9898 + 4.1) * 43758.5453;
            return h - std::floor(h);
        };
        // (strands come in locks: a slower tone over several of them as well)
        double tone = 0.6 * toneOf(index) + 0.4 * toneOf(std::floor(index / 5.0) + 31.0);
        double edge = std::min(across, 1.0 - across);
        double hollow = 1.0 - smoothstep(0.06 - aa, 0.06 + aa * 2.0 + 0.1, edge);
        SurfaceOrnament::Sample sample;
        sample.ink = std::max(0.0, std::min(1.0, 0.55 * tone + 0.75 * hollow));
        sample.height = (0.45 + 0.55 * tone) * strandProfile(std::abs(across - 0.5), 0.5);
        return sample;
    }

}

SurfaceOrnament::Type SurfaceOrnament::typeFromString(const std::string& name)
{
    if ("Knotwork" == name)
        return Type::Knotwork;
    if ("Braid" == name)
        return Type::Braid;
    if ("Diamonds" == name)
        return Type::Diamonds;
    if ("Strands" == name)
        return Type::Strands;
    return Type::None;
}

const char* SurfaceOrnament::typeToString(Type type)
{
    switch (type) {
    case Type::Knotwork:
        return "Knotwork";
    case Type::Braid:
        return "Braid";
    case Type::Diamonds:
        return "Diamonds";
    case Type::Strands:
        return "Strands";
    default:
        return "";
    }
}

Color SurfaceOrnament::defaultOrnamentColor(const Color& baseColor)
{
    return Color(baseColor.r() * 0.5, baseColor.g() * 0.47, baseColor.b() * 0.45, baseColor.alpha());
}

SurfaceOrnament::SurfaceOrnament(const Settings& settings)
    : m_settings(settings)
{
    m_settings.scale = std::max(1e-4, m_settings.scale);
    m_settings.relief = std::max(0.0, std::min(2.0, m_settings.relief));
}

void SurfaceOrnament::setChartSize(double lengthU, double lengthV)
{
    m_lengthU = std::max(1e-9, lengthU);
    m_lengthV = std::max(1e-9, lengthV);
    m_repeatsU = std::max(1.0, std::round(m_lengthU / m_settings.scale));
    m_repeatsV = std::max(1.0, std::round(m_lengthV / m_settings.scale));
    m_alongV = m_lengthV > m_lengthU;
    if (Type::Braid == m_settings.type) {
        // one plait across the short side; along the long side one repeat is about one and
        // a half times as long as the plait is wide
        double across = m_alongV ? m_lengthU : m_lengthV;
        double along = m_alongV ? m_lengthV : m_lengthU;
        double repeats = std::max(1.0, std::round(along / (1.5 * across)));
        m_repeatsU = m_alongV ? 1.0 : repeats;
        m_repeatsV = m_alongV ? repeats : 1.0;
    }
}

SurfaceOrnament::Sample SurfaceOrnament::sampleAt(double u, double v, double footprint) const
{
    double s = u * m_repeatsU, t = v * m_repeatsV;
    // the texel's size in repeats, the larger of the two ways
    double texel = footprint * std::max(m_repeatsU / m_lengthU, m_repeatsV / m_lengthV);
    double aa = std::max(0.012, texel * 0.75);
    Sample sample;
    switch (m_settings.type) {
    case Type::Knotwork:
        sample = knotwork(s, t, aa);
        break;
    case Type::Braid:
        sample = m_alongV ? braid(t, s - std::floor(s), aa) : braid(s, t - std::floor(t), aa);
        break;
    case Type::Diamonds:
        sample = diamonds(s, t, aa);
        break;
    case Type::Strands:
        // along the long side; one sway about every eight strand widths
        sample = m_alongV ? strands(s, t * 0.125, aa) : strands(t, s * 0.125, aa);
        break;
    default:
        return sample;
    }
    // details finer than a couple of texels fade to their average instead of aliasing
    double fade = smoothstep(0.12, 0.35, texel);
    sample.ink = noise::mix(sample.ink, 0.4, fade);
    sample.height = noise::mix(sample.height, 0.4, fade);
    return sample;
}

void SurfaceOrnament::setHangAxis(const Vector3& centre, double radius)
{
    m_hangCentre = centre;
    m_hangRadius = std::max(1e-6, radius);
}

Color SurfaceOrnament::colorAtPosition(const Color& ground, const Vector3& position, double footprint) const
{
    // around the axis (the seam of the count at the front, +z: on a head that is the face)
    double angle = std::atan2(position.x() - m_hangCentre.x(), -(position.z() - m_hangCentre.z()));
    double around = angle * m_hangRadius / m_settings.scale;
    double down = -position.y() / m_settings.scale;
    double aa = std::max(0.012, footprint / m_settings.scale * 0.75);
    Sample sample = strands(around, down * 0.125, aa);
    double fade = smoothstep(0.12, 0.35, footprint / m_settings.scale);
    sample.ink = noise::mix(sample.ink, 0.4, fade);
    const Color& ink = m_settings.ornamentColor;
    double k = std::max(0.0, std::min(1.0, sample.ink)) * ink.alpha();
    return Color(noise::mix(ground.r(), ink.r(), k), noise::mix(ground.g(), ink.g(), k),
        noise::mix(ground.b(), ink.b(), k), ground.alpha());
}

Color SurfaceOrnament::colorAt(const Color& ground, double u, double v, double footprint) const
{
    if (Type::None == m_settings.type)
        return ground;
    Sample sample = sampleAt(u, v, footprint);
    const Color& ink = m_settings.ornamentColor;
    double k = std::max(0.0, std::min(1.0, sample.ink)) * ink.alpha();
    return Color(noise::mix(ground.r(), ink.r(), k), noise::mix(ground.g(), ink.g(), k),
        noise::mix(ground.b(), ink.b(), k), ground.alpha());
}

double SurfaceOrnament::heightAt(double u, double v, double footprint) const
{
    if (Type::None == m_settings.type || m_settings.relief <= 0.0)
        return 0.0;
    // the relief is a small part of one repeat: carving, not sculpture
    return sampleAt(u, v, footprint).height * m_settings.relief * m_settings.scale * 0.07;
}

}
