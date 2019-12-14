#ifndef DUST3D_BOX_H
#define DUST3D_BOX_H
#include <QVector3D>
#include <vector>

void boxmesh(const QVector3D &position, float radius, size_t subdivideTimes, std::vector<QVector3D> &vertices, std::vector<std::vector<size_t>> &faces);

#endif
