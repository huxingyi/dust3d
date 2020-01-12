#ifndef DUST3D_PROJECT_TRIANGLES_TO_NODES_H
#define DUST3D_PROJECT_TRIANGLES_TO_NODES_H
#include <QVector3D>
#include <vector>

void projectFacesToNodes(const std::vector<QVector3D> &vertices,
    const std::vector<std::vector<size_t>> &faces,
    const std::vector<std::pair<QVector3D, float>> &sourceNodes,
    std::vector<size_t> *faceSources);

#endif
