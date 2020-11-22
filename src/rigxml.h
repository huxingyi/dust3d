#ifndef DUST3D_RIG_XML_H
#define DUST3D_RIG_XML_H
#include <QXmlStreamWriter>
#include "rig.h"
#include "object.h"

void saveRigToXmlStream(const Object *object,
    const std::vector<RigBone> *rigBones, 
    const std::map<int, RigVertexWeights> *rigWeights, 
    QXmlStreamWriter *writer);

#endif
