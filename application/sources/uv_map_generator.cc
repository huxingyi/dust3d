#include "uv_map_generator.h"
#include "image_forever.h"
#include <QPainter>
#include <QTransform>
#include <algorithm>
#include <array>
#include <cmath>
#include <dust3d/base/part_target.h>
#include <dust3d/base/position_key.h>
#include <dust3d/base/string.h>
#include <dust3d/uv/cloth_folds.h>
#include <dust3d/uv/surface_pattern.h>
#include <dust3d/uv/uv_map_packer.h>
#include <functional>
#include <limits>
#include <map>
#include <queue>
#include <thread>
#include <unordered_set>

size_t UvMapGenerator::m_textureSize = 4096;

// A wrap chart painted texel by texel: its surface (welded by position, as the UV records are
// keyed), the tangent frame of every triangle, and the details to paint.
struct UvMapGenerator::SurfaceDetailChart {
    const dust3d::UvMapPacker::Layout* layout = nullptr;
    struct Triangle {
        std::array<size_t, 3> vertices;
        dust3d::Vector3 position[3];
        double u[3];
        double v[3];
        // unit direction of increasing u, and the side the bitangent (decreasing v: up in the
        // image, the OpenGL convention of glTF and FBX normal maps) is on: +1 where it is
        // normal x tangent, -1 where the chart is mirrored
        dust3d::Vector3 tangent;
        double handedness = 1.0;
        // world length of one unit of UV
        double worldPerUv = 0.0;
    };
    std::vector<dust3d::Vector3> vertices;
    std::vector<Triangle> triangles;
    dust3d::Color baseColor;
    std::unique_ptr<dust3d::SurfacePattern> pattern;
    std::unique_ptr<dust3d::ClothFolds> folds;
};

UvMapGenerator::UvMapGenerator(std::unique_ptr<dust3d::Object> object, std::unique_ptr<dust3d::Snapshot> snapshot)
    : m_object(std::move(object))
    , m_snapshot(std::move(snapshot))
{
}

UvMapGenerator::~UvMapGenerator() = default;

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

std::unique_ptr<QImage> UvMapGenerator::takeResultTextureEmissiveImage()
{
    return std::move(m_textureEmissiveImage);
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
        part.sourceId = componentTriangleUvIt.first;
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
        partIt.second.sourceId = partIt.first;
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
        if (hasSurfaceDetails(&layout)) {
            // painted texel by texel below; its gutter is then filled by the dilation
            continue;
        }
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

    bakeSurfaceDetailColors();

    dilateTexture(m_textureColorImage.get());
}

