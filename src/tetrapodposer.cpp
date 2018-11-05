#include <cmath>
#include <QtMath>
#include "tetrapodposer.h"
#include "util.h"

TetrapodPoser::TetrapodPoser(const std::vector<RiggerBone> &bones) :
    Poser(bones)
{
}

void TetrapodPoser::resolveTranslation()
{
    for (const auto &item: parameters()) {
        int boneIndex = findBoneIndex(item.first);
        if (-1 == boneIndex) {
            continue;
        }
        auto findTranslateXResult = item.second.find("translateX");
        auto findTranslateYResult = item.second.find("translateY");
        auto findTranslateZResult = item.second.find("translateZ");
        if (findTranslateXResult != item.second.end() ||
                findTranslateYResult != item.second.end() ||
                findTranslateZResult != item.second.end()) {
            float x = valueOfKeyInMapOrEmpty(item.second, "translateX").toFloat();
            float y = valueOfKeyInMapOrEmpty(item.second, "translateY").toFloat();
            float z = valueOfKeyInMapOrEmpty(item.second, "translateZ").toFloat();
            QVector3D translation = {x, y, z};
            m_jointNodeTree.addTranslation(boneIndex, translation);
            continue;
        }
    }
    resolveLimbRotation({QString("LeftUpperArm"), QString("LeftLowerArm"), QString("LeftHand")});
    resolveLimbRotation({QString("RightUpperArm"), QString("RightLowerArm"), QString("RightHand")});
    resolveLimbRotation({QString("LeftUpperLeg"), QString("LeftLowerLeg"), QString("LeftFoot")});
    resolveLimbRotation({QString("RightUpperLeg"), QString("RightLowerLeg"), QString("RightFoot")});
    resolveLimbRotation({QString("Spine"), QString("Chest"), QString("Neck"), QString("Head")});
}

std::pair<bool, QVector3D> TetrapodPoser::findQVector3DFromMap(const std::map<QString, QString> &map, const QString &xName, const QString &yName, const QString &zName)
{
    auto findXResult = map.find(xName);
    auto findYResult = map.find(yName);
    auto findZResult = map.find(zName);
    if (findXResult == map.end() &&
            findYResult == map.end() &&
            findZResult == map.end()) {
        return {false, QVector3D()};
    }
    return {true, {
        valueOfKeyInMapOrEmpty(map, xName).toFloat(),
        valueOfKeyInMapOrEmpty(map, yName).toFloat(),
        valueOfKeyInMapOrEmpty(map, zName).toFloat()
    }};
}

std::pair<bool, std::pair<QVector3D, QVector3D>> TetrapodPoser::findBonePositionsFromParameters(const std::map<QString, QString> &map)
{
    auto findBoneStartResult = findQVector3DFromMap(map, "fromX", "fromY", "fromZ");
    auto findBoneStopResult = findQVector3DFromMap(map, "toX", "toY", "toZ");
    
    if (!findBoneStartResult.first || !findBoneStopResult.first)
        return {false, {QVector3D(), QVector3D()}};
    
    return {true, {findBoneStartResult.second, findBoneStopResult.second}};
}

