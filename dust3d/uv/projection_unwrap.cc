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
#include <dust3d/uv/projection_unwrap.h>
#include <functional>
#include <limits>
#include <map>
#include <numeric>

namespace dust3d {

std::vector<std::array<Vector2, 3>> projectionUnwrap(const std::vector<Vector3>& vertices,
    const std::vector<std::vector<size_t>>& triangles)
{
    std::vector<std::array<Vector2, 3>> uvs(triangles.size());
    if (triangles.empty())
        return uvs;

    // area weighted normals, smoothed over the triangles around each corner
    std::vector<Vector3> normals(triangles.size());
    for (size_t t = 0; t < triangles.size(); ++t) {
        const auto& f = triangles[t];
        if (f.size() >= 3)
            normals[t] = Vector3::crossProduct(vertices[f[1]] - vertices[f[0]], vertices[f[2]] - vertices[f[0]]);
    }
    std::vector<std::vector<size_t>> vertexTriangles(vertices.size());
    for (size_t t = 0; t < triangles.size(); ++t) {
        for (size_t v : triangles[t])
            vertexTriangles[v].push_back(t);
    }
    std::vector<Vector3> smoothed = normals;
    for (int pass = 0; pass < 3; ++pass) {
        std::vector<Vector3> vertexNormals(vertices.size());
        for (size_t v = 0; v < vertices.size(); ++v) {
            for (size_t t : vertexTriangles[v])
                vertexNormals[v] += smoothed[t];
        }
        for (size_t t = 0; t < triangles.size(); ++t) {
            Vector3 sum = normals[t];
            for (size_t v : triangles[t])
                sum += vertexNormals[v].normalized() * normals[t].length();
            smoothed[t] = sum;
        }
    }

    // the side each triangle faces
    std::vector<int> side(triangles.size(), 0);
    for (size_t t = 0; t < triangles.size(); ++t) {
        double most = -1.0;
        for (int axis = 0; axis < 3; ++axis) {
            if (std::abs(smoothed[t][axis]) > most) {
                most = std::abs(smoothed[t][axis]);
                side[t] = axis * 2 + (smoothed[t][axis] < 0.0 ? 1 : 0);
            }
        }
    }

    // islands: same side, joined through shared edges
    std::vector<size_t> group(triangles.size());
    std::iota(group.begin(), group.end(), 0);
    std::function<size_t(size_t)> root = [&](size_t t) {
        while (group[t] != t)
            t = group[t] = group[group[t]];
        return t;
    };
    std::map<std::pair<size_t, size_t>, size_t> edgeOwner;
    for (size_t t = 0; t < triangles.size(); ++t) {
        const auto& f = triangles[t];
        for (size_t k = 0; k < f.size(); ++k) {
            size_t a = f[k], b = f[(k + 1) % f.size()];
            auto edge = std::make_pair(std::min(a, b), std::max(a, b));
            auto inserted = edgeOwner.insert({ edge, t });
            if (!inserted.second && side[inserted.first->second] == side[t])
                group[root(t)] = root(inserted.first->second);
        }
    }
    std::map<size_t, std::vector<size_t>> islands;
    for (size_t t = 0; t < triangles.size(); ++t)
        islands[root(t)].push_back(t);

    // each island flat, seen from its side, up in the model up in the texture
    struct Island {
        std::vector<size_t> triangles;
        int axisU = 0;
        int axisV = 1;
        double flipU = 1.0;
        double flipV = 1.0;
        double minU = 0.0, minV = 0.0, width = 0.0, height = 0.0;
        double x = 0.0, y = 0.0;
    };
    std::vector<Island> flat;
    double area = 0.0;
    for (auto& it : islands) {
        Island island;
        island.triangles = std::move(it.second);
        int s = side[island.triangles[0]];
        int axis = s / 2;
        bool negative = 1 == (s % 2);
        if (1 == axis) {
            island.axisU = 0;
            island.axisV = 2;
            island.flipV = negative ? -1.0 : 1.0;
        } else {
            island.axisU = 0 == axis ? 2 : 0;
            island.axisV = 1;
            island.flipV = -1.0;
            // (seen from outside, not mirrored)
            island.flipU = (0 == axis) == negative ? 1.0 : -1.0;
        }
        island.minU = island.minV = std::numeric_limits<double>::max();
        double maxU = std::numeric_limits<double>::lowest(), maxV = maxU;
        for (size_t t : island.triangles) {
            for (size_t v : triangles[t]) {
                double u = vertices[v][island.axisU] * island.flipU, w = vertices[v][island.axisV] * island.flipV;
                island.minU = std::min(island.minU, u);
                island.minV = std::min(island.minV, w);
                maxU = std::max(maxU, u);
                maxV = std::max(maxV, w);
            }
        }
        island.width = maxU - island.minU;
        island.height = maxV - island.minV;
        area += island.width * island.height;
        flat.push_back(std::move(island));
    }

    // shelves, tallest first, in a square about the size of all the islands together
    double gap = std::sqrt(area) * 0.01;
    double shelfWidth = std::sqrt(area) * 1.1;
    for (const auto& island : flat)
        shelfWidth = std::max(shelfWidth, island.width + 2.0 * gap);
    std::vector<size_t> order(flat.size());
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](size_t a, size_t b) { return flat[a].height > flat[b].height; });
    double x = gap, y = gap, shelfHeight = 0.0, usedWidth = 0.0;
    for (size_t i : order) {
        auto& island = flat[i];
        if (x + island.width + gap > shelfWidth && x > gap) {
            x = gap;
            y += shelfHeight + gap;
            shelfHeight = 0.0;
        }
        island.x = x;
        island.y = y;
        x += island.width + gap;
        usedWidth = std::max(usedWidth, x);
        shelfHeight = std::max(shelfHeight, island.height);
    }
    double side_ = std::max(usedWidth, y + shelfHeight + gap);
    if (side_ <= 0.0)
        side_ = 1.0;
    for (const auto& island : flat) {
        for (size_t t : island.triangles) {
            const auto& f = triangles[t];
            for (size_t k = 0; k < 3 && k < f.size(); ++k) {
                const auto& p = vertices[f[k]];
                uvs[t][k] = Vector2((island.x + p[island.axisU] * island.flipU - island.minU) / side_,
                    (island.y + p[island.axisV] * island.flipV - island.minV) / side_);
            }
        }
    }
    return uvs;
}

}
