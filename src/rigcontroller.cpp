#include <cmath>
#include "rigcontroller.h"
#include "ccdikresolver.h"

RigController::RigController(const JointNodeTree &jointNodeTree) :
    m_inputJointNodeTree(jointNodeTree),
    m_prepared(false),
    m_legHeight(0)
{
}

void RigController::saveFrame(RigFrame &frame)
{
    frame = m_rigFrame;
}

void RigController::collectLegs()
{
    m_legs.clear();
    for (const auto &node: m_inputJointNodeTree.joints()) {
        if (node.boneMark == SkeletonBoneMark::LegStart && node.children.size() == 1) {
            const auto legStart = std::make_pair(node.partId, node.nodeId);
            const JointInfo *loopNode = &m_inputJointNodeTree.joints()[node.children[0]];
            while (loopNode->boneMark != SkeletonBoneMark::LegEnd &&
                    loopNode->children.size() == 1) {
                loopNode = &m_inputJointNodeTree.joints()[loopNode->children[0]];
            }
            if (loopNode->boneMark == SkeletonBoneMark::LegEnd) {
                const auto legEnd = std::make_pair(loopNode->partId, loopNode->nodeId);
                addLeg(legStart, legEnd);
            } else {
                qDebug() << "Find leg" << node.partId << "'s end failed";
            }
        }
    }
}

int RigController::addLeg(std::pair<int, int> legStart, std::pair<int, int> legEnd)
{
    int legIndex = m_legs.size();
    m_legs.push_back(std::make_tuple(legStart.first, legStart.second, legEnd.first, legEnd.second));
    return legIndex;
}

void RigController::prepare()
{
    if (m_prepared)
        return;
    m_prepared = true;
    
    collectLegs();
    m_rigFrame = RigFrame(m_inputJointNodeTree.joints().size());
    calculateAverageLegHeight();
}

void RigController::lift(QVector3D offset)
{
    if (m_inputJointNodeTree.joints().empty())
        return;
    m_rigFrame.translations[0] = offset;
}

void RigController::liftLegs(QVector3D offset, QVector3D &effectedOffset)
{
    if (m_legs.empty())
        return;
    QVector3D effectedOffsetSum;
    for (auto i = 0u; i < m_legs.size(); i++) {
        QVector3D subEffectedOffset;
        liftLegEnd(i, offset, subEffectedOffset);
        effectedOffsetSum += subEffectedOffset;
    }
    effectedOffset = effectedOffsetSum / m_legs.size();
}

void RigController::liftLegEnd(int leg, QVector3D offset, QVector3D &effectedOffset)
{
    Q_ASSERT(leg >= 0 && leg < (int)m_legs.size());
    int legStartPartId = std::get<0>(m_legs[leg]);
    int legStartNodeId = std::get<1>(m_legs[leg]);
    int legEndPartId = std::get<2>(m_legs[leg]);
    int legEndNodeId = std::get<3>(m_legs[leg]);
    int legStartIndex = m_inputJointNodeTree.nodeToJointIndex(legStartPartId, legStartNodeId);
    int legEndIndex = m_inputJointNodeTree.nodeToJointIndex(legEndPartId, legEndNodeId);
    const auto &legStart = m_inputJointNodeTree.joints()[legStartIndex];
    const auto &legEnd = m_inputJointNodeTree.joints()[legEndIndex];
    auto destPosition = legEnd.position + offset;
    qDebug() << "dest move:" << destPosition.distanceToPoint(legEnd.position);
    
    CCDIKSolver ikSolver;
    ikSolver.setMaxRound(10);
    int loopIndex = legStartIndex;
    std::vector<int> ikSolvingIndicies;
    for (;;) {
        const auto &legJoint = m_inputJointNodeTree.joints()[loopIndex];
        ikSolvingIndicies.push_back(loopIndex);
        ikSolver.addNodeInOrder(legJoint.position);
        if (loopIndex == legEndIndex)
            break;
        if (legJoint.children.empty())
            break;
        Q_ASSERT(legStart.children.size() <= 1);
        loopIndex = legJoint.children[0];
    }
    
    ikSolver.solveTo(destPosition);
    int nodeCount = ikSolver.getNodeCount();
    Q_ASSERT(nodeCount == (int)ikSolvingIndicies.size());
    
    JointNodeTree outputJointNodeTree = m_inputJointNodeTree;
    for (int i = 0; i < nodeCount; i++) {
        int jointIndex = ikSolvingIndicies[i];
        const QVector3D &newPosition = ikSolver.getNodeSolvedPosition(i);
        const QVector3D &oldPosition = outputJointNodeTree.joints()[jointIndex].position;
        qDebug() << i << "position moved:" << oldPosition.distanceToPoint(newPosition);
        outputJointNodeTree.joints()[jointIndex].position = newPosition;
    }
    effectedOffset = ikSolver.getNodeSolvedPosition(nodeCount - 1) -
        m_inputJointNodeTree.joints()[ikSolvingIndicies[nodeCount - 1]].position;
    qDebug() << "end effector offset:" << destPosition.distanceToPoint(ikSolver.getNodeSolvedPosition(nodeCount - 1));
    outputJointNodeTree.recalculateMatricesAfterPositionsUpdated();
    QMatrix4x4 parentMatrix;
    for (int i = 0; i < nodeCount; i++) {
        int jointIndex = ikSolvingIndicies[i];
        const auto &inputJoint = m_inputJointNodeTree.joints()[jointIndex];
        const auto &outputJoint = outputJointNodeTree.joints()[jointIndex];
        
        QMatrix4x4 worldMatrix = outputJoint.bindMatrix * inputJoint.inverseBindMatrix;
        QMatrix4x4 trMatrix = worldMatrix * parentMatrix.inverted();
        
        m_rigFrame.rotations[jointIndex] = QQuaternion::fromRotationMatrix(trMatrix.normalMatrix());
        m_rigFrame.translations[jointIndex] = QVector3D(trMatrix(0, 3), trMatrix(1, 3), trMatrix(2, 3));

        parentMatrix = worldMatrix;
    }
}

