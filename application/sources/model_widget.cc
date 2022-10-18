#include "model_widget.h"
#include "dds_file.h"
#include <QCoreApplication>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QSurfaceFormat>
#include <QVector4D>
#include <cmath>

float ModelWidget::m_minZoomRatio = 5.0;
float ModelWidget::m_maxZoomRatio = 80.0;

int ModelWidget::m_defaultXRotation = 30 * 16;
int ModelWidget::m_defaultYRotation = -45 * 16;
int ModelWidget::m_defaultZRotation = 0;
QVector3D ModelWidget::m_defaultEyePosition = QVector3D(0, 0, -2.5);
QString ModelWidget::m_openGLVersion = "";
QString ModelWidget::m_openGLShadingLanguageVersion = "";
bool ModelWidget::m_openGLIsCoreProfile = false;

ModelWidget::ModelWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setAttribute(Qt::WA_AlwaysStackOnTop);
    setAttribute(Qt::WA_TranslucentBackground);

    QSurfaceFormat fmt = format();
    fmt.setAlphaBufferSize(8);
    fmt.setSamples(4);
    setFormat(fmt);

    setContextMenuPolicy(Qt::CustomContextMenu);

    m_widthInPixels = width() * window()->devicePixelRatio();
    m_heightInPixels = height() * window()->devicePixelRatio();

    zoom(200);
}

const QVector3D& ModelWidget::eyePosition()
{
    return m_eyePosition;
}

const QVector3D& ModelWidget::moveToPosition()
{
    return m_moveToPosition;
}

void ModelWidget::setEyePosition(const QVector3D& eyePosition)
{
    m_eyePosition = eyePosition;
    emit eyePositionChanged(m_eyePosition);
    update();
}

void ModelWidget::reRender()
{
    emit renderParametersChanged();
    update();
}

int ModelWidget::xRot()
{
    return m_xRot;
}

int ModelWidget::yRot()
{
    return m_yRot;
}

int ModelWidget::zRot()
{
    return m_zRot;
}

ModelWidget::~ModelWidget()
{
    cleanup();
}

void ModelWidget::setXRotation(int angle)
{
    normalizeAngle(angle);
    if (angle != m_xRot) {
        m_xRot = angle;
        emit xRotationChanged(angle);
        emit renderParametersChanged();
        update();
    }
}

void ModelWidget::setYRotation(int angle)
{
    normalizeAngle(angle);
    if (angle != m_yRot) {
        m_yRot = angle;
        emit yRotationChanged(angle);
        emit renderParametersChanged();
        update();
    }
}

void ModelWidget::setZRotation(int angle)
{
    normalizeAngle(angle);
    if (angle != m_zRot) {
        m_zRot = angle;
        emit zRotationChanged(angle);
        emit renderParametersChanged();
        update();
    }
}

void ModelWidget::cleanup()
{
    if (!m_modelOpenGLProgram)
        return;
    makeCurrent();
    m_modelOpenGLObject.reset();
    m_modelOpenGLProgram.reset();
    m_wireframeOpenGLObject.reset();
    m_monochromeOpenGLProgram.reset();
    doneCurrent();
}

void ModelWidget::disableCullFace()
{
    m_enableCullFace = false;
}

void ModelWidget::setMoveToPosition(const QVector3D& moveToPosition)
{
    m_moveToPosition = moveToPosition;
}

void ModelWidget::updateProjectionMatrix()
{
    m_projection.setToIdentity();
    m_projection.translate(m_moveToPosition.x(), m_moveToPosition.y(), m_moveToPosition.z());
    m_projection.perspective(45.0f, GLfloat(width()) / height(), 0.01f, 100.0f);
}

void ModelWidget::resizeGL(int w, int h)
{
    m_widthInPixels = w * window()->devicePixelRatio();
    m_heightInPixels = h * window()->devicePixelRatio();
    updateProjectionMatrix();
    emit renderParametersChanged();
}

