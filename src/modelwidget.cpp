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
      m_program(0),
      m_renderTriangleVertexCount(0),
      m_renderEdgeVertexCount(0),
      m_mesh(NULL),
      m_meshUpdated(false),
      m_moveStarted(false),
      m_graphicsView(NULL)
{
    m_core = QSurfaceFormat::defaultFormat().profile() == QSurfaceFormat::CoreProfile;
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
    delete m_mesh;
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
    if (m_vboTriangle.isCreated())
        m_vboTriangle.destroy();
    delete m_program;
    m_program = 0;
    doneCurrent();
}

static const char *vertexShaderSourceCore =
    "#version 150\n"
    "in vec4 vertex;\n"
    "in vec3 normal;\n"
    "in vec3 color;\n"
    "out vec3 vert;\n"
    "out vec3 vertNormal;\n"
    "out vec3 vertColor;\n"
    "uniform mat4 projMatrix;\n"
    "uniform mat4 mvMatrix;\n"
    "uniform mat3 normalMatrix;\n"
    "void main() {\n"
    "   vert = vertex.xyz;\n"
    "   vertNormal = normalMatrix * normal;\n"
    "   vertColor = color;\n"
    "   gl_Position = projMatrix * mvMatrix * vertex;\n"
    "}\n";

static const char *fragmentShaderSourceCore =
    "#version 150\n"
    "in highp vec3 vert;\n"
    "in highp vec3 vertNormal;\n"
    "in highp vec3 vertColor;\n"
    "out highp vec4 fragColor;\n"
    "uniform highp vec3 lightPos;\n"
    "void main() {\n"
    "   highp vec3 L = normalize(lightPos - vert);\n"
    "   highp float NL = max(dot(normalize(vertNormal), L), 0.0);\n"
    "   highp vec3 color = vertColor;\n"
    "   highp vec3 col = clamp(color * 0.2 + color * 0.8 * NL, 0.0, 1.0);\n"
    "   fragColor = vec4(col, 1.0);\n"
    "}\n";

static const char *vertexShaderSource =
    "attribute vec4 vertex;\n"
    "attribute vec3 normal;\n"
    "attribute vec3 color;\n"
    "varying vec3 vert;\n"
    "varying vec3 vertNormal;\n"
    "varying vec3 vertColor;\n"
    "uniform mat4 projMatrix;\n"
    "uniform mat4 mvMatrix;\n"
    "uniform mat3 normalMatrix;\n"
    "void main() {\n"
    "   vert = vertex.xyz;\n"
    "   vertNormal = normalMatrix * normal;\n"
    "   vertColor = color;\n"
    "   gl_Position = projMatrix * mvMatrix * vertex;\n"
    "}\n";

