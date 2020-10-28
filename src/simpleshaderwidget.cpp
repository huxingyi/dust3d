// This file follow the Stackoverflow content license: CC BY-SA 4.0,
// since it's based on Prashanth N Udupa's work: https://stackoverflow.com/questions/35134270/how-to-use-qopenglframebufferobject-for-shadow-mapping
#include <QOpenGLTexture>
#include <QOpenGLFramebufferObject>
#include <QGuiApplication>
#include "simpleshaderwidget.h"

static const int SHADOW_WIDTH = 2048;
static const int SHADOW_HEIGHT = 2048;

SimpleShaderWidget::SimpleShaderWidget(QWidget *parent) : 
    QOpenGLWidget(parent)
{
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setProfile(QSurfaceFormat::OpenGLContextProfile::CompatibilityProfile);
    format.setVersion(1, 1);
    setFormat(format);
}

SimpleShaderWidget::~SimpleShaderWidget()
{
    if (m_shadowMapInitialized) {
        glDeleteTextures(1, &m_shadowMapTextureId);
        glDeleteFramebuffers(1, &m_shadowMapFrameBufferId);
    }
}

void SimpleShaderWidget::updateMesh(SimpleShaderMesh *mesh)
{
    m_meshBinder.updateMesh(mesh);
    update();
}

void SimpleShaderWidget::resizeEvent(QResizeEvent *event)
{
    QOpenGLWidget::resizeEvent(event);
}

void SimpleShaderWidget::resizeGL(int w, int h)
{
    m_cameraPositionMatrix.setToIdentity();
    m_cameraPositionMatrix.rotate(20, 0, 1, 0);
    m_cameraPositionMatrix.rotate(-25, 1, 0, 0);

    m_lightPositionMatrix = m_cameraPositionMatrix;
    m_lightPositionMatrix.rotate(45, 0, 1, 0);

    m_lightPositionMatrix.translate(0, 0, 50.0f);

    m_projectionMatrix.setToIdentity();
    m_projectionMatrix.perspective(45.0, float(w)/float(h), 0.1f, 1000.0f);
}

void SimpleShaderWidget::initializeGL()
{
    QOpenGLFunctions::initializeOpenGLFunctions();
    
    constexpr float color = (float)0x25 / 255;
    glClearColor(color, color, color, 1.0f);

    glEnable(GL_DEPTH_TEST);

    glDepthFunc(GL_LEQUAL);
    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(-0.03125f, -0.03125f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void SimpleShaderWidget::paintGL()
{
    renderToShadowMap();
    renderToScreen();
}

void SimpleShaderWidget::renderToShadowMap()
{
    // Refer http://learnopengl.com/#!Advanced-Lighting/Shadows/Shadow-Mapping
    if (!m_shadowMapInitialized) {
        // Create a texture for storing the depth map
        glGenTextures(1, &m_shadowMapTextureId);
        glBindTexture(GL_TEXTURE_2D, m_shadowMapTextureId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
            SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        GLfloat borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        // Create a frame-buffer and associate the texture with it.
        glGenFramebuffers(1, &m_shadowMapFrameBufferId);
        glBindFramebuffer(GL_FRAMEBUFFER, m_shadowMapFrameBufferId);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_shadowMapTextureId, 0);

        // Let OpenGL know that we are not interested in colors for this buffer
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        // Cleanup for now.
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        
        m_shadowMapInitialized = true;
    }

    // Render into the depth framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_shadowMapFrameBufferId);
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glClear(GL_DEPTH_BUFFER_BIT);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    m_lightViewMatrix.setToIdentity();
    m_lightViewMatrix.lookAt(m_lightPositionMatrix.map(QVector3D(0, 0, 0)),
        QVector3D(), 
        m_lightPositionMatrix.map(QVector3D(0, 1, 0)).normalized());

    m_meshBinder.setShadowMapTextureId(0);
    m_meshBinder.renderShadow(m_projectionMatrix, m_lightViewMatrix);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SimpleShaderWidget::renderToScreen()
{
    const int pixelRatio = devicePixelRatio();
    const int w = width() * pixelRatio;
    const int h = height() * pixelRatio;
    
    glViewport(0, 0, w, h);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    const QVector3D center = QVector3D();
    const QVector3D eye(0, 0, 1.0);
    const QVector3D lightDirection = m_lightPositionMatrix.map(QVector3D(0, 0, -1)).normalized();
    
    m_sceneMatrix.setToIdentity();
    m_sceneMatrix.rotate(m_rotationX / 16.0f, 1, 0, 0);
    m_sceneMatrix.rotate(m_rotationY / 16.0f, 0, 1, 0);
    
    auto zoomedMatrix = m_cameraPositionMatrix;
    zoomedMatrix.translate(0, 0, m_zoom);
    
    m_viewMatrix.setToIdentity();
    m_viewMatrix.lookAt(zoomedMatrix.map(QVector3D(0, 0, 0)),
        QVector3D(), zoomedMatrix.map(QVector3D(0, 1, 0)).normalized());

    m_meshBinder.setShadowMapTextureId(m_shadowMapTextureId);
    m_meshBinder.setSceneMatrix(m_sceneMatrix);
    m_meshBinder.renderScene(eye, lightDirection, m_projectionMatrix, m_viewMatrix, m_lightViewMatrix);
}

void SimpleShaderWidget::mousePressEvent(QMouseEvent *event)
{
    if ((event->button() == Qt::LeftButton && QGuiApplication::queryKeyboardModifiers().testFlag(Qt::AltModifier)) ||
            event->button() == Qt::MidButton) {
        m_lastPos = mapFromGlobal(event->globalPos());
        if (!m_moveStarted) {
            m_moveStarted = true;
        }
    }
}

void SimpleShaderWidget::setRotationX(int angle)
{
    normalizeAngle(angle);
    if (angle != m_rotationX) {
        m_rotationX = angle;
        update();
    }
}

void SimpleShaderWidget::setRotationY(int angle)
{
    normalizeAngle(angle);
    if (angle != m_rotationY) {
        m_rotationY = angle;
        update();
    }
}

void SimpleShaderWidget::normalizeAngle(int &angle)
{
    while (angle < 0)
        angle += 360 * 16;
    while (angle > 360 * 16)
        angle -= 360 * 16;
}

void SimpleShaderWidget::mouseMoveEvent(QMouseEvent *event)
{
    QPoint pos = mapFromGlobal(event->globalPos());
    
    if (m_moveStarted) {
        int dx = pos.x() - m_lastPos.x();
        int dy = pos.y() - m_lastPos.y();
    
        setRotationX(m_rotationX + 8 * dy);
        setRotationY(m_rotationY + 8 * dx);
    }
    
    m_lastPos = pos;
}

void SimpleShaderWidget::wheelEvent(QWheelEvent *event)
{
    qreal delta = geometry().height() * 0.1f;
    if (event->delta() < 0)
        delta = -delta;
    zoom(delta);
}

void SimpleShaderWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_moveStarted)
        m_moveStarted = false;
}

void SimpleShaderWidget::zoom(float delta)
{
    m_zoom += m_zoom * (delta > 0 ? -0.1 : 0.1);
    update();
}

