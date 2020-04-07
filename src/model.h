#ifndef DUST3D_MODEL_H
#define DUST3D_MODEL_H
#include <QObject>
#include <vector>
#include <QVector3D>
#include <QColor>
#include <QImage>
#include <QTextStream>
#include "outcome.h"
#include "shadervertex.h"

struct TriangulatedFace
{
    int indices[3];
    QColor color;
};

class Model
{
public:
    Model(const std::vector<QVector3D> &vertices, const std::vector<std::vector<size_t>> &triangles,
        const std::vector<std::vector<QVector3D>> &triangleVertexNormals,
        const QColor &color=Qt::white);
    Model(Outcome &outcome);
    Model(ShaderVertex *triangleVertices, int vertexNum, ShaderVertex *edgeVertices=nullptr, int edgeVertexCount=0);
    Model(const Model &mesh);
    Model();
    ~Model();
    ShaderVertex *triangleVertices();
    int triangleVertexCount();
    ShaderVertex *edgeVertices();
    int edgeVertexCount();
    ShaderVertex *toolVertices();
    int toolVertexCount();
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
    void exportAsObj(QTextStream *textStream);
    void updateTool(ShaderVertex *toolVertices, int vertexNum);
    void updateEdges(ShaderVertex *edgeVertices, int edgeVertexCount);
    void updateTriangleVertices(ShaderVertex *triangleVertices, int triangleVertexCount);
    quint64 meshId() const;
    void setMeshId(quint64 id);
    void removeColor();
private:
    ShaderVertex *m_triangleVertices = nullptr;
    int m_triangleVertexCount = 0;
    ShaderVertex *m_edgeVertices = nullptr;
    int m_edgeVertexCount = 0;
    ShaderVertex *m_toolVertices = nullptr;
    int m_toolVertexCount = 0;
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
    quint64 m_meshId = 0;
};

#endif
