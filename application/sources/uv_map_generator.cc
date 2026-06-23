#include "uv_map_generator.h"
#include "image_forever.h"
#include <QPainter>
#include <QTransform>
#include <cmath>
#include <dust3d/base/part_target.h>
#include <dust3d/uv/uv_map_packer.h>
#include <map>
#include <queue>
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

        // 512×512 horizontal gradient to provide sufficient texture detail and prevent seam artifacts.
        // Larger resolution reduces filtering artifacts at seam boundaries.
        // The image is square so the chart packer has no incentive to flip it.
        const int kGradientSize = 512;
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
        seamPart.width = kGradientSize;
        seamPart.height = kGradientSize;

        // Inset UV coordinates from edges to prevent GPU filtering from sampling outside the gradient.
        // Using a small margin (0.5/kGradientSize ≈ 0.001) to keep UVs within safe bounds.
        const float kUvMargin = 0.5f / kGradientSize;
        const float kUvMin = kUvMargin;
        const float kUvMax = 1.0f - kUvMargin;
        const float kUvMid = (kUvMin + kUvMax) * 0.5f;

        // large side: triangle[0,1] are large-side vertices → u≈0
        //             triangle[2]   is the small-side vertex  → u≈1
        for (const auto& tri : seam.first) {
            seamPart.localUv[tri] = {
                dust3d::Vector2(kUvMin, kUvMin),
                dust3d::Vector2(kUvMin, kUvMax),
                dust3d::Vector2(kUvMax, kUvMid)
            };
        }
        // small side: triangle[0,1] are small-side vertices → u≈1
        //             triangle[2]   is the large-side vertex  → u≈0
        for (const auto& tri : seam.second) {
            seamPart.localUv[tri] = {
                dust3d::Vector2(kUvMax, kUvMin),
                dust3d::Vector2(kUvMax, kUvMax),
                dust3d::Vector2(kUvMin, kUvMid)
            };
        }

        m_mapPacker->addPart(seamPart);
    }

    // Lookup from quantized vertex position back to its 3D coordinate so we can
    // measure the surface area of image-less charts.  The keys in componentTriangleUvs
    // were built from these same vertex positions, so the quantization matches.
    std::map<dust3d::PositionKey, dust3d::Vector3> positionKeyToVertex;
    for (const auto& vertex : m_object->vertices)
        positionKeyToVertex.insert({ dust3d::PositionKey(vertex), vertex });
    auto sumTriangleArea = [&](const std::map<std::array<dust3d::PositionKey, 3>, std::array<dust3d::Vector2, 3>>& localUv) -> double {
        double total = 0.0;
        for (const auto& it : localUv) {
            auto findA = positionKeyToVertex.find(it.first[0]);
            auto findB = positionKeyToVertex.find(it.first[1]);
            auto findC = positionKeyToVertex.find(it.first[2]);
            if (findA == positionKeyToVertex.end() || findB == positionKeyToVertex.end() || findC == positionKeyToVertex.end())
                continue;
            total += dust3d::Vector3::area(findA->second, findB->second, findC->second);
        }
        return total;
    };
    auto componentColorImage = [&](const std::map<std::string, std::string>& component) -> const QImage* {
        const auto& colorImageIdIt = component.find("colorImageId");
        if (colorImageIdIt == component.end())
            return nullptr;
        return ImageForever::get(dust3d::Uuid(colorImageIdIt->second));
    };

    // A part with a texture image occupies a chart sized to the image resolution.  A
    // part without one used to fall back to a fixed 1x1 chart, which collapsed to a
    // near-invisible sliver of the atlas when packed alongside image-based charts.
    // Instead, give each image-less chart an area proportional to the 3D surface area
    // of its triangles, normalized so that all image-less charts together fill about
    // one texture's worth of texels.  This keeps texel density consistent and is
    // invariant to the model's absolute scale.
    double totalImagelessArea = 0.0;
    std::map<dust3d::Uuid, double> componentImagelessArea;
    for (const auto& componentTriangleUvIt : m_object->componentTriangleUvs) {
        auto componentIt = m_snapshot->components.find(componentTriangleUvIt.first.toString());
        if (componentIt == m_snapshot->components.end())
            continue;
        if (nullptr != componentColorImage(componentIt->second))
            continue;
        double area = sumTriangleArea(componentTriangleUvIt.second);
        componentImagelessArea[componentTriangleUvIt.first] = area;
        totalImagelessArea += area;
    }
    const double imagelessSizeScale = totalImagelessArea > 0.0
        ? (double)UvMapGenerator::m_textureSize / std::sqrt(totalImagelessArea)
        : 1.0;

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
        const QImage* image = componentColorImage(componentIt->second);
        if (nullptr != image) {
            const auto& colorImageIdIt = componentIt->second.find("colorImageId");
            imageId = dust3d::Uuid(colorImageIdIt->second);
            width = image->width();
            height = image->height();
        } else {
            // Image-less chart: size it by surface area so it keeps a fair share of the atlas.
            double area = componentImagelessArea[componentTriangleUvIt.first];
            double side = std::max(1.0, std::sqrt(area) * imagelessSizeScale);
            width = side;
            height = side;
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
    m_textureColorImage->fill(QColor(0, 255, 0, 0));

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
    const int bleedPixels = 32;

    for (const auto& layout : m_mapPacker->packedLayouts()) {
        int chartW = (int)(layout.width * UvMapGenerator::m_textureSize);
        int chartH = (int)(layout.height * UvMapGenerator::m_textureSize);
        QPixmap brushPixmap;
        if (layout.id.isNull()) {
            // Solid colour: fill the exact chart area plus bleed border
            brushPixmap = QPixmap(chartW + bleedPixels * 2, chartH + bleedPixels * 2);
            brushPixmap.fill(QColor(QString::fromStdString(layout.color.toString())));
        } else {
            const QImage* image = ImageForever::get(layout.id);
            if (nullptr == image) {
                dust3dDebug << "Find image failed:" << layout.id.toString();
                continue;
            }
            // Build the padded pixmap in two layers:
            //   Layer 1 (bleed)  – image stretched to the full padded size so the bleed
            //                      region is filled with approximate edge content instead
            //                      of the white atlas background, preventing seam artefacts.
            //   Layer 2 (chart)  – image scaled to exactly chartW×chartH and drawn at
            //                      (bleedPixels, bleedPixels), so the UV-mapped region
            //                      receives the correct, undistorted texture.
            if (layout.flipped) {
                auto scaledImage = image->scaled(QSize(chartH, chartW));
                QPoint center = scaledImage.rect().center();
                QTransform matrix;
                matrix.translate(center.x(), center.y());
                matrix.rotate(90);
                auto rotatedImage = scaledImage.transformed(matrix).mirrored(true, false);
                brushPixmap = QPixmap(chartW + bleedPixels * 2, chartH + bleedPixels * 2);
                // Layer 1: stretched bleed
                auto bleedImage = image->scaled(QSize(chartH + bleedPixels * 2, chartW + bleedPixels * 2));
                QPoint bleedCenter = bleedImage.rect().center();
                QTransform bleedMatrix;
                bleedMatrix.translate(bleedCenter.x(), bleedCenter.y());
                bleedMatrix.rotate(90);
                auto bleedRotated = bleedImage.transformed(bleedMatrix).mirrored(true, false);
                QPainter padPainter(&brushPixmap);
                padPainter.drawImage(0, 0, bleedRotated);
                // Layer 2: exact-scale chart content
                padPainter.drawImage(bleedPixels, bleedPixels, rotatedImage);
            } else {
                brushPixmap = QPixmap(chartW + bleedPixels * 2, chartH + bleedPixels * 2);
                // Layer 1: stretched bleed
                auto bleedImage = image->scaled(QSize(chartW + bleedPixels * 2, chartH + bleedPixels * 2));
                QPainter padPainter(&brushPixmap);
                padPainter.drawImage(0, 0, bleedImage);
                // Layer 2: exact-scale chart content
                auto scaledImage = image->scaled(QSize(chartW, chartH));
                padPainter.drawImage(bleedPixels, bleedPixels, scaledImage);
            }
        }
        colorTexturePainter.drawPixmap(layout.left * UvMapGenerator::m_textureSize - bleedPixels,
            layout.top * UvMapGenerator::m_textureSize - bleedPixels,
            brushPixmap);
    }

    colorTexturePainter.end();

    dilateTexture(m_textureColorImage.get());
}

