#ifndef TEXTURE_GENERATOR_H
#define TEXTURE_GENERATOR_H
#include <QObject>
#include <vector>
#include <QImage>
#include "meshresultcontext.h"
#include "meshloader.h"

class TextureGenerator : public QObject
{
    Q_OBJECT
public:
    TextureGenerator(const MeshResultContext &meshResultContext, SkeletonSnapshot *snapshot=nullptr);
    ~TextureGenerator();
    QImage *takeResultTextureGuideImage();
    QImage *takeResultTextureImage();
    QImage *takeResultTextureBorderImage();
    QImage *takeResultTextureColorImage();
    QImage *takeResultTextureNormalImage();
    MeshResultContext *takeResultContext();
    MeshLoader *takeResultMesh();
    void addPartColorMap(QUuid partId, const QImage *image);
    void addPartNormalMap(QUuid partId, const QImage *image);
    void addPartMetalnessMap(QUuid partId, const QImage *image);
    void addPartRoughnessMap(QUuid partId, const QImage *image);
    void addPartAmbientOcclusionMap(QUuid partId, const QImage *image);
    void generate();
signals:
    void finished();
public slots:
    void process();
public:
    static int m_textureSize;
private:
    void prepare();
    QPainterPath expandedPainterPath(const QPainterPath &painterPath);
private:
    MeshResultContext *m_resultContext;
    QImage *m_resultTextureGuideImage;
    QImage *m_resultTextureImage;
    QImage *m_resultTextureBorderImage;
    QImage *m_resultTextureColorImage;
    QImage *m_resultTextureNormalImage;
    QImage *m_resultTextureMetalnessRoughnessAmbientOcclusionImage;
    QImage *m_resultTextureRoughnessImage;
    QImage *m_resultTextureMetalnessImage;
    QImage *m_resultTextureAmbientOcclusionImage;
    MeshLoader *m_resultMesh;
    std::map<QUuid, QImage> m_partColorTextureMap;
    std::map<QUuid, QImage> m_partNormalTextureMap;
    std::map<QUuid, QImage> m_partMetalnessTextureMap;
    std::map<QUuid, QImage> m_partRoughnessTextureMap;
    std::map<QUuid, QImage> m_partAmbientOcclusionTextureMap;
    SkeletonSnapshot *m_snapshot;
};

#endif
