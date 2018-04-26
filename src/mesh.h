#ifndef MESH_H
#define MESH_H
#include <QObject>
#include <QOpenGLFunctions>
#include <vector>
#include <QVector3D>
#include <QColor>
#include "positionmap.h"
#include "theme.h"

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

struct TriangulatedFace
{
    int indicies[3];
    QColor color;
};

class Mesh
{
public:
    Mesh(void *meshlite, int meshId, int triangulatedMeshId = -1, QColor modelColor=Theme::white, std::vector<QColor> *triangleColors=nullptr);
    ~Mesh();
    Vertex *triangleVertices();
    int triangleVertexCount();
    Vertex *edgeVertices();
    int edgeVertexCount();
    const std::vector<QVector3D> &vertices();
    const std::vector<std::vector<int>> &faces();
    const std::vector<QVector3D> &triangulatedVertices();
    const std::vector<TriangulatedFace> &triangulatedFaces();
private:
    Vertex *m_triangleVertices;
    int m_triangleVertexCount;
    Vertex *m_edgeVertices;
    int m_edgeVertexCount;
    std::vector<QVector3D> m_vertices;
    std::vector<std::vector<int>> m_faces;
    std::vector<QVector3D> m_triangulatedVertices;
    std::vector<TriangulatedFace> m_triangulatedFaces;
};

#endif
