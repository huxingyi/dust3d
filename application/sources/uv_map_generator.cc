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
#include <dust3d/uv/surface_ornament.h>
#include <dust3d/uv/surface_paint.h>
#include <dust3d/uv/surface_pattern.h>
#include <dust3d/uv/uv_map_packer.h>
#include <functional>
#include <limits>
#include <map>
#include <queue>
#include <thread>
#include <unordered_set>

size_t UvMapGenerator::m_textureSize = 4096;

// The strip of triangles that joins two parts, with a place of its own in the atlas for each
// of them. Its colour runs from one part's to the other's.
struct UvMapGenerator::SeamChart {
    dust3d::Color colorLarge;
    dust3d::Color colorSmall;
    dust3d::Uuid componentLarge;
    dust3d::Uuid componentSmall;
    // per triangle: how much of the small side's colour each corner has
    std::map<std::array<dust3d::PositionKey, 3>, std::array<double, 3>> cornerSmall;
};

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
        // the model's own normals at the corners (what it is lit with: the normal map is
        // told from these), where it has them
        bool hasCornerNormals = false;
        dust3d::Vector3 cornerNormal[3];
        // on a seam: how much of the small side's colour each corner has
        double cornerSmall[3] = { 0.0, 0.0, 0.0 };
    };
    // a seam between two parts (the colour runs from baseColor to seamColor), or null
    const SeamChart* seam = nullptr;
    // the paint parts that reach this chart, in painting order
    std::vector<const dust3d::SurfacePaint*> paints;
    // how open each vertex is, for the shading painted into the colour (empty: none)
    std::vector<double> vertexOcclusion;
    std::vector<dust3d::Vector3> vertices;
    std::vector<Triangle> triangles;
    dust3d::Color baseColor;
    std::unique_ptr<dust3d::SurfacePattern> pattern;
    std::unique_ptr<dust3d::ClothFolds> folds;
    // An ornament is laid out across the chart's own UV rectangle
    std::unique_ptr<dust3d::SurfaceOrnament> ornament;
    // per triangle: an end cap of a tube, left plain
    std::vector<bool> ornamentSkipped;
    double left = 0.0;
    double top = 0.0;
    double width = 1.0;
    double height = 1.0;
    double ornamentU(double u) const { return (u - left) / width; }
    double ornamentV(double v) const { return (v - top) / height; }
};

