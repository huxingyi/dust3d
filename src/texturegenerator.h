#ifndef DUST3D_TEXTURE_GENERATOR_H
#define DUST3D_TEXTURE_GENERATOR_H
#include <QObject>
#include <vector>
#include <QImage>
#include <QColor>
#include <QPixmap>
#include "outcome.h"
#include "model.h"
#include "snapshot.h"

class TextureGenerator : public QObject
{
    Q_OBJECT
public:
    TextureGenerator(const Outcome &outcome, Snapshot *snapshot=nullptr);
    ~TextureGenerator();
    QImage *takeResultTextureGuideImage();
    QImage *takeResultTextureImage();
    QImage *takeResultTextureBorderImage();
    QImage *takeResultTextureColorImage();
    QImage *takeResultTextureNormalImage();
    QImage *takeResultTextureMetalnessRoughnessAmbientOcclusionImage();
    QImage *takeResultTextureRoughnessImage();
    QImage *takeResultTextureMetalnessImage();
    QImage *takeResultTextureAmbientOcclusionImage();
    Outcome *takeOutcome();
    Model *takeResultMesh();
    bool hasTransparencySettings();
    void addPartColorMap(QUuid partId, const QImage *image, float tileScale);
    void addPartNormalMap(QUuid partId, const QImage *image, float tileScale);
    void addPartMetalnessMap(QUuid partId, const QImage *image, float tileScale);
    void addPartRoughnessMap(QUuid partId, const QImage *image, float tileScale);
    void addPartAmbientOcclusionMap(QUuid partId, const QImage *image, float tileScale);
    void generate();
signals:
    void finished();
public slots:
    void process();
public:
    static QColor m_defaultTextureColor;
private:
    void prepare();
private:
    Outcome *m_outcome;
    QImage *m_resultTextureGuideImage;
    QImage *m_resultTextureImage;
    QImage *m_resultTextureBorderImage;
    QImage *m_resultTextureColorImage;
    QImage *m_resultTextureNormalImage;
    QImage *m_resultTextureMetalnessRoughnessAmbientOcclusionImage;
    QImage *m_resultTextureRoughnessImage;
    QImage *m_resultTextureMetalnessImage;
    QImage *m_resultTextureAmbientOcclusionImage;
    Model *m_resultMesh;
    std::map<QUuid, std::pair<QImage, float>> m_partColorTextureMap;
    std::map<QUuid, std::pair<QImage, float>> m_partNormalTextureMap;
    std::map<QUuid, std::pair<QImage, float>> m_partMetalnessTextureMap;
    std::map<QUuid, std::pair<QImage, float>> m_partRoughnessTextureMap;
    std::map<QUuid, std::pair<QImage, float>> m_partAmbientOcclusionTextureMap;
    std::set<QUuid> m_countershadedPartIds;
    Snapshot *m_snapshot;
    bool m_hasTransparencySettings;
    int m_textureSize;
};

#endif
