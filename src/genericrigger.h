#ifndef DUST3D_GENERIC_RIGGER_H
#define DUST3D_GENERIC_RIGGER_H
#include <QVector3D>
#include <vector>
#include <set>
#include "rigger.h"
#include "meshsplitter.h"

class GenericRigger: public Rigger
{
    Q_OBJECT
public:
    GenericRigger(const std::vector<QVector3D> &verticesPositions,
        const std::set<MeshSplitterTriangle> &inputTriangles);
protected:
    bool validate() override;
    bool isCutOffSplitter(BoneMark boneMark) override;
    bool rig() override;
    BoneMark translateBoneMark(BoneMark boneMark) override;
private:
    void collectJointsForLimb(int markIndex, std::vector<int> &jointMarkIndicies);
    void normalizeButtonColumns();
    QString namingLimb(int spineOrder, SkeletonSide side, int limbOrder, int jointOrder);
};

#endif
