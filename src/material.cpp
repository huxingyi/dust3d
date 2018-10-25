#include "material.h"
#include "imageforever.h"
#include "util.h"

void initializeMaterialTexturesFromSnapshot(const Snapshot &snapshot, const QUuid &materialId, MaterialTextures &materialTextures)
{
    QString materialIdString = materialId.toString();
    for (const auto &materialItem: snapshot.materials) {
        if (materialIdString != valueOfKeyInMapOrEmpty(materialItem.first, "id"))
            continue;
        for (const auto &layer: materialItem.second) {
            //FIXME: Only support one layer currently
            for (const auto &mapItem: layer.second) {
                auto textureType = TextureTypeFromString(valueOfKeyInMapOrEmpty(mapItem, "for").toUtf8().constData());
                if (textureType != TextureType::None) {
                    int index = (int)textureType - 1;
                    if (index >= 0 && index < (int)TextureType::Count - 1) {
                        if ("imageId" == valueOfKeyInMapOrEmpty(mapItem, "linkDataType")) {
                            auto imageIdString = valueOfKeyInMapOrEmpty(mapItem, "linkData");
                            materialTextures.textureImages[index] = ImageForever::get(QUuid(imageIdString));
                        }
                    }
                }
            }
            break;
        }
        break;
    }
}
