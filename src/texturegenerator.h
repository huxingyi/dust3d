#ifndef DUST3D_TEXTURE_GENERATOR_H
#define DUST3D_TEXTURE_GENERATOR_H
#include <QObject>
#include <vector>
#include <QImage>
#include <QColor>
#include <QPixmap>
#include "object.h"
#include "model.h"
#include "snapshot.h"
#include "preferences.h"

class TextureGenerator : public QObject
{
    Q_OBJECT
public:
    TextureGenerator(const Object &object, Snapshot *snapshot=nullptr);
    ~TextureGenerator();
    QImage *takeResultTextureColorImage();
    QImage *takeResultTextureNormalImage();
    QImage *takeResultTextureRoughnessImage();
    QImage *takeResultTextureMetalnessImage();
    QImage *takeResultTextureAmbientOcclusionImage();
    Object *takeObject();
    Model *takeResultMesh();
    bool hasTransparencySettings();
    void addPartColorMap(QUuid partId, const QImage *image, float tileScale);
    void addPartNormalMap(QUuid partId, const QImage *image, float tileScale);
    void addPartMetalnessMap(QUuid partId, const QImage *image, float tileScale);
    void addPartRoughnessMap(QUuid partId, const QImage *image, float tileScale);
    void addPartAmbientOcclusionMap(QUuid partId, const QImage *image, float tileScale);
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
    Object *m_object = nullptr;
    QImage *m_resultTextureColorImage = nullptr;
    QImage *m_resultTextureNormalImage = nullptr;
    QImage *m_resultTextureRoughnessImage = nullptr;
    QImage *m_resultTextureMetalnessImage = nullptr;
    QImage *m_resultTextureAmbientOcclusionImage = nullptr;
    Model *m_resultMesh = nullptr;
    std::map<QUuid, std::pair<QImage, float>> m_partColorTextureMap;
    std::map<QUuid, std::pair<QImage, float>> m_partNormalTextureMap;
    std::map<QUuid, std::pair<QImage, float>> m_partMetalnessTextureMap;
    std::map<QUuid, std::pair<QImage, float>> m_partRoughnessTextureMap;
    std::map<QUuid, std::pair<QImage, float>> m_partAmbientOcclusionTextureMap;
    std::set<QUuid> m_countershadedPartIds;
    Snapshot *m_snapshot = nullptr;
    bool m_hasTransparencySettings = false;
    int m_textureSize = Preferences::instance().textureSize();
};

#endif
