#include <QtGlobal>
#include <QMatrix4x4>
#include <QDebug>
#include <cmath>
#include "ccdikresolver.h"
#include "dust3dutil.h"

CCDIKSolver::CCDIKSolver() :
    m_maxRound(4),
    m_distanceThreshold2(0.001 * 0.001),
    m_distanceCeaseThreshold2(0.001 * 0.001)
{
}

void CCDIKSolver::setMaxRound(int maxRound)
{
    m_maxRound = maxRound;
}

void CCDIKSolver::setDistanceThreshod(float threshold)
{
    m_distanceThreshold2 = threshold * threshold;
}

int CCDIKSolver::addNodeInOrder(const QVector3D &position, const JointConstraint *constraint)
{
    CCDIKNode node;
    node.position = position;
    if (nullptr != constraint) {
        node.constraint = *constraint;
        node.hasConstraint = true;
    }
    int nodeCount = m_nodes.size();
    m_nodes.push_back(node);
    return nodeCount;
}

void CCDIKSolver::solveTo(const QVector3D &position)
{
    //qDebug() << "solveTo:" << position;
    m_destination = position;
    float lastDistance2 = 0;
    for (int i = 0; i < m_maxRound; i++) {
        const auto &endEffector = m_nodes[m_nodes.size() - 1];
        float distance2 = (endEffector.position - m_destination).lengthSquared();
        //qDebug() << "Round:" << i << " distance2:" << distance2;
        if (distance2 <= m_distanceThreshold2)
            break;
        if (lastDistance2 > 0 && fabs(distance2 - lastDistance2) <= m_distanceCeaseThreshold2)
            break;
        lastDistance2 = distance2;
        iterate();
    }
}

const QVector3D &CCDIKSolver::getNodeSolvedPosition(int index)
{
    Q_ASSERT(index >= 0 && index < (int)m_nodes.size());
    return m_nodes[index].position;
}

int CCDIKSolver::getNodeCount(void)
{
    return m_nodes.size();
}

void CCDIKSolver::iterate()
{
    for (int i = m_nodes.size() - 2; i >= 0; i--) {
        const auto &origin = m_nodes[i];
        const auto &endEffector = m_nodes[m_nodes.size() - 1];
        QVector3D from = (endEffector.position - origin.position).normalized();
        QVector3D to = (m_destination - origin.position).normalized();
        if (origin.hasConstraint) {
            if (origin.constraint.type() == JointConstraintType::Hinge) {
                const auto &next = m_nodes[i + 1];
                
                const auto originPointerToChild = projectLineOnPlane(next.position - origin.position, origin.constraint.hingePlaneNormal()).normalized();
                const auto originPointerToParent = i - 1 >= 0 ? projectLineOnPlane(m_nodes[i - 1].position - origin.position, origin.constraint.hingePlaneNormal()).normalized() : QVector3D(0, 1, 0);

                const auto originPointerToDestination = projectLineOnPlane(m_destination - origin.position, origin.constraint.hingePlaneNormal()).normalized();
                const auto originPointerToEndEffector = projectLineOnPlane(endEffector.position - origin.position, origin.constraint.hingePlaneNormal()).normalized();
                
                const auto originPointerToRotatedDestination = QQuaternion::rotationTo(originPointerToEndEffector, originPointerToChild).rotatedVector(originPointerToDestination);
                
                float degrees = angleInRangle360BetweenTwoVectors(originPointerToParent, originPointerToRotatedDestination, origin.constraint.hingePlaneNormal());
                float limitToDegrees = degrees;
                bool needLimit = false;
                if (degrees > origin.constraint.maxHingeDegrees()) {
                    qDebug() << "Limit degrees:" << degrees << "to max:" << origin.constraint.maxHingeDegrees();
                    limitToDegrees = origin.constraint.maxHingeDegrees();
                    needLimit = true;
                } else if (degrees < origin.constraint.minHingeDegrees()) {
                    qDebug() << "Limit degrees:" << degrees << "to min:" << origin.constraint.minHingeDegrees();
                    limitToDegrees = origin.constraint.minHingeDegrees();
                    needLimit = true;
                }
                if (needLimit) {
                    QQuaternion rotation = QQuaternion::fromAxisAndAngle(origin.constraint.hingePlaneNormal(), limitToDegrees);
                    auto revisedRotatedDestDirect = rotation.rotatedVector(originPointerToParent);
                    from = originPointerToEndEffector;
                    to = QQuaternion::rotationTo(originPointerToChild, originPointerToEndEffector).rotatedVector(revisedRotatedDestDirect);
                }
            }
        }
        auto quaternion = QQuaternion::rotationTo(from, to);
        for (size_t j = i + 1; j <= m_nodes.size() - 1; j++) {
            auto &next = m_nodes[j];
            const auto offset = next.position - origin.position;
            next.position = origin.position + quaternion.rotatedVector(offset);
        }
    }
}
