#ifndef DUST3D_ANIMAL_RIGGER_H
#define DUST3D_ANIMAL_RIGGER_H
#include <QVector3D>
#include <vector>
#include <set>
#include "rigger.h"
#include "meshsplitter.h"

class AnimalRigger: public Rigger
{
    Q_OBJECT
public:
    AnimalRigger(const std::vector<QVector3D> &verticesPositions,
        const std::set<MeshSplitterTriangle> &inputTriangles);
protected:
    bool validate() override;
    bool isCutOffSplitter(BoneMark boneMark) override;
    bool rig() override;
    BoneMark translateBoneMark(BoneMark boneMark) override;
private:
    bool collectJontsForChain(int markIndex, std::vector<int> &jointMarkIndices);
    static QString namingSpine(int spineOrder, bool hasTail);
    static QString namingConnector(const QString &spineName, const QString &chainName);
    static QString namingChain(const QString &baseName, SkeletonSide side, int orderInSide, int totalInSide, int jointOrder);
    static QString namingChainPrefix(const QString &baseName, SkeletonSide side, int orderInSide, int totalInSide);
    QVector3D findExtremPointFrom(const std::set<int> &verticies, const QVector3D &from);
    float calculateSpineRadius(const std::vector<float> &leftXs,
        const std::vector<float> &rightXs,
        const std::vector<float> &middleRadiusCollection);
};

#endif
