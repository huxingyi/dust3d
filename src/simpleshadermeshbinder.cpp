// This file follow the Stackoverflow content license: CC BY-SA 4.0,
// since it's based on Prashanth N Udupa's work: https://stackoverflow.com/questions/35134270/how-to-use-qopenglframebufferobject-for-shadow-mapping
#include <QColor>
#include <QMutexLocker>
#include <QDebug>
#include "simpleshadermeshbinder.h"

void SimpleShaderMeshBinder::updateMesh(SimpleShaderMesh *mesh)
{
    QMutexLocker lock(&m_newMeshMutex);
    if (mesh != m_mesh) {
        delete m_newMesh;
        m_newMesh = mesh;
        m_newMeshComing = true;
    }
}

void SimpleShaderMeshBinder::checkNewMesh()
{
    SimpleShaderMesh *newMesh = nullptr;
    if (m_newMeshComing) {
        QMutexLocker lock(&m_newMeshMutex);
        if (m_newMeshComing) {
            newMesh = m_newMesh;
            m_newMesh = nullptr;
            m_newMeshComing = false;
        }
    }
    
    if (nullptr != newMesh) {
        QVector<QVector3D> vertices;
        QVector<int> indexes;
        
        const auto &meshVertices = *newMesh->vertices();
        const auto &meshTriangles = *newMesh->triangles();
        const auto &meshNormals = *newMesh->triangleCornerNormals();
        int i = 0;
        for (size_t triangleIndex = 0; triangleIndex < meshTriangles.size(); ++triangleIndex) {
            const auto &triangle = meshTriangles[triangleIndex];
            const auto &v0 = meshVertices[triangle[0]];
            const auto &v1 = meshVertices[triangle[1]];
            const auto &v2 = meshVertices[triangle[2]];
            vertices.push_back(QVector3D(v0[0], v0[1], v0[2]));
            vertices.push_back(QVector3D(v1[0], v1[1], v1[2]));
            vertices.push_back(QVector3D(v2[0], v2[1], v2[2]));
            indexes << i << i + 1 << i + 2;
            i += 3;
        }
        m_vertexCount = vertices.size();
        m_normalOffset = vertices.size() * int(sizeof(QVector3D));
        for (size_t triangleIndex = 0; triangleIndex < meshTriangles.size(); ++triangleIndex) {
            const auto &normals = meshNormals[triangleIndex];
            const auto &n0 = normals[0];
            const auto &n1 = normals[1];
            const auto &n2 = normals[2];
            vertices.push_back(QVector3D(n0[0], n0[1], n0[2]));
            vertices.push_back(QVector3D(n1[0], n1[1], n1[2]));
            vertices.push_back(QVector3D(n2[0], n2[1], n2[2]));
        }

        delete m_vertexBuffer;
        m_vertexBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        m_vertexBuffer->create();
        m_vertexBuffer->bind();
        m_vertexBuffer->allocate(
                static_cast<const void*>(vertices.constData()),
                vertices.size() * int(sizeof(QVector3D))
            );
        m_vertexBuffer->release();
        
        delete m_indexBuffer;
        m_indexBuffer = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
        m_indexBuffer->create();
        m_indexBuffer->bind();
        m_indexBuffer->allocate(
                  static_cast<const void*>(indexes.constData()),
                  indexes.size() * int(sizeof(int))
            );
        m_indexBuffer->release();
        
        delete newMesh;
    }
}

void SimpleShaderMeshBinder::renderShadow(const QMatrix4x4 &projectionMatrix, const QMatrix4x4 &viewMatrix)
{
    if (nullptr == m_shadowProgram) {
        if (!m_openglFunctionsInitialized) {
            QOpenGLFunctions::initializeOpenGLFunctions();
            m_openglFunctionsInitialized = true;
        }

        m_shadowProgram = new QOpenGLShaderProgram;
        m_shadowProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/shadow.vert");
        m_shadowProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/shadow.frag");
        m_shadowProgram->link();
    }

    checkNewMesh();
    
    if (nullptr == m_indexBuffer) {
        return;
    }
    
    m_shadowProgram->bind();
    m_vertexBuffer->bind();
    m_indexBuffer->bind();

    const QMatrix4x4 modelMatrix = m_sceneMatrix * m_matrix;
    const QMatrix4x4 lightViewProjectionMatrix = projectionMatrix * viewMatrix * modelMatrix;

    m_shadowProgram->enableAttributeArray("qt_Vertex");
    m_shadowProgram->setAttributeBuffer("qt_Vertex", GL_FLOAT, 0, 3, 0);

    m_shadowProgram->setUniformValue("qt_LightViewProjectionMatrix", lightViewProjectionMatrix);

    glDrawElements(GL_TRIANGLES, m_vertexCount, GL_UNSIGNED_INT, nullptr);

    m_indexBuffer->release();
    m_vertexBuffer->release();
    m_shadowProgram->release();
}

