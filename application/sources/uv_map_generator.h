#ifndef DUST3D_APPLICATION_UV_MAP_GENERATOR_H_
#define DUST3D_APPLICATION_UV_MAP_GENERATOR_H_

#include "model_mesh.h"
#include <QImage>
#include <QObject>
#include <dust3d/base/object.h>
#include <dust3d/base/snapshot.h>
#include <dust3d/uv/uv_map_packer.h>
#include <map>
#include <memory>
#include <vector>

namespace dust3d {
class SurfacePaint;
class OcclusionCaster;
}

class UvMapGenerator : public QObject {
    Q_OBJECT
public:
    UvMapGenerator(std::unique_ptr<dust3d::Object> object, std::unique_ptr<dust3d::Snapshot> snapshot);
    ~UvMapGenerator();
    void generate();
    std::unique_ptr<QImage> takeResultTextureColorImage();
    std::unique_ptr<QImage> takeResultTextureNormalImage();
    std::unique_ptr<QImage> takeResultTextureRoughnessImage();
    std::unique_ptr<QImage> takeResultTextureMetalnessImage();
    std::unique_ptr<QImage> takeResultTextureAmbientOcclusionImage();
    std::unique_ptr<QImage> takeResultTextureEmissiveImage();
    std::unique_ptr<ModelMesh> takeResultMesh();
    std::unique_ptr<dust3d::Object> takeObject();
    bool hasTransparencySettings() const;
    // A wrap chart painted texel by texel (see prepareSurfaceDetails()).
    struct SurfaceDetailChart;
    static QImage* combineMetalnessRoughnessAmbientOcclusionImages(QImage* metalnessImage,
        QImage* roughnessImage,
        QImage* ambientOcclusionImage);
signals:
    void finished();
public slots:
    void process();

private:
    std::unique_ptr<dust3d::Object> m_object;
    std::unique_ptr<dust3d::Snapshot> m_snapshot;
    std::unique_ptr<dust3d::UvMapPacker> m_mapPacker;
    std::unique_ptr<QImage> m_textureColorImage;
    std::unique_ptr<QImage> m_textureNormalImage;
    std::unique_ptr<QImage> m_textureRoughnessImage;
    std::unique_ptr<QImage> m_textureMetalnessImage;
    std::unique_ptr<QImage> m_textureAmbientOcclusionImage;
    std::unique_ptr<QImage> m_textureEmissiveImage;
    std::unique_ptr<ModelMesh> m_mesh;
    bool m_hasTransparencySettings = false;
    static size_t m_textureSize;
    // The side of this document's atlas, in texels (the canvas' textureSize; 512 or 1024 for
    // a low poly game character), and how much shading is painted into its colour
    // (textureShading, 0..1: see dust3d::bakedShadeFactor).
    size_t m_atlasSize = 4096;
    double m_shading = 0.0;
    // With paint parts or painted shading, every chart is painted texel by texel from the
    // surface behind it, the seams between parts too: each of their triangles then has its own
    // place in the atlas (see packUvs).
    bool m_painted = false;
    struct SeamChart;
    std::map<dust3d::Uuid, std::unique_ptr<SeamChart>> m_seamCharts;
    std::vector<std::unique_ptr<dust3d::SurfacePaint>> m_surfacePaints;
    std::unique_ptr<dust3d::OcclusionCaster> m_occlusionCaster;
    void packUvs();
    void generateTextureColorImage();
    // Wraps with an animal coat (wrapPattern, see dust3d::SurfacePattern) or folds and
    // wrinkles (wrapWrinkles: the fold stamps the mesh generator placed, see
    // dust3d::ClothFolds) are painted texel by texel, from the surface point behind each
    // texel: the colour, and a tangent-space normal map. So is the ornament of any part or
    // group ("ornament", see dust3d::SurfaceOrnament), laid out across its own UV chart.
    void prepareSurfaceDetails();
    bool hasSurfaceDetails(const dust3d::UvMapPacker::Layout* layout) const;
    void bakeSurfaceDetailColors();
    void generateTextureNormalImage();
    // Per-triangle tangents for the viewport's normal mapping (the direction of increasing u;
    // twice as long where the chart is mirrored, see model.vert).
    void resolveTriangleTangents();
    std::vector<std::unique_ptr<SurfaceDetailChart>> m_surfaceDetailCharts;
    void generateTextureMaterialImages();
    void generateTriangleComponentIds();
    void generateUvCoords();
    static void dilateTexture(QImage* image);
};

#endif
