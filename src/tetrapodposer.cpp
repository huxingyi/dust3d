#include "tetrapodposer.h"
#include "util.h"

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
            QQuaternion rotation = eulerAnglesToQuaternion(valueOfKeyInMapOrEmpty(item.second, "pitch").toFloat(),
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
                QVector3D referenceDirection = bone.name == "RightHand" ? QVector3D(1, 0, 0) : QVector3D(-1, 0, 0);
                auto angleWithX = (int)angleBetweenVectors(handDirection, -referenceDirection);
                auto angleWithZ = (int)angleBetweenVectors(handDirection, QVector3D(0, 0, -1));
                QVector3D rotateAxis = angleWithX < angleWithZ ?
                    QVector3D::crossProduct(handDirection, referenceDirection).normalized() :
                    QVector3D::crossProduct(handDirection, QVector3D(0, 0, -1)).normalized();
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
        auto findXResult = item.second.find("x");
        auto findYResult = item.second.find("y");
        auto findZResult = item.second.find("z");
        if (findXResult != item.second.end() ||
                findYResult != item.second.end() ||
                findZResult != item.second.end()) {
            float x = valueOfKeyInMapOrEmpty(item.second, "x").toFloat();
            float y = valueOfKeyInMapOrEmpty(item.second, "y").toFloat();
            float z = valueOfKeyInMapOrEmpty(item.second, "z").toFloat();
            QVector3D translation = {x, y, z};
            m_jointNodeTree.addTranslation(boneIndex, translation);
            continue;
        }
    }
    
    Poser::commit();
}

