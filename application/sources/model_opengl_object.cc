#include <dust3d/base/debug.h>
#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include "model_opengl_object.h"

void ModelOpenGLObject::update(std::unique_ptr<Model> mesh)
{
    QMutexLocker lock(&m_meshMutex);
    m_mesh = std::move(mesh);
    m_meshIsDirty = true;
}

void ModelOpenGLObject::draw()
{
    copyMeshToOpenGL();
    if (0 == m_meshTriangleVertexCount)
        return;
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    QOpenGLVertexArrayObject::Binder binder(&m_vertexArrayObject);
    f->glDrawArrays(GL_TRIANGLES, 0, m_meshTriangleVertexCount);
}

void ModelOpenGLObject::copyMeshToOpenGL()
{
    std::unique_ptr<Model> mesh;
    bool meshChanged = false;
    if (m_meshIsDirty) {
        QMutexLocker lock(&m_meshMutex);
        if (m_meshIsDirty) {
            m_meshIsDirty = false;
            meshChanged = true;
            mesh = std::move(m_mesh);
        }
    }
    if (!meshChanged)
        return;
    m_meshTriangleVertexCount = 0;
    if (mesh) {
        QOpenGLVertexArrayObject::Binder binder(&m_vertexArrayObject);
        if (m_buffer.isCreated())
            m_buffer.destroy();
        m_buffer.create();
        m_buffer.bind();
        m_buffer.allocate(mesh->triangleVertices(), mesh->triangleVertexCount() * sizeof(ModelShaderVertex));
        m_meshTriangleVertexCount = mesh->triangleVertexCount();
        QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
        f->glEnableVertexAttribArray(0);
        f->glEnableVertexAttribArray(1);
        f->glEnableVertexAttribArray(2);
        f->glEnableVertexAttribArray(3);
        f->glEnableVertexAttribArray(4);
        f->glEnableVertexAttribArray(5);
        f->glEnableVertexAttribArray(6);
        f->glEnableVertexAttribArray(7);
        f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ModelShaderVertex), 0);
        f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ModelShaderVertex), reinterpret_cast<void *>(3 * sizeof(GLfloat)));
        f->glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(ModelShaderVertex), reinterpret_cast<void *>(6 * sizeof(GLfloat)));
        f->glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(ModelShaderVertex), reinterpret_cast<void *>(9 * sizeof(GLfloat)));
        f->glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(ModelShaderVertex), reinterpret_cast<void *>(11 * sizeof(GLfloat)));
        f->glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(ModelShaderVertex), reinterpret_cast<void *>(12 * sizeof(GLfloat)));
        f->glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, sizeof(ModelShaderVertex), reinterpret_cast<void *>(13 * sizeof(GLfloat)));
        f->glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, sizeof(ModelShaderVertex), reinterpret_cast<void *>(16 * sizeof(GLfloat)));
        m_buffer.release();
    }
}