#ifndef DUST3D_TETRAPOD_POSER_H
#define DUST3D_TETRAPOD_POSER_H
#include <vector>
#include "poser.h"

class TetrapodPoser : public Poser
{
    Q_OBJECT
public:
    TetrapodPoser(const std::vector<RiggerBone> &bones);
    void commit() override;
    
private:
    void resolveTranslation();
    void resolveLimbRotation(const std::vector<QString> &limbBoneNames);
    std::pair<bool, QVector3D> findQVector3DFromMap(const std::map<QString, QString> &map, const QString &xName, const QString &yName, const QString &zName);
    std::pair<bool, std::pair<QVector3D, QVector3D>> findBonePositionsFromParameters(const std::map<QString, QString> &map);
};

#endif
