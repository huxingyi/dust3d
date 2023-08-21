#include "model_offscreen_render.h"
#include "model_opengl_object.h"
#include "model_opengl_program.h"

ModelOffscreenRender::ModelOffscreenRender(const QSurfaceFormat& format, QScreen* targetScreen)
    : QOffscreenSurface(targetScreen)
    , m_context(nullptr)
    , m_mesh(nullptr)
{
    setFormat(format);
    create();
}

ModelOffscreenRender::~ModelOffscreenRender()
{
    // FIXME: If delete m_renderFbo inside toImage,
    // sometimes, the application will freeze, maybe there are dead locks inside the destruction call
    // move it here can make sure it will be deleted on the main GUI thread to avoid dead locks
    delete m_renderFbo;

    destroy();
    delete m_mesh;
}

void ModelOffscreenRender::setXRotation(int angle)
{
    m_xRot = angle;
}

void ModelOffscreenRender::setYRotation(int angle)
{
    m_yRot = angle;
}

void ModelOffscreenRender::setZRotation(int angle)
{
    m_zRot = angle;
}

void ModelOffscreenRender::setEyePosition(const QVector3D& eyePosition)
{
    m_eyePosition = eyePosition;
}

void ModelOffscreenRender::setMoveToPosition(const QVector3D& moveToPosition)
{
    m_moveToPosition = moveToPosition;
}

void ModelOffscreenRender::setRenderThread(QThread* thread)
{
    moveToThread(thread);
}

void ModelOffscreenRender::updateMesh(ModelMesh* mesh)
{
    delete m_mesh;
    m_mesh = mesh;
}

QImage ModelOffscreenRender::toImage(const QSize& size)
{
    QImage image;

    if (nullptr == m_mesh)
        return image;

    QMatrix4x4 projection;
    QMatrix4x4 world;
    QMatrix4x4 camera;

    world.setToIdentity();
    world.rotate(m_xRot / 16.0f, 1, 0, 0);
    world.rotate(m_yRot / 16.0f, 0, 1, 0);
    world.rotate(m_zRot / 16.0f, 0, 0, 1);

    projection.setToIdentity();
    projection.translate(m_moveToPosition.x(), m_moveToPosition.y(), m_moveToPosition.z());
    projection.perspective(45.0f, GLfloat(size.width()) / size.height(), 0.01f, 100.0f);

    camera.setToIdentity();
    camera.translate(m_eyePosition);

    m_context = new QOpenGLContext();
    m_context->setFormat(format());
    if (!m_context->create()) {
        delete m_context;
        m_context = nullptr;
        qDebug() << "QOpenGLContext create failed";
        return image;
    }

    if (!m_context->makeCurrent(this)) {
        delete m_context;
        m_context = nullptr;
        qDebug() << "QOpenGLContext makeCurrent failed";
        return image;
    }

    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(4);
    format.setTextureTarget(GL_TEXTURE_2D);
    m_renderFbo = new QOpenGLFramebufferObject(size, format);
    m_renderFbo->bind();

    QOpenGLFunctions* f = m_context->functions();
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    f->glEnable(GL_BLEND);
    f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    f->glEnable(GL_DEPTH_TEST);
    f->glEnable(GL_CULL_FACE);
    f->glViewport(0, 0, size.width(), size.height());

    auto colorTextureImage = std::unique_ptr<QImage>(nullptr != m_mesh ? m_mesh->takeTextureImage() : nullptr);

    auto object = std::make_unique<ModelOpenGLObject>();
    object->update(std::unique_ptr<ModelMesh>(m_mesh));
    m_mesh = nullptr;

    auto program = std::make_unique<ModelOpenGLProgram>();
    program->load(m_context->format().profile() == QSurfaceFormat::CoreProfile);
    if (nullptr != colorTextureImage)
        program->updateTextureImage(std::move(colorTextureImage));
    program->bind();
    program->bindMaps();

    program->setUniformValue(program->getUniformLocationByName("eyePosition"), m_eyePosition);
    program->setUniformValue(program->getUniformLocationByName("projectionMatrix"), projection);
    program->setUniformValue(program->getUniformLocationByName("modelMatrix"), world);
    program->setUniformValue(program->getUniformLocationByName("normalMatrix"), world.normalMatrix());
    program->setUniformValue(program->getUniformLocationByName("viewMatrix"), camera);

    object->draw();

    program->releaseMaps();

    program->release();

    f->glFlush();

    image = m_renderFbo->toImage();

    program.reset();
    object.reset();

    m_renderFbo->release();

    m_context->doneCurrent();
    delete m_context;
    m_context = nullptr;

    return image;
}
