#ifndef DUST3D_APPLICATION_BONE_STRUCTURE_H_
#define DUST3D_APPLICATION_BONE_STRUCTURE_H_

#include <dust3d/rig/rig_generator.h>
#include <QString>
#include <vector>

// Qt wrapper for dust3d::RigNode with QString for UI compatibility
struct BoneNode {
    QString name;
    QString parent;
    float posX, posY, posZ;
    float endX, endY, endZ;

    BoneNode() : posX(0), posY(0), posZ(0), endX(0), endY(0), endZ(0) {}
    BoneNode(const dust3d::RigNode& rigNode)
        : name(QString::fromStdString(rigNode.name)),
          parent(QString::fromStdString(rigNode.parent)),
          posX(rigNode.posX), posY(rigNode.posY), posZ(rigNode.posZ),
          endX(rigNode.endX), endY(rigNode.endY), endZ(rigNode.endZ) {}

    dust3d::RigNode toRigNode() const {
        dust3d::RigNode node;
        node.name = name.toStdString();
        node.parent = parent.toStdString();
        node.posX = posX;
        node.posY = posY;
        node.posZ = posZ;
        node.endX = endX;
        node.endY = endY;
        node.endZ = endZ;
        return node;
    }
};

// Qt wrapper for dust3d::RigStructure with QString for UI compatibility
struct RigStructure {
    QString type;
    QString name;
    QString description;
    std::vector<BoneNode> bones;

    RigStructure() = default;
    RigStructure(const dust3d::RigStructure& rigStruct)
        : type(QString::fromStdString(rigStruct.type)),
          name(QString::fromStdString(rigStruct.name)),
          description(QString::fromStdString(rigStruct.description)) {
        for (const auto& bone : rigStruct.bones) {
            bones.push_back(BoneNode(bone));
        }
    }

    dust3d::RigStructure toRigStructure() const {
        dust3d::RigStructure rigStruct;
        rigStruct.type = type.toStdString();
        rigStruct.name = name.toStdString();
        rigStruct.description = description.toStdString();
        for (const auto& bone : bones) {
            rigStruct.bones.push_back(bone.toRigNode());
        }
        return rigStruct;
    }
};

// Qt wrapper for dust3d::VertexBoneBinding
struct VertexBoneBinding {
    QString bone1;
    float weight1 = 1.0f;
    QString bone2;
    float weight2 = 0.0f;

    VertexBoneBinding() = default;
    VertexBoneBinding(const QString& b1, float w1 = 1.0f, const QString& b2 = QString(), float w2 = 0.0f)
        : bone1(b1), weight1(w1), bone2(b2), weight2(w2) {}

    VertexBoneBinding(const dust3d::VertexBoneBinding& binding)
        : bone1(QString::fromStdString(binding.bone1)),
          weight1(binding.weight1),
          bone2(QString::fromStdString(binding.bone2)),
          weight2(binding.weight2) {}

    bool isValid() const { return !bone1.isEmpty(); }
    bool isSingleBone() const { return bone2.isEmpty(); }
};

// Qt wrapper for dust3d::NodeBoneInfluence
struct NodeBoneInfluence {
    QString primary;
    QString secondary;
    float lerp = 0.0f;

    NodeBoneInfluence() = default;
    NodeBoneInfluence(const QString& p, const QString& s = QString(), float l = 0.0f)
        : primary(p), secondary(s), lerp(l) {}

    NodeBoneInfluence(const dust3d::NodeBoneInfluence& influence)
        : primary(QString::fromStdString(influence.primary)),
          secondary(QString::fromStdString(influence.secondary)),
          lerp(influence.lerp) {}

    bool isValid() const { return !primary.isEmpty(); }
    bool isSingleBone() const { return secondary.isEmpty(); }

    VertexBoneBinding toVertexBinding() const {
        if (isSingleBone()) {
            return VertexBoneBinding(primary, 1.0f);
        } else {
            float w1 = 1.0f - lerp;
            float w2 = lerp;
            return VertexBoneBinding(primary, w1, secondary, w2);
        }
    }
};

#endif
