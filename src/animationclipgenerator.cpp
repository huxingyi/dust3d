#include <QGuiApplication>
#include "animationclipgenerator.h"
#include "skinnedmesh.h"

const std::vector<QString> AnimationClipGenerator::supportedClipNames = {
    "Idle",
    //"Walk",
    //"Run",
    //"Attack",
    //"Hurt",
    //"Die",
};

AnimationClipGenerator::AnimationClipGenerator(const MeshResultContext &resultContext, const JointNodeTree &jointNodeTree,
        const QString &clipName, const std::map<QString, QString> &parameters, bool wantMesh) :
    m_resultContext(resultContext),
    m_jointNodeTree(jointNodeTree),
    m_clipName(clipName),
    m_parameters(parameters),
    m_wantMesh(wantMesh)
{
}

const std::vector<float> &AnimationClipGenerator::times()
{
    return m_times;
}

const std::vector<RigFrame> &AnimationClipGenerator::frames()
{
    return m_frames;
}

AnimationClipGenerator::~AnimationClipGenerator()
{
    for (auto &mesh: m_frameMeshes) {
        delete mesh.second;
    }
}

std::vector<std::pair<float, MeshLoader *>> AnimationClipGenerator::takeFrameMeshes()
{
    std::vector<std::pair<float, MeshLoader *>> result = m_frameMeshes;
    m_frameMeshes.clear();
    return result;
}

void AnimationClipGenerator::generateFrame(SkinnedMesh &skinnedMesh, float amount, float beginTime, float duration)
{
    RigController *rigController = skinnedMesh.rigController();
    JointNodeTree *jointNodeTree = skinnedMesh.jointNodeTree();
    
    rigController->resetFrame();
    
    if (m_clipName == "Idle")
        rigController->idle(amount);
    
    RigFrame frame(jointNodeTree->joints().size());
    rigController->saveFrame(frame);
    
    if (m_wantMesh) {
        skinnedMesh.applyRigFrameToMesh(frame);
        m_frameMeshes.push_back(std::make_pair(duration, skinnedMesh.toMeshLoader()));
    }
    
    m_times.push_back(beginTime);
    m_frames.push_back(frame);
}

void AnimationClipGenerator::generate()
{
    SkinnedMesh skinnedMesh(m_resultContext, m_jointNodeTree);
    skinnedMesh.startRig();
    float duration = 0.1;
    float nextBeginTime = 0;
    for (float amount = 0.0; amount <= 0.05; amount += 0.01) {
        generateFrame(skinnedMesh, amount, nextBeginTime, duration);
        nextBeginTime += duration;
    }
    for (float amount = 0.05; amount >= 0.0; amount -= 0.01) {
        generateFrame(skinnedMesh, amount, nextBeginTime, duration);
        nextBeginTime += duration;
    }
}

void AnimationClipGenerator::process()
{
    generate();
    
    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}
