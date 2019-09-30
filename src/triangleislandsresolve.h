#ifndef DUST3D_TRIANGLE_ISLANDS_RESOLVE_H
#define DUST3D_TRIANGLE_ISLANDS_RESOLVE_H
#include <vector>
#include "outcome.h"

void triangleIslandsResolve(const Outcome &outcome,
        const std::vector<size_t> &group,
        std::vector<std::vector<size_t>> &islands);

#endif

