#ifndef DUST3D_APPLICATION_MODEL_OPENGL_VERTEX_H_
#define DUST3D_APPLICATION_MODEL_OPENGL_VERTEX_H_

#include <QOpenGLFunctions>

#pragma pack(push)
#pragma pack(1)
typedef struct
{
    GLfloat posX;
    GLfloat posY;
    GLfloat posZ;
    GLfloat normX;
    GLfloat normY;
    GLfloat normZ;
    GLfloat colorR;
    GLfloat colorG;
    GLfloat colorB;
    GLfloat texU;
    GLfloat texV;
    GLfloat metalness;
    GLfloat roughness;
    GLfloat tangentX;
    GLfloat tangentY;
    GLfloat tangentZ;
    GLfloat alpha = 1.0;
} ModelOpenGLVertex;
#pragma pack(pop)

#endif
