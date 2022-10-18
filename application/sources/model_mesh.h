#ifndef DUST3D_APPLICATION_MODEL_MESH_H_
#define DUST3D_APPLICATION_MODEL_MESH_H_

#include "model_opengl_vertex.h"
#include <QImage>
#include <QObject>
#include <QTextStream>
#include <dust3d/base/color.h>
#include <dust3d/base/object.h>
#include <dust3d/base/vector3.h>
#include <vector>

class ModelMesh {
public:
    ModelMesh(const std::vector<dust3d::Vector3>& vertices,
        const std::vector<std::vector<size_t>>& triangles,
        const std::vector<std::vector<dust3d::Vector3>>& triangleVertexNormals,
        const dust3d::Color& color = dust3d::Color::createWhite(),
        float metalness = 0.0,
        float roughness = 1.0,
        const std::vector<std::tuple<dust3d::Color, float /*metalness*/, float /*roughness*/>>* vertexProperties = nullptr);
    ModelMesh(dust3d::Object& object);
    ModelMesh(ModelOpenGLVertex* triangleVertices, int vertexNum);
    ModelMesh(const ModelMesh& mesh);
    ModelMesh();
    ~ModelMesh();
    ModelOpenGLVertex* triangleVertices();
    int triangleVertexCount();
    const std::vector<dust3d::Vector3>& vertices();
    const std::vector<std::vector<size_t>>& faces();
    const std::vector<dust3d::Vector3>& triangulatedVertices();
    void setTextureImage(QImage* textureImage);
    const QImage* textureImage();
    QImage* takeTextureImage();
    void setNormalMapImage(QImage* normalMapImage);
    const QImage* normalMapImage();
    QImage* takeNormalMapImage();
    const QImage* metalnessRoughnessAmbientOcclusionMapImage();
    QImage* takeMetalnessRoughnessAmbientOcclusionMapImage();
    void setMetalnessRoughnessAmbientOcclusionMapImage(QImage* image);
    bool hasMetalnessInImage();
    void setHasMetalnessInImage(bool hasInImage);
    bool hasRoughnessInImage();
    void setHasRoughnessInImage(bool hasInImage);
    bool hasAmbientOcclusionInImage();
    void setHasAmbientOcclusionInImage(bool hasInImage);
    static float m_defaultMetalness;
    static float m_defaultRoughness;
    void exportAsObj(const QString& filename);
    void exportAsObj(QTextStream* textStream);
    void updateTriangleVertices(ModelOpenGLVertex* triangleVertices, int triangleVertexCount);
    quint64 meshId() const;
    void setMeshId(quint64 id);
    void removeColor();

private:
    ModelOpenGLVertex* m_triangleVertices = nullptr;
    int m_triangleVertexCount = 0;
    std::vector<dust3d::Vector3> m_vertices;
    std::vector<std::vector<size_t>> m_faces;
    std::vector<dust3d::Vector3> m_triangulatedVertices;
    QImage* m_textureImage = nullptr;
    QImage* m_normalMapImage = nullptr;
    QImage* m_metalnessRoughnessAmbientOcclusionMapImage = nullptr;
    bool m_hasMetalnessInImage = false;
    bool m_hasRoughnessInImage = false;
    bool m_hasAmbientOcclusionInImage = false;
    quint64 m_meshId = 0;
};

#endif
