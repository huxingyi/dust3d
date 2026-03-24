#ifndef DUST3D_APPLICATION_BONE_STRUCTURE_H_
#define DUST3D_APPLICATION_BONE_STRUCTURE_H_

#include <QString>
#include <vector>

struct BoneNode {
    QString name;
    QString parent;
    float posX, posY, posZ;
    float endX, endY, endZ;
};

struct RigStructure {
    QString type;
    QString name;
    QString description;
    std::vector<BoneNode> bones;
};

#endif
