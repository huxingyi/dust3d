#include "tube_uv_preview_image_generator.h"

const size_t TubeUvPreviewImageGenerator::m_uvImageSize = 1024;

TubeUvPreviewImageGenerator::TubeUvPreviewImageGenerator(std::vector<std::vector<dust3d::Vector2>>&& faceUvs)
    : m_faceUvs(faceUvs)
{
}

void TubeUvPreviewImageGenerator::generate()
{
    m_previewImage = std::make_unique<QImage>(m_uvImageSize, m_uvImageSize, QImage::Format_ARGB32);
    // TODO:
}

void TubeUvPreviewImageGenerator::process()
{
    generate();
    emit finished();
}
