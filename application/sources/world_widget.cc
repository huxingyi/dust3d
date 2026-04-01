#include "world_widget.h"
#include <QGuiApplication>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFunctions>
#include <QSurfaceFormat>
#include <QVector2D>
#include <cmath>

float WorldWidget::m_minZoomRatio = 5.0;
float WorldWidget::m_maxZoomRatio = 80.0;

int WorldWidget::m_defaultXRotation = 30 * 16;
int WorldWidget::m_defaultYRotation = -45 * 16;
int WorldWidget::m_defaultZRotation = 0;
QVector3D WorldWidget::m_defaultEyePosition = QVector3D(0, 0, -2.5);

WorldWidget::WorldWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    QSurfaceFormat fmt = format();
    fmt.setSamples(4);
    setFormat(fmt);

    setContextMenuPolicy(Qt::CustomContextMenu);

    m_widthInPixels = width() * window()->devicePixelRatio();
    m_heightInPixels = height() * window()->devicePixelRatio();

    zoom(200);
}

WorldWidget::~WorldWidget()
{
    cleanup();
}

const QVector3D& WorldWidget::eyePosition()
{
    return m_eyePosition;
}

const QVector3D& WorldWidget::moveToPosition()
{
    return m_moveToPosition;
}

int WorldWidget::xRot() { return m_xRot; }
int WorldWidget::yRot() { return m_yRot; }
int WorldWidget::zRot() { return m_zRot; }

void WorldWidget::setEyePosition(const QVector3D& eyePosition)
{
    m_eyePosition = eyePosition;
    emit eyePositionChanged(m_eyePosition);
    update();
}

void WorldWidget::setMoveToPosition(const QVector3D& moveToPosition)
{
    m_moveToPosition = moveToPosition;
}

void WorldWidget::reRender()
{
    emit renderParametersChanged();
    update();
}

void WorldWidget::setXRotation(int angle)
{
    normalizeAngle(angle);
    if (angle != m_xRot) {
        m_xRot = angle;
        emit xRotationChanged(angle);
        emit renderParametersChanged();
        update();
    }
}

void WorldWidget::setYRotation(int angle)
{
    normalizeAngle(angle);
    if (angle != m_yRot) {
        m_yRot = angle;
        emit yRotationChanged(angle);
        emit renderParametersChanged();
        update();
    }
}

void WorldWidget::setZRotation(int angle)
{
    normalizeAngle(angle);
    if (angle != m_zRot) {
        m_zRot = angle;
        emit zRotationChanged(angle);
        emit renderParametersChanged();
        update();
    }
}

void WorldWidget::cleanup()
{
    if (!m_shadowFBOId && !m_worldOpenGLProgram)
        return;

    // During widget teardown the GL context can already be gone.
    // Only issue GL deletes when we actually have a current context.
    const bool canUseGlContext = nullptr != context() && context()->isValid();
    if (canUseGlContext)
        makeCurrent();

    if (nullptr != QOpenGLContext::currentContext()) {
        cleanupShadowFBO();
    } else {
        m_shadowDepthTexture = 0;
        m_shadowFBOId = 0;
    }

    m_modelOpenGLObject.reset();
    m_shadowOpenGLProgram.reset();
    m_worldOpenGLProgram.reset();
    m_groundOpenGLProgram.reset();
    m_groundOpenGLObject.reset();
    m_wireframeOpenGLObject.reset();
    m_monochromeOpenGLProgram.reset();
    if (canUseGlContext && nullptr != QOpenGLContext::currentContext())
        doneCurrent();
}

void WorldWidget::cleanupShadowFBO()
{
    QOpenGLContext* currentContext = QOpenGLContext::currentContext();
    if (nullptr == currentContext)
        return;
    QOpenGLFunctions* f = currentContext->functions();
    if (m_shadowDepthTexture) {
        f->glDeleteTextures(1, &m_shadowDepthTexture);
        m_shadowDepthTexture = 0;
    }
    if (m_shadowFBOId) {
        f->glDeleteFramebuffers(1, &m_shadowFBOId);
        m_shadowFBOId = 0;
    }
}

