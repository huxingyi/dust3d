#ifndef MESH_QUADIFY_H
#define MESH_QUADIFY_H
#include <set>
#include "meshutil.h"
#include "positionmap.h"

int meshQuadify(void *meshlite, int meshId, const std::set<std::pair<PositionMapKey, PositionMapKey>> &sharedQuadEdges,
    PositionMap<int> *positionMapForMakeKey);

#endif
