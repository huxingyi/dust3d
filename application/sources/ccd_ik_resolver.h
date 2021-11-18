#ifndef DUST3D_APPLICATION_CCD_IK_SOLVER_H_
#define DUST3D_APPLICATION_CCD_IK_SOLVER_H_

#include <vector>
#include <QVector3D>
#include <QQuaternion>

struct CcdIkNode
{
    QVector3D position;
    QVector3D axis;
    double minLimitDegrees;
    double maxLimitDegrees;
};

class CcdIkSolver
{
public:
    CcdIkSolver();
    void setMaxRound(int maxRound);
    void setDistanceThreshod(float threshold);
    int addNodeInOrder(const QVector3D &position);
    void solveTo(const QVector3D &position);
    const QVector3D &getNodeSolvedPosition(int index);
    int getNodeCount();
    void setNodeHingeConstraint(int nodeIndex,
        const QVector3D &axis, double minLimitDegrees, double maxLimitDegrees);
    void setSolveFrom(int fromNodeIndex);
private:
    float angleInRangle360BetweenTwoVectors(QVector3D a, QVector3D b, QVector3D planeNormal);
    void iterate();

    std::vector<CcdIkNode> m_nodes;
    QVector3D m_destination;
    int m_maxRound = 4;
    float m_distanceThreshold2 = 0.001 * 0.001;
    float m_distanceCeaseThreshold2 = 0.001 * 0.001;
    int m_fromNodeIndex = 0;
};

#endif
