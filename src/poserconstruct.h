#ifndef DUST3D_POSER_CONSTRUCT_H
#define DUST3D_POSER_CONSTRUCT_H
#include "rigtype.h"
#include "poser.h"
#include "rigger.h"

Poser *newPoser(RigType rigType, const std::vector<RiggerBone> &bones);

#endif