void UvMapGenerator::dilateTexture(QImage* image)
{
    const int w = image->width();
    const int h = image->height();
    const QRgb emptyPixel = qRgba(0, 255, 0, 0);

    std::vector<bool> filled(w * h, false);
    std::queue<int> frontier;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (image->pixel(x, y) != emptyPixel) {
                filled[y * w + x] = true;
                bool onBorder = false;
                if (x > 0 && image->pixel(x - 1, y) == emptyPixel)
                    onBorder = true;
                else if (x < w - 1 && image->pixel(x + 1, y) == emptyPixel)
                    onBorder = true;
                else if (y > 0 && image->pixel(x, y - 1) == emptyPixel)
                    onBorder = true;
                else if (y < h - 1 && image->pixel(x, y + 1) == emptyPixel)
                    onBorder = true;
                if (onBorder)
                    frontier.push(y * w + x);
            }
        }
    }

    const int dx[] = { -1, 1, 0, 0 };
    const int dy[] = { 0, 0, -1, 1 };

    while (!frontier.empty()) {
        int idx = frontier.front();
        frontier.pop();
        int cx = idx % w;
        int cy = idx / w;
        QRgb color = image->pixel(cx, cy);

        for (int d = 0; d < 4; ++d) {
            int nx = cx + dx[d];
            int ny = cy + dy[d];
            if (nx < 0 || nx >= w || ny < 0 || ny >= h)
                continue;
            int nidx = ny * w + nx;
            if (!filled[nidx]) {
                filled[nidx] = true;
                image->setPixel(nx, ny, color);
                frontier.push(nidx);
            }
        }
    }
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
