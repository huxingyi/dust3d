#ifndef DUST3D_APPLICATION_MATERIAL_LAYER_H_
#define DUST3D_APPLICATION_MATERIAL_LAYER_H_

#include <vector>
#include <dust3d/base/texture_type.h>
#include <dust3d/base/uuid.h>

class MaterialMap
{
public:
    dust3d::TextureType forWhat;
    dust3d::Uuid imageId;
};

class MaterialLayer
{
public:
    std::vector<MaterialMap> maps;
    float tileScale = 1.0;
};

#endif