namespace {

// The direction of increasing u over a triangle, and whether its UVs are mirrored relative to
// its winding (the bitangent of decreasing v then lies on the other side of normal x tangent).
bool triangleTangentFrame(const dust3d::Vector3 position[3], const double u[3], const double v[3],
    dust3d::Vector3* tangent, double* handedness)
{
    dust3d::Vector3 edge1 = position[1] - position[0];
    dust3d::Vector3 edge2 = position[2] - position[0];
    double du1 = u[1] - u[0], dv1 = v[1] - v[0];
    double du2 = u[2] - u[0], dv2 = v[2] - v[0];
    double determinant = du1 * dv2 - du2 * dv1;
    dust3d::Vector3 faceNormal = dust3d::Vector3::crossProduct(edge1, edge2);
    if (std::abs(determinant) < 1e-18 || faceNormal.lengthSquared() < 1e-30)
        return false;
    dust3d::Vector3 alongU = (edge1 * dv2 - edge2 * dv1) / determinant;
    dust3d::Vector3 alongV = (edge2 * du1 - edge1 * du2) / determinant;
    if (alongU.lengthSquared() < 1e-30)
        return false;
    *tangent = alongU.normalized();
    dust3d::Vector3 up = -alongV;
    *handedness = dust3d::Vector3::dotProduct(dust3d::Vector3::crossProduct(faceNormal, *tangent), up) < 0.0 ? -1.0 : 1.0;
    return true;
}

// Paints the texels of a chart's triangles in an image of `width` x `height`, and those up to
// `outsidePixels` outside them (the details carried on, so the filtering at the rim samples
// them, not the gutter). A texel is painted from the triangle that covers it rather than a
// neighbour carried over it. Rows are split between threads: no texel is painted by two.
void rasterizeChart(const std::vector<UvMapGenerator::SurfaceDetailChart::Triangle>& triangles,
    int width, int height, double outsidePixels,
    const std::function<void(int x, int y, size_t triangle, const double weights[3])>& paint)
{
    if (triangles.empty())
        return;
    int top = height, bottom = 0, left = width, right = 0;
    for (const auto& triangle : triangles) {
        for (size_t i = 0; i < 3; ++i) {
            top = std::min(top, (int)std::floor(triangle.v[i] * height - outsidePixels));
            bottom = std::max(bottom, (int)std::ceil(triangle.v[i] * height + outsidePixels));
            left = std::min(left, (int)std::floor(triangle.u[i] * width - outsidePixels));
            right = std::max(right, (int)std::ceil(triangle.u[i] * width + outsidePixels));
        }
    }
    top = std::max(0, top);
    left = std::max(0, left);
    bottom = std::min(height - 1, bottom);
    right = std::min(width - 1, right);
    if (bottom < top || right < left)
        return;
    const int columns = right - left + 1;
    std::vector<float> distance((size_t)columns * (bottom - top + 1), std::numeric_limits<float>::max());

    auto paintRows = [&](int rowBegin, int rowEnd) {
        for (size_t t = 0; t < triangles.size(); ++t) {
            const auto& triangle = triangles[t];
            double x[3], y[3];
            for (size_t i = 0; i < 3; ++i) {
                x[i] = triangle.u[i] * width;
                y[i] = triangle.v[i] * height;
            }
            int y0 = std::max(rowBegin, (int)std::floor(std::min({ y[0], y[1], y[2] }) - outsidePixels));
            int y1 = std::min(rowEnd - 1, (int)std::ceil(std::max({ y[0], y[1], y[2] }) + outsidePixels));
            if (y1 < y0)
                continue;
            int x0 = std::max(left, (int)std::floor(std::min({ x[0], x[1], x[2] }) - outsidePixels));
            int x1 = std::min(right, (int)std::ceil(std::max({ x[0], x[1], x[2] }) + outsidePixels));
            double area = (x[1] - x[0]) * (y[2] - y[0]) - (x[2] - x[0]) * (y[1] - y[0]);
            if (std::abs(area) < 1e-12)
                continue;
            // the triangle's heights in pixels, to turn barycentric coordinates into distances
            double heights[3];
            for (size_t i = 0; i < 3; ++i) {
                size_t j = (i + 1) % 3, k = (i + 2) % 3;
                double edge = std::hypot(x[k] - x[j], y[k] - y[j]);
                heights[i] = edge > 1e-12 ? std::abs(area) / edge : 0.0;
            }
            for (int py = y0; py <= y1; ++py) {
                double cy = py + 0.5;
                for (int px = x0; px <= x1; ++px) {
                    double cx = px + 0.5;
                    double w[3];
                    w[0] = ((x[1] - cx) * (y[2] - cy) - (x[2] - cx) * (y[1] - cy)) / area;
                    w[1] = ((x[2] - cx) * (y[0] - cy) - (x[0] - cx) * (y[2] - cy)) / area;
                    w[2] = 1.0 - w[0] - w[1];
                    double outside = 0.0;
                    for (size_t i = 0; i < 3; ++i)
                        outside = std::max(outside, -w[i] * heights[i]);
                    if (outside > outsidePixels)
                        continue;
                    float& best = distance[(size_t)(py - top) * columns + (px - left)];
                    if ((float)outside >= best)
                        continue;
                    best = (float)outside;
                    paint(px, py, t, w);
                }
            }
        }
    };

    int rows = bottom - top + 1;
    int threadCount = (int)std::max(1u, std::min(16u, std::thread::hardware_concurrency()));
    threadCount = std::max(1, std::min(threadCount, rows / 16));
    std::vector<std::thread> workers;
    int band = (rows + threadCount - 1) / threadCount;
    for (int t = 0; t < threadCount; ++t) {
        int rowBegin = top + t * band;
        int rowEnd = std::min(bottom + 1, rowBegin + band);
        if (rowBegin >= rowEnd)
            break;
        workers.emplace_back(paintRows, rowBegin, rowEnd);
    }
    for (auto& worker : workers)
        worker.join();
}

// The surface point behind a texel: its position (carried on past the triangle's edges), its
// smooth normal and its tangent frame (tangent along increasing u, bitangent up the image).
struct TexelFrame {
    dust3d::Vector3 position;
    dust3d::Vector3 normal;
    dust3d::Vector3 tangent;
    dust3d::Vector3 bitangent;
};

TexelFrame texelFrame(const UvMapGenerator::SurfaceDetailChart::Triangle& triangle,
    const std::vector<dust3d::Vector3>& vertexNormals,
    const double w[3])
{
    TexelFrame frame;
    frame.position = triangle.position[0] * w[0] + triangle.position[1] * w[1] + triangle.position[2] * w[2];
    double c0 = std::max(0.0, w[0]), c1 = std::max(0.0, w[1]), c2 = std::max(0.0, w[2]);
    double sum = std::max(1e-12, c0 + c1 + c2);
    const dust3d::Vector3& n0 = vertexNormals[triangle.vertices[0]];
    const dust3d::Vector3& n1 = vertexNormals[triangle.vertices[1]];
    const dust3d::Vector3& n2 = vertexNormals[triangle.vertices[2]];
    frame.normal = (n0 * c0 + n1 * c1 + n2 * c2) / sum;
    if (frame.normal.lengthSquared() < 1e-24)
        frame.normal = dust3d::Vector3::normal(triangle.position[0], triangle.position[1], triangle.position[2]);
    frame.normal.normalize();
    frame.tangent = triangle.tangent - frame.normal * dust3d::Vector3::dotProduct(triangle.tangent, frame.normal);
    if (frame.tangent.lengthSquared() < 1e-24)
        frame.tangent = triangle.tangent;
    frame.tangent.normalize();
    frame.bitangent = dust3d::Vector3::crossProduct(frame.normal, frame.tangent) * triangle.handedness;
    return frame;
}

// Smooth normals of a chart's welded vertices.
std::vector<dust3d::Vector3> chartVertexNormals(const UvMapGenerator::SurfaceDetailChart& chart)
{
    std::vector<dust3d::Vector3> normals(chart.vertices.size());
    for (const auto& triangle : chart.triangles) {
        dust3d::Vector3 weighted = dust3d::Vector3::crossProduct(triangle.position[1] - triangle.position[0],
            triangle.position[2] - triangle.position[0]);
        for (size_t i = 0; i < 3; ++i)
            normals[triangle.vertices[i]] += weighted;
    }
    for (auto& normal : normals) {
        if (normal.lengthSquared() > 1e-30)
            normal.normalize();
    }
    return normals;
}

}

