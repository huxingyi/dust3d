#include <QGuiApplication>
#include <QElapsedTimer>
#include "posepreviewsgenerator.h"
#include "tetrapodposer.h"
#include "posemeshcreator.h"

PosePreviewsGenerator::PosePreviewsGenerator(const std::vector<AutoRiggerBone> *rigBones,
        const std::map<int, AutoRiggerVertexWeights> *rigWeights,
        const MeshResultContext &meshResultContext) :
    m_rigBones(*rigBones),
    m_rigWeights(*rigWeights),
    m_meshResultContext(new MeshResultContext(meshResultContext))
{
}

PosePreviewsGenerator::~PosePreviewsGenerator()
{
    for (auto &item: m_previews) {
        delete item.second;
    }
    delete m_meshResultContext;
}

void PosePreviewsGenerator::addPose(QUuid poseId, const std::map<QString, std::map<QString, QString>> &pose)
{
    m_poses.push_back(std::make_pair(poseId, pose));
}

const std::set<QUuid> &PosePreviewsGenerator::generatedPreviewPoseIds()
{
    return m_generatedPoseIds;
}

MeshLoader *PosePreviewsGenerator::takePreview(QUuid poseId)
{
    MeshLoader *resultMesh = m_previews[poseId];
    m_previews[poseId] = nullptr;
    return resultMesh;
}

void PosePreviewsGenerator::process()
{
    QElapsedTimer countTimeConsumed;
    countTimeConsumed.start();

    TetrapodPoser *poser = new TetrapodPoser(m_rigBones);
    for (const auto &pose: m_poses) {
        poser->parameters() = pose.second;
        poser->commit();
        
        PoseMeshCreator *poseMeshCreator = new PoseMeshCreator(*poser, *m_meshResultContext, m_rigWeights);
        poseMeshCreator->createMesh();
        m_previews[pose.first] = poseMeshCreator->takeResultMesh();
        delete poseMeshCreator;
        
        poser->reset();
        
        m_generatedPoseIds.insert(pose.first);
    }
    delete poser;
    
    qDebug() << "The pose previews generation took" << countTimeConsumed.elapsed() << "milliseconds";
    
    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}
