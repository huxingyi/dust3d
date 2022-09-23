#ifndef DUST3D_APPLICATION_MODEL_OFFSCREEN_RENDER_H_
#define DUST3D_APPLICATION_MODEL_OFFSCREEN_RENDER_H_

#include <QOffscreenSurface>
#include <QSurfaceFormat>
#include <QVector3D>
#include "model_mesh.h"

class ModelOffscreenRender: public QOffscreenSurface
{
public:
    ModelOffscreenRender(const QSurfaceFormat &format, QScreen *targetScreen=Q_NULLPTR);
    ~ModelOffscreenRender();
    void setXRotation(int angle);
    void setYRotation(int angle);
    void setZRotation(int angle);
    void setEyePosition(const QVector3D &eyePosition);
    void setMoveToPosition(const QVector3D &moveToPosition);
    void setRenderThread(QThread *thread);
    void updateMesh(ModelMesh *mesh);
    QImage toImage(const QSize &size);
};

#endif
