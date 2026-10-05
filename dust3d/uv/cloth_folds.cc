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
#include <array>
#include <cmath>
#include <cstdio>
#include <dust3d/uv/cloth_folds.h>
#include <functional>
#include <map>
#include <queue>
#include <set>
#include <sstream>

namespace dust3d {

namespace {

    inline double smoothstep(double edge0, double edge1, double x)
    {
        if (edge1 <= edge0)
            return x < edge0 ? 0.0 : 1.0;
        double t = std::max(0.0, std::min(1.0, (x - edge0) / (edge1 - edge0)));
        return t * t * (3.0 - 2.0 * t);
    }

    // A small deterministic variation (-1..1) from an integer: the same garment always
    // gets the same folds.
    inline double jitter(uint32_t value)
    {
        value ^= value >> 16;
        value *= 0x7feb352du;
        value ^= value >> 15;
        value *= 0x846ca68bu;
        value ^= value >> 16;
        return (double)(value & 0xffffffu) / (double)0x800000 - 1.0;
    }

    // `v` (in the plane orthogonal to `axis`) turned by `angle` around `axis`.
    inline Vector3 rotated(const Vector3& v, const Vector3& axis, double angle)
    {
        return v * std::cos(angle) + Vector3::crossProduct(axis, v) * std::sin(angle);
    }

    inline Vector3 tangential(const Vector3& v, const Vector3& normal)
    {
        return v - normal * Vector3::dotProduct(v, normal);
    }

    inline uint64_t cellKey(int x, int y, int z)
    {
        return ((uint64_t)(uint32_t)(x + (1 << 20)) << 42) | ((uint64_t)(uint32_t)(y + (1 << 20)) << 21) | (uint64_t)(uint32_t)(z + (1 << 20));
    }

    const double kPi = 3.14159265358979323846;

    // Closest point on segment ab to p.
    inline Vector3 closestOnSegment(const Vector3& p, const Vector3& a, const Vector3& b)
    {
        Vector3 ab = b - a;
        double l2 = ab.lengthSquared();
        if (l2 < 1e-24)
            return a;
        double t = std::max(0.0, std::min(1.0, Vector3::dotProduct(p - a, ab) / l2));
        return a + ab * t;
    }

    // Closest point on triangle abc to p, with its barycentric weights.
    Vector3 closestOnTriangle(const Vector3& p, const Vector3& a, const Vector3& b, const Vector3& c, double weights[3])
    {
        Vector3 ab = b - a, ac = c - a, ap = p - a;
        double d1 = Vector3::dotProduct(ab, ap), d2 = Vector3::dotProduct(ac, ap);
        if (d1 <= 0.0 && d2 <= 0.0) {
            weights[0] = 1.0, weights[1] = 0.0, weights[2] = 0.0;
            return a;
        }
        Vector3 bp = p - b;
        double d3 = Vector3::dotProduct(ab, bp), d4 = Vector3::dotProduct(ac, bp);
        if (d3 >= 0.0 && d4 <= d3) {
            weights[0] = 0.0, weights[1] = 1.0, weights[2] = 0.0;
            return b;
        }
        double vc = d1 * d4 - d3 * d2;
        if (vc <= 0.0 && d1 >= 0.0 && d3 <= 0.0) {
            double v = d1 / (d1 - d3);
            weights[0] = 1.0 - v, weights[1] = v, weights[2] = 0.0;
            return a + ab * v;
        }
        Vector3 cp = p - c;
        double d5 = Vector3::dotProduct(ab, cp), d6 = Vector3::dotProduct(ac, cp);
        if (d6 >= 0.0 && d5 <= d6) {
            weights[0] = 0.0, weights[1] = 0.0, weights[2] = 1.0;
            return c;
        }
        double vb = d5 * d2 - d1 * d6;
        if (vb <= 0.0 && d2 >= 0.0 && d6 <= 0.0) {
            double w = d2 / (d2 - d6);
            weights[0] = 1.0 - w, weights[1] = 0.0, weights[2] = w;
            return a + ac * w;
        }
        double va = d3 * d6 - d5 * d4;
        if (va <= 0.0 && (d4 - d3) >= 0.0 && (d5 - d6) >= 0.0) {
            double w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
            weights[0] = 0.0, weights[1] = 1.0 - w, weights[2] = w;
            return b + (c - b) * w;
        }
        double denominator = 1.0 / (va + vb + vc);
        double v = vb * denominator, w = vc * denominator;
        weights[0] = 1.0 - v - w, weights[1] = v, weights[2] = w;
        return a + ab * v + ac * w;
    }

    // Triangles by grid cell, for finding the closest point of a triangle mesh.
    class TriangleGrid {
    public:
        TriangleGrid(const std::vector<Vector3>& vertices, const std::vector<std::array<size_t, 3>>& triangles, double cellSize)
            : m_vertices(vertices)
            , m_triangles(triangles)
            , m_cellSize(std::max(1e-6, cellSize))
        {
            for (size_t t = 0; t < triangles.size(); ++t) {
                Vector3 low = vertices[triangles[t][0]], high = low;
                for (int k = 1; k < 3; ++k) {
                    const Vector3& p = vertices[triangles[t][k]];
                    low = Vector3(std::min(low.x(), p.x()), std::min(low.y(), p.y()), std::min(low.z(), p.z()));
                    high = Vector3(std::max(high.x(), p.x()), std::max(high.y(), p.y()), std::max(high.z(), p.z()));
                }
                for (int x = cell(low.x()); x <= cell(high.x()); ++x)
                    for (int y = cell(low.y()); y <= cell(high.y()); ++y)
                        for (int z = cell(low.z()); z <= cell(high.z()); ++z)
                            m_cells.push_back({ cellKey(x, y, z), t });
            }
            std::sort(m_cells.begin(), m_cells.end());
        }
        // The closest point within `maxDistance` (false if none).
        bool closest(const Vector3& p, double maxDistance, Vector3* point, size_t* triangle, double weights[3]) const
        {
            int cx = cell(p.x()), cy = cell(p.y()), cz = cell(p.z());
            int rings = (int)std::ceil(maxDistance / m_cellSize);
            double best = maxDistance * maxDistance;
            bool found = false;
            for (int r = 0; r <= rings; ++r) {
                for (int x = cx - r; x <= cx + r; ++x) {
                    for (int y = cy - r; y <= cy + r; ++y) {
                        for (int z = cz - r; z <= cz + r; ++z) {
                            if (std::max(std::abs(x - cx), std::max(std::abs(y - cy), std::abs(z - cz))) != r)
                                continue;
                            auto range = std::equal_range(m_cells.begin(), m_cells.end(), std::make_pair(cellKey(x, y, z), (size_t)0),
                                [](const std::pair<uint64_t, size_t>& a, const std::pair<uint64_t, size_t>& b) { return a.first < b.first; });
                            for (auto it = range.first; it != range.second; ++it) {
                                const auto& t = m_triangles[it->second];
                                double w[3];
                                Vector3 q = closestOnTriangle(p, m_vertices[t[0]], m_vertices[t[1]], m_vertices[t[2]], w);
                                double d = (q - p).lengthSquared();
                                if (d < best) {
                                    best = d;
                                    found = true;
                                    if (point)
                                        *point = q;
                                    if (triangle)
                                        *triangle = it->second;
                                    if (weights)
                                        weights[0] = w[0], weights[1] = w[1], weights[2] = w[2];
                                }
                            }
                        }
                    }
                }
                // nothing nearer can be in the next ring
                if (found && std::sqrt(best) <= r * m_cellSize)
                    break;
            }
            return found;
        }

