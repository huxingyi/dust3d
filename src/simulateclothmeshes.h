#ifndef DUST3D_SIMULATE_CLOTH_MESHES_H
#define DUST3D_SIMULATE_CLOTH_MESHES_H
#include <QVector3D>
#include <vector>
#include <QUuid>
#include "clothforce.h"

struct ClothMesh
{
    std::vector<QVector3D> vertices;
    std::vector<std::vector<size_t>> faces;
    std::vector<std::pair<QUuid, QUuid>> vertexSources;
    const std::vector<std::pair<QVector3D, std::pair<QUuid, QUuid>>> *objectNodeVertices;
    ClothForce clothForce;
    float clothOffset;
    float clothStiffness;
    size_t clothIteration;
};

void simulateClothMeshes(std::vector<ClothMesh> *clothMeshes,
    const std::vector<QVector3D> *clothCollisionVertices,
    const std::vector<std::vector<size_t>> *clothCollisionTriangles);

#endif
