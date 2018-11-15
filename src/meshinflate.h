#ifndef DUST3D_MESH_INFLATE_H
#define DUST3D_MESH_INFLATE_H
#include <vector>
#include <QVector3D>

void *meshInflate(void *combinableMesh, const std::vector<std::pair<QVector3D, float>> &inflateBalls, std::vector<std::pair<QVector3D, QVector3D>> &inflatedVertices);

#endif
