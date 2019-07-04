#include <QMutexLocker>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <map>
#include <QDebug>
#include <QDir>
#include "modelmeshbinder.h"
#include "ds3file.h"

ModelMeshBinder::ModelMeshBinder() :
    m_mesh(nullptr),
    m_newMesh(nullptr),
    m_renderTriangleVertexCount(0),
    m_renderEdgeVertexCount(0),
    m_newMeshComing(false),
    m_showWireframes(false),
    m_hasTexture(false),
    m_texture(nullptr),
    m_hasNormalMap(false),
    m_normalMap(nullptr),
    m_hasMetalnessMap(false),
    m_hasRoughnessMap(false),
    m_hasAmbientOcclusionMap(false),
    m_metalnessRoughnessAmbientOcclusionMap(nullptr)
{
}

ModelMeshBinder::~ModelMeshBinder()
{
    delete m_mesh;
    delete m_newMesh;
    delete m_texture;
    delete m_normalMap;
    delete m_metalnessRoughnessAmbientOcclusionMap;
}

void ModelMeshBinder::updateMesh(MeshLoader *mesh)
{
    QMutexLocker lock(&m_newMeshMutex);
    if (mesh != m_mesh) {
        delete m_newMesh;
        m_newMesh = mesh;
        m_newMeshComing = true;
    }
}

void ModelMeshBinder::initialize()
{
    m_vaoTriangle.create();
    m_vaoEdge.create();
}

