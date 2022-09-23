#ifndef DUST3D_APPLICATION_TEXTURE_GENERATOR_H_
#define DUST3D_APPLICATION_TEXTURE_GENERATOR_H_

#include <QObject>
#include <vector>
#include <QImage>
#include <QColor>
#include <QPixmap>
#include <dust3d/base/object.h>
#include <dust3d/base/snapshot.h>
#include "model_mesh.h"

class TextureGenerator : public QObject
{
    Q_OBJECT
public:
    TextureGenerator(const dust3d::Object &object, dust3d::Snapshot *snapshot=nullptr);
    ~TextureGenerator();
    QImage *takeResultTextureColorImage();
    QImage *takeResultTextureNormalImage();
    QImage *takeResultTextureRoughnessImage();
    QImage *takeResultTextureMetalnessImage();
    QImage *takeResultTextureAmbientOcclusionImage();
    dust3d::Object *takeObject();
    ModelMesh *takeResultMesh();
    bool hasTransparencySettings();
    void addPartColorMap(dust3d::Uuid partId, const QImage *image, float tileScale);
    void addPartNormalMap(dust3d::Uuid partId, const QImage *image, float tileScale);
    void addPartMetalnessMap(dust3d::Uuid partId, const QImage *image, float tileScale);
    void addPartRoughnessMap(dust3d::Uuid partId, const QImage *image, float tileScale);
    void addPartAmbientOcclusionMap(dust3d::Uuid partId, const QImage *image, float tileScale);
    void generate();
    static QImage *combineMetalnessRoughnessAmbientOcclusionImages(QImage *metalnessImage,
            QImage *roughnessImage,
            QImage *ambientOcclusionImage);
signals:
    void finished();
public slots:
    void process();
public:
    static QColor m_defaultTextureColor;
private:
    void prepare();
private:
    dust3d::Object *m_object = nullptr;
    QImage *m_resultTextureColorImage = nullptr;
    QImage *m_resultTextureNormalImage = nullptr;
    QImage *m_resultTextureRoughnessImage = nullptr;
    QImage *m_resultTextureMetalnessImage = nullptr;
    QImage *m_resultTextureAmbientOcclusionImage = nullptr;
    ModelMesh *m_resultMesh = nullptr;
    std::map<dust3d::Uuid, std::pair<QImage, float>> m_partColorTextureMap;
    std::map<dust3d::Uuid, std::pair<QImage, float>> m_partNormalTextureMap;
    std::map<dust3d::Uuid, std::pair<QImage, float>> m_partMetalnessTextureMap;
    std::map<dust3d::Uuid, std::pair<QImage, float>> m_partRoughnessTextureMap;
    std::map<dust3d::Uuid, std::pair<QImage, float>> m_partAmbientOcclusionTextureMap;
    std::set<dust3d::Uuid> m_countershadedPartIds;
    dust3d::Snapshot *m_snapshot = nullptr;
    bool m_hasTransparencySettings = false;
    int m_textureSize = 1024;
};

#endif
