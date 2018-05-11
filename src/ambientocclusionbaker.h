#ifndef AMBIENT_OCCLUSION_BAKER_H
#define AMBIENT_OCCLUSION_BAKER_H
#include <QOffscreenSurface>
#include <QScreen>
#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include <QImage>
#include <QThread>
#include "qtlightmapper.h"
#include "meshresultcontext.h"
#include "meshloader.h"

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
class AmbientOcclusionBaker : public QOffscreenSurface, public QtLightMapper
#else
class AmbientOcclusionBaker : public QOffscreenSurface
#endif
{
    Q_OBJECT
signals:
    void finished();
public slots:
    void process();
public:
	AmbientOcclusionBaker(QScreen *targetScreen = Q_NULLPTR);
    ~AmbientOcclusionBaker();
    void setInputMesh(const MeshResultContext &meshResultContext);
    void setBakeSize(int width, int height);
    void setColorImage(const QImage &colorImage);
    void setBorderImage(const QImage &borderImage);
    void setImageUpdateVersion(unsigned long long version);
    unsigned long long getImageUpdateVersion();
    void bake();
    QImage *takeAmbientOcclusionImage();
    QImage *takeTextureImage();
    QImage *takeGuideImage();
    MeshLoader *takeResultMesh();
    void setRenderThread(QThread *thread);
private:
    QOpenGLContext *m_context;
    MeshResultContext m_meshResultContext;
    int m_bakeWidth;
    int m_bakeHeight;
    QImage *m_ambientOcclusionImage;
    QImage *m_colorImage;
    QImage *m_textureImage;
    QImage *m_borderImage;
    QImage *m_guideImage;
    unsigned long long m_imageUpdateVersion;
    MeshLoader *m_resultMesh;
};

#endif
