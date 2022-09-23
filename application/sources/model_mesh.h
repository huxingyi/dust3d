#ifndef DUST3D_APPLICATION_MODEL_MESH_H_
#define DUST3D_APPLICATION_MODEL_MESH_H_

#include <QObject>
#include <vector>
#include <QImage>
#include <QTextStream>
#include <dust3d/base/vector3.h>
#include <dust3d/base/color.h>
#include <dust3d/base/object.h>
#include "model_opengl_vertex.h"

class ModelMesh
{
public:
    ModelMesh(const std::vector<dust3d::Vector3> &vertices, const std::vector<std::vector<size_t>> &triangles,
        const std::vector<std::vector<dust3d::Vector3>> &triangleVertexNormals,
        const dust3d::Color &color=dust3d::Color::createWhite(),
        float metalness=0.0,
        float roughness=0.0);
    ModelMesh(dust3d::Object &object);
    ModelMesh(ModelOpenGLVertex *triangleVertices, int vertexNum);
    ModelMesh(const ModelMesh &mesh);
    ModelMesh();
    ~ModelMesh();
    ModelOpenGLVertex *triangleVertices();
    int triangleVertexCount();
    ModelOpenGLVertex *edgeVertices();
    int edgeVertexCount();
    ModelOpenGLVertex *toolVertices();
    int toolVertexCount();
    const std::vector<dust3d::Vector3> &vertices();
    const std::vector<std::vector<size_t>> &faces();
    const std::vector<dust3d::Vector3> &triangulatedVertices();
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
    void updateTriangleVertices(ModelOpenGLVertex *triangleVertices, int triangleVertexCount);
    quint64 meshId() const;
    void setMeshId(quint64 id);
    void removeColor();
private:
    ModelOpenGLVertex *m_triangleVertices = nullptr;
    int m_triangleVertexCount = 0;
    std::vector<dust3d::Vector3> m_vertices;
    std::vector<std::vector<size_t>> m_faces;
    std::vector<dust3d::Vector3> m_triangulatedVertices;
    QImage *m_textureImage = nullptr;
    QImage *m_normalMapImage = nullptr;
    QImage *m_metalnessRoughnessAmbientOcclusionImage = nullptr;
    bool m_hasMetalnessInImage = false;
    bool m_hasRoughnessInImage = false;
    bool m_hasAmbientOcclusionInImage = false;
    quint64 m_meshId = 0;
};

#endif
