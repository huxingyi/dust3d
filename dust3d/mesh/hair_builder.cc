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
#include <dust3d/base/math.h>
#include <dust3d/mesh/hair_builder.h>
#include <limits>

namespace dust3d {

namespace {

    Vector3 closestOnTriangle(const Vector3& p, const Vector3& a, const Vector3& b, const Vector3& c)
    {
        // (Ericson, Real-Time Collision Detection, 5.1.5)
        Vector3 ab = b - a, ac = c - a, ap = p - a;
        double d1 = Vector3::dotProduct(ab, ap), d2 = Vector3::dotProduct(ac, ap);
        if (d1 <= 0.0 && d2 <= 0.0)
            return a;
        Vector3 bp = p - b;
        double d3 = Vector3::dotProduct(ab, bp), d4 = Vector3::dotProduct(ac, bp);
        if (d3 >= 0.0 && d4 <= d3)
            return b;
        double vc = d1 * d4 - d3 * d2;
        if (vc <= 0.0 && d1 >= 0.0 && d3 <= 0.0)
            return a + ab * (d1 / (d1 - d3));
        Vector3 cp = p - c;
        double d5 = Vector3::dotProduct(ab, cp), d6 = Vector3::dotProduct(ac, cp);
        if (d6 >= 0.0 && d5 <= d6)
            return c;
        double vb = d5 * d2 - d1 * d6;
        if (vb <= 0.0 && d2 >= 0.0 && d6 <= 0.0)
            return a + ac * (d2 / (d2 - d6));
        double va = d3 * d6 - d5 * d4;
        if (va <= 0.0 && (d4 - d3) >= 0.0 && (d5 - d6) >= 0.0)
            return b + (c - b) * ((d4 - d3) / ((d4 - d3) + (d5 - d6)));
        double denominator = 1.0 / (va + vb + vc);
        return a + ab * (vb * denominator) + ac * (vc * denominator);
    }

    class Random {
    public:
        explicit Random(uint32_t seed)
            : m_state(seed ? seed : 0x9e3779b9u)
        {
        }
        // in [0, 1)
        double next()
        {
            m_state ^= m_state << 13;
            m_state ^= m_state >> 17;
            m_state ^= m_state << 5;
            return (double)(m_state & 0xffffff) / (double)0x1000000;
        }

    private:
        uint32_t m_state;
    };

    // the cross-section of a strand: a lens, wide across and thin through
    const double kLens[8][2] = { { 1.0, 0.0 }, { 0.62, 0.75 }, { 0.0, 1.0 }, { -0.62, 0.75 },
        { -1.0, 0.0 }, { -0.62, -0.75 }, { 0.0, -1.0 }, { 0.62, -0.75 } };

}

// The scalp, for the nearest point on it: a bounding volume hierarchy over its triangles.
struct HairBuilder::Scalp {
    struct Triangle {
        Vector3 a, b, c, normal;
    };
    struct Node {
        Vector3 lower, upper;
        uint32_t first = 0, count = 0;
    };
    std::vector<Triangle> triangles;
    std::vector<Node> nodes;

