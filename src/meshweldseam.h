#ifndef MESH_WELD_SEAM_H
#define MESH_WELD_SEAM_H
#include "meshlite.h"
#include <unordered_set>

int meshWeldSeam(void *meshlite, int meshId, float allowedSmallestDistance,
    const std::unordered_set<int> &seamVerticesIndicies=std::unordered_set<int>());

#endif
