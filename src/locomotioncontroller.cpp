#include <cmath>
#include "locomotioncontroller.h"
#include "ikjoint.h"
#include "dust3dutil.h"

LocomotionController::LocomotionController(const JointNodeTree &jointNodeTree) :
    m_inputJointNodeTree(jointNodeTree),
    m_outputJointNodeTree(jointNodeTree)
{
}

LocomotionController::~LocomotionController()
{
    release();
}

void LocomotionController::release()
{
    m_leftPogoSticks.clear();
    m_rightPogoSticks.clear();
}

void LocomotionController::prepare()
{
    release();
    
    for (const auto &leg: m_inputJointNodeTree.leftLegs()) {
        const auto &legStart = m_inputJointNodeTree.joints()[leg.first];
        const auto &legEnd = m_inputJointNodeTree.joints()[leg.second];
        
        std::vector<int> childrenOfLegEnd;
        m_inputJointNodeTree.collectChildren(leg.second, childrenOfLegEnd);
        m_childrenOfLeftLegEnds.push_back(childrenOfLegEnd);
        
        PogoStick pogoStick;
        pogoStick.setGroundLocation(legEnd.position.y());
        pogoStick.setRestPelvisLocation(legStart.position.y());
        m_leftPogoSticks.push_back(pogoStick);
    }
    
    for (const auto &leg: m_inputJointNodeTree.rightLegs()) {
        const auto &legStart = m_inputJointNodeTree.joints()[leg.first];
        const auto &legEnd = m_inputJointNodeTree.joints()[leg.second];
        
        std::vector<int> childrenOfLegEnd;
        m_inputJointNodeTree.collectChildren(leg.second, childrenOfLegEnd);
        m_childrenOfRightLegEnds.push_back(childrenOfLegEnd);
        
        PogoStick pogoStick;
        pogoStick.setGroundLocation(legEnd.position.y());
        pogoStick.setRestPelvisLocation(legStart.position.y());
        m_rightPogoSticks.push_back(pogoStick);
    }
}

void LocomotionController::simulateLeg(PogoStick *pogoStick, const std::vector<int> &childrenOfLegEnd, std::pair<int, int> leg, float amount, QVector3D *footDirection, QVector3D *finalLegStartPosition, float *finalLegStartOffsetY)
{
    float time = amount;
    
    pogoStick->simulate(time);
    
    const auto &legStart = m_outputJointNodeTree.joints()[leg.first];
    const auto &legEnd = m_outputJointNodeTree.joints()[leg.second];
    
    float targetLegStartY = legStart.position.y() + (pogoStick->currentPelvisLocation() - legStart.position.y()) * 0.2;
    float targetLegEndY = pogoStick->currentFootLocation();
    float targetLegStartZ = legStart.position.z() + pogoStick->currentFootHorizontalOffset() * 0.05;
    float targetLegEndZ = legEnd.position.z() + pogoStick->currentFootHorizontalOffset();
    
    QVector3D targetLegStartPosition = QVector3D(legStart.position.x(), targetLegStartY, targetLegStartZ);
    QVector3D targetLegEndPosition = QVector3D(legEnd.position.x(), targetLegEndY, targetLegEndZ);
    QVector3D initialLegStartPosition = legStart.position;
    QVector3D initialLegEndPosition = legEnd.position;
    
    std::vector<int> tracePath;
    int legParentIndex = m_inputJointNodeTree.findHubJoint(leg.first, &tracePath);
    if (-1 == legParentIndex) {
        m_outputJointNodeTree.joints()[leg.first].position = targetLegStartPosition;
    } else {
        moveIkJoints(m_outputJointNodeTree, m_outputJointNodeTree, legParentIndex, leg.first, targetLegStartPosition);
    }
    
    moveIkJoints(m_outputJointNodeTree, m_outputJointNodeTree, leg.first, leg.second, targetLegEndPosition);
    
    QVector3D finalLegEndTranslation = legEnd.position - initialLegEndPosition;

    for (const auto &child: childrenOfLegEnd) {
        m_outputJointNodeTree.joints()[child].position += finalLegEndTranslation;
    }
    
    if (m_outputJointNodeTree.joints()[leg.first].children.size() > 0) {
        int theNodeIndexDirectBelowLegStart = m_outputJointNodeTree.joints()[leg.first].children[0];
        if (footDirection)
            *footDirection = m_outputJointNodeTree.joints()[theNodeIndexDirectBelowLegStart].position - legStart.position;
    } else {
        if (footDirection)
            *footDirection = legEnd.position - legStart.position;
    }
    
    if (finalLegStartPosition)
        *finalLegStartPosition = legEnd.position;
    if (finalLegStartOffsetY)
        *finalLegStartOffsetY = legStart.position.y() - initialLegStartPosition.y();
}

