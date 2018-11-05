#include <QDebug>
#include <cmath>
#include "tetrapodrigger.h"

TetrapodRigger::TetrapodRigger(const std::vector<QVector3D> &verticesPositions,
        const std::set<MeshSplitterTriangle> &inputTriangles) :
    Rigger(verticesPositions, inputTriangles)
{
}

bool TetrapodRigger::isCutOffSplitter(BoneMark boneMark)
{
    return boneMark == BoneMark::Neck ||
        boneMark == BoneMark::Shoulder ||
        boneMark == BoneMark::Hip;
}

bool TetrapodRigger::rig()
{
    if (!validate())
        return false;
    
    std::set<MeshSplitterTriangle> bodyTriangles;
    if (!calculateBodyTriangles(bodyTriangles))
        return false;
    
    auto neckIndicies = m_marksMap.find(std::make_pair(BoneMark::Neck, SkeletonSide::None));
    auto leftShoulderIndicies = m_marksMap.find(std::make_pair(BoneMark::Shoulder, SkeletonSide::Left));
    auto leftElbowIndicies = m_marksMap.find(std::make_pair(BoneMark::Elbow, SkeletonSide::Left));
    auto leftWristIndicies = m_marksMap.find(std::make_pair(BoneMark::Wrist, SkeletonSide::Left));
    auto rightShoulderIndicies = m_marksMap.find(std::make_pair(BoneMark::Shoulder, SkeletonSide::Right));
    auto rightElbowIndicies = m_marksMap.find(std::make_pair(BoneMark::Elbow, SkeletonSide::Right));
    auto rightWristIndicies = m_marksMap.find(std::make_pair(BoneMark::Wrist, SkeletonSide::Right));
    auto leftHipIndicies = m_marksMap.find(std::make_pair(BoneMark::Hip, SkeletonSide::Left));
    auto leftKneeIndicies = m_marksMap.find(std::make_pair(BoneMark::Knee, SkeletonSide::Left));
    auto leftAnkleIndicies = m_marksMap.find(std::make_pair(BoneMark::Ankle, SkeletonSide::Left));
    auto rightHipIndicies = m_marksMap.find(std::make_pair(BoneMark::Hip, SkeletonSide::Right));
    auto rightKneeIndicies = m_marksMap.find(std::make_pair(BoneMark::Knee, SkeletonSide::Right));
    auto rightAnkleIndicies = m_marksMap.find(std::make_pair(BoneMark::Ankle, SkeletonSide::Right));
    
    // 1. Prepare all bones start and stop positions:
    
    QVector3D shouldersCenter = (m_marks[leftShoulderIndicies->second[0]].bonePosition +
        m_marks[rightShoulderIndicies->second[0]].bonePosition) / 2;
    QVector3D hipsCenter = (m_marks[leftHipIndicies->second[0]].bonePosition +
        m_marks[rightHipIndicies->second[0]].bonePosition) / 2;
    
    QVector3D bonesOrigin = hipsCenter;
    QVector3D chestBoneStartPosition = (shouldersCenter + hipsCenter) / 2;
    QVector3D neckBoneStartPosition = shouldersCenter;
    QVector3D headBoneStartPosition = m_marks[neckIndicies->second[0]].bonePosition;
    QVector3D leftUpperArmBoneStartPosition = m_marks[leftShoulderIndicies->second[0]].bonePosition;
    QVector3D leftLowerArmBoneStartPosition = m_marks[leftElbowIndicies->second[0]].bonePosition;
    QVector3D leftHandBoneStartPosition = m_marks[leftWristIndicies->second[0]].bonePosition;
    QVector3D rightUpperArmBoneStartPosition = m_marks[rightShoulderIndicies->second[0]].bonePosition;
    QVector3D rightLowerArmBoneStartPosition = m_marks[rightElbowIndicies->second[0]].bonePosition;
    QVector3D rightHandBoneStartPosition = m_marks[rightWristIndicies->second[0]].bonePosition;
    QVector3D leftUpperLegBoneStartPosition = m_marks[leftHipIndicies->second[0]].bonePosition;
    QVector3D leftLowerLegBoneStartPosition = m_marks[leftKneeIndicies->second[0]].bonePosition;
    QVector3D leftFootBoneStartPosition = m_marks[leftAnkleIndicies->second[0]].bonePosition;
    QVector3D rightUpperLegBoneStartPosition = m_marks[rightHipIndicies->second[0]].bonePosition;
    QVector3D rightLowerLegBoneStartPosition = m_marks[rightKneeIndicies->second[0]].bonePosition;
    QVector3D rightFootBoneStartPosition = m_marks[rightAnkleIndicies->second[0]].bonePosition;
    
    bool isMainBodyVerticalAligned = fabs(shouldersCenter.y() - hipsCenter.y()) > fabs(shouldersCenter.z() - hipsCenter.z());
    bool isLeftArmVerticalAligned = fabs(leftUpperArmBoneStartPosition.y() - leftHandBoneStartPosition.y()) > fabs(leftUpperArmBoneStartPosition.z() - leftHandBoneStartPosition.z());
    bool isRightArmVerticalAligned = fabs(rightUpperArmBoneStartPosition.y() - rightHandBoneStartPosition.y()) > fabs(rightUpperArmBoneStartPosition.z() - rightHandBoneStartPosition.z());
    
    std::set<int> headVertices;
    addTrianglesToVertices(m_marks[neckIndicies->second[0]].smallGroup(), headVertices);
    addTrianglesToVertices(m_marks[neckIndicies->second[0]].markTriangles, headVertices);
    QVector3D headBoneStopPosition;
    if (isMainBodyVerticalAligned) {
        QVector3D maxY = findMaxY(headVertices);
        headBoneStopPosition = QVector3D(headBoneStartPosition.x(),
            maxY.y(),
            maxY.z());
    } else {
        QVector3D maxZ = findMaxZ(headVertices);
        headBoneStopPosition = QVector3D(headBoneStartPosition.x(),
            maxZ.y(),
            maxZ.z());
    }
    
    std::set<int> leftArmVertices;
    addTrianglesToVertices(m_marks[leftShoulderIndicies->second[0]].smallGroup(), leftArmVertices);
    QVector3D leftHandBoneStopPosition;
    if (isLeftArmVerticalAligned) {
        QVector3D minY = findMinY(leftArmVertices);
        leftHandBoneStopPosition = QVector3D(leftHandBoneStartPosition.x(),
            minY.y(),
            minY.z());
    } else {
        QVector3D maxX = findMaxZ(leftArmVertices);
        leftHandBoneStopPosition = QVector3D(maxX.x(),
            maxX.y(),
            leftHandBoneStartPosition.z());
    }
    addTrianglesToVertices(m_marks[leftShoulderIndicies->second[0]].markTriangles, leftArmVertices);
    
    std::set<int> rightArmVertices;
    addTrianglesToVertices(m_marks[rightShoulderIndicies->second[0]].smallGroup(), rightArmVertices);
    QVector3D rightHandBoneStopPosition;
    if (isRightArmVerticalAligned) {
        QVector3D minY = findMinY(rightArmVertices);
        rightHandBoneStopPosition = QVector3D(rightHandBoneStartPosition.x(),
            minY.y(),
            minY.z());
    } else {
        QVector3D minX = findMinX(rightArmVertices);
        rightHandBoneStopPosition = QVector3D(minX.x(),
            minX.y(),
            rightHandBoneStartPosition.z());
    }
    addTrianglesToVertices(m_marks[rightShoulderIndicies->second[0]].markTriangles, rightArmVertices);
    
    std::set<int> leftLegVertices;
    QVector3D leftFootBoneStopPosition;
    addTrianglesToVertices(m_marks[leftHipIndicies->second[0]].smallGroup(), leftLegVertices);
    {
        QVector3D maxZ = findMaxZ(leftLegVertices);
        leftFootBoneStopPosition = QVector3D(leftFootBoneStartPosition.x(),
            maxZ.y(),
            maxZ.z());
    }
    addTrianglesToVertices(m_marks[leftHipIndicies->second[0]].markTriangles, leftLegVertices);
    
    std::set<int> rightLegVertices;
    QVector3D rightFootBoneStopPosition;
    addTrianglesToVertices(m_marks[rightHipIndicies->second[0]].smallGroup(), rightLegVertices);
    {
        QVector3D maxZ = findMaxZ(rightLegVertices);
        rightFootBoneStopPosition = QVector3D(rightFootBoneStartPosition.x(),
            maxZ.y(),
            maxZ.z());
    }
    addTrianglesToVertices(m_marks[rightHipIndicies->second[0]].markTriangles, rightLegVertices);
    
    // 2. Collect vertices for each bone:
    
    // 2.1 Collect vertices for neck bone:
    std::set<int> bodyVertices;
    addTrianglesToVertices(bodyTriangles, bodyVertices);
    addTrianglesToVertices(m_marks[leftShoulderIndicies->second[0]].markTriangles, bodyVertices);
    addTrianglesToVertices(m_marks[rightShoulderIndicies->second[0]].markTriangles, bodyVertices);
    
    std::set<int> bodyVerticesAfterShoulder;
    std::set<int> neckVertices;
    {
        std::set<int> shoulderMarkVertices;
        addTrianglesToVertices(m_marks[leftShoulderIndicies->second[0]].markTriangles, shoulderMarkVertices);
        addTrianglesToVertices(m_marks[rightShoulderIndicies->second[0]].markTriangles, shoulderMarkVertices);
        if (isMainBodyVerticalAligned) {
            QVector3D maxY = findMaxY(shoulderMarkVertices);
            splitVerticesByY(bodyVertices, maxY.y(), neckVertices, bodyVerticesAfterShoulder);
        } else {
            QVector3D maxZ = findMaxZ(shoulderMarkVertices);
            splitVerticesByZ(bodyVertices, maxZ.z(), neckVertices, bodyVerticesAfterShoulder);
        }
    }
    addTrianglesToVertices(m_marks[neckIndicies->second[0]].markTriangles, neckVertices);
    
    // 2.2 Collect vertices for chest bone:
    
    // Calculate neck's radius
    float neckRadius = 0;
    std::set<int> neckMarkVertices;
    addTrianglesToVertices(m_marks[neckIndicies->second[0]].markTriangles, neckMarkVertices);
    {
        QVector3D minX, minY, minZ, maxX, maxY, maxZ;
        resolveBoundingBox(neckMarkVertices, minX, maxX, minY, maxY, minZ, maxZ);
        neckRadius = fabs(minX.x() - maxX.x());
    }
    
    std::set<int> bodyVerticesAfterChest;
    std::set<int> chestVertices;
    if (isMainBodyVerticalAligned) {
        splitVerticesByY(bodyVerticesAfterShoulder, chestBoneStartPosition.y() - neckRadius, chestVertices, bodyVerticesAfterChest);
    } else {
        splitVerticesByZ(bodyVerticesAfterShoulder, chestBoneStartPosition.z() - neckRadius, chestVertices, bodyVerticesAfterChest);
    }
    
    // 2.3 Collect vertices for spine bone:
    std::set<int> bodyVerticesBeforeSpine;
    std::set<int> spineVertices;
    if (isMainBodyVerticalAligned) {
        splitVerticesByY(bodyVerticesAfterShoulder, chestBoneStartPosition.y() + neckRadius, bodyVerticesBeforeSpine, spineVertices);
    } else {
        splitVerticesByZ(bodyVerticesAfterShoulder, chestBoneStartPosition.z() + neckRadius, bodyVerticesBeforeSpine, spineVertices);
    }
    
    // 3. Collect vertices for arms:
    
    // 3.1.1 Collect vertices for left upper arm:
    std::set<int> leftElbowMarkVertices;
    addTrianglesToVertices(m_marks[leftElbowIndicies->second[0]].markTriangles, leftElbowMarkVertices);
    std::set<int> leftUpperArmVertices;
    {
        std::set<int> leftArmVerticesAfterElbow;
        if (isLeftArmVerticalAligned) {
            QVector3D minY = findMinY(leftElbowMarkVertices);
            splitVerticesByY(leftArmVertices, minY.y(), leftUpperArmVertices, leftArmVerticesAfterElbow);
        } else {
            QVector3D maxX = findMaxX(leftElbowMarkVertices);
            splitVerticesByX(leftArmVertices, maxX.x(), leftArmVerticesAfterElbow, leftUpperArmVertices);
        }
    }
    
    // 3.1.2 Collect vertices for left lower arm:
    std::set<int> leftArmVerticesSinceElbow;
    std::set<int> leftLowerArmVertices;
    {
        std::set<int> leftArmVerticesBeforeElbow;
        if (isLeftArmVerticalAligned) {
            splitVerticesByY(leftArmVertices, m_marks[leftElbowIndicies->second[0]].bonePosition.y(), leftArmVerticesBeforeElbow, leftArmVerticesSinceElbow);
        } else {
            splitVerticesByX(leftArmVertices, m_marks[leftElbowIndicies->second[0]].bonePosition.x(), leftArmVerticesSinceElbow, leftArmVerticesBeforeElbow);
        }
    }
    std::set<int> leftWristMarkVertices;
    addTrianglesToVertices(m_marks[leftWristIndicies->second[0]].markTriangles, leftWristMarkVertices);
    {
        std::set<int> leftArmVerticesAfterWrist;
        if (isLeftArmVerticalAligned) {
            QVector3D minY = findMinY(leftWristMarkVertices);
            splitVerticesByY(leftArmVerticesSinceElbow, minY.y(), leftLowerArmVertices, leftArmVerticesAfterWrist);
        } else {
            QVector3D maxX = findMaxX(leftWristMarkVertices);
            splitVerticesByX(leftArmVerticesSinceElbow, maxX.x(), leftArmVerticesAfterWrist, leftLowerArmVertices);
        }
    }
    
    // 3.1.3 Collect vertices for left hand:
    std::set<int> leftHandVertices;
    {
        std::set<int> leftArmVerticesBeforeWrist;
        if (isLeftArmVerticalAligned) {
            splitVerticesByY(leftArmVerticesSinceElbow, m_marks[leftWristIndicies->second[0]].bonePosition.y(), leftArmVerticesBeforeWrist, leftHandVertices);
        } else {
            splitVerticesByX(leftArmVerticesSinceElbow, m_marks[leftWristIndicies->second[0]].bonePosition.x(), leftHandVertices, leftArmVerticesBeforeWrist);
        }
    }
    
    // 3.2.1 Collect vertices for right upper arm:
    std::set<int> rightElbowMarkVertices;
    addTrianglesToVertices(m_marks[rightElbowIndicies->second[0]].markTriangles, rightElbowMarkVertices);
    std::set<int> rightUpperArmVertices;
    {
        std::set<int> rightArmVerticesAfterElbow;
        if (isRightArmVerticalAligned) {
            QVector3D minY = findMinY(rightElbowMarkVertices);
            splitVerticesByY(rightArmVertices, minY.y(), rightUpperArmVertices, rightArmVerticesAfterElbow);
        } else {
            QVector3D minX = findMinX(rightElbowMarkVertices);
            splitVerticesByX(rightArmVertices, minX.x(), rightUpperArmVertices, rightArmVerticesAfterElbow);
        }
    }
    
    // 3.2.2 Collect vertices for right lower arm:
    std::set<int> rightArmVerticesSinceElbow;
    std::set<int> rightLowerArmVertices;
    {
        std::set<int> rightArmVerticesBeforeElbow;
        if (isRightArmVerticalAligned) {
            splitVerticesByY(rightArmVertices, m_marks[rightElbowIndicies->second[0]].bonePosition.y(), rightArmVerticesBeforeElbow, rightArmVerticesSinceElbow);
        } else {
            splitVerticesByX(rightArmVertices, m_marks[rightElbowIndicies->second[0]].bonePosition.x(), rightArmVerticesBeforeElbow, rightArmVerticesSinceElbow);
        }
    }
    std::set<int> rightWristMarkVertices;
    addTrianglesToVertices(m_marks[rightWristIndicies->second[0]].markTriangles, rightWristMarkVertices);
    {
        std::set<int> rightArmVerticesAfterWrist;
        if (isRightArmVerticalAligned) {
            QVector3D minY = findMinY(rightWristMarkVertices);
            splitVerticesByY(rightArmVerticesSinceElbow, minY.y(), rightLowerArmVertices, rightArmVerticesAfterWrist);
        } else {
            QVector3D minX = findMinX(rightWristMarkVertices);
            splitVerticesByX(rightArmVerticesSinceElbow, minX.x(), rightLowerArmVertices, rightArmVerticesAfterWrist);
        }
    }
    
    // 3.2.3 Collect vertices for right hand:
    std::set<int> rightHandVertices;
    {
        std::set<int> rightArmVerticesBeforeWrist;
        if (isRightArmVerticalAligned) {
            splitVerticesByY(rightArmVerticesSinceElbow, m_marks[rightWristIndicies->second[0]].bonePosition.y(), rightArmVerticesBeforeWrist, rightHandVertices);
        } else {
            splitVerticesByX(rightArmVerticesSinceElbow, m_marks[rightWristIndicies->second[0]].bonePosition.x(), rightArmVerticesBeforeWrist, rightHandVertices);
        }
    }
    
    // 4. Collect vertices for legs:
    
    // 4.1.1 Collect vertices for left upper leg:
    std::set<int> leftKneeMarkVertices;
    addTrianglesToVertices(m_marks[leftKneeIndicies->second[0]].markTriangles, leftKneeMarkVertices);
    std::set<int> leftUpperLegVertices;
    {
        std::set<int> leftLegVerticesAfterKnee;
        QVector3D minY = findMinY(leftKneeMarkVertices);
        splitVerticesByY(leftLegVertices, minY.y(), leftUpperLegVertices, leftLegVerticesAfterKnee);
    }
    
    // 4.1.2 Collect vertices for left lower leg:
    std::set<int> leftLegVerticesSinceKnee;
    std::set<int> leftLowerLegVertices;
    {
        std::set<int> leftLegVerticesBeforeKnee;
        QVector3D maxY = findMaxY(leftKneeMarkVertices);
        splitVerticesByY(leftLegVertices, maxY.y(), leftLegVerticesBeforeKnee, leftLegVerticesSinceKnee);
    }
    std::set<int> leftAnkleMarkVertices;
    addTrianglesToVertices(m_marks[leftAnkleIndicies->second[0]].markTriangles, leftAnkleMarkVertices);
    {
        std::set<int> leftLegVerticesAfterAnkle;
        QVector3D maxY = findMaxY(leftAnkleMarkVertices);
        QVector3D minY = findMinY(leftAnkleMarkVertices);
        splitVerticesByY(leftLegVerticesSinceKnee, (maxY.y() + minY.y()) / 2, leftLowerLegVertices, leftLegVerticesAfterAnkle);
    }
    
    // 4.1.3 Collect vertices for left foot:
    std::set<int> leftFootVertices;
    {
        std::set<int> leftLegVerticesBeforeAnkle;
        QVector3D maxY = findMaxY(leftAnkleMarkVertices);
        QVector3D minY = findMinY(leftAnkleMarkVertices);
        splitVerticesByY(leftLegVerticesSinceKnee, (maxY.y() + minY.y()) / 2, leftLegVerticesBeforeAnkle, leftFootVertices);
    }
    
    // 4.2.1 Collect vertices for right upper leg:
    std::set<int> rightKneeMarkVertices;
    addTrianglesToVertices(m_marks[rightKneeIndicies->second[0]].markTriangles, rightKneeMarkVertices);
    std::set<int> rightUpperLegVertices;
    {
        std::set<int> rightLegVerticesAfterKnee;
        QVector3D minY = findMinY(rightKneeMarkVertices);
        splitVerticesByY(rightLegVertices, minY.y(), rightUpperLegVertices, rightLegVerticesAfterKnee);
    }
    
    // 4.2.2 Collect vertices for right lower leg:
    std::set<int> rightLegVerticesSinceKnee;
    std::set<int> rightLowerLegVertices;
    {
        std::set<int> rightLegVerticesBeforeKnee;
        QVector3D maxY = findMaxY(rightKneeMarkVertices);
        splitVerticesByY(rightLegVertices, maxY.y(), rightLegVerticesBeforeKnee, rightLegVerticesSinceKnee);
    }
    std::set<int> rightAnkleMarkVertices;
    addTrianglesToVertices(m_marks[rightAnkleIndicies->second[0]].markTriangles, rightAnkleMarkVertices);
    {
        std::set<int> rightLegVerticesAfterAnkle;
        QVector3D maxY = findMaxY(rightAnkleMarkVertices);
        QVector3D minY = findMinY(rightAnkleMarkVertices);
        splitVerticesByY(rightLegVerticesSinceKnee, (maxY.y() + minY.y()) / 2, rightLowerLegVertices, rightLegVerticesAfterAnkle);
    }
    
    // 4.2.3 Collect vertices for right foot:
    std::set<int> rightFootVertices;
    {
        std::set<int> rightLegVerticesBeforeAnkle;
        QVector3D maxY = findMaxY(rightAnkleMarkVertices);
        QVector3D minY = findMinY(rightAnkleMarkVertices);
        splitVerticesByY(rightLegVerticesSinceKnee, (maxY.y() + minY.y()) / 2, rightLegVerticesBeforeAnkle, rightFootVertices);
    }
    
    // 5. Generate bones
    std::map<QString, int> boneIndexMap;
    
    m_resultBones.push_back(RiggerBone());
    RiggerBone &bodyBone = m_resultBones.back();
    bodyBone.index = m_resultBones.size() - 1;
    bodyBone.name = Rigger::rootBoneName;
    bodyBone.headPosition = QVector3D(0, 0, 0);
    bodyBone.tailPosition = bonesOrigin;
    bodyBone.hasButton = true;
    bodyBone.button = {7, 1};
    bodyBone.buttonParameterType = RiggerButtonParameterType::Translation;
    boneIndexMap[bodyBone.name] = bodyBone.index;
    
    m_resultBones.push_back(RiggerBone());
    RiggerBone &leftHipBone = m_resultBones.back();
    leftHipBone.index = m_resultBones.size() - 1;
    leftHipBone.name = "LeftHip";
    leftHipBone.headPosition = m_resultBones[boneIndexMap[Rigger::rootBoneName]].tailPosition;
    leftHipBone.tailPosition = leftUpperLegBoneStartPosition;
    boneIndexMap[leftHipBone.name] = leftHipBone.index;
    m_resultBones[boneIndexMap[Rigger::rootBoneName]].children.push_back(leftHipBone.index);
    
    m_resultBones.push_back(RiggerBone());
    RiggerBone &leftUpperLegBone = m_resultBones.back();
    leftUpperLegBone.index = m_resultBones.size() - 1;
    leftUpperLegBone.name = "LeftUpperLeg";
    leftUpperLegBone.headPosition = m_resultBones[boneIndexMap["LeftHip"]].tailPosition;
    leftUpperLegBone.tailPosition = leftLowerLegBoneStartPosition;
    leftUpperLegBone.color = BoneMarkToColor(BoneMark::Hip);
    leftUpperLegBone.hasButton = true;
    leftUpperLegBone.button = {4, 2};
    leftUpperLegBone.buttonParameterType = RiggerButtonParameterType::PitchYawRoll;
    boneIndexMap[leftUpperLegBone.name] = leftUpperLegBone.index;
    m_resultBones[boneIndexMap["LeftHip"]].children.push_back(leftUpperLegBone.index);
    
    m_resultBones.push_back(RiggerBone());
    RiggerBone &leftLowerLegBone = m_resultBones.back();
    leftLowerLegBone.index = m_resultBones.size() - 1;
    leftLowerLegBone.name = "LeftLowerLeg";
    leftLowerLegBone.headPosition = m_resultBones[boneIndexMap["LeftUpperLeg"]].tailPosition;
    leftLowerLegBone.tailPosition = leftFootBoneStartPosition;
    leftLowerLegBone.color = BoneMarkToColor(BoneMark::Knee);
    leftLowerLegBone.hasButton = true;
    leftLowerLegBone.button = {5, 2};
    leftLowerLegBone.buttonParameterType = RiggerButtonParameterType::Intersection;
    boneIndexMap[leftLowerLegBone.name] = leftLowerLegBone.index;
    m_resultBones[boneIndexMap["LeftUpperLeg"]].children.push_back(leftLowerLegBone.index);
    
    m_resultBones.push_back(RiggerBone());
    RiggerBone &leftFootBone = m_resultBones.back();
    leftFootBone.index = m_resultBones.size() - 1;
    leftFootBone.name = "LeftFoot";
    leftFootBone.headPosition = m_resultBones[boneIndexMap["LeftLowerLeg"]].tailPosition;
    leftFootBone.tailPosition = leftFootBoneStopPosition;
    leftFootBone.color = BoneMarkToColor(BoneMark::Ankle);
    leftFootBone.hasButton = true;
    leftFootBone.button = {6, 2};
    leftFootBone.buttonParameterType = RiggerButtonParameterType::Intersection;
    boneIndexMap[leftFootBone.name] = leftFootBone.index;
    m_resultBones[boneIndexMap["LeftLowerLeg"]].children.push_back(leftFootBone.index);
    
    m_resultBones.push_back(RiggerBone());
    RiggerBone &rightHipBone = m_resultBones.back();
    rightHipBone.index = m_resultBones.size() - 1;
    rightHipBone.name = "RightHip";
    rightHipBone.headPosition = m_resultBones[boneIndexMap[Rigger::rootBoneName]].tailPosition;
    rightHipBone.tailPosition = rightUpperLegBoneStartPosition;
    boneIndexMap[rightHipBone.name] = rightHipBone.index;
    m_resultBones[boneIndexMap[Rigger::rootBoneName]].children.push_back(rightHipBone.index);
    
    m_resultBones.push_back(RiggerBone());
    RiggerBone &rightUpperLegBone = m_resultBones.back();
    rightUpperLegBone.index = m_resultBones.size() - 1;
    rightUpperLegBone.name = "RightUpperLeg";
    rightUpperLegBone.headPosition = m_resultBones[boneIndexMap["RightHip"]].tailPosition;
    rightUpperLegBone.tailPosition = rightLowerLegBoneStartPosition;
    rightUpperLegBone.color = BoneMarkToColor(BoneMark::Hip);
    rightUpperLegBone.hasButton = true;
    rightUpperLegBone.button = {4, 0};
    rightUpperLegBone.buttonParameterType = RiggerButtonParameterType::PitchYawRoll;
    boneIndexMap[rightUpperLegBone.name] = rightUpperLegBone.index;
    m_resultBones[boneIndexMap["RightHip"]].children.push_back(rightUpperLegBone.index);
    
    m_resultBones.push_back(RiggerBone());
    RiggerBone &rightLowerLegBone = m_resultBones.back();
    rightLowerLegBone.index = m_resultBones.size() - 1;
    rightLowerLegBone.name = "RightLowerLeg";
    rightLowerLegBone.headPosition = m_resultBones[boneIndexMap["RightUpperLeg"]].tailPosition;
    rightLowerLegBone.tailPosition = rightFootBoneStartPosition;
    rightLowerLegBone.color = BoneMarkToColor(BoneMark::Knee);
    rightLowerLegBone.hasButton = true;
    rightLowerLegBone.button = {5, 0};
    rightLowerLegBone.buttonParameterType = RiggerButtonParameterType::Intersection;
    boneIndexMap[rightLowerLegBone.name] = rightLowerLegBone.index;
    m_resultBones[boneIndexMap["RightUpperLeg"]].children.push_back(rightLowerLegBone.index);
    
    m_resultBones.push_back(RiggerBone());
    RiggerBone &rightFootBone = m_resultBones.back();
    rightFootBone.index = m_resultBones.size() - 1;
    rightFootBone.name = "RightFoot";
    rightFootBone.headPosition = m_resultBones[boneIndexMap["RightLowerLeg"]].tailPosition;
    rightFootBone.tailPosition = rightFootBoneStopPosition;
    rightFootBone.color = BoneMarkToColor(BoneMark::Ankle);
    rightFootBone.hasButton = true;
    rightFootBone.button = {6, 0};
    rightFootBone.buttonParameterType = RiggerButtonParameterType::Intersection;
    boneIndexMap[rightFootBone.name] = rightFootBone.index;
    m_resultBones[boneIndexMap["RightLowerLeg"]].children.push_back(rightFootBone.index);
    
    m_resultBones.push_back(RiggerBone());
    RiggerBone &spineBone = m_resultBones.back();
    spineBone.index = m_resultBones.size() - 1;
    spineBone.name = "Spine";
    spineBone.headPosition = m_resultBones[boneIndexMap[Rigger::rootBoneName]].tailPosition;
    spineBone.tailPosition = chestBoneStartPosition;
    spineBone.color = Qt::white;
    spineBone.hasButton = true;
    spineBone.button = {3, 1};
    spineBone.buttonParameterType = RiggerButtonParameterType::PitchYawRoll;
    boneIndexMap[spineBone.name] = spineBone.index;
    m_resultBones[boneIndexMap[Rigger::rootBoneName]].children.push_back(spineBone.index);
    
    m_resultBones.push_back(RiggerBone());
    RiggerBone &chestBone = m_resultBones.back();
    chestBone.index = m_resultBones.size() - 1;
    chestBone.name = "Chest";
    chestBone.headPosition = m_resultBones[boneIndexMap["Spine"]].tailPosition;
    chestBone.tailPosition = neckBoneStartPosition;
    chestBone.color = QColor(0x57, 0x43, 0x98);
    chestBone.hasButton = true;
    chestBone.button = {2, 1};
    chestBone.buttonParameterType = RiggerButtonParameterType::PitchYawRoll;
    boneIndexMap[chestBone.name] = chestBone.index;
    m_resultBones[boneIndexMap["Spine"]].children.push_back(chestBone.index);
    
    m_resultBones.push_back(RiggerBone());
    RiggerBone &leftShoulderBone = m_resultBones.back();
    leftShoulderBone.index = m_resultBones.size() - 1;
    leftShoulderBone.name = "LeftShoulder";
    leftShoulderBone.headPosition = m_resultBones[boneIndexMap["Chest"]].tailPosition;
    leftShoulderBone.tailPosition = leftUpperArmBoneStartPosition;
    boneIndexMap[leftShoulderBone.name] = leftShoulderBone.index;
    m_resultBones[boneIndexMap["Chest"]].children.push_back(leftShoulderBone.index);
    
    m_resultBones.push_back(RiggerBone());
    RiggerBone &leftUpperArmBone = m_resultBones.back();
    leftUpperArmBone.index = m_resultBones.size() - 1;
    leftUpperArmBone.name = "LeftUpperArm";
    leftUpperArmBone.headPosition = m_resultBones[boneIndexMap["LeftShoulder"]].tailPosition;
    leftUpperArmBone.tailPosition = leftLowerArmBoneStartPosition;
    leftUpperArmBone.color = BoneMarkToColor(BoneMark::Shoulder);
    leftUpperArmBone.hasButton = true;
    leftUpperArmBone.button = {1, 2};
    leftUpperArmBone.buttonParameterType = RiggerButtonParameterType::PitchYawRoll;
    boneIndexMap[leftUpperArmBone.name] = leftUpperArmBone.index;
    m_resultBones[boneIndexMap["LeftShoulder"]].children.push_back(leftUpperArmBone.index);
    
    m_resultBones.push_back(RiggerBone());
    RiggerBone &leftLowerArmBone = m_resultBones.back();
    leftLowerArmBone.index = m_resultBones.size() - 1;
    leftLowerArmBone.name = "LeftLowerArm";
    leftLowerArmBone.headPosition = m_resultBones[boneIndexMap["LeftUpperArm"]].tailPosition;
    leftLowerArmBone.tailPosition = leftHandBoneStartPosition;
    leftLowerArmBone.color = BoneMarkToColor(BoneMark::Elbow);
    leftLowerArmBone.hasButton = true;
    leftLowerArmBone.button = {2, 2};
    leftLowerArmBone.buttonParameterType = RiggerButtonParameterType::Intersection;
    boneIndexMap[leftLowerArmBone.name] = leftLowerArmBone.index;
    m_resultBones[boneIndexMap["LeftUpperArm"]].children.push_back(leftLowerArmBone.index);
    
    m_resultBones.push_back(RiggerBone());
    RiggerBone &leftHandBone = m_resultBones.back();
    leftHandBone.index = m_resultBones.size() - 1;
    leftHandBone.name = "LeftHand";
    leftHandBone.headPosition = m_resultBones[boneIndexMap["LeftLowerArm"]].tailPosition;
    leftHandBone.tailPosition = leftHandBoneStopPosition;
    leftHandBone.color = BoneMarkToColor(BoneMark::Wrist);
    leftHandBone.hasButton = true;
    leftHandBone.button = {3, 2};
    leftHandBone.buttonParameterType = RiggerButtonParameterType::Intersection;
    boneIndexMap[leftHandBone.name] = leftHandBone.index;
    m_resultBones[boneIndexMap["LeftLowerArm"]].children.push_back(leftHandBone.index);
    
    m_resultBones.push_back(RiggerBone());
    RiggerBone &rightShoulderBone = m_resultBones.back();
    rightShoulderBone.index = m_resultBones.size() - 1;
    rightShoulderBone.name = "RightShoulder";
    rightShoulderBone.headPosition = m_resultBones[boneIndexMap["Chest"]].tailPosition;
    rightShoulderBone.tailPosition = rightUpperArmBoneStartPosition;
    boneIndexMap[rightShoulderBone.name] = rightShoulderBone.index;
    m_resultBones[boneIndexMap["Chest"]].children.push_back(rightShoulderBone.index);
    
    m_resultBones.push_back(RiggerBone());
    RiggerBone &rightUpperArmBone = m_resultBones.back();
    rightUpperArmBone.index = m_resultBones.size() - 1;
    rightUpperArmBone.name = "RightUpperArm";
    rightUpperArmBone.headPosition = m_resultBones[boneIndexMap["RightShoulder"]].tailPosition;
    rightUpperArmBone.tailPosition = rightLowerArmBoneStartPosition;
    rightUpperArmBone.color = BoneMarkToColor(BoneMark::Shoulder);
    rightUpperArmBone.hasButton = true;
    rightUpperArmBone.button = {1, 0};
    rightUpperArmBone.buttonParameterType = RiggerButtonParameterType::PitchYawRoll;
    boneIndexMap[rightUpperArmBone.name] = rightUpperArmBone.index;
    m_resultBones[boneIndexMap["RightShoulder"]].children.push_back(rightUpperArmBone.index);
    
    m_resultBones.push_back(RiggerBone());
    RiggerBone &rightLowerArmBone = m_resultBones.back();
    rightLowerArmBone.index = m_resultBones.size() - 1;
    rightLowerArmBone.name = "RightLowerArm";
    rightLowerArmBone.headPosition = m_resultBones[boneIndexMap["RightUpperArm"]].tailPosition;
    rightLowerArmBone.tailPosition = rightHandBoneStartPosition;
    rightLowerArmBone.color = BoneMarkToColor(BoneMark::Elbow);
    rightLowerArmBone.hasButton = true;
    rightLowerArmBone.button = {2, 0};
    rightLowerArmBone.buttonParameterType = RiggerButtonParameterType::Intersection;
    boneIndexMap[rightLowerArmBone.name] = rightLowerArmBone.index;
    m_resultBones[boneIndexMap["RightUpperArm"]].children.push_back(rightLowerArmBone.index);
    
    m_resultBones.push_back(RiggerBone());
    RiggerBone &rightHandBone = m_resultBones.back();
    rightHandBone.index = m_resultBones.size() - 1;
    rightHandBone.name = "RightHand";
    rightHandBone.headPosition = m_resultBones[boneIndexMap["RightLowerArm"]].tailPosition;
    rightHandBone.tailPosition = rightHandBoneStopPosition;
    rightHandBone.color = BoneMarkToColor(BoneMark::Wrist);
    rightHandBone.hasButton = true;
    rightHandBone.button = {3, 0};
    rightHandBone.buttonParameterType = RiggerButtonParameterType::Intersection;
    boneIndexMap[rightHandBone.name] = rightHandBone.index;
    m_resultBones[boneIndexMap["RightLowerArm"]].children.push_back(rightHandBone.index);
    
    m_resultBones.push_back(RiggerBone());
    RiggerBone &neckBone = m_resultBones.back();
    neckBone.index = m_resultBones.size() - 1;
    neckBone.name = "Neck";
    neckBone.headPosition = m_resultBones[boneIndexMap["Chest"]].tailPosition;
    neckBone.tailPosition = headBoneStartPosition;
    neckBone.color = BoneMarkToColor(BoneMark::Neck);
    neckBone.hasButton = true;
    neckBone.button = {1, 1};
    neckBone.buttonParameterType = RiggerButtonParameterType::PitchYawRoll;
    boneIndexMap[neckBone.name] = neckBone.index;
    m_resultBones[boneIndexMap["Chest"]].children.push_back(neckBone.index);
    
    m_resultBones.push_back(RiggerBone());
    RiggerBone &headBone = m_resultBones.back();
    headBone.index = m_resultBones.size() - 1;
    headBone.name = "Head";
    headBone.headPosition = m_resultBones[boneIndexMap["Neck"]].tailPosition;
    headBone.tailPosition = headBoneStopPosition;
    headBone.color = QColor(0xfb, 0xef, 0x8b);
    headBone.hasButton = true;
    headBone.button = {0, 1};
    headBone.buttonParameterType = RiggerButtonParameterType::PitchYawRoll;
    boneIndexMap[headBone.name] = headBone.index;
    m_resultBones[boneIndexMap["Neck"]].children.push_back(headBone.index);
    
    // 6. Calculate weights for vertices
    addVerticesToWeights(headVertices, boneIndexMap["Head"]);
    addVerticesToWeights(neckVertices, boneIndexMap["Neck"]);
    addVerticesToWeights(chestVertices, boneIndexMap["Chest"]);
    addVerticesToWeights(spineVertices, boneIndexMap["Spine"]);
    addVerticesToWeights(leftUpperArmVertices, boneIndexMap["LeftUpperArm"]);
    addVerticesToWeights(leftLowerArmVertices, boneIndexMap["LeftLowerArm"]);
    addVerticesToWeights(leftHandVertices, boneIndexMap["LeftHand"]);
    addVerticesToWeights(rightUpperArmVertices, boneIndexMap["RightUpperArm"]);
    addVerticesToWeights(rightLowerArmVertices, boneIndexMap["RightLowerArm"]);
    addVerticesToWeights(rightHandVertices, boneIndexMap["RightHand"]);
    addVerticesToWeights(leftUpperLegVertices, boneIndexMap["LeftUpperLeg"]);
    addVerticesToWeights(leftLowerLegVertices, boneIndexMap["LeftLowerLeg"]);
    addVerticesToWeights(leftFootVertices, boneIndexMap["LeftFoot"]);
    addVerticesToWeights(rightUpperLegVertices, boneIndexMap["RightUpperLeg"]);
    addVerticesToWeights(rightLowerLegVertices, boneIndexMap["RightLowerLeg"]);
    addVerticesToWeights(rightFootVertices, boneIndexMap["RightFoot"]);
    
    for (auto &weights: m_resultWeights) {
        weights.second.finalizeWeights();
    }
    
    return true;
}

