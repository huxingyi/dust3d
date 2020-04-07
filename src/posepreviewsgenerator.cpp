#include <QGuiApplication>
#include <QElapsedTimer>
#include "posepreviewsgenerator.h"
#include "posemeshcreator.h"
#include "poserconstruct.h"
#include "posedocument.h"

PosePreviewsGenerator::PosePreviewsGenerator(RigType rigType,
        const std::vector<RiggerBone> *rigBones,
        const std::map<int, RiggerVertexWeights> *rigWeights,
        const Outcome &outcome) :
    m_rigType(rigType),
    m_rigBones(*rigBones),
    m_rigWeights(*rigWeights),
    m_outcome(new Outcome(outcome))
{
}

PosePreviewsGenerator::~PosePreviewsGenerator()
{
    for (auto &item: m_previews) {
        delete item.second;
    }
    delete m_outcome;
}

void PosePreviewsGenerator::addPose(std::pair<QUuid, int> idAndFrame, const std::map<QString, std::map<QString, QString>> &pose)
{
    m_poses.push_back(std::make_pair(idAndFrame, pose));
}

const std::set<std::pair<QUuid, int>> &PosePreviewsGenerator::generatedPreviewPoseIdAndFrames()
{
    return m_generatedPoseIdAndFrames;
}

Model *PosePreviewsGenerator::takePreview(std::pair<QUuid, int> idAndFrame)
{
    Model *resultMesh = m_previews[idAndFrame];
    m_previews[idAndFrame] = nullptr;
    return resultMesh;
}

void PosePreviewsGenerator::process()
{
    QElapsedTimer countTimeConsumed;
    countTimeConsumed.start();

    Poser *poser = newPoser(m_rigType, m_rigBones);
    for (const auto &pose: m_poses) {
        PoseDocument poseDocument;
        poseDocument.fromParameters(&m_rigBones, pose.second);
        std::map<QString, std::map<QString, QString>> translatedParameters;
        poseDocument.toParameters(translatedParameters);
        poser->parameters() = translatedParameters;
        poser->commit();
        
        PoseMeshCreator *poseMeshCreator = new PoseMeshCreator(poser->resultNodes(), *m_outcome, m_rigWeights);
        poseMeshCreator->createMesh();
        m_previews[pose.first] = poseMeshCreator->takeResultMesh();
        delete poseMeshCreator;
        
        poser->reset();
        
        m_generatedPoseIdAndFrames.insert(pose.first);
    }
    delete poser;
    
    qDebug() << "The pose previews generation took" << countTimeConsumed.elapsed() << "milliseconds";
    
    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}
