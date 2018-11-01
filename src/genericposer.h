#ifndef DUST3D_GENERIC_POSER_H
#define DUST3D_GENERIC_POSER_H
#include "poser.h"

class GenericPoser : public Poser
{
    Q_OBJECT
public:
    GenericPoser(const std::vector<RiggerBone> &bones);
public:
    void commit() override;
};

#endif
