#ifndef DUST3D_ANIMAL_POSER_H
#define DUST3D_ANIMAL_POSER_H
#include <vector>
#include "poser.h"

class AnimalPoser : public Poser
{
    Q_OBJECT
public:
    AnimalPoser(const std::vector<RiggerBone> &bones);
    void commit() override;
    
private:
    void resolveTransform();
    void resolveChainRotation(const std::vector<QString> &limbBoneNames);
    std::pair<bool, QVector3D> findQVector3DFromMap(const std::map<QString, QString> &map, const QString &xName, const QString &yName, const QString &zName);
    std::pair<bool, std::pair<QVector3D, QVector3D>> findBonePositionsFromParameters(const std::map<QString, QString> &map);
};

#endif
