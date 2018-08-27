#include <QGuiApplication>
#include "meshresultpostprocessor.h"

MeshResultPostProcessor::MeshResultPostProcessor(const MeshResultContext &meshResultContext)
{
    m_meshResultContext = new MeshResultContext;
    *m_meshResultContext = meshResultContext;
}

MeshResultPostProcessor::~MeshResultPostProcessor()
{
    delete m_meshResultContext;
}

MeshResultContext *MeshResultPostProcessor::takePostProcessedResultContext()
{
    MeshResultContext *resultContext = m_meshResultContext;
    m_meshResultContext = nullptr;
    return resultContext;
}

void MeshResultPostProcessor::process()
{
    if (!m_meshResultContext->bmeshNodes.empty()) {
        m_meshResultContext->rearrangedVertices();
        m_meshResultContext->rearrangedTriangles();
        m_meshResultContext->parts();
    }
    
    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}