void WorldWidget::updateProjectionMatrix()
{
    m_projection.setToIdentity();
    m_projection.translate(m_moveToPosition.x(), m_moveToPosition.y(), m_moveToPosition.z());
    m_projection.perspective(45.0f, GLfloat(width()) / height(), 0.01f, 100.0f);
}

void WorldWidget::normalizeAngle(int& angle)
{
    while (angle < 0)
        angle += 360 * 16;
    while (angle > 360 * 16)
        angle -= 360 * 16;
}

void WorldWidget::enableMove(bool enabled)
{
    m_moveEnabled = enabled;
}

void WorldWidget::enableZoom(bool enabled)
{
    m_zoomEnabled = enabled;
}

void WorldWidget::setMoveAndZoomByWindow(bool byWindow)
{
    m_moveAndZoomByWindow = byWindow;
}

void WorldWidget::updateMesh(ModelMesh* mesh)
{
    // Create the program early (before paintGL) so texture images can be stored.
    if (!m_worldOpenGLProgram)
        m_worldOpenGLProgram = std::make_unique<WorldOpenGLProgram>();

    m_worldOpenGLProgram->updateTextureImage(
        std::unique_ptr<QImage>(nullptr != mesh ? mesh->takeTextureImage() : nullptr));
    m_worldOpenGLProgram->updateNormalMapImage(
        std::unique_ptr<QImage>(nullptr != mesh ? mesh->takeNormalMapImage() : nullptr));
    m_worldOpenGLProgram->updateMetalnessRoughnessAmbientOcclusionMapImage(
        std::unique_ptr<QImage>(nullptr != mesh ? mesh->takeMetalnessRoughnessAmbientOcclusionMapImage() : nullptr),
        mesh && mesh->hasMetalnessInImage(),
        mesh && mesh->hasRoughnessInImage(),
        mesh && mesh->hasAmbientOcclusionInImage());

    if (!m_modelOpenGLObject)
        m_modelOpenGLObject = std::make_unique<ModelOpenGLObject>();
    m_modelOpenGLObject->update(std::unique_ptr<ModelMesh>(mesh));

    emit renderParametersChanged();
    update();
}

void WorldWidget::updateWireframeMesh(MonochromeMesh* mesh)
{
    if (!m_wireframeOpenGLObject)
        m_wireframeOpenGLObject = std::make_unique<MonochromeOpenGLObject>();
    m_wireframeOpenGLObject->update(std::unique_ptr<MonochromeMesh>(mesh));
    emit renderParametersChanged();
    update();
}

void WorldWidget::toggleWireframe()
{
    m_isWireframeVisible = !m_isWireframeVisible;
    update();
}

void WorldWidget::setWireframeVisible(bool visible)
{
    if (m_isWireframeVisible != visible) {
        m_isWireframeVisible = visible;
        update();
    }
}

bool WorldWidget::isWireframeVisible()
{
    return m_isWireframeVisible;
}

void WorldWidget::toggleRotation()
{
    if (nullptr != m_rotationTimer) {
        delete m_rotationTimer;
        m_rotationTimer = nullptr;
    } else {
        m_rotationTimer = new QTimer(this);
        m_rotationTimer->setInterval(42);
        m_rotationTimer->setSingleShot(false);
        connect(m_rotationTimer, &QTimer::timeout, this, [&]() {
            setYRotation(m_yRot - 8);
        });
        m_rotationTimer->start();
    }
}

void WorldWidget::canvasResized()
{
    resize(parentWidget()->size());
}

void WorldWidget::initializeShadowFBO()
{
    if (m_shadowFBOId)
        return;

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    QOpenGLExtraFunctions* ef = QOpenGLContext::currentContext()->extraFunctions();

    // Create depth texture
    f->glGenTextures(1, &m_shadowDepthTexture);
    f->glBindTexture(GL_TEXTURE_2D, m_shadowDepthTexture);
    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        m_shadowMapSize, m_shadowMapSize, 0,
        GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    f->glBindTexture(GL_TEXTURE_2D, 0);

    // Create depth-only FBO
    f->glGenFramebuffers(1, &m_shadowFBOId);
    f->glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFBOId);
    f->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D, m_shadowDepthTexture, 0);

    // No color buffer: signal this explicitly
    GLenum drawBufs[] = { GL_NONE };
    ef->glDrawBuffers(1, drawBufs);
    ef->glReadBuffer(GL_NONE);

    f->glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
}