bool UvMapGenerator::hasSurfaceDetails(const dust3d::UvMapPacker::Layout* layout) const
{
    for (const auto& chart : m_surfaceDetailCharts) {
        if (chart->layout == layout)
            return true;
    }
    return false;
}

void UvMapGenerator::prepareSurfaceDetails()
{
    m_surfaceDetailCharts.clear();

    std::map<dust3d::PositionKey, dust3d::Vector3> positions;
    for (const auto& vertex : m_object->vertices)
        positions.insert({ dust3d::PositionKey(vertex), vertex });

    for (const auto& layout : m_mapPacker->packedLayouts()) {
        if (!layout.id.isNull() || layout.sourceId.isNull())
            continue;
        auto componentIt = m_snapshot->components.find(layout.sourceId.toString());
        if (componentIt == m_snapshot->components.end())
            continue;
        const auto& component = componentIt->second;
        std::string wrapMode = dust3d::String::valueOrEmpty(component, "wrap");
        if (wrapMode.empty())
            continue;
        auto readFloat = [&](const char* name, double defaultValue) {
            std::string value = dust3d::String::valueOrEmpty(component, name);
            return value.empty() ? defaultValue : (double)dust3d::String::toFloat(value);
        };
        auto patternType = dust3d::SurfacePattern::typeFromString(dust3d::String::valueOrEmpty(component, "wrapPattern"));
        double wrinkleStrength = std::max(0.0, std::min(1.0, readFloat("wrapWrinkles", 0.0)));
        if (dust3d::SurfacePattern::Type::None == patternType && wrinkleStrength <= 0.0)
            continue;

        auto chart = std::make_unique<SurfaceDetailChart>();
        chart->layout = &layout;
        chart->baseColor = layout.color;
        std::map<dust3d::PositionKey, size_t> vertexIndices;
        for (const auto& it : layout.globalUv) {
            SurfaceDetailChart::Triangle triangle;
            bool found = true;
            for (size_t i = 0; i < 3; ++i) {
                auto findPosition = positions.find(it.first[i]);
                if (findPosition == positions.end()) {
                    found = false;
                    break;
                }
                auto insertResult = vertexIndices.insert({ it.first[i], chart->vertices.size() });
                if (insertResult.second)
                    chart->vertices.push_back(findPosition->second);
                triangle.vertices[i] = insertResult.first->second;
                triangle.position[i] = findPosition->second;
                triangle.u[i] = it.second[i].x();
                triangle.v[i] = it.second[i].y();
            }
            if (!found || triangle.vertices[0] == triangle.vertices[1] || triangle.vertices[1] == triangle.vertices[2]
                || triangle.vertices[0] == triangle.vertices[2])
                continue;
            if (!triangleTangentFrame(triangle.position, triangle.u, triangle.v, &triangle.tangent, &triangle.handedness))
                continue;
            double worldLength = 0.0, uvLength = 0.0;
            for (size_t i = 0; i < 3; ++i) {
                size_t j = (i + 1) % 3;
                worldLength += (triangle.position[i] - triangle.position[j]).length();
                uvLength += std::hypot(triangle.u[i] - triangle.u[j], triangle.v[i] - triangle.v[j]);
            }
            if (uvLength <= 1e-12)
                continue;
            triangle.worldPerUv = worldLength / uvLength;
            chart->triangles.push_back(triangle);
        }
        if (chart->triangles.empty())
            continue;

        uint32_t seed = dust3d::SurfacePattern::seedFromString(componentIt->first);
        if (dust3d::SurfacePattern::Type::None != patternType) {
            dust3d::SurfacePattern::Settings settings;
            settings.type = patternType;
            settings.baseColor = layout.color;
            std::string patternColor = dust3d::String::valueOrEmpty(component, "wrapPatternColor");
            settings.patternColor = patternColor.empty() ? dust3d::SurfacePattern::defaultPatternColor(layout.color) : dust3d::Color(patternColor);
            settings.scale = std::max(0.005, readFloat("wrapPatternScale", 0.06));
            settings.belly = readFloat("wrapBelly", 0.0);
            settings.seed = seed;
            chart->pattern = std::make_unique<dust3d::SurfacePattern>(settings, chart->vertices);
        }
        if (wrinkleStrength > 0.0) {
            // the folds the mesh generator placed on this wrap (see ClothFolds::place)
            auto placed = dust3d::ClothFolds::deserialize(dust3d::String::valueOrEmpty(component, "__wrapFolds"));
            if (!placed.empty()) {
                auto folds = std::make_unique<dust3d::ClothFolds>(placed, wrinkleStrength);
                if (!folds->empty())
                    chart->folds = std::move(folds);
            }
        }
        if (nullptr == chart->pattern && nullptr == chart->folds)
            continue;
        m_surfaceDetailCharts.push_back(std::move(chart));
    }
}

