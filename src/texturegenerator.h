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
    TextureGenerator(const MeshResultContext &meshResultContext, QThread *thread);
    ~TextureGenerator();
    QImage *takeResultTextureGuideImage();
    QImage *takeResultTextureImage();
    QImage *takeResultTextureBorderImage();
    QImage *takeResultTextureColorImage();
    MeshResultContext *takeResultContext();
    MeshLoader *takeResultMesh();
signals:
    void finished();
public slots:
    void process();
public:
    static int m_textureWidth;
    static int m_textureHeight;
private:
    MeshResultContext *m_resultContext;
    QImage *m_resultTextureGuideImage;
    QImage *m_resultTextureImage;
    QImage *m_resultTextureBorderImage;
    QImage *m_resultTextureColorImage;
    QThread *m_thread;
    MeshLoader *m_resultMesh;
};

#endif
