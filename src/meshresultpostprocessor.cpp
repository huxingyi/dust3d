#include <QGuiApplication>
#include "meshresultpostprocessor.h"
#include "uvunwrap.h"
#include "triangletangentresolve.h"

MeshResultPostProcessor::MeshResultPostProcessor(const Outcome &outcome)
{
    m_outcome = new Outcome;
    *m_outcome = outcome;
}

MeshResultPostProcessor::~MeshResultPostProcessor()
{
    delete m_outcome;
}

Outcome *MeshResultPostProcessor::takePostProcessedOutcome()
{
    Outcome *outcome = m_outcome;
    m_outcome = nullptr;
    return outcome;
}

void MeshResultPostProcessor::poseProcess()
{
    if (!m_outcome->nodes.empty()) {
        {
            std::vector<std::vector<QVector2D>> triangleVertexUvs;
            std::set<int> seamVertices;
            uvUnwrap(*m_outcome, triangleVertexUvs, seamVertices);
            m_outcome->setTriangleVertexUvs(triangleVertexUvs);
        }
        
        {
            std::vector<QVector3D> triangleTangents;
            triangleTangentResolve(*m_outcome, triangleTangents);
            m_outcome->setTriangleTangents(triangleTangents);
        }
    }
}

void MeshResultPostProcessor::process()
{
    poseProcess();

    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}
