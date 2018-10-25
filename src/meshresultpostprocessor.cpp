#include <QGuiApplication>
#include "meshresultpostprocessor.h"

MeshResultPostProcessor::MeshResultPostProcessor(const Outcome &outcome)
{
    m_outcome = new Outcome;
    *m_outcome = outcome;
}

MeshResultPostProcessor::~MeshResultPostProcessor()
{
    delete m_outcome;
}

Outcome *MeshResultPostProcessor::takePostProcessedResultContext()
{
    Outcome *outcome = m_outcome;
    m_outcome = nullptr;
    return outcome;
}

void MeshResultPostProcessor::process()
{
    if (!m_outcome->bmeshNodes.empty()) {
        (void)m_outcome->triangleTangents();
        (void)m_outcome->parts();
    }
    
    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}


