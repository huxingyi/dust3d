#ifndef MESH_H
#define MESH_H
#include <QObject>
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
} Vertex;
#pragma pack(pop)

class Mesh
{
public:
    Mesh(void *meshlite, int meshId);
    ~Mesh();
    Vertex *triangleVertices();
    int triangleVertexCount();
    Vertex *edgeVertices();
    int edgeVertexCount();
private:
    Vertex *m_triangleVertices;
    int m_triangleVertexCount;
    Vertex *m_edgeVertices;
    int m_edgeVertexCount;
};

#endif
