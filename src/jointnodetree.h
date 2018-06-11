#ifndef JOINT_NODE_TREE_H
#define JOINT_NODE_TREE_H
#include <QVector3D>
#include <QMatrix4x4>
#include <vector>
#include "meshresultcontext.h"
#include "skeletonbonemark.h"

struct JointInfo
{
    int jointIndex = 0;
    int partId = 0;
    int nodeId = 0;
    SkeletonBoneMark boneMark = SkeletonBoneMark::None;
    QVector3D position;
    QVector3D translation;
    QQuaternion rotation;
    QMatrix4x4 localMatrix;
    QMatrix4x4 bindMatrix;
    QMatrix4x4 inverseBindMatrix;
    std::vector<int> children;
};

class JointNodeTree
{
public:
    JointNodeTree(MeshResultContext &resultContext);
    std::vector<JointInfo> &joints();
    int nodeToJointIndex(int partId, int nodeId);
    void recalculateMatricesAfterPositionsUpdated();
private:
    void calculateMatrices();
    void calculateMatricesFrom(int jointIndex, const QVector3D &parentPosition, const QVector3D &parentDirection, const QMatrix4x4 &parentMatrix);
    std::vector<JointInfo> m_tracedJoints;
    std::map<std::pair<int, int>, int> m_tracedNodeToJointIndexMap;
    void traceBoneFromJoint(MeshResultContext &resultContext, std::pair<int, int> node, std::set<std::pair<int, int>> &visitedNodes, std::set<std::pair<std::pair<int, int>, std::pair<int, int>>> &connections, int parentIndex);
};

#endif