void WorldWidget::initializeGL()
{
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &WorldWidget::cleanup);
    initializeShadowFBO();
}

void WorldWidget::resizeGL(int w, int h)
{
    m_widthInPixels = w * window()->devicePixelRatio();
    m_heightInPixels = h * window()->devicePixelRatio();
    updateProjectionMatrix();
    emit renderParametersChanged();
}

void WorldWidget::paintGL()
{
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    bool isCoreProfile = format().profile() == QSurfaceFormat::CoreProfile;

    // Lazily initialize programs
    if (!m_shadowOpenGLProgram) {
        m_shadowOpenGLProgram = std::make_unique<ShadowOpenGLProgram>();
        m_shadowOpenGLProgram->load(isCoreProfile);
    }
    if (!m_worldOpenGLProgram) {
        m_worldOpenGLProgram = std::make_unique<WorldOpenGLProgram>();
        m_worldOpenGLProgram->load(isCoreProfile);
    } else if (!m_worldOpenGLProgram->isLinked()) {
        m_worldOpenGLProgram->load(isCoreProfile);
    }
    if (!m_groundOpenGLProgram) {
        m_groundOpenGLProgram = std::make_unique<WorldGroundOpenGLProgram>();
        m_groundOpenGLProgram->load(isCoreProfile);
    }
    if (!m_groundOpenGLObject) {
        m_groundOpenGLObject = std::make_unique<WorldGroundOpenGLObject>();
        m_groundOpenGLObject->create(-1.0f, 5.0f);
    }

    // Model rotation and camera
    m_world.setToIdentity();
    m_world.rotate(m_xRot / 16.0f, 1, 0, 0);
    m_world.rotate(m_yRot / 16.0f, 0, 1, 0);
    m_world.rotate(m_zRot / 16.0f, 0, 0, 1);

    m_camera.setToIdentity();
    m_camera.translate(m_eyePosition.x(), m_eyePosition.y(), m_eyePosition.z());

    // Light space matrix — fixed directional light matching LIGHT_POS in shaders
    static const QVector3D s_lightDir = QVector3D(10.0f, 15.0f, 10.0f).normalized();
    static const float s_lightDist = 25.0f;
    QVector3D lightPos = s_lightDir * s_lightDist;
    m_lightView.setToIdentity();
    m_lightView.lookAt(lightPos, QVector3D(0, 0, 0), QVector3D(0, 1, 0));
    m_lightProjection.setToIdentity();
    m_lightProjection.ortho(-4.0f, 4.0f, -4.0f, 4.0f, 1.0f, 50.0f);
    m_lightSpaceMatrix = m_lightProjection * m_lightView;

    if (m_isWireframeVisible) {
        if (!m_monochromeOpenGLProgram) {
            m_monochromeOpenGLProgram = std::make_unique<MonochromeOpenGLProgram>();
            m_monochromeOpenGLProgram->load(isCoreProfile);
        }
    }

    // --- Pass 1: Shadow depth ---
    drawShadowPass();

    // --- Pass 2: Scene ---
    f->glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
    f->glViewport(0, 0, m_widthInPixels, m_heightInPixels);
    f->glClearColor(0.18f, 0.18f, 0.20f, 1.0f);
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    f->glEnable(GL_BLEND);
    f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    f->glEnable(GL_DEPTH_TEST);
    f->glEnable(GL_POLYGON_OFFSET_FILL);
    f->glPolygonOffset(1.0f, 1.0f);

    f->glEnable(GL_CULL_FACE);
    f->glCullFace(GL_BACK);
    drawWorldModel();

    f->glDisable(GL_CULL_FACE);
    drawGround();

    if (m_isWireframeVisible)
        drawWireframe();

    f->glDisable(GL_POLYGON_OFFSET_FILL);
}

