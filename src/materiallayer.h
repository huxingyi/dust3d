#ifndef DUST3D_MATERIAL_LAYER_H
#define DUST3D_MATERIAL_LAYER_H
#include <QUuid>
#include <vector>
#include "texturetype.h"

class MaterialMap
{
public:
    TextureType forWhat;
    QUuid imageId;
};

class MaterialLayer
{
public:
    std::vector<MaterialMap> maps;
    float tileScale = 1.0;
};

#endif
