#include "poserconstruct.h"
#include "animalposer.h"

Poser *newPoser(RigType rigType, const std::vector<RiggerBone> &bones)
{
    if (rigType == RigType::Animal)
        return new AnimalPoser(bones);
    return nullptr;
}
