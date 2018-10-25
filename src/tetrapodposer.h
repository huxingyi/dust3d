#ifndef DUST3D_TETRAPOD_POSER_H
#define DUST3D_TETRAPOD_POSER_H
#include "poser.h"

class TetrapodPoser : public Poser
{
    Q_OBJECT
public:
    TetrapodPoser(const std::vector<AutoRiggerBone> &bones);
public:
    void commit();
};

#endif