UvMapGenerator::UvMapGenerator(std::unique_ptr<dust3d::Object> object, std::unique_ptr<dust3d::Snapshot> snapshot)
    : m_object(std::move(object))
    , m_snapshot(std::move(snapshot))
{
    m_atlasSize = m_textureSize;
    if (nullptr != m_snapshot) {
        std::string size = dust3d::String::valueOrEmpty(m_snapshot->canvas, "textureSize");
        if (!size.empty())
            m_atlasSize = (size_t)std::max(64, std::min(8192, dust3d::String::toInt(size)));
        std::string shading = dust3d::String::valueOrEmpty(m_snapshot->canvas, "textureShading");
        if (!shading.empty())
            m_shading = std::max(0.0, std::min(1.0, (double)dust3d::String::toFloat(shading)));
    }
    m_painted = m_shading > 0.0 || (nullptr != m_object && !m_object->paintStrokes.empty());
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
    std::map<dust3d::PositionKey, dust3d::Uuid> vertexToComponentId;
    for (const auto& compIt : m_object->componentTriangleUvs) {
        QColor color(255, 255, 255);
        auto snapshotCompIt = m_snapshot->components.find(compIt.first.toString());
        if (snapshotCompIt != m_snapshot->components.end()) {
            const auto& colorIt = snapshotCompIt->second.find("color");
            if (colorIt != snapshotCompIt->second.end())
                color = QColor(QString::fromStdString(colorIt->second));
        }
        for (const auto& triIt : compIt.second) {
            for (size_t i = 0; i < 3; ++i) {
                vertexToComponentColor.insert({ triIt.first[i], color });
                vertexToComponentId.insert({ triIt.first[i], compIt.first });
            }
        }
    }
    m_seamCharts.clear();

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

        // The colours of the two parts the seam joins, from whichever of its vertices are
        // still vertices of a part. A seam onto an earlier seam (an ear on a head that is
        // itself two parts joined) may have none on one side: it then takes the other side's,
        // or failing that the colour of the nearest part.
        QColor colorLarge, colorSmall;
        auto lookUp = [&](const dust3d::PositionKey& key, QColor* color) {
            if (color->isValid())
                return;
            auto it = vertexToComponentColor.find(key);
            if (it != vertexToComponentColor.end())
                *color = it->second;
        };
        for (const auto& tri : seam.first) {
            lookUp(tri[0], &colorLarge);
            lookUp(tri[1], &colorLarge);
            lookUp(tri[2], &colorSmall);
        }
        for (const auto& tri : seam.second) {
            lookUp(tri[2], &colorLarge);
            lookUp(tri[0], &colorSmall);
            lookUp(tri[1], &colorSmall);
        }
        if (!colorLarge.isValid() && !colorSmall.isValid()) {
            const auto& first = seam.first.empty() ? *seam.second.begin() : *seam.first.begin();
            dust3d::Vector3 at;
            for (const auto& vertex : m_object->vertices) {
                if (dust3d::PositionKey(vertex) == first[0]) {
                    at = vertex;
                    break;
                }
            }
            double nearest = std::numeric_limits<double>::max();
            for (const auto& vertex : m_object->vertices) {
                auto it = vertexToComponentColor.find(dust3d::PositionKey(vertex));
                if (it == vertexToComponentColor.end())
                    continue;
                double distance = (vertex - at).lengthSquared();
                if (distance < nearest) {
                    nearest = distance;
                    colorLarge = it->second;
                }
            }
        }
        if (!colorLarge.isValid())
            colorLarge = colorSmall.isValid() ? colorSmall : QColor(200, 200, 200);
        if (!colorSmall.isValid())
            colorSmall = colorLarge;

        if (m_painted) {
            // painted texel by texel like the parts (see prepareSurfaceDetails): one cell of a
            // grid for each triangle, sized below with the charts of the parts
            auto seamChart = std::make_unique<SeamChart>();
            auto toColor = [](const QColor& c) { return dust3d::Color(c.redF(), c.greenF(), c.blueF(), c.alphaF()); };
            seamChart->colorLarge = toColor(colorLarge);
            seamChart->colorSmall = toColor(colorSmall);
            for (const auto& tri : seam.first) {
                seamChart->cornerSmall[tri] = { 0.0, 0.0, 1.0 };
                auto it = vertexToComponentId.find(tri[0]);
                if (it != vertexToComponentId.end())
                    seamChart->componentLarge = it->second;
                it = vertexToComponentId.find(tri[2]);
                if (it != vertexToComponentId.end())
                    seamChart->componentSmall = it->second;
            }
            for (const auto& tri : seam.second) {
                seamChart->cornerSmall[tri] = { 1.0, 1.0, 0.0 };
                auto it = vertexToComponentId.find(tri[2]);
                if (it != vertexToComponentId.end())
                    seamChart->componentLarge = it->second;
                it = vertexToComponentId.find(tri[0]);
                if (it != vertexToComponentId.end())
                    seamChart->componentSmall = it->second;
            }
            m_seamCharts[dust3d::Uuid::createUuid()] = std::move(seamChart);
            continue;
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
    std::map<dust3d::Uuid, double> componentDetail;
    for (const auto& componentTriangleUvIt : m_object->componentTriangleUvs) {
        auto componentIt = m_snapshot->components.find(componentTriangleUvIt.first.toString());
        if (componentIt == m_snapshot->components.end())
            continue;
        if (nullptr != componentColorImage(componentIt->second))
            continue;
        // (a part can ask for more of the atlas than its size earns it: a head with a face on it)
        double detail = 1.0;
        if ("partId" == dust3d::String::valueOrEmpty(componentIt->second, "linkDataType")) {
            auto partIt = m_snapshot->parts.find(dust3d::String::valueOrEmpty(componentIt->second, "linkData"));
            if (partIt != m_snapshot->parts.end()) {
                std::string textureDetail = dust3d::String::valueOrEmpty(partIt->second, "textureDetail");
                if (!textureDetail.empty())
                    detail = std::max(0.25, std::min(4.0, (double)dust3d::String::toFloat(textureDetail)));
            }
        }
        componentDetail[componentTriangleUvIt.first] = detail;
        double area = sumTriangleArea(componentTriangleUvIt.second) * detail * detail;
        componentImagelessArea[componentTriangleUvIt.first] = area;
        totalImagelessArea += area;
    }
    // The triangles that join two parts are laid out the way a painter unwraps a low poly
    // model: the ones facing the same way and touching each other are one island, seen flat
    // from that side (the front of a head around its nose, the top of a shoulder). A face
    // painted across such a join is then in one piece in the texture, at the density of the
    // parts it joins.
    struct SeamIsland {
        std::unique_ptr<SeamChart> chart;
        std::map<std::array<dust3d::PositionKey, 3>, std::array<dust3d::Vector2, 3>> flat;
        double width = 0.0;
        double height = 0.0;
        double detail = 1.0;
    };
    std::vector<SeamIsland> seamIslands;
    for (const auto& seamIt : m_seamCharts) {
        const SeamChart& seam = *seamIt.second;
        double detail = 1.0;
        for (const auto& id : { seam.componentLarge, seam.componentSmall }) {
            auto findDetail = componentDetail.find(id);
            if (findDetail != componentDetail.end())
                detail = std::max(detail, findDetail->second);
        }
        // by the side they face
        std::map<int, std::vector<std::array<dust3d::PositionKey, 3>>> facing;
        for (const auto& it : seam.cornerSmall) {
            auto findA = positionKeyToVertex.find(it.first[0]);
            auto findB = positionKeyToVertex.find(it.first[1]);
            auto findC = positionKeyToVertex.find(it.first[2]);
            if (findA == positionKeyToVertex.end() || findB == positionKeyToVertex.end() || findC == positionKeyToVertex.end())
                continue;
            dust3d::Vector3 normal = dust3d::Vector3::crossProduct(findB->second - findA->second, findC->second - findA->second);
            int side = 0;
            double most = -1.0;
            for (int axis = 0; axis < 3; ++axis) {
                if (std::abs(normal[axis]) > most) {
                    most = std::abs(normal[axis]);
                    side = axis * 2 + (normal[axis] < 0.0 ? 1 : 0);
                }
            }
            facing[side].push_back(it.first);
        }
        for (const auto& sideIt : facing) {
            const int axis = sideIt.first / 2;
            const int axisU = 1 == axis ? 0 : (0 == axis ? 2 : 0);
            const int axisV = 1 == axis ? 2 : 1;
            // touching each other: joined through the edges they share
            const auto& triangles = sideIt.second;
            std::vector<size_t> group(triangles.size());
            for (size_t t = 0; t < group.size(); ++t)
                group[t] = t;
            std::function<size_t(size_t)> root = [&](size_t t) {
                while (group[t] != t)
                    t = group[t] = group[group[t]];
                return t;
            };
            std::map<std::pair<dust3d::PositionKey, dust3d::PositionKey>, size_t> edgeOwner;
            for (size_t t = 0; t < triangles.size(); ++t) {
                for (size_t k = 0; k < 3; ++k) {
                    auto a = triangles[t][k], b = triangles[t][(k + 1) % 3];
                    auto edge = a < b ? std::make_pair(a, b) : std::make_pair(b, a);
                    auto inserted = edgeOwner.insert({ edge, t });
                    if (!inserted.second)
                        group[root(t)] = root(inserted.first->second);
                }
            }
            std::map<size_t, std::vector<size_t>> islands;
            for (size_t t = 0; t < triangles.size(); ++t)
                islands[root(t)].push_back(t);
            for (const auto& islandIt : islands) {
                SeamIsland island;
                island.chart = std::make_unique<SeamChart>();
                island.chart->colorLarge = seam.colorLarge;
                island.chart->colorSmall = seam.colorSmall;
                island.chart->componentLarge = seam.componentLarge;
                island.chart->componentSmall = seam.componentSmall;
                island.detail = detail;
                double minU = std::numeric_limits<double>::max(), maxU = std::numeric_limits<double>::lowest();
                double minV = minU, maxV = maxU;
                for (size_t t : islandIt.second) {
                    for (size_t k = 0; k < 3; ++k) {
                        const auto& position = positionKeyToVertex.find(triangles[t][k])->second;
                        minU = std::min(minU, position[axisU]);
                        maxU = std::max(maxU, position[axisU]);
                        minV = std::min(minV, position[axisV]);
                        maxV = std::max(maxV, position[axisV]);
                    }
                }
                island.width = maxU - minU;
                island.height = maxV - minV;
                // (seen nearly edge on in one direction: not thinner than a fifth of its length)
                double thin = 0.2 * std::max(island.width, island.height);
                if (thin <= 0.0)
                    continue;
                double padU = std::max(0.0, thin - island.width) * 0.5, padV = std::max(0.0, thin - island.height) * 0.5;
                island.width += 2.0 * padU;
                island.height += 2.0 * padV;
                for (size_t t : islandIt.second) {
                    std::array<dust3d::Vector2, 3> flat;
                    for (size_t k = 0; k < 3; ++k) {
                        const auto& position = positionKeyToVertex.find(triangles[t][k])->second;
                        // (up in the model is up in the texture)
                        flat[k] = dust3d::Vector2((position[axisU] - minU + padU) / island.width,
                            1 == axis ? (position[axisV] - minV + padV) / island.height : 1.0 - (position[axisV] - minV + padV) / island.height);
                    }
                    island.flat[triangles[t]] = flat;
                    island.chart->cornerSmall[triangles[t]] = seam.cornerSmall.find(triangles[t])->second;
                }
                totalImagelessArea += island.width * island.height * detail * detail;
                seamIslands.push_back(std::move(island));
            }
        }
    }
    m_seamCharts.clear();
    const double imagelessSizeScale = totalImagelessArea > 0.0
        ? (double)m_atlasSize / std::sqrt(totalImagelessArea)
        : 1.0;

    for (auto& island : seamIslands) {
        dust3d::UvMapPacker::Part seamPart;
        seamPart.sourceId = dust3d::Uuid::createUuid();
        seamPart.color = island.chart->colorLarge;
        seamPart.width = std::max(2.0, island.width * island.detail * imagelessSizeScale);
        seamPart.height = std::max(2.0, island.height * island.detail * imagelessSizeScale);
        seamPart.localUv = island.flat;
        m_mapPacker->addPart(seamPart);
        m_seamCharts[seamPart.sourceId] = std::move(island.chart);
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
            // As long and as wide in the atlas as it is on the model (the unwrap of a tube is a
            // unit square whatever the tube's proportions): the same texel density both ways.
            double alongU = 0.0, alongV = 0.0;
            for (const auto& it : componentTriangleUvIt.second) {
                auto findA = positionKeyToVertex.find(it.first[0]);
                auto findB = positionKeyToVertex.find(it.first[1]);
                auto findC = positionKeyToVertex.find(it.first[2]);
                if (findA == positionKeyToVertex.end() || findB == positionKeyToVertex.end() || findC == positionKeyToVertex.end())
                    continue;
                double du1 = it.second[1].x() - it.second[0].x(), dv1 = it.second[1].y() - it.second[0].y();
                double du2 = it.second[2].x() - it.second[0].x(), dv2 = it.second[2].y() - it.second[0].y();
                double determinant = du1 * dv2 - du2 * dv1;
                if (std::abs(determinant) < 1e-14)
                    continue;
                dust3d::Vector3 e1 = findB->second - findA->second, e2 = findC->second - findA->second;
                alongU += ((e1 * dv2 - e2 * dv1) / determinant).length() * std::abs(determinant);
                alongV += ((e2 * du1 - e1 * du2) / determinant).length() * std::abs(determinant);
            }
            double aspect = alongU > 0.0 && alongV > 0.0 ? std::sqrt(std::max(0.2, std::min(5.0, alongU / alongV))) : 1.0;
            width = std::max(1.0, side * aspect);
            height = std::max(1.0, side / aspect);
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
    // (the middle of a small chart of the component's colour: its corner would be filtered
    // with whatever chart lies next to it)
    const auto zeroUv = std::array<dust3d::Vector2, 3> {
        dust3d::Vector2(0.5, 0.5), dust3d::Vector2(0.5, 0.5), dust3d::Vector2(0.5, 0.5)
    };
    for (const auto& brokenTrianglesToComponentIdIt : m_object->brokenTrianglesToComponentIdMap) {
        partWithBrokenTriangles[brokenTrianglesToComponentIdIt.second].localUv.insert({ brokenTrianglesToComponentIdIt.first, zeroUv });
    }
    for (auto& partIt : partWithBrokenTriangles) {
        auto componentIt = m_snapshot->components.find(partIt.first.toString());
        if (componentIt == m_snapshot->components.end())
            continue;
        dust3d::Color color(1.0, 1.0, 1.0);
        double width = std::max(4.0, (double)m_atlasSize / 64.0);
        double height = std::max(4.0, (double)m_atlasSize / 64.0);
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
    // A small atlas is painted several times finer and then reduced: every texel is the
    // average of what was painted inside it, so a stroke a texel or two wide (an eye, a seam,
    // a fold) comes out soft and whole instead of as a row of steps.
    const size_t bakeSize = m_painted ? m_atlasSize * (m_atlasSize <= 256 ? 4 : (m_atlasSize <= 1024 ? 2 : 1)) : m_atlasSize;
    m_textureColorImage = std::make_unique<QImage>(bakeSize, bakeSize, QImage::Format_ARGB32);
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
    const int bleedPixels = std::max(2, (int)(bakeSize * 32 / 4096));

    for (const auto& layout : m_mapPacker->packedLayouts()) {
        int chartW = (int)(layout.width * bakeSize);
        int chartH = (int)(layout.height * bakeSize);
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
        colorTexturePainter.drawPixmap(layout.left * bakeSize - bleedPixels,
            layout.top * bakeSize - bleedPixels,
            brushPixmap);
    }

    colorTexturePainter.end();

    bakeSurfaceDetailColors();

    dilateTexture(m_textureColorImage.get());

    if (bakeSize != m_atlasSize)
        m_textureColorImage = std::make_unique<QImage>(m_textureColorImage->scaled((int)m_atlasSize, (int)m_atlasSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
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
    const dust3d::Vector3& n0 = triangle.hasCornerNormals ? triangle.cornerNormal[0] : vertexNormals[triangle.vertices[0]];
    const dust3d::Vector3& n1 = triangle.hasCornerNormals ? triangle.cornerNormal[1] : vertexNormals[triangle.vertices[1]];
    const dust3d::Vector3& n2 = triangle.hasCornerNormals ? triangle.cornerNormal[2] : vertexNormals[triangle.vertices[2]];
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

    // the model's triangles by their corners, for the normals it is lit with
    const auto* objectNormals = m_object->triangleVertexNormals();
    std::map<std::array<dust3d::PositionKey, 3>, size_t> objectTriangles;
    if (nullptr != objectNormals && objectNormals->size() == m_object->triangles.size()) {
        for (size_t t = 0; t < m_object->triangles.size(); ++t) {
            const auto& triangle = m_object->triangles[t];
            if (triangle.size() != 3 || (*objectNormals)[t].size() != 3)
                continue;
            std::array<dust3d::PositionKey, 3> key = { dust3d::PositionKey(m_object->vertices[triangle[0]]),
                dust3d::PositionKey(m_object->vertices[triangle[1]]), dust3d::PositionKey(m_object->vertices[triangle[2]]) };
            std::sort(key.begin(), key.end());
            objectTriangles.insert({ key, t });
        }
    }

    // The paint parts, and the group each is in: a stroke paints the components of its group
    // (one at the top of the tree paints everything).
    m_surfacePaints.clear();
    std::map<std::string, std::string> parents;
    std::vector<std::string> paintScopes;
    if (!m_object->paintStrokes.empty()) {
        for (const auto& it : m_snapshot->components) {
            for (const auto& childId : dust3d::String::split(dust3d::String::valueOrEmpty(it.second, "children"), ','))
                parents[childId] = it.first;
        }
        for (const auto& stroke : m_object->paintStrokes) {
            m_surfacePaints.push_back(std::make_unique<dust3d::SurfacePaint>(stroke));
            auto findParent = parents.find(stroke.componentId.toString());
            paintScopes.push_back(findParent == parents.end() ? std::string() : findParent->second);
        }
    }
    auto isWithin = [&](const dust3d::Uuid& componentId, const std::string& scope) {
        if (scope.empty())
            return true;
        for (auto it = parents.find(componentId.toString()); it != parents.end(); it = parents.find(it->second)) {
            if (it->second == scope)
                return true;
        }
        return false;
    };

    // how open every vertex of the model is, for the shading painted into the colour
    std::map<dust3d::PositionKey, double> occlusion;
    m_occlusionCaster.reset();
    if (m_shading > 0.0) {
        std::map<dust3d::PositionKey, size_t> welded;
        std::vector<dust3d::Vector3> weldedVertices;
        std::vector<std::vector<size_t>> weldedTriangles;
        for (const auto& triangle : m_object->triangles) {
            if (triangle.size() != 3)
                continue;
            std::vector<size_t> corners;
            for (size_t i = 0; i < 3; ++i) {
                auto inserted = welded.insert({ dust3d::PositionKey(m_object->vertices[triangle[i]]), weldedVertices.size() });
                if (inserted.second)
                    weldedVertices.push_back(m_object->vertices[triangle[i]]);
                corners.push_back(inserted.first->second);
            }
            if (corners[0] != corners[1] && corners[1] != corners[2] && corners[0] != corners[2])
                weldedTriangles.push_back(corners);
        }
        // texel by texel where that is affordable (a low poly model on a small atlas: the soft
        // shadows it is painted with), at the vertices otherwise
        if ((double)weldedTriangles.size() * (double)m_atlasSize * (double)m_atlasSize <= 6.0e9) {
            m_occlusionCaster = std::make_unique<dust3d::OcclusionCaster>(weldedVertices, weldedTriangles);
        } else {
            std::vector<double> open = dust3d::computeVertexOcclusion(weldedVertices, weldedTriangles);
            for (const auto& it : welded)
                occlusion[it.first] = open[it.second];
        }
    }

    static const std::map<std::string, std::string> noComponent;
    for (const auto& layout : m_mapPacker->packedLayouts()) {
        if (!layout.id.isNull() || layout.sourceId.isNull())
            continue;
        auto componentIt = m_snapshot->components.find(layout.sourceId.toString());
        auto seamIt = m_seamCharts.find(layout.sourceId);
        const SeamChart* seam = seamIt == m_seamCharts.end() ? nullptr : seamIt->second.get();
        if (componentIt == m_snapshot->components.end() && nullptr == seam)
            continue;
        const auto& component = nullptr != seam ? noComponent : componentIt->second;
        std::string wrapMode = dust3d::String::valueOrEmpty(component, "wrap");
        auto readFloat = [&](const char* name, double defaultValue) {
            std::string value = dust3d::String::valueOrEmpty(component, name);
            return value.empty() ? defaultValue : (double)dust3d::String::toFloat(value);
        };
        // an animal coat and folds belong to a wrap; an ornament to any part or group
        auto patternType = wrapMode.empty() ? dust3d::SurfacePattern::Type::None
                                            : dust3d::SurfacePattern::typeFromString(dust3d::String::valueOrEmpty(component, "wrapPattern"));
        double wrinkleStrength = wrapMode.empty() ? 0.0 : std::max(0.0, std::min(1.0, readFloat("wrapWrinkles", 0.0)));
        auto ornamentType = dust3d::SurfaceOrnament::typeFromString(dust3d::String::valueOrEmpty(component, "ornament"));
        if (dust3d::SurfacePattern::Type::None == patternType && wrinkleStrength <= 0.0
            && dust3d::SurfaceOrnament::Type::None == ornamentType && !m_painted)
            continue;

        auto chart = std::make_unique<SurfaceDetailChart>();
        chart->layout = &layout;
        chart->baseColor = layout.color;
        chart->seam = seam;
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
            {
                std::array<dust3d::PositionKey, 3> sorted = it.first;
                std::sort(sorted.begin(), sorted.end());
                auto findTriangle = objectTriangles.find(sorted);
                if (findTriangle != objectTriangles.end()) {
                    const auto& objectTriangle = m_object->triangles[findTriangle->second];
                    const auto& normals = (*objectNormals)[findTriangle->second];
                    size_t matched = 0;
                    for (size_t i = 0; i < 3; ++i) {
                        for (size_t corner = 0; corner < 3; ++corner) {
                            if (dust3d::PositionKey(m_object->vertices[objectTriangle[corner]]) == it.first[i]) {
                                triangle.cornerNormal[i] = normals[corner];
                                ++matched;
                                break;
                            }
                        }
                    }
                    triangle.hasCornerNormals = 3 == matched;
                }
            }
            if (nullptr != seam) {
                auto findCorners = seam->cornerSmall.find(it.first);
                if (findCorners != seam->cornerSmall.end()) {
                    for (size_t i = 0; i < 3; ++i)
                        triangle.cornerSmall[i] = findCorners->second[i];
                }
            }
            chart->triangles.push_back(triangle);
        }
        if (chart->triangles.empty())
            continue;

        if (!m_surfacePaints.empty()) {
            dust3d::Vector3 lower = chart->vertices.front(), upper = chart->vertices.front();
            for (const auto& vertex : chart->vertices) {
                for (size_t axis = 0; axis < 3; ++axis) {
                    lower[axis] = std::min(lower[axis], vertex[axis]);
                    upper[axis] = std::max(upper[axis], vertex[axis]);
                }
            }
            for (size_t i = 0; i < m_surfacePaints.size(); ++i) {
                bool inScope = nullptr != seam
                    ? (isWithin(seam->componentLarge, paintScopes[i]) || isWithin(seam->componentSmall, paintScopes[i]))
                    : isWithin(layout.sourceId, paintScopes[i]);
                if (inScope && m_surfacePaints[i]->reaches(lower, upper))
                    chart->paints.push_back(m_surfacePaints[i].get());
            }
        }
        if (!occlusion.empty()) {
            chart->vertexOcclusion.assign(chart->vertices.size(), 1.0);
            for (size_t i = 0; i < chart->vertices.size(); ++i) {
                auto findOcclusion = occlusion.find(dust3d::PositionKey(chart->vertices[i]));
                if (findOcclusion != occlusion.end())
                    chart->vertexOcclusion[i] = findOcclusion->second;
            }
        }

        uint32_t seed = dust3d::SurfacePattern::seedFromString(layout.sourceId.toString());
        if (dust3d::SurfacePattern::Type::None != patternType) {
            dust3d::SurfacePattern::Settings settings;
            settings.type = patternType;
            settings.baseColor = layout.color;
            std::string patternColor = dust3d::String::valueOrEmpty(component, "wrapPatternColor");
            settings.patternColor = patternColor.empty() ? dust3d::SurfacePattern::defaultPatternColor(layout.color) : dust3d::Color(patternColor);
            settings.scale = std::max(0.005, readFloat("wrapPatternScale", 0.06));
            settings.belly = readFloat("wrapBelly", 0.0);
            settings.relief = std::max(0.0, std::min(2.0, readFloat("wrapPatternRelief", 1.0)));
            std::string bellyColor = dust3d::String::valueOrEmpty(component, "wrapBellyColor");
            if (!bellyColor.empty())
                settings.bellyColor = dust3d::Color(bellyColor);
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
        if (dust3d::SurfaceOrnament::Type::None != ornamentType) {
            dust3d::SurfaceOrnament::Settings settings;
            settings.type = ornamentType;
            settings.baseColor = layout.color;
            std::string ornamentColor = dust3d::String::valueOrEmpty(component, "ornamentColor");
            settings.ornamentColor = ornamentColor.empty() ? dust3d::SurfaceOrnament::defaultOrnamentColor(layout.color) : dust3d::Color(ornamentColor);
            settings.scale = std::max(0.002, readFloat("ornamentScale", 0.03));
            settings.relief = readFloat("ornamentRelief", 1.0);
            chart->ornament = std::make_unique<dust3d::SurfaceOrnament>(settings);
            // the chart's rectangle in the atlas, and how long its two sides are on the model
            // (the mean stretch of the unwrap each way: a tube's chart is not isometric)
            // The flat ends of a tube share its chart but are not part of its wall. Around a
            // closed tube the directions of one UV axis cancel out, and the other axis runs
            // along the tube: the faces looking along it are the end caps, which stay plain.
            struct Jacobian {
                dust3d::Vector3 alongU;
                dust3d::Vector3 alongV;
                double weight = 0.0;
            };
            std::vector<Jacobian> jacobians(chart->triangles.size());
            dust3d::Vector3 sumU, sumV;
            double sumLengthU = 0.0, sumLengthV = 0.0;
            for (size_t t = 0; t < chart->triangles.size(); ++t) {
                const auto& triangle = chart->triangles[t];
                double du1 = triangle.u[1] - triangle.u[0], dv1 = triangle.v[1] - triangle.v[0];
                double du2 = triangle.u[2] - triangle.u[0], dv2 = triangle.v[2] - triangle.v[0];
                double determinant = du1 * dv2 - du2 * dv1;
                if (std::abs(determinant) < 1e-14)
                    continue;
                dust3d::Vector3 e1 = triangle.position[1] - triangle.position[0];
                dust3d::Vector3 e2 = triangle.position[2] - triangle.position[0];
                jacobians[t].alongU = (e1 * dv2 - e2 * dv1) / determinant;
                jacobians[t].alongV = (e2 * du1 - e1 * du2) / determinant;
                jacobians[t].weight = std::abs(determinant);
                sumU += jacobians[t].alongU * jacobians[t].weight;
                sumV += jacobians[t].alongV * jacobians[t].weight;
                sumLengthU += jacobians[t].alongU.length() * jacobians[t].weight;
                sumLengthV += jacobians[t].alongV.length() * jacobians[t].weight;
            }
            chart->ornamentSkipped.assign(chart->triangles.size(), false);
            double closedU = sumLengthU > 0.0 ? sumU.length() / sumLengthU : 1.0;
            double closedV = sumLengthV > 0.0 ? sumV.length() / sumLengthV : 1.0;
            if (std::min(closedU, closedV) < 0.35 * std::max(closedU, closedV) && std::max(closedU, closedV) > 0.1) {
                dust3d::Vector3 axis = (closedU < closedV ? sumV : sumU).normalized();
                size_t skipped = 0;
                for (size_t t = 0; t < chart->triangles.size(); ++t) {
                    const auto& triangle = chart->triangles[t];
                    dust3d::Vector3 normal = dust3d::Vector3::normal(triangle.position[0], triangle.position[1], triangle.position[2]);
                    if (std::abs(dust3d::Vector3::dotProduct(normal, axis)) > 0.8) {
                        chart->ornamentSkipped[t] = true;
                        ++skipped;
                    }
                }
                if (skipped == chart->triangles.size())
                    chart->ornamentSkipped.assign(chart->triangles.size(), false);
            }
            double minU = 1e30, maxU = -1e30, minV = 1e30, maxV = -1e30;
            double stretchU = 0.0, stretchV = 0.0, weight = 0.0;
            for (size_t t = 0; t < chart->triangles.size(); ++t) {
                if (chart->ornamentSkipped[t] || jacobians[t].weight <= 0.0)
                    continue;
                const auto& triangle = chart->triangles[t];
                for (size_t i = 0; i < 3; ++i) {
                    minU = std::min(minU, triangle.u[i]);
                    maxU = std::max(maxU, triangle.u[i]);
                    minV = std::min(minV, triangle.v[i]);
                    maxV = std::max(maxV, triangle.v[i]);
                }
                stretchU += jacobians[t].alongU.length() * jacobians[t].weight;
                stretchV += jacobians[t].alongV.length() * jacobians[t].weight;
                weight += jacobians[t].weight;
            }
            if (weight > 0.0 && maxU > minU && maxV > minV) {
                chart->left = minU;
                chart->top = minV;
                chart->width = maxU - minU;
                chart->height = maxV - minV;
                chart->ornament->setChartSize(stretchU / weight * chart->width, stretchV / weight * chart->height);
                if (chart->ornament->hangs()) {
                    // the vertical axis the strands are counted around: through the middle of
                    // the surface, as far from it as the surface is on average
                    dust3d::Vector3 centre;
                    double area = 0.0;
                    for (const auto& triangle : chart->triangles) {
                        double a = dust3d::Vector3::area(triangle.position[0], triangle.position[1], triangle.position[2]);
                        centre += (triangle.position[0] + triangle.position[1] + triangle.position[2]) * (a / 3.0);
                        area += a;
                    }
                    if (area > 0.0)
                        centre /= area;
                    double radius = 0.0;
                    for (const auto& triangle : chart->triangles) {
                        double a = dust3d::Vector3::area(triangle.position[0], triangle.position[1], triangle.position[2]);
                        dust3d::Vector3 middle = (triangle.position[0] + triangle.position[1] + triangle.position[2]) / 3.0 - centre;
                        radius += std::sqrt(middle.x() * middle.x() + middle.z() * middle.z()) * a;
                    }
                    chart->ornament->setHangAxis(centre, area > 0.0 ? radius / area : 1.0);
                }
            } else {
                chart->ornament.reset();
            }
        }
        if (nullptr == chart->pattern && nullptr == chart->folds && nullptr == chart->ornament
            && chart->paints.empty() && chart->vertexOcclusion.empty() && nullptr == m_occlusionCaster)
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
            if (nullptr != chart->seam) {
                double small = 0.0;
                for (size_t i = 0; i < 3; ++i)
                    small += std::max(0.0, w[i]) * triangle.cornerSmall[i];
                // (one part's colour up to the middle of the join and the other's from there,
                // with a short blend between: a seam, not a gradient smeared over both)
                small = std::max(0.0, std::min(1.0, (small - 0.4) / 0.2));
                small = small * small * (3.0 - 2.0 * small);
                const auto& from = chart->seam->colorLarge;
                const auto& to = chart->seam->colorSmall;
                color = dust3d::Color(from.r() + (to.r() - from.r()) * small, from.g() + (to.g() - from.g()) * small,
                    from.b() + (to.b() - from.b()) * small, from.alpha() + (to.alpha() - from.alpha()) * small);
            }
            if (chart->folds) {
                // the creases a little darker (they catch less light in any engine)
                double cavity = 0.0;
                chart->folds->evaluate(frame.position, frame.normal, frame.tangent, frame.bitangent, footprint, nullptr, &cavity);
                double shade = 1.0 - 0.3 * std::min(1.0, cavity);
                color = dust3d::Color(color.r() * shade, color.g() * shade, color.b() * shade, color.alpha());
            }
            if (chart->ornament && !chart->ornamentSkipped[t]) {
                double u = triangle.u[0] * w[0] + triangle.u[1] * w[1] + triangle.u[2] * w[2];
                double v = triangle.v[0] * w[0] + triangle.v[1] * w[1] + triangle.v[2] * w[2];
                color = chart->ornament->hangs() ? chart->ornament->colorAtPosition(color, frame.position, footprint)
                                                 : chart->ornament->colorAt(color, chart->ornamentU(u), chart->ornamentV(v), footprint);
            }
            if (!chart->paints.empty()) {
                // The strokes are tried against the rounded form the flat triangle stands for
                // (the point carried towards the planes its corners' normals give it), so the
                // edge of a stroke runs over the facets in one curve and is not scalloped by them.
                dust3d::Vector3 rounded;
                double weightSum = 0.0;
                for (size_t i = 0; i < 3; ++i) {
                    double weight = std::max(0.0, w[i]);
                    const dust3d::Vector3& cornerNormal = triangle.hasCornerNormals ? triangle.cornerNormal[i] : vertexNormals[triangle.vertices[i]];
                    dust3d::Vector3 onPlane = frame.position - cornerNormal * dust3d::Vector3::dotProduct(frame.position - triangle.position[i], cornerNormal);
                    rounded += onPlane * weight;
                    weightSum += weight;
                }
                dust3d::Vector3 at = weightSum > 1e-12 ? frame.position + (rounded / weightSum - frame.position) * 0.75 : frame.position;
                for (const auto* paint : chart->paints)
                    color = paint->colorAt(color, at, footprint);
            }
            if (nullptr != m_occlusionCaster) {
                color = dust3d::bakedShade(color, m_occlusionCaster->occlusionAt(frame.position, frame.normal), frame.normal, m_shading);
            } else if (!chart->vertexOcclusion.empty()) {
                double c0 = std::max(0.0, w[0]), c1 = std::max(0.0, w[1]), c2 = std::max(0.0, w[2]);
                double open = (chart->vertexOcclusion[triangle.vertices[0]] * c0 + chart->vertexOcclusion[triangle.vertices[1]] * c1
                                  + chart->vertexOcclusion[triangle.vertices[2]] * c2)
                    / std::max(1e-12, c0 + c1 + c2);
                color = dust3d::bakedShade(color, open, frame.normal, m_shading);
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
    auto hasRelief = [](const SurfaceDetailChart& chart) {
        return nullptr != chart.folds || (nullptr != chart.ornament && chart.ornament->settings().relief > 0.0)
            || (nullptr != chart.pattern && chart.pattern->hasRelief());
    };
    bool anyWrinkles = false, anyOrnament = false;
    for (const auto& chart : m_surfaceDetailCharts) {
        anyWrinkles = anyWrinkles || hasRelief(*chart);
        // (a skin's relief is as fine as an ornament's)
        anyOrnament = anyOrnament || nullptr != chart->ornament || (nullptr != chart->pattern && chart->pattern->hasRelief());
    }
    if (!anyWrinkles)
        return;

    // half the colour atlas: folds are wider than the finest colour detail (an ornament is
    // not: its normal map is as fine as its colour)
    const int size = anyOrnament ? (int)m_atlasSize : (int)std::max((size_t)256, m_atlasSize / 2);
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
                wrinkled = wrinkled || (chart->layout == &layout && hasRelief(*chart));
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
        if (!hasRelief(*chart))
            continue;
        std::vector<dust3d::Vector3> vertexNormals = chartVertexNormals(*chart);
        rasterizeChart(chart->triangles, size, size, 2.0, [&](int x, int y, size_t t, const double w[3]) {
            const auto& triangle = chart->triangles[t];
            TexelFrame frame = texelFrame(triangle, vertexNormals, w);
            double footprint = triangle.worldPerUv / size;
            dust3d::Vector3 normal(0.0, 0.0, 1.0);
            if (chart->folds)
                chart->folds->evaluate(frame.position, frame.normal, frame.tangent, frame.bitangent, footprint, &normal, nullptr);
            if (chart->ornament && chart->ornament->settings().relief > 0.0 && !chart->ornamentSkipped[t] && !chart->ornament->hangs()) {
                // the slope of the relief along the tangent (increasing u) and the bitangent
                // (decreasing v), by central differences one texel apart
                double u = chart->ornamentU(triangle.u[0] * w[0] + triangle.u[1] * w[1] + triangle.u[2] * w[2]);
                double v = chart->ornamentV(triangle.v[0] * w[0] + triangle.v[1] * w[1] + triangle.v[2] * w[2]);
                double stepU = std::max(1e-6, footprint / chart->ornament->lengthU());
                double stepV = std::max(1e-6, footprint / chart->ornament->lengthV());
                double slopeU = (chart->ornament->heightAt(u + stepU, v, footprint) - chart->ornament->heightAt(u - stepU, v, footprint))
                    / (2.0 * stepU * chart->ornament->lengthU());
                double slopeV = (chart->ornament->heightAt(u, v - stepV, footprint) - chart->ornament->heightAt(u, v + stepV, footprint))
                    / (2.0 * stepV * chart->ornament->lengthV());
                normal = dust3d::Vector3(normal.x() - slopeU * normal.z(), normal.y() - slopeV * normal.z(), normal.z()).normalized();
            }
            if (chart->pattern && chart->pattern->hasRelief()) {
                // the skin's own relief (scales, bumps): its slope along the tangent and the
                // bitangent, from the pattern's height a texel to either side on the surface
                double step = std::max(1e-6, footprint);
                double slopeT = (chart->pattern->heightAt(frame.position + frame.tangent * step, frame.normal, footprint)
                                    - chart->pattern->heightAt(frame.position - frame.tangent * step, frame.normal, footprint))
                    / (2.0 * step);
                double slopeB = (chart->pattern->heightAt(frame.position + frame.bitangent * step, frame.normal, footprint)
                                    - chart->pattern->heightAt(frame.position - frame.bitangent * step, frame.normal, footprint))
                    / (2.0 * step);
                normal = dust3d::Vector3(normal.x() - slopeT * normal.z(), normal.y() - slopeB * normal.z(), normal.z()).normalized();
            }
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

    const int size = (int)std::min((size_t)1024, m_atlasSize);
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
    if (image->format() != QImage::Format_ARGB32)
        *image = image->convertToFormat(QImage::Format_ARGB32);
    const int w = image->width();
    const int h = image->height();
    const QRgb emptyPixel = qRgba(0, 255, 0, 0);
    // (straight on the pixels: QImage::pixel and setPixel check and copy on every call, which on
    // a 4096 atlas is most of the time this takes)
    QRgb* pixels = reinterpret_cast<QRgb*>(image->bits());
    const qsizetype stride = image->bytesPerLine() / (qsizetype)sizeof(QRgb);
    auto at = [&](int x, int y) -> QRgb& { return pixels[(qsizetype)y * stride + x]; };

    std::vector<uint8_t> filled((size_t)w * h, 0);
    std::vector<int> frontier;
    frontier.reserve((size_t)w * 4);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (at(x, y) != emptyPixel) {
                filled[(size_t)y * w + x] = 1;
                bool onBorder = (x > 0 && at(x - 1, y) == emptyPixel)
                    || (x < w - 1 && at(x + 1, y) == emptyPixel)
                    || (y > 0 && at(x, y - 1) == emptyPixel)
                    || (y < h - 1 && at(x, y + 1) == emptyPixel);
                if (onBorder)
                    frontier.push_back(y * w + x);
            }
        }
    }

    const int dx[] = { -1, 1, 0, 0 };
    const int dy[] = { 0, 0, -1, 1 };

    // breadth first, in the same order as a queue
    for (size_t head = 0; head < frontier.size(); ++head) {
        int idx = frontier[head];
        int cx = idx % w;
        int cy = idx / w;
        QRgb color = at(cx, cy);

        for (int d = 0; d < 4; ++d) {
            int nx = cx + dx[d];
            int ny = cy + dy[d];
            if (nx < 0 || nx >= w || ny < 0 || ny >= h)
                continue;
            size_t nidx = (size_t)ny * w + nx;
            if (!filled[nidx]) {
                filled[nidx] = 1;
                at(nx, ny) = color;
                frontier.push_back((int)nidx);
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