void ModelMeshBinder::paint(ModelShaderProgram *program)
{
    MeshLoader *newMesh = nullptr;
    bool hasNewMesh = false;
    if (m_newMeshComing) {
        QMutexLocker lock(&m_newMeshMutex);
        if (m_newMeshComing) {
            newMesh = m_newMesh;
            m_newMesh = nullptr;
            m_newMeshComing = false;
            hasNewMesh = true;
        }
    }
    {
        QMutexLocker lock(&m_meshMutex);
        if (hasNewMesh) {
            delete m_mesh;
            m_mesh = newMesh;
            if (m_mesh) {
            
                m_hasTexture = nullptr != m_mesh->textureImage();
                delete m_texture;
                m_texture = nullptr;
                if (m_hasTexture)
                    m_texture = new QOpenGLTexture(*m_mesh->textureImage());
                
                m_hasNormalMap = nullptr != m_mesh->normalMapImage();
                delete m_normalMap;
                m_normalMap = nullptr;
                if (m_hasNormalMap)
                    m_normalMap = new QOpenGLTexture(*m_mesh->normalMapImage());
                
                m_hasMetalnessMap = m_mesh->hasMetalnessInImage();
                m_hasRoughnessMap = m_mesh->hasRoughnessInImage();
                m_hasAmbientOcclusionMap = m_mesh->hasAmbientOcclusionInImage();
                delete m_metalnessRoughnessAmbientOcclusionMap;
                m_metalnessRoughnessAmbientOcclusionMap = nullptr;
                if (nullptr != m_mesh->metalnessRoughnessAmbientOcclusionImage() &&
                        (m_hasMetalnessMap || m_hasRoughnessMap || m_hasAmbientOcclusionMap))
                    m_metalnessRoughnessAmbientOcclusionMap = new QOpenGLTexture(*m_mesh->metalnessRoughnessAmbientOcclusionImage());
                    
                {
                    QOpenGLVertexArrayObject::Binder vaoBinder(&m_vaoTriangle);
                    if (m_vboTriangle.isCreated())
                        m_vboTriangle.destroy();
                    m_vboTriangle.create();
                    m_vboTriangle.bind();
                    m_vboTriangle.allocate(m_mesh->triangleVertices(), m_mesh->triangleVertexCount() * sizeof(ShaderVertex));
                    m_renderTriangleVertexCount = m_mesh->triangleVertexCount();
                    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
                    f->glEnableVertexAttribArray(0);
                    f->glEnableVertexAttribArray(1);
                    f->glEnableVertexAttribArray(2);
                    f->glEnableVertexAttribArray(3);
                    f->glEnableVertexAttribArray(4);
                    f->glEnableVertexAttribArray(5);
                    f->glEnableVertexAttribArray(6);
                    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ShaderVertex), 0);
                    f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ShaderVertex), reinterpret_cast<void *>(3 * sizeof(GLfloat)));
                    f->glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(ShaderVertex), reinterpret_cast<void *>(6 * sizeof(GLfloat)));
                    f->glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(ShaderVertex), reinterpret_cast<void *>(9 * sizeof(GLfloat)));
                    f->glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(ShaderVertex), reinterpret_cast<void *>(11 * sizeof(GLfloat)));
                    f->glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(ShaderVertex), reinterpret_cast<void *>(12 * sizeof(GLfloat)));
                    f->glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, sizeof(ShaderVertex), reinterpret_cast<void *>(13 * sizeof(GLfloat)));
                    m_vboTriangle.release();
                }
                {
                    QOpenGLVertexArrayObject::Binder vaoBinder(&m_vaoEdge);
                    if (m_vboEdge.isCreated())
                        m_vboEdge.destroy();
                    m_vboEdge.create();
                    m_vboEdge.bind();
                    m_vboEdge.allocate(m_mesh->edgeVertices(), m_mesh->edgeVertexCount() * sizeof(ShaderVertex));
                    m_renderEdgeVertexCount = m_mesh->edgeVertexCount();
                    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
                    f->glEnableVertexAttribArray(0);
                    f->glEnableVertexAttribArray(1);
                    f->glEnableVertexAttribArray(2);
                    f->glEnableVertexAttribArray(3);
                    f->glEnableVertexAttribArray(4);
                    f->glEnableVertexAttribArray(5);
                    f->glEnableVertexAttribArray(6);
                    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ShaderVertex), 0);
                    f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ShaderVertex), reinterpret_cast<void *>(3 * sizeof(GLfloat)));
                    f->glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(ShaderVertex), reinterpret_cast<void *>(6 * sizeof(GLfloat)));
                    f->glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(ShaderVertex), reinterpret_cast<void *>(9 * sizeof(GLfloat)));
                    f->glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(ShaderVertex), reinterpret_cast<void *>(11 * sizeof(GLfloat)));
                    f->glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(ShaderVertex), reinterpret_cast<void *>(12 * sizeof(GLfloat)));
                    f->glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, sizeof(ShaderVertex), reinterpret_cast<void *>(13 * sizeof(GLfloat)));
                    m_vboEdge.release();
                }
            } else {
                m_renderTriangleVertexCount = 0;
                m_renderEdgeVertexCount = 0;
            }
        }
    }
    
    if (m_showWireframes) {
        if (m_renderEdgeVertexCount > 0) {
            QOpenGLVertexArrayObject::Binder vaoBinder(&m_vaoEdge);
			QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
            program->setUniformValue(program->textureEnabledLoc(), 0);
            program->setUniformValue(program->normalMapEnabledLoc(), 0);
            program->setUniformValue(program->metalnessMapEnabledLoc(), 0);
            program->setUniformValue(program->roughnessMapEnabledLoc(), 0);
            program->setUniformValue(program->ambientOcclusionMapEnabledLoc(), 0);
            f->glDrawArrays(GL_LINES, 0, m_renderEdgeVertexCount);
        }
    }
    if (m_renderTriangleVertexCount > 0) {
        QOpenGLVertexArrayObject::Binder vaoBinder(&m_vaoTriangle);
		QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
        if (m_hasTexture) {
            if (m_texture)
                m_texture->bind(0);
            program->setUniformValue(program->textureIdLoc(), 0);
            program->setUniformValue(program->textureEnabledLoc(), 1);
        } else {
            program->setUniformValue(program->textureEnabledLoc(), 0);
        }
        if (m_hasNormalMap) {
            if (m_normalMap)
                m_normalMap->bind(1);
            program->setUniformValue(program->normalMapIdLoc(), 1);
            program->setUniformValue(program->normalMapEnabledLoc(), 1);
        } else {
            program->setUniformValue(program->normalMapEnabledLoc(), 0);
        }
        if (m_hasMetalnessMap || m_hasRoughnessMap || m_hasAmbientOcclusionMap) {
            if (m_metalnessRoughnessAmbientOcclusionMap)
                m_metalnessRoughnessAmbientOcclusionMap->bind(2);
            program->setUniformValue(program->metalnessRoughnessAmbientOcclusionMapIdLoc(), 2);
        }
        program->setUniformValue(program->metalnessMapEnabledLoc(), m_hasMetalnessMap ? 1 : 0);
        program->setUniformValue(program->roughnessMapEnabledLoc(), m_hasRoughnessMap ? 1 : 0);
        program->setUniformValue(program->ambientOcclusionMapEnabledLoc(), m_hasAmbientOcclusionMap ? 1 : 0);
        f->glDrawArrays(GL_TRIANGLES, 0, m_renderTriangleVertexCount);
    }
}

void ModelMeshBinder::cleanup()
{
    if (m_vboTriangle.isCreated())
        m_vboTriangle.destroy();
    if (m_vboEdge.isCreated())
        m_vboEdge.destroy();
    delete m_texture;
    m_texture = nullptr;
    delete m_normalMap;
    m_normalMap = nullptr;
    delete m_metalnessRoughnessAmbientOcclusionMap;
    m_metalnessRoughnessAmbientOcclusionMap = nullptr;
}

void ModelMeshBinder::showWireframes()
{
    m_showWireframes = true;
}

void ModelMeshBinder::hideWireframes()
{
    m_showWireframes = false;
}

bool ModelMeshBinder::isWireframesVisible()
{
    return m_showWireframes;
}
