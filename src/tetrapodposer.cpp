#include "tetrapodposer.h"

TetrapodPoser::TetrapodPoser(const std::vector<RiggerBone> &bones) :
    Poser(bones)
{
}

void TetrapodPoser::commit()
{
    for (const auto &item: parameters()) {
        int boneIndex = findBoneIndex(item.first);
        if (-1 == boneIndex) {
            continue;
        }
        auto findPitchResult = item.second.find("pitch");
        auto findYawResult = item.second.find("yaw");
        auto findRollResult = item.second.find("roll");
        if (findPitchResult != item.second.end() ||
                findYawResult != item.second.end() ||
                findRollResult != item.second.end()) {
            float yawAngle = valueOfKeyInMapOrEmpty(item.second, "yaw").toFloat();
            if (item.first.startsWith("Left")) {
                yawAngle = -yawAngle;
            }
            QQuaternion rotation = QQuaternion::fromEulerAngles(valueOfKeyInMapOrEmpty(item.second, "pitch").toFloat(),
                yawAngle,
                valueOfKeyInMapOrEmpty(item.second, "roll").toFloat());
            m_jointNodeTree.updateRotation(boneIndex, rotation);
            continue;
        }
        auto findIntersectionResult = item.second.find("intersection");
        if (findIntersectionResult != item.second.end()) {
            float intersectionAngle = valueOfKeyInMapOrEmpty(item.second, "intersection").toFloat();
            const RiggerBone &bone = bones()[boneIndex];
            if (bone.name == "LeftHand" || bone.name == "RightHand") {
                QVector3D handDirection = bone.tailPosition - bone.headPosition;
                QVector3D rotateAxis = QVector3D::crossProduct(handDirection, bone.name == "RightHand" ? QVector3D(1, 0, 0) : QVector3D(-1, 0, 0)).normalized();
                QQuaternion rotation = QQuaternion::fromAxisAndAngle(rotateAxis, intersectionAngle);
                m_jointNodeTree.updateRotation(boneIndex, rotation);
                continue;
            } else if (bone.name == "LeftLowerArm" || bone.name == "RightLowerArm") {
                QVector3D lowerArmDirection = bone.tailPosition - bone.headPosition;
                QVector3D rotateAxis = QVector3D::crossProduct(lowerArmDirection, QVector3D(0, 0, 1)).normalized();
                QQuaternion rotation = QQuaternion::fromAxisAndAngle(rotateAxis, intersectionAngle);
                m_jointNodeTree.updateRotation(boneIndex, rotation);
                continue;
            } else if (bone.name == "LeftFoot" || bone.name == "RightFoot") {
                QVector3D footDirection = bone.tailPosition - bone.headPosition;
                QVector3D rotateAxis = QVector3D::crossProduct(footDirection, QVector3D(0, 1, 0)).normalized();
                QQuaternion rotation = QQuaternion::fromAxisAndAngle(rotateAxis, intersectionAngle);
                m_jointNodeTree.updateRotation(boneIndex, rotation);
                continue;
            } else if (bone.name == "LeftLowerLeg" || bone.name == "RightLowerLeg") {
                QVector3D lowerLegDirection = bone.tailPosition - bone.headPosition;
                QVector3D rotateAxis = QVector3D::crossProduct(lowerLegDirection, QVector3D(0, 0, -1)).normalized();
                QQuaternion rotation = QQuaternion::fromAxisAndAngle(rotateAxis, intersectionAngle);
                m_jointNodeTree.updateRotation(boneIndex, rotation);
                continue;
            }
            continue;
        }
    }
    
    Poser::commit();
}

