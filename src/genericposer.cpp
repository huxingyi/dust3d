#include <unordered_map>
#include "genericposer.h"

GenericPoser::GenericPoser(const std::vector<RiggerBone> &bones) :
    Poser(bones)
{
}

void GenericPoser::commit()
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
            QVector3D axis = bone.baseNormal;
            QQuaternion rotation = QQuaternion::fromAxisAndAngle(axis, intersectionAngle);
            m_jointNodeTree.updateRotation(boneIndex, rotation);
            continue;
        }
    }
    
    Poser::commit();
}