void RigController::frameToMatrices(const RigFrame &frame, std::vector<QMatrix4x4> &matrices)
{
    if (m_inputJointNodeTree.joints().empty())
        return;
    matrices.clear();
    matrices.resize(m_inputJointNodeTree.joints().size());

    frameToMatricesAtJoint(frame, matrices, 0, QMatrix4x4());
}

void RigController::frameToMatricesAtJoint(const RigFrame &frame, std::vector<QMatrix4x4> &matrices, int jointIndex, const QMatrix4x4 &parentWorldMatrix)
{
    const auto &joint = m_inputJointNodeTree.joints()[jointIndex];
    
    QMatrix4x4 translateMatrix;
    translateMatrix.translate(frame.translations[jointIndex]);
    
    QMatrix4x4 rotateMatrix;
    rotateMatrix.rotate(frame.rotations[jointIndex]);
    
    QMatrix4x4 worldMatrix = parentWorldMatrix * translateMatrix * rotateMatrix;
    matrices[jointIndex] = worldMatrix;
    
    for (const auto &child: joint.children) {
        frameToMatricesAtJoint(frame, matrices, child, worldMatrix);
    }
}

void RigController::squat(float amount)
{
    prepare();
    
    QVector3D wantOffset;
    wantOffset.setY(m_legHeight * amount);
    QVector3D effectedOffset;
    liftLegs(wantOffset, effectedOffset);
    lift(-effectedOffset);
}

void RigController::calculateAverageLegHeight()
{
    QVector3D averageLegPlaneTop = QVector3D();
    QVector3D averageLegPlaneBottom = QVector3D();
    if (m_legs.empty())
        return;
    for (auto leg = 0u; leg < m_legs.size(); leg++) {
        int legStartPartId = std::get<0>(m_legs[leg]);
        int legStartNodeId = std::get<1>(m_legs[leg]);
        int legEndPartId = std::get<2>(m_legs[leg]);
        int legEndNodeId = std::get<3>(m_legs[leg]);
        int legStartIndex = m_inputJointNodeTree.nodeToJointIndex(legStartPartId, legStartNodeId);
        int legEndIndex = m_inputJointNodeTree.nodeToJointIndex(legEndPartId, legEndNodeId);
        const auto &legStart = m_inputJointNodeTree.joints()[legStartIndex];
        const auto &legEnd = m_inputJointNodeTree.joints()[legEndIndex];
        //qDebug() << "leg:" << leg << "legStartPartId:" << legStartPartId << "legEndPartId:" << legEndPartId << legStart.position << legEnd.position;
        averageLegPlaneTop += legStart.position;
        averageLegPlaneBottom += legEnd.position;
    }
    averageLegPlaneTop /= m_legs.size();
    averageLegPlaneBottom /= m_legs.size();
    m_legHeight = abs(averageLegPlaneBottom.y() - averageLegPlaneTop.y());
}
