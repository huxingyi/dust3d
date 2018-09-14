#include <QOpenGLFramebufferObjectFormat>
#include <QThread>
#include <QDebug>
#include "modelofflinerender.h"

ModelOfflineRender::ModelOfflineRender(QOpenGLWidget *sharedContextWidget, QScreen *targetScreen) :
    QOffscreenSurface(targetScreen),
    m_context(nullptr),
    m_mesh(nullptr),
    m_xRot(0),
    m_yRot(0),
    m_zRot(0)
{
    create();
    
    QOpenGLContext *current = nullptr;
    
    if (nullptr != sharedContextWidget) {
        current = sharedContextWidget->context();
        current->doneCurrent();
    }
    
    m_context = new QOpenGLContext();
    
    if (nullptr != current) {
        m_context->setFormat(current->format());
        m_context->setShareContext(current);
    } else {
        QSurfaceFormat fmt = format();
        fmt.setAlphaBufferSize(8);
        fmt.setSamples(4);
        
        setFormat(fmt);
        
        m_context->setFormat(fmt);
    }
    m_context->create();
    
    if (nullptr != sharedContextWidget) {
        sharedContextWidget->makeCurrent();
    }
}

ModelOfflineRender::~ModelOfflineRender()
{
    delete m_context;
    m_context = nullptr;
    destroy();
    delete m_mesh;
}

void ModelOfflineRender::updateMesh(MeshLoader *mesh)
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
        int xRot = m_xRot;
        int yRot = m_yRot;
        int zRot = m_zRot;
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
        camera.translate(0, 0, -4.5);

        world.setToIdentity();
        world.rotate(xRot / 16.0f, 1, 0, 0);
        world.rotate(yRot / 16.0f, 0, 1, 0);
        world.rotate(zRot / 16.0f, 0, 0, 1);

        proj.setToIdentity();
        proj.perspective(45.0f, GLfloat(size.width()) / size.height(), 0.01f, 100.0f);

        program->bind();
        program->setUniformValue(program->projMatrixLoc(), proj);
        program->setUniformValue(program->mvMatrixLoc(), camera * world);
        QMatrix3x3 normalMatrix = world.normalMatrix();
        program->setUniformValue(program->normalMatrixLoc(), normalMatrix);
        program->setUniformValue(program->textureEnabledLoc(), 0);

        meshBinder.updateMesh(m_mesh);
        meshBinder.paint(program);

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

void ModelOfflineRender::setXRotation(int angle)
{
    m_xRot = angle;
}

void ModelOfflineRender::setYRotation(int angle)
{
    m_yRot = angle;
}

void ModelOfflineRender::setZRotation(int angle)
{
    m_zRot = angle;
}