std::pair<QVector3D, QVector3D> ModelWidget::screenPositionToMouseRay(const QPoint& screenPosition)
{
    auto modelView = m_camera * m_world;
    float x = qMax(qMin(screenPosition.x(), width() - 1), 0);
    float y = qMax(qMin(screenPosition.y(), height() - 1), 0);
    QVector3D nearScreen = QVector3D(x, height() - y, 0.0);
    QVector3D farScreen = QVector3D(x, height() - y, 1.0);
    auto viewPort = QRect(0, 0, width(), height());
    auto nearPosition = nearScreen.unproject(modelView, m_projection, viewPort);
    auto farPosition = farScreen.unproject(modelView, m_projection, viewPort);
    return std::make_pair(nearPosition, farPosition);
}

void ModelWidget::toggleWireframe()
{
    m_isWireframeVisible = !m_isWireframeVisible;
    update();
}

bool ModelWidget::isWireframeVisible()
{
    return m_isWireframeVisible;
}

void ModelWidget::toggleRotation()
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

bool ModelWidget::inputMousePressEventFromOtherWidget(QMouseEvent* event, bool notGraphics)
{
    bool shouldStartMove = false;
    if (event->button() == Qt::LeftButton) {
        if ((notGraphics || QGuiApplication::queryKeyboardModifiers().testFlag(Qt::AltModifier)) && !QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier)) {
            shouldStartMove = m_moveEnabled;
        }
        if (!shouldStartMove && !m_mousePickTargetPositionInModelSpace.isNull())
            emit mousePressed();
    } else if (event->button() == Qt::MidButton) {
        shouldStartMove = m_moveEnabled;
    }
    if (shouldStartMove) {
        m_lastPos = convertInputPosFromOtherWidget(event);
        if (!m_moveStarted) {
            m_moveStartPos = mapToParent(convertInputPosFromOtherWidget(event));
            m_moveStartGeometry = geometry();
            m_moveStarted = true;
            m_directionOnMoveStart = abs(m_xRot) > 180 * 8 ? -1 : 1;
        }
        return true;
    }
    return false;
}

bool ModelWidget::inputMouseReleaseEventFromOtherWidget(QMouseEvent* event)
{
    Q_UNUSED(event);
    if (m_moveStarted) {
        m_moveStarted = false;
        return true;
    }
    if (event->button() == Qt::LeftButton) {
        if (m_mousePickingEnabled)
            emit mouseReleased();
    }
    return false;
}

void ModelWidget::canvasResized()
{
    resize(parentWidget()->size());
}

bool ModelWidget::inputMouseMoveEventFromOtherWidget(QMouseEvent* event)
{
    QPoint pos = convertInputPosFromOtherWidget(event);

    if (m_mousePickingEnabled) {
        auto segment = screenPositionToMouseRay(pos);
        emit mouseRayChanged(segment.first, segment.second);
    }

    if (!m_moveStarted) {
        return false;
    }

    int dx = pos.x() - m_lastPos.x();
    int dy = pos.y() - m_lastPos.y();

    if ((event->buttons() & Qt::MidButton) || (m_moveStarted && (event->buttons() & Qt::LeftButton))) {
        if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier)) {
            if (m_moveStarted) {
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
            }
        } else {
            setXRotation(m_xRot + 8 * dy);
            setYRotation(m_yRot + 8 * dx * m_directionOnMoveStart);
        }
    }
    m_lastPos = pos;

    return true;
}

QPoint ModelWidget::convertInputPosFromOtherWidget(QMouseEvent* event)
{
    return mapFromGlobal(event->globalPos());
}

bool ModelWidget::inputWheelEventFromOtherWidget(QWheelEvent* event)
{
    if (m_moveStarted)
        return true;

    if (m_mousePickingEnabled) {
        if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier)) {
            emit addMouseRadius((float)event->delta() / 200 / height());
            return true;
        }
    }

    if (!m_zoomEnabled)
        return false;

    qreal delta = geometry().height() * 0.1f;
    if (event->delta() < 0)
        delta = -delta;
    zoom(delta);

    return true;
}