void SimpleShaderMeshBinder::renderScene(const QVector3D &eyePosition, const QVector3D &lightDirection,
            const QMatrix4x4 &projectionMatrix, const QMatrix4x4 &viewMatrix,
            const QMatrix4x4 &lightViewMatrix)
{
    if (nullptr == m_sceneProgram) {
        if (!m_openglFunctionsInitialized) {
            QOpenGLFunctions::initializeOpenGLFunctions();
            m_openglFunctionsInitialized = true;
        }

        m_sceneProgram = new QOpenGLShaderProgram;
        m_sceneProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/scene.vert");
        m_sceneProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/scene.frag");
        m_sceneProgram->link();
    }
    
    checkNewMesh();
    
    if (nullptr == m_indexBuffer || 0 == m_shadowMapTextureId) {
        return;
    }
    
    m_sceneProgram->bind();
    m_vertexBuffer->bind();
    m_indexBuffer->bind();

    const QMatrix4x4 modelMatrix = m_sceneMatrix * m_matrix;
    const QMatrix4x4 modelViewMatrix = (viewMatrix * modelMatrix);
    const QMatrix4x4 modelViewProjectionMatrix = projectionMatrix * modelViewMatrix;
    const QMatrix4x4 normalMatrix = modelMatrix.inverted().transposed();

    m_sceneProgram->enableAttributeArray("qt_Vertex");
    m_sceneProgram->setAttributeBuffer("qt_Vertex", GL_FLOAT, 0, 3, 0);

    m_sceneProgram->enableAttributeArray("qt_Normal");
    m_sceneProgram->setAttributeBuffer("qt_Normal", GL_FLOAT, m_normalOffset, 3, 0);

    m_sceneProgram->setUniformValue("qt_ViewMatrix", viewMatrix);
    m_sceneProgram->setUniformValue("qt_NormalMatrix", normalMatrix);
    m_sceneProgram->setUniformValue("qt_ModelMatrix", modelMatrix);
    m_sceneProgram->setUniformValue("qt_ModelViewMatrix", modelViewMatrix);
    m_sceneProgram->setUniformValue("qt_ProjectionMatrix", projectionMatrix);
    m_sceneProgram->setUniformValue("qt_ModelViewProjectionMatrix", modelViewProjectionMatrix);

    m_sceneProgram->setUniformValue("qt_LightMatrix", lightViewMatrix);
    m_sceneProgram->setUniformValue("qt_LightViewMatrix", lightViewMatrix * modelMatrix);
    m_sceneProgram->setUniformValue("qt_LightViewProjectionMatrix", projectionMatrix * lightViewMatrix * modelMatrix);

    if (m_shadowMapTextureId > 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_shadowMapTextureId);
        m_sceneProgram->setUniformValue("qt_ShadowMap", 0);
        m_sceneProgram->setUniformValue("qt_ShadowEnabled", true);
    } else {
        m_sceneProgram->setUniformValue("qt_ShadowEnabled", false);
    }
    
    m_sceneProgram->setUniformValue("qt_Light.ambient", QColor(255, 255, 255));
    m_sceneProgram->setUniformValue("qt_Light.diffuse", QColor(Qt::white));
    m_sceneProgram->setUniformValue("qt_Light.specular", QColor(Qt::white));
    m_sceneProgram->setUniformValue("qt_Light.direction", lightDirection);
    m_sceneProgram->setUniformValue("qt_Light.eye", eyePosition);
    
    QColor ambient = Qt::white;
    ambient.setRgbF(
            ambient.redF() * qreal(0.1f),
            ambient.greenF() * qreal(0.1f),
            ambient.blueF() * qreal(0.1f)
        );

    QColor diffuse = Qt::white;
    diffuse.setRgbF(
            diffuse.redF() * qreal(1.0f),
            diffuse.greenF() * qreal(1.0f),
            diffuse.blueF() * qreal(1.0f)
        );

    QColor specular = Qt::white;

    m_sceneProgram->setUniformValue("qt_Material.ambient", ambient);
    m_sceneProgram->setUniformValue("qt_Material.diffuse", diffuse);
    m_sceneProgram->setUniformValue("qt_Material.specular", specular);
    m_sceneProgram->setUniformValue("qt_Material.specularPower", 0.0f);
    m_sceneProgram->setUniformValue("qt_Material.brightness", 1.0f);
    m_sceneProgram->setUniformValue("qt_Material.opacity", 1.0f);
    
    glDrawElements(GL_TRIANGLES, m_vertexCount, GL_UNSIGNED_INT, nullptr);

    if (m_shadowMapTextureId > 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    m_indexBuffer->release();
    m_vertexBuffer->release();
    m_sceneProgram->release();
}