void UvMapGenerator::bakeSurfaceDetailColors()
{
    if (m_surfaceDetailCharts.empty() || nullptr == m_textureColorImage)
        return;
    QImage& image = *m_textureColorImage;
    const int width = image.width();
    const int height = image.height();
    uchar* bits = image.bits();
    const qsizetype bytesPerLine = image.bytesPerLine();
    for (const auto& chart : m_surfaceDetailCharts) {
        std::vector<dust3d::Vector3> vertexNormals = chartVertexNormals(*chart);
        rasterizeChart(chart->triangles, width, height, 2.0, [&](int x, int y, size_t t, const double w[3]) {
            const auto& triangle = chart->triangles[t];
            TexelFrame frame = texelFrame(triangle, vertexNormals, w);
            double footprint = triangle.worldPerUv / width;
            dust3d::Color color = chart->pattern ? chart->pattern->colorAt(frame.position, frame.normal, footprint) : chart->baseColor;
            if (chart->folds) {
                // the creases a little darker (they catch less light in any engine)
                double cavity = 0.0;
                chart->folds->evaluate(frame.position, frame.normal, frame.tangent, frame.bitangent, footprint, nullptr, &cavity);
                double shade = 1.0 - 0.3 * std::min(1.0, cavity);
                color = dust3d::Color(color.r() * shade, color.g() * shade, color.b() * shade, color.alpha());
            }
            QRgb* line = (QRgb*)(bits + (qsizetype)y * bytesPerLine);
            line[x] = qRgba((int)std::lround(std::max(0.0, std::min(1.0, color.r())) * 255.0),
                (int)std::lround(std::max(0.0, std::min(1.0, color.g())) * 255.0),
                (int)std::lround(std::max(0.0, std::min(1.0, color.b())) * 255.0),
                (int)std::lround(std::max(0.0, std::min(1.0, color.alpha())) * 255.0));
        });
    }
}