bool TetrapodRigger::validate()
{
    bool foundError = false;
    
    std::vector<std::pair<BoneMark, SkeletonSide>> mustPresentedMarks;
    
    mustPresentedMarks.push_back(std::make_pair(BoneMark::Neck, SkeletonSide::None));
    mustPresentedMarks.push_back(std::make_pair(BoneMark::Shoulder, SkeletonSide::Left));
    mustPresentedMarks.push_back(std::make_pair(BoneMark::Elbow, SkeletonSide::Left));
    mustPresentedMarks.push_back(std::make_pair(BoneMark::Wrist, SkeletonSide::Left));
    mustPresentedMarks.push_back(std::make_pair(BoneMark::Shoulder, SkeletonSide::Right));
    mustPresentedMarks.push_back(std::make_pair(BoneMark::Elbow, SkeletonSide::Right));
    mustPresentedMarks.push_back(std::make_pair(BoneMark::Wrist, SkeletonSide::Right));
    mustPresentedMarks.push_back(std::make_pair(BoneMark::Hip, SkeletonSide::Left));
    mustPresentedMarks.push_back(std::make_pair(BoneMark::Knee, SkeletonSide::Left));
    mustPresentedMarks.push_back(std::make_pair(BoneMark::Ankle, SkeletonSide::Left));
    mustPresentedMarks.push_back(std::make_pair(BoneMark::Hip, SkeletonSide::Right));
    mustPresentedMarks.push_back(std::make_pair(BoneMark::Knee, SkeletonSide::Right));
    mustPresentedMarks.push_back(std::make_pair(BoneMark::Ankle, SkeletonSide::Right));
    
    for (const auto &pair: mustPresentedMarks) {
        if (m_marksMap.find(pair) == m_marksMap.end()) {
            foundError = true;
            QString markDispName = SkeletonSideToDispName(pair.second) + " " + BoneMarkToDispName(pair.first);
            m_missingMarkNames.push_back(markDispName);
            m_messages.push_back(std::make_pair(QtCriticalMsg,
                tr("Couldn't find valid \"%1\" mark").arg(markDispName)));
        }
    }
    
    if (foundError)
        return false;
    
    if (!m_errorMarkNames.empty() || !m_missingMarkNames.empty())
        return false;
    
    return true;
}

