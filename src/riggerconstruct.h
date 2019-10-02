#ifndef DUST3D_RIGGER_CONSTRUCT_H
#define DUST3D_RIGGER_CONSTRUCT_H
#include "rigtype.h"
#include "rigger.h"
#include "poser.h"

Rigger *newRigger(RigType rigType, const std::vector<QVector3D> &verticesPositions,
        const std::set<MeshSplitterTriangle> &inputTriangles,
        const std::vector<std::pair<std::pair<size_t, size_t>, std::pair<size_t, size_t>>> &triangleLinks);

#endif
