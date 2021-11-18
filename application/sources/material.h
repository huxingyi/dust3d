#ifndef DUST3D_APPLICATION_MATERIAL_H_
#define DUST3D_APPLICATION_MATERIAL_H_

#include <QImage>
#include <dust3d/base/uuid.h>
#include <dust3d/base/texture_type.h>
#include <dust3d/base/snapshot.h>

struct MaterialTextures
{
    const QImage *textureImages[(int)dust3d::TextureType::Count - 1] = {nullptr};
};

void initializeMaterialTexturesFromSnapshot(const dust3d::Snapshot &snapshot,
    const dust3d::Uuid &materialId,
    MaterialTextures &materialTextures,
    float &tileScale);

#endif
