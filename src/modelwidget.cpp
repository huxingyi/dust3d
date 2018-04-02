#include "modelwidget.h"
#include "ds3file.h"
#include "skeletoneditgraphicsview.h"
#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QCoreApplication>
#include <QGuiApplication>
#include <math.h>

// Modifed from http://doc.qt.io/qt-5/qtopengl-hellogl2-glwidget-cpp.html

bool ModelWidget::m_transparent = true;

ModelWidget::ModelWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      m_xRot(0),
      m_yRot(0),
      m_zRot(0),
      m_program(NULL),
      m_moveStarted(false),
      m_graphicsView(NULL)
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
}

void ModelWidget::setGraphicsView(SkeletonEditGraphicsView *view)
{
    m_graphicsView = view;
}

ModelWidget::~ModelWidget()
{
    cleanup();
}

static void qNormalizeAngle(int &angle)
{
    while (angle < 0)
        angle += 360 * 16;
    while (angle > 360 * 16)
        angle -= 360 * 16;
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
    m_program = 0;
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
    m_camera.translate(0, 0, -2.5);

    // Light position is fixed.
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
    m_world.rotate(180.0f - (m_xRot / 16.0f), 1, 0, 0);
    m_world.rotate(m_yRot / 16.0f, 0, 1, 0);
    m_world.rotate(m_zRot / 16.0f, 0, 0, 1);

    
    m_program->bind();
    m_program->setUniformValue(m_program->projMatrixLoc(), m_proj);
    m_program->setUniformValue(m_program->mvMatrixLoc(), m_camera * m_world);
    QMatrix3x3 normalMatrix = m_world.normalMatrix();
    m_program->setUniformValue(m_program->normalMatrixLoc(), normalMatrix);
    
    m_meshBinder.paint();

    m_program->release();
}

void ModelWidget::resizeGL(int w, int h)
{
    m_proj.setToIdentity();
    m_proj.perspective(45.0f, GLfloat(w) / h, 0.01f, 100.0f);
}

void ModelWidget::mousePressEvent(QMouseEvent *event)
{
    if (!m_moveStarted && m_graphicsView && m_graphicsView->mousePress(event, m_graphicsView->mapToScene(m_graphicsView->mapFromGlobal(event->globalPos()))))
        return;
    m_lastPos = event->pos();
    setMouseTracking(true);
    if (event->button() == Qt::LeftButton) {
        if (!m_moveStarted) {
            m_moveStartPos = mapToParent(event->pos());
            m_moveStartGeometry = geometry();
            m_moveStarted = true;
        }
    }
}

void ModelWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_moveStarted && m_graphicsView && m_graphicsView->mouseRelease(event, m_graphicsView->mapToScene(m_graphicsView->mapFromGlobal(event->globalPos()))))
        return;
    if (m_moveStarted) {
        m_moveStarted = false;
    }
}

void ModelWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_moveStarted && m_graphicsView && m_graphicsView->mouseMove(event, m_graphicsView->mapToScene(m_graphicsView->mapFromGlobal(event->globalPos()))))
        return;
    
    int dx = event->x() - m_lastPos.x();
    int dy = event->y() - m_lastPos.y();

    if (event->buttons() & Qt::LeftButton) {
        if (m_moveStarted) {
            QRect rect = m_moveStartGeometry;
            QPoint pos = mapToParent(event->pos());
            rect.translate(pos.x() - m_moveStartPos.x(), pos.y() - m_moveStartPos.y());
            setGeometry(rect);
        }
    } else if (event->buttons() & Qt::RightButton) {
        setXRotation(m_xRot - 8 * dy);
        setYRotation(m_yRot - 8 * dx);
    }
    m_lastPos = event->pos();
}

void ModelWidget::wheelEvent(QWheelEvent *event)
{
    if (!m_moveStarted && m_graphicsView && m_graphicsView->wheel(event, m_graphicsView->mapToScene(m_graphicsView->mapFromGlobal(event->globalPos()))))
        return;
    if (m_moveStarted)
        return;
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
    QMargins margins(delta, delta, delta, delta);
    setGeometry(geometry().marginsAdded(margins));
}

void ModelWidget::updateMesh(Mesh *mesh)
{
    m_meshBinder.updateMesh(mesh);
    update();
}

void ModelWidget::exportMeshAsObj(const QString &filename)
{
    m_meshBinder.exportMeshAsObj(filename);
}