void WorldWidget::drawShadowPass()
{
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();

    f->glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFBOId);
    f->glViewport(0, 0, m_shadowMapSize, m_shadowMapSize);
    f->glClear(GL_DEPTH_BUFFER_BIT);
    f->glEnable(GL_DEPTH_TEST);

    // Cull front faces to reduce self-shadowing (peter-panning fix)
    f->glEnable(GL_CULL_FACE);
    f->glCullFace(GL_FRONT);

    m_shadowOpenGLProgram->bind();
    m_shadowOpenGLProgram->setUniformValue(
        m_shadowOpenGLProgram->getUniformLocationByName("lightSpaceMatrix"), m_lightSpaceMatrix);
    m_shadowOpenGLProgram->setUniformValue(
        m_shadowOpenGLProgram->getUniformLocationByName("modelMatrix"), m_world);

    if (m_modelOpenGLObject)
        m_modelOpenGLObject->draw();

    m_shadowOpenGLProgram->release();
    f->glCullFace(GL_BACK);
}

void WorldWidget::drawWorldModel()
{
    m_worldOpenGLProgram->bind();

    m_worldOpenGLProgram->setUniformValue(
        m_worldOpenGLProgram->getUniformLocationByName("eyePosition"), m_eyePosition);
    m_worldOpenGLProgram->setUniformValue(
        m_worldOpenGLProgram->getUniformLocationByName("projectionMatrix"), m_projection);
    m_worldOpenGLProgram->setUniformValue(
        m_worldOpenGLProgram->getUniformLocationByName("modelMatrix"), m_world);
    m_worldOpenGLProgram->setUniformValue(
        m_worldOpenGLProgram->getUniformLocationByName("normalMatrix"), m_world.normalMatrix());
    m_worldOpenGLProgram->setUniformValue(
        m_worldOpenGLProgram->getUniformLocationByName("viewMatrix"), m_camera);
    m_worldOpenGLProgram->setUniformValue(
        m_worldOpenGLProgram->getUniformLocationByName("lightSpaceMatrix"), m_lightSpaceMatrix);

    m_worldOpenGLProgram->bindMaps(m_shadowDepthTexture);

    if (m_modelOpenGLObject)
        m_modelOpenGLObject->draw();

    m_worldOpenGLProgram->releaseMaps();
    m_worldOpenGLProgram->release();
}

void WorldWidget::drawGround()
{
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();

    m_groundOpenGLProgram->bind();

    m_groundOpenGLProgram->setUniformValue(
        m_groundOpenGLProgram->getUniformLocationByName("projectionMatrix"), m_projection);
    m_groundOpenGLProgram->setUniformValue(
        m_groundOpenGLProgram->getUniformLocationByName("viewMatrix"), m_camera);
    m_groundOpenGLProgram->setUniformValue(
        m_groundOpenGLProgram->getUniformLocationByName("lightSpaceMatrix"), m_lightSpaceMatrix);

    // Bind shadow depth texture at unit 1
    f->glActiveTexture(GL_TEXTURE1);
    f->glBindTexture(GL_TEXTURE_2D, m_shadowDepthTexture);
    m_groundOpenGLProgram->setUniformValue(
        m_groundOpenGLProgram->getUniformLocationByName("shadowMap"), 1);

    if (m_groundOpenGLObject)
        m_groundOpenGLObject->draw();

    // Unbind shadow texture
    f->glActiveTexture(GL_TEXTURE1);
    f->glBindTexture(GL_TEXTURE_2D, 0);
    f->glActiveTexture(GL_TEXTURE0);

    m_groundOpenGLProgram->release();
}

void WorldWidget::drawWireframe()
{
    m_monochromeOpenGLProgram->bind();

    m_monochromeOpenGLProgram->setUniformValue(m_monochromeOpenGLProgram->getUniformLocationByName("projectionMatrix"), m_projection);
    m_monochromeOpenGLProgram->setUniformValue(m_monochromeOpenGLProgram->getUniformLocationByName("modelMatrix"), m_world);
    m_monochromeOpenGLProgram->setUniformValue(m_monochromeOpenGLProgram->getUniformLocationByName("viewMatrix"), m_camera);

    if (m_monochromeOpenGLProgram->isCoreProfile()) {
        m_monochromeOpenGLProgram->setUniformValue("viewportSize", QVector2D(m_widthInPixels, m_heightInPixels));
    }

    if (m_wireframeOpenGLObject)
        m_wireframeOpenGLObject->draw();

    m_monochromeOpenGLProgram->release();
}

