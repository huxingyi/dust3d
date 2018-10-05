#ifndef MESH_WELD_SEAM_H
#define MESH_WELD_SEAM_H
#include "meshlite.h"
#include "positionmap.h"
#include <unordered_set>

int meshWeldSeam(void *meshlite, int meshId, float allowedSmallestDistance,
    const PositionMap<bool> &excludePositions, int *affectedNum=nullptr);

#endif
