#ifndef DUST3D_MODEL_OFFSCREEN_RENDER_H
#define DUST3D_MODEL_OFFSCREEN_RENDER_H
#include <QOffscreenSurface>
#include <QScreen>
#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include <QImage>
#include <QThread>
#include "modelshaderprogram.h"
#include "modelmeshbinder.h"
#include "model.h"

class ModelOffscreenRender : QOffscreenSurface
{
public:
    ModelOffscreenRender(const QSurfaceFormat &format, QScreen *targetScreen = Q_NULLPTR);
    ~ModelOffscreenRender();
    void setXRotation(int angle);
    void setYRotation(int angle);
    void setZRotation(int angle);
    void setRenderPurpose(int purpose);
    void setRenderThread(QThread *thread);
    void updateMesh(Model *mesh);
    void updateToonNormalAndDepthMaps(QImage *normalMap, QImage *depthMap);
    void setToonShading(bool toonShading);
    QImage toImage(const QSize &size);
private:
    int m_xRot = 0;
    int m_yRot = 0;
    int m_zRot = 0;
    int m_renderPurpose = 0;
    QOpenGLContext *m_context = nullptr;
    Model *m_mesh = nullptr;
    QImage *m_normalMap = nullptr;
    QImage *m_depthMap = nullptr;
    bool m_toonShading = false;
};

#endif
