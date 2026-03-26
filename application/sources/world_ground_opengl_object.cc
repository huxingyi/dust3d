#include "world_ground_opengl_object.h"
#include <QOpenGLContext>
#include <QOpenGLFunctions>

void WorldGroundOpenGLObject::create(float groundY, float extent)
{
    // Two triangles forming a horizontal quad at groundY
    m_vertices = {
        { -extent, groundY, -extent },
        { extent, groundY, -extent },
        { extent, groundY, extent },
        { -extent, groundY, -extent },
        { extent, groundY, extent },
        { -extent, groundY, extent },
    };
    m_isPrepared = false;
}

void WorldGroundOpenGLObject::uploadToOpenGL()
{
    if (m_isPrepared || m_vertices.empty())
        return;
    m_isPrepared = true;

    QOpenGLVertexArrayObject::Binder binder(&m_vertexArrayObject);
    if (m_buffer.isCreated())
        m_buffer.destroy();
    m_buffer.create();
    m_buffer.bind();
    m_buffer.allocate(m_vertices.data(), (int)(m_vertices.size() * sizeof(WorldGroundOpenGLVertex)));
    m_vertexCount = (int)m_vertices.size();

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(WorldGroundOpenGLVertex), 0);
    m_buffer.release();
}

void WorldGroundOpenGLObject::draw()
{
    uploadToOpenGL();
    if (0 == m_vertexCount)
        return;
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    QOpenGLVertexArrayObject::Binder binder(&m_vertexArrayObject);
    f->glDrawArrays(GL_TRIANGLES, 0, m_vertexCount);
}
