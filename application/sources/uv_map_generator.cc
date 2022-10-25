#include "uv_map_generator.h"
#include "image_forever.h"
#include <dust3d/uv/uv_map_packer.h>

UvMapGenerator::UvMapGenerator(std::unique_ptr<dust3d::Object> object, std::unique_ptr<dust3d::Snapshot> snapshot)
    : m_object(std::move(object))
    , m_snapshot(std::move(snapshot))
{
}

void UvMapGenerator::process()
{
    generate();
    emit finished();
}

std::unique_ptr<dust3d::Object> UvMapGenerator::takeObject()
{
    return std::move(m_object);
}

void UvMapGenerator::generate()
{
    if (nullptr == m_object)
        return;

    if (nullptr == m_snapshot)
        return;

    m_mapPacker = std::make_unique<dust3d::UvMapPacker>();

    for (const auto& partIt : m_snapshot->parts) {
        const auto& colorImageIdIt = partIt.second.find("colorImageId");
        if (colorImageIdIt == partIt.second.end())
            continue;
        auto imageId = dust3d::Uuid(colorImageIdIt->second);
        const QImage* image = ImageForever::get(imageId);
        if (nullptr == image)
            continue;
        const auto& findUvs = m_object->partTriangleUvs.find(dust3d::Uuid(partIt.first));
        if (findUvs == m_object->partTriangleUvs.end())
            continue;
        dust3d::UvMapPacker::Part part;
        part.id = imageId;
        part.width = image->width();
        part.height = image->height();
        part.localUv = findUvs->second;
        m_mapPacker->addPart(part);
    }

    m_mapPacker->pack();

    // TODO:
}
