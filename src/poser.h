#ifndef DUST3D_POSER_H
#define DUST3D_POSER_H
#include <QObject>
#include "rigger.h"
#include "jointnodetree.h"
#include "util.h"

class Poser : public QObject
{
    Q_OBJECT
public:
    Poser(const std::vector<RiggerBone> &bones);
    ~Poser();
    const RiggerBone *findBone(const QString &name);
    int findBoneIndex(const QString &name);
    const std::vector<RiggerBone> &bones() const;
    const std::vector<JointNode> &resultNodes() const;
    const JointNodeTree &resultJointNodeTree() const;
    std::map<QString, std::map<QString, QString>> &parameters();
    virtual void commit();
    void reset();
protected:
    std::vector<RiggerBone> m_bones;
    std::map<QString, int> m_boneNameToIndexMap;
    JointNodeTree m_jointNodeTree;
    std::map<QString, std::map<QString, QString>> m_parameters;
};

#endif
