#include "uv_map_generator.h"
#include "image_forever.h"
#include <QPainter>
#include <QTransform>
#include <dust3d/base/part_target.h>
#include <dust3d/uv/uv_map_packer.h>
#include <map>
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

    // Build vertex-position-key → component base color lookup so we can identify
    // the colors on each side of a seam boundary.
    std::map<dust3d::PositionKey, QColor> vertexToComponentColor;
    for (const auto& compIt : m_object->componentTriangleUvs) {
        QColor color(255, 255, 255);
        auto snapshotCompIt = m_snapshot->components.find(compIt.first.toString());
        if (snapshotCompIt != m_snapshot->components.end()) {
            const auto& colorIt = snapshotCompIt->second.find("color");
            if (colorIt != snapshotCompIt->second.end())
                color = QColor(QString::fromStdString(colorIt->second));
        }
        for (const auto& triIt : compIt.second) {
            for (size_t i = 0; i < 3; ++i)
                vertexToComponentColor.insert({ triIt.first[i], color });
        }
    }

    // For each seam create a dedicated gradient chart.  Large-side vertices are
    // mapped to u=0 and small-side vertices to u=1, so the renderer interpolates
    // smoothly from the large-side component color to the small-side component
    // color across every bridging triangle.  Seam parts are added to the packer
    // before component parts so their globalUv entries win in generateUvCoords.
    //
    // seam.first  triangles have layout [large₀, large₁, small].
    // seam.second triangles have layout [small₀, small₁, large].
    for (const auto& seam : m_object->seamTriangleUvs) {
        if (seam.first.empty() && seam.second.empty())
            continue;

        QColor colorLarge(200, 200, 200), colorSmall(200, 200, 200);
        if (!seam.first.empty()) {
            const auto& tri = *seam.first.begin();
            auto it = vertexToComponentColor.find(tri[0]);
            if (it != vertexToComponentColor.end())
                colorLarge = it->second;
            it = vertexToComponentColor.find(tri[2]);
            if (it != vertexToComponentColor.end())
                colorSmall = it->second;
        } else {
            const auto& tri = *seam.second.begin();
            auto it = vertexToComponentColor.find(tri[2]);
            if (it != vertexToComponentColor.end())
                colorLarge = it->second;
            it = vertexToComponentColor.find(tri[0]);
            if (it != vertexToComponentColor.end())
                colorSmall = it->second;
        }

        // 64×64 horizontal gradient: column 0 = colorLarge, column 63 = colorSmall.
        // The image is square so the chart packer has no incentive to flip it.
        const int kGradientSize = 64;
        QImage gradientImage(kGradientSize, kGradientSize, QImage::Format_ARGB32);
        for (int y = 0; y < kGradientSize; ++y) {
            for (int x = 0; x < kGradientSize; ++x) {
                double t = (double)x / (kGradientSize - 1);
                int r = (int)(colorLarge.red() * (1.0 - t) + colorSmall.red() * t);
                int g = (int)(colorLarge.green() * (1.0 - t) + colorSmall.green() * t);
                int b = (int)(colorLarge.blue() * (1.0 - t) + colorSmall.blue() * t);
                int a = (int)(colorLarge.alpha() * (1.0 - t) + colorSmall.alpha() * t);
                gradientImage.setPixelColor(x, y, QColor(r, g, b, a));
            }
        }
        dust3d::Uuid gradientId = ImageForever::add(&gradientImage);

        dust3d::UvMapPacker::Part seamPart;
        seamPart.id = gradientId;
        seamPart.color = dust3d::Color(1.0, 1.0, 1.0);
        seamPart.width = 1.0;
        seamPart.height = 1.0;

        // large side: triangle[0,1] are large-side vertices → u=0
        //             triangle[2]   is the small-side vertex  → u=1
        for (const auto& tri : seam.first) {
            seamPart.localUv[tri] = {
                dust3d::Vector2(0.0, 0.0),
                dust3d::Vector2(0.0, 1.0),
                dust3d::Vector2(1.0, 0.5)
            };
        }
        // small side: triangle[0,1] are small-side vertices → u=1
        //             triangle[2]   is the large-side vertex  → u=0
        for (const auto& tri : seam.second) {
            seamPart.localUv[tri] = {
                dust3d::Vector2(1.0, 0.0),
                dust3d::Vector2(1.0, 1.0),
                dust3d::Vector2(0.0, 0.5)
            };
        }

        m_mapPacker->addPart(seamPart);
    }

    for (const auto& componentTriangleUvIt : m_object->componentTriangleUvs) {
        auto componentIt = m_snapshot->components.find(componentTriangleUvIt.first.toString());
        if (componentIt == m_snapshot->components.end())
            continue;
        dust3d::Uuid imageId;
        dust3d::Color color(1.0, 1.0, 1.0);
        double width = 1.0;
        double height = 1.0;
        const auto& colorIt = componentIt->second.find("color");
        if (colorIt != componentIt->second.end()) {
            color = dust3d::Color(colorIt->second);
        }
        const auto& colorImageIdIt = componentIt->second.find("colorImageId");
        if (colorImageIdIt != componentIt->second.end()) {
            imageId = dust3d::Uuid(colorImageIdIt->second);
            const QImage* image = ImageForever::get(imageId);
            if (nullptr != image) {
                width = image->width();
                height = image->height();
            }
        }
        dust3d::UvMapPacker::Part part;
        part.id = imageId;
        part.color = color;
        part.width = width;
        part.height = height;
        part.localUv = componentTriangleUvIt.second;
        m_mapPacker->addPart(part);
    }

    // The following is to make a component colored UV for those broken triangles which generated by mesh boolean algorithm
    std::map<dust3d::Uuid, dust3d::UvMapPacker::Part> partWithBrokenTriangles;
    const auto zeroUv = std::array<dust3d::Vector2, 3> {
        dust3d::Vector2(0.0, 0.0), dust3d::Vector2(0.0, 0.0), dust3d::Vector2(0.0, 0.0)
    };
    for (const auto& brokenTrianglesToComponentIdIt : m_object->brokenTrianglesToComponentIdMap) {
        partWithBrokenTriangles[brokenTrianglesToComponentIdIt.second].localUv.insert({ brokenTrianglesToComponentIdIt.first, zeroUv });
    }
    for (auto& partIt : partWithBrokenTriangles) {
        auto componentIt = m_snapshot->components.find(partIt.first.toString());
        if (componentIt == m_snapshot->components.end())
            continue;
        dust3d::Color color(1.0, 1.0, 1.0);
        double width = 1.0;
        double height = 1.0;
        const auto& colorIt = componentIt->second.find("color");
        if (colorIt != componentIt->second.end()) {
            color = dust3d::Color(colorIt->second);
        }
        partIt.second.color = color;
        partIt.second.width = width;
        partIt.second.height = height;
        m_mapPacker->addPart(partIt.second);
    }

    m_mapPacker->pack();
}