    uint32_t build(std::vector<uint32_t>& order, const std::vector<Vector3>& centres, uint32_t begin, uint32_t end)
    {
        uint32_t index = (uint32_t)nodes.size();
        nodes.emplace_back();
        Vector3 lower = triangles[order[begin]].a, upper = lower;
        Vector3 centreLower = centres[order[begin]], centreUpper = centreLower;
        for (uint32_t i = begin; i < end; ++i) {
            const auto& t = triangles[order[i]];
            for (const auto* v : { &t.a, &t.b, &t.c }) {
                for (size_t axis = 0; axis < 3; ++axis) {
                    lower[axis] = std::min(lower[axis], (*v)[axis]);
                    upper[axis] = std::max(upper[axis], (*v)[axis]);
                }
            }
            for (size_t axis = 0; axis < 3; ++axis) {
                centreLower[axis] = std::min(centreLower[axis], centres[order[i]][axis]);
                centreUpper[axis] = std::max(centreUpper[axis], centres[order[i]][axis]);
            }
        }
        nodes[index].lower = lower;
        nodes[index].upper = upper;
        Vector3 extent = centreUpper - centreLower;
        int axis = 0;
        if (extent.y() > extent[axis])
            axis = 1;
        if (extent.z() > extent[axis])
            axis = 2;
        uint32_t count = end - begin;
        if (count <= 4 || extent[axis] <= 0.0) {
            nodes[index].first = begin;
            nodes[index].count = count;
            return index;
        }
        uint32_t middle = begin + count / 2;
        std::nth_element(order.begin() + begin, order.begin() + middle, order.begin() + end,
            [&](uint32_t x, uint32_t y) { return centres[x][axis] < centres[y][axis]; });
        build(order, centres, begin, middle);
        nodes[index].first = build(order, centres, middle, end);
        return index;
    }

    static double boxDistanceSquared(const Node& node, const Vector3& p)
    {
        double d = 0.0;
        for (size_t axis = 0; axis < 3; ++axis) {
            double v = std::max({ node.lower[axis] - p[axis], 0.0, p[axis] - node.upper[axis] });
            d += v * v;
        }
        return d;
    }

