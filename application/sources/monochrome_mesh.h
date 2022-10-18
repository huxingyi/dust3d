#ifndef DUST3D_APPLICATION_MONOCHROME_MESH_H_
#define DUST3D_APPLICATION_MONOCHROME_MESH_H_

#include "monochrome_opengl_vertex.h"
#include <dust3d/base/object.h>
#include <memory>

class MonochromeMesh {
public:
    MonochromeMesh(const MonochromeMesh& mesh);
    MonochromeMesh(MonochromeMesh&& mesh);
    MonochromeMesh(const dust3d::Object& object);
    const MonochromeOpenGLVertex* lineVertices();
    int lineVertexCount();

private:
    std::vector<MonochromeOpenGLVertex> m_lineVertices;
};

#endif
