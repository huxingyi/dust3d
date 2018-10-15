#include <simpleuv/triangulate.h>
#include <QVector3D>
#include <cmath>

namespace simpleuv
{

static QVector3D norm(const QVector3D &p1, const QVector3D &p2, const QVector3D &p3)
{
    auto side1 = p2 - p1;
    auto side2 = p3 - p1;
    auto perp = QVector3D::crossProduct(side1, side2);
    return perp.normalized();
}

static float angle360(const QVector3D &a, const QVector3D &b, const QVector3D &direct)
{
    auto angle = std::acos(QVector3D::dotProduct(a, b)) * 180.0 / 3.1415926;
    auto c = QVector3D::crossProduct(a, b);
    if (QVector3D::dotProduct(c, direct) < 0) {
        angle += 180;
    }
    return angle;
}

static QVector3D vertexToQVector3D(const Vertex &vertex)
{
    return QVector3D(vertex.xyz[0], vertex.xyz[1], vertex.xyz[2]);
}

static bool pointInTriangle(const QVector3D &a, const QVector3D &b, const QVector3D &c, const QVector3D &p)
{
    auto u = b - a;
    auto v = c - a;
    auto w = p - a;
    auto vXw = QVector3D::crossProduct(v, w);
    auto vXu = QVector3D::crossProduct(v, u);
    if (QVector3D::dotProduct(vXw, vXu) < 0.0) {
        return false;
    }
    auto uXw = QVector3D::crossProduct(u, w);
    auto uXv = QVector3D::crossProduct(u, v);
    if (QVector3D::dotProduct(uXw, uXv) < 0.0) {
        return false;
    }
    auto denom = uXv.length();
    auto r = vXw.length() / denom;
    auto t = uXw.length() / denom;
    return r + t <= 1.0;
}

static QVector3D ringNorm(const std::vector<Vertex> &vertices, const std::vector<size_t> &ring)
{
    QVector3D normal;
    for (size_t i = 0; i < ring.size(); ++i) {
        auto j = (i + 1) % ring.size();
        auto k = (i + 2) % ring.size();
        const auto &enter = vertexToQVector3D(vertices[ring[i]]);
        const auto &cone = vertexToQVector3D(vertices[ring[j]]);
        const auto &leave = vertexToQVector3D(vertices[ring[k]]);
        normal += norm(enter, cone, leave);
    }
    return normal.normalized();
}

void triangulate(const std::vector<Vertex> &vertices, std::vector<Face> &faces, const std::vector<size_t> &ring)
{
    if (ring.size() < 3)
        return;
    std::vector<size_t> fillRing = ring;
    QVector3D direct = ringNorm(vertices, fillRing);
    while (fillRing.size() > 3) {
        bool newFaceGenerated = false;
        for (decltype(fillRing.size()) i = 0; i < fillRing.size(); ++i) {
            auto j = (i + 1) % fillRing.size();
            auto k = (i + 2) % fillRing.size();
            const auto &enter = vertexToQVector3D(vertices[fillRing[i]]);
            const auto &cone = vertexToQVector3D(vertices[fillRing[j]]);
            const auto &leave = vertexToQVector3D(vertices[fillRing[k]]);
            auto angle = angle360(cone - enter, leave - cone, direct);
            if (angle >= 1.0 && angle <= 179.0) {
                bool isEar = true;
                for (size_t x = 0; x < fillRing.size() - 3; ++x) {
                    auto fourth = vertexToQVector3D(vertices[(i + 3 + k) % fillRing.size()]);
                    if (pointInTriangle(enter, cone, leave, fourth)) {
                        isEar = false;
                        break;
                    }
                }
                if (isEar) {
                    Face newFace;
                    newFace.indicies[0] = fillRing[i];
                    newFace.indicies[1] = fillRing[j];
                    newFace.indicies[2] = fillRing[k];
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
        newFace.indicies[0] = fillRing[0];
        newFace.indicies[1] = fillRing[1];
        newFace.indicies[2] = fillRing[2];
        faces.push_back(newFace);
    }
}

}
