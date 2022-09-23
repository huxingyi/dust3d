#include "model_offscreen_render.h"

ModelOffscreenRender::ModelOffscreenRender(const QSurfaceFormat &format, QScreen *targetScreen):
    QOffscreenSurface(targetScreen)
{
}

ModelOffscreenRender::~ModelOffscreenRender()
{
    // TODO
}

void ModelOffscreenRender::setXRotation(int angle)
{
    // TODO
}

void ModelOffscreenRender::setYRotation(int angle)
{
    // TODO
}

void ModelOffscreenRender::setZRotation(int angle)
{
    // TODO
}

void ModelOffscreenRender::setEyePosition(const QVector3D &eyePosition)
{
    // TODO
}

void ModelOffscreenRender::setMoveToPosition(const QVector3D &moveToPosition)
{
    // TODO
}

void ModelOffscreenRender::setRenderThread(QThread *thread)
{
    // TODO
}

void ModelOffscreenRender::updateMesh(ModelMesh *mesh)
{
    // TODO
}

QImage ModelOffscreenRender::toImage(const QSize &size)
{
    // TODO
    return QImage();
}
