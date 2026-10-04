#ifndef DUST3D_APPLICATION_UV_MAP_GENERATOR_H_
#define DUST3D_APPLICATION_UV_MAP_GENERATOR_H_

#include "model_mesh.h"
#include <QImage>
#include <QObject>
#include <dust3d/base/object.h>
#include <dust3d/base/snapshot.h>
#include <dust3d/uv/uv_map_packer.h>
#include <memory>

struct SurfacePatternRequest;

class UvMapGenerator : public QObject {
    Q_OBJECT
public:
    UvMapGenerator(std::unique_ptr<dust3d::Object> object, std::unique_ptr<dust3d::Snapshot> snapshot);
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
    // Paints the charts of wraps with an animal skin pattern (wrapPattern), texel by texel,
    // from the surface point behind each texel (see dust3d::SurfacePattern).
    bool surfacePatternSettings(const dust3d::Uuid& componentId, const dust3d::Color& baseColor, SurfacePatternRequest* request) const;
    void bakeSurfacePatterns(const std::vector<const dust3d::UvMapPacker::Layout*>& layouts);
    void generateTextureMaterialImages();
    void generateTriangleComponentIds();
    void generateUvCoords();
    static void dilateTexture(QImage* image);
};

#endif
