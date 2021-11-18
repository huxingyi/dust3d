#include <QOpenGLFramebufferObjectFormat>
#include <QThread>
#include <QDebug>
#include "model_offscreen_render.h"

ModelOffscreenRender::ModelOffscreenRender(const QSurfaceFormat &format, QScreen *targetScreen) :
    QOffscreenSurface(targetScreen),
    m_context(nullptr),
    m_mesh(nullptr)
{
    setFormat(format);
    create();
    if (!isValid())
        qDebug() << "ModelOffscreenRender is invalid";
}

ModelOffscreenRender::~ModelOffscreenRender()
{
    // FIXME: If delete m_renderFbo inside toImage, 
    // sometimes, the application will freeze, maybe there are dead locks inside the destruction call
    // move it here can make sure it will be deleted on the main GUI thread to avoid dead locks
    delete m_renderFbo;
    
    destroy();
    delete m_mesh;
    delete m_normalMap;
    delete m_depthMap;
}

void ModelOffscreenRender::updateMesh(Model *mesh)
{
    delete m_mesh;
    m_mesh = mesh;
}

void ModelOffscreenRender::setRenderThread(QThread *thread)
{
	this->moveToThread(thread);
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

void ModelOffscreenRender::setEyePosition(const QVector3D &eyePosition)
{
	m_eyePosition = eyePosition;
}

void ModelOffscreenRender::setMoveToPosition(const QVector3D &moveToPosition)
{
	m_moveToPosition = moveToPosition;
}

void ModelOffscreenRender::enableWireframe()
{
    m_isWireframeVisible = true;
}

void ModelOffscreenRender::setRenderPurpose(int purpose)
{
    m_renderPurpose = purpose;
}

void ModelOffscreenRender::setToonShading(bool toonShading)
{
    m_toonShading = toonShading;
}

void ModelOffscreenRender::enableEnvironmentLight()
{
    m_isEnvironmentLightEnabled = true;
}

void ModelOffscreenRender::updateToonNormalAndDepthMaps(QImage *normalMap, QImage *depthMap)
{
    delete m_normalMap;
    m_normalMap = normalMap;
    
    delete m_depthMap;
    m_depthMap = depthMap;
}

QImage ModelOffscreenRender::toImage(const QSize &size)
{
	QImage image;
	
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
    format.setInternalTextureFormat(GL_RGBA32F_ARB);
    m_renderFbo = new QOpenGLFramebufferObject(size, format);
    m_renderFbo->bind();
    m_context->functions()->glViewport(0, 0, size.width(), size.height());
    
    if (nullptr != m_mesh) {
        QMatrix4x4 projection;
        QMatrix4x4 world;
        QMatrix4x4 camera;
        
        bool isCoreProfile = false;
        const char *versionString = (const char *)m_context->functions()->glGetString(GL_VERSION);
        if (nullptr != versionString &&
                '\0' != versionString[0] &&
                0 == strstr(versionString, "Mesa")) {
            isCoreProfile = m_context->format().profile() == QSurfaceFormat::CoreProfile;
        }
        
        ModelShaderProgram *program = new ModelShaderProgram(isCoreProfile);
        ModelMeshBinder meshBinder;
        meshBinder.initialize();
        if (m_isWireframeVisible)
            meshBinder.showWireframe();
        else
            meshBinder.hideWireframe();
        if (m_isEnvironmentLightEnabled)
            meshBinder.enableEnvironmentLight();
        if (nullptr != m_normalMap && nullptr != m_depthMap) {
            meshBinder.updateToonNormalAndDepthMaps(m_normalMap, m_depthMap);
            m_normalMap = nullptr;
            m_depthMap = nullptr;
        }
		
        m_context->functions()->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        m_context->functions()->glEnable(GL_BLEND);
        m_context->functions()->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        m_context->functions()->glEnable(GL_DEPTH_TEST);
        //m_context->functions()->glEnable(GL_CULL_FACE);
#ifdef GL_LINE_SMOOTH
        m_context->functions()->glEnable(GL_LINE_SMOOTH);
#endif

        world.setToIdentity();
        world.rotate(m_xRot / 16.0f, 1, 0, 0);
        world.rotate(m_yRot / 16.0f, 0, 1, 0);
        world.rotate(m_zRot / 16.0f, 0, 0, 1);

        projection.setToIdentity();
        projection.translate(m_moveToPosition.x(), m_moveToPosition.y(), m_moveToPosition.z());
        projection.perspective(45.0f, GLfloat(size.width()) / size.height(), 0.01f, 100.0f);
        
        camera.setToIdentity();
        camera.translate(m_eyePosition);
        
        program->bind();
        program->setUniformValue(program->eyePosLoc(), m_eyePosition);
        program->setUniformValue(program->toonShadingEnabledLoc(), m_toonShading ? 1 : 0);
        program->setUniformValue(program->projectionMatrixLoc(), projection);
        program->setUniformValue(program->modelMatrixLoc(), world);
        QMatrix3x3 normalMatrix = world.normalMatrix();
        program->setUniformValue(program->normalMatrixLoc(), normalMatrix);
        program->setUniformValue(program->viewMatrixLoc(), camera);
        program->setUniformValue(program->textureEnabledLoc(), 0);
        program->setUniformValue(program->normalMapEnabledLoc(), 0);
        program->setUniformValue(program->mousePickEnabledLoc(), 0);
        program->setUniformValue(program->renderPurposeLoc(), m_renderPurpose);
        
        program->setUniformValue(program->toonEdgeEnabledLoc(), 0);
        program->setUniformValue(program->screenWidthLoc(), (GLfloat)size.width());
        program->setUniformValue(program->screenHeightLoc(), (GLfloat)size.height());
        program->setUniformValue(program->toonNormalMapIdLoc(), 0);
        program->setUniformValue(program->toonDepthMapIdLoc(), 0);

        meshBinder.updateMesh(m_mesh);
        meshBinder.paint(program);

        meshBinder.cleanup();

        program->release();
        delete program;

        m_mesh = nullptr;
    }
    
    m_context->functions()->glFlush();
    
    image = m_renderFbo->toImage();
    
    m_renderFbo->release();

    m_context->doneCurrent();
    delete m_context;
    m_context = nullptr;

    return image;
}
