#ifndef DUST3D_APPLICATION_MONOCHROME_OPENGL_VERTEX_H_
#define DUST3D_APPLICATION_MONOCHROME_OPENGL_VERTEX_H_

#include <QOpenGLFunctions>

#pragma pack(push)
#pragma pack(1)
typedef struct
{
    GLfloat posX;
    GLfloat posY;
    GLfloat posZ;
    GLfloat colorR = 1.0;
    GLfloat colorG = 1.0;
    GLfloat colorB = 1.0;
    GLfloat alpha = 1.0;
} MonochromeOpenGLVertex;
#pragma pack(pop)

#endif
