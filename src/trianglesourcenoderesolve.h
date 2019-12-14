#ifndef DUST3D_TRIANGLE_SOURCE_NODE_RESOLVE_H
#define DUST3D_TRIANGLE_SOURCE_NODE_RESOLVE_H
#include "outcome.h"

void triangleSourceNodeResolve(const Outcome &outcome, std::vector<std::pair<QUuid, QUuid>> &triangleSourceNodes,
    std::vector<std::pair<QUuid, QUuid>> *vertexSourceNodes=nullptr);

#endif
