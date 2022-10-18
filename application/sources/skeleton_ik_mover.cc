#include "skeleton_ik_mover.h"
#include "ccd_ik_resolver.h"
#include <QGuiApplication>

SkeletonIkMover::SkeletonIkMover()
{
}

SkeletonIkMover::~SkeletonIkMover()
{
}

void SkeletonIkMover::setTarget(const QVector3D& target)
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

void SkeletonIkMover::addNode(dust3d::Uuid id, QVector3D position)
{
    SkeletonIkNode ikNode;
    ikNode.id = id;
    ikNode.position = ikNode.newPosition = position;
    m_ikNodes.push_back(ikNode);
}

const std::vector<SkeletonIkNode>& SkeletonIkMover::ikNodes()
{
    return m_ikNodes;
}

void SkeletonIkMover::process()
{
    resolve();

    emit finished();
}

void SkeletonIkMover::resolve()
{
    CcdIkSolver solver;
    for (auto i = 0u; i < m_ikNodes.size(); i++) {
        solver.addNodeInOrder(m_ikNodes[i].position);
    }
    solver.solveTo(m_target);
    for (auto i = 0u; i < m_ikNodes.size(); i++) {
        m_ikNodes[i].newPosition = solver.getNodeSolvedPosition(i);
    }
}
