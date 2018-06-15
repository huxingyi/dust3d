#ifndef ANIMATION_CLIP_GENERATOR_H
#define ANIMATION_CLIP_GENERATOR_H
#include <QObject>
#include <map>
#include <QString>
#include "meshresultcontext.h"
#include "meshloader.h"
#include "skinnedmesh.h"
#include "jointnodetree.h"

class AnimationClipGenerator : public QObject
{
    Q_OBJECT
signals:
    void finished();
public slots:
    void process();
public:
    AnimationClipGenerator(const MeshResultContext &resultContext, const JointNodeTree &jointNodeTree,
        const QString &clipName, const std::map<QString, QString> &parameters, bool wantMesh=true);
    ~AnimationClipGenerator();
    std::vector<std::pair<float, MeshLoader *>> takeFrameMeshes();
    const std::vector<float> &times();
    const std::vector<RigFrame> &frames();
    void generate();
    static const std::vector<QString> supportedClipNames;
private:
    void generateFrame(SkinnedMesh &skinnedMesh, float amount, float beginTime, float duration);
private:
    MeshResultContext m_resultContext;
    JointNodeTree m_jointNodeTree;
    QString m_clipName;
    std::map<QString, QString> m_parameters;
    bool m_wantMesh = true;
    std::vector<std::pair<float, MeshLoader *>> m_frameMeshes;
    std::vector<float> m_times;
    std::vector<RigFrame> m_frames;
};

#endif
