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
    GLfloat colorR = 24.0 / 255.0;
    GLfloat colorG = 29.0 / 255.0;
    GLfloat colorB = 49.0 / 255.0;
    GLfloat alpha = 1.0;
} MonochromeOpenGLVertex;
#pragma pack(pop)

#endif
