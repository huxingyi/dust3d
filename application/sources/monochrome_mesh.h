#ifndef DUST3D_APPLICATION_MONOCHROME_MESH_H_
#define DUST3D_APPLICATION_MONOCHROME_MESH_H_

#include "model_opengl_vertex.h"
#include "monochrome_opengl_vertex.h"
#include <dust3d/base/object.h>
#include <memory>

class MonochromeMesh {
public:
    MonochromeMesh(const MonochromeMesh& mesh);
    MonochromeMesh(MonochromeMesh&& mesh);
    MonochromeMesh(const dust3d::Object& object);
    MonochromeMesh(const ModelOpenGLVertex* triangleVertices, int vertexCount,
        float r = 0.15f, float g = 0.15f, float b = 0.18f, float a = 0.9f);
    const MonochromeOpenGLVertex* lineVertices();
    int lineVertexCount();

private:
    std::vector<MonochromeOpenGLVertex> m_lineVertices;
};

#endif
