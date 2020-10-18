#ifndef DUST3D_REMESH_HOLE_H
#define DUST3D_REMESH_HOLE_H
#include <QVector3D>

void remeshHole(std::vector<QVector3D> &vertices,
    const std::vector<size_t> &hole,
    std::vector<std::vector<size_t>> &newFaces);

#endif
