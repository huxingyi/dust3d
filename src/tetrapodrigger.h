#ifndef DUST3D_TETRAPOD_RIGGER_H
#define DUST3D_TETRAPOD_RIGGER_H
#include <QVector3D>
#include "rigger.h"
#include "meshsplitter.h"

class TetrapodRigger : public Rigger
{
    Q_OBJECT
public:
    TetrapodRigger(const std::vector<QVector3D> &verticesPositions,
        const std::set<MeshSplitterTriangle> &inputTriangles);
protected:
    bool validate() override;
    bool isCutOffSplitter(BoneMark boneMark) override;
    bool rig() override;
};

#endif
