#include <QGuiApplication>
#include "skeletonikmover.h"
#include "ccdikresolver.h"

SkeletonIkMover::SkeletonIkMover() :
    m_updateVersion(0)
{
}

SkeletonIkMover::~SkeletonIkMover()
{
}

void SkeletonIkMover::setTarget(const QVector3D &target)
{
    m_target = target;
}

void SkeletonIkMover::setUpdateVersion(unsigned long long version)
{
    m_updateVersion = version;
}

unsigned long long SkeletonIkMover::getUpdateVersion()
{
    return m_updateVersion;
}

void SkeletonIkMover::addNode(QUuid id, QVector3D position)
{
    SkeletonIkNode ikNode;
    ikNode.id = id;
    ikNode.position = ikNode.newPosition = position;
    m_ikNodes.push_back(ikNode);
}

const std::vector<SkeletonIkNode> &SkeletonIkMover::ikNodes()
{
    return m_ikNodes;
}

void SkeletonIkMover::process()
{
    resolve();
    
    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}

void SkeletonIkMover::resolve()
{
    CCDIKSolver solver;
    for (auto i = 0u; i < m_ikNodes.size(); i++) {
        solver.addNodeInOrder(m_ikNodes[i].position);
    }
    solver.solveTo(m_target);
    for (auto i = 0u; i < m_ikNodes.size(); i++) {
        m_ikNodes[i].newPosition = solver.getNodeSolvedPosition(i);
    }
}
