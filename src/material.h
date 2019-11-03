#ifndef DUST3D_MATERIAL_H
#define DUST3D_MATERIAL_H
#include <QImage>
#include <QUuid>
#include "texturetype.h"
#include "snapshot.h"

struct MaterialTextures
{
    const QImage *textureImages[(int)TextureType::Count - 1] = {nullptr};
};

void initializeMaterialTexturesFromSnapshot(const Snapshot &snapshot,
    const QUuid &materialId,
    MaterialTextures &materialTextures,
    float &tileScale);

#endif
