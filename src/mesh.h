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
} Vertex;
#pragma pack(pop)

class Mesh
{
public:
    Mesh(void *meshlite, int mesh_id);
    ~Mesh();
    Vertex *vertices();
    int vertexCount();
private:
    Vertex *m_vertices;
    int m_vertexCount;
};

#endif
