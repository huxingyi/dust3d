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
#include <cstdint>
#include <dust3d/mesh/rock_mesh.h>
#include <dust3d/mesh/spine_deformer.h>
#include <map>
#include <utility>

namespace dust3d {

void buildRockMesh(const std::vector<MeshNode>& meshNodes,
    float deformWidth,
    float deformThickness,
    int seed,
    float roughness,
    int detail,
    float angularity,
    float flattenBottom,
    float rotation,
    bool mirrored,
    std::vector<Vector3>* resultVertices,
    std::vector<std::vector<size_t>>* resultFaces,
    std::vector<Vector2>* resultVertexUvs)
{
    resultVertices->clear();
    resultFaces->clear();
    resultVertexUvs->clear();
    if (meshNodes.empty())
        return;

    // Integer lattice hash -> [0,1).
    auto hash3 = [](double xd, double yd, double zd) -> double {
        int32_t xi = (int32_t)std::lround(xd);
        int32_t yi = (int32_t)std::lround(yd);
        int32_t zi = (int32_t)std::lround(zd);
        uint32_t h = 2166136261u;
        h = (h ^ (uint32_t)xi) * 16777619u;
        h = (h ^ (uint32_t)yi) * 16777619u;
        h = (h ^ (uint32_t)zi) * 16777619u;
        h ^= h >> 13;
        h *= 0x5bd1e995u;
        h ^= h >> 15;
        return (h & 0x00ffffffu) / 16777216.0;
    };
    // Cosine-smoothed trilinear value noise over the integer lattice.
    auto noise3 = [&](const Vector3& p) -> double {
        double ix = std::floor(p.x());
        double iy = std::floor(p.y());
        double iz = std::floor(p.z());
        auto smooth = [](double t) { return (1.0 - std::cos(t * 3.14159265358979323846)) * 0.5; };
        double sx = smooth(p.x() - ix);
        double sy = smooth(p.y() - iy);
        double sz = smooth(p.z() - iz);
        auto lerp = [](double a, double b, double t) { return a + (b - a) * t; };
        double c000 = hash3(ix, iy, iz);
        double c100 = hash3(ix + 1, iy, iz);
        double c010 = hash3(ix, iy + 1, iz);
        double c110 = hash3(ix + 1, iy + 1, iz);
        double c001 = hash3(ix, iy, iz + 1);
        double c101 = hash3(ix + 1, iy, iz + 1);
        double c011 = hash3(ix, iy + 1, iz + 1);
        double c111 = hash3(ix + 1, iy + 1, iz + 1);
        double x00 = lerp(c000, c100, sx);
        double x10 = lerp(c010, c110, sx);
        double x01 = lerp(c001, c101, sx);
        double x11 = lerp(c011, c111, sx);
        double y0 = lerp(x00, x10, sy);
        double y1 = lerp(x01, x11, sy);
        return lerp(y0, y1, sz);
    };
    // Fractal sum of octaves, normalized to [0,1].
    auto fbm = [&](Vector3 p, int octaves) -> double {
        double ret = 0.0;
        double amp = 1.0;
        double frq = 1.0;
        double total = 0.0;
        for (int i = 0; i < octaves; ++i) {
            ret += noise3(p * frq) * amp;
            total += amp;
            frq *= 2.0;
            amp *= 0.5;
        }
        return total > 0.0 ? ret / total : 0.0;
    };

    // Build an icosphere by subdividing an icosahedron, then normalizing to the unit sphere.
    std::vector<Vector3> vertices;
    std::vector<std::array<size_t, 3>> faces;
    {
        const double t = (1.0 + std::sqrt(5.0)) / 2.0;
        std::vector<Vector3> base = {
            { -1, t, 0 }, { 1, t, 0 }, { -1, -t, 0 }, { 1, -t, 0 },
            { 0, -1, t }, { 0, 1, t }, { 0, -1, -t }, { 0, 1, -t },
            { t, 0, -1 }, { t, 0, 1 }, { -t, 0, -1 }, { -t, 0, 1 }
        };
        for (auto& v : base)
            vertices.push_back(v.normalized());
        faces = {
            { 0, 11, 5 }, { 0, 5, 1 }, { 0, 1, 7 }, { 0, 7, 10 }, { 0, 10, 11 },
            { 1, 5, 9 }, { 5, 11, 4 }, { 11, 10, 2 }, { 10, 7, 6 }, { 7, 1, 8 },
            { 3, 9, 4 }, { 3, 4, 2 }, { 3, 2, 6 }, { 3, 6, 8 }, { 3, 8, 9 },
            { 4, 9, 5 }, { 2, 4, 11 }, { 6, 2, 10 }, { 8, 6, 7 }, { 9, 8, 1 }
        };
        int subdivisions = std::max(0, std::min(4, detail));
        for (int s = 0; s < subdivisions; ++s) {
            std::map<std::pair<size_t, size_t>, size_t> midpointCache;
            auto midpoint = [&](size_t a, size_t b) -> size_t {
                auto key = a < b ? std::make_pair(a, b) : std::make_pair(b, a);
                auto found = midpointCache.find(key);
                if (found != midpointCache.end())
                    return found->second;
                Vector3 m = ((vertices[a] + vertices[b]) * 0.5).normalized();
                size_t index = vertices.size();
                vertices.push_back(m);
                midpointCache.insert({ key, index });
                return index;
            };
            std::vector<std::array<size_t, 3>> newFaces;
            newFaces.reserve(faces.size() * 4);
            for (const auto& f : faces) {
                size_t a = midpoint(f[0], f[1]);
                size_t b = midpoint(f[1], f[2]);
                size_t c = midpoint(f[2], f[0]);
                newFaces.push_back({ f[0], a, c });
                newFaces.push_back({ f[1], b, a });
                newFaces.push_back({ f[2], c, b });
                newFaces.push_back({ a, b, c });
            }
            faces = std::move(newFaces);
        }
    }

    // Seed offsets so different seeds yield different rocks.
    Vector3 seedOffset(hash3(seed, 0, 0) * 100.0,
        hash3(0, seed, 0) * 100.0,
        hash3(0, 0, seed) * 100.0);

    int detailOctaves = std::max(2, std::min(6, detail + 2));
    float clampedRoughness = std::max(0.0f, std::min(1.0f, roughness));
    float clampedAngularity = std::max(0.0f, std::min(1.0f, angularity));

    // Build the rock shape in a canonical local space centred at the origin with
    // Y as the primary axis, mirroring how an imported model is authored. The
    // tube spine transformation below stretches this unit shape along the spine
    // and scales each cross-section to the node radius.
    std::vector<Vector3> positions(vertices.size());
    for (size_t i = 0; i < vertices.size(); ++i) {
        const Vector3& dir = vertices[i];
        // Large lobes break the silhouette; high-frequency fBm adds surface roughness.
        double shape = fbm(dir * 1.2 + seedOffset, 3);
        double surface = fbm(dir * 3.4 + seedOffset + Vector3(13.0, 7.0, 19.0), detailOctaves);
        double radial = 1.0
            + (shape - 0.5) * 0.5
            + (surface - 0.5) * clampedRoughness * 0.45;
        if (radial < 0.35)
            radial = 0.35;
        positions[i] = dir * radial;
    }

    // Angular facets: slice the rock with a few random planes and flatten anything past them.
    int numCuts = (int)std::round(clampedAngularity * 8.0f);
    for (int c = 0; c < numCuts; ++c) {
        Vector3 pn(hash3(c + 1, seed, 11) * 2.0 - 1.0,
            hash3(c + 1, seed, 23) * 2.0 - 1.0,
            hash3(c + 1, seed, 47) * 2.0 - 1.0);
        if (pn.length() < 0.0001)
            continue;
        pn = pn.normalized();
        double cutDist = 0.55 + hash3(c + 1, seed, 91) * 0.35;
        for (auto& p : positions) {
            double dist = Vector3::dotProduct(p, pn);
            if (dist > cutDist)
                p = p - pn * (dist - cutDist);
        }
    }

    // Flatten the bottom so the rock rests on the ground plane.
    if (flattenBottom > 0.0001f) {
        double minY = positions[0].y();
        double maxY = positions[0].y();
        for (const auto& p : positions) {
            minY = std::min(minY, p.y());
            maxY = std::max(maxY, p.y());
        }
        double planeY = minY + (maxY - minY) * std::min(1.0f, flattenBottom) * 0.5;
        for (auto& p : positions) {
            if (p.y() < planeY)
                p.setY(planeY);
        }
    }

    // Azimuthal (top-down disc) UV mapping derived from the original
    // (pre-displacement) sphere direction. The top of the rock maps to the
    // centre of the texture and the bottom maps to the outer rim, so the only
    // singularity / stretched band lands on the flattened underside that is
    // normally not visible. Unlike a lat/long mapping there is no vertical seam.
    const double pi = 3.14159265358979323846;
    resultVertexUvs->resize(vertices.size());
    for (size_t i = 0; i < vertices.size(); ++i) {
        const Vector3& dir = vertices[i];
        double theta = std::acos(std::max(-1.0, std::min(1.0, dir.y()))); // 0 at top (+Y), pi at bottom (-Y)
        double phi = std::atan2(dir.z(), dir.x());
        double r = theta / pi; // 0 at top centre, 1 at bottom rim
        double u = 0.5 + 0.5 * r * std::cos(phi);
        double v = 0.5 + 0.5 * r * std::sin(phi);
        if (mirrored)
            u = 1.0 - u;
        (*resultVertexUvs)[i] = Vector2(u, v);
    }

    // Bounding box of the canonical shape (its Y range is the primary axis).
    Vector3 localMin = positions[0];
    Vector3 localMax = positions[0];
    for (const auto& p : positions) {
        localMin.setX(std::min(localMin.x(), p.x()));
        localMin.setY(std::min(localMin.y(), p.y()));
        localMin.setZ(std::min(localMin.z(), p.z()));
        localMax.setX(std::max(localMax.x(), p.x()));
        localMax.setY(std::max(localMax.y(), p.y()));
        localMax.setZ(std::max(localMax.z(), p.z()));
    }

    if (meshNodes.size() >= 2) {
        // Sweep the rock along the tube spine using the same transformation as
        // the imported model generator: Y selects the position along the spine,
        // X/Z become the cross-section scaled to the interpolated node radius,
        // and the rotation spins the cross-section.
        SpineDeformer spineDeformer(meshNodes, localMin, localMax,
            deformWidth, deformThickness, rotation);
        for (auto& p : positions)
            p = spineDeformer.deformVertex(p);
    } else {
        // Single-node tube: there is no spine direction to follow, so place the
        // rock in the node sphere. Width/thickness squash the cross-section and
        // vertical axis, and the rotation spins it around the vertical axis.
        const Vector3& center = meshNodes[0].origin;
        double radius = meshNodes[0].radius;
        Vector3 extent(radius * deformWidth, radius * deformThickness, radius * deformWidth);
        double rotationAngle = rotation * pi;
        double cosR = std::cos(rotationAngle);
        double sinR = std::sin(rotationAngle);
        for (auto& p : positions) {
            double x = p.x() * extent.x();
            double y = p.y() * extent.y();
            double z = p.z() * extent.z();
            p.setX(center.x() + x * cosR - z * sinR);
            p.setY(center.y() + y);
            p.setZ(center.z() + x * sinR + z * cosR);
        }
    }

    *resultVertices = positions;
    resultFaces->reserve(faces.size());
    for (const auto& f : faces) {
        if (mirrored)
            resultFaces->push_back({ f[0], f[2], f[1] });
        else
            resultFaces->push_back({ f[0], f[1], f[2] });
    }
    if (mirrored) {
        for (auto& v : *resultVertices)
            v.setX(-v.x());
    }
}

}
