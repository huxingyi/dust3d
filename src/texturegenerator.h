#ifndef TEXTURE_GENERATOR_H
#define TEXTURE_GENERATOR_H
#include <QObject>
#include <vector>
#include <QImage>
#include "meshresultcontext.h"

class TextureGenerator : public QObject
{
    Q_OBJECT
public:
    TextureGenerator(const MeshResultContext &meshResultContext);
    ~TextureGenerator();
    QImage *takeResultTextureGuideImage();
    QImage *takeResultTextureImage();
    QImage *takeResultTextureBorderImage();
    MeshResultContext *takeResultContext();
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
};

#endif
