#ifndef DUST3D_FIX_HOLES_H
#define DUST3D_FIX_HOLES_H
#include <QVector3D>
#include <vector>

void fixHoles(const std::vector<QVector3D> &vertices, std::vector<std::vector<size_t>> &faces);

#endif