void LocomotionController::simulate(float amount)
{
    const auto pointerFront = QVector3D(0, 0, 1);
    const auto pointerOutFromCanvas = QVector3D(1, 0, 0);
    const auto pointerUp = QVector3D(0, 1, 0);
    float offset = 0.3;
    float delays[2] = {0};
    m_outputJointNodeTree = m_inputJointNodeTree;
    std::vector<QVector3D> leftPitches;
    std::vector<QVector3D> rightPitches;
    std::vector<QVector3D> leftLegStartPositions;
    std::vector<QVector3D> rightLegStartPositions;
    std::vector<float> leftLegStartOffsetYs;
    std::vector<float> rightLegStartOffsetYs;
    leftPitches.resize(m_leftPogoSticks.size());
    leftLegStartPositions.resize(m_leftPogoSticks.size());
    leftLegStartOffsetYs.resize(m_leftPogoSticks.size());
    for (int i = 0; i < (int)m_leftPogoSticks.size(); i++) {
        const auto &leg = m_inputJointNodeTree.leftLegs()[i];
        auto pogoStick = &m_leftPogoSticks[i];
        QVector3D footDirection;
        simulateLeg(pogoStick, m_childrenOfLeftLegEnds[i], leg, amount + delays[i % 2], &footDirection, &leftLegStartPositions[i], &leftLegStartOffsetYs[i]);
        leftPitches[i] = -QVector3D::crossProduct(-footDirection, pointerOutFromCanvas);
        delays[i % 2] += offset;
    }
    delays[0] = 0.5;
    delays[1] = 0.5;
    rightPitches.resize(m_rightPogoSticks.size());
    rightLegStartPositions.resize(m_rightPogoSticks.size());
    rightLegStartOffsetYs.resize(m_rightPogoSticks.size());
    for (int i = 0; i < (int)m_rightPogoSticks.size(); i++) {
        const auto &leg = m_inputJointNodeTree.rightLegs()[i];
        auto pogoStick = &m_rightPogoSticks[i];
        QVector3D footDirection;
        simulateLeg(pogoStick, m_childrenOfRightLegEnds[i], leg, amount + delays[i % 2], &footDirection, &rightLegStartPositions[i], &rightLegStartOffsetYs[i]);
        rightPitches[i] = -QVector3D::crossProduct(-footDirection, pointerOutFromCanvas);
        delays[i % 2] += offset;
    }
    
    if (m_inputJointNodeTree.spine().empty())
        return;
    
    // Calculate orientation for each leg pair
    std::vector<QVector3D> spineOrientations;
    spineOrientations.resize(std::max(leftLegStartPositions.size(), rightLegStartPositions.size()), pointerFront);
    for (int i = 0; i < (int)leftLegStartPositions.size() && i < (int)rightLegStartPositions.size(); i++) {
        auto rotateDirection = -QVector3D::crossProduct(rightLegStartPositions[i] - leftLegStartPositions[i], pointerUp);
        spineOrientations[i] = (rotateDirection + pointerFront + pointerFront).normalized();
    }
    
    // Calculate average pitch for each leg
    std::vector<QVector3D> spinePitches;
    spinePitches.resize(std::max(leftPitches.size(), rightPitches.size()));
    for (int i = 0; i < (int)m_leftPogoSticks.size(); i++) {
        spinePitches[i] = leftPitches[i];
    }
    for (int i = 0; i < (int)m_rightPogoSticks.size(); i++) {
        spinePitches[i] += rightPitches[i];
    }
    for (int i = 0; i < (int)spinePitches.size(); i++) {
        spinePitches[i].normalize();
        if (i < (int)spineOrientations.size()) {
            auto quaternion = QQuaternion::rotationTo(pointerFront, spineOrientations[i]);
            spinePitches[i] = quaternion.rotatedVector(spinePitches[i]);
        }
    }
    
    // Find spine node for each leg pair
    std::vector<int> spineNodes;
    spineNodes.resize(spinePitches.size(), -1);
    for (int i = 0; i < (int)spinePitches.size(); i++) {
        int leftSpine = -1;
        if (i < (int)m_inputJointNodeTree.leftLegs().size()) {
            leftSpine = m_inputJointNodeTree.findSpineFromChild(m_inputJointNodeTree.leftLegs()[i].first);
        }
        int rightSpine = -1;
        if (i < (int)m_inputJointNodeTree.rightLegs().size()) {
            rightSpine = m_inputJointNodeTree.findSpineFromChild(m_inputJointNodeTree.rightLegs()[i].first);
        }
        if (-1 == leftSpine && -1 == rightSpine)
            continue;
        if (leftSpine == rightSpine) {
            spineNodes[i] = leftSpine;
            continue;
        }
        if (-1 == rightSpine) {
            spineNodes[i] = leftSpine;
            continue;
        }
        if (-1 == leftSpine) {
            spineNodes[i] = rightSpine;
            continue;
        }
        qDebug() << "leftSpine:" << leftSpine << " not equal rightSpine:" << rightSpine;
        if (m_inputJointNodeTree.joints()[leftSpine].position.z() > m_inputJointNodeTree.joints()[rightSpine].position.z()) {
            spineNodes[i] = leftSpine;
            continue;
        }
        spineNodes[i] = rightSpine;
    }
    
    // Merge leg start offsets
    std::vector<QVector3D> legStartOffsets;
    legStartOffsets.resize(std::max(leftLegStartOffsetYs.size(), rightLegStartOffsetYs.size()));
    for (int i = 0; i < (int)spineNodes.size(); i++) {
        float offsetY = 0;
        if (i < (int)leftLegStartOffsetYs.size()) {
            offsetY += leftLegStartOffsetYs[i];
        }
        if (i < (int)rightLegStartOffsetYs.size()) {
            offsetY += rightLegStartOffsetYs[i];
        }
        offsetY /= 2;
        offsetY *= 0.5;
        legStartOffsets[i] = QVector3D(0, offsetY, 0);
    }
    
    // Move spine nodes
    for (int i = 0; i < (int)spineNodes.size(); i++) {
        if (i < (int)legStartOffsets.size()) {
            QVector3D offset = legStartOffsets[i];
            m_outputJointNodeTree.joints()[spineNodes[i]].position += offset;
            std::vector<int> children;
            m_inputJointNodeTree.collectTrivialChildren(spineNodes[i], children);
            for (const auto &child: children) {
                m_outputJointNodeTree.joints()[child].position += offset;
            }
        }
    }
    
    // Move head
    if (-1 != m_inputJointNodeTree.head && spineNodes.size() > 0) {
        const auto offset = legStartOffsets[0];
        m_outputJointNodeTree.joints()[m_inputJointNodeTree.head].position += offset;
        std::vector<int> headChildren;
        m_inputJointNodeTree.collectChildren(m_inputJointNodeTree.head, headChildren);
        for (const auto &child: headChildren) {
            m_outputJointNodeTree.joints()[child].position += offset;
        }
    }
    
    // Move tail
    if (-1 != m_inputJointNodeTree.tail && spineNodes.size() > 0) {
        int lastSpineJointIndex = spineNodes[spineNodes.size() - 1];
        std::vector<int> spineIndicies;
        if (lastSpineJointIndex == m_inputJointNodeTree.findParent(m_inputJointNodeTree.tail,
                lastSpineJointIndex, &spineIndicies)) {
            std::vector<int> tailChildren;
            m_inputJointNodeTree.collectChildren(m_inputJointNodeTree.tail, tailChildren);
            const auto offset = legStartOffsets[spineNodes.size() - 1];
            for (int i = (int)spineIndicies.size() - 2; i >= 0; i--) {
                m_outputJointNodeTree.joints()[spineIndicies[i]].position += offset;
            }
            m_outputJointNodeTree.joints()[m_inputJointNodeTree.tail].position += offset;
            for (const auto &child: tailChildren) {
                m_outputJointNodeTree.joints()[child].position += offset;
            }
        }
    }
    
    // Adjust neck
    if (-1 != m_inputJointNodeTree.head && spineNodes.size() > 0 && spinePitches.size() > 0) {
        int firstSpineJointIndex = spineNodes[0];
        int headParent = m_inputJointNodeTree.joints()[m_inputJointNodeTree.head].parentIndex;
        if (-1 != headParent) {
            makeInbetweenNodesInHermiteCurve(m_inputJointNodeTree.head,
                m_outputJointNodeTree.joints()[m_inputJointNodeTree.head].position - m_outputJointNodeTree.joints()[headParent].position,
                firstSpineJointIndex,
                spinePitches[0]);
        }
    }
    
    // Adjust spine inbetween legs
    for (int i = 1; i < (int)spineNodes.size() && i < (int)spinePitches.size(); i++) {
        makeInbetweenNodesInHermiteCurve(spineNodes[i - 1], spinePitches[i - 1],
            spineNodes[i], spinePitches[i]);
    }
}

