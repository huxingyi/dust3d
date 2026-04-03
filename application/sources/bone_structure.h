#ifndef DUST3D_APPLICATION_BONE_STRUCTURE_H_
#define DUST3D_APPLICATION_BONE_STRUCTURE_H_

#include <QString>
#include <dust3d/rig/rig_generator.h>
#include <vector>

// Qt wrapper for dust3d::RigNode with QString for UI compatibility
struct BoneNode {
    QString name;
    QString parent;
    float posX, posY, posZ;
    float endX, endY, endZ;
    float capsuleRadius;

    BoneNode()
        : posX(0)
        , posY(0)
        , posZ(0)
        , endX(0)
        , endY(0)
        , endZ(0)
        , capsuleRadius(0)
    {
    }
    BoneNode(const dust3d::RigNode& rigNode)
        : name(QString::fromStdString(rigNode.name))
        , parent(QString::fromStdString(rigNode.parent))
        , posX(rigNode.posX)
        , posY(rigNode.posY)
        , posZ(rigNode.posZ)
        , endX(rigNode.endX)
        , endY(rigNode.endY)
        , endZ(rigNode.endZ)
        , capsuleRadius(rigNode.capsuleRadius)
    {
    }

    dust3d::RigNode toRigNode() const
    {
        dust3d::RigNode node;
        node.name = name.toStdString();
        node.parent = parent.toStdString();
        node.posX = posX;
        node.posY = posY;
        node.posZ = posZ;
        node.endX = endX;
        node.endY = endY;
        node.endZ = endZ;
        node.capsuleRadius = capsuleRadius;
        return node;
    }
};

// Qt wrapper for dust3d::RigStructure with QString for UI compatibility
struct RigStructure {
    QString type;
    QString description;
    std::vector<BoneNode> bones;

    RigStructure() = default;
    RigStructure(const dust3d::RigStructure& rigStruct)
        : type(QString::fromStdString(rigStruct.type))
        , description(QString::fromStdString(rigStruct.description))
    {
        for (const auto& bone : rigStruct.bones) {
            bones.push_back(BoneNode(bone));
        }
    }

    dust3d::RigStructure toRigStructure() const
    {
        dust3d::RigStructure rigStruct;
        rigStruct.type = type.toStdString();
        rigStruct.description = description.toStdString();
        for (const auto& bone : bones) {
            rigStruct.bones.push_back(bone.toRigNode());
        }
        return rigStruct;
    }
};

#endif
