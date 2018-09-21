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
    const MeshResultContext &meshResultContext,
    const std::map<int, AutoRiggerVertexWeights> &resultWeights)
{
    if (nullptr != m_poseMeshCreator)
        return false;
    
    qDebug() << "Pose mesh generating..";
    
    QThread *thread = new QThread;
    m_poseMeshCreator = new PoseMeshCreator(poser, meshResultContext, resultWeights);
    m_poseMeshCreator->moveToThread(thread);
    connect(thread, &QThread::started, m_poseMeshCreator, &PoseMeshCreator::process);
    connect(m_poseMeshCreator, &PoseMeshCreator::finished, this, &PosePreviewManager::poseMeshReady);
    connect(m_poseMeshCreator, &PoseMeshCreator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
    
    return true;
}

MeshLoader *PosePreviewManager::takeResultPreviewMesh()
{
    if (nullptr == m_previewMesh)
        return nullptr;
    return new MeshLoader(*m_previewMesh);
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
