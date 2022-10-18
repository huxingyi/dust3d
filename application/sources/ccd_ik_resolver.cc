#include "ccd_ik_resolver.h"
#include <QDebug>
#include <QMatrix4x4>
#include <QtGlobal>
#include <QtMath>
#include <cmath>

CcdIkSolver::CcdIkSolver()
{
}

void CcdIkSolver::setSolveFrom(int fromNodeIndex)
{
    m_fromNodeIndex = fromNodeIndex;
}

void CcdIkSolver::setNodeHingeConstraint(int nodeIndex,
    const QVector3D& axis, double minLimitDegrees, double maxLimitDegrees)
{
    auto& node = m_nodes[nodeIndex];
    node.axis = axis;
    node.minLimitDegrees = minLimitDegrees;
    node.maxLimitDegrees = maxLimitDegrees;
}

void CcdIkSolver::setMaxRound(int maxRound)
{
    m_maxRound = maxRound;
}

void CcdIkSolver::setDistanceThreshod(float threshold)
{
    m_distanceThreshold2 = threshold * threshold;
}

int CcdIkSolver::addNodeInOrder(const QVector3D& position)
{
    CcdIkNode node;
    node.position = position;
    int nodeCount = m_nodes.size();
    m_nodes.push_back(node);
    return nodeCount;
}

void CcdIkSolver::solveTo(const QVector3D& position)
{
    //qDebug() << "solveTo:" << position;
    m_destination = position;
    float lastDistance2 = 0;
    for (int i = 0; i < m_maxRound; i++) {
        const auto& endEffector = m_nodes[m_nodes.size() - 1];
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

const QVector3D& CcdIkSolver::getNodeSolvedPosition(int index)
{
    Q_ASSERT(index >= 0 && index < (int)m_nodes.size());
    return m_nodes[index].position;
}

int CcdIkSolver::getNodeCount()
{
    return (int)m_nodes.size();
}

float CcdIkSolver::angleInRangle360BetweenTwoVectors(QVector3D a, QVector3D b, QVector3D planeNormal)
{
    float degrees = acos(QVector3D::dotProduct(a, b)) * 180.0 / M_PI;
    QVector3D direct = QVector3D::crossProduct(a, b);
    if (QVector3D::dotProduct(direct, planeNormal) < 0)
        return 360 - degrees;
    return degrees;
}

void CcdIkSolver::iterate()
{
    auto rotateChildren = [&](const QQuaternion& quaternion, int i) {
        const auto& origin = m_nodes[i];
        for (size_t j = i + 1; j <= m_nodes.size() - 1; j++) {
            auto& next = m_nodes[j];
            const auto offset = next.position - origin.position;
            next.position = origin.position + quaternion.rotatedVector(offset);
        }
    };

    for (int i = m_nodes.size() - 2; i >= m_fromNodeIndex; i--) {
        const auto& origin = m_nodes[i];
        const auto& endEffector = m_nodes[m_nodes.size() - 1];
        QVector3D from = (endEffector.position - origin.position).normalized();
        QVector3D to = (m_destination - origin.position).normalized();
        auto quaternion = QQuaternion::rotationTo(from, to);
        rotateChildren(quaternion, i);
        if (origin.axis.isNull())
            continue;
        QVector3D oldAxis = origin.axis;
        QVector3D newAxis = quaternion.rotatedVector(oldAxis);
        auto hingQuaternion = QQuaternion::rotationTo(newAxis, oldAxis);
        rotateChildren(hingQuaternion, i);
        // TODO: Support angle limit for other axis
        int parentIndex = i - 1;
        if (parentIndex < 0)
            continue;
        int childIndex = i + 1;
        if (childIndex >= m_nodes.size())
            continue;
        const auto& parent = m_nodes[parentIndex];
        const auto& child = m_nodes[childIndex];
        QVector3D angleFrom = (QVector3D(0.0, parent.position.y(), parent.position.z()) - QVector3D(0.0, origin.position.y(), origin.position.z())).normalized();
        QVector3D angleTo = (QVector3D(0.0, child.position.y(), child.position.z()) - QVector3D(0.0, origin.position.y(), origin.position.z())).normalized();
        float degrees = angleInRangle360BetweenTwoVectors(angleFrom, angleTo, QVector3D(1.0, 0.0, 0.0));
        if (degrees < origin.minLimitDegrees) {
            auto quaternion = QQuaternion::fromAxisAndAngle(QVector3D(1.0, 0.0, 0.0), origin.minLimitDegrees - degrees);
            rotateChildren(quaternion, i);
        } else if (degrees > origin.maxLimitDegrees) {
            auto quaternion = QQuaternion::fromAxisAndAngle(QVector3D(-1.0, 0.0, 0.0), degrees - origin.maxLimitDegrees);
            rotateChildren(quaternion, i);
        }
    }
}
