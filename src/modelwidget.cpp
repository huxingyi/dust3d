#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QCoreApplication>
#include <QGuiApplication>
#include <math.h>
#include <QVector4D>
#include <QSurfaceFormat>
#include "modelwidget.h"
#include "util.h"
#include "preferences.h"

// Modifed from http://doc.qt.io/qt-5/qtopengl-hellogl2-glwidget-cpp.html

bool ModelWidget::m_transparent = true;
const QVector3D ModelWidget::m_cameraPosition = QVector3D(0, 0, -4.0);
float ModelWidget::m_minZoomRatio = 5.0;
float ModelWidget::m_maxZoomRatio = 80.0;

int ModelWidget::m_defaultXRotation = 30 * 16;
int ModelWidget::m_defaultYRotation = -45 * 16;
int ModelWidget::m_defaultZRotation = 0;

ModelWidget::ModelWidget(QWidget *parent) :
    QOpenGLWidget(parent),
    m_xRot(m_defaultXRotation),
    m_yRot(m_defaultYRotation),
    m_zRot(m_defaultZRotation),
    m_program(nullptr),
    m_moveStarted(false),
    m_moveEnabled(true),
    m_zoomEnabled(true),
    m_mousePickingEnabled(false)
{
    // --transparent causes the clear color to be transparent. Therefore, on systems that
    // support it, the widget will become transparent apart from the logo.
    if (m_transparent) {
        setAttribute(Qt::WA_AlwaysStackOnTop);
        setAttribute(Qt::WA_TranslucentBackground);
        QSurfaceFormat fmt = format();
        fmt.setAlphaBufferSize(8);
        fmt.setSamples(4);
        setFormat(fmt);
    } else {
        QSurfaceFormat fmt = format();
        fmt.setSamples(4);
        setFormat(fmt);
    }
    setContextMenuPolicy(Qt::CustomContextMenu);
    
    m_widthInPixels = width() * window()->devicePixelRatio();
	m_heightInPixels = height() * window()->devicePixelRatio();
	
    zoom(200);
    
    connect(&Preferences::instance(), &Preferences::toonShadingChanged, this, &ModelWidget::reRender);
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
    qNormalizeAngle(angle);
    if (angle != m_xRot) {
        m_xRot = angle;
        emit xRotationChanged(angle);
        emit renderParametersChanged();
        update();
    }
}

void ModelWidget::setYRotation(int angle)
{
    qNormalizeAngle(angle);
    if (angle != m_yRot) {
        m_yRot = angle;
        emit yRotationChanged(angle);
        emit renderParametersChanged();
        update();
    }
}

void ModelWidget::setZRotation(int angle)
{
    qNormalizeAngle(angle);
    if (angle != m_zRot) {
        m_zRot = angle;
        emit zRotationChanged(angle);
        emit renderParametersChanged();
        update();
    }
}

MeshLoader *ModelWidget::fetchCurrentMesh()
{
    return m_meshBinder.fetchCurrentMesh();
}

void ModelWidget::cleanup()
{
    if (m_program == nullptr)
        return;
    makeCurrent();
    m_meshBinder.cleanup();
    delete m_program;
    m_program = nullptr;
    doneCurrent();
}

void ModelWidget::initializeGL()
{
    // In this example the widget's corresponding top-level window can change
    // several times during the widget's lifetime. Whenever this happens, the
    // QOpenGLWidget's associated context is destroyed and a new one is created.
    // Therefore we have to be prepared to clean up the resources on the
    // aboutToBeDestroyed() signal, instead of the destructor. The emission of
    // the signal will be followed by an invocation of initializeGL() where we
    // can recreate all resources.
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &ModelWidget::cleanup);

    initializeOpenGLFunctions();
    if (m_transparent) {
        glClearColor(0, 0, 0, 0);
    } else {
        QColor bgcolor = QWidget::palette().color(QWidget::backgroundRole());
        glClearColor(bgcolor.redF(), bgcolor.greenF(), bgcolor.blueF(), 1);
    }
    
    bool isCoreProfile = false;
    const char *versionString = (const char *)glGetString(GL_VERSION);
    if (nullptr != versionString &&
            '\0' != versionString[0] &&
            0 == strstr(versionString, "Mesa")) {
        isCoreProfile = format().profile() == QSurfaceFormat::CoreProfile;
    }
        
    m_program = new ModelShaderProgram(isCoreProfile);
    
    // Create a vertex array object. In OpenGL ES 2.0 and OpenGL 2.x
    // implementations this is optional and support may not be present
    // at all. Nonetheless the below code works in all cases and makes
    // sure there is a VAO when one is needed.
    m_meshBinder.initialize();

    // Our camera never changes in this example.
    m_camera.setToIdentity();
    // FIXME: if change here, please also change the camera pos in PBR shader
    m_camera.translate(m_cameraPosition.x(), m_cameraPosition.y(), m_cameraPosition.z());

    // Light position is fixed.
    // FIXME: PBR render no longer use this parameter
    m_program->setUniformValue(m_program->lightPosLoc(), QVector3D(0, 0, 70));

    m_program->release();
}

void ModelWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
#ifdef GL_LINE_SMOOTH
    glEnable(GL_LINE_SMOOTH);
