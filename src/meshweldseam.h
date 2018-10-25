#ifndef DUST3D_MESH_WELD_SEAM_H
#define DUST3D_MESH_WELD_SEAM_H
#include "meshlite.h"
#include "positionmap.h"
#include <unordered_set>

int meshWeldSeam(void *meshlite, int meshId, float allowedSmallestDistance,
    const PositionMap<bool> &excludePositions, int *affectedNum=nullptr);

#endif