void UvMapGenerator::generateTextureNormalImage()
{
    bool anyWrinkles = false;
    for (const auto& chart : m_surfaceDetailCharts)
        anyWrinkles = anyWrinkles || nullptr != chart->folds;
    if (!anyWrinkles)
        return;

    // half the colour atlas: folds are wider than the finest colour detail
    const int size = (int)std::max((size_t)256, UvMapGenerator::m_textureSize / 2);
    const int bleedPixels = 8;
    m_textureNormalImage = std::make_unique<QImage>(size, size, QImage::Format_ARGB32);
    m_textureNormalImage->fill(QColor(0, 255, 0, 0));
    {
        // every other chart is flat
        QPainter painter(m_textureNormalImage.get());
        painter.setPen(Qt::NoPen);
        for (const auto& layout : m_mapPacker->packedLayouts()) {
            bool wrinkled = false;
            for (const auto& chart : m_surfaceDetailCharts)
                wrinkled = wrinkled || (chart->layout == &layout && nullptr != chart->folds);
            if (wrinkled)
                continue;
            painter.fillRect(QRect((int)(layout.left * size) - bleedPixels, (int)(layout.top * size) - bleedPixels,
                                 (int)(layout.width * size) + bleedPixels * 2, (int)(layout.height * size) + bleedPixels * 2),
                QColor(128, 128, 255));
        }
    }
    QImage& image = *m_textureNormalImage;
    uchar* bits = image.bits();
    const qsizetype bytesPerLine = image.bytesPerLine();
    for (const auto& chart : m_surfaceDetailCharts) {
        if (nullptr == chart->folds)
            continue;
        std::vector<dust3d::Vector3> vertexNormals = chartVertexNormals(*chart);
        rasterizeChart(chart->triangles, size, size, 2.0, [&](int x, int y, size_t t, const double w[3]) {
            const auto& triangle = chart->triangles[t];
            TexelFrame frame = texelFrame(triangle, vertexNormals, w);
            double footprint = triangle.worldPerUv / size;
            dust3d::Vector3 normal(0.0, 0.0, 1.0);
            chart->folds->evaluate(frame.position, frame.normal, frame.tangent, frame.bitangent, footprint, &normal, nullptr);
            QRgb* line = (QRgb*)(bits + (qsizetype)y * bytesPerLine);
            line[x] = qRgba((int)std::lround((normal.x() * 0.5 + 0.5) * 255.0),
                (int)std::lround((normal.y() * 0.5 + 0.5) * 255.0),
                (int)std::lround((normal.z() * 0.5 + 0.5) * 255.0), 255);
        });
    }
    dilateTexture(m_textureNormalImage.get());
}