    private:
        int cell(double value) const { return (int)std::floor(value / m_cellSize); }
        const std::vector<Vector3>& m_vertices;
        const std::vector<std::array<size_t, 3>>& m_triangles;
        double m_cellSize;
        std::vector<std::pair<uint64_t, size_t>> m_cells;
    };

}

// ---------------------------------------------------------------------------------------
// Evaluation

ClothFolds::ClothFolds(const std::vector<Fold>& folds, double strength)
    : m_strength(std::max(0.0, std::min(1.0, strength)))
{
    double maxReach = 0.0;
    for (const auto& fold : folds) {
        if (fold.points.size() < 2 || fold.normals.size() != fold.points.size() || fold.width <= 0.0)
            continue;
        FoldData data;
        data.fold = fold;
        data.fold.peak = std::max(0.02, std::min(0.98, fold.peak));
        data.fold.endDepth = std::max(0.0, std::min(1.0, fold.endDepth));
        data.fold.crease = std::max(0.0, std::min(1.0, fold.crease));
        data.fold.ridgeSide = fold.ridgeSide < 0.0 ? -1.0 : 1.0;
        for (auto& normal : data.fold.normals) {
            if (normal.lengthSquared() > 1e-24)
                normal.normalize();
        }
        data.arc.push_back(0.0);
        for (size_t i = 1; i < fold.points.size(); ++i)
            data.arc.push_back(data.arc.back() + (fold.points[i] - fold.points[i - 1]).length());
        data.length = data.arc.back();
        if (data.length <= 1e-9)
            continue;
        // the direction along it at each point, between its segments' (so the frame turns
        // smoothly round a bend)
        for (size_t i = 0; i < fold.points.size(); ++i) {
            Vector3 before = i > 0 ? fold.points[i] - fold.points[i - 1] : Vector3();
            Vector3 after = i + 1 < fold.points.size() ? fold.points[i + 1] - fold.points[i] : Vector3();
            if (before.lengthSquared() > 1e-24)
                before.normalize();
            if (after.lengthSquared() > 1e-24)
                after.normalize();
            Vector3 tangent = before + after;
            data.tangents.push_back(tangent.lengthSquared() > 1e-24 ? tangent.normalized() : (after.lengthSquared() > 0.0 ? after : before));
        }
        // the profile fades out at 2.6 widths across (see foldHeight), the width grows up
        // to 1.15 of the fold's
        data.reach = fold.width * 1.15 * 2.6;
        maxReach = std::max(maxReach, data.reach);
        m_folds.push_back(data);
    }
    if (m_folds.empty())
        return;
    double segmentLength = 0.0;
    size_t segmentCount = 0;
    for (size_t f = 0; f < m_folds.size(); ++f) {
        for (size_t i = 0; i + 1 < m_folds[f].fold.points.size(); ++i) {
            m_segments.push_back({ f, i });
            segmentLength += m_folds[f].arc[i + 1] - m_folds[f].arc[i];
            ++segmentCount;
        }
    }
    m_cellSize = std::max(1e-5, std::max(maxReach, segmentLength / std::max((size_t)1, segmentCount)));
    for (size_t i = 0; i < m_segments.size(); ++i) {
        const auto& data = m_folds[m_segments[i].fold];
        const Vector3& a = data.fold.points[m_segments[i].index];
        const Vector3& b = data.fold.points[m_segments[i].index + 1];
        double r = data.reach;
        int x0 = (int)std::floor((std::min(a.x(), b.x()) - r) / m_cellSize);
        int y0 = (int)std::floor((std::min(a.y(), b.y()) - r) / m_cellSize);
        int z0 = (int)std::floor((std::min(a.z(), b.z()) - r) / m_cellSize);
        int x1 = (int)std::floor((std::max(a.x(), b.x()) + r) / m_cellSize);
        int y1 = (int)std::floor((std::max(a.y(), b.y()) + r) / m_cellSize);
        int z1 = (int)std::floor((std::max(a.z(), b.z()) + r) / m_cellSize);
        for (int x = x0; x <= x1; ++x)
            for (int y = y0; y <= y1; ++y)
                for (int z = z0; z <= z1; ++z)
                    m_cellSegments.push_back({ cellKey(x, y, z), i });
    }
    std::sort(m_cellSegments.begin(), m_cellSegments.end());
}

double ClothFolds::foldHeight(const FoldData& data, double s, double t, double* crease) const
{
    const Fold& fold = data.fold;
    double u = s / data.length;
    if (u <= 0.0 || u >= 1.0)
        return 0.0;
    // along: rising to the deepest point, then falling to the end depth
    double rise = smoothstep(0.0, fold.peak, u);
    double envelope = u < fold.peak ? rise : 1.0 - (1.0 - fold.endDepth) * smoothstep(fold.peak, 1.0, u);
    if (fold.endDepth > 0.0)
        envelope *= 1.0 - smoothstep(1.0 - 0.6 * fold.width / data.length, 1.0, u); // into the hem
    envelope = std::pow(envelope, 1.25);
    if (envelope <= 0.0)
        return 0.0;
    // narrower where it is shallow: a hanging fold is pinched where it leaves its support
    // and full from its deepest point on; the others narrow toward both ends
    double narrowest = Kind::Hang == fold.kind ? 0.35 : (Kind::Tension == fold.kind ? 0.6 : 0.45);
    double fullness = Kind::Hang == fold.kind ? std::pow(rise, 1.25) : envelope;
    double width = fold.width * (narrowest + (1.15 - narrowest) * std::sqrt(fullness));
    // across, like a sheet folded over: a crease line, a broad soft ridge (the "pillow")
    // rising on one side of it, a gentle trough on the other; a soft fold (a pipe) is a
    // tube with a trough on either side
    double ridgeOffset = fold.ridgeSide * 0.6 * width * fold.crease;
    double ridgeWidth = width * (0.85 + 0.25 * (1.0 - fold.crease));
    double r = (t - ridgeOffset) / ridgeWidth;
    double ridge = std::exp(-r * r);
    double q = (t + ridgeOffset * 1.1) / (ridgeWidth * 1.4);
    double trough = std::exp(-q * q);
    // (the crease line does not thin out to a scratch where the fold narrows)
    double creaseWidth = std::max(width * (0.16 + 0.3 * (1.0 - fold.crease)), fold.width * 0.22);
    double c = t / creaseWidth;
    double valley = std::exp(-c * c);
    double q2 = (t - ridgeOffset) / (ridgeWidth * 1.6);
    double sides = std::exp(-q2 * q2) - ridge;
    double window = 1.0 - smoothstep(1.8 * width, 2.6 * width, std::abs(t));
    if (nullptr != crease)
        *crease += envelope * fold.crease * valley * window;
    return fold.depth * envelope * window
        * (ridge - fold.crease * (0.38 * valley + 0.3 * trough) - (1.0 - fold.crease) * 0.9 * std::max(0.0, sides));
}

double ClothFolds::heightAt(const Vector3& position, const Vector3& normal, double* crease) const
{
    if (m_folds.empty())
        return 0.0;
    uint64_t key = cellKey((int)std::floor(position.x() / m_cellSize), (int)std::floor(position.y() / m_cellSize),
        (int)std::floor(position.z() / m_cellSize));
    auto range = std::equal_range(m_cellSegments.begin(), m_cellSegments.end(), std::make_pair(key, (size_t)0),
        [](const std::pair<uint64_t, size_t>& a, const std::pair<uint64_t, size_t>& b) { return a.first < b.first; });
    double height = 0.0;
    double creaseSum = 0.0;
    // Where the position is relative to each nearby segment of a fold: along it (s),
    // across it (t). The segments of a fold are together (they were numbered fold by fold);
    // a fold's nearest segments are blended (on the inside of a bend the nearest point jumps
    // from one segment to the next).
    struct Local {
        double distance2, s, t, out, facing;
    };
    Local locals[64];
    size_t localCount = 0;
    size_t currentFold = (size_t)-1;
    auto flush = [&]() {
        if ((size_t)-1 == currentFold || 0 == localCount)
            return;
        const FoldData& data = m_folds[currentFold];
        // the nearest of the segments on this side of the surface
        size_t nearestIndex = (size_t)-1;
        for (size_t i = 0; i < localCount; ++i) {
            if (locals[i].facing < 0.2)
                continue;
            if ((size_t)-1 == nearestIndex || locals[i].distance2 < locals[nearestIndex].distance2)
                nearestIndex = i;
        }
        if ((size_t)-1 == nearestIndex)
            return;
        double nearest = locals[nearestIndex].distance2;
        double sigma2 = data.fold.width * data.fold.width * 0.25;
        double weightSum = 0.0, s = 0.0, t = 0.0, out = 0.0, facing = 0.0;
        for (size_t i = 0; i < localCount; ++i) {
            // (only the stretch of the fold around the nearest point: not the other side of
            // a hairpin)
            double ds = (locals[i].s - locals[nearestIndex].s) / (data.fold.width * 1.5);
            // (fading out with the distance before the reach, and on the other side of the
            // surface: nothing changes where a segment drops out)
            double w = std::exp(-(locals[i].distance2 - nearest) / sigma2 - ds * ds)
                * smoothstep(0.2, 0.5, locals[i].facing)
                * (1.0 - smoothstep(data.reach * 0.8, data.reach, std::sqrt(locals[i].distance2)));
            if (w <= 0.0)
                continue;
            weightSum += w;
            s += w * locals[i].s;
            t += w * locals[i].t;
            out += w * locals[i].out;
            facing += w * locals[i].facing;
        }
        if (weightSum <= 1e-12)
            return;
        double fade = std::min(1.0, weightSum);
        s /= weightSum, t /= weightSum, out /= weightSum, facing /= weightSum;
        // only on the side of the surface the fold is on (not through a sleeve to the torso
        // beside it, or to the far side of a thin part)
        if (facing < 0.3)
            return;
        double reach = std::max(data.fold.width * 2.5, data.reach);
        if (out > reach)
            return;
        double weight = fade * smoothstep(0.3, 0.7, facing) * (1.0 - smoothstep(reach * 0.6, reach, out));
        double foldCrease = 0.0;
        height += weight * foldHeight(data, s, t, &foldCrease);
        creaseSum += weight * foldCrease;
    };
    for (auto it = range.first; it != range.second; ++it) {
        const Segment& segment = m_segments[it->second];
        if (segment.fold != currentFold) {
            flush();
            currentFold = segment.fold;
            localCount = 0;
        }
        const FoldData& data = m_folds[segment.fold];
        const Vector3& a = data.fold.points[segment.index];
        const Vector3& b = data.fold.points[segment.index + 1];
        Vector3 ab = b - a;
        double l2 = ab.lengthSquared();
        double tau = l2 > 1e-24 ? std::max(0.0, std::min(1.0, Vector3::dotProduct(position - a, ab) / l2)) : 0.0;
        Vector3 nearest = a + ab * tau;
        Vector3 offset = position - nearest;
        double distance2 = offset.lengthSquared();
        if (distance2 > data.reach * data.reach)
            continue;
        Vector3 foldNormal = data.fold.normals[segment.index] * (1.0 - tau) + data.fold.normals[segment.index + 1] * tau;
        if (foldNormal.lengthSquared() < 1e-24)
            continue;
        foldNormal.normalize();
        Vector3 along = tangential(data.tangents[segment.index] * (1.0 - tau) + data.tangents[segment.index + 1] * tau, foldNormal);
        if (along.lengthSquared() < 1e-24)
            continue;
        along.normalize();
        Vector3 across = Vector3::crossProduct(foldNormal, along);
        Local local;
        local.distance2 = distance2;
        local.s = data.arc[segment.index] + tau * (data.arc[segment.index + 1] - data.arc[segment.index]);
        // past the ends of the line
        if (0 == segment.index && tau <= 0.0)
            local.s = Vector3::dotProduct(offset, along);
        else if (segment.index + 2 == data.fold.points.size() && tau >= 1.0)
            local.s = data.length + Vector3::dotProduct(offset, along);
        local.t = Vector3::dotProduct(offset, across);
        local.out = std::abs(Vector3::dotProduct(offset, foldNormal));
        local.facing = Vector3::dotProduct(normal, foldNormal);
        if (localCount < 64)
            locals[localCount++] = local;
    }
    flush();
    if (nullptr != crease)
        *crease = std::min(1.0, creaseSum);
    return height * m_strength;
}

void ClothFolds::evaluate(const Vector3& position, const Vector3& normal, const Vector3& tangent, const Vector3& bitangent,
    double footprint, Vector3* tangentSpaceNormal, double* cavity) const
{
    double crease = 0.0;
    heightAt(position, normal, &crease);
    if (nullptr != cavity)
        *cavity = crease * m_strength;
    if (nullptr == tangentSpaceNormal)
        return;
    double step = std::max(footprint * 0.75, 1e-5);
    double du = (heightAt(position + tangent * step, normal) - heightAt(position - tangent * step, normal)) / (2.0 * step);
    double dv = (heightAt(position + bitangent * step, normal) - heightAt(position - bitangent * step, normal)) / (2.0 * step);
    *tangentSpaceNormal = Vector3(-du, -dv, 1.0).normalized();
}

// ---------------------------------------------------------------------------------------
// Serialization (the mesh generator places the folds; the texture generator bakes them)

std::string ClothFolds::serialize(const std::vector<Fold>& folds)
{
    std::string text;
    char buffer[256];
    for (const auto& fold : folds) {
        if (fold.points.size() < 2 || fold.normals.size() != fold.points.size())
            continue;
        std::snprintf(buffer, sizeof(buffer), "%d %zu %.5g %.5g %.4g %.4g %.4g %d",
            (int)fold.kind, fold.points.size(), fold.width, fold.depth, fold.crease, fold.peak, fold.endDepth, fold.ridgeSide < 0.0 ? -1 : 1);
        text += buffer;
        for (size_t i = 0; i < fold.points.size(); ++i) {
            std::snprintf(buffer, sizeof(buffer), " %.6g %.6g %.6g %.3f %.3f %.3f",
                fold.points[i].x(), fold.points[i].y(), fold.points[i].z(),
                fold.normals[i].x(), fold.normals[i].y(), fold.normals[i].z());
            text += buffer;
        }
        text += ";";
    }
    return text;
}

std::vector<ClothFolds::Fold> ClothFolds::deserialize(const std::string& text)
{
    std::vector<Fold> folds;
    std::stringstream stream(text);
    std::string item;
    while (std::getline(stream, item, ';')) {
        std::istringstream fields(item);
        int kind = 0, ridgeSide = 1;
        size_t count = 0;
        Fold fold;
        if (!(fields >> kind >> count >> fold.width >> fold.depth >> fold.crease >> fold.peak >> fold.endDepth >> ridgeSide))
            continue;
        if (kind < 0 || kind > (int)Kind::Crease || count < 2 || count > 4096)
            continue;
        fold.kind = (Kind)kind;
        fold.ridgeSide = ridgeSide < 0 ? -1.0 : 1.0;
        bool good = true;
        for (size_t i = 0; i < count && good; ++i) {
            double v[6];
            for (int k = 0; k < 6 && good; ++k)
                good = (bool)(fields >> v[k]);
            if (good) {
                fold.points.push_back(Vector3(v[0], v[1], v[2]));
                fold.normals.push_back(Vector3(v[3], v[4], v[5]));
            }
        }
        if (good)
            folds.push_back(fold);
    }
    return folds;
}

// ---------------------------------------------------------------------------------------
// Placement

namespace {