void LocomotionController::makeInbetweenNodesInHermiteCurve(int firstJointIndex, QVector3D firstPitch, int secondJointIndex, QVector3D secondPitch)
{
    int child = firstJointIndex;
    int parent = secondJointIndex;
    QVector3D childPitch = firstPitch;
    QVector3D parentPitch = secondPitch;
    if (child < parent) {
        child = secondJointIndex;
        parent = firstJointIndex;
        childPitch = secondPitch;
        parentPitch = firstPitch;
    }
    
    std::vector<int> inbetweenIndicies;
    if (parent != m_inputJointNodeTree.findParent(child,
            parent, &inbetweenIndicies)) {
        return;
    }
    if (inbetweenIndicies.size() - 1 <= 0)
        return;
    
    // Calculate total length
    float length = m_inputJointNodeTree.joints()[inbetweenIndicies[0]].position.distanceToPoint(m_inputJointNodeTree.joints()[child].position);
    std::vector<float> lengths;
    float totalLength = length;
    lengths.push_back(length);
    for (int i = 1; i <= (int)inbetweenIndicies.size() - 1; i++) {
        length = m_inputJointNodeTree.joints()[inbetweenIndicies[i - 1]].position.distanceToPoint(m_inputJointNodeTree.joints()[inbetweenIndicies[i]].position);
        lengths.push_back(length);
        totalLength += length;
    }
    if (qFuzzyIsNull(totalLength)) {
        qDebug() << "Inbetween nodes is too short";
    } else {
        float accumLength = 0;
        const QVector3D &p0 = m_inputJointNodeTree.joints()[parent].position;
        const QVector3D &m0 = parentPitch;
        const QVector3D &p1 = m_inputJointNodeTree.joints()[child].position;
        const QVector3D &m1 = childPitch;
        for (int i = (int)inbetweenIndicies.size() - 2; i >= 0; i--) {
            accumLength += lengths[i + 1];
            float t = (accumLength / totalLength) * 0.8;
            QVector3D revisedPosition = pointInHermiteCurve(t, p0, m0, p1, m1);
            qDebug() << "pointInHermiteCurve(t:<<" << t << ", p0:" << p0 << ", m0:" << m0 << ", p1:" << p1 << ", m1:" << m1 << ") result:" << revisedPosition;
            auto &inbetweenJointPosition = m_outputJointNodeTree.joints()[inbetweenIndicies[i]].position;
            QVector3D translation = revisedPosition - inbetweenJointPosition;
            inbetweenJointPosition = revisedPosition;
            std::vector<int> trivialChildren;
            m_inputJointNodeTree.collectTrivialChildren(inbetweenIndicies[i], trivialChildren);
            for (const auto &child: trivialChildren) {
                m_outputJointNodeTree.joints()[child].position += translation;
            }
        }
    }
}

const JointNodeTree &LocomotionController::outputJointNodeTreeOnlyPositions() const
{
    return m_outputJointNodeTree;
}
