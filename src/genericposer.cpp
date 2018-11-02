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
