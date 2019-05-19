#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QCoreApplication>
#include <QGuiApplication>
#include <math.h>
#include "modelwidget.h"
#include "util.h"

// Modifed from http://doc.qt.io/qt-5/qtopengl-hellogl2-glwidget-cpp.html

bool ModelWidget::m_transparent = true;

ModelWidget::ModelWidget(QWidget *parent) :
    QOpenGLWidget(parent),
    m_xRot(30 * 16),
    m_yRot(-45 * 16),
    m_zRot(0),
    m_program(nullptr),
    m_moveStarted(false),
    m_moveEnabled(true),
    m_zoomEnabled(true)
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
    zoom(200);
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
        update();
    }
}

void ModelWidget::setYRotation(int angle)
{
    qNormalizeAngle(angle);
    if (angle != m_yRot) {
        m_yRot = angle;
        emit yRotationChanged(angle);
        update();
    }
}

void ModelWidget::setZRotation(int angle)
{
    qNormalizeAngle(angle);
    if (angle != m_zRot) {
        m_zRot = angle;
        emit zRotationChanged(angle);
        update();
    }
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

    m_program = new ModelShaderProgram;
    
    // Create a vertex array object. In OpenGL ES 2.0 and OpenGL 2.x
    // implementations this is optional and support may not be present
    // at all. Nonetheless the below code works in all cases and makes
    // sure there is a VAO when one is needed.
    m_meshBinder.initialize();

    // Our camera never changes in this example.
    m_camera.setToIdentity();
    // FIXME: if change here, please also change the camera pos in PBR shader
    m_camera.translate(0, 0, -4.0);

    // Light position is fixed.
    // FIXME: PBR render no longer use this parameter
    m_program->setUniformValue(m_program->lightPosLoc(), QVector3D(0, 0, 70));

    m_program->release();
}

void ModelWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LINE_SMOOTH);

    m_world.setToIdentity();
    m_world.rotate(m_xRot / 16.0f, 1, 0, 0);
    m_world.rotate(m_yRot / 16.0f, 0, 1, 0);
    m_world.rotate(m_zRot / 16.0f, 0, 0, 1);

    m_program->bind();
    m_program->setUniformValue(m_program->projectionMatrixLoc(), m_projection);
    m_program->setUniformValue(m_program->modelMatrixLoc(), m_world);
    QMatrix3x3 normalMatrix = m_world.normalMatrix();
    m_program->setUniformValue(m_program->normalMatrixLoc(), normalMatrix);
    m_program->setUniformValue(m_program->viewMatrixLoc(), m_camera);
    m_program->setUniformValue(m_program->textureEnabledLoc(), 0);
    m_program->setUniformValue(m_program->normalMapEnabledLoc(), 0);
    
    m_meshBinder.paint(m_program);

    m_program->release();
}

void ModelWidget::resizeGL(int w, int h)
{
    m_projection.setToIdentity();
    m_projection.perspective(45.0f, GLfloat(w) / h, 0.01f, 100.0f);
}

void ModelWidget::toggleWireframe()
{
    if (m_meshBinder.isWireframesVisible())
        m_meshBinder.hideWireframes();
    else
        m_meshBinder.showWireframes();
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
    return false;
}

bool ModelWidget::inputMouseMoveEventFromOtherWidget(QMouseEvent *event)
{
    if (!m_moveStarted) {
        return false;
    }
    
    QPoint pos = convertInputPosFromOtherWidget(event);
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
    //QPoint globalPos = event->globalPos();
    //QPoint zero;
    //QPoint leftTop = mapToGlobal(zero);
    //QRect globalRect(leftTop.x(), leftTop.y(), width(), height());
    //if (!globalRect.contains(globalPos))
    //    return false;
    
    if (m_moveStarted)
        return true;
    if (!m_zoomEnabled)
        return false;
    qreal delta = event->delta() / 10;
    if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier)) {
        if (delta > 0)
            delta = 1;
        else
            delta = -1;
    } else {
        if (fabs(delta) < 1)
            delta = delta < 0 ? -1.0 : 1.0;
    }
    zoom(delta);
    return true;
}

void ModelWidget::zoom(float delta)
{
    QMargins margins(delta, delta, delta, delta);
    setGeometry(geometry().marginsAdded(margins));
}

void ModelWidget::updateMesh(MeshLoader *mesh)
{
    m_meshBinder.updateMesh(mesh);
    update();
}

void ModelWidget::enableMove(bool enabled)
{
    m_moveEnabled = enabled;
}

void ModelWidget::enableZoom(bool enabled)
{
    m_zoomEnabled = enabled;
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

