#ifndef MESH_LOADER_H
#define MESH_LOADER_H
#include <QObject>
#include <QOpenGLFunctions>
#include <vector>
#include <QVector3D>
#include <QColor>
#include <QImage>
#include "positionmap.h"
#include "theme.h"
#include "qtlightmapper.h"
#include "meshresultcontext.h"

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
} Vertex;
#pragma pack(pop)

struct TriangulatedFace
{
    int indicies[3];
    QColor color;
};

class MeshLoader
{
public:
    MeshLoader(void *meshlite, int meshId, int triangulatedMeshId = -1, QColor modelColor=Theme::white, const std::vector<QColor> *triangleColors=nullptr);
    MeshLoader(MeshResultContext &resultContext);
    MeshLoader(Vertex *triangleVertices, int vertexNum);
    MeshLoader(const MeshLoader &mesh);
    ~MeshLoader();
    Vertex *triangleVertices();
    int triangleVertexCount();
    Vertex *edgeVertices();
    int edgeVertexCount();
    const std::vector<QVector3D> &vertices();
    const std::vector<std::vector<int>> &faces();
    const std::vector<QVector3D> &triangulatedVertices();
    const std::vector<TriangulatedFace> &triangulatedFaces();
    void setTextureImage(QImage *textureImage);
    const QImage *textureImage();
private:
    Vertex *m_triangleVertices;
    int m_triangleVertexCount;
    Vertex *m_edgeVertices;
    int m_edgeVertexCount;
    std::vector<QVector3D> m_vertices;
    std::vector<std::vector<int>> m_faces;
    std::vector<QVector3D> m_triangulatedVertices;
    std::vector<TriangulatedFace> m_triangulatedFaces;
    QImage *m_textureImage;
};

#endif