void TetrapodPoser::resolveLimbRotation(const std::vector<QString> &limbBoneNames)
{
    // We match the poses by the distance and rotation plane

    if (limbBoneNames.size() < 3) {
        qDebug() << "Cann't resolve limb bones with invalid joints:" << limbBoneNames.size();
        return;
    }
    const auto &beginBoneName = limbBoneNames[0];
    const auto &middleBoneName = limbBoneNames[1];
    const auto &endBoneName = limbBoneNames[2];
    
    const auto &beginBoneParameters = parameters().find(beginBoneName);
    if (beginBoneParameters == parameters().end()) {
        qDebug() << beginBoneName << "'s parameters not found";
        return;
    }
    
    auto matchBeginBonePositions = findBonePositionsFromParameters(beginBoneParameters->second);
    if (!matchBeginBonePositions.first) {
        qDebug() << beginBoneName << "'s positions not found";
        return;
    }
    
    const auto &endBoneParameters = parameters().find(endBoneName);
    if (endBoneParameters == parameters().end()) {
        qDebug() << endBoneName << "'s parameters not found";
        return;
    }
    
    auto matchEndBonePositions = findBonePositionsFromParameters(endBoneParameters->second);
    if (!matchEndBonePositions.first) {
        qDebug() << endBoneName << "'s positions not found";
        return;
    }

    float matchLimbLength = (matchBeginBonePositions.second.first - matchBeginBonePositions.second.second).length() +
         (matchBeginBonePositions.second.second - matchEndBonePositions.second.first).length();
    
    auto matchDistanceBetweenBeginAndEndBones = (matchBeginBonePositions.second.first - matchEndBonePositions.second.first).length();
    auto matchRotatePlaneNormal = QVector3D::crossProduct((matchBeginBonePositions.second.second - matchBeginBonePositions.second.first).normalized(), (matchEndBonePositions.second.first - matchBeginBonePositions.second.second).normalized());
    
    auto matchDirectionBetweenBeginAndEndPones = (matchEndBonePositions.second.first - matchBeginBonePositions.second.first).normalized();
    
    auto matchEndBoneDirection = (matchEndBonePositions.second.second - matchEndBonePositions.second.first).normalized();
    
    int beginBoneIndex = findBoneIndex(beginBoneName);
    if (-1 == beginBoneIndex) {
        qDebug() << beginBoneName << "not found in rigged bones";
        return;
    }
    const auto &beginBone = bones()[beginBoneIndex];
    
    int middleBoneIndex = findBoneIndex(middleBoneName);
    if (-1 == middleBoneIndex) {
        qDebug() << middleBoneName << "not found in rigged bones";
        return;
    }
    
    int endBoneIndex = findBoneIndex(endBoneName);
    if (-1 == endBoneIndex) {
        qDebug() << endBoneName << "not found in rigged bones";
        return;
    }
    const auto &endBone = bones()[endBoneIndex];
    
    float targetBeginBoneLength = (beginBone.headPosition - beginBone.tailPosition).length();
    float targetMiddleBoneLength = (beginBone.tailPosition - endBone.headPosition).length();
    float targetLimbLength = targetBeginBoneLength + targetMiddleBoneLength;
    
    float targetDistanceBetweenBeginAndEndBones = matchDistanceBetweenBeginAndEndBones * (targetLimbLength / matchLimbLength);
    QVector3D targetEndBoneStartPosition = beginBone.headPosition + matchDirectionBetweenBeginAndEndPones * targetDistanceBetweenBeginAndEndBones;
    
    float angleBetweenDistanceAndMiddleBones = 0;
    {
        const float &a = targetMiddleBoneLength;
        const float &b = targetDistanceBetweenBeginAndEndBones;
        const float &c = targetBeginBoneLength;
        double cosC = (a*a + b*b - c*c) / (2.0*a*b);
        angleBetweenDistanceAndMiddleBones = qRadiansToDegrees(acos(cosC));
    }
    
    QVector3D targetMiddleBoneStartPosition;
    {
        qDebug() << beginBoneName << "Angle:" << angleBetweenDistanceAndMiddleBones;
        auto rotation = QQuaternion::fromAxisAndAngle(matchRotatePlaneNormal, angleBetweenDistanceAndMiddleBones);
        targetMiddleBoneStartPosition = targetEndBoneStartPosition + rotation.rotatedVector(-matchDirectionBetweenBeginAndEndPones).normalized() * targetMiddleBoneLength;
    }
    
    // Now the bones' positions have been resolved, we calculate the rotation
    
    auto oldBeginBoneDirection = (beginBone.tailPosition - beginBone.headPosition).normalized();
    auto newBeginBoneDirection = (targetMiddleBoneStartPosition - beginBone.headPosition).normalized();
    auto beginBoneRotation = QQuaternion::rotationTo(oldBeginBoneDirection, newBeginBoneDirection);
    m_jointNodeTree.updateRotation(beginBoneIndex, beginBoneRotation);
    
    auto oldMiddleBoneDirection = (endBone.headPosition - beginBone.tailPosition).normalized();
    auto newMiddleBoneDirection = (targetEndBoneStartPosition - targetMiddleBoneStartPosition).normalized();
    oldMiddleBoneDirection = beginBoneRotation.rotatedVector(oldMiddleBoneDirection);
    auto middleBoneRotation = QQuaternion::rotationTo(oldMiddleBoneDirection, newMiddleBoneDirection);
    m_jointNodeTree.updateRotation(middleBoneIndex, middleBoneRotation);
    
    // Calculate the end effectors' rotation
    auto oldEndBoneDirection = (endBone.tailPosition - endBone.headPosition).normalized();
    auto newEndBoneDirection = matchEndBoneDirection;
    oldEndBoneDirection = beginBoneRotation.rotatedVector(oldEndBoneDirection);
    oldEndBoneDirection = middleBoneRotation.rotatedVector(oldEndBoneDirection);
    auto endBoneRotation = QQuaternion::rotationTo(oldEndBoneDirection, newEndBoneDirection);
    m_jointNodeTree.updateRotation(endBoneIndex, endBoneRotation);
    
    if (limbBoneNames.size() > 3) {
        std::vector<QQuaternion> rotations;
        rotations.push_back(beginBoneRotation);
        rotations.push_back(middleBoneRotation);
        rotations.push_back(endBoneRotation);
        for (size_t i = 3; i < limbBoneNames.size(); ++i) {
            const auto &boneName = limbBoneNames[i];
            int boneIndex = findBoneIndex(boneName);
            if (-1 == boneIndex) {
                qDebug() << "Find bone failed:" << boneName;
                continue;
            }
            const auto &bone = bones()[boneIndex];
            const auto &boneParameters = parameters().find(boneName);
            if (boneParameters == parameters().end()) {
                qDebug() << "Find bone parameters:" << boneName;
                continue;
            }
            auto matchBonePositions = findBonePositionsFromParameters(boneParameters->second);
            if (!matchBonePositions.first) {
                qDebug() << "Find bone positions failed:" << boneName;
                continue;
            }
            auto matchBoneDirection = (matchBonePositions.second.second - matchBonePositions.second.first).normalized();
            auto oldBoneDirection = (bone.tailPosition - bone.headPosition).normalized();
            auto newBoneDirection = matchBoneDirection;
            for (const auto &rotation: rotations) {
                oldBoneDirection = rotation.rotatedVector(oldBoneDirection);
            }
            auto boneRotation = QQuaternion::rotationTo(oldBoneDirection, newBoneDirection);
            m_jointNodeTree.updateRotation(boneIndex, boneRotation);
            rotations.push_back(boneRotation);
        }
    }
}

void TetrapodPoser::commit()
{
    resolveTranslation();
    
    Poser::commit();
}

