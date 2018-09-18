#include <QGuiApplication>
#include "posemeshcreator.h"
#include "skinnedmeshcreator.h"

PoseMeshCreator::PoseMeshCreator(const Poser &poser,
        const MeshResultContext &meshResultContext,
        const std::map<int, AutoRiggerVertexWeights> &resultWeights) :
    m_resultNodes(poser.resultNodes()),
    m_meshResultContext(meshResultContext),
    m_resultWeights(resultWeights)
{
}

PoseMeshCreator::~PoseMeshCreator()
{
    delete m_resultMesh;
}

MeshLoader *PoseMeshCreator::takeResultMesh()
{
    MeshLoader *resultMesh = m_resultMesh;
    m_resultMesh = nullptr;
    return resultMesh;
}

void PoseMeshCreator::createMesh()
{
    SkinnedMeshCreator skinnedMeshCreator(m_meshResultContext, m_resultWeights);
    
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
