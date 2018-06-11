#include <cmath>
#include "jointnodetree.h"

JointNodeTree::JointNodeTree(MeshResultContext &resultContext)
{
    const BmeshNode *rootNode = resultContext.centerBmeshNode();
    if (!rootNode) {
        qDebug() << "Cannot construct JointNodeTree because lack of root node";
        return;
    }
    
    JointInfo rootCenterJoint;
    {
        rootCenterJoint.jointIndex = m_tracedJoints.size();
        QMatrix4x4 localMatrix;
        rootCenterJoint.partId = rootNode->bmeshId;
        rootCenterJoint.nodeId = rootNode->nodeId;
        rootCenterJoint.position = rootNode->origin;
        rootCenterJoint.boneMark = rootNode->boneMark;
        m_tracedJoints.push_back(rootCenterJoint);
    }
    
    //qDebug() << "Root node partId:" << rootNode->bmeshId << "nodeId:" << rootNode->nodeId;
    
    std::set<std::pair<int, int>> visitedNodes;
    std::set<std::pair<std::pair<int, int>, std::pair<int, int>>> connections;
    m_tracedNodeToJointIndexMap[std::make_pair(rootNode->bmeshId, rootNode->nodeId)] = rootCenterJoint.jointIndex;
    traceBoneFromJoint(resultContext, std::make_pair(rootNode->bmeshId, rootNode->nodeId), visitedNodes, connections, rootCenterJoint.jointIndex);
    
    calculateMatrices();
}

void JointNodeTree::traceBoneFromJoint(MeshResultContext &resultContext, std::pair<int, int> node, std::set<std::pair<int, int>> &visitedNodes, std::set<std::pair<std::pair<int, int>, std::pair<int, int>>> &connections, int parentIndex)
{
    if (visitedNodes.find(node) != visitedNodes.end())
        return;
    visitedNodes.insert(node);
    const auto &neighbors = resultContext.nodeNeighbors().find(node);
    if (neighbors == resultContext.nodeNeighbors().end())
        return;
    for (const auto &it: neighbors->second) {
        if (connections.find(std::make_pair(std::make_pair(node.first, node.second), std::make_pair(it.first, it.second))) != connections.end())
            continue;
        connections.insert(std::make_pair(std::make_pair(node.first, node.second), std::make_pair(it.first, it.second)));
        connections.insert(std::make_pair(std::make_pair(it.first, it.second), std::make_pair(node.first, node.second)));
        const auto &fromNode = resultContext.bmeshNodeMap().find(std::make_pair(node.first, node.second));
        if (fromNode == resultContext.bmeshNodeMap().end()) {
            qDebug() << "bmeshNodeMap find failed:" << node.first << node.second;
            continue;
        }
        const auto &toNode = resultContext.bmeshNodeMap().find(std::make_pair(it.first, it.second));
        if (toNode == resultContext.bmeshNodeMap().end()) {
            qDebug() << "bmeshNodeMap find failed:" << it.first << it.second;
            continue;
        }
        
        JointInfo joint;
        joint.position = toNode->second->origin;
        joint.jointIndex = m_tracedJoints.size();
        joint.partId = toNode->second->bmeshId;
        joint.nodeId = toNode->second->nodeId;
        joint.boneMark = toNode->second->boneMark;
        
        m_tracedNodeToJointIndexMap[std::make_pair(it.first, it.second)] = joint.jointIndex;
        
        m_tracedJoints.push_back(joint);
        m_tracedJoints[parentIndex].children.push_back(joint.jointIndex);
        
        traceBoneFromJoint(resultContext, it, visitedNodes, connections, joint.jointIndex);
    }
}

std::vector<JointInfo> &JointNodeTree::joints()
{
    return m_tracedJoints;
}

int JointNodeTree::nodeToJointIndex(int partId, int nodeId)
{
    const auto &findIt = m_tracedNodeToJointIndexMap.find(std::make_pair(partId, nodeId));
    if (findIt == m_tracedNodeToJointIndexMap.end())
        return 0;
    return findIt->second;
}

void JointNodeTree::calculateMatrices()
{
    if (joints().empty())
        return;
    calculateMatricesFrom(0, QVector3D(), QVector3D(), QMatrix4x4());
}

void JointNodeTree::calculateMatricesFrom(int jointIndex, const QVector3D &parentPosition, const QVector3D &parentDirection, const QMatrix4x4 &parentMatrix)
{
    auto &joint = joints()[jointIndex];
    QVector3D translation = joint.position - parentPosition;
    QVector3D direction = translation.normalized();
    
    QMatrix4x4 translateMatrix;
    translateMatrix.translate(translation);
    
    QMatrix4x4 rotateMatrix;
    QVector3D cross = QVector3D::crossProduct(parentDirection, direction).normalized();
    float dot = QVector3D::dotProduct(parentDirection, direction);
    float angle = acos(dot);
    QQuaternion rotation = QQuaternion::fromAxisAndAngle(cross, angle);
    rotateMatrix.rotate(QQuaternion::fromAxisAndAngle(cross, angle));
    
    QMatrix4x4 localMatrix = translateMatrix * rotateMatrix;
    QMatrix4x4 bindMatrix = parentMatrix * localMatrix;
    
    joint.localMatrix = localMatrix;
    joint.translation = translation;
    joint.rotation = rotation;
    joint.bindMatrix = bindMatrix;
    joint.inverseBindMatrix = joint.bindMatrix.inverted();
    
    for (const auto &child: joint.children) {
        calculateMatricesFrom(child, joint.position, direction, bindMatrix);
    }
}

void JointNodeTree::recalculateMatricesAfterPositionsUpdated()
{
    calculateMatrices();
}