    double garmentHeightOf(const std::vector<Vector3>& vertices)
    {
        double low = 1e30, high = -1e30;
        for (const auto& v : vertices) {
            low = std::min(low, v.y());
            high = std::max(high, v.y());
        }
        return std::max(1e-6, high - low);
    }

    // Shortest distances over the edges of a mesh from a set of sources.
    std::vector<double> distancesFrom(const std::vector<Vector3>& vertices,
        const std::vector<std::vector<size_t>>& neighbors,
        const std::vector<char>& sources,
        const std::vector<char>& allowed)
    {
        std::vector<double> distance(vertices.size(), 1e30);
        typedef std::pair<double, size_t> Item;
        std::priority_queue<Item, std::vector<Item>, std::greater<Item>> queue;
        for (size_t v = 0; v < vertices.size(); ++v) {
            if (sources[v] && allowed[v]) {
                distance[v] = 0.0;
                queue.push({ 0.0, v });
            }
        }
        while (!queue.empty()) {
            Item item = queue.top();
            queue.pop();
            if (item.first > distance[item.second])
                continue;
            for (size_t u : neighbors[item.second]) {
                if (!allowed[u])
                    continue;
                double d = item.first + (vertices[u] - vertices[item.second]).length();
                if (d < distance[u]) {
                    distance[u] = d;
                    queue.push({ d, u });
                }
            }
        }
        return distance;
    }

}

std::vector<ClothFolds::Fold> ClothFolds::place(const PlacementInput& input)
{
    std::vector<Fold> folds;
    const auto& vertices = input.vertices;
    const size_t n = vertices.size();
    if (0 == n || input.triangles.empty())
        return folds;
    uint32_t jitterCounter = 0;
    // a small deterministic variation, -1..1
    auto vary = [&]() { return jitter(input.seed + 7919u * (++jitterCounter)); };

    // ---- the surface
    std::vector<Vector3> normals(n);
    std::vector<std::vector<size_t>> neighbors(n);
    std::vector<std::array<size_t, 3>> triangles;
    std::map<std::pair<size_t, size_t>, std::vector<size_t>> edgeFaces;
    std::vector<Vector3> faceNormals;
    for (const auto& face : input.triangles) {
        for (size_t i = 1; i + 1 < face.size(); ++i) {
            if (face[0] >= n || face[i] >= n || face[i + 1] >= n)
                continue;
            triangles.push_back({ face[0], face[i], face[i + 1] });
        }
    }
    for (size_t f = 0; f < triangles.size(); ++f) {
        const auto& t = triangles[f];
        Vector3 weighted = Vector3::crossProduct(vertices[t[1]] - vertices[t[0]], vertices[t[2]] - vertices[t[0]]);
        faceNormals.push_back(weighted.normalized());
        for (int i = 0; i < 3; ++i) {
            size_t a = t[i], b = t[(i + 1) % 3];
            normals[a] += weighted;
            neighbors[a].push_back(b);
            neighbors[b].push_back(a);
            edgeFaces[{ std::min(a, b), std::max(a, b) }].push_back(f);
        }
    }
    Vector3 centroid;
    double top = vertices[0].y(), bottom = vertices[0].y();
    double edgeSum = 0.0;
    size_t edgeCount = 0;
    for (size_t v = 0; v < n; ++v) {
        if (normals[v].lengthSquared() > 1e-30)
            normals[v].normalize();
        auto& ring = neighbors[v];
        std::sort(ring.begin(), ring.end());
        ring.erase(std::unique(ring.begin(), ring.end()), ring.end());
        for (size_t u : ring) {
            edgeSum += (vertices[u] - vertices[v]).length();
            ++edgeCount;
        }
        centroid += vertices[v];
        top = std::max(top, vertices[v].y());
        bottom = std::min(bottom, vertices[v].y());
    }
    centroid = centroid / (double)n;
    const double averageEdge = edgeCount > 0 ? edgeSum / edgeCount : 0.01 * garmentHeightOf(vertices);
    const double garmentHeight = std::max(1e-9, top - bottom);

    // concavity: the neighbours' centre lying out of the surface (a crease), smoothed and
    // scaled so the deepest creases reach 1
    std::vector<double> concavity(n, 0.0);
    {
        for (size_t v = 0; v < n; ++v) {
            if (neighbors[v].empty())
                continue;
            Vector3 center;
            for (size_t u : neighbors[v])
                center += vertices[u];
            center = center / (double)neighbors[v].size();
            concavity[v] = Vector3::dotProduct(center - vertices[v], normals[v]) / averageEdge;
        }
        for (int pass = 0; pass < 4; ++pass) {
            std::vector<double> smoothed(n, 0.0);
            for (size_t v = 0; v < n; ++v) {
                double sum = concavity[v];
                for (size_t u : neighbors[v])
                    sum += concavity[u];
                smoothed[v] = sum / (1.0 + neighbors[v].size());
            }
            concavity.swap(smoothed);
        }
        std::vector<double> positive;
        for (double c : concavity) {
            if (c > 0.0)
                positive.push_back(c);
        }
        if (!positive.empty()) {
            size_t index = (positive.size() - 1) * 9 / 10;
            std::nth_element(positive.begin(), positive.begin() + index, positive.end());
            double reference = std::max(1e-9, positive[index]);
            for (auto& c : concavity)
                c = std::max(0.0, std::min(1.0, c / reference));
        } else {
            std::fill(concavity.begin(), concavity.end(), 0.0);
        }
    }

    // ---- the skeleton
    const auto& nodes = input.nodes;
    const size_t m = nodes.size();
    std::vector<std::vector<size_t>> partNeighbors(m);
    for (const auto& link : input.links) {
        if (link.first >= m || link.second >= m || link.first == link.second)
            continue;
        if (nodes[link.first].group != nodes[link.second].group)
            continue;
        partNeighbors[link.first].push_back(link.second);
        partNeighbors[link.second].push_back(link.first);
    }
    for (auto& list : partNeighbors) {
        std::sort(list.begin(), list.end());
        list.erase(std::unique(list.begin(), list.end()), list.end());
    }
    // the outer side of the surface (away from the bones; the lining of a hem faces in)
    std::vector<char> outer(n, 1);
    if (!input.links.empty()) {
        for (size_t v = 0; v < n; ++v) {
            double best = 1e30;
            Vector3 nearest;
            for (const auto& link : input.links) {
                if (link.first >= m || link.second >= m)
                    continue;
                Vector3 q = closestOnSegment(vertices[v], nodes[link.first].position, nodes[link.second].position);
                double d = (vertices[v] - q).lengthSquared();
                if (d < best) {
                    best = d;
                    nearest = q;
                }
            }
            if (best < 1e29)
                outer[v] = Vector3::dotProduct(normals[v], vertices[v] - nearest) > 0.0 ? 1 : 0;
        }
    }
    double skeletonTop = -1e30, skeletonBottom = 1e30;
    for (const auto& node : nodes) {
        skeletonTop = std::max(skeletonTop, node.position.y());
        skeletonBottom = std::min(skeletonBottom, node.position.y());
    }
    const double skeletonHeight = m > 1 ? std::max(1e-9, skeletonTop - skeletonBottom) : garmentHeight;
    // the size of the figure, the unit for how far cloth stands off the body
    const double figure = std::max(skeletonHeight, garmentHeight);
    // the width of one fold: about a hundredth of the figure (a few centimetres on a
    // person), times the size setting
    const double S = 0.012 * figure * std::max(0.05, input.sizeScale);
    // where a part attaches to another (the root of a limb: an armpit, the crotch, the neck)
    std::vector<long> parentOf(m, -1);
    std::map<std::string, size_t> parentCount;
    for (size_t i = 0; i < m; ++i) {
        if (partNeighbors[i].size() > 1)
            continue;
        double bestRadius = -1.0;
        for (size_t j = 0; j < m; ++j) {
            if (nodes[j].group == nodes[i].group)
                continue;
            double reach = (nodes[i].radius + nodes[j].radius) * 1.15;
            if ((nodes[i].position - nodes[j].position).lengthSquared() > reach * reach)
                continue;
            if (nodes[j].radius > bestRadius) {
                bestRadius = nodes[j].radius;
                parentOf[i] = (long)j;
            }
        }
        if (parentOf[i] >= 0)
            parentCount[nodes[parentOf[i]].group]++;
    }
    std::string trunk;
    size_t trunkCount = 0;
    for (const auto& it : parentCount) {
        if (it.second > trunkCount) {
            trunkCount = it.second;
            trunk = it.first;
        }
    }

    // the limbs: chains of nodes attached to another part at one end, long for their
    // thickness (an arm, a leg; not a bust or a lump)
    struct Limb {
        std::vector<size_t> chain; // from where it attaches
        std::vector<double> arc;
        double length = 0.0;
        bool upper = false;
    };
    std::vector<Limb> limbs;
    std::vector<char> limbRoot(m, 0);
    for (size_t i = 0; i < m; ++i) {
        if (parentOf[i] < 0 || partNeighbors[i].size() != 1 || nodes[i].group == trunk)
            continue;
        // an upper limb (an arm) attaches high, a lower limb (a leg) low
        Limb limb;
        std::set<size_t> visited;
        size_t current = i;
        double radiusSum = 0.0;
        while (visited.insert(current).second) {
            limb.chain.push_back(current);
            radiusSum += nodes[current].radius;
            size_t next = (size_t)-1;
            for (size_t u : partNeighbors[current]) {
                if (!visited.count(u))
                    next = u;
            }
            if ((size_t)-1 == next || partNeighbors[current].size() > 2)
                break;
            current = next;
        }
        limb.arc.push_back(0.0);
        for (size_t k = 1; k < limb.chain.size(); ++k)
            limb.arc.push_back(limb.arc.back() + (nodes[limb.chain[k]].position - nodes[limb.chain[k - 1]].position).length());
        limb.length = limb.arc.back();
        double averageRadius = radiusSum / limb.chain.size();
        if (limb.chain.size() < 3 || limb.length < 5.0 * averageRadius)
            continue;
        limb.upper = nodes[i].position.y() > skeletonBottom + 0.55 * skeletonHeight;
        limbRoot[i] = 1;
        limbs.push_back(limb);
    }

    // the outside of the garment, for putting the folds on (not the lining under a hem)
    std::vector<std::array<size_t, 3>> outerTriangles;
    std::vector<size_t> outerTriangleFaces;
    for (size_t f = 0; f < triangles.size(); ++f) {
        const auto& t = triangles[f];
        if (outer[t[0]] && outer[t[1]] && outer[t[2]]) {
            outerTriangles.push_back(t);
            outerTriangleFaces.push_back(f);
        }
    }
    TriangleGrid surfaceGrid(vertices, outerTriangles, averageEdge * 2.5);

    // ---- how far the cloth stands off the body: where it rests on it (the supports) and
    //      where it is free
    const bool hasBody = input.cloth && !input.bodyVertices.empty() && !input.bodyTriangles.empty();
    std::vector<double> slack(n, 0.0);
    // standing off at most this much, the cloth rests on the body
    const double restingSlack = 0.008 * figure;
    // and stands well free at this much
    const double freeSlack = 0.03 * figure;
    if (hasBody) {
        std::vector<std::array<size_t, 3>> bodyTriangles;
        for (const auto& face : input.bodyTriangles) {
            for (size_t i = 1; i + 1 < face.size(); ++i) {
                if (face[0] < input.bodyVertices.size() && face[i] < input.bodyVertices.size() && face[i + 1] < input.bodyVertices.size())
                    bodyTriangles.push_back({ face[0], face[i], face[i + 1] });
            }
        }
        double bodyEdge = 0.0;
        for (const auto& t : bodyTriangles)
            bodyEdge += (input.bodyVertices[t[1]] - input.bodyVertices[t[0]]).length();
        bodyEdge /= std::max((size_t)1, bodyTriangles.size());
        TriangleGrid bodyGrid(input.bodyVertices, bodyTriangles, std::max(bodyEdge * 2.0, 0.01 * figure));
        const double farAway = 0.12 * figure;
        std::vector<double> gap(n, farAway);
        for (size_t v = 0; v < n; ++v) {
            Vector3 q;
            if (bodyGrid.closest(vertices[v], farAway, &q, nullptr, nullptr))
                gap[v] = (q - vertices[v]).length();
        }
        // the closest the garment comes is how it sits on the body (its offset and thickness)
        std::vector<double> outerGaps;
        for (size_t v = 0; v < n; ++v) {
            if (outer[v])
                outerGaps.push_back(gap[v]);
        }
        double fit = 0.0;
        if (!outerGaps.empty()) {
            size_t index = outerGaps.size() / 5;
            std::nth_element(outerGaps.begin(), outerGaps.begin() + index, outerGaps.end());
            fit = outerGaps[index];
        }
        for (size_t v = 0; v < n; ++v)
            slack[v] = std::max(0.0, gap[v] - fit);
        for (int pass = 0; pass < 2; ++pass) {
            std::vector<double> smoothed(n);
            for (size_t v = 0; v < n; ++v) {
                double sum = slack[v] * 2.0, weight = 2.0;
                for (size_t u : neighbors[v]) {
                    if (outer[u] == outer[v]) {
                        sum += slack[u];
                        weight += 1.0;
                    }
                }
                smoothed[v] = sum / weight;
            }
            slack.swap(smoothed);
        }
    }

    auto surfaceVertexNear = [&](const Vector3& target, double maxDistance, bool outerOnly) -> long {
        long best = -1;
        double bestDistance = maxDistance * maxDistance;
        for (size_t v = 0; v < n; ++v) {
            if (outerOnly && !outer[v])
                continue;
            double d = (vertices[v] - target).lengthSquared();
            if (d < bestDistance) {
                bestDistance = d;
                best = (long)v;
            }
        }
        return best;
    };
    // a point onto the surface (with its normal)
    auto project = [&](const Vector3& point, Vector3* onSurface, Vector3* normal, size_t* triangle, double weights[3]) -> bool {
        Vector3 q;
        size_t t = 0;
        double w[3];
        if (!surfaceGrid.closest(point, std::max(S, averageEdge * 3.0), &q, &t, w))
            return false;
        if (onSurface)
            *onSurface = q;
        t = outerTriangleFaces[t];
        if (normal) {
            Vector3 interpolated = normals[triangles[t][0]] * w[0] + normals[triangles[t][1]] * w[1] + normals[triangles[t][2]] * w[2];
            *normal = interpolated.lengthSquared() > 1e-24 ? interpolated.normalized() : faceNormals[t];
        }
        if (triangle)
            *triangle = t;
        if (weights)
            weights[0] = w[0], weights[1] = w[1], weights[2] = w[2];
        return true;
    };
    // a fold along a line on the surface: from `start`, `length` along `direction` (turning
    // with the surface)
    auto addLineFold = [&](Kind kind, const Vector3& start, const Vector3& direction, double length,
                           double width, double depth, double crease, double peak, double ridgeSide) {
        Fold fold;
        fold.kind = kind;
        fold.width = width;
        fold.depth = depth;
        fold.crease = crease;
        fold.peak = peak;
        fold.ridgeSide = ridgeSide;
        Vector3 point, normal;
        if (!project(start, &point, &normal, nullptr, nullptr))
            return;
        Vector3 heading = tangential(direction, normal);
        if (heading.lengthSquared() < 1e-24)
            return;
        heading.normalize();
        int steps = std::max(2, (int)std::ceil(length / std::max(averageEdge * 0.8, width * 0.5)));
        double step = length / steps;
        fold.points.push_back(point);
        fold.normals.push_back(normal);
        for (int i = 0; i < steps; ++i) {
            Vector3 next, nextNormal;
            if (!project(point + heading * step, &next, &nextNormal, nullptr, nullptr))
                break;
            if ((next - point).length() < step * 0.3)
                break;
            heading = tangential(heading, nextNormal);
            if (heading.lengthSquared() < 1e-24)
                break;
            heading.normalize();
            point = next;
            fold.points.push_back(point);
            fold.normals.push_back(nextNormal);
        }
        if (fold.points.size() >= 2)
            folds.push_back(fold);
    };
    // a straight crease centred on `center` (a crease bunched up by compression)
    auto addCrease = [&](const Vector3& center, const Vector3& normal, const Vector3& along, double length,
                         double width, double depth, double ridgeSide) {
        Vector3 a = tangential(along, normal);
        if (a.lengthSquared() < 1e-24)
            return;
        a.normalize();
        Vector3 forward = center + a * (length * 0.5);
        Vector3 backward = center - a * (length * 0.5);
        Vector3 start;
        if (!project(backward, &start, nullptr, nullptr, nullptr))
            return;
        addLineFold(Kind::Crease, start, forward - start, length, width, depth, 1.0, 0.5 + 0.12 * vary(), ridgeSide);
    };
    const double depthScale = input.cloth ? 1.0 : 0.55;

    // ---- compression: creases ringing a limb, stacked along it (the inside of an elbow,
    //      the back of a knee, a sleeve bunching above its cuff)
    auto addAccordion = [&](size_t vertex, const Vector3& axisDirection, double length, double width, double depth, int count) {
        const Vector3& normal = normals[vertex];
        Vector3 axis = tangential(axisDirection, normal);
        if (axis.lengthSquared() < 1e-24)
            return;
        axis.normalize();
        Vector3 side = Vector3::crossProduct(normal, axis);
        double spacing = width * 2.2;
        for (int i = 0; i < count; ++i) {
            double offset = ((double)i - (count - 1) * 0.5) * spacing * (1.0 + 0.08 * vary());
            // tilted a little to alternate sides, like a bunched sleeve (not so much they cross)
            double angle = (i % 2 ? 1.0 : -1.0) * (0.1 + 0.04 * vary());
            Vector3 along = rotated(side, normal, angle);
            double creaseLength = length * (1.0 - 0.15 * std::abs((double)i - (count - 1) * 0.5)) * (1.0 + 0.1 * vary());
            Vector3 center = vertices[vertex] + axis * offset + side * (0.08 * length * vary());
            addCrease(center, normal, along, creaseLength, width * (0.9 + 0.1 * vary()), depth * (0.85 + 0.15 * vary()), (i % 2) ? 1.0 : -1.0);
        }
    };
    // the average slack of some vertices (0 without a body)
    auto averageSlack = [&](const std::vector<size_t>& list) {
        if (list.empty())
            return 0.0;
        double sum = 0.0;
        for (size_t v : list)
            sum += slack[v];
        return sum / list.size();
    };

    for (const auto& limb : limbs) {
        // the joint (an elbow, a knee): where the limb bends most in its middle, or its
        // middle if it is straight
        const size_t count = limb.chain.size();
        if (count < 3)
            continue;
        size_t jointIndex = 0;
        {
            double bestTurn = 0.0, bestMiddle = 1e30;
            size_t bendIndex = 0, middleIndex = 0;
            for (size_t k = 1; k + 1 < count; ++k) {
                double u = limb.arc[k] / limb.length;
                if (std::abs(u - 0.5) < bestMiddle) {
                    bestMiddle = std::abs(u - 0.5);
                    middleIndex = k;
                }
                if (u < 0.3 || u > 0.7)
                    continue;
                Vector3 before = nodes[limb.chain[k]].position - nodes[limb.chain[k - 1]].position;
                Vector3 after = nodes[limb.chain[k + 1]].position - nodes[limb.chain[k]].position;
                if (before.lengthSquared() < 1e-18 || after.lengthSquared() < 1e-18)
                    continue;
                double turn = std::acos(std::max(-1.0, std::min(1.0, Vector3::dotProduct(before.normalized(), after.normalized()))));
                if (turn > bestTurn) {
                    bestTurn = turn;
                    bendIndex = k;
                }
            }
            jointIndex = bestTurn > 10.0 * kPi / 180.0 ? bendIndex : middleIndex;
        }
        if (0 == jointIndex)
            continue;
        // the limb on either side of it, a little way off
        auto chainPointAt = [&](double s) {
            for (size_t k = 0; k + 1 < count; ++k) {
                if (s <= limb.arc[k + 1] || k + 2 == count) {
                    double span = std::max(1e-12, limb.arc[k + 1] - limb.arc[k]);
                    double t = std::max(0.0, std::min(1.0, (s - limb.arc[k]) / span));
                    return nodes[limb.chain[k]].position * (1.0 - t) + nodes[limb.chain[k + 1]].position * t;
                }
            }
            return nodes[limb.chain.back()].position;
        };
        const size_t j = limb.chain[jointIndex];
        const Vector3& joint = nodes[j].position;
        const Vector3 prev = chainPointAt(limb.arc[jointIndex] - 0.2 * limb.length);
        const Vector3 next = chainPointAt(limb.arc[jointIndex] + 0.2 * limb.length);
        Vector3 axis = next - prev;
        if (axis.lengthSquared() < 1e-18)
            continue;
        axis.normalize();
        double radius = std::max(nodes[j].radius, S * 0.5);
        Vector3 bend = (prev + next) * 0.5 - joint;
        bend = bend - axis * Vector3::dotProduct(bend, axis);
        Vector3 inner;
        double bent = 0.0;
        if (bend.length() > radius * 0.08) {
            inner = bend.normalized();
            bent = std::min(1.0, bend.length() / radius);
        } else {
            bool upper = limb.upper;
            // in the rest pose an elbow folds to the front, a knee to the back
            inner = Vector3(0.0, 0.0, upper ? 1.0 : -1.0);
            inner = inner - axis * Vector3::dotProduct(inner, axis);
            if (inner.lengthSquared() < 1e-12)
                inner = Vector3::crossProduct(axis, Vector3(1.0, 0.0, 0.0));
            inner.normalize();
        }
        // the surface ringing the joint
        std::vector<size_t> ring;
        for (size_t v = 0; v < n; ++v) {
            if (!outer[v])
                continue;
            Vector3 d = vertices[v] - joint;
            double alongAxis = Vector3::dotProduct(d, axis);
            if (std::abs(alongAxis) > std::max(radius * 0.7, S))
                continue;
            Vector3 radial = d - axis * alongAxis;
            double distance = radial.length();
            if (distance < radius * 0.5 || distance > radius * 1.35 + S * 3.0)
                continue;
            if (Vector3::dotProduct(normals[v], radial / std::max(1e-12, distance)) < 0.5)
                continue;
            ring.push_back(v);
        }
        // only where the cloth wraps the limb snugly all round (a sleeve, a trouser leg); a
        // skirt hangs past the knees and does not crease there
        {
            Vector3 e1 = Vector3::crossProduct(axis, std::abs(axis.y()) < 0.9 ? Vector3(0.0, 1.0, 0.0) : Vector3(1.0, 0.0, 0.0)).normalized();
            Vector3 e2 = Vector3::crossProduct(axis, e1);
            int sectors = 0;
            bool covered[8] = { false };
            for (size_t v : ring) {
                Vector3 d = vertices[v] - joint;
                double angle = std::atan2(Vector3::dotProduct(d, e2), Vector3::dotProduct(d, e1));
                int sector = std::min(7, std::max(0, (int)std::floor((angle + kPi) / (2.0 * kPi) * 8.0)));
                if (!covered[sector]) {
                    covered[sector] = true;
                    ++sectors;
                }
            }
            if (sectors < 7)
                continue;
        }
        if (hasBody && averageSlack(ring) > restingSlack * 2.0)
            continue;
        // and goes on past the joint both ways (not a skirt or a sleeve ending at it)
        {
            bool above = false, below = false;
            double past = radius * 0.6 + S * 2.0;
            for (size_t v = 0; v < n && !(above && below); ++v) {
                if (!outer[v])
                    continue;
                Vector3 d = vertices[v] - joint;
                double alongAxis = Vector3::dotProduct(d, axis);
                if ((d - axis * alongAxis).length() > radius * 1.35 + S * 3.0)
                    continue;
                above = above || alongAxis > past;
                below = below || alongAxis < -past;
            }
            if (!above || !below)
                continue;
        }
        auto ringVertexToward = [&](const Vector3& direction) -> long {
            long best = -1;
            double bestDot = -2.0;
            for (size_t v : ring) {
                Vector3 radial = vertices[v] - joint;
                radial = radial - axis * Vector3::dotProduct(radial, axis);
                double d = Vector3::dotProduct(radial.normalized(), direction);
                if (d > bestDot) {
                    bestDot = d;
                    best = (long)v;
                }
            }
            return bestDot > 0.5 ? best : -1;
        };
        double circumference = 2.0 * kPi * (radius + S);
        double length = std::max(1.5 * S, std::min(5.0 * S, circumference * 0.28));
        long main = ringVertexToward(inner);
        if (main >= 0)
            addAccordion((size_t)main, axis, length, 0.7 * S, 0.26 * S * (1.0 + 0.5 * bent) * depthScale, 4);
        for (double angle : { 1.4, -1.4 }) {
            long sideVertex = ringVertexToward(rotated(inner, axis, angle));
            if (sideVertex >= 0)
                addAccordion((size_t)sideVertex, axis, length * 0.8, 0.6 * S, 0.16 * S * (1.0 + 0.4 * bent) * depthScale, 3);
        }
    }

    // ---- compression where a limb attaches (armpit, crotch): creases fanning out from the
    //      pinch, where the garment wraps the limb snugly
    if (input.cloth) {
        for (size_t i = 0; i < m; ++i) {
            if (!limbRoot[i])
                continue;
            const Vector3& root = nodes[i].position;
            const Vector3& parent = nodes[parentOf[i]].position;
            Vector3 outward = (nodes[partNeighbors[i][0]].position - root).normalized();
            // only where the garment covers the limb (a tunic does not fold at the crotch)
            std::vector<size_t> covering;
            {
                const SkeletonNode& next = nodes[partNeighbors[i][0]];
                Vector3 middle = (root + next.position) * 0.5;
                double reach = (nodes[i].radius + next.radius) * 0.5 + 3.0 * S;
                for (size_t v = 0; v < n; ++v) {
                    if (outer[v] && (vertices[v] - middle).lengthSquared() < reach * reach)
                        covering.push_back(v);
                }
                if (covering.empty())
                    continue;
            }
            // a loose garment does not pinch there (it hangs instead, see below)
            if (hasBody && averageSlack(covering) > restingSlack * 2.0)
                continue;
            double reach = nodes[i].radius + S * 2.5 + (root - parent).length() * 0.6;
            std::vector<size_t> candidates;
            for (size_t v = 0; v < n; ++v) {
                if (!outer[v] || concavity[v] < 0.35)
                    continue;
                if ((vertices[v] - root).lengthSquared() > reach * reach)
                    continue;
                candidates.push_back(v);
            }
            std::sort(candidates.begin(), candidates.end(), [&](size_t a, size_t b) { return concavity[a] > concavity[b]; });
            std::vector<size_t> centers;
            for (size_t v : candidates) {
                bool far = true;
                for (size_t c : centers) {
                    if ((vertices[v] - vertices[c]).lengthSquared() < (3.0 * S) * (3.0 * S))
                        far = false;
                }
                if (far)
                    centers.push_back(v);
                if (centers.size() >= 2)
                    break;
            }
            for (size_t c : centers) {
                if (hasBody && slack[c] > restingSlack)
                    continue;
                // creases radiating from the pinch: onto the body the limb hangs from, and
                // down the limb
                const Vector3& normal = normals[c];
                for (int group = 0; group < 2; ++group) {
                    Vector3 direction = 0 == group ? parent - root + Vector3(0.0, -(root - parent).length() * 0.5, 0.0) : outward;
                    direction = tangential(direction, normal);
                    if (direction.lengthSquared() < 1e-24)
                        continue;
                    direction.normalize();
                    // as long as the limb is thick there
                    double length = std::max(2.5 * S, (0 == group ? 2.4 : 1.8) * nodes[i].radius);
                    int count = 4;
                    for (int k = 0; k < count; ++k) {
                        double t = (double)k / (count - 1) * 2.0 - 1.0;
                        Vector3 along = rotated(direction, normal, t * 0.6 + 0.06 * vary());
                        double foldLength = length * (1.0 - 0.25 * std::abs(t)) * (1.0 + 0.1 * vary());
                        addLineFold(Kind::Crease, vertices[c] + along * (0.3 * S), along, foldLength,
                            (0 == group ? 0.8 : 0.7) * S, (0 == group ? 0.26 : 0.2) * S, 1.0, 0.3, (k % 2) ? 1.0 : -1.0);
                    }
                }
            }
        }
    }

    // ---- the rims (openings)
    std::vector<char> onRim(n, 0);
    struct Opening {
        Vector3 center;
        Vector3 interior; // unit: from the rim into the garment
        double radius = 0.0;
        std::vector<size_t> vertices;
    };
    std::vector<Opening> openings;
    if (input.cloth) {
        std::vector<std::pair<size_t, size_t>> rimEdges;
        for (const auto& it : edgeFaces) {
            bool rim = it.second.size() == 1;
            if (it.second.size() == 2)
                rim = Vector3::dotProduct(faceNormals[it.second[0]], faceNormals[it.second[1]]) < std::cos(100.0 * kPi / 180.0);
            if (rim) {
                rimEdges.push_back(it.first);
                onRim[it.first.first] = 1;
                onRim[it.first.second] = 1;
            }
        }
        // loops: connected rim vertices
        std::vector<long> loopOf(n, -1);
        std::vector<std::vector<size_t>> rimNeighbors(n);
        for (const auto& e : rimEdges) {
            rimNeighbors[e.first].push_back(e.second);
            rimNeighbors[e.second].push_back(e.first);
        }
        std::vector<std::vector<size_t>> loops;
        for (size_t v = 0; v < n; ++v) {
            if (!onRim[v] || loopOf[v] >= 0)
                continue;
            std::vector<size_t> stack = { v }, loop;
            loopOf[v] = (long)loops.size();
            while (!stack.empty()) {
                size_t a = stack.back();
                stack.pop_back();
                loop.push_back(a);
                for (size_t b : rimNeighbors[a]) {
                    if (loopOf[b] < 0) {
                        loopOf[b] = (long)loops.size();
                        stack.push_back(b);
                    }
                }
            }
            loops.push_back(loop);
        }
        for (const auto& loop : loops) {
            if (loop.size() < 8)
                continue;
            Opening info;
            info.vertices = loop;
            for (size_t v : loop)
                info.center += vertices[v];
            info.center = info.center / (double)loop.size();
            for (size_t v : loop)
                info.radius += (vertices[v] - info.center).length();
            info.radius /= (double)loop.size();
            if (info.radius < 1.2 * S)
                continue;
            // the direction from the rim into the garment: toward the outside of the garment
            // a few rings in from the rim (not the lining of a hem, which turns back in)
            Vector3 interior;
            {
                std::set<size_t> visited(loop.begin(), loop.end());
                std::vector<size_t> frontier(loop.begin(), loop.end());
                Vector3 sum;
                size_t count = 0;
                for (int ringIndex = 0; ringIndex < 4; ++ringIndex) {
                    std::vector<size_t> next;
                    for (size_t a : frontier) {
                        for (size_t b : neighbors[a]) {
                            if (onRim[b] || !visited.insert(b).second)
                                continue;
                            next.push_back(b);
                            if (!outer[b])
                                continue;
                            sum += vertices[b];
                            ++count;
                        }
                    }
                    frontier.swap(next);
                }
                if (count > 0)
                    interior = sum / (double)count - info.center;
            }
            // a rim with no outside next to it is the lining's edge
            if (interior.lengthSquared() < 1e-24)
                continue;
            // the loop's axis (the direction it spreads least in), turned into the garment:
            // the side its bulk is on (a neckline sits in a dip below the shoulders, so the
            // surface right next to it is no guide)
            {
                double c[3][3] = { { 0 } };
                for (size_t v : loop) {
                    Vector3 d = vertices[v] - info.center;
                    for (int r = 0; r < 3; ++r)
                        for (int k = 0; k < 3; ++k)
                            c[r][k] += d[r] * d[k];
                }
                double trace = c[0][0] + c[1][1] + c[2][2];
                // the largest eigenvector of (trace - C) is the smallest of C
                Vector3 axis = interior.normalized();
                for (int iteration = 0; iteration < 48; ++iteration) {
                    Vector3 next((trace - c[0][0]) * axis[0] - c[0][1] * axis[1] - c[0][2] * axis[2],
                        -c[1][0] * axis[0] + (trace - c[1][1]) * axis[1] - c[1][2] * axis[2],
                        -c[2][0] * axis[0] - c[2][1] * axis[1] + (trace - c[2][2]) * axis[2]);
                    if (next.lengthSquared() < 1e-30)
                        break;
                    axis = next.normalized();
                }
                Vector3 bulk = centroid - info.center;
                double side = Vector3::dotProduct(bulk, axis);
                if (std::abs(side) < 0.25 * bulk.length())
                    side = Vector3::dotProduct(interior, axis);
                interior = axis * (side < 0.0 ? -1.0 : 1.0);
            }
            info.interior = interior.normalized();
            // a hem with thickness has two rims (outside and lining) close together: keep one
            bool duplicate = false;
            for (auto& other : openings) {
                if ((other.center - info.center).length() < 2.0 * S + 0.25 * std::max(info.radius, other.radius)
                    && std::abs(other.radius - info.radius) < 0.35 * std::max(info.radius, other.radius)) {
                    duplicate = true;
                    if (info.radius > other.radius)
                        other = info;
                    break;
                }
            }
            if (!duplicate)
                openings.push_back(info);
        }
        for (const auto& opening : openings) {
            // the part the opening rings and how snug it is on it
            double snugRadius = -1.0;
            std::string ringedGroup;
            {
                double best = 1e30;
                for (const auto& link : input.links) {
                    if (link.first >= m || link.second >= m)
                        continue;
                    Vector3 q = closestOnSegment(opening.center, nodes[link.first].position, nodes[link.second].position);
                    double d = (opening.center - q).length();
                    if (d < best) {
                        best = d;
                        double t = (q - nodes[link.first].position).length() / std::max(1e-12, (nodes[link.second].position - nodes[link.first].position).length());
                        snugRadius = nodes[link.first].radius * (1.0 - t) + nodes[link.second].radius * t;
                        ringedGroup = nodes[link.first].group;
                    }
                }
            }
            // the bones passing through the opening: one limb's is a cuff; the trunk's, or
            // several limbs' (both legs through a hem), is a trunk opening
            std::set<std::string> through, centrally;
            for (const auto& link : input.links) {
                if (link.first >= m || link.second >= m)
                    continue;
                const Vector3& a = nodes[link.first].position;
                const Vector3& b = nodes[link.second].position;
                double da = Vector3::dotProduct(a - opening.center, opening.interior);
                double db = Vector3::dotProduct(b - opening.center, opening.interior);
                if (da * db > 0.0 || std::abs(da - db) < 1e-12)
                    continue;
                Vector3 crossing = a + (b - a) * (da / (da - db));
                double off = (crossing - opening.center).length();
                if (off < opening.radius * 1.2)
                    through.insert(nodes[link.first].group);
                if (off < opening.radius * 0.6)
                    centrally.insert(nodes[link.first].group);
            }
            // a cuff rings one limb, round it
            bool limb = through.size() == 1 && centrally.size() == 1 && *through.begin() != trunk;
            if (through.empty())
                limb = !ringedGroup.empty() && ringedGroup != trunk && opening.radius < snugRadius + 3.0 * S;
            bool snug = snugRadius > 0.0 && opening.radius < snugRadius + 3.0 * S;
            if (hasBody) {
                // the body says how snug it is
                snug = averageSlack(opening.vertices) < restingSlack * 1.5;
            }
            bool vertical = std::abs(opening.interior.y()) > 0.7;
            bool garmentAbove = opening.interior.y() > 0.0;
            bool neckline = !limb && m > 0 && opening.center.y() > skeletonBottom + 0.72 * skeletonHeight && !garmentAbove;
            auto ringPoints = [&](int count, double inset) {
                // points evenly round the loop, moved `inset` into the garment
                std::vector<long> result;
                Vector3 axis = opening.interior;
                Vector3 e1 = Vector3::crossProduct(axis, std::abs(axis.y()) < 0.9 ? Vector3(0.0, 1.0, 0.0) : Vector3(1.0, 0.0, 0.0)).normalized();
                Vector3 e2 = Vector3::crossProduct(axis, e1);
                for (int k = 0; k < count; ++k) {
                    double angle = 2.0 * kPi * (k + 0.15 * vary()) / count;
                    Vector3 target = opening.center + (e1 * std::cos(angle) + e2 * std::sin(angle)) * opening.radius + axis * inset;
                    result.push_back(surfaceVertexNear(target, opening.radius + inset + 4.0 * S, true));
                }
                return result;
            };
            if (neckline)
                continue;
            if (limb && snug) {
                // a cuff: the sleeve or leg bunches up just above it
                int count = std::max(3, std::min(8, (int)std::round(2.0 * kPi * opening.radius / (2.4 * S))));
                double length = std::max(1.5 * S, 2.0 * kPi * opening.radius / count * 1.1);
                for (long v : ringPoints(count, 1.8 * S)) {
                    if (v >= 0)
                        addAccordion((size_t)v, opening.interior, length, 0.65 * S, 0.2 * S, 3);
                }
            } else if (vertical && !garmentAbove && snug) {
                // a tight waistband: short gathers hanging from it
                int count = std::max(4, std::min(16, (int)std::round(2.0 * kPi * opening.radius / (2.4 * S))));
                for (long v : ringPoints(count, 0.0)) {
                    if (v < 0)
                        continue;
                    Vector3 down = opening.interior;
                    for (int k = 0; k < 2; ++k) {
                        Vector3 side = Vector3::crossProduct(normals[(size_t)v], down);
                        if (side.lengthSquared() < 1e-24)
                            continue;
                        side.normalize();
                        Vector3 start = vertices[(size_t)v] + side * ((k ? 0.7 : -0.7) * S) + down * (0.2 * S);
                        addLineFold(Kind::Hang, start, rotated(down, normals[(size_t)v], 0.12 * vary()),
                            3.0 * S * (0.75 + 0.25 * vary()), 0.6 * S, 0.2 * S, 0.8, 0.3, (k ? 1.0 : -1.0));
                    }
                }
            } else if (!hasBody && vertical && garmentAbove && input.drape > 0.1) {
                // (without the body to tell where it hangs from) a loose hem: soft folds
                // hanging to it
                double length = std::max(3.0 * S, std::min(0.6 * garmentHeight, (0.2 + 0.5 * input.drape) * garmentHeight));
                int count = std::max(3, std::min(16, (int)std::round(2.0 * kPi * opening.radius / (2.8 * S))));
                for (long v : ringPoints(count, length)) {
                    if (v < 0)
                        continue;
                    size_t foldCount = folds.size();
                    addLineFold(Kind::Hang, vertices[(size_t)v], opening.interior * -1.0, length * 1.1, 1.3 * S,
                        0.6 * S * (0.6 + 0.4 * input.drape), 0.3, 0.65, (foldCount % 2) ? 1.0 : -1.0);
                    if (folds.size() > foldCount)
                        folds.back().endDepth = 0.6;
                }
            }
        }
    }

    // ---- folds from the supports: tension between them, sagging between them, hanging
    //      from them
    if (hasBody) {
        // where the cloth rests on the body, and how free it is elsewhere (0..1)
        std::vector<char> resting(n, 0), usable(n, 0);
        std::vector<double> freedom(n, 0.0);
        for (size_t v = 0; v < n; ++v) {
            usable[v] = outer[v];
            resting[v] = outer[v] && slack[v] < restingSlack;
            freedom[v] = smoothstep(restingSlack, freeSlack, slack[v]);
        }
        // how far from the nearest support over the cloth, and from the nearest rim
        std::vector<double> fromSupport = distancesFrom(vertices, neighbors, resting, usable);
        // (over the lining too: a hem's edge is where its lining ends)
        std::vector<double> fromRim = distancesFrom(vertices, neighbors, onRim, std::vector<char>(n, 1));
        // the way the cloth is pulled: away from its support (up the slope of the distance
        // from it), leaning down with gravity, the more the looser it hangs
        const Vector3 down(0.0, -1.0, 0.0);
        const double gravity = 0.35 + 0.5 * input.drape;
        std::vector<Vector3> pull(n);
        {
            std::vector<Vector3> gradient(n);
            for (size_t f = 0; f < triangles.size(); ++f) {
                const auto& t = triangles[f];
                if (!usable[t[0]] || !usable[t[1]] || !usable[t[2]])
                    continue;
                double d0 = fromSupport[t[0]], d1 = fromSupport[t[1]], d2 = fromSupport[t[2]];
                if (d0 > 1e29 || d1 > 1e29 || d2 > 1e29)
                    continue;
                Vector3 e1 = vertices[t[1]] - vertices[t[0]], e2 = vertices[t[2]] - vertices[t[0]];
                Vector3 normal = Vector3::crossProduct(e1, e2);
                double area2 = normal.length();
                if (area2 < 1e-18)
                    continue;
                normal = normal / area2;
                // the gradient of the linear function with values d0, d1, d2 at the corners
                Vector3 g = (Vector3::crossProduct(normal, vertices[t[2]] - vertices[t[1]]) * d0
                                + Vector3::crossProduct(normal, vertices[t[0]] - vertices[t[2]]) * d1
                                + Vector3::crossProduct(normal, vertices[t[1]] - vertices[t[0]]) * d2)
                    / area2;
                for (int k = 0; k < 3; ++k)
                    gradient[t[k]] += g * area2;
            }
            for (size_t v = 0; v < n; ++v) {
                Vector3 g = tangential(gradient[v], normals[v]);
                if (g.lengthSquared() > 1e-24)
                    g.normalize();
                Vector3 p = g + tangential(down, normals[v]) * gravity;
                pull[v] = p.lengthSquared() > 1e-24 ? p.normalized() : Vector3();
            }
        }

        const double spacing = 2.5 * S;
        const double step = std::min(0.35 * S, 0.5 * averageEdge);
        const double maxLength = std::max(6.0 * S, 0.5 * figure);
        // the points of the folds traced so far, by cell, to keep folds apart
        std::map<uint64_t, std::vector<std::pair<Vector3, size_t>>> placed;
        const double placedCell = spacing;
        auto placedKey = [&](const Vector3& p) {
            return cellKey((int)std::floor(p.x() / placedCell), (int)std::floor(p.y() / placedCell), (int)std::floor(p.z() / placedCell));
        };
        auto nearestPlaced = [&](const Vector3& p, size_t exclude) {
            double best = 1e30;
            int cx = (int)std::floor(p.x() / placedCell), cy = (int)std::floor(p.y() / placedCell), cz = (int)std::floor(p.z() / placedCell);
            for (int x = cx - 1; x <= cx + 1; ++x)
                for (int y = cy - 1; y <= cy + 1; ++y)
                    for (int z = cz - 1; z <= cz + 1; ++z) {
                        auto it = placed.find(cellKey(x, y, z));
                        if (it == placed.end())
                            continue;
                        for (const auto& item : it->second) {
                            if (item.second != exclude)
                                best = std::min(best, (item.first - p).length());
                        }
                    }
            return best;
        };

        // the seeds: evenly along the edges of the supports, where the cloth leaves them for
        // free cloth
        // (the free stretches of cloth, and how free each is at most: one barely standing
        // off the body does not fold)
        std::vector<long> freeRegion(n, -1);
        std::vector<double> regionFreedom;
        for (size_t v = 0; v < n; ++v) {
            if (!usable[v] || resting[v] || freeRegion[v] >= 0)
                continue;
            long region = (long)regionFreedom.size();
            double most = 0.0;
            std::vector<size_t> queue = { v };
            freeRegion[v] = region;
            while (!queue.empty()) {
                size_t a = queue.back();
                queue.pop_back();
                most = std::max(most, freedom[a]);
                for (size_t b : neighbors[a]) {
                    if (usable[b] && !resting[b] && freeRegion[b] < 0) {
                        freeRegion[b] = region;
                        queue.push_back(b);
                    }
                }
            }
            regionFreedom.push_back(most);
        }
        std::vector<char> leaving(n, 0);
        for (size_t v = 0; v < n; ++v) {
            // (cloth needs room to fold: not right at a hem)
            if (!resting[v] || fromRim[v] < 2.0 * S)
                continue;
            for (size_t u : neighbors[v]) {
                if (freeRegion[u] >= 0 && regionFreedom[freeRegion[u]] >= 0.3)
                    leaving[v] = 1;
            }
        }
        std::vector<size_t> seeds;
        {
            std::vector<char> visited(n, 0);
            for (size_t v = 0; v < n; ++v) {
                if (!leaving[v] || visited[v])
                    continue;
                // one edge of a support
                std::vector<size_t> edge, queue = { v };
                visited[v] = 1;
                while (!queue.empty()) {
                    size_t a = queue.back();
                    queue.pop_back();
                    edge.push_back(a);
                    for (size_t b : neighbors[a]) {
                        if (leaving[b] && !visited[b]) {
                            visited[b] = 1;
                            queue.push_back(b);
                        }
                    }
                }
                // in order along it: from an end, always to the nearest one left
                size_t start = edge[0];
                size_t fewest = (size_t)-1;
                for (size_t a : edge) {
                    size_t count = 0;
                    for (size_t b : neighbors[a])
                        count += leaving[b];
                    if (count < fewest || (count == fewest && a < start)) {
                        fewest = count;
                        start = a;
                    }
                }
                std::vector<size_t> ordered = { start };
                std::vector<char> taken(edge.size(), 0);
                taken[std::find(edge.begin(), edge.end(), start) - edge.begin()] = 1;
                for (size_t k = 1; k < edge.size(); ++k) {
                    size_t bestIndex = 0;
                    double bestDistance = 1e30;
                    for (size_t i = 0; i < edge.size(); ++i) {
                        if (taken[i])
                            continue;
                        double d = (vertices[edge[i]] - vertices[ordered.back()]).lengthSquared();
                        if (d < bestDistance) {
                            bestDistance = d;
                            bestIndex = i;
                        }
                    }
                    taken[bestIndex] = 1;
                    ordered.push_back(edge[bestIndex]);
                }
                // a seed every fold spacing along each unbroken stretch of it
                size_t from = 0;
                while (from < ordered.size()) {
                    size_t to = from + 1;
                    std::vector<double> arc = { 0.0 };
                    while (to < ordered.size()) {
                        double d = (vertices[ordered[to]] - vertices[ordered[to - 1]]).length();
                        if (d > averageEdge * 3.0)
                            break;
                        arc.push_back(arc.back() + d);
                        ++to;
                    }
                    double length = arc.back();
                    if (length >= spacing * 0.5) {
                        int count = std::max(1, (int)std::round(length / spacing));
                        for (int k = 0; k < count; ++k) {
                            double at = (k + 0.5) * length / count;
                            size_t index = std::lower_bound(arc.begin(), arc.end(), at) - arc.begin();
                            seeds.push_back(ordered[from + std::min(index, arc.size() - 1)]);
                        }
                    }
                    from = to;
                }
            }
        }
        // cloth hanging from a support above folds first; cloth pushed up from below the
        // least
        std::stable_sort(seeds.begin(), seeds.end(), [&](size_t a, size_t b) {
            return Vector3::dotProduct(pull[a], down) > Vector3::dotProduct(pull[b], down);
        });

        enum class End {
            Free,
            Ridge,
            Support,
            Rim,
            Crowded
        };
        struct Trace {
            std::vector<Vector3> points;
            std::vector<Vector3> normals;
            End end = End::Free;
            double freedom = 0.0;
            double length = 0.0;
        };
        std::vector<Trace> traces;
        auto interpolate = [&](const std::vector<double>& field, size_t t, const double w[3]) {
            return field[triangles[t][0]] * w[0] + field[triangles[t][1]] * w[1] + field[triangles[t][2]] * w[2];
        };
        for (size_t seed : seeds) {
            const Vector3& origin = vertices[seed];
            if (nearestPlaced(origin, (size_t)-1) < spacing * 0.6)
                continue;
            Trace trace;
            size_t index = traces.size();
            Vector3 point = origin, normal = normals[seed];
            Vector3 heading = pull[seed];
            if (heading.lengthSquared() < 1e-24)
                continue;
            trace.points.push_back(point);
            trace.normals.push_back(normal);
            double farthest = 0.0;
            bool wasFree = false;
            while (true) {
                Vector3 next, nextNormal;
                size_t t;
                double w[3];
                if (!project(point + heading * step, &next, &nextNormal, &t, w)) {
                    trace.end = End::Free;
                    break;
                }
                if (!usable[triangles[t][0]] && !usable[triangles[t][1]] && !usable[triangles[t][2]]) {
                    trace.end = End::Rim;
                    break;
                }
                // over the edge of a hem turning under (the surface turning away)
                if (Vector3::dotProduct(nextNormal, normal) < std::cos(25.0 * kPi / 180.0)
                    || (nextNormal.y() < -0.6 && interpolate(fromRim, t, w) < 3.0 * S)) {
                    trace.end = End::Rim;
                    break;
                }
                double distance = interpolate(fromSupport, t, w);
                double nextSlack = interpolate(slack, t, w);
                double nextFreedom = interpolate(freedom, t, w);
                Vector3 nextPull = pull[triangles[t][0]] * w[0] + pull[triangles[t][1]] * w[1] + pull[triangles[t][2]] * w[2];
                nextPull = tangential(nextPull, nextNormal);
                // past the middle between two supports (the pull turns back)
                if (distance < farthest - step * 0.75 || nextPull.lengthSquared() < 1e-12 || Vector3::dotProduct(nextPull.normalized(), heading) < 0.2) {
                    trace.end = End::Ridge;
                    break;
                }
                point = next;
                normal = nextNormal;
                trace.points.push_back(point);
                trace.normals.push_back(nextNormal);
                trace.length += step;
                trace.freedom = std::max(trace.freedom, nextFreedom);
                farthest = std::max(farthest, distance);
                if (nextSlack > restingSlack * 1.5)
                    wasFree = true;
                // onto another support
                if (wasFree && nextSlack < restingSlack) {
                    trace.end = End::Support;
                    break;
                }
                // into a hem (the cloth turns over there)
                if (interpolate(fromRim, t, w) < std::max(averageEdge * 0.6, 1.2 * S)) {
                    trace.end = End::Rim;
                    break;
                }
                if (trace.length > spacing * 0.5 && nearestPlaced(point, index) < spacing * 0.45) {
                    trace.end = End::Crowded;
                    break;
                }
                if (trace.length >= maxLength) {
                    trace.end = End::Free;
                    break;
                }
                heading = nextPull.normalized();
            }
            if (trace.length < 3.5 * S || trace.freedom < 0.2)
                continue;
            for (const auto& p : trace.points)
                placed[placedKey(p)].push_back({ p, index });
            traces.push_back(trace);
        }

        // two folds meeting from either side, from supports side by side, are one fold
        // sagging between them
        std::vector<long> partner(traces.size(), -1);
        for (size_t i = 0; i < traces.size(); ++i) {
            if (partner[i] >= 0 || (traces[i].end != End::Ridge && traces[i].end != End::Crowded))
                continue;
            long best = -1;
            double bestDistance = spacing;
            for (size_t j = 0; j < traces.size(); ++j) {
                if (j == i || partner[j] >= 0 || (traces[j].end != End::Ridge && traces[j].end != End::Crowded))
                    continue;
                if ((traces[i].points.front() - traces[j].points.front()).length() < spacing * 1.5)
                    continue;
                double d = (traces[i].points.back() - traces[j].points.back()).length();
                if (d < bestDistance) {
                    bestDistance = d;
                    best = (long)j;
                }
            }
            if (best >= 0) {
                partner[i] = best;
                partner[best] = (long)i;
            }
        }
        std::vector<char> used(traces.size(), 0);
        for (size_t i = 0; i < traces.size(); ++i) {
            if (used[i])
                continue;
            used[i] = 1;
            const Trace& trace = traces[i];
            Fold fold;
            fold.points = trace.points;
            fold.normals = trace.normals;
            double freedom = trace.freedom;
            if (partner[i] >= 0) {
                const Trace& other = traces[partner[i]];
                used[partner[i]] = 1;
                for (size_t k = other.points.size(); k-- > 0;) {
                    if ((other.points[k] - fold.points.back()).length() < step * 0.5)
                        continue;
                    fold.points.push_back(other.points[k]);
                    fold.normals.push_back(other.normals[k]);
                }
                freedom = std::max(freedom, other.freedom);
                fold.peak = trace.length / std::max(1e-9, trace.length + other.length);
                Vector3 between = other.points.front() - trace.points.front();
                if (std::abs(between.y()) > std::sqrt(between.x() * between.x() + between.z() * between.z())) {
                    // supports above and below: pulled taut between them (from the bust
                    // down to the waist)
                    fold.kind = Kind::Tension;
                    fold.crease = 0.45;
                } else {
                    // supports side by side: sagging between them
                    fold.kind = Kind::Sag;
                    fold.crease = 0.4;
                }
            } else if (End::Support == trace.end) {
                fold.kind = Kind::Tension;
                fold.peak = 0.45;
                fold.crease = 0.45;
            } else {
                fold.kind = Kind::Hang;
                fold.peak = End::Rim == trace.end ? 0.65 : 0.6;
                fold.endDepth = End::Rim == trace.end ? 0.6 : 0.0;
                fold.crease = 0.25 + 0.35 * (1.0 - input.drape);
            }
            fold.width = S * (0.75 + 0.45 * freedom);
            // deeper the freer the cloth; a hanging fold is a round pipe, deep for its width
            double ratio = Kind::Hang == fold.kind ? 0.55 + 0.25 * input.drape : (Kind::Sag == fold.kind ? 0.6 : 0.5);
            fold.depth = fold.width * ratio * (0.6 + 0.4 * freedom);
            fold.ridgeSide = jitter(input.seed + (uint32_t)i * 2654435761u) < 0.0 ? -1.0 : 1.0;
            // a smooth curve (cloth does not turn corners): evenly spaced points, smoothed,
            // back on the surface
            std::vector<Vector3> even;
            {
                double interval = fold.width * 0.5;
                even.push_back(fold.points.front());
                double carried = 0.0;
                for (size_t k = 1; k < fold.points.size(); ++k) {
                    Vector3 a = fold.points[k - 1], b = fold.points[k];
                    double segment = (b - a).length();
                    double at = interval - carried;
                    while (at <= segment) {
                        even.push_back(a + (b - a) * (at / std::max(1e-12, segment)));
                        at += interval;
                    }
                    carried = segment - (at - interval);
                }
                if ((even.back() - fold.points.back()).length() > interval * 0.3)
                    even.push_back(fold.points.back());
            }
            if (even.size() < 3)
                continue;
            for (int pass = 0; pass < 8; ++pass) {
                std::vector<Vector3> smoothed = even;
                for (size_t k = 1; k + 1 < even.size(); ++k)
                    smoothed[k] = even[k] * 0.5 + (even[k - 1] + even[k + 1]) * 0.25;
                even.swap(smoothed);
            }
            Fold smooth = fold;
            smooth.points.clear();
            smooth.normals.clear();
            for (const auto& p : even) {
                Vector3 onSurface, normal;
                if (!project(p, &onSurface, &normal, nullptr, nullptr))
                    continue;
                smooth.points.push_back(onSurface);
                smooth.normals.push_back(normal);
            }
            // where it still turns sharply, keep the longer side
            for (size_t k = 1; k + 1 < smooth.points.size(); ++k) {
                Vector3 before = smooth.points[k] - smooth.points[k - 1];
                Vector3 after = smooth.points[k + 1] - smooth.points[k];
                if (before.lengthSquared() < 1e-24 || after.lengthSquared() < 1e-24)
                    continue;
                if (Vector3::dotProduct(before.normalized(), after.normalized()) > std::cos(45.0 * kPi / 180.0))
                    continue;
                bool keepFirst = k * 2 >= smooth.points.size();
                if (keepFirst) {
                    smooth.points.resize(k + 1);
                    smooth.normals.resize(k + 1);
                    smooth.endDepth = 0.0;
                    smooth.peak = std::min(smooth.peak, 0.6);
                } else {
                    smooth.points.erase(smooth.points.begin(), smooth.points.begin() + k);
                    smooth.normals.erase(smooth.normals.begin(), smooth.normals.begin() + k);
                    smooth.peak = std::max(smooth.peak, 0.4);
                }
                k = 0;
            }
            double length = 0.0;
            for (size_t k = 1; k < smooth.points.size(); ++k)
                length += (smooth.points[k] - smooth.points[k - 1]).length();
            if (smooth.points.size() >= 2 && length >= (Kind::Hang == smooth.kind ? 4.5 : 3.0) * S)
                folds.push_back(smooth);
        }
    }
    return folds;
}

}
