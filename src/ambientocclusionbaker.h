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
class AmbientOcclusionBaker : QOffscreenSurface, public QtLightMapper
#else
class AmbientOcclusionBaker : QOffscreenSurface
#endif
{
public:
	AmbientOcclusionBaker(QScreen *targetScreen = Q_NULLPTR);
    ~AmbientOcclusionBaker();
    void setRenderThread(QThread *thread);
    void setInputMesh(const MeshResultContext &meshResultContext);
    void setBakeSize(int width, int height);
    void bake();
    QImage *takeAmbientOcclusionImage();
private:
    QOpenGLContext *m_context;
    MeshResultContext m_meshResultContext;
    int m_bakeWidth;
    int m_bakeHeight;
    QImage *m_ambientOcclusionImage;
};

#endif
