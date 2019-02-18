#ifndef DUST3D_MESH_LOADER_H
#define DUST3D_MESH_LOADER_H
#include <QObject>
#include <QOpenGLFunctions>
#include <vector>
#include <QVector3D>
#include <QColor>
#include <QImage>
#include "outcome.h"

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
} Vertex;
#pragma pack(pop)

struct TriangulatedFace
{
    int indices[3];
    QColor color;
};

class MeshLoader
{
public:
    MeshLoader(const std::vector<QVector3D> &vertices, const std::vector<std::vector<size_t>> &triangles,
        const std::vector<std::vector<QVector3D>> &triangleVertexNormals,
        const QColor &color=Qt::white);
    MeshLoader(Outcome &outcome);
    MeshLoader(Vertex *triangleVertices, int vertexNum);
    MeshLoader(const MeshLoader &mesh);
    MeshLoader();
    ~MeshLoader();
    Vertex *triangleVertices();
    int triangleVertexCount();
    Vertex *edgeVertices();
    int edgeVertexCount();
    const std::vector<QVector3D> &vertices();
    const std::vector<std::vector<size_t>> &faces();
    const std::vector<QVector3D> &triangulatedVertices();
    const std::vector<TriangulatedFace> &triangulatedFaces();
    void setTextureImage(QImage *textureImage);
    const QImage *textureImage();
    void setNormalMapImage(QImage *normalMapImage);
    const QImage *normalMapImage();
    const QImage *metalnessRoughnessAmbientOcclusionImage();
    void setMetalnessRoughnessAmbientOcclusionImage(QImage *image);
    bool hasMetalnessInImage();
    void setHasMetalnessInImage(bool hasInImage);
    bool hasRoughnessInImage();
    void setHasRoughnessInImage(bool hasInImage);
    bool hasAmbientOcclusionInImage();
    void setHasAmbientOcclusionInImage(bool hasInImage);
    static float m_defaultMetalness;
    static float m_defaultRoughness;
    void exportAsObj(const QString &filename);
private:
    Vertex *m_triangleVertices = nullptr;
    int m_triangleVertexCount = 0;
    Vertex *m_edgeVertices = nullptr;
    int m_edgeVertexCount = 0;
    std::vector<QVector3D> m_vertices;
    std::vector<std::vector<size_t>> m_faces;
    std::vector<QVector3D> m_triangulatedVertices;
    std::vector<TriangulatedFace> m_triangulatedFaces;
    QImage *m_textureImage = nullptr;
    QImage *m_normalMapImage = nullptr;
    QImage *m_metalnessRoughnessAmbientOcclusionImage = nullptr;
    bool m_hasMetalnessInImage = false;
    bool m_hasRoughnessInImage = false;
    bool m_hasAmbientOcclusionInImage = false;
};

#endif
