#ifndef DUST3D_APPLICATION_MONOCHROME_OPENGL_OBJECT_H_
#define DUST3D_APPLICATION_MONOCHROME_OPENGL_OBJECT_H_

#include "monochrome_mesh.h"
#include <QMutex>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <memory>

class MonochromeOpenGLObject {
public:
    void update(std::unique_ptr<MonochromeMesh> mesh);
    void draw();

private:
    void copyMeshToOpenGL();
    QOpenGLVertexArrayObject m_vertexArrayObject;
    QOpenGLBuffer m_buffer;
    std::unique_ptr<MonochromeMesh> m_mesh;
    bool m_meshIsDirty = false;
    QMutex m_meshMutex;
    int m_meshLineVertexCount = 0;
};

#endif