void ModelWidget::zoom(float delta)
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
        return;
    } else {
        m_eyePosition += QVector3D(0, 0, m_eyePosition.z() * (delta > 0 ? -0.1 : 0.1));
        if (m_eyePosition.z() < -15)
            m_eyePosition.setZ(-15);
        else if (m_eyePosition.z() > -0.1)
            m_eyePosition.setZ((float)-0.1);
        emit eyePositionChanged(m_eyePosition);
        emit renderParametersChanged();
        update();
    }
}

void ModelWidget::setMousePickTargetPositionInModelSpace(QVector3D position)
{
    m_mousePickTargetPositionInModelSpace = position;
    update();
}

void ModelWidget::setMousePickRadius(float radius)
{
    m_mousePickRadius = radius;
    update();
}

void ModelWidget::updateMesh(ModelMesh* mesh)
{
    if (!m_modelOpenGLProgram)
        m_modelOpenGLProgram = std::make_unique<ModelOpenGLProgram>();
    m_modelOpenGLProgram->updateTextureImage(std::unique_ptr<QImage>(nullptr != mesh ? mesh->takeTextureImage() : nullptr));
    m_modelOpenGLProgram->updateNormalMapImage(std::unique_ptr<QImage>(nullptr != mesh ? mesh->takeNormalMapImage() : nullptr));
    m_modelOpenGLProgram->updateMetalnessRoughnessAmbientOcclusionMapImage(std::unique_ptr<QImage>(nullptr != mesh ? mesh->takeMetalnessRoughnessAmbientOcclusionMapImage() : nullptr),
        mesh && mesh->hasMetalnessInImage(),
        mesh && mesh->hasRoughnessInImage(),
        mesh && mesh->hasAmbientOcclusionInImage());

    if (!m_modelOpenGLObject)
        m_modelOpenGLObject = std::make_unique<ModelOpenGLObject>();
    m_modelOpenGLObject->update(std::unique_ptr<ModelMesh>(mesh));

    emit renderParametersChanged();
    update();
}

void ModelWidget::updateWireframeMesh(MonochromeMesh* mesh)
{
    if (!m_wireframeOpenGLObject)
        m_wireframeOpenGLObject = std::make_unique<MonochromeOpenGLObject>();
    m_wireframeOpenGLObject->update(std::unique_ptr<MonochromeMesh>(mesh));
    emit renderParametersChanged();
    update();
}

int ModelWidget::widthInPixels()
{
    return m_widthInPixels;
}

int ModelWidget::heightInPixels()
{
    return m_heightInPixels;
}

void ModelWidget::enableMove(bool enabled)
{
    m_moveEnabled = enabled;
}

void ModelWidget::enableZoom(bool enabled)
{
    m_zoomEnabled = enabled;
}

void ModelWidget::enableMousePicking(bool enabled)
{
    m_mousePickingEnabled = enabled;
}

void ModelWidget::setMoveAndZoomByWindow(bool byWindow)
{
    m_moveAndZoomByWindow = byWindow;
}

void ModelWidget::mousePressEvent(QMouseEvent* event)
{
    inputMousePressEventFromOtherWidget(event, m_notGraphics);
}

void ModelWidget::mouseMoveEvent(QMouseEvent* event)
{
    inputMouseMoveEventFromOtherWidget(event);
}

void ModelWidget::wheelEvent(QWheelEvent* event)
{
    inputWheelEventFromOtherWidget(event);
}

void ModelWidget::mouseReleaseEvent(QMouseEvent* event)
{
    inputMouseReleaseEventFromOtherWidget(event);
}

void ModelWidget::setNotGraphics(bool notGraphics)
{
    m_notGraphics = notGraphics;
}

void ModelWidget::normalizeAngle(int& angle)
{
    while (angle < 0)
        angle += 360 * 16;
    while (angle > 360 * 16)
        angle -= 360 * 16;
}

