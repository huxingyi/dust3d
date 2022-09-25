#include <dust3d/base/debug.h>
#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include "monochrome_opengl_object.h"

void MonochromeOpenGLObject::update(std::unique_ptr<MonochromeMesh> mesh)
{
    QMutexLocker lock(&m_meshMutex);
    m_mesh = std::move(mesh);
    m_meshIsDirty = true;
}

void MonochromeOpenGLObject::draw()
{
    copyMeshToOpenGL();
    if (0 == m_meshLineVertexCount && 0 == m_meshTriangleVertexCount)
        return;
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    QOpenGLVertexArrayObject::Binder binder(&m_vertexArrayObject);
    if (m_meshLineVertexCount > 0)
        f->glDrawArrays(GL_LINES, 0, m_meshLineVertexCount);
    if (m_meshTriangleVertexCount > 0)
        f->glDrawArrays(GL_TRIANGLES, m_meshLineVertexCount, m_meshTriangleVertexCount);
}

void MonochromeOpenGLObject::copyMeshToOpenGL()
{
    std::unique_ptr<MonochromeMesh> mesh;
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
    m_meshLineVertexCount = 0;
    m_meshTriangleVertexCount = 0;
    if (mesh) {
        QOpenGLVertexArrayObject::Binder binder(&m_vertexArrayObject);
        if (m_buffer.isCreated())
            m_buffer.destroy();
        m_buffer.create();
        m_buffer.bind();
        m_buffer.allocate(mesh->vertices(), (mesh->lineVertexCount() + mesh->triangleVertexCount()) * sizeof(MonochromeOpenGLVertex));
        m_meshLineVertexCount = mesh->lineVertexCount();
        m_meshTriangleVertexCount = mesh->triangleVertexCount();
        QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
        f->glEnableVertexAttribArray(0);
        f->glEnableVertexAttribArray(1);
        f->glEnableVertexAttribArray(2);
        f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MonochromeOpenGLVertex), 0);
        f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MonochromeOpenGLVertex), reinterpret_cast<void *>(3 * sizeof(GLfloat)));
        f->glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(MonochromeOpenGLVertex), reinterpret_cast<void *>(6 * sizeof(GLfloat)));
        m_buffer.release();
    }
}