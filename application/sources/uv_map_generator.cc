#include "uv_map_generator.h"
#include "image_forever.h"
#include <QMatrix>
#include <QPainter>
#include <dust3d/uv/uv_map_packer.h>
#include <unordered_set>

size_t UvMapGenerator::m_textureSize = 4096;

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

std::unique_ptr<QImage> UvMapGenerator::takeResultTextureColorImage()
{
    return std::move(m_textureColorImage);
}

std::unique_ptr<QImage> UvMapGenerator::takeResultTextureNormalImage()
{
    return std::move(m_textureNormalImage);
}

std::unique_ptr<QImage> UvMapGenerator::takeResultTextureRoughnessImage()
{
    return std::move(m_textureRoughnessImage);
}

std::unique_ptr<QImage> UvMapGenerator::takeResultTextureMetalnessImage()
{
    return std::move(m_textureMetalnessImage);
}

std::unique_ptr<QImage> UvMapGenerator::takeResultTextureAmbientOcclusionImage()
{
    return std::move(m_textureAmbientOcclusionImage);
}

std::unique_ptr<ModelMesh> UvMapGenerator::takeResultMesh()
{
    return std::move(m_mesh);
}

std::unique_ptr<dust3d::Object> UvMapGenerator::takeObject()
{
    return std::move(m_object);
}

bool UvMapGenerator::hasTransparencySettings() const
{
    return m_hasTransparencySettings;
}

QImage* UvMapGenerator::combineMetalnessRoughnessAmbientOcclusionImages(QImage* metalnessImage,
    QImage* roughnessImage,
    QImage* ambientOcclusionImage)
{
    QImage* textureMetalnessRoughnessAmbientOcclusionImage = nullptr;
    if (nullptr != metalnessImage || nullptr != roughnessImage || nullptr != ambientOcclusionImage) {
        int textureSize = 0;
        if (nullptr != metalnessImage)
            textureSize = metalnessImage->height();
        if (nullptr != roughnessImage)
            textureSize = roughnessImage->height();
        if (nullptr != ambientOcclusionImage)
            textureSize = ambientOcclusionImage->height();
        if (textureSize > 0) {
            textureMetalnessRoughnessAmbientOcclusionImage = new QImage(textureSize, textureSize, QImage::Format_ARGB32);
            textureMetalnessRoughnessAmbientOcclusionImage->fill(QColor(255, 255, 0));
            for (int row = 0; row < textureMetalnessRoughnessAmbientOcclusionImage->height(); ++row) {
                for (int col = 0; col < textureMetalnessRoughnessAmbientOcclusionImage->width(); ++col) {
                    QColor color(255, 255, 0);
                    if (nullptr != metalnessImage)
                        color.setBlue(qGray(metalnessImage->pixel(col, row)));
                    if (nullptr != roughnessImage)
                        color.setGreen(qGray(roughnessImage->pixel(col, row)));
                    if (nullptr != ambientOcclusionImage)
                        color.setRed(qGray(ambientOcclusionImage->pixel(col, row)));
                    textureMetalnessRoughnessAmbientOcclusionImage->setPixelColor(col, row, color);
                }
            }
        }
    }
    return textureMetalnessRoughnessAmbientOcclusionImage;
}

void UvMapGenerator::packUvs()
{
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

    m_mapPacker->addSeams(m_object->seamTriangleUvs);

    m_mapPacker->pack();
}

void UvMapGenerator::generateTextureColorImage()
{
    m_textureColorImage = std::make_unique<QImage>(UvMapGenerator::m_textureSize, UvMapGenerator::m_textureSize, QImage::Format_ARGB32);
    m_textureColorImage->fill(Qt::white);

    QPainter colorTexturePainter;
    colorTexturePainter.begin(m_textureColorImage.get());
    colorTexturePainter.setRenderHint(QPainter::Antialiasing);
    colorTexturePainter.setRenderHint(QPainter::HighQualityAntialiasing);
    colorTexturePainter.setPen(Qt::NoPen);

    for (const auto& layout : m_mapPacker->packedLayouts()) {
        const QImage* image = ImageForever::get(layout.id);
        if (nullptr == image) {
            dust3dDebug << "Find image failed:" << layout.id.toString();
            continue;
        }
        QPixmap brushPixmap;
        if (layout.flipped) {
            auto scaledImage = image->scaled(QSize(layout.height * UvMapGenerator::m_textureSize,
                layout.width * UvMapGenerator::m_textureSize));
            QPoint center = scaledImage.rect().center();
            QMatrix matrix;
            matrix.translate(center.x(), center.y());
            matrix.rotate(90);
            auto rotatedImage = scaledImage.transformed(matrix).mirrored(true, false);
            brushPixmap = QPixmap::fromImage(rotatedImage);
        } else {
            auto scaledImage = image->scaled(QSize(layout.width * UvMapGenerator::m_textureSize,
                layout.height * UvMapGenerator::m_textureSize));
            brushPixmap = QPixmap::fromImage(scaledImage);
        }
        colorTexturePainter.drawPixmap(layout.left * UvMapGenerator::m_textureSize,
            layout.top * UvMapGenerator::m_textureSize,
            brushPixmap);
    }

    colorTexturePainter.end();
}

void UvMapGenerator::generateUvCoords()
{
    std::map<std::array<dust3d::PositionKey, 3>, std::array<dust3d::Vector2, 3>> mergedUvs;
    for (const auto& layout : m_mapPacker->packedLayouts()) {
        for (const auto& it : layout.globalUv) {
            mergedUvs.insert({ it.first, it.second });
        }
    }
    std::vector<std::vector<dust3d::Vector2>> triangleUvs(m_object->triangles.size(), std::vector<dust3d::Vector2> { dust3d::Vector2(0.0, 0.0), dust3d::Vector2(0.0, 0.0), dust3d::Vector2(0.0, 0.0) });
    for (size_t n = 0; n < m_object->triangles.size(); ++n) {
        const auto& triangle = m_object->triangles[n];
        auto findUvs = mergedUvs.find({ dust3d::PositionKey(m_object->vertices[triangle[0]]),
            dust3d::PositionKey(m_object->vertices[triangle[1]]),
            dust3d::PositionKey(m_object->vertices[triangle[2]]) });
        if (findUvs == mergedUvs.end()) {
            continue;
        }
        triangleUvs[n][0] = findUvs->second[0];
        triangleUvs[n][1] = findUvs->second[1];
        triangleUvs[n][2] = findUvs->second[2];
    }
    m_object->setTriangleVertexUvs(triangleUvs);
}

void UvMapGenerator::generate()
{
    if (nullptr == m_object)
        return;

    if (nullptr == m_snapshot)
        return;

    packUvs();
    generateTextureColorImage();
    generateUvCoords();

    m_mesh = std::make_unique<ModelMesh>(*m_object);
    m_mesh->setTextureImage(new QImage(*m_textureColorImage));
}