void UvMapGenerator::resolveTriangleTangents()
{
    const auto* triangleVertexUvs = m_object->triangleVertexUvs();
    if (nullptr == triangleVertexUvs)
        return;
    std::vector<dust3d::Vector3> tangents(m_object->triangles.size());
    for (size_t t = 0; t < m_object->triangles.size(); ++t) {
        const auto& triangle = m_object->triangles[t];
        if (triangle.size() < 3 || t >= triangleVertexUvs->size() || (*triangleVertexUvs)[t].size() < 3)
            continue;
        dust3d::Vector3 position[3];
        double u[3], v[3];
        for (size_t i = 0; i < 3; ++i) {
            position[i] = m_object->vertices[triangle[i]];
            u[i] = (*triangleVertexUvs)[t][i].x();
            v[i] = (*triangleVertexUvs)[t][i].y();
        }
        dust3d::Vector3 tangent;
        double handedness = 1.0;
        if (!triangleTangentFrame(position, u, v, &tangent, &handedness)) {
            // no usable UVs (a flat chart): any direction in the triangle's plane
            tangent = (position[1] - position[0]).normalized();
            if (tangent.lengthSquared() < 1e-24)
                tangent = dust3d::Vector3(1.0, 0.0, 0.0);
        }
        // the viewport takes the bitangent as normal x tangent; a tangent twice as long marks
        // a mirrored chart, where it is the other way round (see model.vert)
        tangents[t] = tangent * (handedness < 0.0 ? 2.0 : 1.0);
    }
    m_object->setTriangleTangents(tangents);
}

