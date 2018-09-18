#ifndef POSER_H
#define POSER_H
#include <QObject>
#include "autorigger.h"
#include "jointnodetree.h"
#include "dust3dutil.h"

class Poser : public QObject
{
    Q_OBJECT
public:
    Poser(const std::vector<AutoRiggerBone> &bones);
    ~Poser();
    const AutoRiggerBone *findBone(const QString &name);
    int findBoneIndex(const QString &name);
    const std::vector<AutoRiggerBone> &bones() const;
    const std::vector<JointNode> &resultNodes() const;
    std::map<QString, std::map<QString, QString>> &parameters();
    void commit();
    void reset();
protected:
    std::vector<AutoRiggerBone> m_bones;
    std::map<QString, int> m_boneNameToIndexMap;
    JointNodeTree m_jointNodeTree;
    std::map<QString, std::map<QString, QString>> m_parameters;
};

#endif
