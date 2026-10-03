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

#include <AutoRemesher/IsotropicRemesher>
#include <AutoRemesher/Parameterizer>
#include <AutoRemesher/QuadExtractor>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <dust3d/base/debug.h>
#include <dust3d/mesh/wrap_mesh_builder.h>
#include <dust3d/uv/chart_packer.h>
#include <functional>
#include <limits>
#include <map>
#include <queue>
#include <set>
#include <thread>
#include <unordered_map>

namespace dust3d {

namespace {

    const float kFar = 1.0e6f;

    Vector3 closestPointOnTriangle(const Vector3& p, const Vector3& a, const Vector3& b, const Vector3& c)
    {
        Vector3 ab = b - a;
        Vector3 ac = c - a;
        Vector3 ap = p - a;
        double d1 = Vector3::dotProduct(ab, ap);
        double d2 = Vector3::dotProduct(ac, ap);
        if (d1 <= 0.0 && d2 <= 0.0)
            return a;
        Vector3 bp = p - b;
        double d3 = Vector3::dotProduct(ab, bp);
        double d4 = Vector3::dotProduct(ac, bp);
        if (d3 >= 0.0 && d4 <= d3)
            return b;
        double vc = d1 * d4 - d3 * d2;
        if (vc <= 0.0 && d1 >= 0.0 && d3 <= 0.0) {
            double v = d1 / (d1 - d3);
            return a + ab * v;
        }
        Vector3 cp = p - c;
        double d5 = Vector3::dotProduct(ab, cp);
        double d6 = Vector3::dotProduct(ac, cp);
        if (d6 >= 0.0 && d5 <= d6)
            return c;
        double vb = d5 * d2 - d1 * d6;
        if (vb <= 0.0 && d2 >= 0.0 && d6 <= 0.0) {
            double w = d2 / (d2 - d6);
            return a + ac * w;
        }
        double va = d3 * d6 - d5 * d4;
        if (va <= 0.0 && (d4 - d3) >= 0.0 && (d5 - d6) >= 0.0) {
            double w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
            return b + (c - b) * w;
        }
        double denom = va + vb + vc;
        if (std::abs(denom) < 1e-30)
            return a;
        double v = vb / denom;
        double w = vc / denom;
        return a + ab * v + ac * w;
    }

    // Polynomial smooth minimum (blends a and b over a band k).
    inline float smoothMin(float a, float b, float k)
    {
        if (k <= 0.0f)
            return std::min(a, b);
        float h = std::max(k - std::abs(a - b), 0.0f) / k;
        return std::min(a, b) - h * h * k * 0.25f;
    }

    inline float smoothMax(float a, float b, float k)
    {
        return -smoothMin(-a, -b, k);
    }

    void addWeight(std::vector<WrapMeshBuilder::NodeWeight>& weights, const Uuid& nodeId, float weight)
    {
        for (auto& it : weights) {
            if (it.nodeId == nodeId) {
                it.weight += weight;
                return;
            }
        }
        weights.push_back({ nodeId, weight });
    }