#endif
	glViewport(0, 0, m_widthInPixels, m_heightInPixels);

    m_world.setToIdentity();
    m_world.rotate(m_xRot / 16.0f, 1, 0, 0);
    m_world.rotate(m_yRot / 16.0f, 0, 1, 0);
    m_world.rotate(m_zRot / 16.0f, 0, 0, 1);

    m_program->bind();
    m_program->setUniformValue(m_program->toonShadingEnabledLoc(), Preferences::instance().toonShading() ? 1 : 0);
    m_program->setUniformValue(m_program->projectionMatrixLoc(), m_projection);
    m_program->setUniformValue(m_program->modelMatrixLoc(), m_world);
    QMatrix3x3 normalMatrix = m_world.normalMatrix();
    m_program->setUniformValue(m_program->normalMatrixLoc(), normalMatrix);
    m_program->setUniformValue(m_program->viewMatrixLoc(), m_camera);
    m_program->setUniformValue(m_program->textureEnabledLoc(), 0);
    m_program->setUniformValue(m_program->normalMapEnabledLoc(), 0);
    m_program->setUniformValue(m_program->renderPurposeLoc(), 0);
    
    m_program->setUniformValue(m_program->toonEdgeEnabledLoc(), 0);
    m_program->setUniformValue(m_program->screenWidthLoc(), (GLfloat)m_widthInPixels);
    m_program->setUniformValue(m_program->screenHeightLoc(), (GLfloat)m_heightInPixels);
    m_program->setUniformValue(m_program->toonNormalMapIdLoc(), 0);
    m_program->setUniformValue(m_program->toonDepthMapIdLoc(), 0);
    
    if (m_mousePickingEnabled && !m_mousePickTargetPositionInModelSpace.isNull()) {
        m_program->setUniformValue(m_program->mousePickEnabledLoc(), 1);
        m_program->setUniformValue(m_program->mousePickTargetPositionLoc(),
            m_world * m_mousePickTargetPositionInModelSpace);
    } else {
        m_program->setUniformValue(m_program->mousePickEnabledLoc(), 0);
        m_program->setUniformValue(m_program->mousePickTargetPositionLoc(), QVector3D());
    }
    m_program->setUniformValue(m_program->mousePickRadiusLoc(), m_mousePickRadius);
    
    m_meshBinder.paint(m_program);

    m_program->release();
}

void ModelWidget::resizeGL(int w, int h)
{
	m_widthInPixels = w * window()->devicePixelRatio();
	m_heightInPixels = h * window()->devicePixelRatio();
    m_projection.setToIdentity();
    m_projection.perspective(45.0f, GLfloat(w) / h, 0.01f, 100.0f);
    emit renderParametersChanged();
}

std::pair<QVector3D, QVector3D> ModelWidget::screenPositionToMouseRay(const QPoint &screenPosition)
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
    if (m_meshBinder.isWireframesVisible())
        m_meshBinder.hideWireframes();
    else
        m_meshBinder.showWireframes();
    update();
}

void ModelWidget::enableEnvironmentLight()
{
    m_meshBinder.enableEnvironmentLight();
    update();
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

void ModelWidget::toggleUvCheck()
{
    if (m_meshBinder.isCheckUvEnabled())
        m_meshBinder.disableCheckUv();
    else
        m_meshBinder.enableCheckUv();
    m_meshBinder.reloadMesh();
    update();
}

bool ModelWidget::inputMousePressEventFromOtherWidget(QMouseEvent *event)
{
    bool shouldStartMove = false;
    if (event->button() == Qt::LeftButton) {
        if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::AltModifier) &&
                !QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier)) {
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
        }
        return true;
    }
    return false;
}

bool ModelWidget::inputMouseReleaseEventFromOtherWidget(QMouseEvent *event)
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

bool ModelWidget::inputMouseMoveEventFromOtherWidget(QMouseEvent *event)
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

    if ((event->buttons() & Qt::MidButton) ||
            (m_moveStarted && (event->buttons() & Qt::LeftButton))) {
        if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier)) {
            if (m_moveStarted) {
                QRect rect = m_moveStartGeometry;
                QPoint posInParent = mapToParent(pos);
                rect.translate(posInParent.x() - m_moveStartPos.x(), posInParent.y() - m_moveStartPos.y());
                setGeometry(rect);
            }
        } else {
            setXRotation(m_xRot + 8 * dy);
            setYRotation(m_yRot + 8 * dx);
        }
    }
    m_lastPos = pos;
    
    return true;
}

QPoint ModelWidget::convertInputPosFromOtherWidget(QMouseEvent *event)
{
    return mapFromGlobal(event->globalPos());
}

bool ModelWidget::inputWheelEventFromOtherWidget(QWheelEvent *event)
{
    if (m_moveStarted)
        return true;
    
    if (m_mousePickingEnabled) {
        if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier)) {
            emit addMouseRadius((float)event->delta() / 40 / height());
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

void ModelWidget::updateMesh(MeshLoader *mesh)
{
    m_meshBinder.updateMesh(mesh);
    emit renderParametersChanged();
    update();
}

void ModelWidget::fetchCurrentToonNormalAndDepthMaps(QImage *normalMap, QImage *depthMap)
{
    m_meshBinder.fetchCurrentToonNormalAndDepthMaps(normalMap, depthMap);
}

void ModelWidget::updateToonNormalAndDepthMaps(QImage *normalMap, QImage *depthMap)
{
    m_meshBinder.updateToonNormalAndDepthMaps(normalMap, depthMap);
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

void ModelWidget::mousePressEvent(QMouseEvent *event)
{
    inputMousePressEventFromOtherWidget(event);
}

void ModelWidget::mouseMoveEvent(QMouseEvent *event)
{
    inputMouseMoveEventFromOtherWidget(event);
}

void ModelWidget::wheelEvent(QWheelEvent *event)
{
    inputWheelEventFromOtherWidget(event);
}

void ModelWidget::mouseReleaseEvent(QMouseEvent *event)
{
    inputMouseReleaseEventFromOtherWidget(event);
}

