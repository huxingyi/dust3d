#ifndef MATERIAL_H
#define MATERIAL_H
#include <QImage>
#include <QUuid>
#include "texturetype.h"
#include "skeletonsnapshot.h"

struct Material
{
    QColor color;
    QUuid materialId;
};

struct MaterialTextures
{
    const QImage *textureImages[(int)TextureType::Count - 1] = {nullptr};
};

void initializeMaterialTexturesFromSnapshot(const SkeletonSnapshot &snapshot, const QUuid &materialId, MaterialTextures &materialTextures);

#endif