void UvMapGenerator::generateTextureMaterialImages()
{
    // Per-part metallic, roughness and emissive maps, painted chart by chart like the colour
    // atlas, so an exported model keeps metal parts metallic and glowing parts glowing. Only
    // generated when some part sets one of them.
    struct Material {
        float metallic = 0.0f;
        float roughness = 1.0f;
        float emissive = 0.0f;
        QColor color = QColor(255, 255, 255);
    };
    std::map<dust3d::Uuid, Material> materials;
    bool anyMetallic = false, anyRoughness = false, anyEmissive = false;
    for (const auto& componentIt : m_snapshot->components) {
        if (dust3d::String::valueOrEmpty(componentIt.second, "linkDataType") != "partId")
            continue;
        auto partIt = m_snapshot->parts.find(dust3d::String::valueOrEmpty(componentIt.second, "linkData"));
        if (partIt == m_snapshot->parts.end())
            continue;
        Material material;
        std::string value = dust3d::String::valueOrEmpty(partIt->second, "metallic");
        if (!value.empty())
            material.metallic = dust3d::String::toFloat(value);
        value = dust3d::String::valueOrEmpty(partIt->second, "roughness");
        if (!value.empty())
            material.roughness = dust3d::String::toFloat(value);
        value = dust3d::String::valueOrEmpty(partIt->second, "emissive");
        if (!value.empty())
            material.emissive = dust3d::String::toFloat(value);
        std::string colorString = dust3d::String::valueOrEmpty(componentIt.second, "color");
        if (!colorString.empty())
            material.color = QColor(QString::fromStdString(colorString));
        anyMetallic = anyMetallic || material.metallic > 0.0f;
        anyRoughness = anyRoughness || material.roughness < 1.0f;
        anyEmissive = anyEmissive || material.emissive > 0.0f;
        materials[dust3d::Uuid(componentIt.first)] = material;
    }
    if (!anyMetallic && !anyRoughness && !anyEmissive)
        return;

    const int size = 1024;
    const int bleedPixels = 8;
    auto paint = [&](std::function<QColor(const Material&)> valueOf, QColor background) {
        auto image = std::make_unique<QImage>(size, size, QImage::Format_ARGB32);
        image->fill(QColor(0, 255, 0, 0));
        QPainter painter(image.get());
        painter.setPen(Qt::NoPen);
        for (const auto& layout : m_mapPacker->packedLayouts()) {
            auto findMaterial = materials.find(layout.sourceId);
            QColor color = findMaterial == materials.end() ? background : valueOf(findMaterial->second);
            painter.fillRect(QRect((int)(layout.left * size) - bleedPixels, (int)(layout.top * size) - bleedPixels,
                                 (int)(layout.width * size) + bleedPixels * 2, (int)(layout.height * size) + bleedPixels * 2),
                color);
        }
        painter.end();
        dilateTexture(image.get());
        return image;
    };
    auto gray = [](float v) {
        int g = std::max(0, std::min(255, (int)std::lround(v * 255.0f)));
        return QColor(g, g, g);
    };
    if (anyMetallic)
        m_textureMetalnessImage = paint([&](const Material& m) { return gray(m.metallic); }, gray(0.0f));
    if (anyMetallic || anyRoughness)
        m_textureRoughnessImage = paint([&](const Material& m) { return gray(m.roughness); }, gray(1.0f));
    if (anyEmissive) {
        m_textureEmissiveImage = paint([&](const Material& m) {
            float e = std::max(0.0f, std::min(1.0f, m.emissive));
            return QColor((int)(m.color.red() * e), (int)(m.color.green() * e), (int)(m.color.blue() * e));
        },
            QColor(0, 0, 0));
    }
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

// The component each triangle came from, kept on the object (the rig generator and the
// exporters work on copies of it, some with moved vertices, so it is worked out once here
// while the triangles still match the UV records by position).
void UvMapGenerator::generateTriangleComponentIds()
{
    std::map<std::array<dust3d::PositionKey, 3>, dust3d::Uuid> triangleToComponent;
    for (const auto& componentIt : m_object->componentTriangleUvs) {
        for (const auto& triangleIt : componentIt.second)
            triangleToComponent.insert({ triangleIt.first, componentIt.first });
    }
    for (const auto& brokenIt : m_object->brokenTrianglesToComponentIdMap)
        triangleToComponent.insert({ brokenIt.first, brokenIt.second });
    if (triangleToComponent.empty())
        return;
    {
        // the mesh generator already knows the uncombined parts' triangles
        std::vector<dust3d::Uuid> triangleComponents = m_object->triangleComponentIds;
        triangleComponents.resize(m_object->triangles.size());
        for (size_t t = 0; t < m_object->triangles.size(); ++t) {
            const auto& triangle = m_object->triangles[t];
            if (triangle.size() < 3 || !triangleComponents[t].isNull())
                continue;
            for (size_t rotation = 0; rotation < 3 && triangleComponents[t].isNull(); ++rotation) {
                auto found = triangleToComponent.find({ dust3d::PositionKey(m_object->vertices[triangle[rotation % 3]]),
                    dust3d::PositionKey(m_object->vertices[triangle[(rotation + 1) % 3]]),
                    dust3d::PositionKey(m_object->vertices[triangle[(rotation + 2) % 3]]) });
                if (found != triangleToComponent.end())
                    triangleComponents[t] = found->second;
            }
        }
        // Triangles the booleans made along a join (no UV record) take the part of a neighbour.
        // (by position: vertices may be split at UV seams and hard edges)
        auto edgeKey = [&](size_t a, size_t b) {
            dust3d::PositionKey ka(m_object->vertices[a]), kb(m_object->vertices[b]);
            return kb < ka ? std::make_pair(kb, ka) : std::make_pair(ka, kb);
        };
        std::map<std::pair<dust3d::PositionKey, dust3d::PositionKey>, std::vector<size_t>> edgeTriangles;
        for (size_t t = 0; t < m_object->triangles.size(); ++t) {
            const auto& triangle = m_object->triangles[t];
            for (size_t i = 0; i < triangle.size(); ++i)
                edgeTriangles[edgeKey(triangle[i], triangle[(i + 1) % triangle.size()])].push_back(t);
        }
        for (bool changed = true; changed;) {
            changed = false;
            for (size_t t = 0; t < m_object->triangles.size(); ++t) {
                if (!triangleComponents[t].isNull())
                    continue;
                const auto& triangle = m_object->triangles[t];
                for (size_t i = 0; i < triangle.size() && triangleComponents[t].isNull(); ++i) {
                    for (size_t other : edgeTriangles[edgeKey(triangle[i], triangle[(i + 1) % triangle.size()])]) {
                        if (!triangleComponents[other].isNull()) {
                            triangleComponents[t] = triangleComponents[other];
                            changed = true;
                            break;
                        }
                    }
                }
            }
        }
        m_object->triangleComponentIds = triangleComponents;
    }
}

void UvMapGenerator::generate()
{
    if (nullptr == m_object)
        return;

    if (nullptr == m_snapshot)
        return;

    generateTriangleComponentIds();
    packUvs();
    prepareSurfaceDetails();
    generateTextureColorImage();
    generateTextureNormalImage();
    generateTextureMaterialImages();
    generateUvCoords();
    if (m_textureNormalImage)
        resolveTriangleTangents();

    m_mesh = std::make_unique<ModelMesh>(*m_object);
    m_mesh->setTextureImage(new QImage(*m_textureColorImage));
    if (m_textureNormalImage)
        m_mesh->setNormalMapImage(new QImage(*m_textureNormalImage));

    // Show part materials in the editor: the preview's material map carries metalness (B),
    // roughness (G) and, in alpha, the inverse of the glow strength.
    if (m_textureMetalnessImage || m_textureRoughnessImage || m_textureEmissiveImage) {
        QImage* preview = combineMetalnessRoughnessAmbientOcclusionImages(m_textureMetalnessImage.get(),
            m_textureRoughnessImage.get(), nullptr);
        if (nullptr == preview && m_textureEmissiveImage) {
            preview = new QImage(m_textureEmissiveImage->size(), QImage::Format_ARGB32);
            preview->fill(QColor(255, 255, 0));
        }
        if (nullptr != preview && m_textureEmissiveImage && m_textureEmissiveImage->size() == preview->size()) {
            for (int row = 0; row < preview->height(); ++row) {
                for (int col = 0; col < preview->width(); ++col) {
                    QColor glow = m_textureEmissiveImage->pixelColor(col, row);
                    int strength = std::max(glow.red(), std::max(glow.green(), glow.blue()));
                    if (strength <= 0)
                        continue;
                    QColor color = preview->pixelColor(col, row);
                    color.setAlpha(255 - strength);
                    preview->setPixelColor(col, row, color);
                }
            }
        }
        if (nullptr != preview) {
            m_mesh->setMetalnessRoughnessAmbientOcclusionMapImage(preview);
            m_mesh->setHasMetalnessInImage(nullptr != m_textureMetalnessImage);
            m_mesh->setHasRoughnessInImage(true);
        }
    }
}
