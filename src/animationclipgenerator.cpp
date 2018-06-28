#include <QGuiApplication>
#include "animationclipgenerator.h"
#include "skinnedmesh.h"

const std::vector<QString> AnimationClipGenerator::supportedClipNames = {
    "Idle",
    "Walk",
    //"Run",
    //"Attack",
    //"Hurt",
#if USE_BULLET
    "Die",
#endif
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
#if USE_RAGDOLL
    delete m_ragdoll;
#endif
    delete m_locomotionController;
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
    
    RigFrame frame(jointNodeTree->joints().size());
    
    if (m_clipName == "Idle") {
        rigController->idle(amount);
        rigController->saveFrame(frame);
    } else if (m_clipName == "Walk") {
        m_locomotionController->simulate(amount);
        JointNodeTree outputJointNodeTree = m_locomotionController->outputJointNodeTreeOnlyPositions();
        outputJointNodeTree.recalculateMatricesAfterPositionUpdated();
        m_jointNodeTree.diff(outputJointNodeTree, frame);
    } else if (m_clipName == "Die") {
#if USE_BULLET
        if (nullptr == m_ragdoll) {
            rigController->saveFrame(frame);
        } else {
            m_ragdoll->stepSimulation(duration);
            m_ragdoll->saveFrame(frame);
        }
#endif
    }
    
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
    
    RigController *rigController = skinnedMesh.rigController();
    rigController->resetFrame();
    
    qDebug() << "animation clip name:" << m_clipName;
    
    if (m_clipName == "Idle") {
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
    } else if (m_clipName == "Walk") {
        m_locomotionController = new LocomotionController(m_jointNodeTree);
        m_locomotionController->prepare();
        
        float duration = 0.05;
        float nextBeginTime = 0;
        for (float amount = 0.0; amount <= 1; amount += duration) {
            generateFrame(skinnedMesh, amount, nextBeginTime, duration);
            nextBeginTime += duration;
        }
    } else if (m_clipName == "Die") {
#if USE_BULLET
        delete m_ragdoll;
        float averageLegHeight = rigController->averageLegHeight();
        rigController->liftLeftLegs(QVector3D(0, averageLegHeight * 1.2, 0));
        rigController->liftRightLegs(QVector3D(0, averageLegHeight * 0.5, 0));
        JointNodeTree ragdollJointNodeTree = m_jointNodeTree;
        RigFrame frame;
        rigController->saveFrame(frame);
        rigController->applyRigFrameToJointNodeTree(ragdollJointNodeTree, frame);
        m_ragdoll = new Ragdoll(m_jointNodeTree, ragdollJointNodeTree);
        m_ragdoll->setGroundPlaneY(rigController->minY());
        m_ragdoll->prepare();
        
        float duration = 1.0 / 120;
        float nextBeginTime = 0;
        for (float amount = 0.0; amount <= 3; amount += duration) {
            generateFrame(skinnedMesh, amount, nextBeginTime, duration);
            nextBeginTime += duration;
        }
#endif
    }
}

void AnimationClipGenerator::process()
{
    generate();
    
    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}
