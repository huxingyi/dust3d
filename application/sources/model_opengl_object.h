#ifndef DUST3D_APPLICATION_MODEL_OPENGL_OBJECT_H_
#define DUST3D_APPLICATION_MODEL_OPENGL_OBJECT_H_

#include <memory>
#include <QMutex>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include "model.h"

class ModelOpenGLObject
{
public:
    void update(std::unique_ptr<Model> mesh);
    void draw();
private:
    void copyMeshToOpenGL();
    QOpenGLVertexArrayObject m_vertexArrayObject;
    QOpenGLBuffer m_buffer;
    std::unique_ptr<Model> m_mesh;
    bool m_meshIsDirty = false;
    QMutex m_meshMutex;
    int m_meshTriangleVertexCount = 0;
};

#endif
