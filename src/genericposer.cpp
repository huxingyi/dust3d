#include <unordered_map>
#include "genericposer.h"

GenericPoser::GenericPoser(const std::vector<RiggerBone> &bones) :
    Poser(bones)
{
}

void GenericPoser::commit()
{
    // TODO:
    
    Poser::commit();
}