void ModelWidget::initializeGL()
{
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &ModelWidget::cleanup);

    if (m_openGLVersion.isEmpty()) {
        QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
        const char* openGLVersion = (const char*)f->glGetString(GL_VERSION);
        m_openGLVersion = nullptr != openGLVersion ? openGLVersion : "<Unknown>";
        const char* shadingLanguageVersion = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
        m_openGLShadingLanguageVersion = nullptr != shadingLanguageVersion ? shadingLanguageVersion : "<Unknown>";
        m_openGLIsCoreProfile = QSurfaceFormat::defaultFormat().profile() == QSurfaceFormat::CoreProfile;
    }
}

void ModelWidget::paintGL()
{
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();

    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    f->glEnable(GL_BLEND);
    f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    f->glEnable(GL_DEPTH_TEST);
    if (m_enableCullFace)
        f->glEnable(GL_CULL_FACE);
#ifdef GL_LINE_SMOOTH
    f->glEnable(GL_LINE_SMOOTH);
#endif

    // The built-in line width settings is not been well supported on all platforms,
    // for example, the line range on 5.15.57.1-microsoft-standard-WSL2 is 1,
    // that means the wireframe will not looking good,
    // hence, we use the polygon offset to mimic thickness of wireframes
    f->glEnable(GL_POLYGON_OFFSET_FILL);
    f->glPolygonOffset(1.0, 1.0);

    f->glViewport(0, 0, m_widthInPixels, m_heightInPixels);

    m_world.setToIdentity();
    m_world.rotate(m_xRot / 16.0f, 1, 0, 0);
    m_world.rotate(m_yRot / 16.0f, 0, 1, 0);
    m_world.rotate(m_zRot / 16.0f, 0, 0, 1);

    m_camera.setToIdentity();
    m_camera.translate(m_eyePosition.x(), m_eyePosition.y(), m_eyePosition.z());

    if (!m_modelOpenGLProgram) {
        m_modelOpenGLProgram = std::make_unique<ModelOpenGLProgram>();
        m_modelOpenGLProgram->load(format().profile() == QSurfaceFormat::CoreProfile);
    }

    if (m_isWireframeVisible) {
        if (!m_monochromeOpenGLProgram) {
            m_monochromeOpenGLProgram = std::make_unique<MonochromeOpenGLProgram>();
            m_monochromeOpenGLProgram->load(format().profile() == QSurfaceFormat::CoreProfile);
        }
    }

    drawModel();
    if (m_isWireframeVisible) {
        drawWireframe();
    }
}

void ModelWidget::drawWireframe()
{
    m_monochromeOpenGLProgram->bind();

    m_monochromeOpenGLProgram->setUniformValue(m_monochromeOpenGLProgram->getUniformLocationByName("projectionMatrix"), m_projection);
    m_monochromeOpenGLProgram->setUniformValue(m_monochromeOpenGLProgram->getUniformLocationByName("modelMatrix"), m_world);
    m_monochromeOpenGLProgram->setUniformValue(m_monochromeOpenGLProgram->getUniformLocationByName("viewMatrix"), m_camera);

    if (m_wireframeOpenGLObject)
        m_wireframeOpenGLObject->draw();

    m_monochromeOpenGLProgram->release();
}

void ModelWidget::drawModel()
{
    m_modelOpenGLProgram->bind();

    m_modelOpenGLProgram->setUniformValue(m_modelOpenGLProgram->getUniformLocationByName("eyePosition"), m_eyePosition);
    m_modelOpenGLProgram->setUniformValue(m_modelOpenGLProgram->getUniformLocationByName("projectionMatrix"), m_projection);
    m_modelOpenGLProgram->setUniformValue(m_modelOpenGLProgram->getUniformLocationByName("modelMatrix"), m_world);
    m_modelOpenGLProgram->setUniformValue(m_modelOpenGLProgram->getUniformLocationByName("normalMatrix"), m_world.normalMatrix());
    m_modelOpenGLProgram->setUniformValue(m_modelOpenGLProgram->getUniformLocationByName("viewMatrix"), m_camera);

    m_modelOpenGLProgram->bindMaps();

    if (m_modelOpenGLObject)
        m_modelOpenGLObject->draw();

    m_modelOpenGLProgram->releaseMaps();

    m_modelOpenGLProgram->release();
}
