#include "poserconstruct.h"
#include "tetrapodposer.h"
#include "genericposer.h"

Poser *newPoser(RigType rigType, const std::vector<RiggerBone> &bones)
{
    if (rigType == RigType::Tetrapod)
        return new TetrapodPoser(bones);
    else if (rigType == RigType::Generic)
        return new GenericPoser(bones);
    return nullptr;
}