    // the nearest point on the scalp and the normal of the triangle it is on
    bool nearest(const Vector3& p, Vector3* point, Vector3* normal) const
    {
        if (nodes.empty())
            return false;
        double best = std::numeric_limits<double>::max();
        uint32_t stack[64];
        int top = 0;
        stack[top++] = 0;
        while (top > 0) {
            uint32_t index = stack[--top];
            const Node& node = nodes[index];
            if (boxDistanceSquared(node, p) >= best)
                continue;
            if (node.count > 0) {
                for (uint32_t i = node.first; i < node.first + node.count; ++i) {
                    const auto& t = triangles[i];
                    Vector3 q = closestOnTriangle(p, t.a, t.b, t.c);
                    double d = (q - p).lengthSquared();
                    if (d < best) {
                        best = d;
                        *point = q;
                        *normal = t.normal;
                    }
                }
                continue;
            }
            if (top + 2 > 64)
                continue;
            // the nearer child last, so it is searched first
            uint32_t first = index + 1, second = node.first;
            if (boxDistanceSquared(nodes[first], p) < boxDistanceSquared(nodes[second], p))
                std::swap(first, second);
            stack[top++] = first;
            stack[top++] = second;
        }
        return best < std::numeric_limits<double>::max();
    }
};

HairBuilder::HairBuilder() = default;
HairBuilder::~HairBuilder() = default;

void HairBuilder::setScalp(const std::vector<Vector3>& vertices, const std::vector<std::vector<size_t>>& triangles)
{
    m_scalp = std::make_unique<Scalp>();
    std::vector<Vector3> centres;
    for (const auto& face : triangles) {
        for (size_t i = 1; i + 1 < face.size(); ++i) {
            Scalp::Triangle t;
            t.a = vertices[face[0]];
            t.b = vertices[face[i]];
            t.c = vertices[face[i + 1]];
            t.normal = Vector3::crossProduct(t.b - t.a, t.c - t.a);
            if (t.normal.lengthSquared() < 1e-30)
                continue;
            t.normal.normalize();
            m_scalp->triangles.push_back(t);
            centres.push_back((t.a + t.b + t.c) / 3.0);
        }
    }
    if (m_scalp->triangles.empty()) {
        m_scalp.reset();
        return;
    }
    std::vector<uint32_t> order(m_scalp->triangles.size());
    for (uint32_t i = 0; i < order.size(); ++i)
        order[i] = i;
    m_scalp->nodes.reserve(2 * order.size());
    m_scalp->build(order, centres, 0, (uint32_t)order.size());
    std::vector<Scalp::Triangle> sorted(order.size());
    for (size_t i = 0; i < order.size(); ++i)
        sorted[i] = m_scalp->triangles[order[i]];
    m_scalp->triangles = std::move(sorted);
}

void HairBuilder::build(const std::vector<MeshNode>& guide, const Parameters& parameters)
{
    m_vertices.clear();
    m_faces.clear();
    m_faceUvs.clear();
    m_vertexSources.clear();
    if (guide.size() < 2)
        return;
    const size_t strandCount = std::max((size_t)1, std::min((size_t)16, parameters.strands));
    const double thickness = std::max(0.1, std::min(2.0, parameters.thickness));
    const double tip = std::max(0.0, std::min(1.0, parameters.tip));

    // The guide, smooth: a Catmull-Rom curve through the nodes, sampled evenly.
    std::vector<double> nodeDistances(guide.size(), 0.0);
    for (size_t i = 1; i < guide.size(); ++i)
        nodeDistances[i] = nodeDistances[i - 1] + (guide[i].origin - guide[i - 1].origin).length();
    const double length = nodeDistances.back();
    if (length <= 1e-9)
        return;
    double meanRadius = 0.0;
    for (const auto& node : guide)
        meanRadius += node.radius;
    meanRadius /= guide.size();
    const size_t samples = std::max((size_t)8, std::min((size_t)32, (size_t)std::ceil(length / std::max(1e-6, meanRadius * 0.5))));
    auto curve = [&](double s, double* radius, size_t* nodeIndex) {
        // s in [0, length]
        size_t i = 0;
        while (i + 2 < guide.size() && nodeDistances[i + 1] < s)
            ++i;
        double span = std::max(1e-12, nodeDistances[i + 1] - nodeDistances[i]);
        double t = std::max(0.0, std::min(1.0, (s - nodeDistances[i]) / span));
        const Vector3& p1 = guide[i].origin;
        const Vector3& p2 = guide[i + 1].origin;
        Vector3 p0 = i > 0 ? guide[i - 1].origin : p1 * 2.0 - p2;
        Vector3 p3 = i + 2 < guide.size() ? guide[i + 2].origin : p2 * 2.0 - p1;
        double t2 = t * t, t3 = t2 * t;
        *radius = guide[i].radius * (1.0 - t) + guide[i + 1].radius * t;
        *nodeIndex = t < 0.5 ? i : i + 1;
        return (p1 * 2.0 + (p2 - p0) * t + (p0 * 2.0 - p1 * 5.0 + p2 * 4.0 - p3) * t2 + (p1 * 3.0 - p0 - p2 * 3.0 + p3) * t3) * 0.5;
    };
    std::vector<Vector3> centre(samples);
    std::vector<double> radius(samples);
    std::vector<size_t> source(samples);
    for (size_t i = 0; i < samples; ++i)
        centre[i] = curve(length * i / (samples - 1), &radius[i], &source[i]);

    // Its frame: along it, out from the scalp under it, and across.
    std::vector<Vector3> along(samples), out(samples), across(samples);
    for (size_t i = 0; i < samples; ++i) {
        Vector3 d = centre[std::min(samples - 1, i + 1)] - centre[i > 0 ? i - 1 : 0];
        along[i] = d.lengthSquared() > 1e-24 ? d.normalized() : Vector3(0.0, -1.0, 0.0);
        Vector3 point, normal;
        if (m_scalp && m_scalp->nearest(centre[i], &point, &normal)) {
            out[i] = normal;
        } else {
            out[i] = std::abs(along[i].y()) < 0.9 ? Vector3(0.0, 1.0, 0.0) : Vector3(0.0, 0.0, 1.0);
        }
    }
    for (int pass = 0; pass < 3; ++pass) {
        std::vector<Vector3> smoothed = out;
        for (size_t i = 0; i < samples; ++i)
            smoothed[i] = out[i > 0 ? i - 1 : 0] + out[i] * 2.0 + out[std::min(samples - 1, i + 1)];
        out = smoothed;
    }
    for (size_t i = 0; i < samples; ++i) {
        across[i] = Vector3::crossProduct(along[i], out[i]);
        if (across[i].lengthSquared() < 1e-24)
            across[i] = Vector3::crossProduct(along[i], Vector3(1.0, 0.0, 0.0));
        across[i].normalize();
        out[i] = Vector3::crossProduct(across[i], along[i]).normalized();
    }

    Random random(parameters.seed);
    auto taper = [&](double t) {
        // a little narrower at the root, full a third of the way, to a point at the tip
        double rise = 0.7 + 0.3 * std::sin(Math::Pi * 0.5 * std::min(1.0, t * 3.0));
        double fall = 1.0 - (1.0 - tip) * std::pow(std::max(0.0, (t - 0.3) / 0.7), 1.3);
        return rise * fall;
    };

    for (size_t k = 0; k < strandCount; ++k) {
        // where across the lock it runs (-1 one side, 1 the other), how long it is
        double u = strandCount > 1 ? -1.0 + 2.0 * (k + 0.5) / strandCount : 0.0;
        u += (random.next() - 0.5) * 0.5 / strandCount;
        double reach = 1.0 - 0.16 * std::abs(u) - 0.08 * random.next();
        // (neighbouring strands lie a little over each other, not in one plane)
        double layer = (k % 2) ? 0.6 : 0.0;
        const size_t rings = std::max((size_t)4, (size_t)std::round(samples * reach));
        std::vector<Vector3> points(rings), acrossAt(rings), outAt(rings);
        std::vector<double> halfWidth(rings), halfThickness(rings);
        std::vector<size_t> sources(rings);
        for (size_t i = 0; i < rings; ++i) {
            double t = (double)i / (rings - 1);
            double at = t * reach * (samples - 1);
            size_t a = std::min(samples - 2, (size_t)std::floor(at));
            double f = at - a;
            Vector3 c = centre[a] * (1.0 - f) + centre[a + 1] * f;
            double r = radius[a] * (1.0 - f) + radius[a + 1] * f;
            acrossAt[i] = (across[a] * (1.0 - f) + across[a + 1] * f).normalized();
            outAt[i] = (out[a] * (1.0 - f) + out[a + 1] * f).normalized();
            sources[i] = f < 0.5 ? source[a] : source[a + 1];
            double spread = 0.8 * (1.0 - 0.4 * t);
            halfWidth[i] = std::max(1e-5, r * 1.7 / strandCount * taper(t * reach));
            halfThickness[i] = halfWidth[i] * thickness;
            points[i] = c + acrossAt[i] * (u * r * spread) + outAt[i] * (layer * halfThickness[i]);
        }

        // Lying on the scalp: lifted out of it wherever it would go in, the root sunk a little
        // into it so it grows from it.
        const double hug = std::max(0.0, std::min(1.0, parameters.hug));
        auto lift = [&]() {
            if (!m_scalp)
                return;
            for (size_t i = 0; i < rings; ++i) {
                Vector3 point, normal;
                if (!m_scalp->nearest(points[i], &point, &normal))
                    continue;
                double clearance = 0 == i ? -halfThickness[i] * 0.6 : halfThickness[i] * 1.15;
                double height = Vector3::dotProduct(points[i] - point, normal);
                Vector3 tangential = points[i] - point - normal * height;
                if (height < clearance) {
                    points[i] = point + normal * clearance + tangential;
                } else if (hug > 0.0) {
                    // (drawn down onto the scalp where it is near it: within a few widths of
                    // the lock; what hangs well clear of the head, falls free)
                    double t = (double)i / (rings - 1);
                    double near = std::max(0.0, 1.0 - (height - clearance) / std::max(1e-9, 4.0 * halfWidth[0] * strandCount));
                    double pull = hug * near * (1.0 - 0.7 * t);
                    points[i] = point + normal * (clearance + (height - clearance) * (1.0 - pull)) + tangential;
                }
            }
        };
        lift();
        for (int pass = 0; pass < 2; ++pass) {
            std::vector<Vector3> smoothed = points;
            for (size_t i = 1; i + 1 < rings; ++i)
                smoothed[i] = (points[i - 1] + points[i] * 2.0 + points[i + 1]) * 0.25;
            points = smoothed;
            lift();
        }

        // The strand: a ring of the lens at each point, closed at the root, to a point at the tip.
        size_t firstVertex = m_vertices.size();
        for (size_t i = 0; i + 1 < rings; ++i) {
            Vector3 direction = points[i + 1] - points[i > 0 ? i - 1 : 0];
            direction = direction.lengthSquared() > 1e-24 ? direction.normalized() : along[0];
            Vector3 side = acrossAt[i] - direction * Vector3::dotProduct(acrossAt[i], direction);
            side = side.lengthSquared() > 1e-24 ? side.normalized() : acrossAt[i];
            Vector3 up = Vector3::crossProduct(side, direction).normalized();
            for (size_t j = 0; j < 8; ++j) {
                m_vertices.push_back(points[i] + side * (kLens[j][0] * halfWidth[i]) + up * (kLens[j][1] * halfThickness[i]));
                m_vertexSources.push_back(guide[sources[i]].sourceId);
            }
        }
        size_t tipVertex = m_vertices.size();
        m_vertices.push_back(points[rings - 1]);
        m_vertexSources.push_back(guide[sources[rings - 1]].sourceId);
        size_t rootVertex = m_vertices.size();
        m_vertices.push_back(points[0] - (points[1] - points[0]).normalized() * halfThickness[0] * 0.5);
        m_vertexSources.push_back(guide[sources[0]].sourceId);

        const size_t ringCount = rings - 1;
        double u0 = (double)k / strandCount, du = 1.0 / strandCount;
        auto uvAt = [&](size_t ring, size_t j) {
            return Vector2(u0 + du * j / 8.0, (double)ring / ringCount);
        };
        // (which way round the rings go: so the faces look out)
        bool flip = false;
        {
            Vector3 a = m_vertices[firstVertex], b = m_vertices[firstVertex + 1], c = m_vertices[firstVertex + 8 + 1];
            Vector3 normal = Vector3::crossProduct(b - a, c - a);
            Vector3 outward = (a + b) * 0.5 - points[0];
            flip = Vector3::dotProduct(normal, outward) < 0.0;
        }
        auto addFace = [&](std::vector<size_t> face, std::vector<Vector2> uv) {
            if (flip) {
                std::reverse(face.begin(), face.end());
                std::reverse(uv.begin(), uv.end());
            }
            m_faces.push_back(std::move(face));
            m_faceUvs.push_back(std::move(uv));
        };
        for (size_t i = 0; i + 1 < ringCount; ++i) {
            for (size_t j = 0; j < 8; ++j) {
                size_t jn = (j + 1) % 8;
                addFace({ firstVertex + i * 8 + j, firstVertex + i * 8 + jn, firstVertex + (i + 1) * 8 + jn, firstVertex + (i + 1) * 8 + j },
                    { uvAt(i, j), uvAt(i, j + 1), uvAt(i + 1, j + 1), uvAt(i + 1, j) });
            }
        }
        size_t lastRing = firstVertex + (ringCount - 1) * 8;
        for (size_t j = 0; j < 8; ++j) {
            size_t jn = (j + 1) % 8;
            addFace({ lastRing + j, lastRing + jn, tipVertex }, { uvAt(ringCount - 1, j), uvAt(ringCount - 1, j + 1), uvAt(ringCount, j) });
            addFace({ firstVertex + jn, firstVertex + j, rootVertex }, { uvAt(0, j + 1), uvAt(0, j), uvAt(0, j) });
        }
    }
}

}
