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
#include <dust3d/uv/surface_paint.h>
#include <limits>
#include <thread>

namespace dust3d {

namespace {

    inline double clamp01(double x)
    {
        return std::max(0.0, std::min(1.0, x));
    }

    inline double smooth01(double x)
    {
        x = clamp01(x);
        return x * x * (3.0 - 2.0 * x);
    }

    inline Color scaled(const Color& color, double factor)
    {
        return Color(color.r() * factor, color.g() * factor, color.b() * factor, color.alpha());
    }

    inline Color mixed(const Color& from, const Color& to, double t)
    {
        return Color(from.r() + (to.r() - from.r()) * t, from.g() + (to.g() - from.g()) * t,
            from.b() + (to.b() - from.b()) * t, from.alpha());
    }

}

SurfacePaint::SurfacePaint(const ObjectPaintStroke& stroke)
    : m_stroke(stroke)
{
    m_lower = Vector3(std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    m_upper = Vector3(std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest());
    const double shadow = 0.03;
    for (const auto& node : m_stroke.nodes) {
        for (size_t axis = 0; axis < 3; ++axis) {
            m_lower[axis] = std::min(m_lower[axis], node.first[axis] - node.second - shadow);
            m_upper[axis] = std::max(m_upper[axis], node.first[axis] + node.second + shadow);
        }
    }
}

bool SurfacePaint::reaches(const Vector3& lower, const Vector3& upper) const
{
    if (m_stroke.nodes.empty())
        return false;
    for (size_t axis = 0; axis < 3; ++axis) {
        if (lower[axis] > m_upper[axis] || upper[axis] < m_lower[axis])
            return false;
    }
    return true;
}

double SurfacePaint::distance(const Vector3& position, double* radius) const
{
    Nearest found = nearest(position);
    if (nullptr != radius)
        *radius = found.radius;
    return found.distance;
}

SurfacePaint::Nearest SurfacePaint::nearest(const Vector3& position) const
{
    Nearest found;
    found.distance = std::numeric_limits<double>::max();
    const auto& nodes = m_stroke.nodes;
    if (1 == nodes.size()) {
        found.distance = (position - nodes[0].first).length() - nodes[0].second;
        found.radius = nodes[0].second;
    }
    for (size_t i = 0; i + 1 < nodes.size(); ++i) {
        // the brush between two nodes: its centre goes straight from one to the other and
        // its size changes evenly on the way
        Vector3 axis = nodes[i + 1].first - nodes[i].first;
        double lengthSquared = axis.lengthSquared();
        double length = std::sqrt(lengthSquared);
        double along = lengthSquared > 1e-24 ? Vector3::dotProduct(position - nodes[i].first, axis) / lengthSquared : 0.0;
        double t = std::max(0.0, std::min(1.0, along));
        double brush = nodes[i].second + (nodes[i + 1].second - nodes[i].second) * t;
        double d = (position - (nodes[i].first + axis * t)).length() - brush;
        if (!m_stroke.rounded && lengthSquared > 1e-24) {
            bool first = 0 == i, last = i + 2 == nodes.size();
            if ((first && along < 0.0) || (last && along > 1.0)) {
                // past a square end: outside by how far past, or by how far out to the side
                double past = (along < 0.0 ? -along : along - 1.0) * length;
                double aside = (position - (nodes[i].first + axis * along)).length() - brush;
                d = std::max(past, aside);
            } else {
                // inside: the square end may be nearer than the side of the brush
                double toEnd = std::numeric_limits<double>::max();
                if (first)
                    toEnd = std::min(toEnd, along * length);
                if (last)
                    toEnd = std::min(toEnd, (1.0 - along) * length);
                if (-toEnd > d)
                    d = -toEnd;
            }
        }
        if (d < found.distance) {
            found.distance = d;
            found.radius = brush;
        }
    }
    return found;
}

Color SurfacePaint::colorAt(const Color& under, const Vector3& position, double footprint) const
{
    Nearest found = nearest(position);
    const double d = found.distance;
    const double radius = found.radius;
    const Color& ink = m_stroke.color;
    if (m_stroke.softness > 0.0) {
        if (d >= footprint)
            return under;
        double fade = std::max(footprint, m_stroke.softness * radius);
        return mixed(under, ink, smooth01(-d / fade) * ink.alpha());
    }

    // (an edge shows as far as the two colours differ)
    const double contrast = clamp01(3.0 * (std::abs(under.r() - ink.r()) + std::abs(under.g() - ink.g()) + std::abs(under.b() - ink.b())) - 0.15);
    // the shadow it drops on what is under it
    const double shadow = contrast > 0.0 ? std::min(0.028, 0.8 * radius) : 0.0;
    if (d >= std::max(footprint, shadow))
        return under;
    Color result = under;
    if (shadow > 0.0 && d > 0.0) {
        double t = 1.0 - d / shadow;
        result = scaled(under, 1.0 - 0.26 * contrast * t * t);
    }
    double cover = clamp01(0.5 - d / std::max(1e-9, footprint));
    if (cover <= 0.0)
        return result;
    const double depth = -d;
    // airbrushed: darker towards the edge, over a good part of the brush
    const double band = std::max(2.0 * footprint, std::min(0.055, 0.6 * radius));
    double shade = 1.0 - 0.20 * contrast * (1.0 - smooth01(depth / band));
    // and a thin soft line just inside the edge
    const double line = std::min(std::max(1.2 * footprint, 0.003), 0.2 * radius);
    if (line > 0.0 && depth < line)
        shade *= 1.0 - 0.22 * contrast * (1.0 - smooth01(depth / line));
    return mixed(result, scaled(ink, shade), cover * ink.alpha());
}

namespace {

    // Moller-Trumbore: the distance along the ray to the triangle, or a negative number.
    inline double rayTriangle(const Vector3& origin, const Vector3& direction,
        const Vector3& a, const Vector3& edge1, const Vector3& edge2)
    {
        Vector3 p = Vector3::crossProduct(direction, edge2);
        double determinant = Vector3::dotProduct(edge1, p);
        if (std::abs(determinant) < 1e-18)
            return -1.0;
        double inverse = 1.0 / determinant;
        Vector3 s = origin - a;
        double u = Vector3::dotProduct(s, p) * inverse;
        if (u < 0.0 || u > 1.0)
            return -1.0;
        Vector3 q = Vector3::crossProduct(s, edge1);
        double v = Vector3::dotProduct(direction, q) * inverse;
        if (v < 0.0 || u + v > 1.0)
            return -1.0;
        return Vector3::dotProduct(edge2, q) * inverse;
    }

}

OcclusionCaster::OcclusionCaster(const std::vector<Vector3>& vertices, const std::vector<std::vector<size_t>>& triangles)
{
    if (vertices.empty())
        return;
    Vector3 lower = vertices[0], upper = vertices[0];
    for (const auto& vertex : vertices) {
        for (size_t axis = 0; axis < 3; ++axis) {
            lower[axis] = std::min(lower[axis], vertex[axis]);
            upper[axis] = std::max(upper[axis], vertex[axis]);
        }
    }
    const double size = (upper - lower).length();
    if (size <= 0.0)
        return;
    // What shuts a point in is what is near it: a ray is stopped only within this reach,
    // and less the further off it is.
    m_reach = 0.16 * size;
    m_lift = 0.003 * size;
    std::vector<Vector3> centres;
    std::vector<std::pair<Vector3, Vector3>> bounds;
    for (const auto& triangle : triangles) {
        if (triangle.size() != 3)
            continue;
        const Vector3& a = vertices[triangle[0]];
        const Vector3& b = vertices[triangle[1]];
        const Vector3& c = vertices[triangle[2]];
        Triangle face;
        face.a = a;
        face.edge1 = b - a;
        face.edge2 = c - a;
        m_triangles.push_back(face);
        Vector3 low, high;
        for (size_t axis = 0; axis < 3; ++axis) {
            low[axis] = std::min({ a[axis], b[axis], c[axis] });
            high[axis] = std::max({ a[axis], b[axis], c[axis] });
        }
        bounds.push_back({ low, high });
        centres.push_back((a + b + c) / 3.0);
    }
    if (m_triangles.empty())
        return;
    std::vector<uint32_t> order(m_triangles.size());
    for (uint32_t i = 0; i < order.size(); ++i)
        order[i] = i;
    m_nodes.reserve(2 * m_triangles.size());
    build(order, centres, bounds, 0, (uint32_t)order.size());
    // the leaves' triangles in the order the tree holds them
    std::vector<Triangle> sorted(m_triangles.size());
    for (size_t i = 0; i < order.size(); ++i)
        sorted[i] = m_triangles[order[i]];
    m_triangles = std::move(sorted);

    const size_t rays = 28;
    m_hemisphere.resize(rays);
    for (size_t i = 0; i < rays; ++i) {
        // cosine weighted, by a golden angle spiral
        double r = std::sqrt(((double)i + 0.5) / (double)rays);
        double angle = (double)i * 2.399963229728653;
        m_hemisphere[i] = Vector3(r * std::cos(angle), r * std::sin(angle), std::sqrt(std::max(0.0, 1.0 - r * r)));
    }
}

uint32_t OcclusionCaster::build(std::vector<uint32_t>& order, std::vector<Vector3>& centres,
    std::vector<std::pair<Vector3, Vector3>>& bounds, uint32_t begin, uint32_t end)
{
    uint32_t index = (uint32_t)m_nodes.size();
    m_nodes.emplace_back();
    Vector3 low = bounds[order[begin]].first, high = bounds[order[begin]].second;
    Vector3 centreLow = centres[order[begin]], centreHigh = centreLow;
    for (uint32_t i = begin; i < end; ++i) {
        for (size_t axis = 0; axis < 3; ++axis) {
            low[axis] = std::min(low[axis], bounds[order[i]].first[axis]);
            high[axis] = std::max(high[axis], bounds[order[i]].second[axis]);
            centreLow[axis] = std::min(centreLow[axis], centres[order[i]][axis]);
            centreHigh[axis] = std::max(centreHigh[axis], centres[order[i]][axis]);
        }
    }
    for (size_t axis = 0; axis < 3; ++axis) {
        m_nodes[index].lower[axis] = (float)low[axis];
        m_nodes[index].upper[axis] = (float)high[axis];
    }
    const uint32_t count = end - begin;
    int axis = 0;
    Vector3 extent = centreHigh - centreLow;
    if (extent.y() > extent[axis])
        axis = 1;
    if (extent.z() > extent[axis])
        axis = 2;
    if (count <= 4 || extent[axis] <= 0.0) {
        m_nodes[index].first = begin;
        m_nodes[index].count = count;
        return index;
    }
    // split at the median along the longest side of the triangles' centres
    uint32_t middle = begin + count / 2;
    std::nth_element(order.begin() + begin, order.begin() + middle, order.begin() + end,
        [&](uint32_t a, uint32_t b) { return centres[a][axis] < centres[b][axis]; });
    build(order, centres, bounds, begin, middle);
    uint32_t second = build(order, centres, bounds, middle, end);
    m_nodes[index].first = second;
    m_nodes[index].count = 0;
    return index;
}

double OcclusionCaster::nearestHit(const Vector3& origin, const Vector3& direction, double reach) const
{
    double nearest = reach;
    double inverse[3];
    for (size_t axis = 0; axis < 3; ++axis)
        inverse[axis] = std::abs(direction[axis]) > 1e-30 ? 1.0 / direction[axis] : std::copysign(1e30, direction[axis]);
    uint32_t stack[64];
    int top = 0;
    stack[top++] = 0;
    while (top > 0) {
        const Node& node = m_nodes[stack[--top]];
        // (the box against the ray, within what has been found so far)
        double enter = 0.0, leave = nearest;
        bool missed = false;
        for (size_t axis = 0; axis < 3; ++axis) {
            double t0 = (node.lower[axis] - origin[axis]) * inverse[axis];
            double t1 = (node.upper[axis] - origin[axis]) * inverse[axis];
            if (t0 > t1)
                std::swap(t0, t1);
            enter = std::max(enter, t0);
            leave = std::min(leave, t1);
            if (enter > leave) {
                missed = true;
                break;
            }
        }
        if (missed)
            continue;
        if (node.count > 0) {
            for (uint32_t i = node.first; i < node.first + node.count; ++i) {
                const auto& face = m_triangles[i];
                double t = rayTriangle(origin, direction, face.a, face.edge1, face.edge2);
                if (t > 0.0 && t < nearest)
                    nearest = t;
            }
            continue;
        }
        if (top + 2 > 64)
            continue;
        uint32_t self = (uint32_t)(&node - m_nodes.data());
        stack[top++] = node.first;
        stack[top++] = self + 1;
    }
    return nearest;
}

double OcclusionCaster::occlusionAt(const Vector3& position, const Vector3& normal) const
{
    if (m_nodes.empty() || normal.lengthSquared() < 1e-30)
        return 1.0;
    Vector3 origin = position + normal * m_lift;
    Vector3 tangent = Vector3::crossProduct(std::abs(normal.y()) < 0.9 ? Vector3(0.0, 1.0, 0.0) : Vector3(1.0, 0.0, 0.0), normal).normalized();
    Vector3 bitangent = Vector3::crossProduct(normal, tangent);
    double shut = 0.0;
    for (const auto& sample : m_hemisphere) {
        Vector3 direction = tangent * sample.x() + bitangent * sample.y() + normal * sample.z();
        shut += 1.0 - nearestHit(origin, direction, m_reach) / m_reach;
    }
    return 1.0 - shut / (double)m_hemisphere.size();
}

std::vector<double> computeVertexOcclusion(const std::vector<Vector3>& vertices,
    const std::vector<std::vector<size_t>>& triangles)
{
    std::vector<double> occlusion(vertices.size(), 1.0);
    if (vertices.empty() || triangles.empty())
        return occlusion;
    OcclusionCaster caster(vertices, triangles);
    std::vector<Vector3> normals(vertices.size());
    for (const auto& triangle : triangles) {
        if (triangle.size() != 3)
            continue;
        Vector3 weighted = Vector3::crossProduct(vertices[triangle[1]] - vertices[triangle[0]], vertices[triangle[2]] - vertices[triangle[0]]);
        for (size_t i = 0; i < 3; ++i)
            normals[triangle[i]] += weighted;
    }
    auto work = [&](size_t begin, size_t end) {
        for (size_t v = begin; v < end; ++v) {
            if (normals[v].lengthSquared() > 1e-30)
                occlusion[v] = caster.occlusionAt(vertices[v], normals[v].normalized());
        }
    };
    size_t threadCount = std::max((size_t)1, std::min((size_t)16, (size_t)std::thread::hardware_concurrency()));
    threadCount = std::max((size_t)1, std::min(threadCount, vertices.size() / 64));
    std::vector<std::thread> workers;
    size_t span = (vertices.size() + threadCount - 1) / threadCount;
    for (size_t t = 0; t < threadCount; ++t) {
        size_t begin = t * span, end = std::min(vertices.size(), begin + span);
        if (begin >= end)
            break;
        workers.emplace_back(work, begin, end);
    }
    for (auto& worker : workers)
        worker.join();
    return occlusion;
}

Color bakedShade(const Color& color, double occlusion, const Vector3& normal, double amount)
{
    amount = clamp01(amount);
    if (amount <= 0.0)
        return color;
    // shut in: darker and a little cooler
    double open = std::pow(clamp01(occlusion), 0.8);
    double r = 0.50 + 0.50 * open, g = 0.47 + 0.53 * open, b = 0.55 + 0.45 * open;
    // lit from above: warm where it faces the light, cool underneath; the sides in between
    double facing = 0.5 + 0.5 * std::max(-1.0, std::min(1.0, normal.y()));
    facing = facing * facing * (3.0 - 2.0 * facing);
    r *= 0.70 + (1.16 - 0.70) * facing;
    g *= 0.69 + (1.13 - 0.69) * facing;
    b *= 0.78 + (1.05 - 0.78) * facing;
    return Color(color.r() * (1.0 + (r - 1.0) * amount), color.g() * (1.0 + (g - 1.0) * amount),
        color.b() * (1.0 + (b - 1.0) * amount), color.alpha());
}

}
