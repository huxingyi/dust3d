#ifndef DUST3D_TRIANGULATE_FACES_H
#define DUST3D_TRIANGULATE_FACES_H
#include <QVector3D>
#include <vector>

bool triangulateFacesWithoutKeepVertices(std::vector<QVector3D> &vertices, const std::vector<std::vector<size_t>> &faces, std::vector<std::vector<size_t>> &triangles);

#endif
