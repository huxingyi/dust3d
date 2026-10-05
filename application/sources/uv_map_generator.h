#ifndef DUST3D_APPLICATION_UV_MAP_GENERATOR_H_
#define DUST3D_APPLICATION_UV_MAP_GENERATOR_H_

#include "model_mesh.h"
#include <QImage>
#include <QObject>
#include <dust3d/base/object.h>
#include <dust3d/base/snapshot.h>
#include <dust3d/uv/uv_map_packer.h>
#include <memory>
#include <vector>

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
    void packUvs();
    void generateTextureColorImage();
    // Wraps with an animal coat (wrapPattern, see dust3d::SurfacePattern) or folds and
    // wrinkles (wrapWrinkles: the fold stamps the mesh generator placed, see
    // dust3d::ClothFolds) are painted texel by texel, from the surface point behind each
    // texel: the colour, and a tangent-space normal map.
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