void UvMapGenerator::generateTextureColorImage()
{
    m_textureColorImage = std::make_unique<QImage>(UvMapGenerator::m_textureSize, UvMapGenerator::m_textureSize, QImage::Format_ARGB32);
    m_textureColorImage->fill(Qt::white);

    QPainter colorTexturePainter;
    colorTexturePainter.begin(m_textureColorImage.get());
    colorTexturePainter.setRenderHint(QPainter::Antialiasing);
#if QT_VERSION < 0x060000
    colorTexturePainter.setRenderHint(QPainter::HighQualityAntialiasing);
#endif
    colorTexturePainter.setPen(Qt::NoPen);

    // Extend each chart's painted region by bleedPixels on every side to prevent
    // UV seam white lines caused by GPU bilinear filtering sampling white background
    // pixels just outside the chart boundary. The chart padding (~20px) comfortably
    // accommodates this bleed without overlapping adjacent charts.
    const int bleedPixels = 5;

    for (const auto& layout : m_mapPacker->packedLayouts()) {
        QPixmap brushPixmap;
        if (layout.id.isNull()) {
            brushPixmap = QPixmap(layout.width * UvMapGenerator::m_textureSize + bleedPixels * 2,
                layout.height * UvMapGenerator::m_textureSize + bleedPixels * 2);
            brushPixmap.fill(QColor(QString::fromStdString(layout.color.toString())));
        } else {
            const QImage* image = ImageForever::get(layout.id);
            if (nullptr == image) {
                dust3dDebug << "Find image failed:" << layout.id.toString();
                continue;
            }
            if (layout.flipped) {
                auto scaledImage = image->scaled(QSize(layout.height * UvMapGenerator::m_textureSize + bleedPixels * 2,
                    layout.width * UvMapGenerator::m_textureSize + bleedPixels * 2));
                QPoint center = scaledImage.rect().center();
                QTransform matrix;
                matrix.translate(center.x(), center.y());
                matrix.rotate(90);
                auto rotatedImage = scaledImage.transformed(matrix).mirrored(true, false);
                brushPixmap = QPixmap::fromImage(rotatedImage);
            } else {
                auto scaledImage = image->scaled(QSize(layout.width * UvMapGenerator::m_textureSize + bleedPixels * 2,
                    layout.height * UvMapGenerator::m_textureSize + bleedPixels * 2));
                brushPixmap = QPixmap::fromImage(scaledImage);
            }
        }
        colorTexturePainter.drawPixmap(layout.left * UvMapGenerator::m_textureSize - bleedPixels,
            layout.top * UvMapGenerator::m_textureSize - bleedPixels,
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
