#ifndef DUST3D_ISOTROPICREMESH_H
#define DUST3D_ISOTROPICREMESH_H
#include <QVector3D>
#include <vector>

void isotropicRemesh(const std::vector<QVector3D> &inputVertices,
        std::vector<std::vector<size_t>> &inputTriangles,
        std::vector<QVector3D> &outputVertices,
        std::vector<std::vector<size_t>> &outputTriangles,
        float targetEdgeLength,
        unsigned int iterationNum);

#endif