    void normalizeWeights(std::vector<WrapMeshBuilder::NodeWeight>& weights, size_t maxCount)
    {
        std::sort(weights.begin(), weights.end(), [](const WrapMeshBuilder::NodeWeight& a, const WrapMeshBuilder::NodeWeight& b) {
            if (a.weight != b.weight)
                return a.weight > b.weight;
            return a.nodeId < b.nodeId;
        });
        if (weights.size() > maxCount)
            weights.resize(maxCount);
        float sum = 0.0f;
        for (const auto& it : weights)
            sum += it.weight;
        if (sum <= 0.0f) {
            weights.clear();
            return;
        }
        for (auto& it : weights)
            it.weight /= sum;
        // drop negligible influences, they only cost bone slots
        weights.erase(std::remove_if(weights.begin(), weights.end(), [](const WrapMeshBuilder::NodeWeight& w) {
            return w.weight < 0.01f;
        }),
            weights.end());
        sum = 0.0f;
        for (const auto& it : weights)
            sum += it.weight;
        if (sum > 0.0f) {
            for (auto& it : weights)
                it.weight /= sum;
        }
    }

}

void WrapMeshBuilder::setParameters(const Parameters& parameters)
{
    m_parameters = parameters;
}

void WrapMeshBuilder::addSource(const std::vector<Vector3>& vertices,
    const std::vector<std::vector<size_t>>& faces,
    bool subtract)
{
    Source source;
    source.vertices = vertices;
    source.subtract = subtract;
    for (const auto& face : faces) {
        if (face.size() < 3)
            continue;
        for (size_t i = 1; i + 1 < face.size(); ++i) {
            if (face[0] >= vertices.size() || face[i] >= vertices.size() || face[i + 1] >= vertices.size())
                continue;
            source.triangles.push_back({ face[0], face[i], face[i + 1] });
        }
    }
    if (source.triangles.empty())
        return;
    m_sources.emplace_back(std::move(source));
}

void WrapMeshBuilder::addBindSample(const Vector3& position, const Uuid& nodeId)
{
    m_bindSamples.push_back({ position, nodeId });
}

std::vector<Uuid> WrapMeshBuilder::bindNodeIds() const
{
    std::set<Uuid> ids;
    for (const auto& sample : m_bindSamples)
        ids.insert(sample.second);
    return std::vector<Uuid>(ids.begin(), ids.end());
}

bool WrapMeshBuilder::build()
{
    m_errorMessage.clear();
    if (m_sources.empty()) {
        m_errorMessage = "Nothing to wrap";
        return false;
    }

    // unions first, then the carving sources
    std::stable_sort(m_sources.begin(), m_sources.end(), [](const Source& a, const Source& b) {
        return (int)a.subtract < (int)b.subtract;
    });

    double sourceArea = 0.0;
    m_sourceMin = Vector3(std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    m_sourceMax = Vector3(std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest());
    for (const auto& source : m_sources) {
        if (source.subtract)
            continue;
        for (const auto& triangle : source.triangles) {
            sourceArea += Vector3::area(source.vertices[triangle[0]], source.vertices[triangle[1]], source.vertices[triangle[2]]);
            for (size_t i = 0; i < 3; ++i) {
                const auto& v = source.vertices[triangle[i]];
                for (size_t a = 0; a < 3; ++a) {
                    m_sourceMin[a] = std::min(m_sourceMin[a], v[a]);
                    m_sourceMax[a] = std::max(m_sourceMax[a], v[a]);
                }
            }
        }
    }
    if (sourceArea <= 0.0) {
        m_errorMessage = "Sources have no area";
        return false;
    }

    double height = m_sourceMax.y() - m_sourceMin.y();
    m_openTopY = m_sourceMax.y() - m_parameters.openTop * height;
    m_openBottomY = m_sourceMin.y() + m_parameters.openBottom * height;

    size_t targetFaces = std::max((size_t)64, m_parameters.targetFaces);

    // 1. A fine surface around the field, denser than wanted: the quad
    //    remesher places its vertices on it, so it has to carry the shape. (Three times
    //    the faces; the isotropic stage then evens it out at half the quad size.)
    // 2. The quad remesher (AutoRemesher): a cross field aligned with the curvature, a
    //    quad parameterization of it and the quads extracted from that. Edge loops run
    //    around the limbs and the torso, along creases, with few singularities.
    // The sources overlap and the crevices between them get filled, so the wrap has
    // less area than the sources together: the remesher is sized from the area of the
    // fine surface itself.
    bool remeshed = false;
    double fineCell = std::sqrt(sourceArea * 0.7 / ((double)targetFaces * 3.0));
    auto clock = std::chrono::steady_clock::now();
    auto lap = [&](const char* name) {
        auto now = std::chrono::steady_clock::now();
        dust3dDebug << "Wrap stage" << name << std::chrono::duration<double>(now - clock).count() << "s";
        clock = now;
    };
    if (buildAtCellSize(fineCell) && !m_faces.empty()) {
        lap("field and fine surface");
        m_vertexOnOpening.assign(m_vertices.size(), false);
        relaxSurface(true);
        cutAlongCreases();
        lap("relax and creases");
        remeshed = remeshQuads(targetFaces);
        lap("remesh");
    }
    if (!remeshed) {
        // fallback: the grid surface itself, its cell sized to the wanted face count
        double cellSize = std::sqrt(sourceArea * 0.7 / (double)targetFaces);
        for (int attempt = 0; attempt < 3; ++attempt) {
            if (!buildAtCellSize(cellSize))
                return false;
            double faceCount = (double)m_faces.size();
            if (faceCount <= 0.0) {
                m_errorMessage = "Empty wrap surface";
                return false;
            }
            double ratio = faceCount / (double)targetFaces;
            if (ratio > 0.85 && ratio < 1.15)
                break;
            if (attempt == 2)
                break;
            cellSize *= std::sqrt(ratio);
        }
        m_vertexOnOpening.assign(m_vertices.size(), false);
        relaxSurface(true);
    }
    m_remeshed = remeshed;

    openSurface();
    // a remeshed surface keeps its edge flow: only the rims of the openings are evened out
    relaxSurface(!remeshed);
    addThickness();
    triangulate();
    lap("openings and thickness");
    transferWeights();
    lap("weights");
    generateUvs();
    lap("uvs");
    dumpForDebugging();
    return !m_resultTriangles.empty();
}

void WrapMeshBuilder::dumpForDebugging() const
{
    // DUST3D_WRAP_DUMP=<folder> writes the sources and the result of every wrap as OBJ files
    const char* folder = std::getenv("DUST3D_WRAP_DUMP");
    if (nullptr == folder || 0 == folder[0])
        return;
    static int counter = 0;
    int index = counter++;
    auto writeObj = [&](const std::string& path, const std::vector<Vector3>& vertices, const std::vector<std::vector<size_t>>& faces) {
        FILE* file = std::fopen(path.c_str(), "w");
        if (nullptr == file)
            return;
        for (const auto& v : vertices)
            std::fprintf(file, "v %f %f %f\n", v.x(), v.y(), v.z());
        for (const auto& f : faces) {
            std::fprintf(file, "f");
            for (size_t i : f)
                std::fprintf(file, " %zu", i + 1);
            std::fprintf(file, "\n");
        }
        std::fclose(file);
    };
    std::string prefix = std::string(folder) + "/wrap" + std::to_string(index);
    writeObj(prefix + "_result.obj", m_resultVertices, m_resultTriangleAndQuads);
    for (size_t s = 0; s < m_sources.size(); ++s) {
        std::vector<std::vector<size_t>> faces;
        for (const auto& t : m_sources[s].triangles)
            faces.push_back({ t[0], t[1], t[2] });
        writeObj(prefix + "_source" + std::to_string(s) + ".obj", m_sources[s].vertices, faces);
    }
}

bool WrapMeshBuilder::buildAtCellSize(double cellSize)
{
    computeField(cellSize);
    if (m_grid.values.empty()) {
        m_errorMessage = "Wrap grid is too large";
        return false;
    }
    extractSurface();
    return true;
}

void WrapMeshBuilder::computeField(double cellSize)
{
    const auto& p = m_parameters;
    double band = std::abs(p.offset) + p.smoothness + p.thickness + 3.0 * cellSize;

    // keep the grid size reasonable whatever the requested face count
    const double maxCells = 6.0e6;
    while (true) {
        double margin = std::abs(p.offset) + p.smoothness * 0.5 + p.thickness + 3.0 * cellSize;
        double sx = (m_sourceMax.x() - m_sourceMin.x()) + 2.0 * margin;
        double sy = (m_sourceMax.y() - m_sourceMin.y()) + 2.0 * margin;
        double sz = (m_sourceMax.z() - m_sourceMin.z()) + 2.0 * margin;
        double cells = (sx / cellSize + 2.0) * (sy / cellSize + 2.0) * (sz / cellSize + 2.0);
        if (cells <= maxCells)
            break;
        cellSize *= std::cbrt(cells / maxCells) * 1.01;
    }
    m_cellSize = cellSize;
    band = std::abs(p.offset) + p.smoothness + p.thickness + 3.0 * cellSize;
    double margin = std::abs(p.offset) + p.smoothness * 0.5 + p.thickness + 3.0 * cellSize;

    Grid& grid = m_grid;
    grid = Grid();
    grid.h = cellSize;
    Vector3 lower = m_sourceMin - Vector3(margin, margin, margin);
    Vector3 upper = m_sourceMax + Vector3(margin, margin, margin);

    // A left-right symmetric model gets a grid that is symmetric about x = 0, so the
    // sampled field, and the surface extracted from it, are symmetric too.
    double width = m_sourceMax.x() - m_sourceMin.x();
    bool symmetric = std::abs(m_sourceMin.x() + m_sourceMax.x()) < 0.02 * width + 1e-6;
    if (symmetric) {
        double halfX = std::max(std::abs(lower.x()), std::abs(upper.x()));
        grid.nx = (int)std::ceil(2.0 * halfX / cellSize) + 1;
        grid.origin.setX(-(grid.nx - 1) * cellSize * 0.5);
    } else {
        grid.nx = (int)std::ceil((upper.x() - lower.x()) / cellSize) + 1;
        grid.origin.setX(lower.x());
    }
    grid.ny = (int)std::ceil((upper.y() - lower.y()) / cellSize) + 1;
    grid.nz = (int)std::ceil((upper.z() - lower.z()) / cellSize) + 1;
    grid.origin.setY(lower.y());
    grid.origin.setZ(lower.z());
    if (grid.nx < 3 || grid.ny < 3 || grid.nz < 3) {
        grid.values.clear();
        return;
    }
    grid.values.assign((size_t)grid.nx * grid.ny * grid.nz, kFar);

    m_carve.clear();
    for (const auto& source : m_sources) {
        if (!source.subtract)
            addSourceToField(source, band);
    }

    if (p.mode == Mode::Cloth && p.drape > 0.0)
        applyDrape();
    // A skin or a garment is smooth: no ridges from the children's polygonal cross
    // sections, no kinks from the drape (both would pull the cross field around).
    // (one pass on a skin: more erodes thin parts, the hands)
    smoothField(p.mode == Mode::Cloth ? 2 : 1, (float)band);

    if (p.offset != 0.0) {
        float offset = (float)p.offset;
        for (auto& value : grid.values)
            value -= offset;
    }

    // the closed surface before anything is cut away: the creases of openings lie on it
    if (p.mode == Mode::Cloth)
        m_baseField = grid.values;
    else
        m_baseField.clear();

    // Carving comes after the offset, so a carving child is cut out exactly. On cloth
    // it opens the surface (a neckline, an armhole): the carved faces are removed later.
    bool hasCarving = false;
    for (const auto& source : m_sources) {
        if (source.subtract) {
            if (!hasCarving) {
                m_carve.assign(grid.values.size(), kFar);
                hasCarving = true;
            }
            addSourceToField(source, band);
        }
    }

    if (p.mode == Mode::Cloth)
        applyOpenings();

    disambiguate();

    // the outer layer of the grid is always outside, so the surface is closed
    float outside = (float)(cellSize * 0.5);
    for (int k = 0; k < grid.nz; ++k) {
        for (int j = 0; j < grid.ny; ++j) {
            for (int i = 0; i < grid.nx; ++i) {
                if (i == 0 || j == 0 || k == 0 || i == grid.nx - 1 || j == grid.ny - 1 || k == grid.nz - 1) {
                    auto& value = grid.values[grid.index(i, j, k)];
                    value = std::max(value, outside);
                }
            }
        }
    }
}

void WrapMeshBuilder::addSourceToField(const Source& source, double band)
{
    Grid& grid = m_grid;
    const double h = grid.h;

    Vector3 bmin(std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    Vector3 bmax(std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest());
    for (const auto& v : source.vertices) {
        for (size_t a = 0; a < 3; ++a) {
            bmin[a] = std::min(bmin[a], v[a]);
            bmax[a] = std::max(bmax[a], v[a]);
        }
    }
    int lo[3], hi[3], dims[3] = { grid.nx, grid.ny, grid.nz };
    for (size_t a = 0; a < 3; ++a) {
        lo[a] = std::max(0, (int)std::floor((bmin[a] - band - grid.origin[a]) / h));
        hi[a] = std::min(dims[a] - 1, (int)std::ceil((bmax[a] + band - grid.origin[a]) / h));
        if (lo[a] > hi[a])
            return;
    }
    const int mx = hi[0] - lo[0] + 1;
    const int my = hi[1] - lo[1] + 1;
    const int mz = hi[2] - lo[2] + 1;
    const size_t count = (size_t)mx * my * mz;
    auto localIndex = [&](int i, int j, int k) { return ((size_t)k * my + j) * mx + i; };
    auto localPosition = [&](int i, int j, int k) {
        return grid.origin + Vector3((lo[0] + i) * h, (lo[1] + j) * h, (lo[2] + k) * h);
    };

    // 1. exact distance close to the surface, with the closest point
    std::vector<float> distance(count, kFar);
    std::vector<Vector3> closest(count);
    std::vector<char> known(count, 0);
    const double seedReach = 1.5 * h;
    for (const auto& triangle : source.triangles) {
        const Vector3& a = source.vertices[triangle[0]];
        const Vector3& b = source.vertices[triangle[1]];
        const Vector3& c = source.vertices[triangle[2]];
        int tlo[3], thi[3];
        for (size_t axis = 0; axis < 3; ++axis) {
            double minValue = std::min(a[axis], std::min(b[axis], c[axis])) - seedReach;
            double maxValue = std::max(a[axis], std::max(b[axis], c[axis])) + seedReach;
            tlo[axis] = std::max(0, (int)std::ceil((minValue - grid.origin[axis]) / h) - lo[axis]);
            int dimension = (axis == 0 ? mx : (axis == 1 ? my : mz));
            thi[axis] = std::min(dimension - 1, (int)std::floor((maxValue - grid.origin[axis]) / h) - lo[axis]);
        }
        for (int k = tlo[2]; k <= thi[2]; ++k) {
            for (int j = tlo[1]; j <= thi[1]; ++j) {
                for (int i = tlo[0]; i <= thi[0]; ++i) {
                    Vector3 position = localPosition(i, j, k);
                    Vector3 point = closestPointOnTriangle(position, a, b, c);
                    float d = (float)(position - point).length();
                    size_t index = localIndex(i, j, k);
                    if (d < distance[index]) {
                        distance[index] = d;
                        closest[index] = point;
                        known[index] = 1;
                    }
                }
            }
        }
    }

    // 2. spread the closest points over the rest of the box (fast sweeping)
    for (int pass = 0; pass < 2; ++pass) {
        for (int order = 0; order < 8; ++order) {
            int sx = (order & 1) ? -1 : 1;
            int sy = (order & 2) ? -1 : 1;
            int sz = (order & 4) ? -1 : 1;
            for (int kk = 0; kk < mz; ++kk) {
                int k = sz > 0 ? kk : mz - 1 - kk;
                for (int jj = 0; jj < my; ++jj) {
                    int j = sy > 0 ? jj : my - 1 - jj;
                    for (int ii = 0; ii < mx; ++ii) {
                        int i = sx > 0 ? ii : mx - 1 - ii;
                        size_t index = localIndex(i, j, k);
                        Vector3 position;
                        bool positionReady = false;
                        const int neighbors[3][3] = { { i - sx, j, k }, { i, j - sy, k }, { i, j, k - sz } };
                        for (const auto& n : neighbors) {
                            if (n[0] < 0 || n[0] >= mx || n[1] < 0 || n[1] >= my || n[2] < 0 || n[2] >= mz)
                                continue;
                            size_t neighborIndex = localIndex(n[0], n[1], n[2]);
                            if (!known[neighborIndex])
                                continue;
                            if (!positionReady) {
                                position = localPosition(i, j, k);
                                positionReady = true;
                            }
                            float d = (float)(position - closest[neighborIndex]).length();
                            if (d < distance[index]) {
                                distance[index] = d;
                                closest[index] = closest[neighborIndex];
                                known[index] = 1;
                            }
                        }
                    }
                }
            }
        }
    }

    // 3. inside / outside: ray parity along each axis, majority of the three
    std::vector<unsigned char> votes(count, 0);
    const int localDims[3] = { mx, my, mz };
    for (int axis = 0; axis < 3; ++axis) {
        int u = (axis + 1) % 3;
        int v = (axis + 2) % 3;
        int nu = localDims[u];
        int nv = localDims[v];
        std::vector<std::vector<double>> rays((size_t)nu * nv);
        // a tiny shift of the rays keeps them off triangle edges and vertices
        const double jitterU = h * 1.3e-4;
        const double jitterV = h * 0.7e-4;
        for (const auto& triangle : source.triangles) {
            const Vector3& a = source.vertices[triangle[0]];
            const Vector3& b = source.vertices[triangle[1]];
            const Vector3& c = source.vertices[triangle[2]];
            double au = a[u], av = a[v], bu = b[u], bv = b[v], cu = c[u], cv = c[v];
            double area2 = (bu - au) * (cv - av) - (bv - av) * (cu - au);
            if (std::abs(area2) < 1e-20)
                continue;
            double minU = std::min(au, std::min(bu, cu));
            double maxU = std::max(au, std::max(bu, cu));
            double minV = std::min(av, std::min(bv, cv));
            double maxV = std::max(av, std::max(bv, cv));
            int iu0 = std::max(0, (int)std::ceil((minU - grid.origin[u]) / h) - lo[u]);
            int iu1 = std::min(nu - 1, (int)std::floor((maxU - grid.origin[u]) / h) - lo[u]);
            int iv0 = std::max(0, (int)std::ceil((minV - grid.origin[v]) / h) - lo[v]);
            int iv1 = std::min(nv - 1, (int)std::floor((maxV - grid.origin[v]) / h) - lo[v]);
            for (int iv = iv0; iv <= iv1; ++iv) {
                double pv = grid.origin[v] + (lo[v] + iv) * h + jitterV;
                for (int iu = iu0; iu <= iu1; ++iu) {
                    double pu = grid.origin[u] + (lo[u] + iu) * h + jitterU;
                    double w0 = (bu - pu) * (cv - pv) - (bv - pv) * (cu - pu);
                    double w1 = (cu - pu) * (av - pv) - (cv - pv) * (au - pu);
                    double w2 = (au - pu) * (bv - pv) - (av - pv) * (bu - pu);
                    bool inside = (w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0);
                    if (!inside)
                        continue;
                    double hit = (w0 * a[axis] + w1 * b[axis] + w2 * c[axis]) / area2;
                    rays[(size_t)iv * nu + iu].push_back(hit);
                }
            }
        }
        for (int iv = 0; iv < nv; ++iv) {
            for (int iu = 0; iu < nu; ++iu) {
                auto& hits = rays[(size_t)iv * nu + iu];
                if (hits.size() < 2)
                    continue;
                std::sort(hits.begin(), hits.end());
                size_t passed = 0;
                int n = localDims[axis];
                for (int t = 0; t < n; ++t) {
                    double coordinate = grid.origin[axis] + (lo[axis] + t) * h;
                    while (passed < hits.size() && hits[passed] < coordinate)
                        ++passed;
                    if (passed % 2 == 1) {
                        int idx[3];
                        idx[axis] = t;
                        idx[u] = iu;
                        idx[v] = iv;
                        votes[localIndex(idx[0], idx[1], idx[2])]++;
                    }
                }
            }
        }
    }

    // 4. blend into the field
    const float k = (float)m_parameters.smoothness;
    for (int kk = 0; kk < mz; ++kk) {
        for (int jj = 0; jj < my; ++jj) {
            for (int ii = 0; ii < mx; ++ii) {
                size_t index = localIndex(ii, jj, kk);
                if (!known[index])
                    continue;
                float d = votes[index] >= 2 ? -distance[index] : distance[index];
                size_t gridIndex = grid.index(lo[0] + ii, lo[1] + jj, lo[2] + kk);
                float& value = grid.values[gridIndex];
                if (source.subtract) {
                    value = smoothMax(value, -d, k);
                    if (!m_carve.empty())
                        m_carve[gridIndex] = std::min(m_carve[gridIndex], d);
                } else {
                    value = smoothMin(value, d, k);
                }
            }
        }
    }
}

void WrapMeshBuilder::disambiguate()
{
    // Surface nets puts one vertex in a cell. Where a cell, or one of its faces, has two
    // separate inside (or outside) regions, the two sheets of surface share that vertex
    // and the mesh is not manifold there. Such features are smaller than a cell, so they
    // are resolved by moving the corner closest to the surface to the other side.
    Grid& grid = m_grid;
    const float nudge = (float)(grid.h * 1e-3);
    static const int corners[8][3] = { { 0, 0, 0 }, { 1, 0, 0 }, { 0, 1, 0 }, { 1, 1, 0 }, { 0, 0, 1 }, { 1, 0, 1 }, { 0, 1, 1 }, { 1, 1, 1 } };
    static const int edges[12][2] = { { 0, 1 }, { 2, 3 }, { 4, 5 }, { 6, 7 }, { 0, 2 }, { 1, 3 }, { 4, 6 }, { 5, 7 }, { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 } };
    static const int faces[6][4] = { { 0, 1, 3, 2 }, { 4, 5, 7, 6 }, { 0, 1, 5, 4 }, { 2, 3, 7, 6 }, { 0, 2, 6, 4 }, { 1, 3, 7, 5 } };
    auto regions = [&](const bool* inside, bool side) {
        int label[8];
        for (int c = 0; c < 8; ++c)
            label[c] = c;
        std::function<int(int)> find = [&](int x) { return label[x] == x ? x : (label[x] = find(label[x])); };
        for (const auto& edge : edges) {
            if (inside[edge[0]] == side && inside[edge[1]] == side)
                label[find(edge[0])] = find(edge[1]);
        }
        int count = 0;
        for (int c = 0; c < 8; ++c) {
            if (inside[c] == side && find(c) == c)
                ++count;
        }
        return count;
    };
    for (int pass = 0; pass < 8; ++pass) {
        size_t changes = 0;
        for (int k = 0; k + 1 < grid.nz; ++k) {
            for (int j = 0; j + 1 < grid.ny; ++j) {
                for (int i = 0; i + 1 < grid.nx; ++i) {
                    float* value[8];
                    bool inside[8];
                    int insideCount = 0;
                    for (int c = 0; c < 8; ++c) {
                        value[c] = &grid.values[grid.index(i + corners[c][0], j + corners[c][1], k + corners[c][2])];
                        inside[c] = *value[c] < 0.0f;
                        insideCount += inside[c] ? 1 : 0;
                    }
                    if (insideCount == 0 || insideCount == 8)
                        continue;
                    int flip = -1;
                    for (const auto& face : faces) {
                        // checkerboard face: a,c on one side, b,d on the other
                        if (inside[face[0]] == inside[face[2]] && inside[face[1]] == inside[face[3]] && inside[face[0]] != inside[face[1]]) {
                            flip = face[0];
                            for (int f = 1; f < 4; ++f) {
                                if (std::abs(*value[face[f]]) < std::abs(*value[flip]))
                                    flip = face[f];
                            }
                            break;
                        }
                    }
                    if (flip < 0 && (regions(inside, true) > 1 || regions(inside, false) > 1)) {
                        flip = 0;
                        for (int c = 1; c < 8; ++c) {
                            if (std::abs(*value[c]) < std::abs(*value[flip]))
                                flip = c;
                        }
                    }
                    if (flip < 0)
                        continue;
                    *value[flip] = inside[flip] ? nudge : -nudge;
                    ++changes;
                }
            }
        }
        if (0 == changes)
            break;
    }
}

void WrapMeshBuilder::smoothField(int passes, float limit)
{
    // Far from every source the field holds a placeholder (kFar): clamp it to the band
    // where distances are true, or the blur would pull it into thin parts (arms).
    for (auto& value : m_grid.values)
        value = std::max(-limit, std::min(limit, value));
    // A light 1-2-1 blur along each axis: the children's polygonal cross sections (a
    // hexagonal torso has ridges), the drape (a running minimum down each column) and
    // the blend of many guides leave small kinks in the field, which a curvature
    // aligned remesher would follow. Flat cuts and carving come after this, so the
    // creases of openings stay sharp.
    Grid& grid = m_grid;
    std::vector<float> temporary(grid.values.size());
    for (int pass = 0; pass < passes; ++pass) {
        for (int axis = 0; axis < 3; ++axis) {
            int n[3] = { grid.nx, grid.ny, grid.nz };
            for (int k = 0; k < grid.nz; ++k) {
                for (int j = 0; j < grid.ny; ++j) {
                    for (int i = 0; i < grid.nx; ++i) {
                        int p[3] = { i, j, k };
                        size_t index = grid.index(i, j, k);
                        if (p[axis] == 0 || p[axis] == n[axis] - 1) {
                            temporary[index] = grid.values[index];
                            continue;
                        }
                        int a[3] = { i, j, k }, b[3] = { i, j, k };
                        a[axis] -= 1;
                        b[axis] += 1;
                        temporary[index] = 0.25f * grid.values[grid.index(a[0], a[1], a[2])]
                            + 0.5f * grid.values[index]
                            + 0.25f * grid.values[grid.index(b[0], b[1], b[2])];
                    }
                }
            }
            grid.values.swap(temporary);
        }
    }
}

void WrapMeshBuilder::applyDrape()
{
    // Cloth falls from an overhang instead of following the body back in under it:
    // a point takes the value of the field above it plus the distance it has fallen,
    // scaled down by the drape. With no drape the scale is 1, which changes nothing
    // because the field already grows by at most 1 per unit of distance.
    Grid& grid = m_grid;
    float slope = (float)std::max(0.02, 1.0 - std::min(1.0, m_parameters.drape));
    float step = (float)(slope * grid.h);
    if (m_parameters.drapeLength <= 0.0) {
        for (int k = 0; k < grid.nz; ++k) {
            for (int i = 0; i < grid.nx; ++i) {
                for (int j = grid.ny - 2; j >= 0; --j) {
                    float above = grid.values[grid.index(i, j + 1, k)];
                    float& value = grid.values[grid.index(i, j, k)];
                    value = std::min(value, above + step);
                }
            }
        }
        return;
    }
    // limited fall: the cloth hangs at most drapeLength below what holds it up
    int reach = std::max(1, (int)std::round(m_parameters.drapeLength / grid.h));
    std::vector<float> column(grid.ny);
    for (int k = 0; k < grid.nz; ++k) {
        for (int i = 0; i < grid.nx; ++i) {
            for (int j = 0; j < grid.ny; ++j)
                column[j] = grid.values[grid.index(i, j, k)];
            for (int j = 0; j < grid.ny; ++j) {
                float best = column[j];
                int top = std::min(grid.ny - 1, j + reach);
                for (int a = j + 1; a <= top; ++a)
                    best = std::min(best, column[a] + step * (float)(a - j));
                grid.values[grid.index(i, j, k)] = best;
            }
        }
    }
}

void WrapMeshBuilder::applyOpenings()
{
    Grid& grid = m_grid;
    bool top = m_parameters.openTop > 0.0;
    bool bottom = m_parameters.openBottom > 0.0;
    if (!top && !bottom)
        return;
    for (int k = 0; k < grid.nz; ++k) {
        for (int j = 0; j < grid.ny; ++j) {
            double y = grid.origin.y() + j * grid.h;
            for (int i = 0; i < grid.nx; ++i) {
                float& value = grid.values[grid.index(i, j, k)];
                if (top)
                    value = std::max(value, (float)(y - m_openTopY));
                if (bottom)
                    value = std::max(value, (float)(m_openBottomY - y));
            }
        }
    }
}

double WrapMeshBuilder::sampleField(const Vector3& position) const
{
    return sampleGrid(m_grid.values, position);
}

double WrapMeshBuilder::sampleCarve(const Vector3& position) const
{
    if (m_carve.empty())
        return kFar;
    return sampleGrid(m_carve, position);
}

double WrapMeshBuilder::sampleGrid(const std::vector<float>& values, const Vector3& position) const
{
    const Grid& grid = m_grid;
    double fx = (position.x() - grid.origin.x()) / grid.h;
    double fy = (position.y() - grid.origin.y()) / grid.h;
    double fz = (position.z() - grid.origin.z()) / grid.h;
    fx = std::max(0.0, std::min((double)grid.nx - 1.000001, fx));
    fy = std::max(0.0, std::min((double)grid.ny - 1.000001, fy));
    fz = std::max(0.0, std::min((double)grid.nz - 1.000001, fz));
    int i = (int)fx, j = (int)fy, k = (int)fz;
    double tx = fx - i, ty = fy - j, tz = fz - k;
    auto value = [&](int di, int dj, int dk) { return (double)values[grid.index(i + di, j + dj, k + dk)]; };
    double c00 = value(0, 0, 0) * (1 - tx) + value(1, 0, 0) * tx;
    double c10 = value(0, 1, 0) * (1 - tx) + value(1, 1, 0) * tx;
    double c01 = value(0, 0, 1) * (1 - tx) + value(1, 0, 1) * tx;
    double c11 = value(0, 1, 1) * (1 - tx) + value(1, 1, 1) * tx;
    double c0 = c00 * (1 - ty) + c10 * ty;
    double c1 = c01 * (1 - ty) + c11 * ty;
    return c0 * (1 - tz) + c1 * tz;
}

Vector3 WrapMeshBuilder::sampleGradient(const Vector3& position) const
{
    double e = m_grid.h * 0.5;
    return Vector3(sampleField(position + Vector3(e, 0, 0)) - sampleField(position - Vector3(e, 0, 0)),
               sampleField(position + Vector3(0, e, 0)) - sampleField(position - Vector3(0, e, 0)),
               sampleField(position + Vector3(0, 0, e)) - sampleField(position - Vector3(0, 0, e)))
        / (2.0 * e);
}

Vector3 WrapMeshBuilder::projectToCarve(const Vector3& position) const
{
    if (m_carve.empty())
        return position;
    return projectToGrid(m_carve, position);
}

Vector3 WrapMeshBuilder::projectToGrid(const std::vector<float>& values, const Vector3& position) const
{
    // Newton steps onto the zero set of a grid field
    Vector3 result = position;
    double e = m_grid.h * 0.5;
    for (int iteration = 0; iteration < 3; ++iteration) {
        double value = sampleGrid(values, result);
        if (std::abs(value) < m_grid.h * 1e-3 || std::abs(value) > m_grid.h * 3.0)
            break;
        Vector3 gradient(sampleGrid(values, result + Vector3(e, 0, 0)) - sampleGrid(values, result - Vector3(e, 0, 0)),
            sampleGrid(values, result + Vector3(0, e, 0)) - sampleGrid(values, result - Vector3(0, e, 0)),
            sampleGrid(values, result + Vector3(0, 0, e)) - sampleGrid(values, result - Vector3(0, 0, e)));
        gradient /= 2.0 * e;
        double length2 = gradient.lengthSquared();
        if (length2 < 1e-12)
            break;
        Vector3 step = gradient * (value / length2);
        double stepLength = step.length();
        if (stepLength > m_grid.h)
            step *= m_grid.h / stepLength;
        result -= step;
    }
    return result;
}

double WrapMeshBuilder::openingSide(const Vector3& position) const
{
    // > 0 on the garment, < 0 on what an opening removes (the surface of a carving
    // child, the cap of a flat cut), 0 on the crease between them
    if (m_parameters.mode != Mode::Cloth || m_baseField.empty())
        return 1.0;
    double base = sampleGrid(m_baseField, position);
    double side = std::numeric_limits<double>::max();
    if (!m_carve.empty())
        side = std::min(side, base + sampleCarve(position));
    if (m_parameters.openTop > 0.0)
        side = std::min(side, base + (m_openTopY - position.y()));
    if (m_parameters.openBottom > 0.0)
        side = std::min(side, base + (position.y() - m_openBottomY));
    return side;
}

bool WrapMeshBuilder::hasOpenings() const
{
    return m_parameters.mode == Mode::Cloth && !m_baseField.empty()
        && (!m_carve.empty() || m_parameters.openTop > 0.0 || m_parameters.openBottom > 0.0);
}

Vector3 WrapMeshBuilder::projectToCrease(const Vector3& position) const
{
    // onto the cloth (the field before cutting) and the cutting shape, alternately
    Vector3 result = position;
    bool top = m_parameters.openTop > 0.0 && std::abs(position.y() - m_openTopY) < 2.0 * m_grid.h;
    bool bottom = m_parameters.openBottom > 0.0 && std::abs(position.y() - m_openBottomY) < 2.0 * m_grid.h;
    for (int alternation = 0; alternation < 4; ++alternation) {
        result = projectToGrid(m_baseField, result);
        if (top)
            result.setY(m_openTopY);
        else if (bottom)
            result.setY(m_openBottomY);
        else
            result = projectToCarve(result);
    }
    return result;
}

void WrapMeshBuilder::cutAlongCreases()
{
    // Where an opening will be cut the closed surface has a crease, which the grid has
    // rounded off. Cut the fine surface exactly along it: an edge chain on the crease,
    // which the remesher keeps as a feature, so an edge loop runs along it and the
    // opening is later removed along that loop. Near an existing vertex the vertex is
    // moved onto the crease instead of splitting, so no sliver triangles are made.
    if (!hasOpenings())
        return;
    std::vector<std::array<size_t, 3>> triangles;
    for (const auto& face : m_faces) {
        for (size_t i = 1; i + 1 < face.size(); ++i)
            triangles.push_back({ face[0], face[i], face[i + 1] });
    }
    std::vector<double> side(m_vertices.size());
    for (size_t v = 0; v < m_vertices.size(); ++v)
        side[v] = openingSide(m_vertices[v]);

    // 1. snap the vertices that are close to the crease along a crossing edge
    std::vector<bool> snapped(m_vertices.size(), false);
    for (const auto& triangle : triangles) {
        for (size_t i = 0; i < 3; ++i) {
            size_t a = triangle[i], b = triangle[(i + 1) % 3];
            if ((side[a] > 0.0) == (side[b] > 0.0) || snapped[a] || snapped[b])
                continue;
            double t = side[a] / (side[a] - side[b]);
            size_t near = t < 0.3 ? a : (t > 0.7 ? b : (size_t)-1);
            if (near == (size_t)-1)
                continue;
            m_vertices[near] = projectToCrease(m_vertices[near]);
            side[near] = 0.0;
            snapped[near] = true;
        }
    }
    // 2. split the edges that still cross it
    std::map<std::pair<size_t, size_t>, size_t> splitVertex;
    auto crossing = [&](size_t a, size_t b) -> size_t {
        if (side[a] == 0.0 || side[b] == 0.0 || (side[a] > 0.0) == (side[b] > 0.0))
            return (size_t)-1;
        auto key = std::make_pair(std::min(a, b), std::max(a, b));
        auto found = splitVertex.find(key);
        if (found != splitVertex.end())
            return found->second;
        double t = side[a] / (side[a] - side[b]);
        Vector3 position = projectToCrease(m_vertices[a] + (m_vertices[b] - m_vertices[a]) * t);
        size_t index = m_vertices.size();
        m_vertices.push_back(position);
        side.push_back(0.0);
        splitVertex[key] = index;
        return index;
    };
    std::vector<std::vector<size_t>> faces;
    for (const auto& triangle : triangles) {
        size_t cut[3];
        int cuts = 0;
        for (size_t i = 0; i < 3; ++i) {
            cut[i] = crossing(triangle[i], triangle[(i + 1) % 3]);
            if (cut[i] != (size_t)-1)
                ++cuts;
        }
        if (0 == cuts) {
            faces.push_back({ triangle[0], triangle[1], triangle[2] });
            continue;
        }
        // rotate so the first cut edge is (0,1)
        size_t r = 0;
        while (cut[r] == (size_t)-1)
            ++r;
        size_t v0 = triangle[r], v1 = triangle[(r + 1) % 3], v2 = triangle[(r + 2) % 3];
        size_t c01 = cut[r], c12 = cut[(r + 1) % 3], c20 = cut[(r + 2) % 3];
        if (1 == cuts) {
            // the crease runs from the cut to the opposite vertex (which is on it)
            faces.push_back({ v0, c01, v2 });
            faces.push_back({ c01, v1, v2 });
        } else if (c12 != (size_t)-1) {
            // v1 alone on one side
            faces.push_back({ c01, v1, c12 });
            faces.push_back({ v0, c01, c12 });
            faces.push_back({ v0, c12, v2 });
        } else {
            // v0 alone on one side (cuts on 0-1 and 2-0)
            faces.push_back({ v0, c01, c20 });
            faces.push_back({ c01, v1, v2 });
            faces.push_back({ c01, v2, c20 });
        }
    }
    m_faces.swap(faces);

    // even out the triangles the cut made, keeping the crease where it is
    std::vector<std::vector<size_t>> neighbors(m_vertices.size());
    for (const auto& face : m_faces) {
        for (size_t i = 0; i < face.size(); ++i) {
            neighbors[face[i]].push_back(face[(i + 1) % face.size()]);
            neighbors[face[(i + 1) % face.size()]].push_back(face[i]);
        }
    }
    for (int iteration = 0; iteration < 4; ++iteration) {
        std::vector<Vector3> relaxed = m_vertices;
        for (size_t v = 0; v < m_vertices.size(); ++v) {
            if (0.0 == side[v] || neighbors[v].empty())
                continue;
            Vector3 average;
            for (size_t n : neighbors[v])
                average += m_vertices[n];
            average /= (double)neighbors[v].size();
            Vector3 delta = (average - m_vertices[v]) * 0.5;
            Vector3 normal = sampleGradient(m_vertices[v]).normalized();
            delta -= normal * Vector3::dotProduct(delta, normal);
            Vector3 position = projectToSurface(m_vertices[v] + delta);
            // stay on the side of the crease it was on
            if ((openingSide(position) > 0.0) == (side[v] > 0.0))
                relaxed[v] = position;
        }
        m_vertices.swap(relaxed);
    }
}

bool WrapMeshBuilder::remeshQuads(size_t targetFaces)
{
    std::vector<AutoRemesher::Vector3> vertices;
    vertices.reserve(m_vertices.size());
    for (const auto& v : m_vertices)
        vertices.push_back(AutoRemesher::Vector3(v.x(), v.y(), v.z()));
    std::vector<std::vector<size_t>> triangles;
    double area = 0.0;
    double edgeSum = 0.0;
    size_t edgeCount = 0;
    for (const auto& face : m_faces) {
        for (size_t i = 1; i + 1 < face.size(); ++i) {
            triangles.push_back({ face[0], face[i], face[i + 1] });
            area += Vector3::area(m_vertices[face[0]], m_vertices[face[i]], m_vertices[face[i + 1]]);
        }
    }
    for (const auto& triangle : triangles) {
        for (size_t i = 0; i < 3; ++i) {
            edgeSum += (m_vertices[triangle[i]] - m_vertices[triangle[(i + 1) % 3]]).length();
            ++edgeCount;
        }
    }
    if (triangles.empty() || edgeCount == 0 || area <= 0.0)
        return false;
    double averageEdge = edgeSum / edgeCount;
    // the face count is for what is kept: openings remove the rest after remeshing
    double keptArea = area;
    if (hasOpenings()) {
        keptArea = 0.0;
        for (const auto& triangle : triangles) {
            Vector3 center = (m_vertices[triangle[0]] + m_vertices[triangle[1]] + m_vertices[triangle[2]]) / 3.0;
            if (openingSide(center) >= 0.0)
                keptArea += Vector3::area(m_vertices[triangle[0]], m_vertices[triangle[1]], m_vertices[triangle[2]]);
        }
        keptArea = std::max(keptArea, area * 0.05);
    }
    double targetEdge = std::sqrt(keptArea / (double)targetFaces);
    // the island targets below are shares of the whole surface
    targetFaces = (size_t)std::max(16.0, std::round(area / (targetEdge * targetEdge)));
    // the quad edge is the scaling times the average edge of the input
    double scaling = targetEdge / averageEdge;
    // creases count as features only on cloth that is cut open along them
    bool creases = m_parameters.mode == Mode::Cloth
        && (m_parameters.openTop > 0.0 || m_parameters.openBottom > 0.0 || !m_carve.empty());

    if (const char* folder = std::getenv("DUST3D_WRAP_DUMP")) {
        static int counter = 0;
        std::string path = std::string(folder) + "/remesh" + std::to_string(counter++) + "_input.obj";
        if (FILE* file = std::fopen(path.c_str(), "w")) {
            for (const auto& v : m_vertices)
                std::fprintf(file, "v %f %f %f\n", v.x(), v.y(), v.z());
            for (const auto& t : triangles)
                std::fprintf(file, "f %zu %zu %zu\n", t[0] + 1, t[1] + 1, t[2] + 1);
            std::fclose(file);
            dust3dDebug << "Wrap remesh input" << path.c_str() << "scaling" << scaling << "target" << targetFaces;
        }
    }
    // Like AutoRemesher itself, every island (a boot on each foot) is remeshed on its
    // own, on its own thread, then the islands are merged.
    std::vector<size_t> islandOf(m_vertices.size(), (size_t)-1);
    std::vector<std::vector<size_t>> vertexTriangles(m_vertices.size());
    for (size_t t = 0; t < triangles.size(); ++t) {
        for (size_t index : triangles[t])
            vertexTriangles[index].push_back(t);
    }
    size_t islandCount = 0;
    for (size_t start = 0; start < m_vertices.size(); ++start) {
        if (islandOf[start] != (size_t)-1 || vertexTriangles[start].empty())
            continue;
        std::vector<size_t> stack = { start };
        islandOf[start] = islandCount;
        while (!stack.empty()) {
            size_t v = stack.back();
            stack.pop_back();
            for (size_t t : vertexTriangles[v]) {
                for (size_t n : triangles[t]) {
                    if (islandOf[n] == (size_t)-1) {
                        islandOf[n] = islandCount;
                        stack.push_back(n);
                    }
                }
            }
        }
        ++islandCount;
    }
    struct Island {
        std::vector<AutoRemesher::Vector3> vertices;
        std::vector<std::vector<size_t>> triangles;
        double area = 0.0;
        std::vector<AutoRemesher::Vector3> resultVertices;
        std::vector<std::vector<size_t>> resultQuads;
        bool ok = false;
    };
    std::vector<Island> islands(islandCount);
    {
        std::vector<size_t> local(m_vertices.size(), (size_t)-1);
        for (size_t v = 0; v < m_vertices.size(); ++v) {
            if (islandOf[v] == (size_t)-1)
                continue;
            local[v] = islands[islandOf[v]].vertices.size();
            islands[islandOf[v]].vertices.push_back(vertices[v]);
        }
        for (const auto& triangle : triangles) {
            Island& island = islands[islandOf[triangle[0]]];
            island.triangles.push_back({ local[triangle[0]], local[triangle[1]], local[triangle[2]] });
            island.area += Vector3::area(m_vertices[triangle[0]], m_vertices[triangle[1]], m_vertices[triangle[2]]);
        }
    }
    auto closedManifold = [](const std::vector<AutoRemesher::Vector3>& vertices, const std::vector<std::vector<size_t>>& quads) {
        std::map<std::pair<size_t, size_t>, int> use;
        for (const auto& face : quads) {
            if (face.size() < 3)
                return false;
            for (size_t i = 0; i < face.size(); ++i) {
                size_t a = face[i], b = face[(i + 1) % face.size()];
                if (a >= vertices.size() || b >= vertices.size() || a == b)
                    return false;
                use[{ std::min(a, b), std::max(a, b) }]++;
            }
        }
        for (const auto& it : use) {
            if (it.second != 2)
                return false;
        }
        return !quads.empty();
    };
    auto remeshIslandWith = [&](Island& island, double sharpDegrees) {
        island.ok = false;
        size_t islandTarget = std::max((size_t)16, (size_t)std::round((double)targetFaces * island.area / area));
        // AutoRemesher's isotropic stage: even triangles a few times smaller than the
        // quads, sharp creases kept as edge chains
        AutoRemesher::IsotropicRemesher isotropic(island.vertices, island.triangles);
        isotropic.setTargetEdgeLength(targetEdge / 2.0);
        isotropic.setSharpEdgeDegrees(sharpDegrees);
        bool isotropicDone = false;
        try {
            isotropicDone = isotropic.remesh();
        } catch (...) {
            isotropicDone = false;
        }
        std::vector<AutoRemesher::Vector3> evenVertices = isotropicDone ? isotropic.remeshedVertices() : island.vertices;
        std::vector<std::vector<size_t>> evenTriangles = isotropicDone ? isotropic.remeshedTriangles() : island.triangles;
        if (evenTriangles.empty())
            return;
        double evenEdgeSum = 0.0;
        size_t evenEdgeCount = 0;
        for (const auto& triangle : evenTriangles) {
            for (size_t i = 0; i < 3; ++i) {
                evenEdgeSum += (evenVertices[triangle[i]] - evenVertices[triangle[(i + 1) % 3]]).length();
                ++evenEdgeCount;
            }
        }
        double islandScaling = targetEdge / (evenEdgeSum / std::max((size_t)1, evenEdgeCount));
        for (int attempt = 0; attempt < 2; ++attempt) {
            AutoRemesher::Parameterizer parameterizer(&evenVertices, &evenTriangles, nullptr);
            parameterizer.setScaling(islandScaling);
            parameterizer.setSharpEdgeDegrees(sharpDegrees);
            bool parameterized = false;
            try {
                parameterized = parameterizer.parameterize();
            } catch (...) {
                parameterized = false;
            }
            if (!parameterized)
                return;
            auto uvs = parameterizer.takeTriangleUvs();
            if (nullptr == uvs)
                return;
            std::vector<std::vector<AutoRemesher::Vector2>> originalUvs = parameterizer.originalTriangleUvs();
            std::vector<size_t> singularVertices = parameterizer.singularVertexIndices();
            AutoRemesher::QuadExtractor extractor(&evenVertices, &evenTriangles, uvs.get());
            extractor.setOriginalTriangleUvs(&originalUvs);
            extractor.setSingularVertices(&singularVertices);
            bool extracted = false;
            try {
                extracted = extractor.extract();
            } catch (...) {
                extracted = false;
            }
            if (!extracted || !closedManifold(extractor.remeshedVertices(), extractor.remeshedQuads())) {
                // keep an earlier attempt that was clean, if there was one
                return;
            }
            island.resultVertices = extractor.remeshedVertices();
            island.resultQuads = extractor.remeshedQuads();
            island.ok = true;
            double ratio = (double)island.resultQuads.size() / (double)islandTarget;
            if (ratio > 0.8 && ratio < 1.25)
                break;
            islandScaling *= std::sqrt(ratio);
        }
    };
    auto remeshIsland = [&](Island& island) {
        // creases as features first (edge loops along the openings); if that does not
        // give a clean closed surface, the plain cross field
        remeshIslandWith(island, creases ? 45.0 : 90.0);
        if (!island.ok && creases)
            remeshIslandWith(island, 90.0);
        if (!island.ok)
            dust3dDebug << "Wrap remesh of an island failed:" << island.triangles.size() << "triangles";
    };
    {
        std::vector<std::thread> workers;
        for (auto& island : islands)
            workers.emplace_back([&remeshIsland, &island]() { remeshIsland(island); });
        for (auto& worker : workers)
            worker.join();
    }
    std::vector<AutoRemesher::Vector3> resultVertices;
    std::vector<std::vector<size_t>> resultQuads;
    for (const auto& island : islands) {
        if (!island.ok)
            return false;
        size_t offset = resultVertices.size();
        resultVertices.insert(resultVertices.end(), island.resultVertices.begin(), island.resultVertices.end());
        for (auto quad : island.resultQuads) {
            for (auto& index : quad)
                index += offset;
            resultQuads.push_back(quad);
        }
    }

    // a quad mesh to keep: closed and manifold, like the surface it replaces
    std::map<std::pair<size_t, size_t>, int> edgeUse;
    for (const auto& face : resultQuads) {
        if (face.size() < 3)
            return false;
        for (size_t i = 0; i < face.size(); ++i) {
            size_t a = face[i], b = face[(i + 1) % face.size()];
            if (a >= resultVertices.size() || b >= resultVertices.size())
                return false;
            edgeUse[{ std::min(a, b), std::max(a, b) }]++;
        }
    }
    for (const auto& it : edgeUse) {
        if (it.second != 2) {
            dust3dDebug << "Wrap remesh is not closed and manifold, keeping the grid surface";
            return false;
        }
    }

    m_vertices.clear();
    for (const auto& v : resultVertices)
        m_vertices.push_back(projectToSurface(Vector3(v.x(), v.y(), v.z())));
    m_faces = resultQuads;
    m_cellSize = targetEdge;
    if (hasOpenings()) {
        for (auto& position : m_vertices) {
            if (std::abs(openingSide(position)) < 0.2 * targetEdge)
                position = projectToCrease(position);
        }
    }
    m_vertexOnOpening.assign(m_vertices.size(), false);
    return true;
}

Vector3 WrapMeshBuilder::projectToSurface(const Vector3& position) const
{
    Vector3 result = position;
    for (int iteration = 0; iteration < 4; ++iteration) {
        double value = sampleField(result);
        if (std::abs(value) < m_grid.h * 1e-3)
            break;
        Vector3 gradient = sampleGradient(result);
        double length2 = gradient.lengthSquared();
        if (length2 < 1e-12)
            break;
        Vector3 step = gradient * (value / length2);
        double stepLength = step.length();
        if (stepLength > m_grid.h)
            step *= m_grid.h / stepLength;
        result -= step;
    }
    return result;
}

void WrapMeshBuilder::extractSurface()
{
    const Grid& grid = m_grid;
    m_vertices.clear();
    m_faces.clear();
    const int cx = grid.nx - 1, cy = grid.ny - 1, cz = grid.nz - 1;
    std::vector<int> cellVertex((size_t)cx * cy * cz, -1);
    auto cellIndex = [&](int i, int j, int k) { return ((size_t)k * cy + j) * cx + i; };
    static const int corners[8][3] = { { 0, 0, 0 }, { 1, 0, 0 }, { 0, 1, 0 }, { 1, 1, 0 }, { 0, 0, 1 }, { 1, 0, 1 }, { 0, 1, 1 }, { 1, 1, 1 } };
    static const int edges[12][2] = { { 0, 1 }, { 2, 3 }, { 4, 5 }, { 6, 7 }, { 0, 2 }, { 1, 3 }, { 4, 6 }, { 5, 7 }, { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 } };
    for (int k = 0; k < cz; ++k) {
        for (int j = 0; j < cy; ++j) {
            for (int i = 0; i < cx; ++i) {
                float values[8];
                int insideCount = 0;
                for (int c = 0; c < 8; ++c) {
                    values[c] = grid.values[grid.index(i + corners[c][0], j + corners[c][1], k + corners[c][2])];
                    if (values[c] < 0.0f)
                        ++insideCount;
                }
                if (insideCount == 0 || insideCount == 8)
                    continue;
                Vector3 sum;
                int crossings = 0;
                for (const auto& edge : edges) {
                    float a = values[edge[0]];
                    float b = values[edge[1]];
                    if ((a < 0.0f) == (b < 0.0f))
                        continue;
                    double t = a / (a - b);
                    Vector3 pa = grid.position(i + corners[edge[0]][0], j + corners[edge[0]][1], k + corners[edge[0]][2]);
                    Vector3 pb = grid.position(i + corners[edge[1]][0], j + corners[edge[1]][1], k + corners[edge[1]][2]);
                    sum += pa + (pb - pa) * t;
                    ++crossings;
                }
                cellVertex[cellIndex(i, j, k)] = (int)m_vertices.size();
                m_vertices.push_back(sum / (double)crossings);
            }
        }
    }
    // one quad around every grid edge that crosses the surface
    for (int axis = 0; axis < 3; ++axis) {
        int u = (axis + 1) % 3;
        int v = (axis + 2) % 3;
        int n[3] = { grid.nx, grid.ny, grid.nz };
        for (int k = 0; k < n[2]; ++k) {
            for (int j = 0; j < n[1]; ++j) {
                for (int i = 0; i < n[0]; ++i) {
                    int p[3] = { i, j, k };
                    if (p[axis] >= n[axis] - 1)
                        continue;
                    if (p[u] < 1 || p[u] > n[u] - 2 || p[v] < 1 || p[v] > n[v] - 2)
                        continue;
                    int q[3] = { i, j, k };
                    q[axis] += 1;
                    bool insideA = grid.values[grid.index(p[0], p[1], p[2])] < 0.0f;
                    bool insideB = grid.values[grid.index(q[0], q[1], q[2])] < 0.0f;
                    if (insideA == insideB)
                        continue;
                    const int offsets[4][2] = { { -1, -1 }, { 0, -1 }, { 0, 0 }, { -1, 0 } };
                    std::vector<size_t> face;
                    bool valid = true;
                    for (const auto& offset : offsets) {
                        int c[3] = { i, j, k };
                        c[u] += offset[0];
                        c[v] += offset[1];
                        int vertex = cellVertex[cellIndex(c[0], c[1], c[2])];
                        if (vertex < 0) {
                            valid = false;
                            break;
                        }
                        face.push_back((size_t)vertex);
                    }
                    if (!valid)
                        continue;
                    // the order above winds counter-clockwise around +axis
                    if (!insideA)
                        std::reverse(face.begin(), face.end());
                    m_faces.push_back(face);
                }
            }
        }
    }
    for (auto& vertex : m_vertices)
        vertex = projectToSurface(vertex);
}

void WrapMeshBuilder::openSurface()
{
    m_vertexOnOpening.assign(m_vertices.size(), false);
    bool cloth = m_parameters.mode == Mode::Cloth;
    bool top = cloth && m_parameters.openTop > 0.0;
    bool bottom = cloth && m_parameters.openBottom > 0.0;
    bool carved = cloth && !m_carve.empty();
    if (!top && !bottom && !carved)
        return;
    if (m_remeshed) {
        clipOpenings();
        return;
    }
    double tolerance = m_cellSize * 0.35;
    std::vector<std::vector<size_t>> kept;
    for (const auto& face : m_faces) {

        if (carved) {
            // on the surface of a carving child: an opening
            bool onCarving = true;
            for (size_t index : face) {
                if (std::abs(sampleCarve(m_vertices[index])) > m_cellSize * 0.45) {
                    onCarving = false;
                    break;
                }
            }
            if (onCarving)
                continue;
        }
        Vector3 normal;
        for (size_t i = 1; i + 1 < face.size(); ++i)
            normal += Vector3::crossProduct(m_vertices[face[i]] - m_vertices[face[0]], m_vertices[face[i + 1]] - m_vertices[face[0]]);
        normal.normalize();
        bool onTop = top && normal.y() > 0.7;
        bool onBottom = bottom && normal.y() < -0.7;
        for (size_t index : face) {
            if (onTop && m_vertices[index].y() < m_openTopY - tolerance)
                onTop = false;
            if (onBottom && m_vertices[index].y() > m_openBottomY + tolerance)
                onBottom = false;
        }
        if (onTop || onBottom)
            continue;
        kept.push_back(face);
    }
    if (kept.size() == m_faces.size())
        return;
    m_faces.swap(kept);

    // drop the vertices nothing uses any more
    std::vector<int> remap(m_vertices.size(), -1);
    std::vector<Vector3> vertices;
    for (auto& face : m_faces) {
        for (auto& index : face) {
            if (remap[index] < 0) {
                remap[index] = (int)vertices.size();
                vertices.push_back(m_vertices[index]);
            }
            index = (size_t)remap[index];
        }
    }
    m_vertices.swap(vertices);

    std::map<std::pair<size_t, size_t>, int> edgeUse;
    for (const auto& face : m_faces) {
        for (size_t i = 0; i < face.size(); ++i) {
            size_t a = face[i], b = face[(i + 1) % face.size()];
            edgeUse[{ std::min(a, b), std::max(a, b) }]++;
        }
    }
    m_vertexOnOpening.assign(m_vertices.size(), false);
    for (const auto& it : edgeUse) {
        if (it.second == 1) {
            m_vertexOnOpening[it.first.first] = true;
            m_vertexOnOpening[it.first.second] = true;
        }
    }
}

void WrapMeshBuilder::clipOpenings()
{
    // The remeshed surface is closed: cut away what the openings remove, exactly along
    // their creases. A face the crease crosses keeps its part on the garment side (a quad
    // cut across stays a quad); where the crease passes close to a vertex, the vertex is
    // moved onto it instead, so no slivers are made. Where the remesher put an edge loop
    // on the crease, nothing needs cutting.
    std::vector<double> side(m_vertices.size());
    for (size_t v = 0; v < m_vertices.size(); ++v)
        side[v] = openingSide(m_vertices[v]);
    std::vector<bool> moved(m_vertices.size(), false);
    for (const auto& face : m_faces) {
        for (size_t i = 0; i < face.size(); ++i) {
            size_t a = face[i], b = face[(i + 1) % face.size()];
            if (side[a] == 0.0 || side[b] == 0.0 || (side[a] > 0.0) == (side[b] > 0.0) || moved[a] || moved[b])
                continue;
            double t = side[a] / (side[a] - side[b]);
            size_t near = t < 0.25 ? a : (t > 0.75 ? b : (size_t)-1);
            if (near == (size_t)-1)
                continue;
            m_vertices[near] = projectToCrease(m_vertices[near]);
            side[near] = 0.0;
            moved[near] = true;
        }
    }
    std::map<std::pair<size_t, size_t>, size_t> crossings;
    auto crossing = [&](size_t a, size_t b) {
        auto key = std::make_pair(std::min(a, b), std::max(a, b));
        auto found = crossings.find(key);
        if (found != crossings.end())
            return found->second;
        double t = side[a] / (side[a] - side[b]);
        size_t index = m_vertices.size();
        m_vertices.push_back(projectToCrease(m_vertices[a] + (m_vertices[b] - m_vertices[a]) * t));
        side.push_back(0.0);
        crossings[key] = index;
        return index;
    };
    std::vector<std::vector<size_t>> faces;
    for (const auto& face : m_faces) {
        bool anyKept = false, anyRemoved = false;
        for (size_t index : face) {
            if (side[index] > 0.0)
                anyKept = true;
            else if (side[index] < 0.0)
                anyRemoved = true;
        }
        if (!anyKept)
            continue;
        if (!anyRemoved) {
            faces.push_back(face);
            continue;
        }
        std::vector<size_t> clipped;
        for (size_t i = 0; i < face.size(); ++i) {
            size_t a = face[i], b = face[(i + 1) % face.size()];
            if (side[a] >= 0.0)
                clipped.push_back(a);
            if (side[a] != 0.0 && side[b] != 0.0 && (side[a] > 0.0) != (side[b] > 0.0))
                clipped.push_back(crossing(a, b));
        }
        if (clipped.size() < 3)
            continue;
        if (clipped.size() <= 4) {
            faces.push_back(clipped);
            continue;
        }
        // five or six corners: a quad and what is left, from a corner on the garment side
        size_t start = 0;
        while (start < clipped.size() && side[clipped[start]] == 0.0)
            ++start;
        if (start == clipped.size())
            start = 0;
        std::rotate(clipped.begin(), clipped.begin() + start, clipped.end());
        faces.push_back({ clipped[0], clipped[1], clipped[2], clipped[3] });
        std::vector<size_t> rest = { clipped[0] };
        rest.insert(rest.end(), clipped.begin() + 3, clipped.end());
        faces.push_back(rest);
    }
    m_faces.swap(faces);

    std::vector<int> remap(m_vertices.size(), -1);
    std::vector<Vector3> vertices;
    for (auto& face : m_faces) {
        for (auto& index : face) {
            if (remap[index] < 0) {
                remap[index] = (int)vertices.size();
                vertices.push_back(m_vertices[index]);
            }
            index = (size_t)remap[index];
        }
    }
    m_vertices.swap(vertices);
    std::map<std::pair<size_t, size_t>, int> edgeUse;
    for (const auto& face : m_faces) {
        for (size_t i = 0; i < face.size(); ++i) {
            size_t a = face[i], b = face[(i + 1) % face.size()];
            edgeUse[{ std::min(a, b), std::max(a, b) }]++;
        }
    }
    m_vertexOnOpening.assign(m_vertices.size(), false);
    for (const auto& it : edgeUse) {
        if (it.second == 1) {
            m_vertexOnOpening[it.first.first] = true;
            m_vertexOnOpening[it.first.second] = true;
        }
    }
}

void WrapMeshBuilder::relaxSurface(bool interior)
{
    std::vector<std::vector<size_t>> neighbors(m_vertices.size());
    std::vector<std::vector<size_t>> openingNeighbors(m_vertices.size());
    std::map<std::pair<size_t, size_t>, int> edgeUse;
    for (const auto& face : m_faces) {
        for (size_t i = 0; i < face.size(); ++i) {
            size_t a = face[i], b = face[(i + 1) % face.size()];
            edgeUse[{ std::min(a, b), std::max(a, b) }]++;
        }
    }
    for (const auto& it : edgeUse) {
        neighbors[it.first.first].push_back(it.first.second);
        neighbors[it.first.second].push_back(it.first.first);
        if (it.second == 1) {
            openingNeighbors[it.first.first].push_back(it.first.second);
            openingNeighbors[it.first.second].push_back(it.first.first);
        }
    }
    double tolerance = m_cellSize;
    for (int iteration = 0; interior && iteration < m_parameters.relaxIterations; ++iteration) {
        std::vector<Vector3> relaxed = m_vertices;
        for (size_t v = 0; v < m_vertices.size(); ++v) {
            bool opening = v < m_vertexOnOpening.size() && m_vertexOnOpening[v];
            const auto& around = opening ? openingNeighbors[v] : neighbors[v];
            if (around.empty())
                continue;
            Vector3 average;
            for (size_t n : around)
                average += m_vertices[n];
            average /= (double)around.size();
            Vector3 delta = (average - m_vertices[v]) * 0.5;
            Vector3 normal = sampleGradient(m_vertices[v]).normalized();
            delta -= normal * Vector3::dotProduct(delta, normal);
            Vector3 position = projectToSurface(m_vertices[v] + delta);
            if (opening) {
                // keep the rim of an opening on its plane
                bool top = m_parameters.openTop > 0.0 && std::abs(position.y() - m_openTopY) < tolerance;
                bool bottom = m_parameters.openBottom > 0.0 && std::abs(position.y() - m_openBottomY) < tolerance;
                if (top)
                    position.setY(m_openTopY);
                else if (bottom)
                    position.setY(m_openBottomY);
                else if (!m_carve.empty() && std::abs(sampleCarve(position)) < 2.0 * m_cellSize) {
                    // the rim of a cut-out lies where the cloth meets the carving shape
                    for (int alternation = 0; alternation < 3; ++alternation)
                        position = projectToSurface(projectToCarve(position));
                }
            }
            relaxed[v] = position;
        }
        m_vertices.swap(relaxed);
    }

    // The rims of openings follow the grid cells they were cut through: smooth them as
    // curves (on the surface, and on their plane or carving shape), then let the faces
    // next to them settle.
    bool anyOpening = std::find(m_vertexOnOpening.begin(), m_vertexOnOpening.end(), true) != m_vertexOnOpening.end();
    if (!anyOpening)
        return;
    for (int iteration = 0; iteration < 12; ++iteration) {
        std::vector<Vector3> relaxed = m_vertices;
        for (size_t v = 0; v < m_vertices.size(); ++v) {
            if (!m_vertexOnOpening[v] || openingNeighbors[v].size() != 2)
                continue;
            Vector3 average = (m_vertices[openingNeighbors[v][0]] + m_vertices[openingNeighbors[v][1]]) * 0.5;
            Vector3 position = projectToSurface(m_vertices[v] + (average - m_vertices[v]) * 0.6);
            bool top = m_parameters.openTop > 0.0 && std::abs(position.y() - m_openTopY) < tolerance;
            bool bottom = m_parameters.openBottom > 0.0 && std::abs(position.y() - m_openBottomY) < tolerance;
            if (top)
                position.setY(m_openTopY);
            else if (bottom)
                position.setY(m_openBottomY);
            else if (!m_carve.empty() && std::abs(sampleCarve(position)) < 2.0 * m_cellSize) {
                for (int alternation = 0; alternation < 2; ++alternation)
                    position = projectToSurface(projectToCarve(position));
            }
            relaxed[v] = position;
        }
        m_vertices.swap(relaxed);
    }
    for (int iteration = 0; interior && iteration < 2; ++iteration) {
        std::vector<Vector3> relaxed = m_vertices;
        for (size_t v = 0; v < m_vertices.size(); ++v) {
            if (m_vertexOnOpening[v] || neighbors[v].empty())
                continue;
            Vector3 average;
            for (size_t n : neighbors[v])
                average += m_vertices[n];
            average /= (double)neighbors[v].size();
            Vector3 delta = (average - m_vertices[v]) * 0.5;
            Vector3 normal = sampleGradient(m_vertices[v]).normalized();
            delta -= normal * Vector3::dotProduct(delta, normal);
            relaxed[v] = projectToSurface(m_vertices[v] + delta);
        }
        m_vertices.swap(relaxed);
    }
}

void WrapMeshBuilder::addThickness()
{
    size_t outerCount = m_vertices.size();
    m_vertexSourceVertex.resize(outerCount);
    for (size_t v = 0; v < outerCount; ++v)
        m_vertexSourceVertex[v] = v;
    double thickness = m_parameters.thickness;
    if (m_parameters.mode != Mode::Cloth || thickness <= 0.0)
        return;
    bool hasOpening = std::find(m_vertexOnOpening.begin(), m_vertexOnOpening.end(), true) != m_vertexOnOpening.end();
    if (!hasOpening)
        return;

    std::vector<Vector3> normals(outerCount);
    for (const auto& face : m_faces) {
        Vector3 normal;
        for (size_t i = 1; i + 1 < face.size(); ++i)
            normal += Vector3::crossProduct(m_vertices[face[i]] - m_vertices[face[0]], m_vertices[face[i + 1]] - m_vertices[face[0]]);
        for (size_t index : face)
            normals[index] += normal;
    }

    // The inner shell is only a lip around the openings (a hem): further in, the inside of
    // the garment is never seen, so it would only cost triangles.
    const int lipRings = 2;
    std::vector<std::vector<size_t>> neighbors(outerCount);
    std::map<std::pair<size_t, size_t>, int> edgeUse;
    for (const auto& face : m_faces) {
        for (size_t i = 0; i < face.size(); ++i) {
            size_t a = face[i], b = face[(i + 1) % face.size()];
            if (0 == edgeUse[{ std::min(a, b), std::max(a, b) }]++) {
                neighbors[a].push_back(b);
                neighbors[b].push_back(a);
            }
        }
    }
    std::vector<int> ring(outerCount, -1);
    std::queue<size_t> queue;
    for (size_t v = 0; v < outerCount; ++v) {
        if (m_vertexOnOpening[v]) {
            ring[v] = 0;
            queue.push(v);
        }
    }
    while (!queue.empty()) {
        size_t v = queue.front();
        queue.pop();
        if (ring[v] >= lipRings)
            continue;
        for (size_t n : neighbors[v]) {
            if (ring[n] < 0) {
                ring[n] = ring[v] + 1;
                queue.push(n);
            }
        }
    }
    std::vector<int> inner(outerCount, -1);
    auto innerVertex = [&](size_t v) {
        if (inner[v] < 0) {
            Vector3 normal = normals[v].normalized();
            if (m_vertexOnOpening[v]) {
                // the rim of a plane opening stays on its plane
                const Vector3& p = m_vertices[v];
                bool onPlane = (m_parameters.openTop > 0.0 && std::abs(p.y() - m_openTopY) < m_cellSize * 0.5)
                    || (m_parameters.openBottom > 0.0 && std::abs(p.y() - m_openBottomY) < m_cellSize * 0.5);
                if (onPlane) {
                    normal.setY(0.0);
                    normal.normalize();
                }
            }
            inner[v] = (int)m_vertices.size();
            m_vertices.push_back(m_vertices[v] - normal * thickness);
            m_vertexSourceVertex.push_back(v);
        }
        return (size_t)inner[v];
    };
    std::vector<std::vector<size_t>> innerFaces;
    for (const auto& face : m_faces) {
        bool inLip = true;
        for (size_t index : face) {
            if (ring[index] < 0) {
                inLip = false;
                break;
            }
        }
        if (!inLip)
            continue;
        std::vector<size_t> innerFace;
        for (auto it = face.rbegin(); it != face.rend(); ++it)
            innerFace.push_back(innerVertex(*it));
        innerFaces.push_back(innerFace);
    }
    std::vector<std::vector<size_t>> rimFaces;
    for (const auto& face : m_faces) {
        for (size_t i = 0; i < face.size(); ++i) {
            size_t a = face[i], b = face[(i + 1) % face.size()];
            if (edgeUse[{ std::min(a, b), std::max(a, b) }] != 1)
                continue;
            rimFaces.push_back({ b, a, innerVertex(a), innerVertex(b) });
        }
    }
    m_faces.insert(m_faces.end(), innerFaces.begin(), innerFaces.end());
    m_faces.insert(m_faces.end(), rimFaces.begin(), rimFaces.end());
}

void WrapMeshBuilder::triangulate()
{
    m_resultVertices = m_vertices;
    m_resultTriangles.clear();
    m_resultTriangleAndQuads.clear();
    m_resultQuadDiagonals.clear();
    for (const auto& face : m_faces) {
        if (face.size() == 4) {
            double d02 = (m_vertices[face[0]] - m_vertices[face[2]]).lengthSquared();
            double d13 = (m_vertices[face[1]] - m_vertices[face[3]]).lengthSquared();
            if (d02 <= d13) {
                m_resultTriangles.push_back({ face[0], face[1], face[2] });
                m_resultTriangles.push_back({ face[2], face[3], face[0] });
                m_resultQuadDiagonals.push_back({ face[0], face[2] });
            } else {
                m_resultTriangles.push_back({ face[1], face[2], face[3] });
                m_resultTriangles.push_back({ face[3], face[0], face[1] });
                m_resultQuadDiagonals.push_back({ face[1], face[3] });
            }
            m_resultTriangleAndQuads.push_back(face);
            continue;
        }
        for (size_t i = 1; i + 1 < face.size(); ++i)
            m_resultTriangles.push_back({ face[0], face[i], face[i + 1] });
        m_resultTriangleAndQuads.push_back(face);
    }
}

void WrapMeshBuilder::transferWeights()
{
    m_resultVertexNodeWeights.assign(m_resultVertices.size(), {});
    if (m_bindSamples.empty())
        return;
    size_t outerCount = 0;
    for (size_t v = 0; v < m_vertexSourceVertex.size(); ++v) {
        if (m_vertexSourceVertex[v] == v)
            outerCount = v + 1;
    }

    // The blend radius is a distance in the world, not a number of rings of the new
    // surface: a body and the garments over it get the same weights at the same place,
    // whatever their resolutions, so they bend together and the body stays inside.
    const bool cloth = m_parameters.mode == Mode::Cloth;
    double radius = m_parameters.weightRadius;
    if (radius <= 0.0)
        radius = 0.03;
    radius = std::max(radius, 1.0 * m_cellSize);
    int smoothIterations = m_parameters.weightSmoothIterations;
    if (smoothIterations <= 0) {
        // only cloth hanging away from the body (drape) is evened out over the surface
        smoothIterations = cloth ? 1 + (int)std::round(3.0 * m_parameters.drape) : 1;
    }

    // spatial hash of the samples
    double cell = std::max(radius, m_cellSize * 2.0);
    struct Key {
        int x, y, z;
        bool operator==(const Key& o) const { return x == o.x && y == o.y && z == o.z; }
    };
    struct KeyHash {
        size_t operator()(const Key& k) const { return ((size_t)(k.x * 73856093) ^ (size_t)(k.y * 19349663) ^ (size_t)(k.z * 83492791)); }
    };
    std::unordered_map<Key, std::vector<size_t>, KeyHash> buckets;
    auto keyOf = [&](const Vector3& p) { return Key { (int)std::floor(p.x() / cell), (int)std::floor(p.y() / cell), (int)std::floor(p.z() / cell) }; };
    for (size_t s = 0; s < m_bindSamples.size(); ++s)
        buckets[keyOf(m_bindSamples[s].first)].push_back(s);

    // node graph: edges inside a part, and touching node spheres of different parts
    // (except a part and its mirror twin, which meet only through what is between them)
    std::map<Uuid, size_t> nodeIndex;
    for (size_t n = 0; n < m_bindNodes.size(); ++n)
        nodeIndex[m_bindNodes[n].id] = n;
    std::vector<std::vector<size_t>> nodeLinks(m_bindNodes.size());
    auto link = [&](size_t a, size_t b) {
        if (a == b)
            return;
        nodeLinks[a].push_back(b);
        nodeLinks[b].push_back(a);
    };
    for (const auto& it : m_bindLinks) {
        auto a = nodeIndex.find(it.first);
        auto b = nodeIndex.find(it.second);
        if (a != nodeIndex.end() && b != nodeIndex.end())
            link(a->second, b->second);
    }
    for (size_t a = 0; a < m_bindNodes.size(); ++a) {
        for (size_t b = a + 1; b < m_bindNodes.size(); ++b) {
            const auto& na = m_bindNodes[a];
            const auto& nb = m_bindNodes[b];
            if (na.group == nb.group || na.twinGroup == nb.twinGroup)
                continue;
            if ((na.position - nb.position).length() < na.radius + nb.radius + 0.002)
                link(a, b);
        }
    }
    const int maxHops = 2;
    std::map<Uuid, std::set<Uuid>> reachableCache;
    auto reachable = [&](const Uuid& start) -> const std::set<Uuid>* {
        auto found = nodeIndex.find(start);
        if (found == nodeIndex.end())
            return nullptr;
        auto cached = reachableCache.find(start);
        if (cached != reachableCache.end())
            return &cached->second;
        std::set<Uuid>& result = reachableCache[start];
        std::vector<int> hops(m_bindNodes.size(), -1);
        std::queue<size_t> frontier;
        hops[found->second] = 0;
        frontier.push(found->second);
        while (!frontier.empty()) {
            size_t n = frontier.front();
            frontier.pop();
            result.insert(m_bindNodes[n].id);
            if (hops[n] >= maxHops)
                continue;
            for (size_t m : nodeLinks[n]) {
                if (hops[m] < 0) {
                    hops[m] = hops[n] + 1;
                    frontier.push(m);
                }
            }
        }
        return &result;
    };

    for (size_t v = 0; v < outerCount; ++v) {
        const Vector3& position = m_resultVertices[v];
        Key center = keyOf(position);
        double best2 = std::numeric_limits<double>::max();
        size_t bestSample = 0;
        for (int ring = 0; ring < 64; ++ring) {
            if (best2 < std::numeric_limits<double>::max()) {
                double reach = (ring - 1) * cell;
                if (reach > 0.0 && reach * reach > best2)
                    break;
            }
            for (int dz = -ring; dz <= ring; ++dz) {
                for (int dy = -ring; dy <= ring; ++dy) {
                    for (int dx = -ring; dx <= ring; ++dx) {
                        if (std::max(std::abs(dx), std::max(std::abs(dy), std::abs(dz))) != ring)
                            continue;
                        auto found = buckets.find(Key { center.x + dx, center.y + dy, center.z + dz });
                        if (found == buckets.end())
                            continue;
                        for (size_t s : found->second) {
                            double d2 = (m_bindSamples[s].first - position).lengthSquared();
                            if (d2 < best2) {
                                best2 = d2;
                                bestSample = s;
                            }
                        }
                    }
                }
            }
        }
        if (best2 == std::numeric_limits<double>::max())
            continue;
        double best = std::sqrt(best2);
        double limit = best + radius;
        int reach = (int)std::ceil(limit / cell);
        auto& weights = m_resultVertexNodeWeights[v];
        const std::set<Uuid>* allowed = reachable(m_bindSamples[bestSample].second);
        for (int dz = -reach; dz <= reach; ++dz) {
            for (int dy = -reach; dy <= reach; ++dy) {
                for (int dx = -reach; dx <= reach; ++dx) {
                    auto found = buckets.find(Key { center.x + dx, center.y + dy, center.z + dz });
                    if (found == buckets.end())
                        continue;
                    for (size_t s : found->second) {
                        double d = (m_bindSamples[s].first - position).length();
                        if (d > limit)
                            continue;
                        if (nullptr != allowed && allowed->find(m_bindSamples[s].second) == allowed->end())
                            continue;
                        double t = 1.0 - (d - best) / radius;
                        addWeight(weights, m_bindSamples[s].second, (float)(t * t));
                    }
                }
            }
        }
        normalizeWeights(weights, MaxNodeInfluences * 2);
    }

    // smooth over the new surface, so the wrap bends evenly at the joints
    std::vector<std::vector<size_t>> neighbors(outerCount);
    for (const auto& face : m_faces) {
        for (size_t i = 0; i < face.size(); ++i) {
            size_t a = face[i], b = face[(i + 1) % face.size()];
            if (a >= outerCount || b >= outerCount)
                continue;
            neighbors[a].push_back(b);
        }
    }
    for (int iteration = 0; iteration < smoothIterations; ++iteration) {
        std::vector<std::vector<NodeWeight>> smoothed(outerCount);
        for (size_t v = 0; v < outerCount; ++v) {
            auto& result = smoothed[v];
            for (const auto& w : m_resultVertexNodeWeights[v])
                addWeight(result, w.nodeId, w.weight * 0.5f);
            if (neighbors[v].empty()) {
                for (auto& w : result)
                    w.weight *= 2.0f;
                continue;
            }
            float share = 0.5f / (float)neighbors[v].size();
            for (size_t n : neighbors[v]) {
                for (const auto& w : m_resultVertexNodeWeights[n])
                    addWeight(result, w.nodeId, w.weight * share);
            }
        }
        for (auto& weights : smoothed)
            normalizeWeights(weights, MaxNodeInfluences * 2);
        m_resultVertexNodeWeights.swap(smoothed);
        m_resultVertexNodeWeights.resize(m_resultVertices.size());
    }
    for (auto& weights : m_resultVertexNodeWeights)
        normalizeWeights(weights, MaxNodeInfluences);
    for (size_t v = outerCount; v < m_resultVertices.size(); ++v)
        m_resultVertexNodeWeights[v] = m_resultVertexNodeWeights[m_vertexSourceVertex[v]];
}

void WrapMeshBuilder::generateUvs()
{
    // Box projection: every triangle goes to the axis its normal faces most, connected
    // triangles of the same axis form a chart, and the charts are packed into [0, 1].
    const auto& vertices = m_resultVertices;
    const auto& triangles = m_resultTriangles;
    m_resultTriangleUvs.assign(triangles.size(), { Vector2(), Vector2(), Vector2() });
    if (triangles.empty())
        return;

    std::vector<Vector3> normals(triangles.size());
    std::vector<int> labels(triangles.size());
    for (size_t t = 0; t < triangles.size(); ++t) {
        const auto& tri = triangles[t];
        normals[t] = Vector3::normal(vertices[tri[0]], vertices[tri[1]], vertices[tri[2]]);
        int axis = 0;
        for (int a = 1; a < 3; ++a) {
            if (std::abs(normals[t][a]) > std::abs(normals[t][axis]))
                axis = a;
        }
        labels[t] = axis * 2 + (normals[t][axis] < 0.0 ? 1 : 0);
    }
    std::map<std::pair<size_t, size_t>, std::vector<size_t>> edgeTriangles;
    for (size_t t = 0; t < triangles.size(); ++t) {
        for (size_t i = 0; i < 3; ++i) {
            size_t a = triangles[t][i], b = triangles[t][(i + 1) % 3];
            edgeTriangles[{ std::min(a, b), std::max(a, b) }].push_back(t);
        }
    }
    std::vector<std::vector<size_t>> adjacency(triangles.size());
    for (const auto& it : edgeTriangles) {
        if (it.second.size() != 2)
            continue;
        adjacency[it.second[0]].push_back(it.second[1]);
        adjacency[it.second[1]].push_back(it.second[0]);
    }
    // remove specks: a triangle that disagrees with most of its neighbors joins them
    for (int pass = 0; pass < 3; ++pass) {
        std::vector<int> updated = labels;
        for (size_t t = 0; t < triangles.size(); ++t) {
            std::map<int, int> counts;
            for (size_t n : adjacency[t])
                counts[labels[n]]++;
            for (const auto& it : counts) {
                if (it.first != labels[t] && it.second >= 2) {
                    int axis = it.first / 2;
                    double sign = (it.first % 2) ? -1.0 : 1.0;
                    if (normals[t][axis] * sign > 0.2)
                        updated[t] = it.first;
                }
            }
        }
        labels.swap(updated);
    }

    std::vector<int> chartOfTriangle(triangles.size(), -1);
    std::vector<std::vector<size_t>> charts;
    for (size_t t = 0; t < triangles.size(); ++t) {
        if (chartOfTriangle[t] >= 0)
            continue;
        int chart = (int)charts.size();
        charts.push_back({});
        std::queue<size_t> queue;
        queue.push(t);
        chartOfTriangle[t] = chart;
        while (!queue.empty()) {
            size_t current = queue.front();
            queue.pop();
            charts[chart].push_back(current);
            for (size_t n : adjacency[current]) {
                if (chartOfTriangle[n] >= 0 || labels[n] != labels[t])
                    continue;
                chartOfTriangle[n] = chart;
                queue.push(n);
            }
        }
    }

    auto project = [&](const Vector3& p, int label) {
        int axis = label / 2;
        bool negative = label % 2;
        int u = (axis + 1) % 3;
        int v = (axis + 2) % 3;
        return Vector2(negative ? -p[u] : p[u], p[v]);
    };
    std::vector<std::pair<float, float>> chartSizes;
    std::vector<std::pair<Vector2, Vector2>> chartBounds;
    for (const auto& chart : charts) {
        int label = labels[chart[0]];
        Vector2 lower(std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
        Vector2 upper(std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest());
        for (size_t t : chart) {
            for (size_t i = 0; i < 3; ++i) {
                Vector2 uv = project(vertices[triangles[t][i]], label);
                lower = Vector2(std::min(lower.x(), uv.x()), std::min(lower.y(), uv.y()));
                upper = Vector2(std::max(upper.x(), uv.x()), std::max(upper.y(), uv.y()));
            }
        }
        chartBounds.push_back({ lower, upper });
        chartSizes.push_back({ (float)std::max(1e-4, upper.x() - lower.x()), (float)std::max(1e-4, upper.y() - lower.y()) });
    }
    ChartPacker packer;
    packer.setCharts(chartSizes);
    float textureSize = packer.pack();
    const auto& packed = packer.getResult();
    if (packed.size() != charts.size() || textureSize <= 0.0f)
        return;
    for (size_t c = 0; c < charts.size(); ++c) {
        int label = labels[charts[c][0]];
        float left = std::get<0>(packed[c]);
        float top = std::get<1>(packed[c]);
        float width = std::get<2>(packed[c]);
        float height = std::get<3>(packed[c]);
        bool rotated = std::get<4>(packed[c]);
        const auto& lower = chartBounds[c].first;
        double chartWidth = chartSizes[c].first;
        double chartHeight = chartSizes[c].second;
        for (size_t t : charts[c]) {
            for (size_t i = 0; i < 3; ++i) {
                Vector2 uv = project(vertices[triangles[t][i]], label);
                double s = (uv.x() - lower.x()) / chartWidth;
                double r = (uv.y() - lower.y()) / chartHeight;
                if (rotated)
                    m_resultTriangleUvs[t][i] = Vector2(left + r * height, top + s * width);
                else
                    m_resultTriangleUvs[t][i] = Vector2(left + s * width, top + r * height);
            }
        }
    }
}

}