static const char *fragmentShaderSource =
    "varying highp vec3 vert;\n"
    "varying highp vec3 vertNormal;\n"
    "varying highp vec3 vertColor;\n"
    "uniform highp vec3 lightPos;\n"
    "void main() {\n"
    "   highp vec3 L = normalize(lightPos - vert);\n"
    "   highp float NL = max(dot(normalize(vertNormal), L), 0.0);\n"
    "   highp vec3 color = vertColor;\n"
    "   highp vec3 col = clamp(color * 0.2 + color * 0.8 * NL, 0.0, 1.0);\n"
    "   gl_FragColor = vec4(col, 1.0);\n"
    "}\n";

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

    m_program = new QOpenGLShaderProgram;
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, m_core ? vertexShaderSourceCore : vertexShaderSource);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, m_core ? fragmentShaderSourceCore : fragmentShaderSource);
    m_program->bindAttributeLocation("vertex", 0);
    m_program->bindAttributeLocation("normal", 1);
    m_program->bindAttributeLocation("color", 2);
    m_program->link();

    m_program->bind();
    m_projMatrixLoc = m_program->uniformLocation("projMatrix");
    m_mvMatrixLoc = m_program->uniformLocation("mvMatrix");
    m_normalMatrixLoc = m_program->uniformLocation("normalMatrix");
    m_lightPosLoc = m_program->uniformLocation("lightPos");

    // Create a vertex array object. In OpenGL ES 2.0 and OpenGL 2.x
    // implementations this is optional and support may not be present
    // at all. Nonetheless the below code works in all cases and makes
    // sure there is a VAO when one is needed.
    m_vaoTriangle.create();
    m_vaoEdge.create();

    // Our camera never changes in this example.
    m_camera.setToIdentity();
    m_camera.translate(0, 0, -2.5);

    // Light position is fixed.
    m_program->setUniformValue(m_lightPosLoc, QVector3D(0, 0, 70));

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

    {
        QMutexLocker lock(&m_meshMutex);
        if (m_meshUpdated) {
            if (m_mesh) {
                {
                    QOpenGLVertexArrayObject::Binder vaoBinder(&m_vaoTriangle);
                    if (m_vboTriangle.isCreated())
                        m_vboTriangle.destroy();
                    m_vboTriangle.create();
                    m_vboTriangle.bind();
                    m_vboTriangle.allocate(m_mesh->triangleVertices(), m_mesh->triangleVertexCount() * sizeof(Vertex));
                    m_renderTriangleVertexCount = m_mesh->triangleVertexCount();
                    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
                    f->glEnableVertexAttribArray(0);
                    f->glEnableVertexAttribArray(1);
                    f->glEnableVertexAttribArray(2);
                    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), 0);
                    f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), reinterpret_cast<void *>(3 * sizeof(GLfloat)));
                    f->glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), reinterpret_cast<void *>(6 * sizeof(GLfloat)));
                    m_vboTriangle.release();
                }
                {
                    QOpenGLVertexArrayObject::Binder vaoBinder(&m_vaoEdge);
                    if (m_vboEdge.isCreated())
                        m_vboEdge.destroy();
                    m_vboEdge.create();
                    m_vboEdge.bind();
                    m_vboEdge.allocate(m_mesh->edgeVertices(), m_mesh->edgeVertexCount() * sizeof(Vertex));
                    m_renderEdgeVertexCount = m_mesh->edgeVertexCount();
                    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
                    f->glEnableVertexAttribArray(0);
                    f->glEnableVertexAttribArray(1);
                    f->glEnableVertexAttribArray(2);
                    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), 0);
                    f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), reinterpret_cast<void *>(3 * sizeof(GLfloat)));
                    f->glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), reinterpret_cast<void *>(6 * sizeof(GLfloat)));
                    m_vboEdge.release();
                }
            } else {
                m_renderTriangleVertexCount = 0;
                m_renderEdgeVertexCount = 0;
            }
            m_meshUpdated = false;
        }
    }
    
    m_program->bind();
    m_program->setUniformValue(m_projMatrixLoc, m_proj);
    m_program->setUniformValue(m_mvMatrixLoc, m_camera * m_world);
    QMatrix3x3 normalMatrix = m_world.normalMatrix();
    m_program->setUniformValue(m_normalMatrixLoc, normalMatrix);
    
    if (m_renderEdgeVertexCount > 0) {
        QOpenGLVertexArrayObject::Binder vaoBinder(&m_vaoEdge);
        glDrawArrays(GL_LINES, 0, m_renderEdgeVertexCount);
    }
    if (m_renderTriangleVertexCount > 0) {
        QOpenGLVertexArrayObject::Binder vaoBinder(&m_vaoTriangle);
        glDrawArrays(GL_TRIANGLES, 0, m_renderTriangleVertexCount);
    }

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
    QMutexLocker lock(&m_meshMutex);
    if (mesh != m_mesh) {
        delete m_mesh;
        m_mesh = mesh;
        m_meshUpdated = true;
        update();
    }
}

void ModelWidget::exportMeshAsObj(const QString &filename)
{
    QMutexLocker lock(&m_meshMutex);
    if (m_mesh) {
        QFile file(filename);
        if (file.open(QIODevice::WriteOnly)) {
            QTextStream stream(&file);
            stream << "# " << Ds3FileReader::m_applicationName << endl;
            for (std::vector<const QVector3D>::iterator it = m_mesh->vertices().begin() ; it != m_mesh->vertices().end(); ++it) {
                stream << "v " << (*it).x() << " " << (*it).y() << " " << (*it).z() << endl;
            }
            for (std::vector<const std::vector<int>>::iterator it = m_mesh->faces().begin() ; it != m_mesh->faces().end(); ++it) {
                stream << "f";
                for (std::vector<const int>::iterator subIt = (*it).begin() ; subIt != (*it).end(); ++subIt) {
                    stream << " " << (1 + *subIt);
                }
                stream << endl;
            }
        }
    }
}
