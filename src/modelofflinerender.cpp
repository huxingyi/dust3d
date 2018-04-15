#include <QOpenGLFramebufferObjectFormat>
#include <QThread>
#include <QDebug>
#include "modelofflinerender.h"

ModelOfflineRender::ModelOfflineRender(QScreen *targetScreen) :
    QOffscreenSurface(targetScreen),
    m_context(nullptr),
    m_mesh(nullptr)
{
    create();
    m_context = new QOpenGLContext();
    
    QSurfaceFormat fmt = format();
    fmt.setAlphaBufferSize(8);
    fmt.setSamples(4);
    
    setFormat(fmt);
    
    m_context->setFormat(fmt);
    m_context->create();
}

ModelOfflineRender::~ModelOfflineRender()
{
    delete m_context;
    m_context = nullptr;
    destroy();
    delete m_mesh;
}

void ModelOfflineRender::updateMesh(Mesh *mesh)
{
    delete m_mesh;
    m_mesh = mesh;
}

void ModelOfflineRender::setRenderThread(QThread *thread)
{
    m_context->moveToThread(thread);
}

QImage ModelOfflineRender::toImage(const QSize &size)
{
    QImage image;
    
    m_context->makeCurrent(this);
    
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(16);
    QOpenGLFramebufferObject *renderFbo = new QOpenGLFramebufferObject(size, format);
    renderFbo->bind();
    m_context->functions()->glViewport(0, 0, size.width(), size.height());
    
    if (nullptr != m_mesh) {
        int xRot = -30 * 16;
        int yRot = 45 * 16;
        int zRot = 0;
        QMatrix4x4 proj;
        QMatrix4x4 camera;
        QMatrix4x4 world;

        ModelShaderProgram *program = new ModelShaderProgram;
        ModelMeshBinder meshBinder;
        meshBinder.initialize();
        meshBinder.hideWireframes();

        program->setUniformValue(program->lightPosLoc(), QVector3D(0, 0, 70));
		
        m_context->functions()->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        m_context->functions()->glEnable(GL_DEPTH_TEST);
        m_context->functions()->glEnable(GL_CULL_FACE);
        m_context->functions()->glEnable(GL_LINE_SMOOTH);

        camera.setToIdentity();
        camera.translate(0, 0, -2.1);

        world.setToIdentity();
        world.rotate(180.0f - (xRot / 16.0f), 1, 0, 0);
        world.rotate(yRot / 16.0f, 0, 1, 0);
        world.rotate(zRot / 16.0f, 0, 0, 1);

        proj.setToIdentity();
        proj.perspective(45.0f, GLfloat(size.width()) / size.height(), 0.01f, 100.0f);

        program->bind();
        program->setUniformValue(program->projMatrixLoc(), proj);
        program->setUniformValue(program->mvMatrixLoc(), camera * world);
        QMatrix3x3 normalMatrix = world.normalMatrix();
        program->setUniformValue(program->normalMatrixLoc(), normalMatrix);

        meshBinder.updateMesh(m_mesh);
        meshBinder.paint();

        meshBinder.cleanup();

        program->release();
        delete program;

        m_mesh = nullptr;
    }

    m_context->functions()->glFlush();

    image = renderFbo->toImage();

    renderFbo->bindDefault();
    delete renderFbo;

    m_context->doneCurrent();

    return image;
}
