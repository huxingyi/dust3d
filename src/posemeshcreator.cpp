#include <QGuiApplication>
#include "posemeshcreator.h"
#include "skinnedmeshcreator.h"

PoseMeshCreator::PoseMeshCreator(const std::vector<JointNode> &resultNodes,
        const Outcome &outcome,
        const std::map<int, RiggerVertexWeights> &resultWeights) :
    m_resultNodes(resultNodes),
    m_outcome(outcome),
    m_resultWeights(resultWeights)
{
}

PoseMeshCreator::~PoseMeshCreator()
{
    delete m_resultMesh;
}

Model *PoseMeshCreator::takeResultMesh()
{
    Model *resultMesh = m_resultMesh;
    m_resultMesh = nullptr;
    return resultMesh;
}

void PoseMeshCreator::createMesh()
{
    SkinnedMeshCreator skinnedMeshCreator(m_outcome, m_resultWeights);
    
    std::vector<QMatrix4x4> matricies;
    matricies.resize(m_resultNodes.size());
    for (decltype(m_resultNodes.size()) i = 0; i < m_resultNodes.size(); i++) {
        const auto &node = m_resultNodes[i];
        matricies[i] = node.transformMatrix;
    }
    
    delete m_resultMesh;
    m_resultMesh = skinnedMeshCreator.createMeshFromTransform(matricies);
}

void PoseMeshCreator::process()
{
    createMesh();
    
    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}
