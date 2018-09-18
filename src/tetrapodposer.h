#ifndef TETRAPOD_POSER_H
#define TETRAPOD_POSER_H
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
