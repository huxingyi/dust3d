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
    delete m_jointNodeTree;
}

MeshResultContext *MeshResultPostProcessor::takePostProcessedResultContext()
{
    MeshResultContext *resultContext = m_meshResultContext;
    m_meshResultContext = nullptr;
    return resultContext;
}

JointNodeTree *MeshResultPostProcessor::takeJointNodeTree()
{
    JointNodeTree *jointNodeTree = m_jointNodeTree;
    m_jointNodeTree = nullptr;
    return jointNodeTree;
}

void MeshResultPostProcessor::process()
{
    if (!m_meshResultContext->bmeshNodes.empty()) {
        m_meshResultContext->resolveBmeshConnectivity();
        m_meshResultContext->resolveBmeshEdgeDirections();
        m_meshResultContext->vertexWeights();
        m_meshResultContext->removeIntermediateBones();
        m_meshResultContext->rearrangedVertices();
        m_meshResultContext->rearrangedTriangles();
        m_meshResultContext->parts();
    }
    
    m_jointNodeTree = new JointNodeTree(*m_meshResultContext);
    
    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}


