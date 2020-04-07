#include <QThread>
#include <QGridLayout>
#include "posepreviewmanager.h"

PosePreviewManager::PosePreviewManager()
{
}

PosePreviewManager::~PosePreviewManager()
{
    delete m_previewMesh;
}

bool PosePreviewManager::isRendering()
{
    return nullptr != m_poseMeshCreator;
}

bool PosePreviewManager::postUpdate(const Poser &poser,
    const Outcome &outcome,
    const std::map<int, RiggerVertexWeights> &resultWeights)
{
    if (nullptr != m_poseMeshCreator)
        return false;
    
    qDebug() << "Pose mesh generating..";
    
    QThread *thread = new QThread;
    m_poseMeshCreator = new PoseMeshCreator(poser.resultNodes(), outcome, resultWeights);
    m_poseMeshCreator->moveToThread(thread);
    connect(thread, &QThread::started, m_poseMeshCreator, &PoseMeshCreator::process);
    connect(m_poseMeshCreator, &PoseMeshCreator::finished, this, &PosePreviewManager::poseMeshReady);
    connect(m_poseMeshCreator, &PoseMeshCreator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
    
    return true;
}

Model *PosePreviewManager::takeResultPreviewMesh()
{
    if (nullptr == m_previewMesh)
        return nullptr;
    return new Model(*m_previewMesh);
}

void PosePreviewManager::poseMeshReady()
{
    delete m_previewMesh;
    m_previewMesh = m_poseMeshCreator->takeResultMesh();

    qDebug() << "Pose mesh generation done";
    
    delete m_poseMeshCreator;
    m_poseMeshCreator = nullptr;
    
    emit resultPreviewMeshChanged();
    emit renderDone();
}
