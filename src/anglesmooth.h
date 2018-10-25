#ifndef DUST3D_ANGLE_SMOOTH_H
#define DUST3D_ANGLE_SMOOTH_H
#include <QVector3D>
#include <vector>

void angleSmooth(const std::vector<QVector3D> &vertices,
    const std::vector<std::tuple<size_t, size_t, size_t>> &triangles,
    const std::vector<QVector3D> &triangleNormals,
    float thresholdAngleDegrees, std::vector<QVector3D> &triangleVertexNormals);

#endif
