#ifndef DUST3D_APPLICATION_MODEL_OFFSCREEN_RENDER_H_
#define DUST3D_APPLICATION_MODEL_OFFSCREEN_RENDER_H_

#include "model_mesh.h"
#include <QOffscreenSurface>
#include <QOpenGLFramebufferObject>
#include <QSurfaceFormat>
#include <QVector3D>
#include <memory>

class ModelOpenGLProgram;

class ModelOffscreenRender : public QOffscreenSurface {
public:
    ModelOffscreenRender(const QSurfaceFormat& format, QScreen* targetScreen = Q_NULLPTR);
    ~ModelOffscreenRender();
    void setXRotation(int angle);
    void setYRotation(int angle);
    void setZRotation(int angle);
    void setEyePosition(const QVector3D& eyePosition);
    void setMoveToPosition(const QVector3D& moveToPosition);
    void setRenderThread(QThread* thread);
    void updateMesh(ModelMesh* mesh);
    QImage toImage(const QSize& size);
    // Let go of the context and the shader program made for the images (call it on the thread
    // that rendered them, once the last is taken).
    void releaseResources();

private:
    int m_xRot = 0;
    int m_yRot = 0;
    int m_zRot = 0;
    QVector3D m_eyePosition;
    QVector3D m_moveToPosition;
    QOpenGLContext* m_context = nullptr;
    QOpenGLFramebufferObject* m_renderFbo = nullptr;
    // (made once and used for every image: a context, a framebuffer and a compiled shader for
    // each image of a few dozen parts took seconds)
    std::unique_ptr<ModelOpenGLProgram> m_program;
    ModelMesh* m_mesh = nullptr;
    bool prepare(const QSize& size);
};

#endif
