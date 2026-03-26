#ifndef DUST3D_APPLICATION_WORLD_GROUND_OPENGL_VERTEX_H_
#define DUST3D_APPLICATION_WORLD_GROUND_OPENGL_VERTEX_H_

#include <QOpenGLFunctions>

#pragma pack(push)
#pragma pack(1)
typedef struct {
    GLfloat posX;
    GLfloat posY;
    GLfloat posZ;
} WorldGroundOpenGLVertex;
#pragma pack(pop)

#endif
