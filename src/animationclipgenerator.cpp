#include <QGuiApplication>
#include "animationclipgenerator.h"
#include "skinnedmesh.h"

AnimationClipGenerator::AnimationClipGenerator(const MeshResultContext &resultContext,
        const QString &motionName, const std::map<QString, QString> &parameters) :
    m_resultContext(resultContext),
    m_motionName(motionName),
    m_parameters(parameters)
{
}

AnimationClipGenerator::~AnimationClipGenerator()
{
    for (auto &mesh: m_frameMeshes) {
        delete mesh.second;
    }
}

std::vector<std::pair<int, MeshLoader *>> AnimationClipGenerator::takeFrameMeshes()
{
    std::vector<std::pair<int, MeshLoader *>> result = m_frameMeshes;
    m_frameMeshes.clear();
    return result;
}

void AnimationClipGenerator::generate()
{
    SkinnedMesh skinnedMesh(m_resultContext);
    skinnedMesh.startRig();
    
    RigController *rigController = skinnedMesh.rigController();
    
    for (float amount = 0.0; amount <= 0.5; amount += 0.05) {
        rigController->squat(amount);
        RigFrame frame;
        rigController->saveFrame(frame);
        skinnedMesh.applyRigFrameToMesh(frame);
        m_frameMeshes.push_back(std::make_pair(10, skinnedMesh.toMeshLoader()));
    }
}

void AnimationClipGenerator::process()
{
    generate();
    
    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}
