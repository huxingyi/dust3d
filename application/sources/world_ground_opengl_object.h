#ifndef DUST3D_APPLICATION_WORLD_GROUND_OPENGL_OBJECT_H_
#define DUST3D_APPLICATION_WORLD_GROUND_OPENGL_OBJECT_H_

#include "world_ground_opengl_vertex.h"
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <vector>

class WorldGroundOpenGLObject {
public:
    void create(float groundY = -1.0f, float extent = 5.0f);
    void draw();

private:
    void uploadToOpenGL();

    QOpenGLVertexArrayObject m_vertexArrayObject;
    QOpenGLBuffer m_buffer;
    std::vector<WorldGroundOpenGLVertex> m_vertices;
    int m_vertexCount = 0;
    bool m_isPrepared = false;
};

#endif
