#ifndef MODEL_MESH_BINDER_H
#define MODEL_MESH_BINDER_H
#include <QOpenGLVertexArrayObject>
#include <QMutex>
#include <QOpenGLBuffer>
#include <QString>
#include <QOpenGLTexture>
#include "meshloader.h"
#include "modelshaderprogram.h"

class ModelMeshBinder
{
public:
    ModelMeshBinder();
    ~ModelMeshBinder();
    void updateMesh(MeshLoader *mesh);
    void exportMeshAsObj(const QString &filename);
    void exportMeshAsObjPlusMaterials(const QString &filename);
    void initialize();
    void paint(ModelShaderProgram *program);
    void cleanup();
    void showWireframes();
    void hideWireframes();
    bool isWireframesVisible();
private:
    MeshLoader *m_mesh;
    MeshLoader *m_newMesh;
    int m_renderTriangleVertexCount;
    int m_renderEdgeVertexCount;
    bool m_newMeshComing;
    bool m_showWireframes;
    bool m_hasTexture;
    QOpenGLTexture *m_texture;
private:
    QOpenGLVertexArrayObject m_vaoTriangle;
    QOpenGLBuffer m_vboTriangle;
    QOpenGLVertexArrayObject m_vaoEdge;
    QOpenGLBuffer m_vboEdge;
    QMutex m_meshMutex;
    QMutex m_newMeshMutex;
};

#endif
