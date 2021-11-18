#include "material.h"
#include "image_forever.h"

void initializeMaterialTexturesFromSnapshot(const dust3d::Snapshot &snapshot,
    const dust3d::Uuid &materialId,
    MaterialTextures &materialTextures,
    float &tileScale)
{
    auto materialIdString = materialId.toString();
    for (const auto &materialItem: snapshot.materials) {
        if (materialIdString != dust3d::String::valueOrEmpty(materialItem.first, "id"))
            continue;
        for (const auto &layer: materialItem.second) {
            //FIXME: Only support one layer currently
            auto findTileScale = layer.first.find("tileScale");
            if (findTileScale != layer.first.end())
                tileScale = dust3d::String::toFloat(findTileScale->second);
            for (const auto &mapItem: layer.second) {
                auto textureType = dust3d::TextureTypeFromString(dust3d::String::valueOrEmpty(mapItem, "for").c_str());
                if (textureType != dust3d::TextureType::None) {
                    int index = (int)textureType - 1;
                    if (index >= 0 && index < (int)dust3d::TextureType::Count - 1) {
                        if ("imageId" == dust3d::String::valueOrEmpty(mapItem, "linkDataType")) {
                            auto imageIdString = dust3d::String::valueOrEmpty(mapItem, "linkData");
                            materialTextures.textureImages[index] = ImageForever::get(dust3d::Uuid(imageIdString));
                        }
                    }
                }
            }
            break;
        }
        break;
    }
}
