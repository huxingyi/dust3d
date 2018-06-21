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
        rootCenterJoint.scale = QVector3D(1.0, 1.0, 1.0);
        rootCenterJoint.radius = rootNode->radius;
        m_tracedJoints.push_back(rootCenterJoint);
    }
    
    //qDebug() << "Root node partId:" << rootNode->bmeshId << "nodeId:" << rootNode->nodeId;
    
    std::set<std::pair<int, int>> visitedNodes;
    std::set<std::pair<std::pair<int, int>, std::pair<int, int>>> connections;
    m_tracedNodeToJointIndexMap[std::make_pair(rootNode->bmeshId, rootNode->nodeId)] = rootCenterJoint.jointIndex;
    traceBoneFromJoint(resultContext, std::make_pair(rootNode->bmeshId, rootNode->nodeId), visitedNodes, connections, rootCenterJoint.jointIndex);
    
    calculateMatricesByPosition();
}

void JointNodeTree::traceBoneFromJoint(MeshResultContext &resultContext, std::pair<int, int> node, std::set<std::pair<int, int>> &visitedNodes, std::set<std::pair<std::pair<int, int>, std::pair<int, int>>> &connections, int parentIndex)
{
    if (visitedNodes.find(node) != visitedNodes.end())
        return;
    visitedNodes.insert(node);
    const auto &neighbors = resultContext.nodeNeighbors().find(node);
    if (neighbors == resultContext.nodeNeighbors().end())
        return;
    std::vector<std::pair<int, std::pair<int, int>>> neighborJoints;
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
        joint.scale = QVector3D(1.0, 1.0, 1.0);
        joint.radius = toNode->second->radius;
        
        m_tracedNodeToJointIndexMap[std::make_pair(it.first, it.second)] = joint.jointIndex;
        
        m_tracedJoints.push_back(joint);
        m_tracedJoints[parentIndex].children.push_back(joint.jointIndex);
        
        neighborJoints.push_back(std::make_pair(joint.jointIndex, it));
    }
    
    for (const auto &joint: neighborJoints) {
        traceBoneFromJoint(resultContext, joint.second, visitedNodes, connections, joint.first);
    }
}

const std::vector<JointInfo> &JointNodeTree::joints() const
{
    return m_tracedJoints;
}

std::vector<JointInfo> &JointNodeTree::joints()
{
    return m_tracedJoints;
}

int JointNodeTree::nodeToJointIndex(int partId, int nodeId) const
{
    const auto &findIt = m_tracedNodeToJointIndexMap.find(std::make_pair(partId, nodeId));
    if (findIt == m_tracedNodeToJointIndexMap.end()) {
        qDebug() << "node to joint index map failed, partId:" << partId << "nodeId:" << nodeId;
        return 0;
    }
    return findIt->second;
}

void JointNodeTree::calculateMatricesByPosition()
{
    if (joints().empty())
        return;
    calculateMatricesByPositionFrom(0, QVector3D(), QVector3D(), QMatrix4x4());
}

void JointNodeTree::calculateMatricesByTransform()
{
    for (auto &joint: joints()) {
        QMatrix4x4 translateMatrix;
        translateMatrix.translate(joint.translation);
        
        QMatrix4x4 rotateMatrix;
        rotateMatrix.rotate(joint.rotation);
        
        QMatrix4x4 scaleMatrix;
        scaleMatrix.scale(joint.scale);
        
        QMatrix4x4 localMatrix = translateMatrix * rotateMatrix * scaleMatrix;
        QMatrix4x4 bindMatrix = joint.parentIndex == -1 ? localMatrix : (joints()[joint.parentIndex].bindMatrix * localMatrix);
        
        bool invertible = true;
        
        joint.bindMatrix = bindMatrix;
        
        joint.position = joint.inverseBindMatrix * joint.bindMatrix * joint.position;
        
        joint.inverseBindMatrix = joint.bindMatrix.inverted(&invertible);
        
        if (!invertible)
            qDebug() << "jointIndex:" << joint.jointIndex << "invertible:" << invertible;
    }
}

void JointNodeTree::calculateMatricesByPositionFrom(int jointIndex, const QVector3D &parentPosition, const QVector3D &parentDirection, const QMatrix4x4 &parentMatrix)
{
    auto &joint = joints()[jointIndex];
    QVector3D translation = joint.position - parentPosition;
    QVector3D direction = QVector3D();
    
    QMatrix4x4 translateMatrix;
    translateMatrix.translate(translation);
    
    QMatrix4x4 rotateMatrix;
    QQuaternion rotation;
    if (!parentDirection.isNull()) {
        direction = translation.normalized();
        
        rotation = QQuaternion::rotationTo(parentDirection, direction);
        rotateMatrix.rotate(rotation);
    }
    
    QMatrix4x4 scaleMatrix;
    scaleMatrix.scale(joint.scale);
    
    QMatrix4x4 localMatrix = translateMatrix * rotateMatrix * scaleMatrix;
    QMatrix4x4 bindMatrix = parentMatrix * localMatrix;
    
    bool invertible = true;
    
    joint.localMatrix = localMatrix;
    joint.translation = translation;
    joint.rotation = rotation;
    joint.bindMatrix = bindMatrix;
    joint.inverseBindMatrix = joint.bindMatrix.inverted(&invertible);
    
    if (!invertible)
        qDebug() << "jointIndex:" << jointIndex << "invertible:" << invertible;
    
    for (const auto &child: joint.children) {
        calculateMatricesByPositionFrom(child, joint.position, direction, bindMatrix);
    }
}

void JointNodeTree::recalculateMatricesAfterPositionUpdated()
{
    calculateMatricesByPosition();
}

void JointNodeTree::recalculateMatricesAfterTransformUpdated()
{
    calculateMatricesByTransform();
}

