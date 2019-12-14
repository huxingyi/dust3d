#include "boxmesh.h"
#include "strokemeshbuilder.h"
#include "triangulate.h"

static const std::vector<QVector3D> subdivedBoxObjVertices = {
    {-0.025357, -0.025357, 0.025357},
    {-0.025357, 0.025357, 0.025357},
    {-0.025357, 0.025357, -0.025357},
    {-0.025357, -0.025357, -0.025357},
    {0.025357, -0.025357, 0.025357},
    {0.025357, -0.025357, -0.025357},
    {0.025357, 0.025357, 0.025357},
    {0.025357, 0.025357, -0.025357},
    {-0.030913, -0.030913, -0.000000},
    {-0.030913, -0.000000, 0.030913},
    {-0.030913, 0.030913, 0.000000},
    {-0.030913, 0.000000, -0.030913},
    {0.030913, -0.030913, -0.000000},
    {0.000000, -0.030913, 0.030913},
    {-0.000000, -0.030913, -0.030913},
    {0.030913, -0.000000, 0.030913},
    {-0.000000, 0.030913, 0.030913},
    {0.030913, 0.030913, 0.000000},
    {0.030913, 0.000000, -0.030913},
    {-0.000000, 0.030913, -0.030913},
    {-0.042574, 0.000000, -0.000000},
    {-0.000000, -0.042574, -0.000000},
    {0.000000, -0.000000, 0.042574},
    {0.042574, -0.000000, -0.000000},
    {-0.000000, 0.000000, -0.042574},
    {0.000000, 0.042574, 0.000000},
};

static const std::vector<std::vector<size_t>> subdivedBoxObjFaces = {
    {1, 10, 21, 9},
    {10, 2, 11, 21},
    {21, 11, 3, 12},
    {9, 21, 12, 4},
    {5, 14, 22, 13},
    {14, 1, 9, 22},
    {22, 9, 4, 15},
    {13, 22, 15, 6},
    {1, 14, 23, 10},
    {14, 5, 16, 23},
    {23, 16, 7, 17},
    {10, 23, 17, 2},
    {7, 16, 24, 18},
    {16, 5, 13, 24},
    {24, 13, 6, 19},
    {18, 24, 19, 8},
    {4, 12, 25, 15},
    {12, 3, 20, 25},
    {25, 20, 8, 19},
    {15, 25, 19, 6},
    {2, 17, 26, 11},
    {17, 7, 18, 26},
    {26, 18, 8, 20},
    {11, 26, 20, 3},
};

void boxmesh(const QVector3D &position, float radius, size_t subdivideTimes, std::vector<QVector3D> &vertices, std::vector<std::vector<size_t>> &faces)
{
    if (subdivideTimes > 0) {
        vertices.reserve(subdivedBoxObjVertices.size());
        auto ratio = 24 * radius;
        for (const auto &vertex: subdivedBoxObjVertices) {
            auto newVertex = vertex * ratio + position;
            vertices.push_back(newVertex);
        }
        faces.reserve(subdivedBoxObjFaces.size());
        for (const auto &face: subdivedBoxObjFaces) {
            auto newFace = {face[0] - 1,
                face[1] - 1,
                face[2] - 1,
                face[3] - 1,
            };
            faces.push_back(newFace);
        }
        return;
    }
    std::vector<QVector3D> beginPlane = {
        {-radius, -radius,  radius},
        { radius, -radius,  radius},
        { radius,  radius,  radius},
        {-radius,  radius,  radius},
    };
    std::vector<QVector3D> endPlane = {
        beginPlane[0],
        beginPlane[3],
        beginPlane[2],
        beginPlane[1]
    };
    for (auto &vertex: endPlane) {
        vertex.setZ(vertex.z() - radius - radius);
    }
    for (const auto &vertex: beginPlane) {
        vertices.push_back(vertex);
    }
    for (const auto &vertex: endPlane) {
        vertices.push_back(vertex);
    }
    std::vector<size_t> beginLoop = {
        0, 1, 2, 3
    };
    std::vector<size_t> endLoop = {
        4, 5, 6, 7
    };
    std::vector<size_t> alignedEndLoop = {
        4, 7, 6, 5
    };
    faces.push_back(beginLoop);
    faces.push_back(endLoop);
    for (size_t i = 0; i < beginLoop.size(); ++i) {
        size_t j = (i + 1) % beginLoop.size();
        faces.push_back({
            beginLoop[j],
            beginLoop[i],
            alignedEndLoop[i],
            alignedEndLoop[j]
        });
    }
    for (auto &vertex: vertices) {
        vertex += position;
    }
}
