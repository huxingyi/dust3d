#include <cmath>
#include "rigcontroller.h"
#include "ccdikresolver.h"

RigController::RigController(const JointNodeTree &jointNodeTree) :
    m_inputJointNodeTree(jointNodeTree),
    m_prepared(false),
    m_legHeight(0),
    m_averageLegEndY(0)
{
}

void RigController::saveFrame(RigFrame &frame)
{
    frame = m_rigFrame;
}

void RigController::collectParts()
{
    m_legs.clear();
    m_spine.clear();
    for (const auto &node: m_inputJointNodeTree.joints()) {
        if (node.boneMark == SkeletonBoneMark::Spine) {
            m_spine.push_back(std::make_pair(node.partId, node.nodeId));
        }
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
    
    collectParts();
    calculateAverageLegHeight();
}

void RigController::resetFrame()
{
    m_rigFrame = RigFrame(m_inputJointNodeTree.joints().size());
}

void RigController::lift(QVector3D offset)
{
    if (m_inputJointNodeTree.joints().empty())
        return;
    if (m_rigFrame.translatedIndicies.find(0) != m_rigFrame.translatedIndicies.end())
        m_rigFrame.updateTranslation(0, m_rigFrame.translations[0] + offset);
    else
        m_rigFrame.updateTranslation(0, m_inputJointNodeTree.joints()[0].translation + offset);
}

void RigController::breathe(float amount)
{
    if (m_inputJointNodeTree.joints().empty() || amount <= 0)
        return;
    std::vector<int> spineJoints;
    for (auto i = 0u; i < m_spine.size(); i++) {
        int jointIndex = m_inputJointNodeTree.nodeToJointIndex(m_spine[i].first, m_spine[i].second);
        spineJoints.push_back(jointIndex);
    }
    if (spineJoints.empty()) {
        // if no spine joints found, make the root node and its direct children as spine
        spineJoints.push_back(0);
        for (const auto &child: m_inputJointNodeTree.joints()[0].children)
            spineJoints.push_back(child);
    }
    if (spineJoints.empty())
        return;
    // make sure parent get processed first
    std::sort(spineJoints.begin(), spineJoints.end());
    float inverseAmount = 1 / amount;
    for (const auto &jointIndex: spineJoints) {
        m_rigFrame.updateScale(jointIndex, m_rigFrame.scales[jointIndex] * amount);
        for (const auto &child: m_inputJointNodeTree.joints()[jointIndex].children) {
            m_rigFrame.updateScale(child, m_rigFrame.scales[child] * inverseAmount);
        }
    }
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
    outputJointNodeTree.recalculateMatricesAfterPositionUpdated();
    for (int i = 0; i < nodeCount; i++) {
        int jointIndex = ikSolvingIndicies[i];
        const auto &outputJoint = outputJointNodeTree.joints()[jointIndex];
        m_rigFrame.updateRotation(jointIndex, outputJoint.rotation);
        m_rigFrame.updateTranslation(jointIndex, outputJoint.translation);
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
    if (frame.translatedIndicies.find(jointIndex) != frame.translatedIndicies.end())
        translateMatrix.translate(frame.translations[jointIndex]);
    else
        translateMatrix.translate(joint.translation);
    
    QMatrix4x4 rotateMatrix;
    if (frame.rotatedIndicies.find(jointIndex) != frame.rotatedIndicies.end())
        rotateMatrix.rotate(frame.rotations[jointIndex]);
    else
        rotateMatrix.rotate(joint.rotation);
    
    QMatrix4x4 scaleMatrix;
    if (frame.scaledIndicies.find(jointIndex) != frame.scaledIndicies.end())
        scaleMatrix.scale(frame.scales[jointIndex]);
    else
        scaleMatrix.scale(joint.scale);
    
    QMatrix4x4 worldMatrix = parentWorldMatrix * translateMatrix * rotateMatrix * scaleMatrix;
    matrices[jointIndex] = worldMatrix * joint.inverseBindMatrix;
    
    for (const auto &child: joint.children) {
        frameToMatricesAtJoint(frame, matrices, child, worldMatrix);
    }
}

void RigController::idle(float amount)
{
    prepare();
    
    QVector3D wantOffset;
    wantOffset.setY(m_legHeight * amount);
    QVector3D effectedOffset;
    liftLegs(wantOffset, effectedOffset);
    breathe(1 + amount);
    
    JointNodeTree finalJointNodeTree = m_inputJointNodeTree;
    applyRigFrameToJointNodeTree(finalJointNodeTree, m_rigFrame);
    
    QVector3D leftOffset;
    leftOffset.setY(calculateAverageLegEndPosition(finalJointNodeTree).y() - m_averageLegEndY);
    lift(-leftOffset);
}

void RigController::calculateAverageLegHeight()
{
    m_averageLegEndY = calculateAverageLegEndPosition(m_inputJointNodeTree).y();
    m_legHeight = abs(m_averageLegEndY - calculateAverageLegStartPosition(m_inputJointNodeTree).y());
}

QVector3D RigController::calculateAverageLegStartPosition(JointNodeTree &jointNodeTree)
{
    QVector3D averageLegPlaneTop = QVector3D();
    if (m_legs.empty())
        return QVector3D();
    for (auto leg = 0u; leg < m_legs.size(); leg++) {
        int legStartPartId = std::get<0>(m_legs[leg]);
        int legStartNodeId = std::get<1>(m_legs[leg]);
        int legStartIndex = jointNodeTree.nodeToJointIndex(legStartPartId, legStartNodeId);
        const auto &legStart = jointNodeTree.joints()[legStartIndex];
        averageLegPlaneTop += legStart.position;
    }
    averageLegPlaneTop /= m_legs.size();
    return averageLegPlaneTop;
}

QVector3D RigController::calculateAverageLegEndPosition(JointNodeTree &jointNodeTree)
{
    QVector3D averageLegPlaneBottom = QVector3D();
    if (m_legs.empty())
        return QVector3D();
    for (auto leg = 0u; leg < m_legs.size(); leg++) {
        int legEndPartId = std::get<2>(m_legs[leg]);
        int legEndNodeId = std::get<3>(m_legs[leg]);
        int legEndIndex = jointNodeTree.nodeToJointIndex(legEndPartId, legEndNodeId);
        const auto &legEnd = jointNodeTree.joints()[legEndIndex];
        averageLegPlaneBottom += legEnd.position;
    }
    averageLegPlaneBottom /= m_legs.size();
    return averageLegPlaneBottom;
}

void RigController::applyRigFrameToJointNodeTree(JointNodeTree &jointNodeTree, const RigFrame &frame)
{
    for (const auto &jointIndex: frame.translatedIndicies) {
        jointNodeTree.joints()[jointIndex].translation = frame.translations[jointIndex];
    }
    for (const auto &jointIndex: frame.rotatedIndicies) {
        jointNodeTree.joints()[jointIndex].rotation = frame.rotations[jointIndex];
    }
    for (const auto &jointIndex: frame.scaledIndicies) {
        jointNodeTree.joints()[jointIndex].scale = frame.scales[jointIndex];
    }
    jointNodeTree.recalculateMatricesAfterTransformUpdated();
}