void WorldWidget::zoom(float delta)
{
    if (m_moveAndZoomByWindow) {
        QMargins margins(delta, delta, delta, delta);
        if (0 == m_modelInitialHeight) {
            m_modelInitialHeight = height();
        } else {
            float ratio = (float)height() / m_modelInitialHeight;
            if (ratio <= m_minZoomRatio) {
                if (delta < 0)
                    return;
            } else if (ratio >= m_maxZoomRatio) {
                if (delta > 0)
                    return;
            }
        }
        setGeometry(geometry().marginsAdded(margins));
        emit renderParametersChanged();
        update();
    } else {
        m_eyePosition += QVector3D(0, 0, m_eyePosition.z() * (delta > 0 ? -0.1 : 0.1));
        if (m_eyePosition.z() < -15)
            m_eyePosition.setZ(-15);
        else if (m_eyePosition.z() > -0.1)
            m_eyePosition.setZ(-0.1f);
        emit eyePositionChanged(m_eyePosition);
        emit renderParametersChanged();
        update();
    }
}

void WorldWidget::mousePressEvent(QMouseEvent* event)
{
    bool shouldStartMove = false;
    if (event->button() == Qt::LeftButton) {
        if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::AltModifier) && !QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier))
            shouldStartMove = m_moveEnabled;
    } else if (event->button() == Qt::MiddleButton) {
        shouldStartMove = m_moveEnabled;
    }
    if (shouldStartMove) {
        m_lastPos = event->pos();
        if (!m_moveStarted) {
            m_moveStartPos = mapToParent(event->pos());
            m_moveStartGeometry = geometry();
            m_moveStarted = true;
            m_directionOnMoveStart = abs(m_xRot) > 180 * 8 ? -1 : 1;
        }
    }
}

void WorldWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_moveStarted)
        return;

    QPoint pos = event->pos();
    int dx = pos.x() - m_lastPos.x();
    int dy = pos.y() - m_lastPos.y();

    if ((event->buttons() & Qt::MiddleButton) || (m_moveStarted && (event->buttons() & Qt::LeftButton))) {
        if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier)) {
            if (m_moveAndZoomByWindow) {
                QPoint posInParent = mapToParent(pos);
                QRect rect = m_moveStartGeometry;
                rect.translate(posInParent.x() - m_moveStartPos.x(), posInParent.y() - m_moveStartPos.y());
                setGeometry(rect);
            } else {
                m_moveToPosition.setX(m_moveToPosition.x() + (float)2 * dx / width());
                m_moveToPosition.setY(m_moveToPosition.y() + (float)2 * -dy / height());
                if (m_moveToPosition.x() < -1.0)
                    m_moveToPosition.setX(-1.0);
                if (m_moveToPosition.x() > 1.0)
                    m_moveToPosition.setX(1.0);
                if (m_moveToPosition.y() < -1.0)
                    m_moveToPosition.setY(-1.0);
                if (m_moveToPosition.y() > 1.0)
                    m_moveToPosition.setY(1.0);
                updateProjectionMatrix();
                emit moveToPositionChanged(m_moveToPosition);
                emit renderParametersChanged();
                update();
            }
        } else {
            setXRotation(m_xRot + 8 * dy);
            setYRotation(m_yRot + 8 * dx * m_directionOnMoveStart);
        }
    }
    m_lastPos = pos;
}

void WorldWidget::wheelEvent(QWheelEvent* event)
{
    if (m_moveStarted)
        return;
    if (!m_zoomEnabled)
        return;

    qreal delta = geometry().height() * 0.1f;
    if (event->pixelDelta().y() < 0)
        delta = -delta;
    zoom(delta);
}

void WorldWidget::mouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    if (m_moveStarted)
        m_moveStarted = false;
}
