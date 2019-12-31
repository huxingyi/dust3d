#include <simpleuv/triangulate.h>
#include <Eigen/Dense>
#include <cmath>

namespace simpleuv
{

static Eigen::Vector3d norm(const Eigen::Vector3d &p1, const Eigen::Vector3d &p2, const Eigen::Vector3d &p3)
{
    auto side1 = p2 - p1;
    auto side2 = p3 - p1;
    auto perp = side1.cross(side2);
    return perp.normalized();
}

static float angle360(const Eigen::Vector3d &a, const Eigen::Vector3d &b, const Eigen::Vector3d &direct)
{
    auto angle = atan2((a.cross(b)).norm(), a.dot(b)) * 180.0 / 3.1415926;
    auto c = a.cross(b);
    if (c.dot(direct) < 0) {
        angle = 360 - angle;
    }
    return angle;
}

static Eigen::Vector3d vertexToEigenVector3d(const Vertex &vertex)
{
    return Eigen::Vector3d(vertex.xyz[0], vertex.xyz[1], vertex.xyz[2]);
}

static bool pointInTriangle(const Eigen::Vector3d &a, const Eigen::Vector3d &b, const Eigen::Vector3d &c, const Eigen::Vector3d &p)
{
    auto u = b - a;
    auto v = c - a;
    auto w = p - a;
    auto vXw = v.cross(w);
    auto vXu = v.cross(u);
    if (vXw.dot(vXu) < 0.0) {
        return false;
    }
    auto uXw = u.cross(w);
    auto uXv = u.cross(v);
    if (uXw.dot(uXv) < 0.0) {
        return false;
    }
    auto denom = uXv.norm();
    auto r = vXw.norm() / denom;
    auto t = uXw.norm() / denom;
    return r + t <= 1.0;
}

static Eigen::Vector3d ringNorm(const std::vector<Vertex> &vertices, const std::vector<size_t> &ring)
{
    Eigen::Vector3d normal(0.0, 0.0, 0.0);
    for (size_t i = 0; i < ring.size(); ++i) {
        auto j = (i + 1) % ring.size();
        auto k = (i + 2) % ring.size();
        const auto &enter = vertexToEigenVector3d(vertices[ring[i]]);
        const auto &cone = vertexToEigenVector3d(vertices[ring[j]]);
        const auto &leave = vertexToEigenVector3d(vertices[ring[k]]);
        normal += norm(enter, cone, leave);
    }
    return normal.normalized();
}

void triangulate(const std::vector<Vertex> &vertices, std::vector<Face> &faces, const std::vector<size_t> &ring)
{
    if (ring.size() < 3)
        return;
    std::vector<size_t> fillRing = ring;
    Eigen::Vector3d direct = ringNorm(vertices, fillRing);
    while (fillRing.size() > 3) {
        bool newFaceGenerated = false;
        for (decltype(fillRing.size()) i = 0; i < fillRing.size(); ++i) {
            auto j = (i + 1) % fillRing.size();
            auto k = (i + 2) % fillRing.size();
            const auto &enter = vertexToEigenVector3d(vertices[fillRing[i]]);
            const auto &cone = vertexToEigenVector3d(vertices[fillRing[j]]);
            const auto &leave = vertexToEigenVector3d(vertices[fillRing[k]]);
            auto angle = angle360(cone - enter, leave - cone, direct);
            if (angle >= 1.0 && angle <= 179.0) {
                bool isEar = true;
                for (size_t x = 0; x < fillRing.size() - 3; ++x) {
                    auto fourth = vertexToEigenVector3d(vertices[(i + 3 + k) % fillRing.size()]);
                    if (pointInTriangle(enter, cone, leave, fourth)) {
                        isEar = false;
                        break;
                    }
                }
                if (isEar) {
                    Face newFace;
                    newFace.indices[0] = fillRing[i];
                    newFace.indices[1] = fillRing[j];
                    newFace.indices[2] = fillRing[k];
                    faces.push_back(newFace);
                    fillRing.erase(fillRing.begin() + j);
                    newFaceGenerated = true;
                    break;
                }
            }
        }
        if (!newFaceGenerated)
            break;
    }
    if (fillRing.size() == 3) {
        Face newFace;
        newFace.indices[0] = fillRing[0];
        newFace.indices[1] = fillRing[1];
        newFace.indices[2] = fillRing[2];
        faces.push_back(newFace);
    }
}

}
