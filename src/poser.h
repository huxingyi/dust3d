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
    void setYtranslationScale(float scale);
    virtual void commit();
    void reset();
    static void fetchChains(const std::vector<QString> &boneNames, std::map<QString, std::vector<QString>> &chains);
protected:
    std::vector<RiggerBone> m_bones;
    std::map<QString, int> m_boneNameToIndexMap;
    JointNodeTree m_jointNodeTree;
    std::map<QString, std::map<QString, QString>> m_parameters;
    float m_yTranslationScale = 1.0;
};

#endif
