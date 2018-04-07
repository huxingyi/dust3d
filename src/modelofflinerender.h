#ifndef MODEL_OFFLINE_RENDER_H
#define MODEL_OFFLINE_RENDER_H
#include <QOffscreenSurface>
#include <QScreen>
#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include <QImage>
#include <QThread>
#include "modelshaderprogram.h"
#include "modelmeshbinder.h"
#include "mesh.h"

class ModelOfflineRender : QOffscreenSurface
{
public:
	ModelOfflineRender(QScreen *targetScreen = Q_NULLPTR);
    ~ModelOfflineRender();
    void setRenderThread(QThread *thread);
    void updateMesh(Mesh *mesh);
    QImage toImage(const QSize &size);
private:
    QOpenGLContext *m_context;
    Mesh *m_mesh;
};

#endif
