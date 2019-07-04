#ifndef DUST3D_SHADER_VERTEX_H
#define DUST3D_SHADER_VERTEX_H
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
} ShaderVertex;
#pragma pack(pop)

#endif
